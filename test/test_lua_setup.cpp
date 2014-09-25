#include "../src/lua_setup.hpp"
#include "../src/lua_engine.hpp"
#include <theconf/configuration.hpp>
#include <igloo/igloo_alt.h>

#include <cstdlib>
#include <sol.hpp>

using namespace igloo;

namespace
{

class OnlyOnce
{
  private:
    static bool done_already;
  public:
    OnlyOnce( std::function< void() > todo )
    {
      if ( done_already )
      {
        return;
      }

      done_already = true;
      todo();
    }
};

bool OnlyOnce::done_already = false;
}

Describe( lua_setup )
{
  void SetUp()
  {
    the::conf::set( "lua_configuration_path", configuration_path );
    OnlyOnce init(
        []()
        {
          yarrr::initialize_lua_engine();
        } );
  }

  It( exports_configuration_path_to_lua )
  {
    AssertThat( yarrr::Lua::state().get< std::string >( "lua_configuration_path" ), Equals( configuration_path ) );
  }

  It( loads_lua_configuration_from_the_exported_folder )
  {
    AssertThat( yarrr::Lua::state().get< bool >( "was_lua_conf_loaded" ), Equals( true ) );
  }

  It( allows_lua_files_to_load_other_lua_files )
  {
    AssertThat( yarrr::Lua::state().get< bool >( "was_other_lua_file_loaded" ), Equals( true ) );
  }

  const std::string configuration_path{ std::string( std::getenv( "PWD" ) ) + "/" };
};

