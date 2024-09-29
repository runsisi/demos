#include <gtk/gtk.h>

#include <iostream>

using namespace std;

int main() {
    gtk_init(NULL, NULL);

    int n = gdk_screen_get_n_monitors(gdk_screen_get_default());

    cout << n << endl;

    for (int i = 0; i < n; i++) {
        GdkRectangle rect;
        // not the same as geometry!
        gdk_screen_get_monitor_workarea(gdk_screen_get_default(), i, &rect);

        cout << rect.width << "x" << rect.height << "+" << rect.x << "+" << rect.y << endl;

        // call gdk_monitor_get_geometry internally
        gdk_screen_get_monitor_geometry(gdk_screen_get_default(), i, &rect);

        cout << rect.width << "x" << rect.height << "+" << rect.x << "+" << rect.y << endl;
    }

    // requires gtk 3.22+
    for (int i = 0; i < n; i++) {
        GdkMonitor *m = gdk_display_get_monitor(gdk_display_get_default(), i);

        GdkRectangle geo;
        gdk_monitor_get_geometry(m, &geo);
        cout << geo.width << "x" << geo.height << "+" << geo.x << "+" << geo.y << endl;
    }

    return 0;
}
