#include "../src/db.hpp"

#include <unordered_map>
#include <unordered_set>

namespace test
{

class Db : public yarrrs::Db
{
  public:
    bool is_ok{ true };

    void commands_should_fail()
    {
      is_ok = false;
    }


    virtual bool set_hash_field(
        const std::string& key,
        const std::string& field,
        const std::string& value ) override
    {
      m_hashes[ key ][ field ] = value;
      return is_ok;
    }

    virtual bool get_hash_field(
        const std::string& key,
        const std::string& field,
        std::string& value ) override
    {
      if ( !has_key_field( key, field ) )
      {
        return false;
      }

      value = m_hashes.at( key ).at( field );
      return is_ok;
    }


    virtual bool key_exists( const std::string& key ) override
    {
      return m_hashes.find( key ) != std::end( m_hashes );
    }


    bool has_key_field(
      const std::string& key,
      const std::string& field ) const
    {
      auto hash_iterator( m_hashes.find( key ) );
      if ( hash_iterator  == std::end( m_hashes ) )
      {
        return false;
      }

      auto field_iterator( hash_iterator->second.find( field ) );
      if ( field_iterator == std::end( hash_iterator->second ) )
      {
        return false;
      }

      return is_ok;
    }

    virtual bool add_to_set(
        const std::string& key,
        const std::string& value ) override
    {
      m_sets[ key ].insert( value );
      return is_ok;
    }

    bool does_set_contain(
        const std::string& key,
        const std::string& value ) const
    {
      auto set_iterator( m_sets.find( key ) );
      if ( std::end( m_sets ) == set_iterator )
      {
        return false;
      }

      const auto& set( set_iterator->second );
      return set.find( value ) != std::end( set );
    }

  private:
    using Value = std::string;

    using FieldValue = std::unordered_map< Value, Value >;
    using Hashes = std::unordered_map< Value, FieldValue >;
    Hashes m_hashes;

    using Set = std::unordered_set< Value >;
    using Sets = std::unordered_map< Value, Set >;
    Sets m_sets;
};

}

