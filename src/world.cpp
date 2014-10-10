#include "world.hpp"
#include "player.hpp"
#include "local_event_dispatcher.hpp"
#include "object_factory.hpp"
#include "notifier.hpp"
#include <thectci/service_registry.hpp>
#include <yarrr/log.hpp>
#include <yarrr/engine_dispatcher.hpp>
#include <yarrr/main_thread_callback_queue.hpp>
#include <yarrr/delete_object.hpp>
#include <yarrr/destruction_handlers.hpp>
#include <yarrr/chat_message.hpp>

namespace
{

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

  yarrr::Object::Pointer new_object( the::ctci::service< yarrrs::ObjectFactory >().create_a( "ship" ) );
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
  the::ctci::service< yarrr::MainThreadCallbackQueue >().push_back(
      [ this, id ]()
      {
        broadcast( yarrr::DeleteObject( id ).serialize() );
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

  yarrr::Object::Pointer new_object( the::ctci::service< yarrrs::ObjectFactory >().create_a( "ship" ) );
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
            login.connection_wrapper ) ) ) );
  m_players[ login.id ]->assign_object( *new_object );
  m_objects.add_object( std::move( new_object ) );

  const std::string notification( std::string( "New player logged in." ) + login.name );
  thelog( yarrr::log::info )( notification );
  broadcast( yarrr::ChatMessage( notification, "server" ).serialize() );
  the::ctci::service< yarrrs::Notifier >().send( notification );
}


void
World::handle_player_logged_out( const PlayerLoggedOut& logout ) const
{
  thelog_trace( yarrr::log::info, __PRETTY_FUNCTION__ );
  const auto player( m_players.find( logout.id ) );
  if ( player == std::end( m_players ) )
  {
    return;
  }

  m_objects.delete_object( player->second->object_id() );
  m_players.erase( logout.id );
}


void
World::broadcast( const yarrr::Data& message ) const
{
  for ( const auto& player : m_players )
  {
    player.second->send( yarrr::Data( message ) );
  }
}

}
