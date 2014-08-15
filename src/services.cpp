#include <yarrr/engine_dispatcher.hpp>
#include <yarrr/dummy_graphical_engine.hpp>
#include <yarrr/main_thread_callback_queue.hpp>
#include <thectci/service_registry.hpp>

#include "dummy_particle_factory.hpp"

namespace
{
  the::ctci::AutoServiceRegister< yarrr::EngineDispatcher, yarrr::EngineDispatcher >
    auto_engine_dispatcher_register;

  the::ctci::AutoServiceRegister< yarrr::GraphicalEngine, yarrr::DummyGraphicalEngine >
    auto_graphical_engine_register;

  the::ctci::AutoServiceRegister< yarrr::MainThreadCallbackQueue, yarrr::MainThreadCallbackQueue >
    auto_main_thread_callback_queue_register;

  the::ctci::AutoServiceRegister< yarrr::ParticleFactory, DummyParticleFactory >
    auto_particle_factory_register;
}

