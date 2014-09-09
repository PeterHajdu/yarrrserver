#pragma once

#include "network_service.hpp"
#include <yarrr/types.hpp>
#include <yarrr/object_container.hpp>
#include <yarrr/basic_behaviors.hpp>
#include <thectci/dispatcher.hpp>
#include <memory>

namespace yarrr
{

class ChatMessage;
class DeleteObject;
class PlayerKilled;

}

namespace yarrrs
{

class PlayerLoggedIn;
class PlayerLoggedOut;
class Players;

class Player
{
  public:
    typedef std::unique_ptr< Player > Pointer;

    Player(
        Players&,
        int network_id,
        const std::string& name,
        ConnectionWrapper& connection_wrapper );

    bool send( yarrr::Data&& message ) const;

    const std::string name;

    //todo: rewrite this whole mess
    yarrr::Object::Id object_id();
    yarrr::Object::Pointer create_new_ship();

  private:
    void handle_chat_message( const yarrr::ChatMessage& );

    const int m_network_id;
    Players& m_players;
    ConnectionWrapper& m_connection_wrapper;
    yarrr::Object* m_last_ship;
};


class Players
{
  public:
    Players( yarrr::ObjectContainer& );
    void broadcast( const std::vector< yarrr::Data > messages ) const;
    void handle_chat_message_from( const yarrr::ChatMessage&, int id );

  private:
    void handle_player_login( const PlayerLoggedIn& login );
    void handle_player_logout( const PlayerLoggedOut& logout );
    void greet_new_player( const PlayerLoggedIn& login );

    void handle_add_object( const yarrr::ObjectCreated& );
    void handle_delete_object( const yarrr::DeleteObject& );
    void handle_player_killed( const yarrr::PlayerKilled& );

    void postponed_delete_object_with_id( yarrr::Object::Id );
    void delete_object_with_id( yarrr::Object::Id );

    Player* player_from_object_id( yarrr::Object::Id id );

    typedef std::unordered_map< int, Player::Pointer > PlayerContainer;
    PlayerContainer m_players;
    yarrr::ObjectContainer& m_object_container;
};

}

