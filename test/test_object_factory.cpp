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

  It( creates_an_object_by_key )
  {
    yarrr::Object::Pointer object( object_factory->create_a( "duck" ) );
    (void)object;
  }

  std::unique_ptr< yarrrs::ObjectFactory > object_factory;
};

