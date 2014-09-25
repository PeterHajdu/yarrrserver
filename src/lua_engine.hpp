#pragma once

#include <sol.hpp>
#include <yarrr/log.hpp>

namespace yarrr
{

class Lua final
{
  public:
    static Lua& instance();
    static sol::state& state()
    {
      return instance().m_state;
    }

  private:
    Lua();

    sol::state m_state;
};

class AutoLuaRegister final
{
  public:
    AutoLuaRegister( std::function< void( Lua& ) > );
};

template < typename ClassToBeRegistered, typename...Args >
class SimpleLuaRegister
{
  public:
    SimpleLuaRegister( const char* name )
    {
      Lua::state().new_userdata< ClassToBeRegistered, Args... >( name );
      thelog( yarrr::log::info )( "Expose class:", name, "to lua world." );
    }
};

}

