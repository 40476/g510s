/*
 *  This file is part of g510s.
 *
 *  g510s is  free  software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as published by
 *  the  Free Software Foundation; either version 3 of the License, or (at your
 *  option)  any later version.
 *
 *  g510s is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with g510s; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1335  USA
 *
 *  Copyright © 2015 John Augustine
 *  Copyright © 2025 usr_40476
 */


#include <stdlib.h>
#include <pthread.h>
#include <libg15.h>
#include <libg15render.h>
#include <gtk/gtk.h> // Use standard GTK include for type visibility
#include <libappindicator3-0.1/libappindicator/app-indicator.h>
#include <glib-2.0/gio/gio.h> // Add for DBus support
#include <glib.h>    // Add for guint definition

#include "g510s.h"

// DBus interface and object path
#define G510S_DBUS_NAME "org.g510s.control"
#define G510S_DBUS_PATH "/org/g510s/control"
#define G510S_DBUS_INTERFACE "org.g510s.control"

// DBus property names (add all settings)
#define G510S_DBUS_PROP_MODE "Mode"
#define G510S_DBUS_PROP_COLOR "Color"
#define G510S_DBUS_PROP_COLORFADE "ColorFade"
#define G510S_DBUS_PROP_AUTOSAVE "AutoSaveOnQuit"
#define G510S_DBUS_PROP_GUIHIDDEN "GuiHidden"
// Add more as needed for all settings

// Forward declarations for DBus handlers
static gboolean on_handle_set_mode(GDBusConnection *connection, const gchar *sender, const gchar *object_path,
                                   const gchar *interface_name, const gchar *method_name, GVariant *parameters,
                                   GDBusMethodInvocation *invocation, gpointer user_data);

static gboolean on_handle_set_color(GDBusConnection *connection, const gchar *sender, const gchar *object_path,
                                    const gchar *interface_name, const gchar *method_name, GVariant *parameters,
                                    GDBusMethodInvocation *invocation, gpointer user_data);

static gboolean on_handle_set_colorfade(GDBusConnection *connection, const gchar *sender, const gchar *object_path,
                                        const gchar *interface_name, const gchar *method_name, GVariant *parameters,
                                        GDBusMethodInvocation *invocation, gpointer user_data);
static gboolean on_handle_set_autosave(GDBusConnection *connection, const gchar *sender, const gchar *object_path,
                                        const gchar *interface_name, const gchar *method_name, GVariant *parameters,
                                        GDBusMethodInvocation *invocation, gpointer user_data);
static gboolean on_handle_set_guihidden(GDBusConnection *connection, const gchar *sender, const gchar *object_path,
                                        const gchar *interface_name, const gchar *method_name, GVariant *parameters,
                                        GDBusMethodInvocation *invocation, gpointer user_data);

static void emit_properties_changed(GDBusConnection *connection);
static gboolean on_handle_save_config(GDBusConnection *connection, const gchar *sender, const gchar *object_path,
                                      const gchar *interface_name, const gchar *method_name, GVariant *parameters,
                                      GDBusMethodInvocation *invocation, gpointer user_data);

// Forward declarations for macro DBus handlers
static gboolean on_handle_get_macro(GDBusConnection *connection, const gchar *sender, const gchar *object_path,
                                    const gchar *interface_name, const gchar *method_name, GVariant *parameters,
                                    GDBusMethodInvocation *invocation, gpointer user_data);
static gboolean on_handle_set_macro(GDBusConnection *connection, const gchar *sender, const gchar *object_path,
                                    const gchar *interface_name, const gchar *method_name, GVariant *parameters,
                                    GDBusMethodInvocation *invocation, gpointer user_data);
static gboolean on_handle_run_macro(GDBusConnection *connection, const gchar *sender, const gchar *object_path,
                                    const gchar *interface_name, const gchar *method_name, GVariant *parameters,
                                    GDBusMethodInvocation *invocation, gpointer user_data);

// --- GUI refresh support ---
#include <string.h>

static void refresh_gui_internal();
static gboolean refresh_gui_idle(gpointer data) {
    refresh_gui_internal();
    return G_SOURCE_REMOVE;
}
void refresh_gui() {
    g_idle_add(refresh_gui_idle, NULL);
}

// DBus property getter
static GVariant* on_get_property(GDBusConnection *connection, const gchar *sender, const gchar *object_path,
                                 const gchar *interface_name, const gchar *property_name, GError **error, gpointer user_data) {
    if (g_strcmp0(property_name, G510S_DBUS_PROP_MODE) == 0) {
        // Fetch mode on demand
        return g_variant_new_int32(g510s_data.mkey_state);
    } else if (g_strcmp0(property_name, G510S_DBUS_PROP_COLOR) == 0) {
        // Fetch color on demand
        int color[3] = {g510s_data.led_red, g510s_data.led_green, g510s_data.led_blue};
        return g_variant_new_fixed_array(G_VARIANT_TYPE_INT32, color, 3, sizeof(int));
    } else if (g_strcmp0(property_name, G510S_DBUS_PROP_COLORFADE) == 0) {
        return g_variant_new_boolean(g510s_data.color_fade);
    } else if (g_strcmp0(property_name, G510S_DBUS_PROP_AUTOSAVE) == 0) {
        return g_variant_new_boolean(g510s_data.auto_save_on_quit);
    } else if (g_strcmp0(property_name, G510S_DBUS_PROP_GUIHIDDEN) == 0) {
        return g_variant_new_boolean(g510s_data.gui_hidden);
    }
    // ... add more as needed ...
    return NULL;
}

// DBus property setter (allow changing settings via DBus)
static gboolean on_set_property(GDBusConnection *connection, const gchar *sender, const gchar *object_path,
                                const gchar *interface_name, const gchar *property_name, GVariant *value, GError **error, gpointer user_data) {
    if (g_strcmp0(property_name, G510S_DBUS_PROP_COLORFADE) == 0) {
        g510s_data.color_fade = g_variant_get_boolean(value);
        save_config();
        emit_properties_changed(connection);
        refresh_gui();
        return TRUE;
    } else if (g_strcmp0(property_name, G510S_DBUS_PROP_AUTOSAVE) == 0) {
        g510s_data.auto_save_on_quit = g_variant_get_boolean(value);
        save_config();
        emit_properties_changed(connection);
        refresh_gui();
        return TRUE;
    } else if (g_strcmp0(property_name, G510S_DBUS_PROP_GUIHIDDEN) == 0) {
        g510s_data.gui_hidden = g_variant_get_boolean(value);
        save_config();
        emit_properties_changed(connection);
        refresh_gui();
        return TRUE;
    }
    // ... add more as needed ...
    return FALSE;
}

