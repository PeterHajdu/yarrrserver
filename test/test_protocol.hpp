#pragma once

#include <yarrr/test_connection.hpp>
#include <yarrr/protocol.hpp>
#include <yarrr/object.hpp>

namespace test
{

inline void
assert_object_assigned( test::Connection& connection, yarrr::Object::Id id )
{
  using namespace igloo;
  AssertThat( connection.has_entity< yarrr::Command >(), Equals( true ) );
  auto command( connection.get_entity< yarrr::Command >() );
  AssertThat( command->command(), Equals( yarrr::Protocol::object_assigned ) );
  AssertThat( command->parameters(), !IsEmpty() );
  AssertThat( command->parameters(), Contains( std::to_string( id ) ) );
}

}

