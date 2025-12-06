#pragma once

#include "dbus.hpp"
#include <hyprland/src/plugins/PluginAPI.hpp>

#define CONFIG_KEYWORD_ICON_MAPPING "plugin:dbus-notifications:icon_map"

inline HANDLE PHANDLE = nullptr;

inline const CHyprColor errorColor = CHyprColor{1.0, 0.2, 0.2, 1.0};
inline const uint32_t errorTimeoutMs = 5000;

struct SIconMapping {
  std::string icon = "";
  Urgency urgency = Urgency::Normal;
};

inline std::unordered_map<eIcons, SIconMapping> g_iconMappings;

inline std::unique_ptr<CDbus> g_pDbus = nullptr;
