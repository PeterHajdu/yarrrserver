#include "player.hpp"
#include "command_handler.hpp"
#include "network_service.hpp"

#include <yarrr/object.hpp>
#include <yarrr/protocol.hpp>
#include <yarrr/chat_message.hpp>
#include <yarrr/log.hpp>
#include <yarrr/command.hpp>
#include <yarrr/mission_exporter.hpp>

#include <yarrr/command.hpp>

#include <thectci/service_registry.hpp>

namespace
{

yarrr::Hash&
assign_new_modell_if_needed_to( yarrr::Hash& player_modell, const std::string& category )
{
  const std::string category_id_key{ category + "_id" };
  if ( player_modell.has( category_id_key ) )
  {
    auto assigned_modell_id( player_modell[ category_id_key ].get() );
    return the::ctci::service< yarrr::ModellContainer >().create_with_id_if_needed(
        category,
        assigned_modell_id );
  }

  auto& assigned_modell( the::ctci::service< yarrr::ModellContainer >().create( category ) );
  player_modell[ category_id_key ] = assigned_modell.get( "id" );
  return assigned_modell;
}

void
create_assorted_modells_if_needed( yarrr::Hash& player_modell )
{
  auto& character_modell( assign_new_modell_if_needed_to( player_modell, "character" ) );
  character_modell[ "name" ] = player_modell.get( "id" );
}

yarrr::Hash&
create_permanent_object_if_needed_for(  yarrr::Hash& player_modell )
{
  auto& object_modell( assign_new_modell_if_needed_to( player_modell, "object" ) );
  object_modell[ "type" ] = "player_controlled";
  return object_modell;
}

}

namespace yarrrs
{

Player::Player(
    const Container& players,
    const std::string& name,
    ConnectionWrapper& connection_wrapper,
    const CommandHandler& command_handler )
  : name( name )
  , m_players( players )
  , m_connection_wrapper( connection_wrapper )
  , m_current_object( nullptr )
  , m_mission_contexts( the::ctci::service< yarrrs::Models >().mission_contexts )
  , m_missions( std::bind( &Player::handle_mission_finished, this, std::placeholders::_1 ) )
  , m_command_handler( command_handler )
  , m_player_modell( the::ctci::service< yarrr::ModellContainer >().create_with_id_if_needed( "player", name ) )
  , m_permanent_object_modell( create_permanent_object_if_needed_for( m_player_modell ) )
{
  m_player_modell[ "availability" ] = "online";
  create_assorted_modells_if_needed( m_player_modell );
  connection_wrapper.register_listener< yarrr::ChatMessage >(
      std::bind( &Player::handle_chat_message, this, std::placeholders::_1 ) );

  thelog( yarrr::log::debug )( "registering command callback for", this );
  connection_wrapper.register_listener< yarrr::Command >(
      std::bind( &Player::handle_command, this, std::placeholders::_1 ) );
}

Player::~Player()
{
  m_player_modell[ "availability" ] = "offline";
}

void
Player::handle_command( const yarrr::Command& command )
{
  const auto result( m_command_handler.execute( command, *this ) );
  const bool did_succeed( std::get< 0 >( result ) );
  if ( did_succeed )
  {
    return;
  }

  const std::string error_message( "Command failed: " + command.command() + " | " + std::get< 1 >( result ) );
  send( yarrr::ChatMessage( error_message, "server" ).serialize() );
}

bool
Player::send( yarrr::Data&& message ) const
{
  return m_connection_wrapper.connection->send( std::move( message ) );
}

void
Player::handle_chat_message( const yarrr::ChatMessage& chat_message )
{
  const yarrr::Data message( chat_message.serialize() );
  for ( const auto& player : m_players )
  {
    player.second->send( yarrr::Data( message ) );
  }
}

yarrr::Object::Id
Player::object_id() const
{
  return m_current_object->id();
}

void
Player::assign_object( yarrr::Object& object )
{
  if ( m_current_object )
  {
    m_connection_wrapper.remove_dispatcher( m_current_object->dispatcher );
  }

  m_current_object = &object;
  m_permanent_object_modell[ "realtime_object_id" ] = std::to_string( object.id() );
  send( yarrr::Command( { yarrr::Protocol::object_assigned, std::to_string( object.id() ) } ).serialize() );
  thelog( yarrr::log::debug )( "Assigning object to user.", object.id(), name );
  m_connection_wrapper.register_dispatcher( object.dispatcher );
  refresh_mission_models();
}


void
Player::refresh_mission_models()
{
  thelog( yarrr::log::debug )( "Refreshing mission models." );
  m_own_mission_contexts.clear();
  const yarrr::Object::Id current_object( object_id() );
  for ( const auto& mission : m_missions.missions() )
  {
    thelog( yarrr::log::debug )( "Refreshing mission model:", mission->id() );
    add_mission_model_of( *mission, current_object );
  }
}


void
Player::start_mission( yarrr::Mission::Pointer&& mission )
{
  add_mission_model_of( *mission, object_id() );
  m_missions.add_mission( std::move( mission ) );
}

void
Player::add_mission_model_of( const yarrr::Mission& mission, const yarrr::Object::Id& current_object )
{
  m_own_mission_contexts[ mission.id() ] = std::make_unique< yarrr::MissionModel >(
          std::to_string( mission.id() ),
          std::to_string( current_object ),
          m_mission_contexts );
}

void
Player::update_missions()
{
  m_missions.update();
  for ( const auto& mission : m_missions.missions() )
  {
    send( mission->serialize() );
  }
}

void
Player::handle_mission_finished( const yarrr::Mission& mission )
{
  send( mission.serialize() );
  m_own_mission_contexts.erase( mission.id() );
}

void
broadcast( const Player::Container& players, const yarrr::Entity& entity )
{
  const yarrr::Data message( entity.serialize() );
  for ( const auto& player : players )
  {
    player.second->send( yarrr::Data( message ) );
  }
}


void
Player::player_killed()
{
  m_missions.fail_missions();
}

}

