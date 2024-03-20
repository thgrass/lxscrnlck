/*
  Compile with:
gcc `pkg-config --cflags gtk+-3.0 lxpanel cairo` -shared -fPIC lxscrnlck.c -o lxscrnlck.so `pkg-config --libs gtk+-3.0 lxpanel cairo` -lXext -lX11

  Install with:
Move the compiled .so file to the LXPanel plugins directory, typically /usr/lib/arm-linux-gnueabihf/lxpanel/plugins/ or /usr/lib/aarch64-linux-gnu/lxpanel/plugins/, but the exact path may vary.
Restart LXPanel with lxpanelctl restart.
Add the plugin to the panel using the LXPanel configuration UI.
*/

#include <lxpanel/plugin.h>
#include <gtk/gtk.h>
#include <X11/Xlib.h>
#include <X11/extensions/dpms.h>
#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

static Display *disp;

// Toggle DPMS/screen blanking state
static void toggle_dpms(GtkWidget *widget, gpointer data) {
    BOOL dpmsEnabled;
    CARD16 powerLevel;
    DPMSInfo(disp, &powerLevel, &dpmsEnabled);

    if (dpmsEnabled) {
        DPMSDisable(disp);
    } else {
        DPMSEnable(disp);
    }

    XFlush(disp);
}

// Handler for the screen lock command
static void lock_screen(GtkWidget *widget, gpointer data) {
    system("dm-tool lock");
}

// Set DPMS state from menu selection
static void set_dpms_state(GtkWidget *widget, gpointer data) {
    if (GPOINTER_TO_INT(data)) {
        DPMSEnable(disp);
    } else {
        DPMSDisable(disp);
    }

    XFlush(disp);
}

// Create and show the right-click menu
static void show_menu(GtkWidget *widget, GdkEventButton *event) {
    GtkWidget *menu = gtk_menu_new();
    GtkWidget *enableItem = gtk_menu_item_new_with_label("Enable Screen Blanking");
    GtkWidget *disableItem = gtk_menu_item_new_with_label("Disable Screen Blanking");
    GtkWidget *lockItem = gtk_menu_item_new_with_label("Lock Screen Now");

    g_signal_connect(G_OBJECT(enableItem), "activate", G_CALLBACK(set_dpms_state), GINT_TO_POINTER(1));
    g_signal_connect(G_OBJECT(disableItem), "activate", G_CALLBACK(set_dpms_state), GINT_TO_POINTER(0));
    g_signal_connect(G_OBJECT(lockItem), "activate", G_CALLBACK(lock_screen), NULL);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), enableItem);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), disableItem);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), lockItem);

    gtk_widget_show_all(menu);

    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button, event->time);
}

// Plugin button clicked callback
static gboolean button_clicked(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
        show_menu(widget, event);
        return TRUE;
    } else if (event->type == GDK_BUTTON_PRESS && event->button == 1) {
        toggle_dpms(widget, NULL);
        return TRUE;
    }
    return FALSE;
}

// Plugin constructor
static GtkWidget *construct_plugin_widget(LXPanel *panel, config_setting_t *settings) {
    disp = XOpenDisplay(NULL);
    if (!disp) {
        g_warning("Unable to open display");
        return NULL;
    }

    GtkWidget *button = gtk_button_new_with_label("Screen Lock Control");
    g_signal_connect(G_OBJECT(button), "button-press-event", G_CALLBACK(button_clicked), NULL);

    gtk_container_add(GTK_CONTAINER(panel), button);
    gtk_widget_show_all(button);

    return button;
}

// Plugin initialization
FM_DEFINE_MODULE(lxpanel_gtk, screen_control)

// Module entry
void fm_module_init(LXPanel *panel, config_setting_t *settings) {
    lxpanel_plugin_add_common(panel, settings, "lxscrnlck", construct_plugin_widget);
}
