#include "test_services.hpp"
#include "../src/local_event_dispatcher.hpp"
#include <yarrr/object_factory.hpp>
#include <yarrr/mission_factory.hpp>
#include <yarrr/engine_dispatcher.hpp>
#include <yarrr/dummy_graphical_engine.hpp>
#include <yarrr/main_thread_callback_queue.hpp>
#include <yarrr/lua_engine.hpp>
#include <thectci/service_registry.hpp>
#include <themodel/node_list.hpp>

#include <sstream>

namespace test
{

Services::Services()
  : lua()
  , engine_dispatcher_register()
  , engine_dispatcher( the::ctci::service< yarrr::EngineDispatcher >() )
  , local_event_dispatcher_register()
  , local_event_dispatcher( the::ctci::service< LocalEventDispatcher >() )
  , id_generator()
  , database()
  , modell_container_register( lua, id_generator, database )
  , modell_container( the::ctci::service< yarrr::ModellContainer >() )
  , models_register( lua )
  , models( the::ctci::service< yarrrs::Models >() )
  , callback_queue_register()
  , main_thread_callback_queue( the::ctci::service< yarrr::MainThreadCallbackQueue >() )
  , players()
  , objects()
  , command_handler()
  , world()
{
  reset_world();
}

}

namespace
{
  the::ctci::AutoServiceRegister< yarrr::GraphicalEngine, yarrr::DummyGraphicalEngine >
    graphical_engine_register;

  the::ctci::AutoServiceRegister< yarrr::ObjectFactory, yarrr::ObjectFactory >
    object_factory_register;

  the::ctci::AutoServiceRegister< yarrr::MissionFactory, yarrr::MissionFactory >
    mission_factory_register;
}

