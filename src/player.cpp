#include "player.hpp"
#include "local_event_dispatcher.hpp"

#include <yarrr/object_container.hpp>
#include <yarrr/delete_object.hpp>
#include <yarrr/object.hpp>
#include <yarrr/basic_behaviors.hpp>
#include <yarrr/chat_message.hpp>

namespace
{
  yarrr::Object::Pointer create_object( const PlayerLoggedIn& login )
  {
    yarrr::Object::Pointer ship( new yarrr::Object() );
    ship->add_behavior( yarrr::ObjectBehavior::Pointer( new yarrr::PhysicalBehavior() ) );
    ship->add_behavior( yarrr::ObjectBehavior::Pointer( new yarrr::Engine() ) );
    login.connection_wrapper.register_dispatcher( *ship );
    return ship;
  }
}

Player::Player(
    Players& players,
    int network_id,
    const std::string& name,
    ConnectionWrapper& connection_wrapper )
  : name( name )
  , m_players( players )
  , m_id( network_id )
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
  m_players.handle_chat_message_from( message, m_id );
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
  yarrr::Object::Pointer new_object( create_object( login ) );
  login.connection_wrapper.connection.send( yarrr::LoginResponse( new_object->id ).serialize() );
  m_object_container.add_object( std::move( new_object ) );
  greet_new_player( login );

  m_players.emplace( std::make_pair(
        login.id,
        Player::Pointer( new Player(
            *this,
            login.id,
            login.name,
            login.connection_wrapper ) ) ) );
  broadcast( { yarrr::ChatMessage( "New player logged in: " + login.name, "server" ).serialize() } );
}


void
Players::greet_new_player( const PlayerLoggedIn& login )
{
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
  m_object_container.delete_object( logout.id );
  broadcast( {
      yarrr::ChatMessage( "Player logged out: " + m_players[ logout.id ]->name, "server" ).serialize(),
      yarrr::DeleteObject( logout.id ).serialize() } );
  m_players.erase( logout.id );
}

