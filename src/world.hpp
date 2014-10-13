#pragma once

#include "player.hpp"
#include "command_handler.hpp"

namespace yarrr
{
  class ObjectContainer;
  class DeleteObject;
  class PlayerKilled;
  class ObjectCreated;
}

namespace yarrrs
{

class World
{
  public:
    World( Player::Container&, yarrr::ObjectContainer& );

  private:
    void handle_player_logged_in( const PlayerLoggedIn& ) const;
    void handle_player_logged_out( const PlayerLoggedOut& ) const;
    void handle_object_created( const yarrr::ObjectCreated& add_object ) const;
    void handle_delete_object( const yarrr::DeleteObject& delete_object ) const;
    void handle_player_killed( const yarrr::PlayerKilled& player_killed ) const;

    void delete_object( yarrr::Object::Id id ) const;
    void add_object( yarrr::Object::Pointer&& object ) const;

    void broadcast( const yarrr::Data& message ) const;

    Player::Container& m_players;
    yarrr::ObjectContainer& m_objects;
    yarrrs::CommandHandler m_command_handler;
};

}

