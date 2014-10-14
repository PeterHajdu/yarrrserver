#include "duck_hunt.hpp"
#include "object_factory.hpp"
#include "local_event_dispatcher.hpp"
#include "network_service.hpp"
#include <thectci/service_registry.hpp>
#include <yarrr/object_container.hpp>
#include <yarrr/object_creator.hpp>
#include <yarrr/object.hpp>
#include <yarrr/object_behavior.hpp>
#include <yarrr/inventory.hpp>
#include <yarrr/canon.hpp>
#include <yarrr/basic_behaviors.hpp>
#include <yarrr/thruster.hpp>
#include <yarrr/physical_parameters.hpp>
#include <yarrr/collider.hpp>
#include <yarrr/destruction_handlers.hpp>
#include <yarrr/shape_behavior.hpp>
#include <yarrr/entity_factory.hpp>
#include <thetime/timer.hpp>

#include <random>

using namespace yarrr;

namespace
{
std::random_device random_device;
std::default_random_engine random_engine( random_device() );
std::uniform_int_distribution<int> velocity_distribution{ -75_metres, 75_metres };
std::uniform_int_distribution<int> angular_velocity_distribution{ -500_degrees, +500_degrees };

void randomize_physical_behavior( const the::time::Time& now, yarrr::PhysicalBehavior& behavior )
{
  behavior.physical_parameters.coordinate = yarrr::Coordinate( 0, 0 );
  behavior.physical_parameters.velocity += yarrr::Coordinate(
      velocity_distribution( random_engine ),
      velocity_distribution( random_engine ) );

  behavior.physical_parameters.angular_velocity +=
    angular_velocity_distribution( random_engine );

  behavior.physical_parameters.timestamp = now;
}

yarrr::Object::Pointer create_duck( const the::time::Time& now )
{
  yarrr::Object::Pointer duck( the::ctci::service< yarrrs::ObjectFactory >().create_a( "duck" ) );

  if ( !duck )
  {
    duck = the::ctci::service< yarrrs::ObjectFactory >().create_a( "ship" );
  }

  duck->add_behavior( ObjectBehavior::Pointer( new SelfDestructor( duck->id, 360000000u ) ) );
  randomize_physical_behavior( now, yarrr::component_of< yarrr::PhysicalBehavior>( *duck ) );

  return duck;
}

}

namespace yarrrs
{

DuckHunt::DuckHunt( yarrr::ObjectContainer& objects, const the::time::Clock& clock )
  : m_objects( objects )
  , m_clock( clock )
{
  the::ctci::Dispatcher& local_event_dispatcher(
      the::ctci::service< LocalEventDispatcher >().dispatcher );
  local_event_dispatcher.register_listener< PlayerLoggedIn >(
      std::bind( &DuckHunt::handle_player_login, this, std::placeholders::_1 ) );
}


void
DuckHunt::handle_player_login( const PlayerLoggedIn& ) const
{
  3_times( [ this ]() { m_objects.add_object( create_duck( m_clock.now() ) ); } );
}

}

