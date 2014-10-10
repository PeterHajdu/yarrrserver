#include "player.hpp"
#include "world.hpp"

#include <yarrr/object.hpp>
#include <yarrr/chat_message.hpp>
#include <yarrr/log.hpp>
#include <yarrr/login.hpp>

#include <thectci/service_registry.hpp>

namespace yarrrs
{

Player::Player(
    const Container& players,
    const std::string& name,
    ConnectionWrapper& connection_wrapper )
  : name( name )
  , m_players( players )
  , m_connection_wrapper( connection_wrapper )
  , m_current_object( nullptr )
{
  connection_wrapper.register_listener< yarrr::ChatMessage >(
      std::bind( &Player::handle_chat_message, this, std::placeholders::_1 ) );
}

bool
Player::send( yarrr::Data&& message ) const
{
  return m_connection_wrapper.connection.send( std::move( message ) );
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
  return m_current_object->id;
}

void
Player::assign_object( yarrr::Object& object )
{
  if ( m_current_object )
  {
    m_connection_wrapper.remove_dispatcher( m_current_object->dispatcher );
  }

  m_current_object = &object;
  send( yarrr::ObjectAssigned( object.id ).serialize() );
  m_connection_wrapper.register_dispatcher( object.dispatcher );
}

}

