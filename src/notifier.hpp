#pragma once

#include <thectci/id.hpp>
#include <ostream>

namespace yarrrs
{

class Notifier
{
  public:
    add_ctci( "yarrrs_notifier" );
    Notifier( std::ostream& notification_stream );
    void send( const std::string& ) const;

  private:
    std::ostream& m_notification_stream;
};

}

