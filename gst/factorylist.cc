#include <string>

#include <glib-2.0/glib.h>
#include <gst/gst.h>

#define gst_syslog(priority, str, ...) printf("Gstreamer plugin: " str "\n", ## __VA_ARGS__)

const std::string encoder = "vaapih264enc";

int main() {
    gst_init(nullptr, nullptr);

    GstCaps *caps = gst_caps_new_simple("video/x-h264",
        "stream-format", G_TYPE_STRING, "byte-stream",
        nullptr);
    gst_caps_set_simple(caps, "framerate", GST_TYPE_FRACTION, 60, 1, nullptr);
    gchar *caps_str = gst_caps_to_string(caps);

    GList *encoders = gst_element_factory_list_get_elements(GST_ELEMENT_FACTORY_TYPE_VIDEO_ENCODER, GST_RANK_NONE);
    GList *filtered = gst_element_factory_list_filter(encoders, caps, GST_PAD_SRC, false);

    GstElementFactory *factory = nullptr;
    if (filtered) {
        gst_syslog(LOG_NOTICE, "Looking for encoder plugins which can produce a '%s' stream", caps_str);
        for (GList *l = filtered; l != nullptr; l = l->next) {
            if (!factory && !encoder.compare(GST_ELEMENT_NAME(l->data))) {
                factory = (GstElementFactory*)l->data;
            }
            gst_syslog(LOG_NOTICE, "'%s' plugin is available", GST_ELEMENT_NAME(l->data));
        }

        if (!factory && !encoder.empty()) {
            gst_syslog(LOG_WARNING,
                "Specified encoder named '%s' cannot produce '%s' streams. Make sure that "
                "gst.CODEC=ENCODER is correctly specified and that the encoder is available.",
                encoder.c_str(), caps_str);
        }

        factory = factory ? factory : (GstElementFactory*)filtered->data;
        gst_syslog(LOG_NOTICE, "'%s' encoder plugin is used", GST_ELEMENT_NAME(factory));
    }

    gst_plugin_feature_list_free(filtered);
    gst_plugin_feature_list_free(encoders);
    g_free(caps_str);
    gst_caps_unref(caps);

    return 0;
}