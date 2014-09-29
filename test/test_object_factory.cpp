#include "../src/object_factory.hpp"
#include <yarrr/object.hpp>
#include <igloo/igloo_alt.h>

using namespace igloo;

Describe( an_object_factory )
{
  void SetUp()
  {
    object_factory.reset( new yarrrs::ObjectFactory() );
    created_object_id = 0;
    object_factory->register_creator( key,
        [ this ]()
        {
          yarrr::Object::Pointer object( new yarrr::Object() );
          created_object_id = object->id;
          return object;
        } );
  }

  It( allows_factory_function_registration_by_type )
  {
    object_factory->register_creator( key,
        []()
        {
          return yarrr::Object::Pointer( nullptr );
        } );
  }

  It( creates_an_object_by_key )
  {
    yarrr::Object::Pointer object( object_factory->create_a( key ) );
    AssertThat( object->id, Equals( created_object_id ) );
  }

  yarrr::Object::Id created_object_id;
  std::unique_ptr< yarrrs::ObjectFactory > object_factory;
  const std::string key{ "duck" };
};

