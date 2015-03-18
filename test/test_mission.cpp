#include "../src/player.hpp"
#include "../src/local_event_dispatcher.hpp"
#include "test_services.hpp"

#include <yarrr/object_factory.hpp>
#include <yarrr/engine_dispatcher.hpp>
#include <yarrr/mission_factory.hpp>
#include <yarrr/mission_exporter.hpp>
#include <yarrr/command.hpp>
#include <yarrr/destruction_handlers.hpp>
#include <yarrr/object_container.hpp>
#include <yarrr/main_thread_callback_queue.hpp>
#include <yarrr/delete_object.hpp>
#include <yarrr/chat_message.hpp>
#include <thectci/service_registry.hpp>
#include <igloo/igloo_alt.h>

using namespace igloo;

Describe( a_mission_command_handler )
{

  void add_ship_factory()
  {
    the::ctci::service< yarrr::ObjectFactory >().register_creator(
        "ship", [ this ]()
        {
          auto new_ship( yarrr::Object::create() );
          player_ship_id = new_ship->id();
          return new_ship;
        } );
  }

  void create_world()
  {
    add_ship_factory();
  }


  void check_help_message()
  {
    AssertThat( connection->has_entity< yarrr::ChatMessage >(), Equals( true ) );
    const auto message( connection->get_entity< yarrr::ChatMessage >()->message() );
    AssertThat( message, Contains( "/mission list" ) );
    AssertThat( message, Contains( "/mission request <mission name>" ) );
  }

  void log_in_player()
  {
    player_bundle = services->log_in_player( "Kilgore Trout" );
    connection = &player_bundle->connection;
    check_help_message();
    connection->flush_connection();
  }

  void set_up_mission_factory()
  {
    was_mission_created = false;
    the::ctci::service< yarrr::MissionFactory >().register_creator(
        yarrr::Mission::Info( mission_name, "" ),
        [ this ]()
        {
          yarrr::Mission::Pointer new_mission( new yarrr::Mission() );
          last_mission_id = std::to_string( new_mission->id() );
          was_mission_created = true;
          return new_mission;
        } );
  }

  void SetUp()
  {
    services = std::make_unique< test::Services >();
    create_world();
    log_in_player();
    set_up_mission_factory();
  }

  It ( creates_the_requested_mission )
  {
    connection->wrapper.dispatch( mission_request );
    AssertThat( was_mission_created, Equals( true ) );
  }

  It ( starts_the_created_mission )
  {
    connection->wrapper.dispatch( mission_request );
    AssertThat( services->lua.assert_that(
        the::model::index_lua_table( yarrr::mission_contexts, last_mission_id ) ),
        Equals( true ) );
  }

  It ( marks_killed_players_missions_failed )
  {
    connection->wrapper.dispatch( mission_request );
    services->engine_dispatcher.dispatch( yarrr::PlayerKilled( player_ship_id ) );
    AssertThat( services->lua.assert_that(
        the::model::index_lua_table( yarrr::mission_contexts, last_mission_id ) ),
        Equals( false ) );
  }

  It ( sends_error_message_if_the_mission_is_unknown )
  {
    connection->wrapper.dispatch( mission_request_with_unknown_mission_name );
    AssertThat( connection->has_entity< yarrr::ChatMessage >(), Equals( true ) );
    AssertThat( connection->get_entity< yarrr::ChatMessage >()->message(), Contains( "Unknown mission type" ) );
    AssertThat( connection->get_entity< yarrr::ChatMessage >()->message(), Contains( unknown_mission_name ) );
  }

  It ( sends_error_message_if_mission_name_misses )
  {
    connection->wrapper.dispatch( mission_request_without_mission_name );
    AssertThat( connection->has_entity< yarrr::ChatMessage >(), Equals( true ) );
    AssertThat( connection->get_entity< yarrr::ChatMessage >()->message(), Contains( "Invalid mission request." ) );
  }

  It ( sends_error_message_to_unknown_subcommand )
  {
    connection->wrapper.dispatch( mission_command_with_unknown_subcommand );
    AssertThat( connection->has_entity< yarrr::ChatMessage >(), Equals( true ) );
    AssertThat( connection->get_entity< yarrr::ChatMessage >()->message(), Contains( "Unknown subcommand." ) );
  }

  It ( sends_error_message_to_mission_subcommand )
  {
    connection->wrapper.dispatch( mission_command_without_subcommand );
    AssertThat( connection->has_entity< yarrr::ChatMessage >(), Equals( true ) );
    AssertThat( connection->get_entity< yarrr::ChatMessage >()->message(), Contains( "Invalid mission command." ) );
  }

  It ( can_send_the_list_of_registered_missions )
  {
    connection->wrapper.dispatch( mission_list_command );
    AssertThat( connection->has_entity< yarrr::ChatMessage >(), Equals( true ) );

    const auto message( connection->get_entity< yarrr::ChatMessage >()->message() );
    AssertThat( message, Contains( "Registered missions:" ) );
    AssertThat( message, Contains( mission_name ) );
  }

  std::unique_ptr< test::Services::PlayerBundle > player_bundle;
  test::Connection* connection;

  const std::string mission_name{ "test_mission" };
  const yarrr::Command mission_request{ { "mission", "request", mission_name } };

  const std::string unknown_mission_name{ "unknown_mission" };
  const yarrr::Command mission_request_with_unknown_mission_name{ { "mission", "request", unknown_mission_name } };
  const yarrr::Command mission_request_without_mission_name{ { "mission", "request" } };
  const yarrr::Command mission_command_with_unknown_subcommand{ { "mission", "unknown subcommand", "sldkfj" } };
  const yarrr::Command mission_command_without_subcommand{ { "mission", } };
  const yarrr::Command mission_list_command{ { "mission", "list" } };

  std::string last_mission_id;
  bool was_mission_created;
  yarrr::Object::Id player_ship_id;
  std::unique_ptr< test::Services > services;
};

