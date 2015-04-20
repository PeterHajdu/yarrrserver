#include "network_service.hpp"
#include "local_event_dispatcher.hpp"

#include <yarrr/db.hpp>
#include <yarrr/connection_wrapper.hpp>
#include <yarrr/command.hpp>
#include <yarrr/callback_queue.hpp>
#include <yarrr/clock_synchronizer.hpp>
#include <yarrr/log.hpp>

#include <thectci/dispatcher.hpp>
#include <thenet/service.hpp>
#include <thetime/clock.hpp>
#include <theconf/configuration.hpp>

namespace yarrrs
{

NetworkService::NetworkService( the::time::Clock& clock )
  : m_clock( clock )
  , m_network_service(
      std::bind( &NetworkService::handle_new_connection, this, std::placeholders::_1 ),
      std::bind( &NetworkService::handle_connection_lost, this, std::placeholders::_1 ) )
{
  m_network_service.listen_on( the::conf::get<int>( "port" ) );
  m_network_service.start();
}

void
NetworkService::handle_new_connection( the::net::Connection::Pointer connection )
{
  thelog_trace( yarrr::log::info, __PRETTY_FUNCTION__ );
  connection->register_task( the::net::NetworkTask::Pointer(
        new yarrr::clock_sync::Server< the::time::Clock, the::net::Connection >(
          m_clock,
          *connection ) ) );

  //todo: connection might be deleted before it gets added on the main thread
  //todo: should we lock?
  m_callback_queue.push_back( std::bind(
        &NetworkService::handle_new_connection_on_main_thread, this, connection ) );
}

void
NetworkService::handle_new_connection_on_main_thread( the::net::Connection::Pointer connection )
{
  thelog_trace( yarrr::log::info, __PRETTY_FUNCTION__ );
  ConnectionBundle::Pointer new_connection_bundle( new ConnectionBundle( connection ) );
  m_connection_bundles.emplace(
      connection->id,
      std::move( new_connection_bundle ) );
}

void
NetworkService::handle_connection_lost( the::net::Connection::Pointer connection )
{
  thelog_trace( yarrr::log::info, __PRETTY_FUNCTION__ );
  m_callback_queue.push_back( std::bind(
        &NetworkService::handle_connection_lost_on_main_thread, this, connection->id ) );
}

void
NetworkService::handle_connection_lost_on_main_thread( int connection_id )
{
  thelog_trace( yarrr::log::info, __PRETTY_FUNCTION__ );
  m_connection_bundles.erase( connection_id );
}

void
NetworkService::process_network_events()
{
  m_callback_queue.process_callbacks();
  for ( auto& bundle : m_connection_bundles )
  {
    bundle.second->connection_wrapper.process_incoming_messages();
  }
}

}

