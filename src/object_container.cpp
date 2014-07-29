#include "object_container.hpp"

ObjectContainer::ObjectContainer()
{
}


void
ObjectContainer::add_object( int id, yarrr::Object::Pointer&& object )
{
  m_objects.emplace( id, std::move( object ) );
}


void
ObjectContainer::delete_object( int id )
{
  m_objects.erase( id );
}

