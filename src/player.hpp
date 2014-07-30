#pragma once

#include "network_service.hpp"
#include <yarrr/types.hpp>
#include <thectci/dispatcher.hpp>
#include <memory>

class PlayerLoggedIn;
class PlayerLoggedOut;

class Players;
namespace yarrr
{
class ChatMessage;
}

class Player
{
  public:
    typedef std::unique_ptr< Player > Pointer;

    Player( Players&, int network_id, const std::string& name, ConnectionWrapper& connection_wrapper );
    bool send( yarrr::Data&& message ) const;
    const std::string name;

  private:
    void handle_chat_message( const yarrr::ChatMessage& );

    Players& m_players;
    int m_id;
    the::net::Connection& m_connection;
};


class Players
{
  public:
    Players();
    void broadcast( const std::vector< yarrr::Data > messages ) const;
    void handle_chat_message_from( const yarrr::ChatMessage&, int id );

  private:
    void handle_player_login( const PlayerLoggedIn& login );
    void handle_player_logout( const PlayerLoggedOut& logout );
    void greet_new_player( const PlayerLoggedIn& login );

    typedef std::unordered_map< int, Player::Pointer > PlayerContainer;
    PlayerContainer m_players;
};

