#pragma once

#include "login_handler.hpp"
#include "local_event_dispatcher.hpp"
#include "db.hpp"
#include <thectci/dispatcher.hpp>
#include <thenet/service.hpp>
#include <yarrr/command.hpp>
#include <yarrr/callback_queue.hpp>
#include <yarrr/clock_synchronizer.hpp>


namespace the
{
namespace time
{
class Clock;
}
}

namespace yarrrs
{


class ConnectionBundle
{
  public:
    typedef std::unique_ptr< ConnectionBundle > Pointer;
    ConnectionBundle( the::net::Connection::Pointer connection )
      : connection_wrapper( connection )
      , login_handler(
          connection_wrapper,
          the::ctci::service< LocalEventDispatcher >().dispatcher,
          the::ctci::service< Db >() )
    {
    }

    ConnectionWrapper connection_wrapper;
    LoginHandler login_handler;
};

class NetworkService
{
  public:
    NetworkService( the::time::Clock& clock );
    void process_network_events();

  private:
    void handle_new_connection( the::net::Connection::Pointer connection );
    void handle_new_connection_on_main_thread( the::net::Connection::Pointer connection );
    void handle_connection_lost( the::net::Connection::Pointer connection );
    void handle_connection_lost_on_main_thread( int connection_id );

    the::time::Clock& m_clock;
    the::net::Service m_network_service;
    yarrr::CallbackQueue m_callback_queue;
    std::unordered_map< int, ConnectionBundle::Pointer > m_connection_bundles;
};

}

