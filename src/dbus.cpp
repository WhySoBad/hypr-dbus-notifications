#include "dbus.hpp"
#include "globals.hpp"
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <sdbus-c++/Error.h>
#include <sdbus-c++/IProxy.h>
#include <sdbus-c++/Types.h>
#include <vector>

CDbus::CDbus() {
  try {
    pConnection = sdbus::createSessionBusConnection();
  } catch (sdbus::Error &err) {
    HyprlandAPI::addNotification(PHANDLE, "[dbus-notifications] Unable to connect to dbus session bus", CHyprColor{1.0, 0.2, 0.2, 1.0}, 5000);
    Debug::log(ERR, "[dbus-notifications] Unable to connect to dbus session bus: {} - {}", err.getName().c_str(), err.getMessage());
    return;
  }

  try {
    pNotificationProxy = sdbus::createProxy(*pConnection, sdbus::BusName("org.freedesktop.Notifications"), sdbus::ObjectPath("/org/freedesktop/Notifications"));
  } catch (sdbus::Error &err) {
    HyprlandAPI::addNotification(PHANDLE, "[dbus-notifications] Unable to create dbus notifications proxy", CHyprColor{1.0, 0.2, 0.2, 1.0}, 5000);
    Debug::log(ERR, "[dbus-notifications] Unable to create dbus notifications proxy: {} - {}", err.getName().c_str(), err.getMessage());
    return;
  }
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
