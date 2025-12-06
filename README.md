# hypr-dbus-notifications

This plugin transforms your inbuilt Hyprland notifications into dbus notifications which can be displayed using the notification service of your choice.

![](https://github.com/user-attachments/assets/2102f986-1c88-4904-8fd6-dbb700712f13)

## Installation

The plugin can be installed using `hyprpm`.

```bash
hyprpm add https://github.com/whysobad/hypr-dbus-notifications
hyprpm enable dbus-notifications
```

## Configuration

The plugin adds the `plugin:dbus-notifications:icon_map` keyword to your config. It can be used to map an Hyprland notification icon to an optional dbus notification urgency and an optional dbus icon.

>[!NOTE]
>Which dbus icons are supported depends on your notification service, the general spec guideline can be found [here](https://specifications.freedesktop.org/notification/1.2/icons-and-images.html)

At the moment the `warning`, `info`, `hint`, `error`, `confused`, `ok` and `none` Hyprland notification icons are supported. 

Additionally, the `low`, `normal` and `critical` notification urgencies are available. If the urgency is left empty, the `normal` urgency is used. 

The keyword can then be used like this:
```ini
# hyprland.conf
plugin:dbus-notifications {
    # syntax: map_icon = <icon_variant>,urgency:<notification_urgency>,icon:<notification_icon>
    
    # send all notificaitons which are sent using the `warning` icon with critical urgency
    map_icon = warning,urgency:critical
    # send all notifications which are sent using the `info` icon with an info icon
    map_icon = info,icon:info
}
```

By default, all icons are mapped to the `normal` urgency and have no icon (empty string). The only exception is the `error` icon which has `critical` urgency if left unchanged.

## Development

When developing this plugin, the Makefile can be used to build and (un)load the plugin. When building the plugin, the output file is located in `out/dbus-notifications.so`.

```bash
# build the plugin and load it into the active hyprland session
# this target also first unloads the plugin if it was already loaded before
make load

# unload the plugin from the active hyprland session
make unload
```