#include "../src/player.hpp"
#include "../src/world.hpp"
#include "../src/command_handler.hpp"
#include <yarrr/protocol.hpp>
#include <yarrr/object_container.hpp>
#include <yarrr/chat_message.hpp>
#include <yarrr/command.hpp>
#include <yarrr/mission.hpp>
#include <yarrr/mission_exporter.hpp>
#include <igloo/igloo_alt.h>
#include <yarrr/test_connection.hpp>
#include "test_services.hpp"
#include "test_protocol.hpp"
#include <themodel/json_exporter.hpp>

using namespace igloo;

namespace
{

std::string assigned_id_key_from( const std::string& category )
{
  return "_" + category + "_id";
}

}

Describe( a_player )
{
  yarrr::Hash& player_model()
  {
    AssertThat( services->modell_container.exists( "player", player_name ), Equals( true ) );
    return services->modell_container.create_with_id_if_needed( "player", player_name );
  }

  std::string assigned_model_id( const yarrr::Hash& owner, const std::string& category )
  {
    const auto key( assigned_id_key_from( category ) );
    AssertThat( owner.has( key ), Equals( true ) );
    return owner.get( key );
  }

  yarrr::Hash& get_assigned_model( const yarrr::Hash& from, const std::string& category )
  {
    const auto& assigned_id( from.get( assigned_id_key_from( category ) ) );
    AssertThat( services->modell_container.exists( category, assigned_id ), Equals( true ) );
    return services->modell_container.create_with_id_if_needed( category, assigned_id );
  }

  void assert_nth_model_synchronized_with_id( size_t n, const std::string& category, const std::string& id )
  {
    auto serialized_models( player->connection.entities< yarrr::ModellSerializer >() );
    AssertThat( serialized_models.size(), IsGreaterThan( n ) );
    auto& nth_model( serialized_models.at( n ) );

    AssertThat( nth_model->category(), Equals( category ) );
    AssertThat( nth_model->id(), Equals( id ) );
  }

  void assert_resends_model_if_it_changes( const std::string& category, const std::string& id )
  {
    player->connection.flush_connection();
    AssertThat( services->modell_container.exists( category, id ), Equals( true ) );
    auto& model( services->modell_container.create_with_id_if_needed( category, id ) );
    model[ "a_new_key"] = "a new value";
    assert_nth_model_synchronized_with_id( 0, category, id );
  }

  yarrr::Hash& character_model_of( const std::string& player_name )
  {
    const auto& player_model( services->modell_container.create_with_id_if_needed( "player", player_name ) );
    return get_assigned_model( player_model, "character" );
  }

  void assert_model_of_category_is_assigned_to( const yarrr::Hash& owner, const std::string& category )
  {
    const auto assigned_id( assigned_model_id( owner, category ) );
    AssertThat( services->lua.assert_that(
          the::model::path_from( {
            "modells",
            owner.get( yarrr::model::category ),
            owner.get( yarrr::model::id ),
            assigned_id_key_from( category ) } ) ),
          Equals( true ) );

    AssertThat( services->lua.assert_that( the::model::path_from( {
          "modells", category, assigned_id } ) ),
          Equals( true ) );
  }

  std::string assigned_object_id()
  {
    return assigned_model_id( get_assigned_model( player_model(), "character" ), "object" );
  }

  void set_up_command_handler()
  {
    command_result = std::make_tuple( true, "success" );
    dispatched_player = nullptr;
    dispatched_command = nullptr;
    services->command_handler.register_handler( command_name,
        [ this ]
        ( const yarrr::Command& command, yarrrs::Player& player ) -> yarrrs::CommandHandler::Result
        {
          dispatched_command = &command;
          dispatched_player = &player;
          return command_result;
        } );
  }

  void set_up_mission()
  {
    should_finish_mission = false;
    mission.reset( new yarrr::Mission() );
    mission->add_objective( yarrr::Mission::Objective( "",
          [ this ]( yarrr::Mission& ) -> yarrr::TaskState
          {
              return should_finish_mission ?
                yarrr::succeeded :
                yarrr::ongoing;
          } ) );
    mission_id = mission->id();
    player->player.start_mission( std::move( mission ) );
  }

  void finish_mission()
  {
    should_finish_mission = true;
    player->player.update_missions();
  }

  void set_up_another_player()
  {
    auto& player_modell( services->modell_container.create_with_id_if_needed( "player", another_player_name ) );
    auto& character_modell( services->modell_container.create( "character" ) );
    player_modell[ yarrr::model::character_id ] = character_modell.get( yarrr::model::id );
    original_character_id_of_another_player = character_modell.get( yarrr::model::id );
    another_player = services->create_player( another_player_name );
    services->players[ another_player->connection.connection->id ] = another_player->take_player_ownership();
  }

  void SetUp()
  {
    services = std::make_unique< test::Services >();
    set_up_command_handler();
    player = services->create_player( player_name );

    set_up_another_player();

    ship.reset( new yarrr::Object() );
    was_command_dispatched_to_ship = false;
    ship->dispatcher.register_listener< yarrr::Command >(
        [ this ]( const yarrr::Command& ){ was_command_dispatched_to_ship = true; } );
    player->player.assign_object( *ship );

    set_up_mission();
  }

  void TearDown()
  {
    player.reset();
    another_player.reset();
    services.reset();
  }

  It ( broadcasts_chat_messages )
  {
    player->connection.wrapper.dispatch( yarrr::ChatMessage( "", "" ) );
    AssertThat( another_player->connection.has_no_data(), Equals( false ) );
  }

  It ( sends_object_assigned_to_the_player )
  {
    AssertThat( player->player.object_id(), Equals( ship->id() ) );
    AssertThat( player->connection.has_no_data(), Equals( false ) );

    test::assert_object_assigned( player->connection, ship->id() );
  }

  It ( forwards_commands_from_the_network_to_the_object )
  {
    player->connection.wrapper.dispatch( yarrr::Command() );
    AssertThat( was_command_dispatched_to_ship, Equals( true ) );
  }

  It ( forwards_commands_only_to_the_last_assigned_object )
  {
    yarrr::Object new_ship;
    player->player.assign_object( new_ship );
    player->connection.wrapper.dispatch( yarrr::Command() );
    AssertThat( was_command_dispatched_to_ship, Equals( false ) );
  }

  It( stores_and_exports_started_missions )
  {
    const auto mission_context_path( the::model::index_lua_table( yarrr::mission_contexts, std::to_string( mission_id ) ) );
    AssertThat(
        services->lua.assert_that( mission_context_path ),
        Equals( true ) );
  }

  It( exports_the_current_object_id )
  {
    AssertThat( services->lua.assert_equals( the::model::path_from( {
          the::model::index_lua_table( yarrr::mission_contexts, std::to_string( mission_id ) ),
          "character",
          "object_id" } ), std::to_string( ship->id() ) ), Equals( true ) );
  }

  It( removes_finished_mission_objects )
  {
    finish_mission();
    AssertThat( services->lua.assert_that(
          the::model::index_lua_table( yarrr::mission_contexts, std::to_string( mission_id ) ) ),
          Equals( false ) );
  }

  It( exports_player_model )
  {
    AssertThat( services->lua.assert_that(
          the::model::index_lua_table( yarrrs::Models::players_path, player_name ) ),
          Equals( true ) );
  }

  It( creates_character_model_if_it_did_not_exist_before )
  {
    assert_model_of_category_is_assigned_to( player_model(), "character" );
  }

  It( gives_availability_information_via_the_player_modell )
  {
    const auto& player_modell( services->modell_container.create_with_id_if_needed( "player", player_name ) );
    AssertThat( player_modell.get( yarrr::model::availability ), Equals( "online" ) );

    player.reset();
    AssertThat( player_modell.get( yarrr::model::availability ), Equals( "offline" ) );
  }

  It( creates_the_character_with_the_same_name_as_the_player )
  {
    AssertThat( character_model_of( player_name ).get( yarrr::model::name ), Equals( player_name ) );
  }

  It( does_not_create_a_new_character_if_it_existed_before )
  {
    AssertThat( character_model_of( another_player_name ).get( yarrr::model::id ), Equals( original_character_id_of_another_player ) );
  }

  It ( sends_player_model_to_the_player )
  {
    assert_nth_model_synchronized_with_id( 0, "player", player_name );
  }

  It ( sends_character_model_to_the_player )
  {
    assert_nth_model_synchronized_with_id( 1, "character", assigned_model_id( player_model(), "character" ) );
  }

  It ( sends_permanent_object_model_to_the_player )
  {
    const auto& character_model( character_model_of( player_name ) );
    assert_nth_model_synchronized_with_id( 2, "object", assigned_object_id() );
  }

  It ( resends_player_model_if_it_changes )
  {
    player->connection.flush_connection();
    auto& player_model( services->modell_container.create_with_id_if_needed( "player", player_name ) );
    player_model[ "a_new_key"] = "a new value";
    assert_nth_model_synchronized_with_id( 0, "player", player_name );
  }

  It ( resends_character_model_if_it_changes )
  {
    assert_resends_model_if_it_changes( "character", assigned_model_id( player_model(), "character" ) );
  }

  It ( resends_object_model_if_it_changes )
  {
    assert_resends_model_if_it_changes( "object", assigned_object_id() );
  }

  It( creates_a_permanent_object_if_it_did_not_exist_before )
  {
    auto& character_model( character_model_of( player_name ) );
    AssertThat( character_model.has( yarrr::model::object_id ), Equals( true ) );
  }


  It( creates_the_permanent_object_with_type_player_controlled )
  {
    const auto& character( get_assigned_model( player_model(), "character" ) );
    const auto& assigned_object( get_assigned_model( character, "object" ) );
    AssertThat( assigned_object.get( yarrr::model::object_type ), Equals( "player_controlled" ) );
  }

  It( sends_mission_object_to_the_player_when_updated )
  {
    player->player.update_missions();
    AssertThat( player->connection.get_entity< yarrr::Mission >()->id(), Equals( mission_id ) );
  }

  It( sends_out_the_finished_state_of_the_mission )
  {
    player->connection.flush_connection();
    finish_mission();
    AssertThat( player->connection.has_entity< yarrr::Mission >(), Equals( true ) );
    AssertThat( player->connection.get_entity< yarrr::Mission >()->state(), Equals( yarrr::succeeded ) );
  }

  It( refreshes_exported_object_id_on_missions_when_new_object_is_assigned )
  {
    yarrr::Object new_ship;
    player->player.assign_object( new_ship );
    player->player.update_missions();
    AssertThat( services->lua.assert_equals( the::model::path_from( {
          the::model::index_lua_table( yarrr::mission_contexts, std::to_string( mission_id ) ),
          "character",
          "object_id" } ), std::to_string( new_ship.id() ) ), Equals( true ) );
  }

  It( refreshes_exported_object_id_on_the_permanent_object_as_well )
  {
    yarrr::Object new_ship;
    player->player.assign_object( new_ship );
    const auto& permanent_object(
        get_assigned_model( get_assigned_model( player_model(), "character" ), "object" ) );
    AssertThat( permanent_object.get( yarrr::model::realtime_object_id ), Equals( std::to_string( new_ship.id() ) ) );
  }

  It ( executes_commands_with_the_command_handler )
  {
    player->connection.wrapper.dispatch( command );
    AssertThat( dispatched_command, Equals( &command ) );
    AssertThat( dispatched_player, Equals( &player->player ) );
  }

  It ( sends_no_chat_message_for_a_successfull_command_execution )
  {
    player->connection.flush_connection();
    player->connection.wrapper.dispatch( command );
    AssertThat( player->connection.has_entity< yarrr::ChatMessage >(), Equals( false ) );
  }

  It ( sends_a_chat_message_for_a_failed_command_containing_the_error_message )
  {
    const std::string the_error_message( "an error message" );
    command_result = std::make_tuple( false, the_error_message );
    player->connection.flush_connection();
    player->connection.wrapper.dispatch( command );
    AssertThat( player->connection.get_entity< yarrr::ChatMessage >()->message(), Contains( "failed" ) );
    AssertThat( player->connection.get_entity< yarrr::ChatMessage >()->message(), Contains( command_name ) );
    AssertThat( player->connection.get_entity< yarrr::ChatMessage >()->message(), Contains( the_error_message ) );
  }

  std::unique_ptr< test::Services > services;

  const std::string command_name{ "some strange command" };
  const yarrr::Command command{ { command_name } };
  yarrrs::Player* dispatched_player{ nullptr };
  const yarrr::Command* dispatched_command{ nullptr };
  yarrrs::CommandHandler::Result command_result;

  std::unique_ptr< test::Services::PlayerBundle > player;
  std::unique_ptr< test::Services::PlayerBundle > another_player;

  bool was_command_dispatched_to_ship;
  yarrr::Object::Pointer ship;

  const std::string player_name{ "KilgorTrout" };
  const std::string another_player_name{ "Nakaya" };
  std::string original_character_id_of_another_player;

  yarrr::Mission::Pointer mission;
  yarrr::Mission::Id mission_id;

  bool should_finish_mission;
};

