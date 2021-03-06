#include "world.hpp"
#include "player.hpp"
#include "local_event_dispatcher.hpp"
#include <thectci/service_registry.hpp>

#include <yarrr/object_factory.hpp>
#include <yarrr/object_created.hpp>
#include <yarrr/object_identity.hpp>
#include <yarrr/mission_factory.hpp>
#include <yarrr/log.hpp>
#include <yarrr/object_container.hpp>
#include <yarrr/engine_dispatcher.hpp>
#include <yarrr/main_thread_callback_queue.hpp>
#include <yarrr/delete_object.hpp>
#include <yarrr/destruction_handlers.hpp>
#include <yarrr/chat_message.hpp>
#include <yarrr/command.hpp>
#include <yarrr/basic_behaviors.hpp>
#include <yarrr/modell.hpp>
#include <sstream>

namespace
{

template < typename T >
T
string_to( const std::string& from )
{
  std::stringstream str( from );
  T to;
  str >> to;
  return to;
}

std::string
has_key_or( const yarrr::Hash& hash, const std::string& key, const std::string& fallback )
{
  return hash.has( key ) ?
    hash.get( key ) :
    fallback;
}

void
synchronize_realtime_and_permanent_objects(
    yarrr::Object& realtime,
    yarrr::Hash& permanent )
{
  permanent[ yarrr::model::realtime_object_id ] = std::to_string( realtime.id() );
  auto& physical_parameters( yarrr::component_of< yarrr::PhysicalBehavior >( realtime ).physical_parameters );
  physical_parameters.coordinate.x = string_to< yarrr::Coordinate::type >(
      has_key_or( permanent, yarrr::model::hidden_x, "0" ) );
  physical_parameters.coordinate.y = string_to< yarrr::Coordinate::type >(
      has_key_or( permanent, yarrr::model::hidden_y, "0" ) );
  physical_parameters.angular_velocity = string_to< yarrr::Angle >(
      has_key_or( permanent, yarrr::model::hidden_angular_velocity, "0" ) );
}


void
create_permanent_objects( yarrr::ObjectContainer& realtime_objects )
{
  auto& models( the::ctci::service< yarrr::ModellContainer >() );
  const auto& objects( models.get( "object" ) );
  for ( const auto& object : objects )
  {
    auto& object_model( *object.second );
    if ( object_model.has( yarrr::model::object_type ) && object_model.get( yarrr::model::object_type ) == yarrr::model::player_controlled )
    {
      continue;
    }

    const auto ship_type( has_key_or( object_model, yarrr::model::ship_type, "ship" ) );
    yarrr::Object::Pointer realtime_object( the::ctci::service< yarrr::ObjectFactory >().create_a( ship_type ) );
    synchronize_realtime_and_permanent_objects( *realtime_object, object_model );
    realtime_objects.add_object( std::move( realtime_object ) );
  }
}

yarrr::Object::Pointer
create_player_ship( const std::string& type, const std::string& player_name_as_captain )
{
  yarrr::Object::Pointer new_ship( the::ctci::service< yarrr::ObjectFactory >().create_a( type ) );
  if ( !new_ship )
  {
    return nullptr;
  }

  new_ship->add_behavior( yarrr::kill_player_when_destroyed() );
  new_ship->add_behavior( std::make_unique< yarrr::ObjectIdentity >( player_name_as_captain ) );
  return new_ship;
}

std::string
concatenate_list( const std::string& start, const yarrr::MissionFactory::MissionList& list )
{
  std::string missions( start );
  for ( const auto& mission : list )
  {
    missions += mission;
    missions += ", ";
  }
  return missions;
}

void send_help_message_to( yarrrs::Player& player )
{
  player.send( yarrr::ChatMessage(
        "commands: /mission list, /mission request <mission name>, /ship list, /ship request <object type>",
        "server" ).serialize() );

  player.send( yarrr::ChatMessage(
        "Welcome to yarrr. If this is the first time you log in to yarrr type in the following command: /mission request tutorial",
        "server" ).serialize() );

  player.send( yarrr::ChatMessage(
        "You can always roll back to this text with the page up key. Fair winds and following seas.",
        "server" ).serialize() );
}

void
add_command_handlers_to(
    yarrrs::CommandHandler& command_handler,
    yarrr::ObjectContainer& objects,
    const yarrrs::Player::Container& players )
{
  //todo: clean up command handlers
  command_handler.register_handler( "ship",
      [ &objects, &players ]( const yarrr::Command& command, yarrrs::Player& player ) -> yarrrs::CommandHandler::Result
      {
        const auto& parameters( command.parameters() );
        if ( parameters.size() < 1 )
        {
          thelog( yarrr::log::warning )( "Invalid ship command from", player.name );
          return yarrrs::CommandHandler::Result( false, "Invalid ship command." );
        }

        const auto sub_command( command.parameters().at( 0 ) );

        if ( sub_command == "list" )
        {
          const auto& object_list( the::ctci::service< yarrr::ObjectFactory >().objects() );
          player.send( yarrr::ChatMessage(
                concatenate_list( "Registered object types:", object_list ), "server" ).serialize() );
          return yarrrs::CommandHandler::Result( true, "" );
        }


        if ( sub_command != "request" )
        {
          thelog( yarrr::log::warning )( "Invalid ship command from", player.name );
          return yarrrs::CommandHandler::Result( false, "Unknown subcommand: " + sub_command );
        }

        if ( command.parameters().size() < 2 )
        {
          thelog( yarrr::log::warning )( "Invalid ship request from", player.name );
          return yarrrs::CommandHandler::Result( false, "Invalid ship request. Please define ship type." );
        }

        const std::string requested_ship_type( command.parameters().at( 1 ) );
        thelog( yarrr::log::info )( "Ship type requested", requested_ship_type, "by", player.name );
        yarrr::Object::Pointer new_ship( create_player_ship( requested_ship_type, player.name ) );

        if ( !new_ship )
        {
          thelog( yarrr::log::info )( "Unable to create ship", requested_ship_type, "for", player.name );
          return yarrrs::CommandHandler::Result( false, "Unknown ship type: " + requested_ship_type );
        }

        yarrrs::broadcast( players, yarrr::DeleteObject( player.object_id() ) );
        objects.delete_object( player.object_id() );
        player.assign_object( *new_ship );
        objects.add_object( std::move( new_ship ) );
        return yarrrs::CommandHandler::Result( true, "" );
      } );

  command_handler.register_handler( "mission",
      [ &objects, &players ]( const yarrr::Command& command, yarrrs::Player& player ) -> yarrrs::CommandHandler::Result
      {
        const auto& parameters( command.parameters() );
        if ( parameters.size() < 1 )
        {
          return yarrrs::CommandHandler::Result( false, "Invalid mission command." );
        }

        const auto sub_command( command.parameters()[ 0 ] );
        yarrr::MissionFactory& mission_factory{ the::ctci::service< yarrr::MissionFactory >() };

        if ( sub_command == "list" )
        {
          player.send( yarrr::ChatMessage(
                concatenate_list( "Registered missions: ", mission_factory.missions() ), "server" ).serialize() );
          return yarrrs::CommandHandler::Result( true, "" );
        }

        if ( "request" != sub_command )
        {
          return yarrrs::CommandHandler::Result( false, "Unknown subcommand." );
        }

        if ( parameters.size() < 2 )
        {
          return yarrrs::CommandHandler::Result( false, "Invalid mission request." );
        }

        const auto requested_mission( command.parameters()[ 1 ] );
        auto new_mission( mission_factory.create_a( requested_mission ) );
        if ( !new_mission )
        {
          return yarrrs::CommandHandler::Result( false, "Unknown mission type: " + requested_mission );
        }

        player.start_mission( std::move( new_mission ) );
        return yarrrs::CommandHandler::Result( true, "" );
      } );
}


yarrrs::Player*
player_with_object_id( yarrrs::Player::Container& players, yarrr::Object::Id id )
{
  for ( const auto& player : players )
  {
    if ( player.second->object_id() == id )
    {
      return player.second.get();
    }
  }

  return nullptr;
}

yarrrs::Player*
player_with_name( yarrrs::Player::Container& players, const std::string& name )
{
  for ( const auto& player : players )
  {
    if ( player.second->name == name )
    {
      return player.second.get();
    }
  }

  return nullptr;
}

}

