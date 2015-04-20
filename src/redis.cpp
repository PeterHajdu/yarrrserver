#include "redis.hpp"
#include <yarrr/log.hpp>
#include <theconf/configuration.hpp>
#include <hiredis/hiredis.h>
#include <memory>
#include <cassert>
#include <unordered_map>

namespace
{

const std::unordered_map< int, std::string > redis_type_to_string{
  { REDIS_REPLY_ERROR, "REDIS_REPLY_ERROR" },
  { REDIS_REPLY_STATUS, "REDIS_REPLY_STATUS" },
  { REDIS_REPLY_INTEGER, "REDIS_REPLY_INTEGER" },
  { REDIS_REPLY_NIL, "REDIS_REPLY_NIL" },
  { REDIS_REPLY_STRING, "REDIS_REPLY_STRING" },
  { REDIS_REPLY_ARRAY, "REDIS_REPLY_ARRAY" } };

auto free_context = []( redisContext* context ){ redisFree( context ); };
auto free_reply = []( redisReply* reply ){ freeReplyObject( reply ); };

class RedisCommand
{
  public:
    RedisCommand( std::string command )
      : m_command( std::move( command ) )
      , m_context( redisConnect(
            the::conf::get_value( "redis_ip" ).c_str(),
            the::conf::get< int >( "redis_port" ) ),
          free_context )
      , m_reply( static_cast< redisReply* >( redisCommand( m_context.get(), m_command.c_str() ) ), free_reply )
    {
      if ( !is_ok() )
      {
        thelog( yarrr::log::error )( "Redis command finished with error:", m_reply->str, "command:", m_command );
        return;
      }

      thelog( yarrr::log::debug )(
          "Redis command executed:", m_command,
          "reply is of type:", redis_type_to_string.at( m_reply->type ),
          "with value:", string_value_of_reply() );
    }

    bool is_ok() const
    {
      return m_reply->type != REDIS_REPLY_ERROR;
    }

    bool is_integer() const
    {
      assert( is_ok() );
      return m_reply->type == REDIS_REPLY_INTEGER;
    }

    long long integer() const
    {
      assert( is_integer() );
      return m_reply->integer;
    }

    bool is_string() const
    {
      assert( is_ok() );
      return m_reply->type == REDIS_REPLY_STRING;
    }

    std::string string() const
    {
      assert( is_string() );
      return std::string( m_reply->str, m_reply->len );
    }

    std::string string_value_of_reply()
    {
      if ( is_integer() )
      {
        return std::to_string( integer() );
      }

      if ( is_string() )
      {
        return string();
      }

      return "n/a";
    }

  private:
    std::string m_command;
    std::unique_ptr< redisContext, decltype( free_context ) > m_context;
    std::unique_ptr< redisReply, decltype( free_reply ) > m_reply;
};

}

namespace yarrrs
{

bool
RedisDb::set_hash_field(
    const std::string& key,
    const std::string& field,
    const std::string& value )
{
  RedisCommand set_field( std::string( "hset " ) + key + " " + field + " " + value );
  return set_field.is_ok();
}


bool
RedisDb::get_hash_field(
    const std::string& key,
    const std::string& field,
    std::string& value )
{
  RedisCommand get_field( std::string( "hget " ) + key + " " + field );
  if ( !get_field.is_ok() )
  {
    return false;
  }

  value = get_field.string_value_of_reply();

  return true;
}


bool
RedisDb::add_to_set(
    const std::string& key,
    const std::string& value )
{
  RedisCommand add_member( std::string( "sadd " ) + key + " " + value );
  return add_member.is_ok();
}


bool
RedisDb::key_exists( const std::string& key )
{
  RedisCommand does_exist( std::string( "exists " ) + key );
  return
    does_exist.is_ok() &&
    does_exist.is_integer() &&
    does_exist.integer() == 1;
}

}

