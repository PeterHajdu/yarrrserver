#include "object_factory.hpp"

namespace yarrrs
{

yarrr::Object::Pointer
ObjectFactory::create_a( const std::string& key )
{
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

