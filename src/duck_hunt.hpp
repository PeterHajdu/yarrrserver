#pragma once

#include <thetime/clock.hpp>

namespace yarrr
{

class ObjectContainer;

}

namespace yarrrs
{
class PlayerLoggedIn;

class DuckHunt
{
  public:
    DuckHunt( yarrr::ObjectContainer& objects, const the::time::Clock& );

  private:
    void handle_player_login( const PlayerLoggedIn& ) const;
    yarrr::ObjectContainer& m_objects;
    const the::time::Clock& m_clock;
};

}

