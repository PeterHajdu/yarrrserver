#pragma once
#include <yarrr/db.hpp>
#include <string>

namespace yarrrs
{

class RedisDb : public yarrr::Db
{
  public:
    virtual bool set_hash_field(
        const std::string& key,
        const std::string& field,
        const std::string& value ) override;

    virtual bool get_hash_field(
        const std::string& key,
        const std::string& field,
        std::string& value ) override;

    virtual bool add_to_set(
        const std::string& key,
        const std::string& value ) override;

    virtual bool key_exists( const std::string& key ) override;

    virtual bool get_set_members(
        const std::string& key,
        Values& ) override;

    virtual bool get_hash_fields(
        const std::string& key,
        Values& ) override;
};

}

