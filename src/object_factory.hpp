#pragma once

#include <yarrr/object.hpp>
#include <functional>

namespace yarrrs
{

class ObjectFactory final
{
  public:
    typedef std::function< yarrr::Object::Pointer() > Creator;
    yarrr::Object::Pointer create_a( const std::string& key );
    void register_creator( const std::string& key, Creator creator );
};

}

