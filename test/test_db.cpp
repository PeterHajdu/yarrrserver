#include "../src/db.hpp"
#include <igloo/igloo_alt.h>

using namespace igloo;

Describe( key_generators )
{

  It( generates_user_keys_from_user_id )
  {
    AssertThat( yarrrs::user_key_from_id( "dogfood" ), Equals( "user:dogfood" ) );
  }

};

