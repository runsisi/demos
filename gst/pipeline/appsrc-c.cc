#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

static GMainLoop *loop;

static void
cb_need_data(GstAppSrc *appsrc,
    guint unused_size,
    gpointer user_data) {
    static gboolean white = FALSE;
    static GstClockTime timestamp = 0;
    GstBuffer *buffer;
    guint size;
    GstFlowReturn ret;

    size = 385 * 288 * 2;

    buffer = gst_buffer_new_allocate(NULL, size, NULL);

    /* this makes the image black/white */
    gst_buffer_memset(buffer, 0, white ? 0xff : 0x0, size);

    white = !white;

    GST_BUFFER_PTS (buffer) = timestamp;
    GST_BUFFER_DURATION (buffer) = gst_util_uint64_scale_int(1, GST_SECOND, 2);

    timestamp += GST_BUFFER_DURATION (buffer);

    // owns the buffer, do not unref
    ret = gst_app_src_push_buffer(appsrc, buffer);
    if (ret != GST_FLOW_OK) {
        /* something wrong, stop pushing */
        g_main_loop_quit(loop);
    }
}

gboolean bus_callback(GstBus *bus, GstMessage *msg, gpointer data) {
    GMainLoop *loop = (GMainLoop *)data;

    switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_EOS:
        g_print("End of stream\n");
        g_main_loop_quit(loop);
        break;

    case GST_MESSAGE_ERROR: {
        gchar *debug;
        GError *error;

        gst_message_parse_error(msg, &error, &debug);
        g_free(debug);

        g_printerr("Error: %s\n", error->message);
        g_error_free(error);

        g_main_loop_quit(loop);
        break;
    }
    default:
        g_print("misc message: %d!\n", GST_MESSAGE_TYPE (msg));
        break;
    }

    // means G_SOURCE_CONTINUE
    return TRUE;
}

gint
main(gint argc, gchar *argv[]) {
    GstElement *pipeline, *appsrc, *conv, *videosink;

    /* init GStreamer */
    gst_init(&argc, &argv);

    loop = g_main_loop_new(NULL, FALSE);

    /* setup pipeline */
    pipeline = gst_pipeline_new("pipeline");
    appsrc = gst_element_factory_make("appsrc", "source");
    conv = gst_element_factory_make("videoconvert", "conv");
    videosink = gst_element_factory_make("xvimagesink", "videosink");

    GstBus *bus = gst_element_get_bus(GST_ELEMENT(pipeline));
    gst_bus_add_watch(bus, (GstBusFunc)bus_callback, loop);
    gst_object_unref(bus);

    /* setup */
    GstCaps *caps = gst_caps_new_simple("video/x-raw",
    "format", G_TYPE_STRING, "RGB16",
        "width", G_TYPE_INT, 384,
        "height", G_TYPE_INT, 288,
        "framerate", GST_TYPE_FRACTION, 0, 1,
        NULL);
    gst_app_src_set_caps(GST_APP_SRC(appsrc), caps);
    gst_object_unref(caps);

    gst_bin_add_many(GST_BIN (pipeline), appsrc, conv, videosink, NULL);
    gst_element_link_many(appsrc, conv, videosink, NULL);

    /* setup appsrc */
    gst_app_src_set_stream_type(GST_APP_SRC(appsrc), GST_APP_STREAM_TYPE_STREAM);
    gst_base_src_set_format(GST_BASE_SRC(appsrc), GST_FORMAT_TIME);

    GstAppSrcCallbacks callbacks;
    callbacks.need_data = cb_need_data;
    gst_app_src_set_callbacks(GST_APP_SRC(appsrc), &callbacks, NULL, NULL);

    /* play */
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    g_main_loop_run(loop);

    /* clean up */
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT (pipeline));
    g_main_loop_unref(loop);

    gst_deinit();

    return 0;
}
