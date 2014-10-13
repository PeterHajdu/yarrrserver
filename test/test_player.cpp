#include "../src/player.hpp"
#include "../src/world.hpp"
#include "../src/command_handler.hpp"
#include "test_connection.hpp"
#include <yarrr/object_container.hpp>
#include <yarrr/chat_message.hpp>
#include <yarrr/command.hpp>
#include <igloo/igloo_alt.h>

using namespace igloo;

Describe( a_player )
{

  void SetUp()
  {
    players.clear();
    connection.reset( new test::Connection() );
    player.reset( new yarrrs::Player(
          players,
          player_name,
          connection->wrapper,
          command_handler ) );

    another_connection.reset( new test::Connection() );
    another_player = new yarrrs::Player(
          players,
          player_name,
          another_connection->wrapper,
          command_handler );
    players[ another_connection->connection.id ] = yarrrs::Player::Pointer( another_player );

    ship.reset( new yarrr::Object() );
    was_command_dispatched = false;
    ship->dispatcher.register_listener< yarrr::Command >(
        [ this ]( const yarrr::Command& ){ was_command_dispatched = true; } );
  }

  It ( broadcasts_chat_messages )
  {
    AssertThat( another_connection->has_no_data(), Equals( true ) );
    connection->wrapper.dispatch( yarrr::ChatMessage( "", "" ) );
    AssertThat( another_connection->has_no_data(), Equals( false ) );
  }

  It ( sends_object_assigned_to_the_player )
  {
    AssertThat( connection->has_no_data(), Equals( true ) );
    player->assign_object( *ship );
    AssertThat( player->object_id(), Equals( ship->id ) );
    AssertThat( connection->has_no_data(), Equals( false ) );
    AssertThat( connection->get_entity< yarrr::ObjectAssigned >()->object_id(), Equals( ship->id ) );
  }

  It ( forwards_commands_from_the_network_to_the_object )
  {
    player->assign_object( *ship );
    connection->wrapper.dispatch( yarrr::Command() );
    AssertThat( was_command_dispatched, Equals( true ) );
  }

  It ( forwards_commands_only_to_the_last_assigned_object )
  {
    player->assign_object( *ship );
    yarrr::Object new_ship;
    player->assign_object( new_ship );
    connection->wrapper.dispatch( yarrr::Command() );
    AssertThat( was_command_dispatched, Equals( false ) );
  }

  It ( executes_commands_with_the_command_handler )
  {
    bool was_handler_called{ false };
    command_handler.register_handler( "test",
        [ &was_handler_called ]( const yarrr::Command&, yarrrs::Player& )
        { was_handler_called = true; } );
    connection->wrapper.dispatch( yarrr::Command( { "test", "parameter" } ) );
    AssertThat( was_handler_called, Equals( true ) );
  }

  std::unique_ptr< test::Connection > connection;
  yarrrs::Player::Pointer player;

  std::unique_ptr< test::Connection > another_connection;
  yarrrs::Player* another_player;

  yarrrs::Player::Container players;

  bool was_command_dispatched;
  yarrr::Object::Pointer ship;

  const std::string player_name{ "Kilgor Trout" };

  yarrrs::CommandHandler command_handler;
};

