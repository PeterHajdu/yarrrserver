#pragma once

#include "network_service.hpp"
#include <yarrr/types.hpp>
#include <thectci/dispatcher.hpp>
#include <memory>

class PlayerLoggedIn;
class PlayerLoggedOut;

class Player
{
  public:
    typedef std::unique_ptr< Player > Pointer;

    Player( int network_id, const std::string& name, ConnectionWrapper& connection_wrapper );
    bool send( yarrr::Data&& message ) const;
    const std::string name;

  private:
    the::net::Connection& m_connection;
};


class Players
{
  public:
    Players();
    void broadcast( const std::vector< yarrr::Data > messages ) const;

  private:
    void handle_player_login( const PlayerLoggedIn& login );
    void handle_player_logout( const PlayerLoggedOut& logout );

    typedef std::unordered_map< int, Player::Pointer > PlayerContainer;
    PlayerContainer m_players;
};