// Helper to emit PropertiesChanged (add all properties)
static void emit_properties_changed(GDBusConnection *connection) {
    GVariantBuilder builder;
    GVariantBuilder invalidated;
    g_variant_builder_init(&builder, G_VARIANT_TYPE_VARDICT);
    g_variant_builder_init(&invalidated, G_VARIANT_TYPE_STRING_ARRAY);
    g_variant_builder_add(&builder, "{sv}", G510S_DBUS_PROP_MODE, g_variant_new_int32(g510s_data.mkey_state));
    int color[3] = {g510s_data.led_red, g510s_data.led_green, g510s_data.led_blue};
    g_variant_builder_add(&builder, "{sv}", G510S_DBUS_PROP_COLOR, g_variant_new_fixed_array(G_VARIANT_TYPE_INT32, color, 3, sizeof(int)));
    g_variant_builder_add(&builder, "{sv}", G510S_DBUS_PROP_COLORFADE, g_variant_new_boolean(g510s_data.color_fade));
    g_variant_builder_add(&builder, "{sv}", G510S_DBUS_PROP_AUTOSAVE, g_variant_new_boolean(g510s_data.auto_save_on_quit));
    g_variant_builder_add(&builder, "{sv}", G510S_DBUS_PROP_GUIHIDDEN, g_variant_new_boolean(g510s_data.gui_hidden));
    // ... add more as needed ...
    g_dbus_connection_emit_signal(
        connection,
        NULL,
        G510S_DBUS_PATH,
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged",
        g_variant_new("(sa{sv}as)", G510S_DBUS_INTERFACE, &builder, &invalidated),
        NULL
    );
}

// DBus method handler
static void on_method_call(GDBusConnection *connection, const gchar *sender, const gchar *object_path,
                           const gchar *interface_name, const gchar *method_name, GVariant *parameters,
                           GDBusMethodInvocation *invocation, gpointer user_data) {
    if (g_strcmp0(method_name, "SetMode") == 0) {
        on_handle_set_mode(connection, sender, object_path, interface_name, method_name, parameters, invocation, user_data);
    } else if (g_strcmp0(method_name, "SetColor") == 0) {
        on_handle_set_color(connection, sender, object_path, interface_name, method_name, parameters, invocation, user_data);
    } else if (g_strcmp0(method_name, "SetColorFade") == 0) {
        on_handle_set_colorfade(connection, sender, object_path, interface_name, method_name, parameters, invocation, user_data);
    } else if (g_strcmp0(method_name, "SetAutoSaveOnQuit") == 0) {
        on_handle_set_autosave(connection, sender, object_path, interface_name, method_name, parameters, invocation, user_data);
    } else if (g_strcmp0(method_name, "SetGuiHidden") == 0) {
        on_handle_set_guihidden(connection, sender, object_path, interface_name, method_name, parameters, invocation, user_data);
    } else if (g_strcmp0(method_name, "GetMacro") == 0) {
        on_handle_get_macro(connection, sender, object_path, interface_name, method_name, parameters, invocation, user_data);
    } else if (g_strcmp0(method_name, "SetMacro") == 0) {
        on_handle_set_macro(connection, sender, object_path, interface_name, method_name, parameters, invocation, user_data);
    } else if (g_strcmp0(method_name, "RunMacro") == 0) {
        on_handle_run_macro(connection, sender, object_path, interface_name, method_name, parameters, invocation, user_data);
    } else if (g_strcmp0(method_name, "SaveConfig") == 0) {
        on_handle_save_config(connection, sender, object_path, interface_name, method_name, parameters, invocation, user_data);
    } else {
        g_dbus_method_invocation_return_error(invocation, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, "Unknown method: %s", method_name);
    }
}

// SetMode handler: expects (i) for mode (1,2,3,4)
static gboolean on_handle_set_mode(GDBusConnection *connection, const gchar *sender, const gchar *object_path,
                                   const gchar *interface_name, const gchar *method_name, GVariant *parameters,
                                   GDBusMethodInvocation *invocation, gpointer user_data) {
    int mode;
    g_variant_get(parameters, "(i)", &mode);
    if (mode < 1 || mode > 4) {
        g_dbus_method_invocation_return_error(invocation, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT, "Invalid mode: %d", mode);
        return TRUE;
    }
    g510s_data.mkey_state = mode;
    set_mkey_state(mode);
    // No more dbus_mode assignment
    emit_properties_changed(connection);
    refresh_gui();
    g_dbus_method_invocation_return_value(invocation, NULL);
    return TRUE;
}

// SetColor handler: expects (iii b) for red, green, blue, save (bool)
static gboolean on_handle_set_color(GDBusConnection *connection, const gchar *sender, const gchar *object_path,
                                    const gchar *interface_name, const gchar *method_name, GVariant *parameters,
                                    GDBusMethodInvocation *invocation, gpointer user_data) {
    int r, g, b;
    gboolean save;
    g_variant_get(parameters, "(iiib)", &r, &g, &b, &save);
    if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
        g_dbus_method_invocation_return_error(invocation, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT, "Invalid color values");
        return TRUE;
    }
    setG510LEDColor(r, g, b);
    g510s_data.led_red = r;
    g510s_data.led_green = g;
    g510s_data.led_blue = b;
    if (save) {
        // Save to current mode
        if (g510s_data.mkey_state == 1) {
            g510s_data.m1.red = r;
            g510s_data.m1.green = g;
            g510s_data.m1.blue = b;
        } else if (g510s_data.mkey_state == 2) {
            g510s_data.m2.red = r;
            g510s_data.m2.green = g;
            g510s_data.m2.blue = b;
        } else if (g510s_data.mkey_state == 3) {
            g510s_data.m3.red = r;
            g510s_data.m3.green = g;
            g510s_data.m3.blue = b;
        } else if (g510s_data.mkey_state == 4) {
            g510s_data.mr.red = r;
            g510s_data.mr.green = g;
            g510s_data.mr.blue = b;
        }
        save_config();
    }
    emit_properties_changed(connection);
    refresh_gui();
    g_dbus_method_invocation_return_value(invocation, NULL);
    return TRUE;
}

// SetColorFade handler: expects (b)
static gboolean on_handle_set_colorfade(GDBusConnection *connection, const gchar *sender, const gchar *object_path,
                                        const gchar *interface_name, const gchar *method_name, GVariant *parameters,
                                        GDBusMethodInvocation *invocation, gpointer user_data) {
    gboolean fade;
    g_variant_get(parameters, "(b)", &fade);
    g510s_data.color_fade = fade;
    save_config();
    emit_properties_changed(connection);
    refresh_gui();
    g_dbus_method_invocation_return_value(invocation, NULL);
    return TRUE;
}

