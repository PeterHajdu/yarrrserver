#include "login_handler.hpp"
#include "local_event_dispatcher.hpp"
#include <yarrr/db.hpp>
#include <yarrr/command.hpp>
#include <yarrr/crypto.hpp>
#include <yarrr/log.hpp>
#include <yarrr/protocol.hpp>

namespace
{

const std::string auth_token_field_name( "auth_token" );
const std::string login_error_message( "Unable to log in.  Please restart the client with the --username command line parameter.  If you are unable to solve the issue send an email to info@yarrrthegame.com" );
const std::string invalid_username_message( "Invalid username.  Username must not contain the following characters: space" );
const std::string database_error( "There seems to be a problem with the database, please notify: info@yarrrthegame.com" );

}

namespace yarrrs
{

LoginHandler::LoginHandler(
    ConnectionWrapper& connection_wrapper,
    the::ctci::Dispatcher& dispatcher,
    yarrr::Db& db )
  : m_connection_wrapper( connection_wrapper )
  , m_connection( connection_wrapper.connection )
  , m_id( m_connection->id )
  , m_command_callback( connection_wrapper.register_smart_listener< yarrr::Command >(
        [ this ]( const yarrr::Command& cmd )
        {
          if ( cmd.command() == yarrr::Protocol::registration_request )
          {
            handle_registration_request( cmd );
            return;
          }

          if ( cmd.command() == yarrr::Protocol::login_request )
          {
            handle_login_request( cmd );
            return;
          }

          if ( cmd.command() == yarrr::Protocol::authentication_response )
          {
            handle_authentication_response( cmd );
            return;
          }
        }) )
  , m_dispatcher( dispatcher )
  , m_db( db )
  , m_was_authentication_request_sent_out( false )
{
}


void
LoginHandler::send_login_error_message( const std::string& additional_information ) const
{
  m_connection->send( yarrr::Command{ {
      yarrr::Protocol::error,
      login_error_message + " additional information: " + additional_information } }.serialize() );
}


void
LoginHandler::handle_registration_request( const yarrr::Command& request )
{
  thelog_trace( yarrr::log::info, __PRETTY_FUNCTION__ );

  if ( request.parameters().size() < 2 )
  {
    thelog( yarrr::log::warning )( "Ivalid registration request." );
    return;
  }

  m_username = request.parameters()[ 0 ];
  const std::string user_key( yarrr::player_key_from_id( m_username ) );
  thelog( yarrr::log::debug )( "Using user key:", user_key );
  if ( m_db.key_exists( user_key ) )
  {
    thelog( yarrr::log::warning )( "Registration request for already existing user denied." );
    send_login_error_message( "User already exists, please choose another username." );
    return;
  }

  if ( !m_db.set_hash_field(
      user_key,
      auth_token_field_name,
      request.parameters()[ 1 ] ) )
  {
    send_login_error_message( invalid_username_message );
  }

  if ( !m_db.add_to_set(
      "users",
      m_username ) )
  {
    send_login_error_message( database_error );
  }

  m_dispatcher.dispatch(
      PlayerLoggedIn( m_connection_wrapper, m_id, m_username ) );
}


void
LoginHandler::handle_login_request( const yarrr::Command& request )
{
  if ( request.parameters().empty() )
  {
    thelog( yarrr::log::warning )( "Malformed login request." );
    return;
  }

  m_username = request.parameters().back();

  if ( !m_db.key_exists( yarrr::player_key_from_id( m_username ) ) )
  {
    thelog( yarrr::log::warning )( "Login request with unknown username: ", m_username );
    send_login_error_message( "Unknown username." );
    return;
  }

  const size_t challenge_length( 256u );
  m_challenge = yarrr::random( challenge_length );
  m_connection->send( yarrr::Command{ {
        yarrr::Protocol::authentication_request,
        m_challenge } }.serialize() );
  m_was_authentication_request_sent_out = true;
}


void
LoginHandler::handle_authentication_response( const yarrr::Command& response ) const
{
  if ( !m_was_authentication_request_sent_out )
  {
    thelog( yarrr::log::warning )( "Authentication response received without request from:", m_username );
    return;
  }

  if ( response.parameters().empty() )
  {
    thelog( yarrr::log::warning )( "Invalid authentication response received from:", m_username );
    return;
  }

  std::string auth_token;
  if ( !m_db.get_hash_field(
        yarrr::player_key_from_id( m_username ),
        auth_token_field_name,
        auth_token ) )
  {
    thelog( yarrr::log::error )( "Wasn't able to retrieve authentication token of user:", m_username );
    return;
  }

  if ( auth_token.empty() )
  {
    thelog( yarrr::log::warning )( "Invalid authentication token in the database for user:", m_username );
    return;
  }

  auto expected_response( yarrr::auth_hash( m_challenge + auth_token ) );
  if ( response.parameters().back() != expected_response )
  {
    thelog( yarrr::log::warning )( "Invalid authentication response from user:", m_username );
    send_login_error_message( "Invalid authentication." );
    return;
  }

  log_in();
}

void
LoginHandler::log_in() const
{
  m_dispatcher.dispatch( PlayerLoggedIn( m_connection_wrapper, m_id, m_username ) );
}


LoginHandler::~LoginHandler()
{
  m_dispatcher.dispatch( PlayerLoggedOut( m_id ) );
}

}

