#include <iostream>
#include <array>
#include <thread>
#include <chrono>
#include <functional>
#include <mutex>
#include <algorithm>

#include <yarrr/ship.hpp>
#include <thenet/socket_pool.hpp>
#include <thenet/message_queue.hpp>
#include <thenet/connection_buffer.hpp>

namespace
{
  size_t dummy_parser( const char*, size_t length )
  {
    return length;
  }
}

class Connection
{
  public:
    typedef std::unique_ptr<Connection> Pointer;
    Connection( the::net::Socket& socket )
      : id( socket.id )
      , m_socket( socket )
      , m_message_queue(
          std::bind(
            &the::net::Socket::send,
            &m_socket,
            std::placeholders::_1, std::placeholders::_2 ) )
      , m_incoming_buffer(
          &dummy_parser,
          std::bind(
            &the::net::MessageQueue::message_from_network, &m_message_queue, std::placeholders::_1 ) )
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

    void data_from_network( const char* data, size_t length )
    {
      m_incoming_buffer.receive( data, length );
    }

    const int id;
  private:
    the::net::Socket& m_socket;
    the::net::MessageQueue m_message_queue;
    the::net::ConnectionBuffer m_incoming_buffer;
};


class ConnectionPool
{
  public:
    typedef std::function< void(Connection&) > ConnectionCallback;
    ConnectionPool(
        ConnectionCallback new_connection,
        ConnectionCallback lost_connection )
      : m_new_connection( new_connection )
      , m_lost_connection( lost_connection )
    {
    }

    void on_new_socket( the::net::Socket& socket )
    {
      Connection::Pointer new_connection( new Connection( socket ) );
      m_new_connection( *new_connection );

      m_connections.emplace( std::make_pair(
            socket.id,
            std::move( new_connection ) ) );
    }

    void on_lost_socket( the::net::Socket& socket )
    {
      ConnectionContainer::iterator connection( m_connections.find( socket.id ) );
      if ( connection == m_connections.end() )
      {
        return;
      }

      m_lost_connection( *connection->second );
      m_connections.erase( connection );
    }

    void on_data_arrived( the::net::Socket& socket, const char* message, size_t length )
    {
      ConnectionContainer::iterator connection( m_connections.find( socket.id ) );
      if ( connection == m_connections.end() )
      {
        return;
      }

      connection->second->data_from_network( message, length );
    }


    void wake_up_on_network_thread()
    {
      for ( auto& connection : m_connections )
      {
        connection.second->wake_up_on_network_thread();
      }
    }

  private:
    typedef std::unordered_map< int, Connection::Pointer > ConnectionContainer;
    ConnectionContainer m_connections;
    ConnectionCallback m_new_connection;
    ConnectionCallback m_lost_connection;
};


class Player
{
  public:
    typedef std::unique_ptr< Player > Pointer;

    Player( int network_id )
      : id( network_id )
      , m_ship( "Ship: 1000 1100 1 2 3000 3" )
    {
    }
    const int id;

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
  std::unordered_map< int, Player::Pointer > players;
  std::mutex players_mutex;

  std::vector< std::reference_wrapper< Connection > > connections;

  ConnectionPool cpool(
      [ &connections, &players, &players_mutex ]( Connection& connection )
      {
        std::lock_guard<std::mutex> lock( players_mutex );
        Player::Pointer new_player( new Player( connection.id ) );
        players.emplace( std::make_pair(
          connection.id,
          std::move( new_player ) ) );
        connections.emplace_back( connection );
      },
      [ &connections, &players, &players_mutex ]( Connection& connection )
      {
        std::lock_guard<std::mutex> lock( players_mutex );
        players.erase( connection.id );
        connections.erase(
          std::remove_if( begin( connections ), end( connections ),
          [ &connection ]( std::reference_wrapper< Connection >& element )
          {
            return connection.id == element.get().id;
          } ),
          std::end( connections ) );
      } );

  the::net::SocketPool pool(
      std::bind( &ConnectionPool::on_new_socket, &cpool, std::placeholders::_1 ),
      std::bind( &ConnectionPool::on_lost_socket, &cpool, std::placeholders::_1 ),
      std::bind( &ConnectionPool::on_data_arrived, &cpool,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3 ) );

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
        player.second->update();
        ship_states.emplace_back( player.second->serialize() );
      }

      for ( auto& connection : connections )
      {
        for ( auto ship_state : ship_states )
        {
          connection.get().send( the::net::Data( ship_state ) );
        }

        the::net::Data message;
        while ( connection.get().receive( message ) )
        {
          std::cout << "message arrived from player: " << connection.get().id << " " <<
            std::string( begin( message ), end( message ) ) << std::endl;
        }
      }
    }

    std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
  }

  network_thread.join();
  return 0;
}