// SetAutoSaveOnQuit handler: expects (b)
static gboolean on_handle_set_autosave(GDBusConnection *connection, const gchar *sender, const gchar *object_path,
                                        const gchar *interface_name, const gchar *method_name, GVariant *parameters,
                                        GDBusMethodInvocation *invocation, gpointer user_data) {
    gboolean autosave;
    g_variant_get(parameters, "(b)", &autosave);
    g510s_data.auto_save_on_quit = autosave;
    save_config();
    emit_properties_changed(connection);
    refresh_gui();
    g_dbus_method_invocation_return_value(invocation, NULL);
    return TRUE;
}

// SetGuiHidden handler: expects (b)
static gboolean on_handle_set_guihidden(GDBusConnection *connection, const gchar *sender, const gchar *object_path,
                                        const gchar *interface_name, const gchar *method_name, GVariant *parameters,
                                        GDBusMethodInvocation *invocation, gpointer user_data) {
    gboolean hidden;
    g_variant_get(parameters, "(b)", &hidden);
    g510s_data.gui_hidden = hidden;
    save_config();
    emit_properties_changed(connection);
    refresh_gui();
    g_dbus_method_invocation_return_value(invocation, NULL);
    return TRUE;
}

// Implementation for on_handle_save_config
static gboolean on_handle_save_config(GDBusConnection *connection, const gchar *sender, const gchar *object_path,
                                      const gchar *interface_name, const gchar *method_name, GVariant *parameters,
                                      GDBusMethodInvocation *invocation, gpointer user_data) {
    save_config();
    emit_properties_changed(connection);
    refresh_gui();
    g_dbus_method_invocation_return_value(invocation, NULL);
    return TRUE;
}

// Macro helpers
static char* get_macro_by_index(int mode, int idx) {
    // Helper for macro string access
    switch (mode) {
        case 1:
            switch (idx) {
                case 1: return g510s_data.m1.g1;
                case 2: return g510s_data.m1.g2;
                case 3: return g510s_data.m1.g3;
                case 4: return g510s_data.m1.g4;
                case 5: return g510s_data.m1.g5;
                case 6: return g510s_data.m1.g6;
                case 7: return g510s_data.m1.g7;
                case 8: return g510s_data.m1.g8;
                case 9: return g510s_data.m1.g9;
                case 10: return g510s_data.m1.g10;
                case 11: return g510s_data.m1.g11;
                case 12: return g510s_data.m1.g12;
                case 13: return g510s_data.m1.g13;
                case 14: return g510s_data.m1.g14;
                case 15: return g510s_data.m1.g15;
                case 16: return g510s_data.m1.g16;
                case 17: return g510s_data.m1.g17;
                case 18: return g510s_data.m1.g18;
                default: return NULL;
            }
        case 2:
            switch (idx) {
                case 1: return g510s_data.m2.g1;
                case 2: return g510s_data.m2.g2;
                case 3: return g510s_data.m2.g3;
                case 4: return g510s_data.m2.g4;
                case 5: return g510s_data.m2.g5;
                case 6: return g510s_data.m2.g6;
                case 7: return g510s_data.m2.g7;
                case 8: return g510s_data.m2.g8;
                case 9: return g510s_data.m2.g9;
                case 10: return g510s_data.m2.g10;
                case 11: return g510s_data.m2.g11;
                case 12: return g510s_data.m2.g12;
                case 13: return g510s_data.m2.g13;
                case 14: return g510s_data.m2.g14;
                case 15: return g510s_data.m2.g15;
                case 16: return g510s_data.m2.g16;
                case 17: return g510s_data.m2.g17;
                case 18: return g510s_data.m2.g18;
                default: return NULL;
            }
        case 3:
            switch (idx) {
                case 1: return g510s_data.m3.g1;
                case 2: return g510s_data.m3.g2;
                case 3: return g510s_data.m3.g3;
                case 4: return g510s_data.m3.g4;
                case 5: return g510s_data.m3.g5;
                case 6: return g510s_data.m3.g6;
                case 7: return g510s_data.m3.g7;
                case 8: return g510s_data.m3.g8;
                case 9: return g510s_data.m3.g9;
                case 10: return g510s_data.m3.g10;
                case 11: return g510s_data.m3.g11;
                case 12: return g510s_data.m3.g12;
                case 13: return g510s_data.m3.g13;
                case 14: return g510s_data.m3.g14;
                case 15: return g510s_data.m3.g15;
                case 16: return g510s_data.m3.g16;
                case 17: return g510s_data.m3.g17;
                case 18: return g510s_data.m3.g18;
                default: return NULL;
            }
        case 4:
            switch (idx) {
                case 1: return g510s_data.mr.g1;
                case 2: return g510s_data.mr.g2;
                case 3: return g510s_data.mr.g3;
                case 4: return g510s_data.mr.g4;
                case 5: return g510s_data.mr.g5;
                case 6: return g510s_data.mr.g6;
                case 7: return g510s_data.mr.g7;
                case 8: return g510s_data.mr.g8;
                case 9: return g510s_data.mr.g9;
                case 10: return g510s_data.mr.g10;
                case 11: return g510s_data.mr.g11;
                case 12: return g510s_data.mr.g12;
                case 13: return g510s_data.mr.g13;
                case 14: return g510s_data.mr.g14;
                case 15: return g510s_data.mr.g15;
                case 16: return g510s_data.mr.g16;
                case 17: return g510s_data.mr.g17;
                case 18: return g510s_data.mr.g18;
                default: return NULL;
            }
        default: return NULL;
    }
}

