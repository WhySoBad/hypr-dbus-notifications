#include "dbus.hpp"
#include "src/debug/Log.hpp"
#include <cmath>
#include <cstdint>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <memory>
#include <string>

#include "globals.hpp"
#include "src/desktop/DesktopTypes.hpp"

std::unique_ptr<CDbus> g_pDbus = nullptr;

typedef void (*origAddNotification)(void *, const std::string &, const CHyprColor &, const float, const eIcons, const float);
inline CFunctionHook *g_pAddNotificationHook = nullptr;
void hkAddNotification(void *thisptr, const std::string &text, const CHyprColor &color, const float timeMs, const eIcons icon, const float fontSize) {
  if (!g_pDbus->isAvailable()) {
    (*(origAddNotification)g_pAddNotificationHook->m_original)(thisptr, text, color, timeMs, icon, fontSize);
    return;
  }

  try {
      // send notification to dbus interface
      g_pDbus->sendNotification(text, (int32_t)round(timeMs), Urgency::Critical);
  } catch(sdbus::Error& err) {
      Debug::log(ERR, "[dbus-notifications] Unable to send dbus notification: {} - {}", err.getName().c_str(), err.getMessage());
  }
}

typedef void (*origDraw)(void *, PHLMONITOR);
inline CFunctionHook *g_pDrawHook = nullptr;
void hkDraw(void *thisptr, PHLMONITOR monitor) {
  if (!g_pDbus->isAvailable()) {
    (*(origDraw)g_pDrawHook->m_original)(thisptr, monitor);
    return;
  }
  // we don't need to do anything, just ensure the original draw function is never
  // executed in order to never allocate the cairo surface
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
  PHANDLE = handle;

  // check that header version aligns with running version
  const std::string CLIENT_HASH = __hyprland_api_get_client_hash();
  const std::string COMPOSITOR_HASH = __hyprland_api_get_hash();
  if (COMPOSITOR_HASH != CLIENT_HASH) {
    HyprlandAPI::addNotification(PHANDLE, "[dbus-notifications] Failed to load, mismatched versions", CHyprColor{1.0, 0.2, 0.2, 1.0}, 5000);
    throw std::runtime_error(std::format("version mismatch, built against: {}, running compositor: {}", CLIENT_HASH, COMPOSITOR_HASH));
  }

  // init dbus
  g_pDbus = std::make_unique<CDbus>();

  // hook functions
  static const auto NOTIFICATION_OVERLAY_METHODS = HyprlandAPI::findFunctionsByName(PHANDLE, "CHyprNotificationOverlay");

  Debug::log(TRACE, "Found {} matching functions for `CHyprNotificationOverlay` name:", NOTIFICATION_OVERLAY_METHODS.size());
  for (auto method : NOTIFICATION_OVERLAY_METHODS) {
    Debug::log(TRACE, "Address: {}, Signature: {}", method.address, method.signature);
  }

  g_pAddNotificationHook = HyprlandAPI::createFunctionHook(handle, NOTIFICATION_OVERLAY_METHODS[0].address, (void *)&hkAddNotification);
  g_pDrawHook = HyprlandAPI::createFunctionHook(handle, NOTIFICATION_OVERLAY_METHODS[3].address, (void *)&hkDraw);

  g_pAddNotificationHook->hook();
  g_pDrawHook->hook();

  return {"dbus-notifications", "a plugin which turns Hyprland notifications into dbus notifications", "wysbd", "0.1"};
}

APICALL EXPORT void PLUGIN_EXIT() {}

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() { return HYPRLAND_API_VERSION; }
