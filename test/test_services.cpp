#include <thectci/service_registry.hpp>
#include "../src/local_event_dispatcher.hpp"

namespace
{
  the::ctci::AutoServiceRegister< LocalEventDispatcher, LocalEventDispatcher >
    local_event_dispatcher_register;
}
