#pragma once
#include <thectci/id.hpp>
#include <string>

namespace yarrrs
{

class Db
{
  public:
    add_ctci( "yarrrs_db" );
    virtual ~Db() = default;

    virtual bool set_hash_field(
        const std::string& key,
        const std::string& field,
        const std::string& value ) = 0;

    virtual bool get_hash_field(
        const std::string& key,
        const std::string& field,
        std::string& value ) = 0;

    virtual bool add_to_set(
        const std::string& key,
        const std::string& value ) = 0;

    virtual bool key_exists( const std::string& key ) = 0;
};

class RedisDb : public Db
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
};

std::string
user_key_from_id( const std::string& user_id );

}

