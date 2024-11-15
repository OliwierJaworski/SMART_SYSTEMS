#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>

static void on_pad_added(GstElement *src, GstPad *new_pad, GstElement *depay);
static gboolean on_bus_message(GstBus *bus, GstMessage *message, GMainLoop *loop);
static void start_rtsp_pipeline(const gchar *input_uri, const gchar *output_mount_point);

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    // Start multiple pipelines with different input and output URIs
    start_rtsp_pipeline("rtsp://192.168.0.145:8554/stream1", "/output1");
    start_rtsp_pipeline("rtsp://192.168.0.145:8553/stream2", "/output2");

    // Main loop for RTSP server
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

    return 0;
}

// Function to create and start an RTSP pipeline for each stream
static void start_rtsp_pipeline(const gchar *input_uri, const gchar *output_mount_point) {
    GstElement *pipeline, *source, *depay, *queue, *decoder, *convert, *encoder, *rtppay;
    GstRTSPServer *server = gst_rtsp_server_new();
    GstRTSPMediaFactory *factory = gst_rtsp_media_factory_new();

    // Create elements for RTSP pipeline
    source = gst_element_factory_make("rtspsrc", "source");
    depay = gst_element_factory_make("rtph264depay", "depay");
    queue = gst_element_factory_make("queue", "queue");
    decoder = gst_element_factory_make("nvv4l2decoder", "decoder");
    convert = gst_element_factory_make("nvvidconv", "convert");
    encoder = gst_element_factory_make("nvv4l2h264enc", "encoder");
    rtppay = gst_element_factory_make("rtph264pay", "rtppay");

    if (!source || !depay || !queue || !decoder || !convert || !encoder || !rtppay) {
        g_printerr("Element creation failed.\n");
        return;
    }

    // Set the properties of the RTSP source
    g_object_set(source, "location", input_uri, "timeout", (guint64)30 * GST_SECOND, 
                 "do-timestamp", TRUE, "latency", 2000, NULL);
    g_object_set(queue, "max-size-time", 10000000000L, "max-size-buffers", 4000, "max-size-bytes", 10000000, NULL);
    g_object_set(rtppay, "pt", 96, NULL);
    g_object_set(encoder, "bitrate", 2000000, "control-rate", 2, "iframeinterval", 30, NULL);

    // Create and configure pipeline
    pipeline = gst_pipeline_new(NULL);
    gst_bin_add_many(GST_BIN(pipeline), source, depay, queue, decoder, convert, encoder, rtppay, NULL);
    g_signal_connect(source, "pad-added", G_CALLBACK(on_pad_added), depay);

    if (!gst_element_link_many(depay, queue, decoder, convert, encoder, rtppay, NULL)) {
        g_printerr("Failed to link elements.\n");
        return;
    }

    // Set up error handling for the pipeline
    GstBus *bus = gst_element_get_bus(pipeline);
    gst_bus_add_watch(bus, (GstBusFunc)on_bus_message, NULL);
    gst_object_unref(bus);

    // Configure the media factory for RTSP output
    gchar *launch_string = g_strdup_printf(
        "( rtspsrc location=%s ! rtph264depay ! queue ! nvv4l2decoder ! nvvidconv ! nvv4l2h264enc bitrate=4000000 control-rate=1 preset-level=1 ! rtph264pay name=pay0 pt=96 )",
        input_uri);
    gst_rtsp_media_factory_set_launch(factory, launch_string);
    g_free(launch_string);

    // Set up RTSP server and mount points
    GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);
    gst_rtsp_mount_points_add_factory(mounts, output_mount_point, factory);
    gst_rtsp_server_attach(server, NULL);

    // Start playing the pipeline
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    g_print("RTSP server started at rtsp://127.0.0.1:8554%s\n", output_mount_point);
}

// Callback for dynamic pad-added signal
static void on_pad_added(GstElement *src, GstPad *new_pad, GstElement *depay) {
    GstPad *sink_pad = gst_element_get_static_pad(depay, "sink");
    if (gst_pad_is_linked(sink_pad)) {
        g_object_unref(sink_pad);
        return;
    }
    if (gst_pad_link(new_pad, sink_pad) != GST_PAD_LINK_OK) {
        g_printerr("Failed to link new pad to depay sink pad.\n");
    }
    g_object_unref(sink_pad);
}

// Error and EOS handling
static gboolean on_bus_message(GstBus *bus, GstMessage *message, GMainLoop *loop) {
    if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_ERROR) {
        GError *err;
        gchar *debug_info;
        gst_message_parse_error(message, &err, &debug_info);
        g_printerr("Error received: %s\n", err->message);
        g_error_free(err);
        g_free(debug_info);
        g_main_loop_quit(loop);
    } else if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_EOS) {
        g_main_loop_quit(loop);
    }
    return TRUE;
}

