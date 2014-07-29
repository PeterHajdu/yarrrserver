#include "player.hpp"
#include "local_event_dispatcher.hpp"

#include <yarrr/delete_object.hpp>

Player::Player(
    int network_id,
    const std::string& name,
    ConnectionWrapper& connection_wrapper )
  : m_name( name )
  , m_connection( connection_wrapper.connection )
{
  connection_wrapper.register_dispatcher( m_dispatcher );
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
  //todo: send chat message
}


void
Players::handle_player_logout( const PlayerLoggedOut& logout )
{
  m_players.erase( logout.id );
  for ( auto& player : m_players )
  {
    player.second->send( yarrr::DeleteObject( logout.id ).serialize() );
  }
  //todo: send chat message
}