// GetMacro handler: expects (ii) for mode, index
static gboolean on_handle_get_macro(GDBusConnection *connection, const gchar *sender, const gchar *object_path,
                                    const gchar *interface_name, const gchar *method_name, GVariant *parameters,
                                    GDBusMethodInvocation *invocation, gpointer user_data) {
    int mode, idx;
    g_variant_get(parameters, "(ii)", &mode, &idx);
    char *macro = get_macro_by_index(mode, idx);
    if (!macro) {
        g_dbus_method_invocation_return_error(invocation, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT, "Invalid mode or index");
        return TRUE;
    }
    g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", macro));
    return TRUE;
}
// SetMacro handler: expects (ii s) for mode, index, value
static gboolean on_handle_set_macro(GDBusConnection *connection, const gchar *sender, const gchar *object_path,
                                    const gchar *interface_name, const gchar *method_name, GVariant *parameters,
                                    GDBusMethodInvocation *invocation, gpointer user_data) {
    int mode, idx;
    const char *value;
    g_variant_get(parameters, "(iis)", &mode, &idx, &value);
    char *macro = get_macro_by_index(mode, idx);
    if (!macro) {
        g_dbus_method_invocation_return_error(invocation, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT, "Invalid mode or index");
        return TRUE;
    }
    strncpy(macro, value, 63);
    macro[63] = '\0';
    save_config();
    emit_properties_changed(connection);
    refresh_gui();
    g_dbus_method_invocation_return_value(invocation, NULL);
    return TRUE;
}
// RunMacro handler: expects (ii) for mode, index
static gboolean on_handle_run_macro(GDBusConnection *connection, const gchar *sender, const gchar *object_path,
                                    const gchar *interface_name, const gchar *method_name, GVariant *parameters,
                                    GDBusMethodInvocation *invocation, gpointer user_data) {
    int mode, idx;
    g_variant_get(parameters, "(ii)", &mode, &idx);
    char *macro = get_macro_by_index(mode, idx);
    if (!macro) {
        g_dbus_method_invocation_return_error(invocation, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT, "Invalid mode or index");
        return TRUE;
    }
    // Simulate macro run (replace with actual macro execution logic if needed)
    // run_macro(mode, idx); // If you have such a function
    system(macro);
    printf("RunMacro: mode=%d idx=%d macro=%s\n", mode, idx, macro);
    g_dbus_method_invocation_return_value(invocation, NULL);
    return TRUE;
}

// DBus registration callback
static void on_bus_acquired(GDBusConnection *connection, const gchar *name, gpointer user_data) {
    static GDBusNodeInfo *introspection_data = NULL;
    static const gchar introspection_xml[] =
        "<node>"
        "  <interface name='org.g510s.control'>"
        "    <method name='SetMode'>"
        "      <arg type='i' name='mode' direction='in'/>"
        "    </method>"
        "    <method name='SetColor'>"
        "      <arg type='i' name='red' direction='in'/>"
        "      <arg type='i' name='green' direction='in'/>"
        "      <arg type='i' name='blue' direction='in'/>"
        "      <arg type='b' name='save' direction='in'/>"
        "    </method>"
        "    <method name='SetColorFade'>"
        "      <arg type='b' name='fade' direction='in'/>"
        "    </method>"
        "    <method name='SetAutoSaveOnQuit'>"
        "      <arg type='b' name='autosave' direction='in'/>"
        "    </method>"
        "    <method name='SetGuiHidden'>"
        "      <arg type='b' name='hidden' direction='in'/>"
        "    </method>"
        "    <method name='GetMacro'>"
        "      <arg type='i' name='mode' direction='in'/>"
        "      <arg type='i' name='index' direction='in'/>"
        "      <arg type='s' name='macro' direction='out'/>"
        "    </method>"
        "    <method name='SetMacro'>"
        "      <arg type='i' name='mode' direction='in'/>"
        "      <arg type='i' name='index' direction='in'/>"
        "      <arg type='s' name='macro' direction='in'/>"
        "    </method>"
        "    <method name='RunMacro'>"
        "      <arg type='i' name='mode' direction='in'/>"
        "      <arg type='i' name='index' direction='in'/>"
        "    </method>"
        "    <method name='SaveConfig'/>"
        "    <property name='Mode' type='i' access='readwrite'/>"
        "    <property name='Color' type='ai' access='readwrite'/>"
        "    <property name='ColorFade' type='b' access='readwrite'/>"
        "    <property name='AutoSaveOnQuit' type='b' access='readwrite'/>"
        "    <property name='GuiHidden' type='b' access='readwrite'/>"
        "  </interface>"
        "</node>";

    if (!introspection_data)
        introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, NULL);

    guint registration_id = g_dbus_connection_register_object(
        connection,
        G510S_DBUS_PATH,
        introspection_data->interfaces[0],
        &(GDBusInterfaceVTable){
            .method_call = on_method_call,
            .get_property = on_get_property,
            .set_property = on_set_property
        },
        NULL, NULL, NULL);

    if (registration_id == 0) {
        g_warning("Failed to register DBus object");
    } //else {
    //     g_message("DBus object registered at %s with id %u", G510S_DBUS_PATH, registration_id);
    //     g_message("Introspection XML:\n%s", introspection_xml);
    //     GError *flush_error = NULL;
    //     if (!g_dbus_connection_flush_sync(connection, NULL, &flush_error)) {
    //         g_warning("DBus connection flush failed: %s", flush_error ? flush_error->message : "unknown error");
    //         if (flush_error) g_error_free(flush_error);
    //     } else {
    //         g_message("DBus connection flush successful");
    //     }
    // }
}

// Move widget pointers to file scope for global access
GtkRange *redscale_m1;
GtkRange *greenscale_m1;
GtkRange *bluescale_m1;
GtkRange *redscale_m2;
GtkRange *greenscale_m2;
GtkRange *bluescale_m2;
GtkRange *redscale_m3;
GtkRange *greenscale_m3;
GtkRange *bluescale_m3;
GtkRange *redscale_mr;
GtkRange *greenscale_mr;
GtkRange *bluescale_mr;
GtkEntry *entry_m1g1, *entry_m1g2, *entry_m1g3, *entry_m1g4, *entry_m1g5, *entry_m1g6, *entry_m1g7, *entry_m1g8, *entry_m1g9, *entry_m1g10, *entry_m1g11, *entry_m1g12, *entry_m1g13, *entry_m1g14, *entry_m1g15, *entry_m1g16, *entry_m1g17, *entry_m1g18;
GtkEntry *entry_m2g1, *entry_m2g2, *entry_m2g3, *entry_m2g4, *entry_m2g5, *entry_m2g6, *entry_m2g7, *entry_m2g8, *entry_m2g9, *entry_m2g10, *entry_m2g11, *entry_m2g12, *entry_m2g13, *entry_m2g14, *entry_m2g15, *entry_m2g16, *entry_m2g17, *entry_m2g18;
GtkEntry *entry_m3g1, *entry_m3g2, *entry_m3g3, *entry_m3g4, *entry_m3g5, *entry_m3g6, *entry_m3g7, *entry_m3g8, *entry_m3g9, *entry_m3g10, *entry_m3g11, *entry_m3g12, *entry_m3g13, *entry_m3g14, *entry_m3g15, *entry_m3g16, *entry_m3g17, *entry_m3g18;
GtkEntry *entry_mrg1, *entry_mrg2, *entry_mrg3, *entry_mrg4, *entry_mrg5, *entry_mrg6, *entry_mrg7, *entry_mrg8, *entry_mrg9, *entry_mrg10, *entry_mrg11, *entry_mrg12, *entry_mrg13, *entry_mrg14, *entry_mrg15, *entry_mrg16, *entry_mrg17, *entry_mrg18;
GtkCheckMenuItem *menuhidden;
GtkCheckMenuItem *menuautosave;
GtkCheckMenuItem *menucolorfade;
AppIndicator *indicator;

