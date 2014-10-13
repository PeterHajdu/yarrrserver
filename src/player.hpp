#pragma once

#include "network_service.hpp"
#include <memory>
#include <unordered_map>

namespace yarrr
{

class ChatMessage;
class Command;

}

namespace yarrrs
{

class CommandHandler;
class Player
{
  public:
    typedef std::unique_ptr< Player > Pointer;
    typedef std::unordered_map< int, Pointer > Container;

    Player(
        const Container&,
        const std::string& name,
        ConnectionWrapper& connection_wrapper,
        const CommandHandler& );

    Player( const Player& ) = delete;
    Player& operator=( const Player& ) = delete;

    bool send( yarrr::Data&& message ) const;

    const std::string name;
    yarrr::Object::Id object_id() const;
    void assign_object( yarrr::Object& object );


  private:
    void handle_chat_message( const yarrr::ChatMessage& );

    const Container& m_players;
    ConnectionWrapper& m_connection_wrapper;
    yarrr::Object* m_current_object;
};

}

