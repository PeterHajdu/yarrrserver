#include "../src/player.hpp"
#include "../src/world.hpp"
#include "../src/command_handler.hpp"
#include <yarrr/protocol.hpp>
#include <yarrr/object_container.hpp>
#include <yarrr/chat_message.hpp>
#include <yarrr/command.hpp>
#include <yarrr/mission.hpp>
#include <yarrr/mission_exporter.hpp>
#include <igloo/igloo_alt.h>
#include <yarrr/test_connection.hpp>
#include "test_services.hpp"
#include "test_protocol.hpp"

using namespace igloo;

Describe( a_player )
{
  void set_up_command_handler()
  {
    command_result = std::make_tuple( true, "success" );
    dispatched_player = nullptr;
    dispatched_command = nullptr;
    services->command_handler.register_handler( command_name,
        [ this ]
        ( const yarrr::Command& command, yarrrs::Player& player ) -> yarrrs::CommandHandler::Result
        {
          dispatched_command = &command;
          dispatched_player = &player;
          return command_result;
        } );
  }

  void set_up_mission()
  {
    should_finish_mission = false;
    mission.reset( new yarrr::Mission() );
    mission->add_objective( yarrr::Mission::Objective( "",
          [ this ]( yarrr::Mission& ) -> yarrr::TaskState
          {
              return should_finish_mission ?
                yarrr::succeeded :
                yarrr::ongoing;
          } ) );
    mission_id = mission->id();
    player->player.start_mission( std::move( mission ) );
  }

  void finish_mission()
  {
    should_finish_mission = true;
    player->player.update_missions();
  }

  void SetUp()
  {
    services = std::make_unique< test::Services >();
    set_up_command_handler();
    player = services->create_player( player_name );
    another_player = services->create_player( player_name );

    services->players[ another_player->connection.connection->id ] = another_player->take_player_ownership();

    ship.reset( new yarrr::Object() );
    was_command_dispatched_to_ship = false;
    ship->dispatcher.register_listener< yarrr::Command >(
        [ this ]( const yarrr::Command& ){ was_command_dispatched_to_ship = true; } );
    player->player.assign_object( *ship );

    set_up_mission();
  }

  void TearDown()
  {
    player.reset();
  }

  It ( broadcasts_chat_messages )
  {
    player->connection.wrapper.dispatch( yarrr::ChatMessage( "", "" ) );
    AssertThat( another_player->connection.has_no_data(), Equals( false ) );
  }

  It ( sends_object_assigned_to_the_player )
  {
    AssertThat( player->player.object_id(), Equals( ship->id() ) );
    AssertThat( player->connection.has_no_data(), Equals( false ) );

    test::assert_object_assigned( player->connection, ship->id() );
  }

  It ( forwards_commands_from_the_network_to_the_object )
  {
    player->connection.wrapper.dispatch( yarrr::Command() );
    AssertThat( was_command_dispatched_to_ship, Equals( true ) );
  }

  It ( forwards_commands_only_to_the_last_assigned_object )
  {
    yarrr::Object new_ship;
    player->player.assign_object( new_ship );
    player->connection.wrapper.dispatch( yarrr::Command() );
    AssertThat( was_command_dispatched_to_ship, Equals( false ) );
  }

  It( stores_and_exports_started_missions )
  {
    const auto mission_context_path( the::model::index_lua_table( yarrr::mission_contexts, std::to_string( mission_id ) ) );
    AssertThat(
        services->lua.assert_that( mission_context_path ),
        Equals( true ) );
  }

  It( exports_the_current_object_id )
  {
    AssertThat( services->lua.assert_equals( the::model::path_from( {
          the::model::index_lua_table( yarrr::mission_contexts, std::to_string( mission_id ) ),
          "character",
          "object_id" } ), std::to_string( ship->id() ) ), Equals( true ) );
  }

  It( removes_finished_mission_objects )
  {
    finish_mission();
    AssertThat( services->lua.assert_that(
          the::model::index_lua_table( yarrr::mission_contexts, std::to_string( mission_id ) ) ),
          Equals( false ) );
  }

  It( exports_player_model )
  {
    AssertThat( services->lua.assert_that(
          the::model::index_lua_table( yarrrs::Models::players_path, player_name ) ),
          Equals( true ) );
  }

  It( sends_mission_object_to_the_player_when_updated )
  {
    player->player.update_missions();
    AssertThat( player->connection.get_entity< yarrr::Mission >()->id(), Equals( mission_id ) );
  }

  It( sends_out_the_finished_state_of_the_mission )
  {
    player->connection.flush_connection();
    finish_mission();
    AssertThat( player->connection.has_entity< yarrr::Mission >(), Equals( true ) );
    AssertThat( player->connection.get_entity< yarrr::Mission >()->state(), Equals( yarrr::succeeded ) );
  }

  It( refreshes_exported_object_id_on_missions_when_new_object_is_assigned )
  {
    yarrr::Object new_ship;
    player->player.assign_object( new_ship );
    player->player.update_missions();
    AssertThat( services->lua.assert_equals( the::model::path_from( {
          the::model::index_lua_table( yarrr::mission_contexts, std::to_string( mission_id ) ),
          "character",
          "object_id" } ), std::to_string( new_ship.id() ) ), Equals( true ) );
  }

  It ( executes_commands_with_the_command_handler )
  {
    player->connection.wrapper.dispatch( command );
    AssertThat( dispatched_command, Equals( &command ) );
    AssertThat( dispatched_player, Equals( &player->player ) );
  }

  It ( sends_no_chat_message_for_a_successfull_command_execution )
  {
    player->connection.flush_connection();
    player->connection.wrapper.dispatch( command );
    AssertThat( player->connection.has_entity< yarrr::ChatMessage >(), Equals( false ) );
  }

  It ( sends_a_chat_message_for_a_failed_command_containing_the_error_message )
  {
    const std::string the_error_message( "an error message" );
    command_result = std::make_tuple( false, the_error_message );
    player->connection.flush_connection();
    player->connection.wrapper.dispatch( command );
    AssertThat( player->connection.get_entity< yarrr::ChatMessage >()->message(), Contains( "failed" ) );
    AssertThat( player->connection.get_entity< yarrr::ChatMessage >()->message(), Contains( command_name ) );
    AssertThat( player->connection.get_entity< yarrr::ChatMessage >()->message(), Contains( the_error_message ) );
  }

  const std::string command_name{ "some strange command" };
  const yarrr::Command command{ { command_name } };
  yarrrs::Player* dispatched_player{ nullptr };
  const yarrr::Command* dispatched_command{ nullptr };
  yarrrs::CommandHandler::Result command_result;

  std::unique_ptr< test::Services::PlayerBundle > player;
  std::unique_ptr< test::Services::PlayerBundle > another_player;

  bool was_command_dispatched_to_ship;
  yarrr::Object::Pointer ship;

  const std::string player_name{ "Kilgor Trout" };

  yarrr::Mission::Pointer mission;
  yarrr::Mission::Id mission_id;

  bool should_finish_mission;
  std::unique_ptr< test::Services > services;
};

