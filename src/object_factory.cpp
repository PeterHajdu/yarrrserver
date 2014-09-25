#include "object_factory.hpp"
#include "lua_engine.hpp"
#include <yarrr/log.hpp>
#include <thectci/service_registry.hpp>
#include <yarrr/object_creator.hpp>

namespace
{

void register_object_factory( const std::string& name, sol::function factory )
{
  the::ctci::service< yarrrs::ObjectFactory >().register_creator(
      name,
      [ factory, name ]()
      {
        thelog( yarrr::log::info )( "Calling lua factory method:", name );
        try
        {
          return factory.call<yarrr::Object&>().clone();
        }
        catch( std::exception& e )
        {
          thelog( yarrr::log::info )( "Lua factory method failed:", name, e.what() );
        }
        return yarrr::create_ship();
      } );
  thelog( yarrr::log::info )( "Registered lua factory method for object type:", name );
}

the::ctci::AutoServiceRegister< yarrrs::ObjectFactory, yarrrs::ObjectFactory > auto_object_factory_register;
yarrr::AutoLuaRegister function_register(
    []( yarrr::Lua& lua )
    {
      lua.state().set_function( "register_object_factory", register_object_factory );
    } );
}

namespace yarrrs
{

yarrr::Object::Pointer
ObjectFactory::create_a( const std::string& key )
{
  Creators::const_iterator creator( m_creators.find( key ) );
  if ( creator == m_creators.end() )
  {
    thelog( yarrr::log::warning )( "Unknown object key requested: ", key );
    return nullptr;
  }

  return m_creators[ key ]();
}

void
ObjectFactory::register_creator(
    const std::string& key,
    Creator creator )
{
  m_creators[ key ] = creator;
  thelog( yarrr::log::info )( "Registered creator for type: ", key );
}

}

