#include <yarrr/engine_dispatcher.hpp>
#include <yarrr/dummy_graphical_engine.hpp>
#include <thectci/service_registry.hpp>

namespace
{
  the::ctci::AutoServiceRegister< yarrr::EngineDispatcher, yarrr::EngineDispatcher >
    auto_engine_dispatcher_register;

  the::ctci::AutoServiceRegister< yarrr::GraphicalEngine, yarrr::DummyGraphicalEngine >
    auto_graphical_engine_register;
}

