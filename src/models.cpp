#include "models.hpp"
#include <yarrr/mission_exporter.hpp>

namespace yarrrs
{
const std::string
Models::players_path( "modells.player" );

Models::Models( the::model::Lua& lua )
  : mission_contexts( yarrr::mission_contexts, lua )
{
}

}

