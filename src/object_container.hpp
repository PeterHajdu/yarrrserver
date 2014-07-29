#pragma once

#include <yarrr/object.hpp>
#include <thectci/id.hpp>

class ObjectContainer
{
  public:
    add_ctci( "object_container" );
    void add_object( int id, yarrr::Object::Pointer&& );
    void delete_object( int id );

    //todo: add remove multiplexer and remove dispatcher to multiplexer
    template < typename Event >
    void dispatch( const Event& event )
    {
      for ( const auto& object : m_objects )
      {
        object.second->dispatch( event );
      }
    }

  private:
    std::unordered_map< int, yarrr::Object::Pointer > m_objects;
};

