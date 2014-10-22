#include "../src/world.hpp"
#include "../src/player.hpp"
#include "../src/local_event_dispatcher.hpp"
#include "test_connection.hpp"
#include "test_services.hpp"
#include <yarrr/object_factory.hpp>
#include <yarrr/basic_behaviors.hpp>
#include <yarrr/object_container.hpp>
#include <yarrr/engine_dispatcher.hpp>
#include <yarrr/main_thread_callback_queue.hpp>
#include <yarrr/delete_object.hpp>
#include <yarrr/destruction_handlers.hpp>
#include <yarrr/chat_message.hpp>
#include <thectci/service_registry.hpp>
#include <igloo/igloo_alt.h>

using namespace igloo;

Describe( a_world )
{
  void cleanup()
  {
    engine_dispatcher = &the::ctci::service< yarrr::EngineDispatcher >();
    engine_dispatcher->clear();
    local_event_dispatcher = &the::ctci::service< LocalEventDispatcher >().dispatcher;
    local_event_dispatcher->clear();

    players.clear();
    objects.reset( new yarrr::ObjectContainer() );
    connection.reset( new test::Connection() );
    connection_id = connection->connection.id;
    world.reset( new yarrrs::World( players, *objects ) );
  }

  void log_in_for_connection( test::Connection& connection )
  {
    local_dispatch( yarrrs::PlayerLoggedIn( connection.wrapper, connection.connection.id, player_name ) );
  }

  void SetUp()
  {
    cleanup();
    the::ctci::service< yarrr::ObjectFactory >().register_creator(
        "ship",
        [ this ]()
        {
          yarrr::Object* new_ship( new yarrr::Object() );
          last_object_id_created = new_ship->id();
          return yarrr::Object::Pointer( new_ship );
        });

    log_in_for_connection( *connection );
    connection->flush_connection();

    AssertThat( players.find( connection_id )!=std::end( players ), Equals( true ) );
    player = players[ connection_id ].get();
  }

  void TearDown()
  {
    the::ctci::service< yarrr::MainThreadCallbackQueue >().process_callbacks();
  }

  template < typename Event >
  void local_dispatch( const Event& event )
  {
    local_event_dispatcher->dispatch( event );
  }

  template < typename Event >
  void engine_dispatch( const Event& event )
  {
    engine_dispatcher->dispatch( event );
  }

  It ( does_not_create_anything_if_ship_can_not_be_created )
  {
    cleanup();
    the::ctci::service< yarrr::ObjectFactory >().register_creator(
        "ship", [ this ]() { return yarrr::Object::Pointer( nullptr ); });

    local_dispatch( yarrrs::PlayerLoggedIn( connection->wrapper, connection->connection.id, player_name ) );
    AssertThat( players, IsEmpty() );
    AssertThat( *objects, IsEmpty() );
  }

  It ( creates_a_new_player_with_the_given_id_and_name_if_player_logged_in_event_arrives )
  {
    AssertThat( players[ connection_id ]->name, Equals( player_name ) );
  }

  It ( creates_a_new_object_and_assigns_it_if_player_logged_in_event_arrives )
  {
    AssertThat( player->object_id(), Equals( last_object_id_created ) );
    AssertThat( objects->has_object_with_id( player->object_id() ), Equals( true ) );
  }

  It ( creates_a_new_object_for_player_with_respawn_capabilities )
  {
    AssertThat(
        yarrr::has_component< yarrr::RespawnWhenDestroyed>( objects->object_with_id( player->object_id() ) ),
        Equals( true ) );
  }

  It ( deletes_the_player_and_the_object_assigned_when_player_logged_out_arrives )
  {
    const yarrr::Object::Id deleted_ship( last_object_id_created );
    another_connection.reset( new test::Connection() );
    log_in_for_connection( *another_connection );

    local_dispatch( yarrrs::PlayerLoggedOut( connection_id ) );
    the::ctci::service< yarrr::MainThreadCallbackQueue >().process_callbacks();
    AssertThat( players.find( connection_id ) == players.end(), Equals( true ) );
    AssertThat( another_connection->get_entity< yarrr::DeleteObject >()->object_id(), Equals( deleted_ship ) );
    AssertThat( objects->has_object_with_id( deleted_ship ), Equals( false ) );
  }

  It ( handles_invalid_logged_out_events )
  {
    local_dispatch( yarrrs::PlayerLoggedOut( last_object_id_created + 1 ) );
    AssertThat( players, HasLength( 1 ) );
    AssertThat( *objects, HasLength( 1 ) );
  }

  It ( deletes_the_old_ship_of_a_killed_player )
  {
    yarrr::Object::Id old_ship_id{ last_object_id_created };
    engine_dispatch( yarrr::PlayerKilled( old_ship_id ) );
    AssertThat( objects->has_object_with_id( old_ship_id ), Equals( true ) );
    the::ctci::service< yarrr::MainThreadCallbackQueue >().process_callbacks();
    AssertThat( objects->has_object_with_id( old_ship_id ), Equals( false ) );

    AssertThat( connection->has_entity< yarrr::DeleteObject >(), Equals( true ) );
  }

  It ( assignes_a_new_object_to_a_killed_player )
  {
    yarrr::Object::Id old_ship_id{ last_object_id_created };
    engine_dispatch( yarrr::PlayerKilled( old_ship_id ) );

    yarrr::Object::Id new_ship_id{ last_object_id_created };
    AssertThat( new_ship_id, !Equals( old_ship_id ) );
    AssertThat( player->object_id(), Equals( new_ship_id ) );
    AssertThat( connection->has_entity< yarrr::ObjectAssigned >(), Equals( true ) );
  }

  It ( adds_the_new_object_of_a_killed_player_postponed )
  {
    engine_dispatch( yarrr::PlayerKilled( last_object_id_created ) );
    yarrr::Object::Id new_ship_id{ last_object_id_created };

    AssertThat( objects->has_object_with_id( new_ship_id ), Equals( false ) );
    the::ctci::service< yarrr::MainThreadCallbackQueue >().process_callbacks();
    AssertThat( objects->has_object_with_id( new_ship_id ), Equals( true ) );
  }

  It ( adds_respawn_to_the_new_ship_of_a_killed_player )
  {
    engine_dispatch( yarrr::PlayerKilled( last_object_id_created ) );
    yarrr::Object::Id new_ship_id{ last_object_id_created };

    the::ctci::service< yarrr::MainThreadCallbackQueue >().process_callbacks();

    AssertThat(
        yarrr::has_component< yarrr::RespawnWhenDestroyed>( objects->object_with_id( new_ship_id ) ),
        Equals( true ) );
  }

  It ( handles_objects_created_by_the_engine )
  {
    yarrr::Object* new_object( new yarrr::Object() );
    engine_dispatch( yarrr::ObjectCreated( yarrr::Object::Pointer( new_object ) ) );
    AssertThat( objects->has_object_with_id( new_object->id() ), Equals( false ) );
    the::ctci::service< yarrr::MainThreadCallbackQueue >().process_callbacks();
    AssertThat( objects->has_object_with_id( new_object->id() ), Equals( true ) );
  }

  It ( handles_delete_object_requested_by_the_engine )
  {
    engine_dispatch( yarrr::DeleteObject( last_object_id_created ) );
    AssertThat( objects->has_object_with_id( last_object_id_created ), Equals( true ) );
    the::ctci::service< yarrr::MainThreadCallbackQueue >().process_callbacks();
    AssertThat( objects->has_object_with_id( last_object_id_created ), Equals( false ) );
  }

  It ( sends_delete_object_to_all_registered_players )
  {
    another_connection.reset( new test::Connection() );
    log_in_for_connection( *another_connection );
    connection->flush_connection();
    another_connection->flush_connection();

    engine_dispatch( yarrr::DeleteObject( last_object_id_created ) );

    AssertThat( connection->has_no_data(), Equals( true ) );
    AssertThat( another_connection->has_no_data(), Equals( true ) );
    the::ctci::service< yarrr::MainThreadCallbackQueue >().process_callbacks();
    AssertThat( connection->has_no_data(), Equals( false ) );
    AssertThat( another_connection->has_no_data(), Equals( false ) );

    AssertThat( connection->get_entity< yarrr::DeleteObject >()->object_id(), Equals( last_object_id_created ) );
  }


  It ( notifies_players_when_someone_logs_in )
  {
    another_connection.reset( new test::Connection() );
    log_in_for_connection( *another_connection );

    AssertThat( connection->has_no_data(), Equals( false ) );
    AssertThat( another_connection->has_no_data(), Equals( false ) );

    auto chat_message( connection->get_entity< yarrr::ChatMessage >() );
    AssertThat( chat_message->message(), Contains( player_name ) );
  }

  It ( sends_notification_when_someone_logs_in )
  {
    auto& stream( test::get_notification_stream() );
    stream.str( "" );
    another_connection.reset( new test::Connection() );
    log_in_for_connection( *another_connection );
    AssertThat( stream.str(), !IsEmpty() );
    AssertThat( stream.str(), Contains( player_name ) );
  }

  std::unique_ptr< yarrrs::World > world;
  std::unique_ptr< yarrr::ObjectContainer > objects;
  std::unique_ptr< test::Connection > connection;
  std::unique_ptr< test::Connection > another_connection;
  yarrrs::Player::Container players;
  const std::string player_name{ "Kilgor Trout" };
  int connection_id;
  yarrrs::Player* player;
  the::ctci::Dispatcher* local_event_dispatcher;
  the::ctci::Dispatcher* engine_dispatcher;
  yarrr::Object::Id last_object_id_created;
};

