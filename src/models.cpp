#include "models.hpp"
#include <yarrr/mission_exporter.hpp>

namespace yarrrs
{

Models::Models( the::model::Lua& lua )
  : mission_contexts( yarrr::mission_contexts, lua )
{
}

}

