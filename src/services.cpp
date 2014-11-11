#include <yarrr/engine_dispatcher.hpp>
#include <yarrr/dummy_graphical_engine.hpp>
#include <yarrr/main_thread_callback_queue.hpp>
#include <yarrr/mission_factory.hpp>
#include <yarrr/object_factory.hpp>
#include <thectci/service_registry.hpp>

#include "dummy_particle_factory.hpp"
#include "local_event_dispatcher.hpp"

namespace
{
  the::ctci::AutoServiceRegister< yarrr::MissionFactory, yarrr::MissionFactory >
    mission_factory_register;

  the::ctci::AutoServiceRegister< yarrr::ObjectFactory, yarrr::ObjectFactory >
    object_factory_register;

  the::ctci::AutoServiceRegister< LocalEventDispatcher, LocalEventDispatcher >
    local_event_dispatcher_register;

  the::ctci::AutoServiceRegister< yarrr::EngineDispatcher, yarrr::EngineDispatcher >
    auto_engine_dispatcher_register;

  the::ctci::AutoServiceRegister< yarrr::GraphicalEngine, yarrr::DummyGraphicalEngine >
    auto_graphical_engine_register;

  the::ctci::AutoServiceRegister< yarrr::MainThreadCallbackQueue, yarrr::MainThreadCallbackQueue >
    auto_main_thread_callback_queue_register;

  the::ctci::AutoServiceRegister< yarrr::ParticleFactory, DummyParticleFactory >
    auto_particle_factory_register;
}

