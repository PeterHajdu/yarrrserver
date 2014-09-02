#include <yarrr/entity.hpp>
#include <yarrr/entity_factory.hpp>

namespace
{

class ClockSyncCatcher : public yarrr::Entity
{
  public:
    add_polymorphic_ctci( "clock_sync" );
};

yarrr::AutoEntityRegister< ClockSyncCatcher > clock_sync_catcher_register;

}

