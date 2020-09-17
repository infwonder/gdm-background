#include <gio/gio.h>

static GDBusProxy *
get_proxy_helper()
{
    GError *error = NULL;
    GDBusConnection *system_bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if (error != NULL) {
	g_printerr("Error connecting to system bus: %s\n", error->message);
        g_error_free(error);
    }
    GDBusProxy *proxy_helper = g_dbus_proxy_new_sync(system_bus,G_DBUS_PROXY_FLAGS_NONE,
	NULL, "xyz.thiggy01.GDMBackground", "/xyz/thiggy01/GDMBackground",
	"xyz.thiggy01.GDMBackground", NULL, &error);
    if (error != NULL) {
	g_printerr("Error getting gdm-background proxy helper: %s\n", error->message);
	g_error_free(error);
    }
    return proxy_helper;
}

int
set_background_image(const gchar *task)
{
    GError *error = NULL;
    if (g_strcmp0(task, "backup") == 0) {
	g_dbus_proxy_call_sync(get_proxy_helper(), "SetImage", g_variant_new("(s)", task),
	G_DBUS_CALL_FLAGS_ALLOW_INTERACTIVE_AUTHORIZATION, -1, NULL, &error);
	if (error != NULL) {
	    g_printerr("Error calling method SetImage \"backup\" on proxy helper: %s\n",
		error->message);
	    g_error_free(error);
	    return -1;
	}
    } else if (g_strcmp0(task, "set") == 0) {
	GVariant *result = g_dbus_proxy_call_sync(get_proxy_helper(), "SetImage",
	g_variant_new("(s)", task), G_DBUS_CALL_FLAGS_ALLOW_INTERACTIVE_AUTHORIZATION,
	-1, NULL, &error);
	if (error != NULL) {
	    g_printerr("Error calling method SetImage \"set\" on proxy helper: %s\n",
		error->message);
	    g_error_free(error);
	    return -1;
	}
	GVariant *authorization = g_variant_get_child_value(result, 0);
	if (!g_variant_get_boolean(authorization)) {
	    return 1;
	}
	g_variant_unref(result);
	g_variant_unref(authorization);
    }
    return 0;
}

int
restore_backup_theme()
{
    GError *error = NULL;
    GVariant *result = g_dbus_proxy_call_sync(get_proxy_helper(), "RestoreBackup",
	g_variant_new("(s)", "restore"), G_DBUS_CALL_FLAGS_ALLOW_INTERACTIVE_AUTHORIZATION,
	-1, NULL, &error);
    if (error != NULL) {
	g_printerr("Error calling method RestoreBacktup on proxy helper: %s\n", error->message);
	g_error_free(error);
	return -1;
    }
    GVariant *authorization = g_variant_get_child_value(result, 0);
    if (!g_variant_get_boolean(authorization)) {
	return 1;
    }
    g_variant_unref(result);
    g_variant_unref(authorization);
    return 0;
}

int
restart_gdm_service(const gchar *action_id)
{
    GError *error = NULL;
    GVariant *result = g_dbus_proxy_call_sync(get_proxy_helper(), "RestartGDM",
	g_variant_new("(s)", action_id), G_DBUS_CALL_FLAGS_ALLOW_INTERACTIVE_AUTHORIZATION,
	-1, NULL, &error);
    if (error != NULL) {
	g_printerr("Error calling method RestartGDM on proxy helper: %s\n", error->message);
	g_error_free(error);
	return -1;
    }
    g_variant_unref(result);
    return 0;
}

