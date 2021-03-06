#include "network_service.hpp"
#include "player.hpp"
#include "world.hpp"
#include "models.hpp"
#include "redis.hpp"

#include <yarrr/lua_setup.hpp>
#include <yarrr/object_container.hpp>
#include <yarrr/timer_update.hpp>
#include <yarrr/resources.hpp>
#include <yarrr/main_thread_callback_queue.hpp>
#include <yarrr/log.hpp>
#include <yarrr/object_exporter.hpp>
#include <yarrr/mission_exporter.hpp>
#include <yarrr/clock_exporter.hpp>
#include <yarrr/id_generator.hpp>
#include <yarrr/modell.hpp>

#include <thetime/frequency_stabilizer.hpp>
#include <thetime/clock.hpp>
#include <thetime/once_in.hpp>
#include <thectci/service_registry.hpp>
#include <theconf/configuration.hpp>
#include <themodel/zmq_remote.hpp>
#include <themodel/json_exporter.hpp>
#include <thenet/address.hpp>

#include <iostream>
#include <fstream>

#include <stdlib.h>

namespace
{

std::unique_ptr< the::model::ZmqRemote >
create_remote_model_endpoint_if_needed()
{
  const auto remote_model_endpoint_key( "remote-model-endpoint" );
  if ( !the::conf::has( remote_model_endpoint_key ) )
  {
    return nullptr;
  }

  return std::make_unique< the::model::ZmqRemote >(
    the::conf::get<std::string>( remote_model_endpoint_key ).c_str(),
    yarrr::LuaEngine::model(),
    the::model::export_json );
}

void
send_update_messages_from(
    const yarrr::ObjectContainer& objects,
    yarrrs::Player::Container& players )
{
  std::vector< yarrr::ObjectUpdate::Pointer > object_updates( objects.generate_object_updates() );
  std::vector< yarrr::Data > update_messages;
  for ( const auto& update : object_updates )
  {
    const yarrr::Data message( update->serialize() );

    for ( const auto& player : players )
    {
      player.second->send( yarrr::Data( message ) );
    }

  }
}


void
print_help_and_exit()
{
  std::cout << "usage: yarrrserver --port <port>" << std::endl;
  std::cout << "  --loglevel <int>" << std::endl;
  std::cout << "  --redis_url <ip:port>" << std::endl;
  exit( 0 );
}


void
parse_and_handle_configuration( const the::conf::ParameterVector& parameters )
{
  the::conf::parse( parameters );
  if ( !the::conf::has( "port" ) )
  {
    print_help_and_exit();
  }

  const int loglevel(
      the::conf::has( "loglevel" ) ?
      the::conf::get<int>( "loglevel" ) :
      yarrr::log::info );

  the::log::Logger::set_loglevel( loglevel );
  thelog( yarrr::log::info )( "Loglevel is set to", loglevel );

  if ( the::conf::has( "redis_url" ) )
  {
    const the::net::Address redis_address{ the::conf::get_value( "redis_url" ) };
    the::conf::set( "redis_ip", redis_address.host );
    the::conf::set( "redis_port", redis_address.port );
  }
  else
  {
    the::conf::set( "redis_ip", "127.0.0.1" );
    the::conf::set( "redis_port", 6379 );
  }
}

}

int main( int argc, char ** argv )
{
  parse_and_handle_configuration( the::conf::ParameterVector( argv, argv + argc ) );

  the::ctci::AutoServiceRegister< yarrr::Db, yarrrs::RedisDb > db;
  //todo: move stuff to new models
  the::ctci::AutoServiceRegister< yarrrs::Models, yarrrs::Models > models( yarrr::LuaEngine::model() );
  yarrr::IdGenerator id_generator;
  the::ctci::AutoServiceRegister< yarrr::ModellContainer, yarrr::ModellContainer > modell_container(
      yarrr::LuaEngine::model(),
      id_generator,
      db.get() );

  const std::string home_folder( std::string( getenv( "HOME" ) ) + "/.yarrrserver/" );
  the::conf::set( "lua_configuration_path", home_folder );

  the::ctci::AutoServiceRegister< yarrr::ResourceFinder, yarrr::ResourceFinder > resource_finder_register(
      yarrr::ResourceFinder::PathList{
      home_folder,
      "/usr/local/share/yarrr/",
      "/usr/share/yarrr/" } );

  yarrr::initialize_lua_engine();

  the::time::Clock clock;
  yarrr::ClockExporter clock_exporter( clock, yarrr::LuaEngine::model() );
  yarrrs::NetworkService network_service( clock );
  yarrr::ObjectContainer object_container;
  yarrr::ObjectExporter object_exporter( object_container, yarrr::LuaEngine::model() );
  yarrrs::Player::Container players;
  yarrrs::World world( players, object_container );

  the::time::FrequencyStabilizer< 10, the::time::Clock > frequency_stabilizer( clock );
  the::time::OnceIn< the::time::Clock > update_missions_once_per_second( clock, the::time::Clock::ticks_per_second,
      [ &players ]( const the::time::Time& )
      {
        for ( auto& player : players )
        {
          player.second->update_missions();
        }
      } );

  std::unique_ptr< the::model::ZmqRemote > remote_model_access( create_remote_model_endpoint_if_needed() );

  while ( true )
  {
    network_service.process_network_events();
    object_container.dispatch( yarrr::TimerUpdate( clock.now() ) );
    object_container.check_collision();
    object_exporter.refresh();
    send_update_messages_from( object_container, players );
    update_missions_once_per_second.tick();
    frequency_stabilizer.stabilize();
    the::ctci::service< yarrr::MainThreadCallbackQueue >().process_callbacks();
    if ( remote_model_access )
    {
      remote_model_access->handle_requests();
    }
  }

  return 0;
}

