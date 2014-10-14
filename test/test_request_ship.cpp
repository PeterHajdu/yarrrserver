#include "../src/world.hpp"
#include "../src/player.hpp"
#include "../src/object_factory.hpp"
#include "../src/local_event_dispatcher.hpp"
#include "test_connection.hpp"
#include <yarrr/command.hpp>
#include <yarrr/destruction_handlers.hpp>
#include <yarrr/object_container.hpp>
#include <yarrr/main_thread_callback_queue.hpp>
#include <yarrr/delete_object.hpp>
#include <thectci/service_registry.hpp>
#include <igloo/igloo_alt.h>

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
      *id_spy = new_object->id;
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
    the::ctci::service< yarrrs::ObjectFactory >().register_creator(
        type,
        DummyShipCreator( type, last_objects_type, last_objects_id ) );
  }

  void SetUp()
  {
    the::ctci::service< LocalEventDispatcher >().dispatcher.clear();
    players.clear();
    objects.reset( new yarrr::ObjectContainer() );
    connection.reset( new test::Connection() );
    connection_id = connection->connection.id;

    world.reset( new yarrrs::World( players, *objects ) );
    last_objects_id = 0;
    last_objects_type = "";
    add_ship_factory( basic_ship );
    add_ship_factory( giant_ship );

    the::ctci::service< LocalEventDispatcher >().dispatcher.dispatch( yarrrs::PlayerLoggedIn(
          connection->wrapper, connection->connection.id, player_name ) );

    the::ctci::service< yarrr::MainThreadCallbackQueue >().process_callbacks();
    connection->flush_connection();

    player = std::begin( players )->second.get();
    AssertThat( last_objects_id, Equals( player->object_id() ) );
  }

  void TearDown()
  {
    the::ctci::service< yarrr::MainThreadCallbackQueue >().process_callbacks();
  }

  It ( creates_a_ship_of_the_requested_type )
  {
    connection->wrapper.dispatch( giant_request );
    AssertThat( last_objects_type, Equals( giant_ship ) );
  }

  It ( adds_the_new_object_to_the_container )
  {
    connection->wrapper.dispatch( giant_request );
    AssertThat( objects->has_object_with_id( last_objects_id ), Equals( true ) );
  }

  It ( creates_a_ship_with_respawn_capabilities )
  {
    connection->wrapper.dispatch( giant_request );
    AssertThat(
        yarrr::has_component< yarrr::RespawnWhenDestroyed>( objects->object_with_id( last_objects_id ) ),
        Equals( true ) );
  }


  It ( deletes_the_old_ship_from_the_container )
  {
    const yarrr::Object::Id old_ship_id( player->object_id() );
    connection->wrapper.dispatch( giant_request );
    AssertThat( objects->has_object_with_id( old_ship_id ), Equals( false ) );
    AssertThat( connection->has_entity< yarrr::DeleteObject >(), Equals( true ) );
    AssertThat( connection->get_entity< yarrr::DeleteObject >()->object_id(), Equals( old_ship_id ) );
  }

  It ( assigns_the_created_ship_to_the_player )
  {
    connection->wrapper.dispatch( giant_request );
    AssertThat( connection->has_entity< yarrr::ObjectAssigned >(), Equals( true ) );
    AssertThat( connection->get_entity< yarrr::ObjectAssigned >()->object_id(), Equals( last_objects_id ) );
    AssertThat( player->object_id(), Equals( last_objects_id ) );
  }

  It ( does_not_change_anything_if_requested_ship_type_is_unknown )
  {
    const yarrr::Object::Id old_ship_id( player->object_id() );
    connection->wrapper.dispatch( unknown_ship_request );
    AssertThat( objects->has_object_with_id( old_ship_id ), Equals( true ) );
    AssertThat( player->object_id(), Equals( old_ship_id ) );
  }

  It ( does_not_change_anything_if_theres_no_ship_type )
  {
    const yarrr::Object::Id old_ship_id( player->object_id() );
    connection->wrapper.dispatch( invalid_request );
    AssertThat( objects->has_object_with_id( old_ship_id ), Equals( true ) );
    AssertThat( player->object_id(), Equals( old_ship_id ) );
  }

  std::unique_ptr< yarrrs::World > world;
  std::unique_ptr< yarrr::ObjectContainer > objects;
  std::unique_ptr< test::Connection > connection;
  yarrrs::Player::Container players;
  const std::string player_name{ "Kilgor Trout" };
  int connection_id;
  yarrrs::Player* player;

  std::string last_objects_type;
  yarrr::Object::Id last_objects_id;
  const std::string basic_ship{ "ship" };
  const std::string giant_ship{ "giant" };

  const yarrr::Command giant_request{ { "request_ship", giant_ship } };
  const yarrr::Command unknown_ship_request{ { "request_ship", "blabla unknown ship type" } };
  const yarrr::Command invalid_request{ { "request_ship" } };
};

