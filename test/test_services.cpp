#include "../src/local_event_dispatcher.hpp"
#include "../src/notifier.hpp"
#include <yarrr/object_factory.hpp>
#include <yarrr/engine_dispatcher.hpp>
#include <yarrr/dummy_graphical_engine.hpp>
#include <yarrr/main_thread_callback_queue.hpp>
#include <yarrr/lua_engine.hpp>
#include <thectci/service_registry.hpp>
#include <themodel/node_list.hpp>

#include <sstream>

namespace test
{
  std::stringstream&
  get_notification_stream()
  {
    static std::stringstream notification_stream;
    return notification_stream;
  }
}

namespace
{
  the::ctci::AutoServiceRegister< yarrr::GraphicalEngine, yarrr::DummyGraphicalEngine >
    graphical_engine_register;

  the::ctci::AutoServiceRegister< LocalEventDispatcher, LocalEventDispatcher >
    local_event_dispatcher_register;

  the::ctci::AutoServiceRegister< yarrr::EngineDispatcher, yarrr::EngineDispatcher >
    auto_engine_dispatcher_register;

  the::ctci::AutoServiceRegister< yarrrs::Notifier, yarrrs::Notifier > notifier_register(
      test::get_notification_stream() );

  the::ctci::AutoServiceRegister< yarrr::MainThreadCallbackQueue, yarrr::MainThreadCallbackQueue >
    main_thread_callback_queue_register;

  the::ctci::AutoServiceRegister< yarrr::ObjectFactory, yarrr::ObjectFactory >
    object_factory_register;

  the::model::OwningNodeList missions_owner( "missions", yarrr::LuaEngine::model() );
}

