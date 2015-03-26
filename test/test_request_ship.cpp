#include "../src/world.hpp"
#include "../src/player.hpp"
#include "../src/local_event_dispatcher.hpp"

#include <yarrr/object_factory.hpp>
#include <yarrr/command.hpp>
#include <yarrr/destruction_handlers.hpp>
#include <yarrr/object_container.hpp>
#include <yarrr/main_thread_callback_queue.hpp>
#include <yarrr/delete_object.hpp>
#include <yarrr/chat_message.hpp>
#include <thectci/service_registry.hpp>
#include <igloo/igloo_alt.h>
#include <yarrr/test_connection.hpp>
#include "test_protocol.hpp"
#include "test_services.hpp"

using namespace igloo;

namespace
{

class DummyShipCreator
{
  public:
    DummyShipCreator(
        const std::string& type,
        std::string& type_spy,
        yarrr::Object::Id& id_spy )
      : type( type )
      , type_spy( &type_spy )
      , id_spy( &id_spy )
    {
    }

    yarrr::Object::Pointer operator()()
    {
      yarrr::Object::Pointer new_object( new yarrr::Object() );
      *type_spy = type;
      *id_spy = new_object->id();
      return new_object;
    }

    const std::string type;
    std::string* type_spy;
    yarrr::Object::Id* id_spy;
};

}

Describe( a_ship_request )
{

  void add_ship_factory( const std::string& type )
  {
    the::ctci::service< yarrr::ObjectFactory >().register_creator(
        type,
        DummyShipCreator( type, last_objects_type, last_objects_id ) );
  }

  void check_help_message()
  {
    AssertThat( connection->has_entity< yarrr::ChatMessage >(), Equals( true ) );
    const auto message( connection->get_entity< yarrr::ChatMessage >()->message() );
    AssertThat( message, Contains( "/ship list" ) );
    AssertThat( message, Contains( "/ship request <object type>" ) );
  }


  void SetUp()
  {
    services = std::make_unique< test::Services >();
    add_ship_factory( basic_ship );
    add_ship_factory( giant_ship );

    last_objects_id = 0;
    last_objects_type = "";
    player_bundle = services->log_in_player( "Kilgore Trout" );
    connection = &player_bundle->connection;
    connection_id = connection->connection->id;
    check_help_message();
    connection->flush_connection();
    player = &player_bundle->player;

    AssertThat( last_objects_id, Equals( player->object_id() ) );
  }

  It ( creates_a_ship_of_the_requested_type )
  {
    connection->wrapper.dispatch( giant_request );
    AssertThat( last_objects_type, Equals( giant_ship ) );
  }

  It ( adds_the_new_object_to_the_container )
  {
    connection->wrapper.dispatch( giant_request );
    AssertThat( services->objects.has_object_with_id( last_objects_id ), Equals( true ) );
  }


  It ( deletes_the_old_ship_from_the_container )
  {
    const yarrr::Object::Id old_ship_id( player->object_id() );
    connection->wrapper.dispatch( giant_request );
    AssertThat( services->objects.has_object_with_id( old_ship_id ), Equals( false ) );
    AssertThat( connection->has_entity< yarrr::DeleteObject >(), Equals( true ) );
    AssertThat( connection->get_entity< yarrr::DeleteObject >()->object_id(), Equals( old_ship_id ) );
  }

  It ( assigns_the_created_ship_to_the_player )
  {
    connection->wrapper.dispatch( giant_request );
    AssertThat( connection->has_entity< yarrr::Command >(), Equals( true ) );

    test::assert_object_assigned( *connection, last_objects_id );
  }

  It ( does_not_change_anything_if_requested_ship_type_is_unknown )
  {
    const yarrr::Object::Id old_ship_id( player->object_id() );
    connection->flush_connection();
    connection->wrapper.dispatch( unknown_ship_request );
    AssertThat( services->objects.has_object_with_id( old_ship_id ), Equals( true ) );
    AssertThat( player->object_id(), Equals( old_ship_id ) );
    AssertThat( connection->get_entity< yarrr::ChatMessage >()->message(), Contains( "Unknown ship type" ) );
    AssertThat( connection->get_entity< yarrr::ChatMessage >()->message(), Contains( unknown_ship_name ) );
  }

  It ( does_not_change_anything_if_theres_no_ship_type )
  {
    const yarrr::Object::Id old_ship_id( player->object_id() );
    connection->wrapper.dispatch( ship_request_without_type );
    AssertThat( services->objects.has_object_with_id( old_ship_id ), Equals( true ) );
    AssertThat( player->object_id(), Equals( old_ship_id ) );
    AssertThat( connection->get_entity< yarrr::ChatMessage >()->message(), Contains( "Please define ship type." ) );
  }

  It ( does_not_change_anything_if_theres_no_subcommand )
  {
    const yarrr::Object::Id old_ship_id( player->object_id() );
    connection->wrapper.dispatch( ship_command_without_subcommand );
    AssertThat( services->objects.has_object_with_id( old_ship_id ), Equals( true ) );
    AssertThat( player->object_id(), Equals( old_ship_id ) );
    AssertThat( connection->get_entity< yarrr::ChatMessage >()->message(), Contains( "Invalid ship command." ) );
  }

  It ( can_send_a_list_of_the_registered_object_types )
  {
    connection->wrapper.dispatch( list_command );
    const std::string message{ connection->get_entity< yarrr::ChatMessage >()->message() };
    AssertThat( message, Contains( "Registered object types:" ) );
    AssertThat( message, Contains( giant_ship ) );
    AssertThat( message, Contains( basic_ship ) );
  }

  std::unique_ptr< test::Services::PlayerBundle > player_bundle;
  test::Connection* connection;
  int connection_id;
  yarrrs::Player* player;

  std::string last_objects_type;
  yarrr::Object::Id last_objects_id;
  const std::string basic_ship{ "ship" };
  const std::string giant_ship{ "giant" };

  const yarrr::Command giant_request{ { "ship", "request", giant_ship } };
  const std::string unknown_ship_name{ "blabla unknown ship type" };
  const yarrr::Command unknown_ship_request{ { "ship", "request", unknown_ship_name } };
  const yarrr::Command ship_request_without_type{ { "ship", "request" } };
  const yarrr::Command ship_command_without_subcommand{ { "ship" } };

  const yarrr::Command list_command{ { "ship", "list" } };
  std::unique_ptr< test::Services > services;
};

