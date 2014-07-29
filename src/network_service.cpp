#include "network_service.hpp"

#include "local_event_dispatcher.hpp"
#include <thectci/dispatcher.hpp>
#include <thenet/service.hpp>
#include <yarrr/connection_wrapper.hpp>
#include <yarrr/login.hpp>
#include <yarrr/callback_queue.hpp>
#include <yarrr/clock_synchronizer.hpp>

typedef yarrr::ConnectionWrapper<the::net::Connection> ConnectionWrapper;

LoginHandler::LoginHandler( ConnectionWrapper& connection_wrapper )
  : m_dispatcher()
  , m_connection_wrapper( connection_wrapper )
  , m_connection( connection_wrapper.connection )
    , m_id( m_connection.id )
{
  m_dispatcher.register_listener<yarrr::LoginRequest>(
      std::bind( &LoginHandler::handle_login_request, this, std::placeholders::_1 ) );
  connection_wrapper.register_dispatcher( m_dispatcher );
}

void
LoginHandler::handle_login_request( const yarrr::LoginRequest& request )
{
  m_connection.send( yarrr::LoginResponse( m_id ).serialize() );
  the::ctci::service< LocalEventDispatcher >().dispatcher.dispatch(
      PlayerLoggedIn( m_connection_wrapper, m_id, request.login_id() ) );
}

LoginHandler::~LoginHandler()
{
  the::ctci::service< LocalEventDispatcher >().dispatcher.dispatch(
      PlayerLoggedOut( m_id ) );
}


NetworkService::NetworkService( the::time::Clock& clock )
  : m_clock( clock )
  , m_network_service(
      std::bind( &NetworkService::handle_new_connection, this, std::placeholders::_1 ),
      std::bind( &NetworkService::handle_connection_lost, this, std::placeholders::_1 ) )
{
  m_network_service.listen_on( 2001 );
  m_network_service.start();
}

void
NetworkService::handle_new_connection( the::net::Connection& connection )
{
  connection.register_task( the::net::NetworkTask::Pointer(
        new yarrr::clock_sync::Server< the::time::Clock, the::net::Connection >(
          m_clock,
          connection ) ) );

  //todo: connection might be deleted before it gets added on the main thread
  //todo: should we lock?
  m_callback_queue.push_back( std::bind(
        &NetworkService::handle_new_connection_on_main_thread, this, &connection ) );
}

void
NetworkService::handle_new_connection_on_main_thread( the::net::Connection* connection )
{
  ConnectionBundle::Pointer new_connection_bundle( new ConnectionBundle( *connection ) );
  m_connection_bundles.emplace(
      connection->id,
      std::move( new_connection_bundle ) );
}

void
NetworkService::handle_connection_lost( the::net::Connection& connection )
{
  m_callback_queue.push_back( std::bind(
        &NetworkService::handle_connection_lost_on_main_thread, this, connection.id ) );
}

void
NetworkService::handle_connection_lost_on_main_thread( int connection_id )
{
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

