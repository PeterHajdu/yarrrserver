Coordinate.new( 1, 2 )
TileCoordinate.new( 1, 2 )
a_tile = Tile.new(
  TileCoordinate.new( 0, 0 ),
  TileCoordinate.new( 1, -1 ) )

a_shape = Shape.new()
a_shape:add_tile( a_tile )

behavior = ShapeBehavior.new( a_shape )
behavior.shape = a_shape

ShapeGraphics.new()

PhysicalBehavior.new()
an_object = Object.new()
an_object:add_behavior( PhysicalBehavior.new() )

Inventory.new()

assert( ship_layer )
Collider.new( ship_layer )
DamageCauser.new( 100 )
LootDropper.new()
DeleteWhenDestroyed.new()
RespawnWhenDestroyed.new()

assert( main_thruster )
Thruster.new( main_thruster, Coordinate.new( 0, 0 ), 180 )
assert( port_thruster )
assert( starboard_thruster )

Canon.new( TileCoordinate.new( 0, 0 ) )

