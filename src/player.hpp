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

class World;
class Player
{
  public:
    typedef std::unique_ptr< Player > Pointer;
    typedef std::unordered_map< int, Pointer > Container;

    Player(
        const World&,
        int network_id,
        const std::string& name,
        ConnectionWrapper& connection_wrapper );

    bool send( yarrr::Data&& message ) const;

    const std::string name;

    yarrr::Object::Id object_id() const;
    void assign_object( yarrr::Object& object )
    {
      m_current_object = &object;
    }

  private:
    void handle_chat_message( const yarrr::ChatMessage& );

    const int m_network_id;
    const World& m_world;
    ConnectionWrapper& m_connection_wrapper;
    yarrr::Object* m_current_object;
};

}