void on_menucolorfade_toggled(GtkCheckMenuItem *menuitem, gpointer user_data) {
    g510s_data.color_fade = gtk_check_menu_item_get_active(menuitem) ? 1 : 0;
    save_config();
}

// --- GUI refresh implementation ---
static void refresh_gui_internal() {
    // Update color scales
    gtk_range_set_value(redscale_m1, g510s_data.m1.red);
    gtk_range_set_value(greenscale_m1, g510s_data.m1.green);
    gtk_range_set_value(bluescale_m1, g510s_data.m1.blue);
    gtk_range_set_value(redscale_m2, g510s_data.m2.red);
    gtk_range_set_value(greenscale_m2, g510s_data.m2.green);
    gtk_range_set_value(bluescale_m2, g510s_data.m2.blue);
    gtk_range_set_value(redscale_m3, g510s_data.m3.red);
    gtk_range_set_value(greenscale_m3, g510s_data.m3.green);
    gtk_range_set_value(bluescale_m3, g510s_data.m3.blue);
    gtk_range_set_value(redscale_mr, g510s_data.mr.red);
    gtk_range_set_value(greenscale_mr, g510s_data.mr.green);
    gtk_range_set_value(bluescale_mr, g510s_data.mr.blue);
    // Update macro entries
    gtk_entry_set_text(entry_m1g1, g510s_data.m1.g1);
    gtk_entry_set_text(entry_m1g2, g510s_data.m1.g2);
    gtk_entry_set_text(entry_m1g3, g510s_data.m1.g3);
    gtk_entry_set_text(entry_m1g4, g510s_data.m1.g4);
    gtk_entry_set_text(entry_m1g5, g510s_data.m1.g5);
    gtk_entry_set_text(entry_m1g6, g510s_data.m1.g6);
    gtk_entry_set_text(entry_m1g7, g510s_data.m1.g7);
    gtk_entry_set_text(entry_m1g8, g510s_data.m1.g8);
    gtk_entry_set_text(entry_m1g9, g510s_data.m1.g9);
    gtk_entry_set_text(entry_m1g10, g510s_data.m1.g10);
    gtk_entry_set_text(entry_m1g11, g510s_data.m1.g11);
    gtk_entry_set_text(entry_m1g12, g510s_data.m1.g12);
    gtk_entry_set_text(entry_m1g13, g510s_data.m1.g13);
    gtk_entry_set_text(entry_m1g14, g510s_data.m1.g14);
    gtk_entry_set_text(entry_m1g15, g510s_data.m1.g15);
    gtk_entry_set_text(entry_m1g16, g510s_data.m1.g16);
    gtk_entry_set_text(entry_m1g17, g510s_data.m1.g17);
    gtk_entry_set_text(entry_m1g18, g510s_data.m1.g18);
    gtk_entry_set_text(entry_m2g1, g510s_data.m2.g1);
    gtk_entry_set_text(entry_m2g2, g510s_data.m2.g2);
    gtk_entry_set_text(entry_m2g3, g510s_data.m2.g3);
    gtk_entry_set_text(entry_m2g4, g510s_data.m2.g4);
    gtk_entry_set_text(entry_m2g5, g510s_data.m2.g5);
    gtk_entry_set_text(entry_m2g6, g510s_data.m2.g6);
    gtk_entry_set_text(entry_m2g7, g510s_data.m2.g7);
    gtk_entry_set_text(entry_m2g8, g510s_data.m2.g8);
    gtk_entry_set_text(entry_m2g9, g510s_data.m2.g9);
    gtk_entry_set_text(entry_m2g10, g510s_data.m2.g10);
    gtk_entry_set_text(entry_m2g11, g510s_data.m2.g11);
    gtk_entry_set_text(entry_m2g12, g510s_data.m2.g12);
    gtk_entry_set_text(entry_m2g13, g510s_data.m2.g13);
    gtk_entry_set_text(entry_m2g14, g510s_data.m2.g14);
    gtk_entry_set_text(entry_m2g15, g510s_data.m2.g15);
    gtk_entry_set_text(entry_m2g16, g510s_data.m2.g16);
    gtk_entry_set_text(entry_m2g17, g510s_data.m2.g17);
    gtk_entry_set_text(entry_m2g18, g510s_data.m2.g18);
    gtk_entry_set_text(entry_m3g1, g510s_data.m3.g1);
    gtk_entry_set_text(entry_m3g2, g510s_data.m3.g2);
    gtk_entry_set_text(entry_m3g3, g510s_data.m3.g3);
    gtk_entry_set_text(entry_m3g4, g510s_data.m3.g4);
    gtk_entry_set_text(entry_m3g5, g510s_data.m3.g5);
    gtk_entry_set_text(entry_m3g6, g510s_data.m3.g6);
    gtk_entry_set_text(entry_m3g7, g510s_data.m3.g7);
    gtk_entry_set_text(entry_m3g8, g510s_data.m3.g8);
    gtk_entry_set_text(entry_m3g9, g510s_data.m3.g9);
    gtk_entry_set_text(entry_m3g10, g510s_data.m3.g10);
    gtk_entry_set_text(entry_m3g11, g510s_data.m3.g11);
    gtk_entry_set_text(entry_m3g12, g510s_data.m3.g12);
    gtk_entry_set_text(entry_m3g13, g510s_data.m3.g13);
    gtk_entry_set_text(entry_m3g14, g510s_data.m3.g14);
    gtk_entry_set_text(entry_m3g15, g510s_data.m3.g15);
    gtk_entry_set_text(entry_m3g16, g510s_data.m3.g16);
    gtk_entry_set_text(entry_m3g17, g510s_data.m3.g17);
    gtk_entry_set_text(entry_m3g18, g510s_data.m3.g18);
    gtk_entry_set_text(entry_mrg1, g510s_data.mr.g1);
    gtk_entry_set_text(entry_mrg2, g510s_data.mr.g2);
    gtk_entry_set_text(entry_mrg3, g510s_data.mr.g3);
    gtk_entry_set_text(entry_mrg4, g510s_data.mr.g4);
    gtk_entry_set_text(entry_mrg5, g510s_data.mr.g5);
    gtk_entry_set_text(entry_mrg6, g510s_data.mr.g6);
    gtk_entry_set_text(entry_mrg7, g510s_data.mr.g7);
    gtk_entry_set_text(entry_mrg8, g510s_data.mr.g8);
    gtk_entry_set_text(entry_mrg9, g510s_data.mr.g9);
    gtk_entry_set_text(entry_mrg10, g510s_data.mr.g10);
    gtk_entry_set_text(entry_mrg11, g510s_data.mr.g11);
    gtk_entry_set_text(entry_mrg12, g510s_data.mr.g12);
    gtk_entry_set_text(entry_mrg13, g510s_data.mr.g13);
    gtk_entry_set_text(entry_mrg14, g510s_data.mr.g14);
    gtk_entry_set_text(entry_mrg15, g510s_data.mr.g15);
    gtk_entry_set_text(entry_mrg16, g510s_data.mr.g16);
    gtk_entry_set_text(entry_mrg17, g510s_data.mr.g17);
    gtk_entry_set_text(entry_mrg18, g510s_data.mr.g18);
    // Update menu toggles
    gtk_check_menu_item_set_active(menuautosave, g510s_data.auto_save_on_quit ? TRUE : FALSE);
    gtk_check_menu_item_set_active(menucolorfade, g510s_data.color_fade ? TRUE : FALSE);
    gtk_check_menu_item_set_active(menuhidden, g510s_data.gui_hidden ? TRUE : FALSE);
}

