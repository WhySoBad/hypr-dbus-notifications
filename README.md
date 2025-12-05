# hypr-dbus-notifications

This plugin transforms your inbuilt Hyprland notifications into dbus notifications which can be displayed using the notification service of your choice.

<img width="2556" height="624" alt="image" src="https://github.com/user-attachments/assets/2102f986-1c88-4904-8fd6-dbb700712f13" />

## Installation

The plugin can be installed using `hyprpm`.

```bash
hyprpm add https://github.com/whysobad/hypr-dbus-notifications
hyprpm enable dbus-notifications
```

## Development

When developing this plugin, the Makefile can be used to build and (un)load the plugin. When building the plugin, the output file is located in `out/dbus-notifications.so`.

```bash
# build the plugin and load it into the active hyprland session
# this target also first unloads the plugin if it was already loaded before
make load

# unload the plugin from the active hyprland session
make unload
```