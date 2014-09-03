#include "notifier.hpp"

namespace yarrrs
{

Notifier::Notifier( std::ostream& notification_stream )
  : m_notification_stream( notification_stream )
{
}

void
Notifier::send( const std::string& message ) const
{
  m_notification_stream << message << std::endl;
}

}

