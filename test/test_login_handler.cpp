#include "../src/login_handler.hpp"
#include <yarrr/test_connection.hpp>
#include <yarrr/test_db.hpp>
#include <yarrr/command.hpp>
#include <yarrr/crypto.hpp>
#include <yarrr/protocol.hpp>
#include <igloo/igloo_alt.h>

using namespace igloo;

Describe_Only( a_login_handler )
{
  void SetUp()
  {
    registration_request = yarrr::Command{ {
      yarrr::Protocol::registration_request,
      username,
      auth_token_sent_by_client } };
    login_request = yarrr::Command{ {
      yarrr::Protocol::login_request,
      username } };
    login_handler.reset();
    dispatcher.clear();
    connection = std::make_unique< test::Connection >();
    login_handler = std::make_unique< yarrrs::LoginHandler >(
        connection->wrapper,
        dispatcher );
    was_player_logged_in = false;
    dispatcher.register_listener< yarrrs::PlayerLoggedIn >(
        [
        &was_dispatched = was_player_logged_in,
        &player_name = last_player_logged_in ]( const yarrrs::PlayerLoggedIn& login )
        {
          was_dispatched = true;
          player_name = login.name;
        });

    was_player_logged_out = false;
    dispatcher.register_listener< yarrrs::PlayerLoggedOut >(
        [ &was_dispatched = was_player_logged_out ]( const yarrrs::PlayerLoggedOut& )
        {
          was_dispatched = true;
        });
  }

  It( dispatches_player_logged_in_after_registration_request_if_the_user_did_not_exist_before )
  {
    connection->wrapper.dispatch( registration_request );
    AssertThat( was_player_logged_in, Equals( true ) );
    AssertThat( last_player_logged_in, Equals( username ) );
  }

  It( creates_player_model_if_it_did_not_exist_before )
  {
    //todo
    connection->wrapper.dispatch( registration_request );
    std::string auth_token_in_db;
    //AssertThat(
    //    database->get_hash_field(
    //      yarrr::player_key_from_id( username ),
    //      "auth_token",
    //      auth_token_in_db ),
    //    Equals( true ) );

    AssertThat( auth_token_in_db, Equals( auth_token_sent_by_client ) );
  }

  It( sends_invalid_username_error_message_if_the_username_was_malformed )
  {
    //todo
    assert_login_error_was_sent();
  }

  void set_up_player_modell()
  {
    //todo
    //database->set_hash_field(
    //    yarrr::player_key_from_id( username ),
    //    "auth_token",
    //    original_auth_token );
  }

  It( does_not_dispatch_player_logged_in_after_registration_request_for_existing_user )
  {
    set_up_player_modell();
    connection->wrapper.dispatch( registration_request );
    AssertThat( was_player_logged_in, Equals( false ) );
  }

  void assert_login_error_was_sent()
  {
    AssertThat( connection->has_entity< yarrr::Command >(), Equals( true ) );
    auto command( connection->get_entity< yarrr::Command >() );
    AssertThat( command->command(), Equals( yarrr::Protocol::error ) );
    AssertThat( command->parameters().back(), Contains( "Unable to log in." ) );
  }

  It( sends_an_error_message_after_registration_request_for_existing_user )
  {
    set_up_player_modell();
    connection->wrapper.dispatch( registration_request );
    assert_login_error_was_sent();
  }

  It( does_not_modify_authentication_token_of_a_registered_player )
  {
    //todo
    set_up_player_modell();
    connection->wrapper.dispatch( registration_request );
    std::string auth_token_in_db;
    //AssertThat(
    //    database->get_hash_field(
    //      yarrr::player_key_from_id( username ),
    //      "auth_token",
    //      auth_token_in_db ),
    //    Equals( true ) );

    AssertThat( auth_token_in_db, Equals( original_auth_token ) );
  }

  It( does_not_dispatch_logged_in_for_malformed_registratin_requests )
  {
    const yarrr::Command registration_request_without_username{ {
        yarrr::Protocol::registration_request } };
    connection->wrapper.dispatch( registration_request_without_username );
    AssertThat( was_player_logged_in, Equals( false ) );

    const yarrr::Command registration_request_without_accesstoken{ {
        yarrr::Protocol::registration_request,
        username } };
    connection->wrapper.dispatch( registration_request_without_accesstoken );
    AssertThat( was_player_logged_in, Equals( false ) );

    const yarrr::Command a_different_command{ {
        "something else",
        username,
        auth_token_sent_by_client } };
    connection->wrapper.dispatch( a_different_command );
    AssertThat( was_player_logged_in, Equals( false ) );
  }


  It( sends_out_player_logged_out_event_when_deleted )
  {
    login_handler.reset();
    AssertThat( was_player_logged_out, Equals( true ) );
  }


  It( does_not_dispatch_player_logged_in_after_login_request )
  {
    connection->wrapper.dispatch( login_request );
    AssertThat( was_player_logged_in, Equals( false ) );

    set_up_player_modell();
    connection->wrapper.dispatch( login_request );
    AssertThat( was_player_logged_in, Equals( false ) );
  }

  It( sends_out_authentication_request_as_a_response_to_a_login_request )
  {
    set_up_player_modell();
    connection->wrapper.dispatch( login_request );
    AssertThat( connection->has_entity< yarrr::Command >(), Equals( true ) );

    auto command( connection->get_entity< yarrr::Command >() );
    AssertThat( command->command(), Equals( yarrr::Protocol::authentication_request ) );
    AssertThat( command->parameters(), !IsEmpty() );
  }

  It( sends_error_message_after_login_request_of_not_existing_subscriber )
  {
    connection->wrapper.dispatch( login_request );
    assert_login_error_was_sent();
  }

  It( does_not_respond_to_malformed_login_requests )
  {
    const yarrr::Command malformed_login_request{ {
        yarrr::Protocol::login_request } };

    connection->wrapper.dispatch( malformed_login_request );
    AssertThat( connection->has_entity< yarrr::Command >(), Equals( false ) );
  }

  It( dispatches_player_logged_in_after_authentication_response_with_proper_response_to_the_challenge )
  {
    set_up_player_modell();
    connection->wrapper.dispatch( login_request );

    auto challenge( connection->get_entity< yarrr::Command >()->parameters().back() );

    const yarrr::Command authentication_response{ {
        yarrr::Protocol::authentication_response,
        yarrr::auth_hash( challenge + original_auth_token ) } };

    connection->wrapper.dispatch( authentication_response );
    AssertThat( was_player_logged_in, Equals( true ) );
    AssertThat( last_player_logged_in, Equals( username ) );
  }

  void make_invalid_login_attempt()
  {
    set_up_player_modell();
    connection->wrapper.dispatch( login_request );

    auto challenge( connection->get_entity< yarrr::Command >()->parameters().back() );

    const auto different_auth_token( original_auth_token + "1" );
    const yarrr::Command invalid_authentication_response{ {
        yarrr::Protocol::authentication_response,
        yarrr::auth_hash( challenge + different_auth_token ) } };

    connection->flush_connection();
    connection->wrapper.dispatch( invalid_authentication_response );
  }

  It( does_not_dispatches_player_logged_in_if_authentication_tokens_differ )
  {
    make_invalid_login_attempt();
    AssertThat( was_player_logged_in, Equals( false ) );
  }

  It( sends_login_error_message_if_authentication_tokens_differ )
  {
    make_invalid_login_attempt();
    assert_login_error_was_sent();
  }

  It( handles_malformed_authentication_responses )
  {
    set_up_player_modell();
    connection->wrapper.dispatch( login_request );

    const yarrr::Command malformed_authentication_response{ {
        yarrr::Protocol::authentication_response } };

    connection->wrapper.dispatch( malformed_authentication_response );
  }


  std::unique_ptr< test::Connection > connection;
  the::ctci::Dispatcher dispatcher;
  std::unique_ptr< yarrrs::LoginHandler > login_handler;

  const std::string original_auth_token{ "original auth token" };
  const std::string username{ "Kilgore Trout" };
  const std::string auth_token_sent_by_client{ "auth_token" };
  bool was_player_logged_in;
  std::string last_player_logged_in;
  bool was_player_logged_out;

  yarrr::Command registration_request;
  yarrr::Command login_request;
};

