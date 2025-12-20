#include "src/SharedDefs.hpp"
#include <cstdint>
#include <format>
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprlang.hpp>
#include <sdbus-c++/Error.h>
#include <stdexcept>
#include <string>

#include "globals.hpp"
#include "src/helpers/Color.hpp"

const std::string addNotificationSignatureDemangled =
    "CHyprNotificationOverlay::addNotification(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, CHyprColor const&, float, eIcons, float)";
typedef void (*origAddNotification)(void *, const std::string &, const CHyprColor &, const float, const eIcons, const float);
inline CFunctionHook *g_pAddNotificationHook = nullptr;
void hkAddNotification(void *thisptr, const std::string &text, const CHyprColor &color, const float timeMs, const eIcons icon, const float fontSize) {
  if (!g_pDbus->isAvailable()) {
    (*(origAddNotification)g_pAddNotificationHook->m_original)(thisptr, text, color, timeMs, icon, fontSize);
    return;
  }

  SIconMapping mapping = g_iconMappings.contains(icon) ? g_iconMappings.at(icon) : SIconMapping{.urgency = icon == ICON_ERROR ? Urgency::Critical : Urgency::Normal};

  try {
    // send notification to dbus interface
    g_pDbus->sendNotification(text, (int32_t)round(timeMs), mapping.icon, mapping.urgency);
  } catch (sdbus::Error &err) {
    Log::logger->log(Log::ERR, "[dbus-notifications] Unable to send dbus notification: {} - {}", err.getName().c_str(), err.getMessage());
  }
}

const std::string drawSignatureDemangled = "CHyprNotificationOverlay::draw(Hyprutils::Memory::CSharedPointer<CMonitor>)";
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

Hyprlang::CParseResult onNewIconMapping(const char *command, const char *value) {
  std::string value_str = value;
  CVarList vars(value_str);

  Hyprlang::CParseResult result;

  eIcons variant;
  SIconMapping mapping;

  // icon_mapping = <icon_variant>, icon:<notification_icon>, urgency:<notification_urgency>
  if (vars[0].empty()) {
    result.setError("icon variant cannot be empty");
    return result;
  } else {
    if (vars[0] == "warning")
      variant = ICON_WARNING;
    else if (vars[0] == "info")
      variant = ICON_INFO;
    else if (vars[0] == "hint")
      variant = ICON_HINT;
    else if (vars[0] == "error")
      variant = ICON_ERROR;
    else if (vars[0] == "confused")
      variant = ICON_CONFUSED;
    else if (vars[0] == "ok")
      variant = ICON_OK;
    else if (vars[0] == "none")
      variant = ICON_NONE;
    else {
      result.setError(std::format("got invalid icon variant {}: expected either warning, info, hint, error, confused, ok or none", vars[0]).c_str());
      return result;
    }
  }

  for (auto &var : std::ranges::subrange(vars.begin() + 1, vars.end())) {
    if (var.starts_with("urgency:")) {
      auto urgency_val = var.substr(8);
      if (urgency_val == "low")
        mapping.urgency = Urgency::Low;
      else if (urgency_val == "normal")
        mapping.urgency = Urgency::Normal;
      else if (urgency_val == "critical")
        mapping.urgency = Urgency::Critical;
      else {
        result.setError(std::format("got invalid urgency {}: expected either low, normal or critical", urgency_val).c_str());
        return result;
      }
    } else if (var.starts_with("icon:")) {
      auto icon_val = var.substr(5);
      mapping.icon = icon_val;
    }
  }

  g_iconMappings.insert_or_assign(variant, mapping);

  return result;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
  PHANDLE = handle;

  // check that header version aligns with running version
  const std::string CLIENT_HASH = __hyprland_api_get_client_hash();
  const std::string COMPOSITOR_HASH = __hyprland_api_get_hash();
  if (COMPOSITOR_HASH != CLIENT_HASH) {
    HyprlandAPI::addNotification(PHANDLE, "[dbus-notifications] Failed to load, mismatched versions", errorColor, errorTimeoutMs);
    throw std::runtime_error(std::format("Version mismatch, built against: {}, running compositor: {}", CLIENT_HASH, COMPOSITOR_HASH));
  }

  // hook functions
  static const auto NOTIFICATION_OVERLAY_METHODS = HyprlandAPI::findFunctionsByName(PHANDLE, "CHyprNotificationOverlay");

  for (auto &method : NOTIFICATION_OVERLAY_METHODS) {
    if (!g_pAddNotificationHook && method.demangled == addNotificationSignatureDemangled) {
      g_pAddNotificationHook = HyprlandAPI::createFunctionHook(handle, method.address, (void *)&hkAddNotification);
    }
    if (!g_pDrawHook && method.demangled == drawSignatureDemangled) {
      g_pDrawHook = HyprlandAPI::createFunctionHook(handle, method.address, (void *)&hkDraw);
    }
  }

  if (!g_pAddNotificationHook) {
    HyprlandAPI::addNotification(PHANDLE, "[dbus-notifications] Found no function which matches CHyprNotificationOverlay::addNotification signature", errorColor, errorTimeoutMs);
    throw std::runtime_error(std::format("Found no function which matches {} signature", addNotificationSignatureDemangled));
  } else {
    if (!g_pAddNotificationHook->hook()) {
      HyprlandAPI::addNotification(PHANDLE, "[dbus-notifications] Unable to hook into CHyprNotificationOverlay::addNotification function", errorColor, errorTimeoutMs);
      throw std::runtime_error(std::format("Unable to hook into {} function", addNotificationSignatureDemangled));
    }
  }

  if (!g_pDrawHook) {
    HyprlandAPI::addNotification(PHANDLE, "[dbus-notifications] Found no function which matches CHyprNotificationOverlay::draw signature", errorColor, errorTimeoutMs);
    throw std::runtime_error(std::format("Found no function which matches {} signature", drawSignatureDemangled));
  } else {
    if (!g_pDrawHook->hook()) {
      HyprlandAPI::addNotification(PHANDLE, "[dbus-notifications] Unable to hook into CHyprNotificationOverlay::draw function", errorColor, errorTimeoutMs);
      throw std::runtime_error(std::format("Unable to hook into {} function", drawSignatureDemangled));
    }
  }

  // add config
  if (!HyprlandAPI::addConfigKeyword(PHANDLE, CONFIG_KEYWORD_ICON_MAPPING, onNewIconMapping, Hyprlang::SHandlerOptions{})) {
    HyprlandAPI::addNotification(PHANDLE, "[dbus-notifications] Unable to add icon_map config keyword", errorColor, errorTimeoutMs);
    throw std::runtime_error("Unable to add icon_map config keyword");
  }

  // init dbus
  try {
    g_pDbus = std::make_unique<CDbus>();
  } catch (sdbus::Error &err) {
    HyprlandAPI::addNotification(PHANDLE, "[dbus-notifications] Unable to setup dbus notifications", errorColor, errorTimeoutMs);
    throw std::runtime_error(std::format("Unable to setup dbus notifications: {} - {}", err.getName().c_str(), err.getMessage()));
  }

  return {"dbus-notifications", "a plugin which turns Hyprland notifications into dbus notifications", "wysbd", "0.1"};
}

APICALL EXPORT void PLUGIN_EXIT() {}

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() { return HYPRLAND_API_VERSION; }
