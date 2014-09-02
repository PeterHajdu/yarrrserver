#include "../src/player.hpp"
#include "../src/local_event_dispatcher.hpp"
#include <yarrr/object_container.hpp>
#include <thectci/service_registry.hpp>
#include <igloo/igloo_alt.h>

using namespace igloo;

Describe( a_resource_finder )
{
  void SetUp()
  {
    players.reset( new yarrrs::Players( objects ) );
  }

  It( should_not_crash_if_logout_arrives_from_invalid_connection )
  {
    the::ctci::service< LocalEventDispatcher >().dispatcher.dispatch( yarrrs::PlayerLoggedOut( 100 ) );
  }

  yarrr::ObjectContainer objects;
  std::unique_ptr< yarrrs::Players > players;
};

