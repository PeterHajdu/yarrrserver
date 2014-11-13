#include "../src/command_handler.hpp"
#include "../src/player.hpp"
#include "test_connection.hpp"
#include <yarrr/command.hpp>
#include <igloo/igloo_alt.h>

using namespace igloo;

Describe( a_command_handler )
{
  void SetUp()
  {
    command_handler.reset( new yarrrs::CommandHandler() );

    passed_command = nullptr;
    passed_player = nullptr;

    was_a_command_handler_executed = false;
    command_handler->register_handler( a_command.command(),
        [ this ]( const yarrr::Command& command, yarrrs::Player& player )
        {
          was_a_command_handler_executed = true;
          passed_command = &command;
          passed_player = &player;
        } );


    was_another_command_handler_executed = false;
    command_handler->register_handler( another_command.command(),
        [ this ]( const yarrr::Command&, yarrrs::Player& )
        { was_another_command_handler_executed = true; } );

    player.reset( new yarrrs::Player( players, "a player", connection.wrapper, dummy ) );
  }

  It ( executes_only_the_command_handler_of_the_given_command )
  {
    command_handler->execute( a_command, *player );
    AssertThat( was_a_command_handler_executed, Equals( true ) );
    AssertThat( was_another_command_handler_executed, Equals( false ) );

    command_handler->execute( another_command, *player );
    AssertThat( was_another_command_handler_executed, Equals( true ) );
  }

  It ( passes_the_command_to_the_executor )
  {
    command_handler->execute( a_command, *player );
    AssertThat( passed_command, Equals( &a_command ) );
  }

  It ( passes_the_player_to_the_executor )
  {
    command_handler->execute( a_command, *player );
    AssertThat( passed_player, Equals( player.get() ) );
  }

  It ( handles_unknown_commands )
  {
    command_handler->execute( yarrr::Command( { "unknown" } ), *player );
  }

  std::unique_ptr< yarrrs::CommandHandler > command_handler;
  const yarrr::Command* passed_command;
  bool was_a_command_handler_executed;
  yarrr::Command a_command{ { "a command", "parameter of command" } };
  bool was_another_command_handler_executed;
  yarrr::Command another_command{ { "another command", "parameter of another command" } };

  yarrrs::Player::Container players;
  test::Connection connection;
  yarrrs::CommandHandler dummy;
  std::unique_ptr< yarrrs::Player > player;
  yarrrs::Player* passed_player;
};

