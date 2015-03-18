#pragma once

#include <thectci/dispatcher.hpp>
#include <thectci/service_registry.hpp>

//todo: move to namespace yarrrs
class LocalEventDispatcher
{
  public:
    add_ctci( "local_event_dispatcher" );
    the::ctci::Dispatcher dispatcher;
};

