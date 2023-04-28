#include <imgui.h>
#include <glib-2.0/glib.h>
#include <gst/gst.h>
#include <uv.h>

void bus_call(uv_poll_t* handle, int status, int events)
{
    GstBus *bus = (GstBus *)handle->data;
    GstMessage *msg = gst_bus_pop(bus);

    switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_EOS:
        g_print("End of stream\n");
        uv_poll_stop(handle);
        break;

    case GST_MESSAGE_ERROR: {
        gchar *debug;
        GError *error;

        gst_message_parse_error(msg, &error, &debug);
        g_free(debug);

        g_printerr("Error: %s\n", error->message);
        g_error_free(error);

        uv_poll_stop(handle);
        break;
    }
    default:
        g_print("misc message: %d!\n", GST_MESSAGE_TYPE (msg));
        break;
    }

    gst_message_unref (msg);
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    uv_loop_t *loop = (uv_loop_t *)malloc(sizeof(uv_loop_t));
    uv_loop_init(loop);

    GstElement *pipeline, *source, *sink, *parser, *decoder;
    pipeline = gst_pipeline_new("h264-player");
    source = gst_element_factory_make("filesrc", "file-source");
    parser = gst_element_factory_make("h264parse", "h264-parser");
    decoder = gst_element_factory_make("openh264dec", "decoder");
    sink = gst_element_factory_make("autovideosink", "video-output");

    g_object_set(G_OBJECT (source), "location", argv[1], NULL);

    GstBus *bus;
    bus = gst_pipeline_get_bus(GST_PIPELINE (pipeline));

    gst_bin_add_many(GST_BIN (pipeline), source, parser, decoder, sink, NULL);

    // gst_element_link(source, parser);
    gst_element_link_many(source, parser, decoder, sink, NULL);
    // g_signal_connect (parser, "pad-added", G_CALLBACK(on_pad_added), decoder);

    /* note that the demuxer will be linked to the decoder dynamically.
     The reason is that Ogg may contain various streams (for example
     audio and video). The source pad(s) will be created at run time,
     by the demuxer when it detects the amount and nature of streams.
     Therefore we connect a callback function which will be executed
     when the "pad-added" is emitted.*/

    /* Set the pipeline to "playing" state*/
    g_print("Now playing: %s\n", argv[1]);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    GPollFD pfd;
    gst_bus_get_pollfd(bus, &pfd);

    uv_poll_t poll;
    uv_poll_init(loop, &poll, pfd.fd);
    poll.data = bus;
    uv_poll_start(&poll, UV_READABLE, (uv_poll_cb)bus_call);

    /* Iterate */
    g_print("Running...\n");
    uv_run(loop, UV_RUN_DEFAULT);

    gst_object_unref(bus);

    /* Out of the main loop, clean up nicely */
    g_print("Returned, stopping playback\n");
    gst_element_set_state(pipeline, GST_STATE_NULL);

    g_print("Deleting pipeline\n");
    gst_object_unref(GST_OBJECT (pipeline));

    uv_loop_close(loop);
    free(loop);

    return 0;
}
