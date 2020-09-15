#ifndef DBUS_CLIENT_H
#define DBUS_CLIENT_H

int set_background_image(const gchar *task);

int restore_backup_theme();

int restart_gdm_service(const gchar *action_id);

#endif
