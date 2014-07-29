#include "player.hpp"
#include "local_event_dispatcher.hpp"
#include "object_container.hpp"

#include <yarrr/delete_object.hpp>
#include <yarrr/object.hpp>
#include <yarrr/basic_behaviors.hpp>
#include <yarrr/chat_message.hpp>

namespace
{
  yarrr::ObjectBehavior::Pointer create_physical_behavior( const PlayerLoggedIn& login )
  {
    yarrr::LocalPhysicalBehavior* physical_behavior( new yarrr::LocalPhysicalBehavior() );
    yarrr::ObjectBehavior::Pointer behavior( physical_behavior );
    physical_behavior->physical_parameters.id = login.id;
    return behavior;
  }

  yarrr::Object::Pointer create_object( const PlayerLoggedIn& login )
  {
    yarrr::Object::Pointer ship( new yarrr::Object() );
    ship->add_behavior( yarrr::ObjectBehavior::Pointer( create_physical_behavior( login ) ) );
    ship->add_behavior( yarrr::ObjectBehavior::Pointer( new yarrr::SimplePhysicsUpdater() ) );
    ship->add_behavior( yarrr::ObjectBehavior::Pointer( new yarrr::Engine() ) );
    ship->add_behavior( yarrr::ObjectBehavior::Pointer( new yarrr::PhysicalParameterSerializer() ) );
    login.connection_wrapper.register_dispatcher( *ship );
    return ship;
  }
}

Player::Player(
    int network_id,
    const std::string& name,
    ConnectionWrapper& connection_wrapper )
  : name( name )
  , m_connection( connection_wrapper.connection )
{
}

bool
Player::send( yarrr::Data&& message ) const
{
  return m_connection.send( std::move( message ) );
}


Players::Players()
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
Players::handle_player_login( const PlayerLoggedIn& login )
{
  m_players.emplace( std::make_pair(
        login.id,
        Player::Pointer( new Player(
            login.id,
            login.name,
            login.connection_wrapper ) ) ) );

  the::ctci::service< ObjectContainer >().add_object( login.id, create_object( login ) );
  broadcast( { yarrr::ChatMessage( "New player logged in: " + login.name, "server" ).serialize() } );
}


void
Players::handle_player_logout( const PlayerLoggedOut& logout )
{
  the::ctci::service< ObjectContainer >().delete_object( logout.id );
  broadcast( {
      yarrr::ChatMessage( "Player logged out: " + m_players[ logout.id ]->name, "server" ).serialize(),
      yarrr::DeleteObject( logout.id ).serialize() } );
  m_players.erase( logout.id );
}

