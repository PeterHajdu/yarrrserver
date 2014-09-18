#pragma once

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
    DuckHunt( yarrr::ObjectContainer& objects );

  private:
    void handle_player_login( const PlayerLoggedIn& ) const;
    yarrr::ObjectContainer& m_objects;
};

}