namespace yarrrs
{

//todo: tear this up to separate handlers
World::World( Player::Container& players, yarrr::ObjectContainer& objects )
  : m_players( players )
  , m_objects( objects )
{
  the::ctci::Dispatcher& local_event_dispatcher(
      the::ctci::service< LocalEventDispatcher >().dispatcher );

  local_event_dispatcher.register_listener< PlayerLoggedIn >(
      [ this ]( const PlayerLoggedIn& logged_in ){ handle_player_logged_in( logged_in ); } );

  local_event_dispatcher.register_listener< PlayerLoggedOut >(
      [ this ]( const PlayerLoggedOut& logged_out ){ handle_player_logged_out( logged_out ); } );

  the::ctci::Dispatcher& engine_dispatcher(
      the::ctci::service< yarrr::EngineDispatcher >() );

  engine_dispatcher.register_listener< yarrr::ObjectCreated >(
      [ this ]( const yarrr::ObjectCreated& created ){ handle_object_created( created ); } );

  engine_dispatcher.register_listener< yarrr::DeleteObject >(
      [ this ]( const yarrr::DeleteObject& deleted ){ handle_delete_object( deleted ); } );

  engine_dispatcher.register_listener< yarrr::PlayerKilled >(
      [ this ]( const yarrr::PlayerKilled& killed ){ handle_player_killed( killed ); } );

  add_command_handlers_to( m_command_handler, m_objects, m_players );
  create_permanent_objects( objects );
}

void
World::handle_player_killed( const yarrr::PlayerKilled& player_killed ) const
{
  Player* player( player_with_object_id( m_players, player_killed.object_id ) );
  if ( !player )
  {
    thelog( yarrr::log::error )( "Unable to find killed player." );
    return;
  }

  thelog( yarrr::log::debug )( "Player killed.", player->name );

  yarrr::Object::Pointer new_object( create_player_ship( "ship", player->name ) );
  if ( !new_object )
  {
    thelog( yarrr::log::error )( "Unable to create ship." );
    return;
  }

  player->player_killed();
  player->assign_object( *new_object );
  add_object( std::move( new_object ) );
  delete_object( player_killed.object_id );
}

void
World::handle_delete_object( const yarrr::DeleteObject& del_object ) const
{
  thelog( yarrr::log::debug )( __PRETTY_FUNCTION__, del_object.object_id() );
  delete_object( del_object.object_id() );
}

void
World::add_object( yarrr::Object::Pointer&& object_ptr ) const
{
  //todo: fix this abomination
  yarrr::Object* object( object_ptr.release() );
  the::ctci::service< yarrr::MainThreadCallbackQueue >().push_back(
      [ this, object ]()
      {
        m_objects.add_object( yarrr::Object::Pointer( object ) );
      } );
}

void
World::delete_object( yarrr::Object::Id id ) const
{
  thelog( yarrr::log::debug )( "Deleting object", id );
  the::ctci::service< yarrr::MainThreadCallbackQueue >().push_back(
      [ this, id ]()
      {
        yarrrs::broadcast( m_players, yarrr::DeleteObject( id ) );
        m_objects.delete_object( id );
      } );
}

void
World::handle_object_created( const yarrr::ObjectCreated& object_created ) const
{
  //todo: fix this abomination
  yarrr::Object* object( object_created.object.release() );
  the::ctci::service< yarrr::MainThreadCallbackQueue >().push_back(
      [ this, object ]()
      {
        m_objects.add_object( yarrr::Object::Pointer( object ) );
      } );
}

void
World::handle_player_logged_in( const PlayerLoggedIn& login ) const
{
  thelog_trace( yarrr::log::info, __PRETTY_FUNCTION__ );

  if ( player_with_name( m_players, login.name ) != nullptr )
  {
    thelog( yarrr::log::warning )( "User is already logged in:", login.name );
    return;
  }

  yarrr::Object::Pointer new_object( create_player_ship( "ship", login.name ) );
  if ( !new_object )
  {
    thelog( yarrr::log::error )( "Unable to create ship for new user:", login.name );
    return;
  }

  m_players.emplace( std::make_pair(
        login.id,
        Player::Pointer( new Player(
            m_players,
            login.name,
            login.connection_wrapper,
            m_command_handler ) ) ) );

  Player& new_player( *m_players[ login.id ] );
  send_help_message_to( new_player );
  new_player.assign_object( *new_object );
  m_objects.add_object( std::move( new_object ) );

  const std::string notification( std::string( "Player logged in: " ) + login.name );
  thelog( yarrr::log::info )( notification );
  yarrrs::broadcast( m_players, yarrr::ChatMessage( notification, "server" ) );
}


void
World::handle_player_logged_out( const PlayerLoggedOut& logout ) const
{
  thelog_trace( yarrr::log::info, __PRETTY_FUNCTION__ );
  const auto player( m_players.find( logout.id ) );
  if ( player == std::end( m_players ) )
  {
    thelog( yarrr::log::warning )( "Unknown player logged out." );
    return;
  }

  const auto player_name( player->second->name );
  const auto object_id( player->second->object_id() );
  m_players.erase( logout.id );

  thelog( yarrr::log::warning )( "Deleting player and object.", object_id, player_name );
  yarrrs::broadcast( m_players, yarrr::ChatMessage( "Player logged out: " + player_name, "server" ) );
  delete_object( object_id );
}


}

