/*
  Compile with:
gcc `pkg-config --cflags gtk+-3.0 lxpanel` -shared -fPIC lxscrnlck.c -o lxscrnlck.so `pkg-config --libs gtk+-3.0 lxpanel` -lXext -lX11

  Install with:
Move the compiled .so file to the LXPanel plugins directory, typically /usr/lib/arm-linux-gnueabihf/lxpanel/plugins/ or /usr/lib/aarch64-linux-gnu/lxpanel/plugins/, but the exact path may vary.
Restart LXPanel with lxpanelctl restart.
Add the plugin to the panel using the LXPanel configuration UI.
*/

#include <lxpanel/plugin.h>
#include <gtk/gtk.h>
#include <X11/Xlib.h>
#include <X11/extensions/dpms.h>
#include <stdlib.h>

// Function declarations
static GtkWidget *create_plugin_widget(LXPanel *panel, config_setting_t *settings);
static void toggle_dpms(GtkWidget *widget, gpointer data);
static void show_menu(GtkWidget *widget, GdkEventButton *event);

// Define the plugin structure
PluginClass plugin_class = {
    .name = "Screen Lock & Blanking",
    .description = "Control screen blanking and lock the screen",
    .one_per_system = 1,
    .expand_available = 1,
    .new_instance = create_plugin_widget
};

// Plugin widget constructor
static GtkWidget *create_plugin_widget(LXPanel *panel, config_setting_t *settings) {
    // Create a button for the plugin
    GtkWidget *button = gtk_button_new_with_label("Screen Control");
    g_signal_connect(G_OBJECT(button), "button-press-event", G_CALLBACK(toggle_dpms), NULL);
    
    // Show the widget and return
    gtk_widget_show_all(button);
    return button;
}

// Toggle DPMS (screen blanking) state
static void toggle_dpms(GtkWidget *widget, gpointer data) {
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) {
        g_warning("Unable to open display");
        return;
    }

    BOOL dpmsEnabled;
    CARD16 powerLevel;
    DPMSInfo(dpy, &powerLevel, &dpmsEnabled);

    if (dpmsEnabled) {
        DPMSDisable(dpy);
    } else {
        DPMSEnable(dpy);
    }

    XFlush(dpy);
    XCloseDisplay(dpy);
}

// Show right-click menu
static void show_menu(GtkWidget *widget, GdkEventButton *event) {
    if (event->button == 3) { // Right-click
        GtkWidget *menu = gtk_menu_new();

        // Add menu items here
        // For example, to lock the screen:
        GtkWidget *lockScreenItem = gtk_menu_item_new_with_label("Lock Screen");
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), lockScreenItem);
        g_signal_connect(lockScreenItem, "activate", G_CALLBACK(system), "dm-tool lock");

        gtk_widget_show_all(menu);
        gtk_menu_popup_at_pointer(GTK_MENU(menu), (const GdkEvent *)event);
    }
}

// Initialization function, if needed, to perform any setup required for your plugin
// This function can be left empty if no initialization code is needed
void lxpanel_plugin_init(GModule *module) {}
