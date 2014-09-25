#include <yarrr/shape.hpp>
#include <yarrr/basic_behaviors.hpp>
#include <igloo/igloo_alt.h>

#include <sol.hpp>

using namespace igloo;

Describe( shape_creation )
{
  void SetUp()
  {
    lua.reset( new sol::state() );
    lua->new_userdata< yarrr::Tile::Coordinate, int, int >( "coordinate" );
    lua->new_userdata< yarrr::Tile, yarrr::Tile::Coordinate, yarrr::Tile::Coordinate >( "tile" );
    lua->new_userdata< yarrr::Shape >( "shape", "add_tile", &yarrr::Shape::add_tile );
    lua->new_userdata< yarrr::PhysicalBehavior >( "physical_behavior" );
    lua->new_userdata< yarrr::Object >( "object", "add_behavior", &yarrr::Object::add_behavior_clone );
  }

  It( can_create_a_coordinate )
  {
    lua->script( "apple = coordinate.new( 1, 2 )" );
    const auto apple( lua->get< yarrr::Tile::Coordinate >( "apple" ) );
    AssertThat( apple, Equals( yarrr::Tile::Coordinate{ 1, 2 } ) );
  }

  It( can_create_a_tile )
  {
    lua->script( "tree = tile.new( coordinate.new( 1, 2 ), coordinate.new( 3, 4 ) )" );
    const auto tree( lua->get< yarrr::Tile >( "tree" ) );
    AssertThat( tree, Equals( yarrr::Tile{ { 1, 2 }, { 3, 4 } } ) );
  }

  It( can_create_a_shape )
  {
    lua->script( "a_shape = shape.new()\na_shape:add_tile( tile.new( coordinate.new( 1, 2 ), coordinate.new( 3, 4 ) ) )" );
    const auto shape( lua->get< yarrr::Shape >( "a_shape" ) );
    AssertThat( shape.tiles(), HasLength( 1 ) );
    AssertThat( shape.tiles().back(), Equals( yarrr::Tile{ { 1, 2 }, { 3, 4 } } ) );
  }

  It( can_create_a_physical_behavior )
  {
    lua->script( "physics = physical_behavior.new()" );
    auto behavior( lua->get< yarrr::PhysicalBehavior >( "physics" ) );

    yarrr::Object object;
    object.add_behavior( behavior.clone() );
    AssertThat( object.components.has_component< yarrr::PhysicalBehavior >(), Equals( true ) );
  }

  It( can_create_an_object )
  {
    lua->script( "an_object = object.new()" );
    lua->script( "an_object:add_behavior( physical_behavior.new() )" );

    auto& object( lua->get< yarrr::Object >( "an_object" ) );
    AssertThat( object.components.has_component< yarrr::PhysicalBehavior >(), Equals( true ) );
  }

  It( can_return_an_object_from_a_function )
  {
    lua->script( "function create_object()\nreturn object.new()\nend" );
    sol::function object_creator( (*lua)[ "create_object" ] );
    auto& object( object_creator.call< yarrr::Object& >() );
    (void)object;
  }

  std::unique_ptr< sol::state > lua;
};

