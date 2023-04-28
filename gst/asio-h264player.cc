#include <imgui.h>
#include <glib-2.0/glib.h>
#include <gst/gst.h>
#include <asio.hpp>

static void
on_pad_added(GstElement *element, GstPad *pad, gpointer data) {
    GstPad *sinkpad;
    GstElement *parser = (GstElement *) data;

    /* We can now link this pad with the decoder sink pad */
    g_print("Dynamic pad created, linking demux/parse\n");

    sinkpad = gst_element_get_static_pad(parser, "sink");

    gst_pad_link(pad, sinkpad);

    gst_object_unref(sinkpad);
}

void schedule_wait(asio::executor_work_guard<asio::io_context::executor_type> &wg,
    asio::posix::stream_descriptor &descriptor, GstBus *bus)
{
    descriptor.async_wait(asio::posix::stream_descriptor::wait_read,
        [&wg, &descriptor, bus](const asio::error_code &error) {
            GstMessage *msg = gst_bus_pop(bus);

            switch (GST_MESSAGE_TYPE (msg)) {
            case GST_MESSAGE_EOS:
                g_print("End of stream\n");
                wg.reset();
                break;

            case GST_MESSAGE_ERROR: {
                gchar *debug;
                GError *error;

                gst_message_parse_error(msg, &error, &debug);
                g_free(debug);

                g_printerr("Error: %s\n", error->message);
                g_error_free(error);

                wg.reset();
                break;
            }
            default:
                g_print("misc message: %d!\n", GST_MESSAGE_TYPE (msg));

                // reschedule wait
                schedule_wait(wg, descriptor, bus);
                break;
            }

            gst_message_unref (msg);
        }
    );
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    // GMainLoop *loop = g_main_loop_new(NULL, FALSE);

    asio::io_context ioctx;
    // Prevent io_context::run from returning
    // https://dens.website/tutorials/cpp-asio/work
    asio::executor_work_guard<asio::io_context::executor_type> wg = asio::make_work_guard(ioctx);

    GstElement *pipeline, *source, *sink, *demux, *parser, *decoder;
    pipeline = gst_pipeline_new("h264-player");
    source = gst_element_factory_make("filesrc", "file-source");
    demux = gst_element_factory_make("qtdemux", "demux");
    parser = gst_element_factory_make("h264parse", "h264-parser");
    decoder = gst_element_factory_make("openh264dec", "decoder");
    sink = gst_element_factory_make("autovideosink", "video-output");

    g_object_set(G_OBJECT (source), "location", argv[1], NULL);

    GstBus *bus;
    bus = gst_pipeline_get_bus(GST_PIPELINE (pipeline));

    gst_bin_add_many(GST_BIN (pipeline), source, demux, parser, decoder, sink, NULL);

    gst_element_link(source, demux);
    gst_element_link_many(parser, decoder, sink, NULL);
    g_signal_connect (demux, "pad-added", G_CALLBACK(on_pad_added), parser);

    /* note that the demuxer will be linked to the decoder dynamically.
     The reason is that Ogg may contain various streams (for example
     audio and video). The source pad(s) will be created at run time,
     by the demuxer when it detects the amount and nature of streams.
     Therefore we connect a callback function which will be executed
     when the "pad-added" is emitted.*/

    /* Set the pipeline to "playing" state*/
    g_print("Now playing: %s\n", argv[1]);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    /* Iterate */
    g_print("Running...\n");
    // g_main_loop_run(loop);

    GPollFD pfd;
    gst_bus_get_pollfd(bus, &pfd);

    asio::posix::stream_descriptor descriptor(ioctx, pfd.fd);
    schedule_wait(wg, descriptor, bus);

    ioctx.run();

    gst_object_unref(bus);

    /* Out of the main loop, clean up nicely */
    g_print("Returned, stopping playback\n");
    gst_element_set_state(pipeline, GST_STATE_NULL);

    g_print("Deleting pipeline\n");
    gst_object_unref(GST_OBJECT (pipeline));

    return 0;
}
