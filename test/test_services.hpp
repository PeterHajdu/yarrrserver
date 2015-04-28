#pragma once

#include <yarrr/id_generator.hpp>
#include <yarrr/engine_dispatcher.hpp>
#include <yarrr/object_container.hpp>
#include <yarrr/main_thread_callback_queue.hpp>
#include <yarrr/test_db.hpp>
#include <yarrr/modell.hpp>
#include <themodel/lua.hpp>
#include <themodel/node_list.hpp>
#include "../src/models.hpp"
#include "../src/player.hpp"
#include "../src/command_handler.hpp"
#include "../src/local_event_dispatcher.hpp"
#include "../src/world.hpp"
#include <yarrr/test_connection.hpp>
#include <sstream>

namespace test
{

class Services
{
  public:
    Services();
    the::model::Lua lua;
    the::ctci::AutoServiceRegister< yarrr::EngineDispatcher, yarrr::EngineDispatcher > engine_dispatcher_register;
    yarrr::EngineDispatcher& engine_dispatcher;
    the::ctci::AutoServiceRegister< LocalEventDispatcher, LocalEventDispatcher > local_event_dispatcher_register;
    LocalEventDispatcher& local_event_dispatcher;
    the::ctci::AutoServiceRegister< yarrrs::Models, yarrrs::Models > models_register;
    yarrrs::Models& models;
    the::ctci::AutoServiceRegister< yarrr::MainThreadCallbackQueue, yarrr::MainThreadCallbackQueue > callback_queue_register;
    yarrr::MainThreadCallbackQueue& main_thread_callback_queue;
    yarrrs::Player::Container players;
    yarrr::ObjectContainer objects;
    yarrrs::CommandHandler command_handler;
    yarrrs::World world;
    yarrr::IdGenerator id_generator;
    test::Db database;
    the::ctci::AutoServiceRegister< yarrr::ModellContainer, yarrr::ModellContainer > modell_container_register;
    yarrr::ModellContainer& modell_container;

    ~Services()
    {
      main_thread_callback_queue.process_callbacks();
    }

    class PlayerBundle
    {
      public:
        PlayerBundle(
            const yarrrs::Player::Container& players,
            const std::string& name,
            const yarrrs::CommandHandler& command_handler )
          : m_owned_connection( std::make_unique< test::Connection >() )
          , m_owned_player( std::make_unique< yarrrs::Player >(
              players,
              name,
              m_owned_connection->wrapper,
              command_handler ) )
          , connection( *m_owned_connection )
          , player( *m_owned_player )
        {
        }

        yarrrs::Player::Pointer take_player_ownership()
        {
          return std::move( m_owned_player );
        }

        PlayerBundle(
            std::unique_ptr< test::Connection >&& connection,
            yarrrs::Player& player )
          : m_owned_connection( std::move( connection ) )
          , connection( *m_owned_connection )
          , player( player )
        {
        }

        using Pointer = std::unique_ptr< PlayerBundle >;

      private:
        std::unique_ptr< test::Connection > m_owned_connection;
        yarrrs::Player::Pointer m_owned_player;

      public:
        test::Connection& connection;
        yarrrs::Player& player;
    };

    auto create_player( const std::string& name )
    {
      return std::make_unique< PlayerBundle >( players, name, command_handler );
    }

    auto log_in_player( std::string player_name )
    {
      auto connection( std::make_unique< test::Connection >() );
      local_event_dispatcher.dispatcher.dispatch(
          yarrrs::PlayerLoggedIn(
            connection->wrapper,
            connection->connection->id,
            std::move( player_name ) ) );
      main_thread_callback_queue.process_callbacks();
      assert( !players.empty() );
      auto& player( *std::begin( players )->second.get() );
      return std::make_unique< PlayerBundle >( std::move( connection ), player );
    }
};

std::stringstream& get_notification_stream();

}