int main(int argc, char *argv[]) {
  int i = 1;
  int help = 0;
  int debug = 0;
  int opt_invalid = 0;
  int dflag = 0;
  
  pthread_t key_thread;
  pthread_t update_thread;
  pthread_t server_thread;
  
  GtkBuilder *builder;
  GtkWidget *window;
  GtkAboutDialog *aboutdialog;
  GtkWidget *indicator_menu;
  
  leaving = 0;
  update = 0;
  device_found = 0;
  connected_clients = 0;
  current_key_state = 0;
  
  // parse command line options
  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i],"--help") || !strcmp(argv[i],"-h")) {
      help = 1;
      break;
    } else if (!strcmp(argv[i],"--debug") || !strcmp(argv[i],"-d")) {
      if (argv[i+1] && !is_number(argv[i+1])) {
        i++;
        debug = atoi(argv[i]);
        dflag++;
      } else {
        opt_invalid = 1;
        break;
      }
    } else {
      opt_invalid = 1;
      break;
    }
    if (dflag > 1) {
      opt_invalid = 1;
      break;
    }
  }
  
  if (opt_invalid) {
    printf("G510s: invalid option specified!\n\n");
    help = 1;
  }

  // print help and exit
  if (help) {
    printf("Usage: g510s <option> <value>\n\n");
    printf("Options:\n");
    printf("  --help|-h\tShow this help\n");
    printf("  --debug|-d\tSet debug level (default 0)\n");
    return 0;
  }

  // enable debug
  if (debug != 0) {
    libg15Debug(debug);
    printf("G510s: debugging enabled, level %i\n", debug);
  }
  
  // init libg15
  if (setupLibG15(0x46d, 0xc22d, 0) == G15_NO_ERROR) { // g510/g510s no audio
    printf("G510s: found device 046d:c22d\n");
    device_found = 1;
    usb_id="046d:c22d";
  } else if (setupLibG15(0x46d, 0xc22e, 0) == G15_NO_ERROR) { // g510/g510s with audio
    printf("G510s: found device 046d:c22e\n");
    device_found = 1;
    usb_id="046d:c22e";
  } else {
    printf("G510s: failed to initialize libg15\n");
    device_found = 0;
  }
  
  // init uinput only if a device is found
  if (device_found) {
    // media keys wont work without uinput
    if (init_uinput() != 0) {
      printf("G510s: failed to initialize uinput, media keys not available\n");
    }
  }
  
  // init data structure
  init_data();
  
  // init lcd list
  lcdlist_t *lcdlist = lcdlist_init();
  
  // try to create user save directory
  check_dir();
  
  // try to load previously saved data
  load_config();
  
  // start threads
  pthread_create(&server_thread, NULL, server_function, lcdlist);
  pthread_create(&update_thread, NULL, update_function, lcdlist);
  pthread_create(&key_thread, NULL, key_function, lcdlist);
  //pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  //pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  
  // init gtk
  gtk_init(&argc, &argv);
  
  builder = gtk_builder_new();
  // FIXME: hardcode to installed path
  gtk_builder_add_from_file(builder, "/usr/local/share/g510s/g510s.glade", NULL);
  
  window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
  aboutdialog = GTK_ABOUT_DIALOG(gtk_builder_get_object(builder, "aboutdialog"));
  
  // scales
  redscale_m1 = GTK_RANGE(gtk_builder_get_object(builder, "redscale_m1"));
  greenscale_m1 = GTK_RANGE(gtk_builder_get_object(builder, "greenscale_m1"));
  bluescale_m1 = GTK_RANGE(gtk_builder_get_object(builder, "bluescale_m1"));
  
  redscale_m2 = GTK_RANGE(gtk_builder_get_object(builder, "redscale_m2"));
  greenscale_m2 = GTK_RANGE(gtk_builder_get_object(builder, "greenscale_m2"));
  bluescale_m2 = GTK_RANGE(gtk_builder_get_object(builder, "bluescale_m2"));
  
  redscale_m3 = GTK_RANGE(gtk_builder_get_object(builder, "redscale_m3"));
  greenscale_m3 = GTK_RANGE(gtk_builder_get_object(builder, "greenscale_m3"));
  bluescale_m3 = GTK_RANGE(gtk_builder_get_object(builder, "bluescale_m3"));
  
  redscale_mr = GTK_RANGE(gtk_builder_get_object(builder, "redscale_mr"));
  greenscale_mr = GTK_RANGE(gtk_builder_get_object(builder, "greenscale_mr"));
  bluescale_mr = GTK_RANGE(gtk_builder_get_object(builder, "bluescale_mr"));
  
  // text entries
  entry_m1g1 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m1g1"));
  entry_m1g2 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m1g2"));
  entry_m1g3 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m1g3"));
  entry_m1g4 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m1g4"));
  entry_m1g5 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m1g5"));
  entry_m1g6 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m1g6"));
  entry_m1g7 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m1g7"));
  entry_m1g8 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m1g8"));
  entry_m1g9 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m1g9"));
  entry_m1g10 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m1g10"));
  entry_m1g11 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m1g11"));
  entry_m1g12 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m1g12"));
  entry_m1g13 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m1g13"));
  entry_m1g14 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m1g14"));
  entry_m1g15 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m1g15"));
  entry_m1g16 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m1g16"));
  entry_m1g17 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m1g17"));
  entry_m1g18 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m1g18"));
  
  entry_m2g1 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m2g1"));
  entry_m2g2 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m2g2"));
  entry_m2g3 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m2g3"));
  entry_m2g4 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m2g4"));
  entry_m2g5 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m2g5"));
  entry_m2g6 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m2g6"));
  entry_m2g7 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m2g7"));
  entry_m2g8 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m2g8"));
  entry_m2g9 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m2g9"));
  entry_m2g10 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m2g10"));
  entry_m2g11 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m2g11"));
  entry_m2g12 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m2g12"));
  entry_m2g13 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m2g13"));
  entry_m2g14 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m2g14"));
  entry_m2g15 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m2g15"));
  entry_m2g16 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m2g16"));
  entry_m2g17 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m2g17"));
  entry_m2g18 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m2g18"));
  
  entry_m3g1 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m3g1"));
  entry_m3g2 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m3g2"));
  entry_m3g3 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m3g3"));
  entry_m3g4 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m3g4"));
  entry_m3g5 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m3g5"));
  entry_m3g6 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m3g6"));
  entry_m3g7 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m3g7"));
  entry_m3g8 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m3g8"));
  entry_m3g9 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m3g9"));
  entry_m3g10 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m3g10"));
  entry_m3g11 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m3g11"));
  entry_m3g12 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m3g12"));
  entry_m3g13 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m3g13"));
  entry_m3g14 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m3g14"));
  entry_m3g15 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m3g15"));
  entry_m3g16 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m3g16"));
  entry_m3g17 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m3g17"));
  entry_m3g18 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_m3g18"));
  
  entry_mrg1 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_mrg1"));
  entry_mrg2 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_mrg2"));
  entry_mrg3 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_mrg3"));
  entry_mrg4 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_mrg4"));
  entry_mrg5 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_mrg5"));
  entry_mrg6 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_mrg6"));
  entry_mrg7 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_mrg7"));
  entry_mrg8 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_mrg8"));
  entry_mrg9 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_mrg9"));
  entry_mrg10 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_mrg10"));
  entry_mrg11 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_mrg11"));
  entry_mrg12 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_mrg12"));
  entry_mrg13 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_mrg13"));
  entry_mrg14 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_mrg14"));
  entry_mrg15 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_mrg15"));
  entry_mrg16 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_mrg16"));
  entry_mrg17 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_mrg17"));
  entry_mrg18 = GTK_ENTRY(gtk_builder_get_object(builder, "entry_mrg18"));
  
  menuhidden = GTK_CHECK_MENU_ITEM(gtk_builder_get_object(builder, "menuhidden"));
  menuautosave = GTK_CHECK_MENU_ITEM(gtk_builder_get_object(builder, "menuautosaveonquit"));
  menucolorfade = GTK_CHECK_MENU_ITEM(gtk_builder_get_object(builder, "menucolorfade"));
  // Set initial value from menu state
  
  // indicator
  indicator_menu = GTK_WIDGET(gtk_builder_get_object(builder, "indicator_menu"));
  // FIXME: hardcode to installed path
  indicator = app_indicator_new("G510s", "/usr/local/share/g510s/g510s.svg", APP_INDICATOR_CATEGORY_HARDWARE);
  // FIXME: hardcode to installed path
  app_indicator_set_attention_icon(indicator, "/usr/local/share/g510s/g510s-alert.svg");
  app_indicator_set_menu(indicator, GTK_MENU(indicator_menu));
  
  gtk_builder_connect_signals(builder, NULL);
  g_object_unref(G_OBJECT(builder));
  
  // set program version
  gtk_about_dialog_set_version(aboutdialog, G510S_VERSION);
  
  // set our range values
  gtk_range_set_value(redscale_m1, g510s_data.m1.red);
  gtk_range_set_value(greenscale_m1, g510s_data.m1.green);
  gtk_range_set_value(bluescale_m1, g510s_data.m1.blue);
  
  gtk_range_set_value(redscale_m2, g510s_data.m2.red);
  gtk_range_set_value(greenscale_m2, g510s_data.m2.green);
  gtk_range_set_value(bluescale_m2, g510s_data.m2.blue);
  
  gtk_range_set_value(redscale_m3, g510s_data.m3.red);
  gtk_range_set_value(greenscale_m3, g510s_data.m3.green);
  gtk_range_set_value(bluescale_m3, g510s_data.m3.blue);
  
  gtk_range_set_value(redscale_mr, g510s_data.mr.red);
  gtk_range_set_value(greenscale_mr, g510s_data.mr.green);
  gtk_range_set_value(bluescale_mr, g510s_data.mr.blue);
  
  // set our entry text
  gtk_entry_set_text(entry_m1g1, g510s_data.m1.g1);
  gtk_entry_set_text(entry_m1g2, g510s_data.m1.g2);
  gtk_entry_set_text(entry_m1g3, g510s_data.m1.g3);
  gtk_entry_set_text(entry_m1g4, g510s_data.m1.g4);
  gtk_entry_set_text(entry_m1g5, g510s_data.m1.g5);
  gtk_entry_set_text(entry_m1g6, g510s_data.m1.g6);
  gtk_entry_set_text(entry_m1g7, g510s_data.m1.g7);
  gtk_entry_set_text(entry_m1g8, g510s_data.m1.g8);
  gtk_entry_set_text(entry_m1g9, g510s_data.m1.g9);
  gtk_entry_set_text(entry_m1g10, g510s_data.m1.g10);
  gtk_entry_set_text(entry_m1g11, g510s_data.m1.g11);
  gtk_entry_set_text(entry_m1g12, g510s_data.m1.g12);
  gtk_entry_set_text(entry_m1g13, g510s_data.m1.g13);
  gtk_entry_set_text(entry_m1g14, g510s_data.m1.g14);
  gtk_entry_set_text(entry_m1g15, g510s_data.m1.g15);
  gtk_entry_set_text(entry_m1g16, g510s_data.m1.g16);
  gtk_entry_set_text(entry_m1g17, g510s_data.m1.g17);
  gtk_entry_set_text(entry_m1g18, g510s_data.m1.g18);
  
  gtk_entry_set_text(entry_m2g1, g510s_data.m2.g1);
  gtk_entry_set_text(entry_m2g2, g510s_data.m2.g2);
  gtk_entry_set_text(entry_m2g3, g510s_data.m2.g3);
  gtk_entry_set_text(entry_m2g4, g510s_data.m2.g4);
  gtk_entry_set_text(entry_m2g5, g510s_data.m2.g5);
  gtk_entry_set_text(entry_m2g6, g510s_data.m2.g6);
  gtk_entry_set_text(entry_m2g7, g510s_data.m2.g7);
  gtk_entry_set_text(entry_m2g8, g510s_data.m2.g8);
  gtk_entry_set_text(entry_m2g9, g510s_data.m2.g9);
  gtk_entry_set_text(entry_m2g10, g510s_data.m2.g10);
  gtk_entry_set_text(entry_m2g11, g510s_data.m2.g11);
  gtk_entry_set_text(entry_m2g12, g510s_data.m2.g12);
  gtk_entry_set_text(entry_m2g13, g510s_data.m2.g13);
  gtk_entry_set_text(entry_m2g14, g510s_data.m2.g14);
  gtk_entry_set_text(entry_m2g15, g510s_data.m2.g15);
  gtk_entry_set_text(entry_m2g16, g510s_data.m2.g16);
  gtk_entry_set_text(entry_m2g17, g510s_data.m2.g17);
  gtk_entry_set_text(entry_m2g18, g510s_data.m2.g18);
  
  gtk_entry_set_text(entry_m3g1, g510s_data.m3.g1);
  gtk_entry_set_text(entry_m3g2, g510s_data.m3.g2);
  gtk_entry_set_text(entry_m3g3, g510s_data.m3.g3);
  gtk_entry_set_text(entry_m3g4, g510s_data.m3.g4);
  gtk_entry_set_text(entry_m3g5, g510s_data.m3.g5);
  gtk_entry_set_text(entry_m3g6, g510s_data.m3.g6);
  gtk_entry_set_text(entry_m3g7, g510s_data.m3.g7);
  gtk_entry_set_text(entry_m3g8, g510s_data.m3.g8);
  gtk_entry_set_text(entry_m3g9, g510s_data.m3.g9);
  gtk_entry_set_text(entry_m3g10, g510s_data.m3.g10);
  gtk_entry_set_text(entry_m3g11, g510s_data.m3.g11);
  gtk_entry_set_text(entry_m3g12, g510s_data.m3.g12);
  gtk_entry_set_text(entry_m3g13, g510s_data.m3.g13);
  gtk_entry_set_text(entry_m3g14, g510s_data.m3.g14);
  gtk_entry_set_text(entry_m3g15, g510s_data.m3.g15);
  gtk_entry_set_text(entry_m3g16, g510s_data.m3.g16);
  gtk_entry_set_text(entry_m3g17, g510s_data.m3.g17);
  gtk_entry_set_text(entry_m3g18, g510s_data.m3.g18);
  
  gtk_entry_set_text(entry_mrg1, g510s_data.mr.g1);
  gtk_entry_set_text(entry_mrg2, g510s_data.mr.g2);
  gtk_entry_set_text(entry_mrg3, g510s_data.mr.g3);
  gtk_entry_set_text(entry_mrg4, g510s_data.mr.g4);
  gtk_entry_set_text(entry_mrg5, g510s_data.mr.g5);
  gtk_entry_set_text(entry_mrg6, g510s_data.mr.g6);
  gtk_entry_set_text(entry_mrg7, g510s_data.mr.g7);
  gtk_entry_set_text(entry_mrg8, g510s_data.mr.g8);
  gtk_entry_set_text(entry_mrg9, g510s_data.mr.g9);
  gtk_entry_set_text(entry_mrg10, g510s_data.mr.g10);
  gtk_entry_set_text(entry_mrg11, g510s_data.mr.g11);
  gtk_entry_set_text(entry_mrg12, g510s_data.mr.g12);
  gtk_entry_set_text(entry_mrg13, g510s_data.mr.g13);
  gtk_entry_set_text(entry_mrg14, g510s_data.mr.g14);
  gtk_entry_set_text(entry_mrg15, g510s_data.mr.g15);
  gtk_entry_set_text(entry_mrg16, g510s_data.mr.g16);
  gtk_entry_set_text(entry_mrg17, g510s_data.mr.g17);
  gtk_entry_set_text(entry_mrg18, g510s_data.mr.g18);
  
  // set whether we want to hide the window
  if (g510s_data.gui_hidden) {
    gtk_widget_hide(window);
    gtk_check_menu_item_set_active(menuhidden, TRUE);
  } else {
    gtk_widget_show(window);
    gtk_check_menu_item_set_active(menuhidden, FALSE);
  }

  // Set menuautosave toggle state based on loaded config
  if (g510s_data.auto_save_on_quit == 1) {
      gtk_check_menu_item_set_active(menuautosave, TRUE);
  } else {
      gtk_check_menu_item_set_active(menuautosave, FALSE);
  }
  
  // Set menucolorfade toggle state based on loaded config
  if (g510s_data.color_fade == 1) {
      gtk_check_menu_item_set_active(menucolorfade, TRUE);
  } else {
      gtk_check_menu_item_set_active(menucolorfade, FALSE);
  }
  
  // now we're ready to update the keyboard
  if (device_found) {
    set_mkey_state(g510s_data.mkey_state);
    app_indicator_set_status(indicator, APP_INDICATOR_STATUS_ACTIVE);
  } else {
    app_indicator_set_status(indicator, APP_INDICATOR_STATUS_ATTENTION);
  }
  
  // gtk_check_menu_item_set_active(menuautosave, TRUE);
  // --- DBus setup ---
  guint owner_id = g_bus_own_name(
      G_BUS_TYPE_SESSION,
      G510S_DBUS_NAME,
      G_BUS_NAME_OWNER_FLAGS_NONE,
      on_bus_acquired,
      NULL,
      NULL,
      NULL,
      NULL
  );
  // --- end DBus setup ---

  gtk_main();
  
  // notify threads to exit
  leaving = 1;
  
  // key_thread needs to be canceled
  //pthread_cancel(key_thread);
  
  // save data before leaving
  if (menuautosave) {
      g510s_data.auto_save_on_quit = gtk_check_menu_item_get_active(menuautosave) ? 1 : 0;
  }
  if (g510s_data.auto_save_on_quit) {
      save_config();
  }

  // DBus cleanup
  g_bus_unown_name(owner_id);
  
  // close gracefully
  if (device_found) {
    // close uinput
    exit_uinput();
    // close libg15
    exitLibG15();
    // return keyboard to default
    if (usb_id) {
      char cmd[128];
      snprintf(cmd, sizeof(cmd), "usbreset %s", usb_id);
      system(cmd);
    }
  }
  
  // clean up lcdlist
  lcdlist_destroy(&lcdlist);
  
  return 0;
}
