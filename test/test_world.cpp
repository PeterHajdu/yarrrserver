#include "../src/world.hpp"
#include "../src/player.hpp"
#include "../src/local_event_dispatcher.hpp"
#include <yarrr/object_factory.hpp>
#include <yarrr/object_identity.hpp>
#include <yarrr/basic_behaviors.hpp>
#include <yarrr/object_container.hpp>
#include <yarrr/engine_dispatcher.hpp>
#include <yarrr/main_thread_callback_queue.hpp>
#include <yarrr/delete_object.hpp>
#include <yarrr/object_created.hpp>
#include <yarrr/destruction_handlers.hpp>
#include <yarrr/chat_message.hpp>
#include <thectci/service_registry.hpp>

#include <igloo/igloo_alt.h>
#include <yarrr/test_connection.hpp>
#include "test_services.hpp"
#include "test_protocol.hpp"

using namespace igloo;
namespace test
{

yarrr::Object::Id
object_id_from_string( const std::string& string_id )
{
  std::stringstream str( string_id );
  yarrr::Object::Id object_id;
  str >> object_id;
  return object_id;
}

}

Describe( a_world )
{
  void cleanup()
  {
    services = std::make_unique< test::Services >();
  }

  void set_up_object_factory()
  {
    another_ship_type_was_constructed = false;
    the::ctci::service< yarrr::ObjectFactory >().register_creator(
        "ship",
        [ this ]()
        {
          yarrr::Object* new_ship( new yarrr::Object() );
          new_ship->add_behavior( std::make_unique< yarrr::PhysicalBehavior >() );
          last_object_id_created = new_ship->id();
          return yarrr::Object::Pointer( new_ship );
        });

    the::ctci::service< yarrr::ObjectFactory >().register_creator(
        "another_ship_type",
        [ this ]()
        {
          yarrr::Object* new_ship( new yarrr::Object() );
          new_ship->add_behavior( std::make_unique< yarrr::PhysicalBehavior >() );
          another_ship_type_was_constructed = true;
          return yarrr::Object::Pointer( new_ship );
        });
  }

  void SetUp()
  {
    cleanup();

    set_up_object_factory();

    player_bundle = services->log_in_player( player_name );
    connection = &player_bundle->connection;
    connection->flush_connection();
    connection_id = connection->connection->id;

    AssertThat( services->players.find( connection_id )!=std::end( services->players ), Equals( true ) );
    player = services->players[ connection_id ].get();
  }

  template < typename Event >
  void local_dispatch( const Event& event )
  {
    services->local_event_dispatcher.dispatcher.dispatch( event );
  }

  template < typename Event >
  void engine_dispatch( const Event& event )
  {
    services->engine_dispatcher.dispatch( event );
  }

  It ( creates_realtime_objects_on_startup_from_permanent_objects )
  {
    auto& an_object( services->modell_container.create( "object" ) );
    services->reset_world();
    const auto object_id( test::object_id_from_string( an_object.get( yarrr::model::realtime_object_id ) ) );
    AssertThat( services->objects.has_object_with_id( object_id ), Equals( true ) );
  }

  It ( does_not_create_realtime_objects_on_startup_for_player_controlled_objects )
  {
    auto& an_object( services->modell_container.create( "object" ) );
    an_object[ yarrr::model::object_type ] = yarrr::model::player_controlled;
    services->reset_world();
    AssertThat( an_object.has( yarrr::model::realtime_object_id ), Equals( false ) );
  }

  It ( creates_realtime_objects_on_startup_with_the_proper_ship_type )
  {
    auto& an_object( services->modell_container.create( "object" ) );
    an_object[ yarrr::model::ship_type ] = "another_ship_type";
    services->reset_world();
    AssertThat( another_ship_type_was_constructed, Equals( true ) );
  }

  It ( creates_realtime_objects_on_startup_with_the_proper_physical_parameters )
  {
    auto& an_object( services->modell_container.create( "object" ) );
    yarrr::Coordinate coordinate( 1000, 200 );
    yarrr::Angle angular_velocity( 1234 );
    an_object[ yarrr::model::hidden_x ] = std::to_string( coordinate.x );
    an_object[ yarrr::model::hidden_y ] = std::to_string( coordinate.y );
    an_object[ yarrr::model::hidden_angular_velocity ] = std::to_string( angular_velocity );
    services->reset_world();

    const auto object_id( test::object_id_from_string( an_object.get( yarrr::model::realtime_object_id ) ) );
    auto& realtime_object( services->objects.object_with_id( object_id ) );
    auto& physical_parameters( yarrr::component_of< yarrr::PhysicalBehavior >( realtime_object ).physical_parameters );

    AssertThat( physical_parameters.coordinate, Equals( coordinate ) );
    AssertThat( physical_parameters.angular_velocity, Equals( angular_velocity ) );
  }

  It ( does_not_create_new_ship_if_the_user_is_already_logged_in )
  {
    auto first_players_ship_id( last_object_id_created );
    auto player_bundle = services->log_in_player( player_name );
    AssertThat( first_players_ship_id, Equals( last_object_id_created ) );
  }

  It ( does_not_create_anything_if_ship_can_not_be_created )
  {
    cleanup();
    the::ctci::service< yarrr::ObjectFactory >().register_creator(
        "ship", [ this ]() { return yarrr::Object::Pointer( nullptr ); });

    local_dispatch( yarrrs::PlayerLoggedIn( connection->wrapper, connection->connection->id, player_name ) );
    AssertThat( services->players, IsEmpty() );
    AssertThat( services->objects, IsEmpty() );
  }

  It ( creates_a_new_player_with_the_given_id_and_name_if_player_logged_in_event_arrives )
  {
    AssertThat( services->players[ connection_id ]->name, Equals( player_name ) );
  }

  It ( creates_a_new_object_and_assigns_it_if_player_logged_in_event_arrives )
  {
    AssertThat( player->object_id(), Equals( last_object_id_created ) );
    AssertThat( services->objects.has_object_with_id( player->object_id() ), Equals( true ) );
  }

  It ( creates_objects_with_the_player_as_the_captain )
  {
    const yarrr::Object& object( services->objects.object_with_id( last_object_id_created ) );
    AssertThat( yarrr::has_component< yarrr::ObjectIdentity >( object ), Equals( true ) );
    AssertThat( yarrr::component_of< yarrr::ObjectIdentity >( object ).captain(), Equals( player_name ) );
  }

  It ( deletes_the_player_and_the_object_assigned_when_player_logged_out_arrives )
  {
    const yarrr::Object::Id deleted_ship( last_object_id_created );

    auto another_player( services->log_in_player( "asdf" ) );
    test::Connection& another_connection( another_player->connection );

    local_dispatch( yarrrs::PlayerLoggedOut( connection_id ) );
    services->main_thread_callback_queue.process_callbacks();
    AssertThat( services->players.find( connection_id ) == services->players.end(), Equals( true ) );
    AssertThat( another_connection.get_entity< yarrr::DeleteObject >()->object_id(), Equals( deleted_ship ) );
    AssertThat( services->objects.has_object_with_id( deleted_ship ), Equals( false ) );
  }

  It ( handles_invalid_logged_out_events )
  {
    local_dispatch( yarrrs::PlayerLoggedOut( last_object_id_created + 1 ) );
    AssertThat( services->players, HasLength( 1 ) );
    AssertThat( services->objects, HasLength( 1 ) );
  }

  It ( deletes_the_old_ship_of_a_killed_player )
  {
    yarrr::Object::Id old_ship_id{ last_object_id_created };
    engine_dispatch( yarrr::PlayerKilled( old_ship_id ) );
    AssertThat( services->objects.has_object_with_id( old_ship_id ), Equals( true ) );
    services->main_thread_callback_queue.process_callbacks();
    AssertThat( services->objects.has_object_with_id( old_ship_id ), Equals( false ) );

    AssertThat( connection->has_entity< yarrr::DeleteObject >(), Equals( true ) );
  }

  It ( assigns_a_new_object_to_a_killed_player )
  {
    yarrr::Object::Id old_ship_id{ last_object_id_created };
    engine_dispatch( yarrr::PlayerKilled( old_ship_id ) );

    yarrr::Object::Id new_ship_id{ last_object_id_created };
    AssertThat( new_ship_id, !Equals( old_ship_id ) );
    AssertThat( player->object_id(), Equals( new_ship_id ) );

    test::assert_object_assigned( *connection, new_ship_id );
  }

  It ( adds_the_new_object_of_a_killed_player_postponed )
  {
    engine_dispatch( yarrr::PlayerKilled( last_object_id_created ) );
    yarrr::Object::Id new_ship_id{ last_object_id_created };

    AssertThat( services->objects.has_object_with_id( new_ship_id ), Equals( false ) );
    services->main_thread_callback_queue.process_callbacks();
    AssertThat( services->objects.has_object_with_id( new_ship_id ), Equals( true ) );
  }

  It ( handles_objects_created_by_the_engine )
  {
    yarrr::Object* new_object( new yarrr::Object() );
    engine_dispatch( yarrr::ObjectCreated( yarrr::Object::Pointer( new_object ) ) );
    AssertThat( services->objects.has_object_with_id( new_object->id() ), Equals( false ) );
    services->main_thread_callback_queue.process_callbacks();
    AssertThat( services->objects.has_object_with_id( new_object->id() ), Equals( true ) );
  }

  It ( handles_delete_object_requested_by_the_engine )
  {
    engine_dispatch( yarrr::DeleteObject( last_object_id_created ) );
    AssertThat( services->objects.has_object_with_id( last_object_id_created ), Equals( true ) );
    services->main_thread_callback_queue.process_callbacks();
    AssertThat( services->objects.has_object_with_id( last_object_id_created ), Equals( false ) );
  }

  It ( sends_delete_object_to_all_registered_players )
  {
    auto another_player( services->log_in_player( "asdf" ) );
    test::Connection& another_connection( another_player->connection );

    connection->flush_connection();
    another_connection.flush_connection();

    engine_dispatch( yarrr::DeleteObject( last_object_id_created ) );

    AssertThat( connection->has_no_data(), Equals( true ) );
    AssertThat( another_connection.has_no_data(), Equals( true ) );
    services->main_thread_callback_queue.process_callbacks();
    AssertThat( connection->has_no_data(), Equals( false ) );
    AssertThat( another_connection.has_no_data(), Equals( false ) );

    AssertThat( connection->get_entity< yarrr::DeleteObject >()->object_id(), Equals( last_object_id_created ) );
  }


  It ( notifies_players_when_someone_logs_in )
  {
    const std::string new_player_name( "asdf" );
    auto another_player( services->log_in_player( new_player_name ) );
    test::Connection& another_connection( another_player->connection );

    AssertThat( connection->has_no_data(), Equals( false ) );
    AssertThat( another_connection.has_no_data(), Equals( false ) );

    auto chat_message( connection->get_entity< yarrr::ChatMessage >() );
    AssertThat( chat_message->message(), Contains( new_player_name ) );
  }

  std::unique_ptr< test::Services::PlayerBundle > player_bundle;
  test::Connection* connection;

  const std::string player_name{ "Kilgor Trout" };
  int connection_id;
  yarrrs::Player* player;
  yarrr::Object::Id last_object_id_created;
  std::unique_ptr< test::Services > services;
  bool another_ship_type_was_constructed;
};

