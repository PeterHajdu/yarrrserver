#include "player.hpp"
#include "local_event_dispatcher.hpp"
#include "notifier.hpp"

#include <yarrr/object_container.hpp>
#include <yarrr/delete_object.hpp>
#include <yarrr/object.hpp>
#include <yarrr/chat_message.hpp>
#include <yarrr/engine_dispatcher.hpp>
#include <yarrr/main_thread_callback_queue.hpp>
#include <yarrr/log.hpp>

#include <thectci/service_registry.hpp>

namespace yarrrs
{

Player::Player(
    Players& players,
    int network_id,
    const std::string& name,
    ConnectionWrapper& connection_wrapper,
    yarrr::Object::Id object_id )
  : name( name )
  , object_id( object_id )
  , m_network_id( network_id )
  , m_players( players )
  , m_connection( connection_wrapper.connection )
{
  connection_wrapper.register_listener< yarrr::ChatMessage >(
      std::bind( &Player::handle_chat_message, this, std::placeholders::_1 ) );
}

bool
Player::send( yarrr::Data&& message ) const
{
  return m_connection.send( std::move( message ) );
}

void
Player::handle_chat_message( const yarrr::ChatMessage& message )
{
  m_players.handle_chat_message_from( message, m_network_id );
}

Players::Players( yarrr::ObjectContainer& object_container )
  : m_object_container( object_container )
{
  the::ctci::Dispatcher& local_event_dispatcher(
      the::ctci::service< LocalEventDispatcher >().dispatcher );
  local_event_dispatcher.register_listener< PlayerLoggedIn >(
      std::bind( &Players::handle_player_login, this, std::placeholders::_1 ) );
  local_event_dispatcher.register_listener< PlayerLoggedOut >(
      std::bind( &Players::handle_player_logout, this, std::placeholders::_1 ) );

  the::ctci::service< yarrr::EngineDispatcher >().register_listener< yarrr::Canon::AddObject >(
      std::bind( &Players::handle_add_laser, this, std::placeholders::_1 ) );

  the::ctci::service< yarrr::EngineDispatcher >().register_listener< yarrr::DeleteObject >(
      std::bind( &Players::handle_delete_laser, this, std::placeholders::_1 ) );
}


void
Players::broadcast( const std::vector< yarrr::Data > messages ) const
{
  for ( const auto& player : m_players )
  {
    for ( const auto& message : messages )
    {
      player.second->send( yarrr::Data( message ) );
    }
  }
}

void
Players::handle_chat_message_from( const yarrr::ChatMessage& message, int id )
{
  thelog( yarrr::log::info )( "chat message: ", message.sender(), message.message() );
  yarrr::Data serialized_message( message.serialize() );
  for ( const auto& player : m_players )
  {
    if ( player.first == id )
    {
      continue;
    }

    player.second->send( yarrr::Data( serialized_message ) );
  }
}

void
Players::handle_player_login( const PlayerLoggedIn& login )
{
  thelog_trace( yarrr::log::info, __PRETTY_FUNCTION__ );
  yarrr::Object::Pointer new_object( yarrr::create_ship() );
  login.connection_wrapper.register_dispatcher( new_object->dispatcher );
  login.connection_wrapper.connection.send( yarrr::LoginResponse( new_object->id ).serialize() );
  greet_new_player( login );

  the::ctci::service< yarrrs::Notifier >().send( "Player logged in: " + login.name );

  m_players.emplace( std::make_pair(
        login.id,
        Player::Pointer( new Player(
            *this,
            login.id,
            login.name,
            login.connection_wrapper,
            new_object->id ) ) ) );

  thelog( yarrr::log::info )( "player logged in", login.id, login.name );

  m_object_container.add_object( std::move( new_object ) );
  broadcast( { yarrr::ChatMessage( "New player logged in: " + login.name, "server" ).serialize() } );
}


void
Players::greet_new_player( const PlayerLoggedIn& login )
{
  thelog_trace( yarrr::log::info, __PRETTY_FUNCTION__ );
  std::string greeting( "Welcome " + login.name + "! The following players are online: " );
  for ( const auto& player : m_players )
  {
    greeting += player.second->name + ", ";
  }
  login.connection_wrapper.connection.send( yarrr::ChatMessage( greeting, "server" ).serialize() );
}


void
Players::handle_player_logout( const PlayerLoggedOut& logout )
{
  thelog_trace( yarrr::log::info, __PRETTY_FUNCTION__ );

  PlayerContainer::const_iterator player( m_players.find( logout.id ) );
  if ( m_players.end() == player )
  {
    thelog( yarrr::log::warning )( "Logout arrived with invalid player id." );
    return;
  }

  delete_object_with_id( player->second->object_id );
  broadcast( { yarrr::ChatMessage( "Player logged out: " + m_players[ logout.id ]->name, "server" ).serialize() });
  m_players.erase( logout.id );
}


void
Players::handle_add_laser( const yarrr::Canon::AddObject& add_object )
{
  yarrr::Object* object( add_object.object.release() );
  the::ctci::service< yarrr::MainThreadCallbackQueue >().push_back(
      [ this, object ]()
      {
        m_object_container.add_object( yarrr::Object::Pointer( object ) );
      } );
}


void
Players::handle_delete_laser( const yarrr::DeleteObject& delete_object )
{
  yarrr::Object::Id object_id( delete_object.object_id() );
  the::ctci::service< yarrr::MainThreadCallbackQueue >().push_back(
      [ this, object_id ]()
      {
        delete_object_with_id( object_id );
      } );
}

void
Players::delete_object_with_id( yarrr::Object::Id id )
{
  broadcast( { yarrr::DeleteObject( id ).serialize() } );
  m_object_container.delete_object( id );
}

}

