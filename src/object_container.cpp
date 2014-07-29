#include "object_container.hpp"
#include <thectci/service_registry.hpp>

void
ObjectContainer::add_object( int id, yarrr::Object::Pointer&& object )
{
  register_dispatcher( *object );
  m_objects.emplace( id, std::move( object ) );
}


void
ObjectContainer::delete_object( int id )
{
  Container::iterator object( m_objects.find( id ) );
  if ( object == m_objects.end() )
  {
    return;
  }

  remove_dispatcher( *(object->second) );
  m_objects.erase( object );
}

namespace
{
  the::ctci::AutoServiceRegister< ObjectContainer, ObjectContainer > auto_register_object_container;
}

