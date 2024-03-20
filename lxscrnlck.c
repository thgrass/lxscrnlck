/*
  Compile with:
gcc `pkg-config --cflags gtk+-3.0 lxpanel cairo` -shared -fPIC lxscrnlck.c -o lxscrnlck.so `pkg-config --libs gtk+-3.0 lxpanel cairo` -lXext -lX11
*/

#include <lxpanel/plugin.h>
#include <gtk/gtk.h>
#include <X11/Xlib.h>
#include <X11/extensions/dpms.h>
#include <cairo.h>

static void update_icon(GtkWidget *button);

// Draw the icon with Cairo based on DPMS state
static void draw_icon(cairo_t *cr, gboolean dpmsEnabled) {
    cairo_set_line_width(cr, 2);
    if (dpmsEnabled) {
        // Drawing a simple "enabled" icon - a green circle
        cairo_set_source_rgb(cr, 0, 1, 0);
        cairo_arc(cr, 16, 16, 15, 0, 2 * G_PI);
        cairo_fill(cr);
    } else {
        // Drawing a simple "disabled" icon - a red circle
        cairo_set_source_rgb(cr, 1, 0, 0);
        cairo_arc(cr, 16, 16, 15, 0, 2 * G_PI);
        cairo_fill(cr);
    }
}

// Create an icon as a GdkPixbuf
static GdkPixbuf *create_icon(gboolean dpmsEnabled) {
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 32, 32);
    cairo_t *cr = cairo_create(surface);

    draw_icon(cr, dpmsEnabled);

    GdkPixbuf *pixbuf = gdk_pixbuf_get_from_surface(surface, 0, 0, 32, 32);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    return pixbuf;
}

// Update the icon on the button to reflect the current DPMS state
static void update_icon(GtkWidget *button) {
    Display *display = XOpenDisplay(NULL);
    if (!display) {
        g_warning("Unable to open display");
        return;
    }

    BOOL dpmsEnabled;
    CARD16 powerLevel;
    DPMSInfo(display, &powerLevel, &dpmsEnabled);

    GdkPixbuf *pixbuf = create_icon(dpmsEnabled);
    GtkWidget *image = gtk_image_new_from_pixbuf(pixbuf);
    gtk_button_set_image(GTK_BUTTON(button), image);

    g_object_unref(pixbuf); // Cleanup
    XCloseDisplay(display);
}

// Toggle DPMS/screen blanking state
static void toggle_dpms(GtkWidget *widget, gpointer data) {
    Display *display = XOpenDisplay(NULL);
    if (!display) {
        g_warning("Unable to open display");
        return;
    }

    BOOL dpmsEnabled;
    CARD16 powerLevel;
    DPMSInfo(display, &powerLevel, &dpmsEnabled);

    if (dpmsEnabled) {
        DPMSDisable(display);
    } else {
        DPMSEnable(display);
    }

    XFlush(display);
    XCloseDisplay(display);

    update_icon(widget); // Update the icon based on the new DPMS state
}

// Lock the screen immediately
static void lock_screen(GtkWidget *widget, gpointer data) {
    system("dm-tool lock");
}

// Set DPMS state from menu selection
static void set_dpms_state(GtkWidget *widget, gpointer data) {
    Display *display = XOpenDisplay(NULL);
    if (!display) {
        g_warning("Unable to open display");
        return;
    }

    if (GPOINTER_TO_INT(data)) {
        DPMSEnable(display);
    } else {
        DPMSDisable(display);
    }

    XFlush(display);
    XCloseDisplay(display);

    update_icon(GTK_WIDGET(gtk_widget_get_parent(gtk_widget_get_parent(widget))));
}

// Create and show the right-click menu
static void create_menu(GtkWidget *button) {
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
    gtk_menu_popup_at_widget(GTK_MENU(menu), button, GDK_GRAVITY_SOUTH, GDK_GRAVITY_NORTH, NULL);
}

// Handler for right-click events on the button
static gboolean button_event(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    if (event->type == GDK_BUTTON_PRESS && event->button == 3) { // Right-click
        create_menu(widget);
        return TRUE; // Event handled
    }
    return FALSE; // Event not handled, pass it on
}

// Constructor for the plugin, setting up the button and its signals
static GtkWidget *constructor(LXPanel *panel, config_setting_t *settings) {
    // Create a new button without any label
    GtkWidget *button = gtk_button_new();
    update_icon(button); // Set the initial icon based on the current DPMS state

    // Connect signals for left-click to toggle DPMS and right-click to show menu
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(toggle_dpms), NULL);
    g_signal_connect(G_OBJECT(button), "button-press-event", G_CALLBACK(button_event), NULL);

    // This makes the button as small as possible
    gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
    gtk_container_set_border_width(GTK_CONTAINER(button), 0);

    return button;
}

// Plugin descriptor
FM_DEFINE_MODULE(lxpanel_gtk, myplugin)

// Initialization function for the plugin
void fm_module_init(LXPanel *panel, const FmModuleInitData *init_data) {
    // Register the plugin, providing the name and the constructor function
    lxpanel_plugin_register(init_data->name, constructor, NULL);
}
