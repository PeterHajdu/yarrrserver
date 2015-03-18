#pragma once

#include <themodel/node_list.hpp>
#include <thectci/id.hpp>

namespace yarrrs
{

class Models
{
  public:
    add_ctci( "yarrrs_models" );
    Models( the::model::Lua& lua );

    using MissionContexts = the::model::OwningNode;
    MissionContexts mission_contexts;
};

}

