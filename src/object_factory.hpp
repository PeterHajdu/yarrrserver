#pragma once

#include <yarrr/object.hpp>

namespace yarrrs
{

class ObjectFactory final
{
  public:
    yarrr::Object::Pointer create_a( const std::string& key );
};

}

