#include "../src/local_event_dispatcher.hpp"
#include "../src/notifier.hpp"
#include <yarrr/engine_dispatcher.hpp>
#include <thectci/service_registry.hpp>

#include <sstream>

namespace test
{
  std::ostream&
  get_notification_stream()
  {
    static std::stringstream notification_stream;
    return notification_stream;
  }
}

namespace
{
  the::ctci::AutoServiceRegister< LocalEventDispatcher, LocalEventDispatcher >
    local_event_dispatcher_register;

  the::ctci::AutoServiceRegister< yarrr::EngineDispatcher, yarrr::EngineDispatcher >
    auto_engine_dispatcher_register;

  the::ctci::AutoServiceRegister< yarrrs::Notifier, yarrrs::Notifier > notifier_register(
      test::get_notification_stream() );
}

