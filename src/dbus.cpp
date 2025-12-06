#include "dbus.hpp"
#include <sdbus-c++/IProxy.h>
#include <sdbus-c++/Types.h>
#include <vector>

CDbus::CDbus() { connect(); }

void CDbus::connect() {
  pConnection = sdbus::createSessionBusConnection();
  pNotificationProxy = sdbus::createProxy(*pConnection, sdbus::BusName("org.freedesktop.Notifications"), sdbus::ObjectPath("/org/freedesktop/Notifications"));
}

uint32_t CDbus::sendNotification(const std::string &body, const int32_t timeout, const Urgency urgency) {
  if (!isAvailable())
    return 0;

  std::map<std::string, sdbus::Variant> hints;
  std::vector<std::string> actions;

  hints.insert_or_assign("urgency", sdbus::Variant((uint8_t)urgency));

  uint32_t id;

  pNotificationProxy->callMethod("Notify")
      .onInterface("org.freedesktop.Notifications")
      // app_name, replaced_id, icon_name, title, body, actions, hints, timeout
      .withArguments("Hyprland", 0u, "", "Hyprland", body, actions, hints, timeout)
      .storeResultsTo(id);
  return id;
}

const bool CDbus::isAvailable() { return pConnection && pNotificationProxy; }
