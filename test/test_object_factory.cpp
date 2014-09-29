#include "../src/object_factory.hpp"
#include <yarrr/object.hpp>
#include <igloo/igloo_alt.h>

using namespace igloo;

Describe( an_object_factory )
{
  void SetUp()
  {
    object_factory.reset( new yarrrs::ObjectFactory() );
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
  }

  std::unique_ptr< yarrrs::ObjectFactory > object_factory;
  const std::string key{ "duck" };
};

