#include "duck_hunt.hpp"
#include <thectci/service_registry.hpp>
#include "local_event_dispatcher.hpp"
#include "network_service.hpp"
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

ObjectBehavior::Pointer random_physical_behavior( const the::time::Time& now )
{
  PhysicalBehavior* behavior( new PhysicalBehavior() );

  behavior->physical_parameters.coordinate = yarrr::Coordinate( 0, 0 );

  behavior->physical_parameters.velocity += yarrr::Coordinate(
      velocity_distribution( random_engine ),
      velocity_distribution( random_engine ) );

  behavior->physical_parameters.angular_velocity +=
    angular_velocity_distribution( random_engine );

  behavior->physical_parameters.timestamp = now;

  return ObjectBehavior::Pointer( behavior );
}

yarrr::Object::Pointer create_duck( const the::time::Time& now )
{
  yarrr::Object::Pointer duck( new Object() );
  duck->add_behavior( random_physical_behavior( now ) );
  duck->add_behavior( ObjectBehavior::Pointer( new yarrr::Inventory() ) );

  ShapeBehavior* shape( new ShapeBehavior() );
  shape->shape.add_tile( Tile{ { -1, 0 }, { 2, 0 } } );
  shape->shape.add_tile( Tile{ { 0, 1 }, { 0, 1 } } );
  shape->shape.add_tile( Tile{ { 0, -1 }, { 0, -1 } } );
  duck->add_behavior( ObjectBehavior::Pointer( shape ) );

  const Coordinate main_thruster_relative_to_center_of_mass(
      Coordinate{ -5_metres, Tile::unit_length / 2 } - shape->shape.center_of_mass() );

  duck->add_behavior( ObjectBehavior::Pointer( new Thruster(
          Command::main_thruster,
          main_thruster_relative_to_center_of_mass,
          180_degrees ) ) );

  const Coordinate front_thrusters_relative_to_center_of_mass(
      Coordinate{ 15_metres, Tile::unit_length / 2 } - shape->shape.center_of_mass() );

  duck->add_behavior( ObjectBehavior::Pointer( new Thruster(
          Command::port_thruster,
          front_thrusters_relative_to_center_of_mass,
          90_degrees ) ) );

  duck->add_behavior( ObjectBehavior::Pointer( new Thruster(
          Command::starboard_thruster,
          front_thrusters_relative_to_center_of_mass,
          -90_degrees ) ) );

  duck->add_behavior( ObjectBehavior::Pointer( new Canon() ) );
  duck->add_behavior( ObjectBehavior::Pointer( new Collider( Collider::ship_layer ) ) );
  duck->add_behavior( ObjectBehavior::Pointer( new DamageCauser( 100 ) ) );
  duck->add_behavior( ObjectBehavior::Pointer( new LootDropper() ) );
  duck->add_behavior( ObjectBehavior::Pointer( new ShapeGraphics() ) );
  duck->add_behavior( ObjectBehavior::Pointer( new DeleteWhenDestroyed() ) );
  duck->add_behavior( ObjectBehavior::Pointer( new SelfDestructor( duck->id, 360000000u ) ) );

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

