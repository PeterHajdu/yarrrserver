#include <iostream>
#include <array>
#include <thread>
#include <chrono>
#include <functional>
#include <mutex>

#include <yarrr/ship.hpp>
#include <thenet/socket_pool.hpp>
#include <thenet/message_queue.hpp>


namespace
{
  void lost_connection( the::net::Socket& )
  {
    std::cout << "connection lost" << std::endl;
  }

  void data_available_on( the::net::Socket& socket, const char* message, size_t length )
  {
    std::cout << "data arrived: " << std::string( message, length ) << std::endl;
  }
}

class Connection
{
  public:
    Connection( the::net::Socket& socket )
      : m_socket( socket )
      , m_message_queue(
          std::bind(
            &the::net::Socket::send,
            &m_socket,
            std::placeholders::_1, std::placeholders::_2 ) )
    {
    }

    bool send( the::net::Data&& message )
    {
      return m_message_queue.send( std::move( message ) );
    }

    bool receive( the::net::Data& message )
    {
      return m_message_queue.receive( message );
    }

    void wake_up_on_network_thread()
    {
      m_message_queue.wake_up();
    }

  private:
    the::net::Socket& m_socket;
    the::net::MessageQueue m_message_queue;
};


class ConnectionPool
{
  public:
    typedef std::function< void(Connection&) > NewConnectionCallback;
    ConnectionPool( NewConnectionCallback new_connection )
      : m_new_connection( new_connection )
    {
    }

    void on_new_socket( the::net::Socket& socket )
    {
      m_connections.emplace_back( new Connection( socket ) );
      m_new_connection( *m_connections.back() );
    }

    void wake_up_on_network_thread()
    {
      for ( auto& connection : m_connections )
      {
        connection->wake_up_on_network_thread();
      }
    }

  private:
    std::vector< std::unique_ptr< Connection > > m_connections;
    NewConnectionCallback m_new_connection;
};


class Player
{
  public:
    typedef std::unique_ptr< Player > Pointer;

    Player()
      : m_ship( "Ship: 1000 1100 1 2 3000 3" )
    {
    }

    void update()
    {
      yarrr::time_step( m_ship );
    }

    the::net::Data serialize() const
    {
      const std::string ship_data( yarrr::serialize( m_ship ) );
      return the::net::Data( begin( ship_data ) , end( ship_data ) );
    }

  private:
    yarrr::Ship m_ship;
};

int main( int argc, char ** argv )
{
  std::vector< Player::Pointer > players;
  std::mutex players_mutex;

  std::vector< std::reference_wrapper< Connection > > connections;

  ConnectionPool cpool(
      [ &connections, &players, &players_mutex ]( Connection& connection )
      {
        std::lock_guard<std::mutex> lock( players_mutex );
        players.emplace_back( new Player() );
        connections.emplace_back( connection );
      } );

  the::net::SocketPool pool(
      std::bind( &ConnectionPool::on_new_socket, &cpool, std::placeholders::_1 ),
      lost_connection,
      data_available_on );

  pool.listen( 2000 );

  std::thread network_thread(
      [ &cpool, &pool, &players_mutex, &players ]()
      {
        while ( true )
        {
          const int ten_milliseconds( 10 );
          pool.run_for( ten_milliseconds );
          {
            std::lock_guard<std::mutex> lock( players_mutex );
            cpool.wake_up_on_network_thread();
          }
        }
      } );

  while ( true )
  {
    {
      std::lock_guard<std::mutex> lock( players_mutex );
      std::vector< the::net::Data > ship_states;
      for ( auto& player : players )
      {
        player->update();
        ship_states.emplace_back( player->serialize() );
      }

      for ( auto& connection : connections )
      {
        for ( auto ship_state : ship_states )
        {
          connection.get().send( the::net::Data( ship_state ) );
        }
      }
    }

    std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
  }

  network_thread.join();
  return 0;
}

