#include "../src/local_event_dispatcher.hpp"
#include <yarrr/engine_dispatcher.hpp>

#include <thectci/service_registry.hpp>

namespace
{
  the::ctci::AutoServiceRegister< LocalEventDispatcher, LocalEventDispatcher >
    local_event_dispatcher_register;

  the::ctci::AutoServiceRegister< yarrr::EngineDispatcher, yarrr::EngineDispatcher >
    auto_engine_dispatcher_register;
}

