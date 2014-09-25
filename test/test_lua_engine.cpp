#include "../src/lua_engine.hpp"
#include <igloo/igloo_alt.h>

#include <sol.hpp>

using namespace igloo;

Describe( a_lua_engine )
{
  It( is_a_singleton )
  {
    yarrr::Lua& lua_engine( yarrr::Lua::instance() );
    AssertThat( &yarrr::Lua::instance(), Equals( &lua_engine ) );
  }

  It( can_return_the_sol_state )
  {
    sol::state& sol_state( yarrr::Lua::state() );
    (void) sol_state;
  }

  It( loads_base_libraries )
  {
    try
    {
      yarrr::Lua::state().script( "print( \"hello lua\" ) " );
    }
    catch ( std::exception& e )
    {
      std::cout << e.what() << std::endl;
      throw e;
    }
  }
};

Describe( an_auto_lua_register )
{
  It( calls_the_function_when_constructed_with_the_lua_state_as_parameter )
  {
    yarrr::Lua* called_with{ nullptr };
    yarrr::AutoLuaRegister auto_lua_register(
        [ &called_with ]( yarrr::Lua& lua )
        {
          called_with = &lua;
        } );
    AssertThat( called_with, Equals( &yarrr::Lua::instance() ) );
  }
};

namespace
{

class class_without_constructor_parameter_and_function
{
};

class class_with_constructor_parameter_and_without_functions
{
  public:
    class_with_constructor_parameter_and_without_functions( int a )
    {
    }
};

}

Describe( a_simple_lua_register )
{
  It( registers_a_type_without_constructor_parameters_and_exported_functions )
  {
    yarrr::SimpleLuaRegister< class_without_constructor_parameter_and_function >( "a_class" );
    yarrr::Lua::state().script( "a_class.new()" );
  }

  It( registers_a_type_with_constructor_parameters_and_without_exported_functions )
  {
    yarrr::SimpleLuaRegister< class_with_constructor_parameter_and_without_functions, int >( "another_class" );
    yarrr::Lua::state().script( "another_class.new( 10 )" );
  }
};

