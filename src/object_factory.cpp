#include "object_factory.hpp"
#include <yarrr/log.hpp>

namespace yarrrs
{

yarrr::Object::Pointer
ObjectFactory::create_a( const std::string& key )
{
  Creators::const_iterator creator( m_creators.find( key ) );
  if ( creator == m_creators.end() )
  {
    thelog( yarrr::log::warning )( "Unknown object key requested: ", key );
    return nullptr;
  }

  return m_creators[ key ]();
}

void
ObjectFactory::register_creator(
    const std::string& key,
    Creator creator )
{
  m_creators[ key ] = creator;
}

}

