#include "world.hpp"
#include "player.hpp"
#include "local_event_dispatcher.hpp"
#include "object_factory.hpp"
#include "notifier.hpp"
#include <thectci/service_registry.hpp>
#include <yarrr/log.hpp>
#include <yarrr/object_container.hpp>
#include <yarrr/engine_dispatcher.hpp>
#include <yarrr/main_thread_callback_queue.hpp>
#include <yarrr/delete_object.hpp>
#include <yarrr/destruction_handlers.hpp>
#include <yarrr/chat_message.hpp>
#include <yarrr/command.hpp>
#include <yarrr/basic_behaviors.hpp>

namespace
{

yarrr::Object::Pointer
create_player_ship( const std::string& type )
{
  yarrr::Object::Pointer new_ship( the::ctci::service< yarrrs::ObjectFactory >().create_a( type ) );
  if ( !new_ship )
  {
    return nullptr;
  }

  new_ship->add_behavior( yarrr::ObjectBehavior::Pointer( new yarrr::RespawnWhenDestroyed() ) );
  return new_ship;
}


void
add_command_handlers_to(
    yarrrs::CommandHandler& command_handler,
    yarrr::ObjectContainer& objects,
    const yarrrs::Player::Container& players )
{
  command_handler.register_handler( "request_ship",
      [ &objects, &players ]( const yarrr::Command& command, yarrrs::Player& player )
      {
        if ( command.parameters().empty() )
        {
          thelog( yarrr::log::warning )( "Invalid ship request from", player.name );
          return;
        }

        const std::string requested_ship_type( *std::begin( command.parameters() ) );
        thelog( yarrr::log::info )( "Ship type requested", requested_ship_type, "by", player.name );
        yarrr::Object::Pointer new_ship( create_player_ship( requested_ship_type ) );

        if ( !new_ship )
        {
          thelog( yarrr::log::info )( "Unanle to create ship", requested_ship_type, "for", player.name );
          return;
        }

        yarrrs::broadcast( players, yarrr::DeleteObject( player.object_id() ) );
        objects.delete_object( player.object_id() );
        player.assign_object( *new_ship );
        objects.add_object( std::move( new_ship ) );
      } );
}


yarrrs::Player*
player_with_object_id( yarrrs::Player::Container& players, yarrr::Object::Id id )
{
  for ( const auto& player : players )
  {
    if ( player.second->object_id() == id )
    {
      return player.second.get();
    }
  }

  return nullptr;
}

}

namespace yarrrs
{

//todo: tear this up to separate handlers
World::World( Player::Container& players, yarrr::ObjectContainer& objects )
  : m_players( players )
  , m_objects( objects )
{
  the::ctci::Dispatcher& local_event_dispatcher(
      the::ctci::service< LocalEventDispatcher >().dispatcher );

  local_event_dispatcher.register_listener< PlayerLoggedIn >(
      std::bind( &World::handle_player_logged_in, this, std::placeholders::_1 ) );

  local_event_dispatcher.register_listener< PlayerLoggedOut >(
      std::bind( &World::handle_player_logged_out, this, std::placeholders::_1 ) );

  the::ctci::Dispatcher& engine_dispatcher(
      the::ctci::service< yarrr::EngineDispatcher >() );

  engine_dispatcher.register_listener< yarrr::ObjectCreated >(
      std::bind( &World::handle_object_created, this, std::placeholders::_1 ) );

  engine_dispatcher.register_listener< yarrr::DeleteObject >(
      std::bind( &World::handle_delete_object, this, std::placeholders::_1 ) );

  engine_dispatcher.register_listener< yarrr::PlayerKilled >(
      std::bind( &World::handle_player_killed, this, std::placeholders::_1 ) );

  add_command_handlers_to( m_command_handler, m_objects, m_players );
}

void
World::handle_player_killed( const yarrr::PlayerKilled& player_killed ) const
{
  Player* player( player_with_object_id( m_players, player_killed.object_id ) );
  if ( !player )
  {
    thelog( yarrr::log::error )( "Unable to find killed player." );
    return;
  }

  thelog( yarrr::log::debug )( "Player killed.", player->name );

  yarrr::Object::Pointer new_object( create_player_ship( "ship" ) );
  if ( !new_object )
  {
    thelog( yarrr::log::error )( "Unable to create ship." );
    return;
  }

  player->assign_object( *new_object );
  add_object( std::move( new_object ) );
  delete_object( player_killed.object_id );
}

void
World::handle_delete_object( const yarrr::DeleteObject& del_object ) const
{
  thelog( yarrr::log::debug )( __PRETTY_FUNCTION__, del_object.object_id() );
  delete_object( del_object.object_id() );
}

void
World::add_object( yarrr::Object::Pointer&& object_ptr ) const
{
  yarrr::Object* object( object_ptr.release() );
  the::ctci::service< yarrr::MainThreadCallbackQueue >().push_back(
      [ this, object ]()
      {
        m_objects.add_object( yarrr::Object::Pointer( object ) );
      } );
}

void
World::delete_object( yarrr::Object::Id id ) const
{
  thelog( yarrr::log::debug )( "Deleting object", id );
  the::ctci::service< yarrr::MainThreadCallbackQueue >().push_back(
      [ this, id ]()
      {
        yarrrs::broadcast( m_players, yarrr::DeleteObject( id ) );
        m_objects.delete_object( id );
      } );
}

void
World::handle_object_created( const yarrr::ObjectCreated& object_created ) const
{
  yarrr::Object* object( object_created.object.release() );
  the::ctci::service< yarrr::MainThreadCallbackQueue >().push_back(
      [ this, object ]()
      {
        m_objects.add_object( yarrr::Object::Pointer( object ) );
      } );
}

void
World::handle_player_logged_in( const PlayerLoggedIn& login ) const
{
  thelog_trace( yarrr::log::info, __PRETTY_FUNCTION__ );

  yarrr::Object::Pointer new_object( create_player_ship( "ship" ) );
  if ( !new_object )
  {
    thelog( yarrr::log::error )( "Unable to create ship for new user:", login.name );
    return;
  }

  m_players.emplace( std::make_pair(
        login.id,
        Player::Pointer( new Player(
            m_players,
            login.name,
            login.connection_wrapper,
            m_command_handler ) ) ) );
  m_players[ login.id ]->assign_object( *new_object );
  m_objects.add_object( std::move( new_object ) );

  const std::string notification( std::string( "Player logged in: " ) + login.name );
  thelog( yarrr::log::info )( notification );
  yarrrs::broadcast( m_players, yarrr::ChatMessage( notification, "server" ) );
  the::ctci::service< yarrrs::Notifier >().send( notification );
}


void
World::handle_player_logged_out( const PlayerLoggedOut& logout ) const
{
  thelog_trace( yarrr::log::info, __PRETTY_FUNCTION__ );
  const auto player( m_players.find( logout.id ) );
  if ( player == std::end( m_players ) )
  {
    thelog( yarrr::log::warning )( "Unknown player logged out." );
    return;
  }
  thelog( yarrr::log::warning )( "Deleting player and object.", player->second->object_id(), player->second->name );
  yarrrs::broadcast( m_players, yarrr::ChatMessage( "Player logged out: " + player->second->name, "server" ) );

  delete_object( player->second->object_id() );
  m_players.erase( logout.id );
}


}

