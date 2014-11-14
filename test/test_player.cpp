#include "../src/player.hpp"
#include "../src/world.hpp"
#include "../src/command_handler.hpp"
#include "test_connection.hpp"
#include <yarrr/object_container.hpp>
#include <yarrr/chat_message.hpp>
#include <yarrr/command.hpp>
#include <yarrr/mission.hpp>
#include <themodel/node_list.hpp>
#include <igloo/igloo_alt.h>

using namespace igloo;

Describe( a_player )
{

  void set_up_mission()
  {
    finish_mission = false;
    mission.reset( new yarrr::Mission() );
    mission->add_objective( yarrr::Mission::Objective( "",
          [ this ]( const std::string& ) -> yarrr::TaskState
          {
              return finish_mission ?
                yarrr::succeeded :
                yarrr::ongoing;
          } ) );
    mission_id = mission->id();
    player->start_mission( std::move( mission ) );
  }

  void SetUp()
  {
    set_up_command_handler();
    lua = &yarrr::LuaEngine::model();
    players.clear();
    connection.reset( new test::Connection() );
    player.reset( new yarrrs::Player(
          players,
          player_name,
          connection->wrapper,
          *command_handler ) );

    another_connection.reset( new test::Connection() );
    another_player = new yarrrs::Player(
          players,
          player_name,
          another_connection->wrapper,
          *command_handler );
    players[ another_connection->connection.id ] = yarrrs::Player::Pointer( another_player );

    ship.reset( new yarrr::Object() );
    was_command_dispatched = false;
    ship->dispatcher.register_listener< yarrr::Command >(
        [ this ]( const yarrr::Command& ){ was_command_dispatched = true; } );
    player->assign_object( *ship );

    set_up_mission();
  }

  void TearDown()
  {
    player.reset();
  }

  It ( broadcasts_chat_messages )
  {
    connection->wrapper.dispatch( yarrr::ChatMessage( "", "" ) );
    AssertThat( another_connection->has_no_data(), Equals( false ) );
  }

  It ( sends_object_assigned_to_the_player )
  {
    AssertThat( player->object_id(), Equals( ship->id() ) );
    AssertThat( connection->has_no_data(), Equals( false ) );
    AssertThat( connection->get_entity< yarrr::ObjectAssigned >()->object_id(), Equals( ship->id() ) );
  }

  It ( forwards_commands_from_the_network_to_the_object )
  {
    connection->wrapper.dispatch( yarrr::Command() );
    AssertThat( was_command_dispatched, Equals( true ) );
  }

  It ( forwards_commands_only_to_the_last_assigned_object )
  {
    yarrr::Object new_ship;
    player->assign_object( new_ship );
    connection->wrapper.dispatch( yarrr::Command() );
    AssertThat( was_command_dispatched, Equals( false ) );
  }

  It( stores_and_exports_started_missions )
  {
    AssertThat( lua->assert_that( the::model::index_lua_table( "missions", std::to_string( mission_id ) )  ), Equals( true ) );
  }

  It( exports_the_current_object_id )
  {
    AssertThat( lua->assert_equals( the::model::path_from( {
          the::model::index_lua_table( "missions", std::to_string( mission_id ) ),
          "character",
          "object_id" } ), std::to_string( ship->id() ) ), Equals( true ) );
  }

  It( removes_finished_mission_objects )
  {
    finish_mission = true;
    player->update_missions();
    AssertThat( lua->assert_that(
          the::model::index_lua_table( "missions", std::to_string( mission_id ) ) ),
          Equals( false ) );
  }

  It( sends_mission_object_to_the_player_when_updated )
  {
    player->update_missions();
    AssertThat( connection->get_entity< yarrr::Mission >()->id(), Equals( mission_id ) );
  }

  It( sends_out_the_finished_state_of_the_mission )
  {
    connection->flush_connection();
    finish_mission = true;
    player->update_missions();
    AssertThat( connection->has_entity< yarrr::Mission >(), Equals( true ) );
    AssertThat( connection->get_entity< yarrr::Mission >()->state(), Equals( yarrr::succeeded ) );
  }

  It( refreshes_exported_object_id_on_missions_when_new_object_is_assigned )
  {
    yarrr::Object new_ship;
    player->assign_object( new_ship );
    player->update_missions();
    AssertThat( lua->assert_equals( the::model::path_from( {
          the::model::index_lua_table( "missions", std::to_string( mission_id ) ),
          "character",
          "object_id" } ), std::to_string( new_ship.id() ) ), Equals( true ) );
  }

  void set_up_command_handler()
  {
    command_result = std::make_tuple( true, "success" );
    dispatched_player = nullptr;
    dispatched_command = nullptr;
    command_handler.reset( new yarrrs::CommandHandler() );
    command_handler->register_handler( command_name,
        [ this ]
        ( const yarrr::Command& command, yarrrs::Player& player ) -> yarrrs::CommandHandler::Result
        {
          dispatched_command = &command;
          dispatched_player = &player;
          return command_result;
        } );
  }

  It ( executes_commands_with_the_command_handler )
  {
    connection->wrapper.dispatch( command );
    AssertThat( dispatched_command, Equals( &command ) );
    AssertThat( dispatched_player, Equals( player.get() ) );
  }

  It ( sends_no_chat_message_for_a_successfull_command_execution )
  {
    connection->flush_connection();
    connection->wrapper.dispatch( command );
    AssertThat( connection->has_entity< yarrr::ChatMessage >(), Equals( false ) );
  }

  It ( sends_a_chat_message_for_a_failed_command_containing_the_error_message )
  {
    const std::string the_error_message( "an error message" );
    command_result = std::make_tuple( false, the_error_message );
    connection->flush_connection();
    connection->wrapper.dispatch( command );
    AssertThat( connection->get_entity< yarrr::ChatMessage >()->message(), Contains( "failed" ) );
    AssertThat( connection->get_entity< yarrr::ChatMessage >()->message(), Contains( command_name ) );
    AssertThat( connection->get_entity< yarrr::ChatMessage >()->message(), Contains( the_error_message ) );
  }

  const std::string command_name{ "some strange command" };
  const yarrr::Command command{ { command_name } };
  yarrrs::Player* dispatched_player{ nullptr };
  const yarrr::Command* dispatched_command{ nullptr };
  std::unique_ptr< yarrrs::CommandHandler > command_handler;
  yarrrs::CommandHandler::Result command_result;

  std::unique_ptr< test::Connection > connection;
  yarrrs::Player::Pointer player;

  std::unique_ptr< test::Connection > another_connection;
  yarrrs::Player* another_player;

  yarrrs::Player::Container players;

  bool was_command_dispatched;
  yarrr::Object::Pointer ship;

  const std::string player_name{ "Kilgor Trout" };


  the::model::Lua* lua;

  yarrr::Mission::Pointer mission;
  yarrr::Mission::Id mission_id;

  bool finish_mission;
};

