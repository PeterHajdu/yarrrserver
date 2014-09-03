#include "../src/notifier.hpp"
#include <thectci/service_registry.hpp>
#include <igloo/igloo_alt.h>

using namespace igloo;

Describe( a_notifier )
{
  It( sends_notifications_to_the_notification_stream )
  {
    std::stringstream notification_stream;
    yarrrs::Notifier notifier( notification_stream );
    notifier.send( a_notification_message );
    AssertThat( notification_stream.str(), Contains( a_notification_message ) );
  }

  const std::string a_notification_message{ "a notification message" };
};

