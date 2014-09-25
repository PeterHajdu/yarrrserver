#include "lua_engine.hpp"

#include <memory>

namespace
{

std::unique_ptr< yarrr::Lua> engine_instance;

}

namespace yarrr
{

Lua::Lua()
{
  m_state.open_libraries( sol::lib::base );
  m_state.open_libraries( sol::lib::package );
}

Lua& Lua::instance()
{
  if ( !engine_instance )
  {
    engine_instance.reset( new Lua() );
  }

  return *engine_instance;
}

AutoLuaRegister::AutoLuaRegister( std::function< void( Lua& ) > register_function )
{
  register_function( Lua::instance() );
}

}

