#pragma once

#include "network_service.hpp"
#include "models.hpp"
#include <memory>
#include <unordered_map>
#include <yarrr/mission.hpp>
#include <yarrr/mission_container.hpp>
#include <yarrr/mission_exporter.hpp>
#include <yarrr/object.hpp>
#include <themodel/node_list.hpp>

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

    void start_mission( yarrr::Mission::Pointer&& );
    void update_missions();
    void player_killed();

  private:
    void refresh_mission_models();
    void add_mission_model_of( const yarrr::Mission&, const yarrr::Object::Id& );
    void handle_command( const yarrr::Command& );
    void handle_chat_message( const yarrr::ChatMessage& );
    void handle_mission_finished( const yarrr::Mission& );

    const Container& m_players;
    ConnectionWrapper& m_connection_wrapper;
    yarrr::Object* m_current_object;
    Models::MissionContexts& m_mission_contexts;
    yarrr::MissionContainer m_missions;
    const CommandHandler& m_command_handler;

    using MissionModelContainer = std::unordered_map< yarrr::Mission::Id, std::unique_ptr< yarrr::MissionModel > >;
    MissionModelContainer m_own_mission_contexts;

    Models::Players& m_players_model;
    the::model::Variable< std::string > m_player_model;
};

void broadcast( const Player::Container& players, const yarrr::Entity& entity );

}

