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

  It( calls_the_correct_creator_in_case_of_multiple_keys )
  {
    bool was_another_creator_called{ false };
    object_factory->register_creator( "another key",
        [ &was_another_creator_called ]()
        {
          was_another_creator_called = true;
          return yarrr::Object::Pointer( nullptr );
        } );
    object_factory->create_a( key );
    AssertThat( was_another_creator_called, Equals( false ) );
    object_factory->create_a( "another key" );
    AssertThat( was_another_creator_called, Equals( true ) );
  }

  It( returns_nullptr_if_the_key_is_unknown )
  {
    auto object( object_factory->create_a( "cat" ) );
    AssertThat( object.get() == nullptr, Equals( true ) );
  }

  yarrr::Object::Id created_object_id;
  std::unique_ptr< yarrrs::ObjectFactory > object_factory;
  const std::string key{ "duck" };
};

