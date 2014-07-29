#include "object_container.hpp"
#include <thectci/service_registry.hpp>

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

namespace
{
  the::ctci::AutoServiceRegister< ObjectContainer, ObjectContainer > auto_register_object_container;
}

