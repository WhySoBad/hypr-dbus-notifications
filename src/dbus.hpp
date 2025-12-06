#pragma once

#include <memory>
#include <sdbus-c++/IConnection.h>
#include <sdbus-c++/IProxy.h>

enum Urgency { Low = 0, Normal = 1, Critical = 2 };

class CDbus {
public:
  std::unique_ptr<sdbus::IConnection> pConnection = nullptr;
  std::unique_ptr<sdbus::IProxy> pNotificationProxy = nullptr;

  CDbus();
  void connect();
  /*
   * Send a notification to the org.freedesktop.Notifications dbus interface
   * Returns 0 when the dbus connection/proxy is not available
   *
   * @see https://specifications.freedesktop.org/notification/1.2/protocol.html
   */
  uint32_t sendNotification(const std::string &body, const int32_t timeout, const Urgency urgency);
  const bool isAvailable();
};