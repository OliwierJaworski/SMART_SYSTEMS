#include <iostream>
#include <cstdlib>  // For setenv
#include "gstreamer-1.0/gst/gst.h"
#include "glib-2.0/glib.h"

int main(int argc, char *argv[]) {
    std::cout << "**The beginning**\n";

    /* Set GST_DEBUG environment variable for this process */
    setenv("GST_DEBUG", "3", 1);

    /* GStreamer type init */
    GstElement *pipeline;
    GstBus *bus;
    GstMessage *msg;

    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Build the pipeline */
    pipeline = gst_parse_launch(
        "rtspsrc location=rtsp://192.168.0.145:8554/xx ! "
        "decodebin ! "
        "x264enc tune=zerolatency ! "
        "mp4mux ! "
        "filesink location=output.mp4", 
        NULL);

    /* Start the pipeline */
    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "Unable to set the pipeline to the playing state." << std::endl;
        return -1;
    }

    /* Create a GMainLoop */
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);

    /* Add a timeout to quit the loop after 10 seconds */
    g_timeout_add_seconds(10, (GSourceFunc) g_main_loop_quit, loop);

    /* Wait for error or EOS */
    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, 
        static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    /* Free resources */
    if (msg != nullptr) {
        gst_message_unref(msg);
    }

    /* Clean up */
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    
    /* Run the main loop */
    g_main_loop_run(loop);
    
    /* Free the main loop */
    g_main_loop_unref(loop);

    return 0;
}
