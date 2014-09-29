#include "object_factory.hpp"

namespace yarrrs
{

yarrr::Object::Pointer
ObjectFactory::create_a( const std::string& key )
{
  return m_creator();
}

void
ObjectFactory::register_creator(
    const std::string& key,
    Creator creator )
{
  m_creator = creator;
}

}

