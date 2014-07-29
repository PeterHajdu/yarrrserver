#pragma once

#include <yarrr/object.hpp>
#include <thectci/id.hpp>
#include <thectci/dispatcher.hpp>

class ObjectContainer : public the::ctci::Dispatcher
{
  public:
    add_ctci( "object_container" );
    void add_object( int id, yarrr::Object::Pointer&& );
    void delete_object( int id );

  private:
    typedef std::unordered_map< int, yarrr::Object::Pointer > Container;
    Container m_objects;
};

