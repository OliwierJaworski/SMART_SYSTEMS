#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <glib.h>

// Function to start the RTSP server
void start_rtsp_server() {
    GstRTSPServer *server;
    GstRTSPMountPoints *mounts;
    GstRTSPMediaFactory *factory;

    // Create a new RTSP server
    server = gst_rtsp_server_new();
    mounts = gst_rtsp_server_get_mount_points(server);
    factory = gst_rtsp_media_factory_new();

    // Set up the media factory to launch the desired pipeline
    // Ensure that the source URL is correctly specified
    gst_rtsp_media_factory_set_launch(factory,
        "( rtspsrc location=rtsp://192.168.0.145:8554/xx ! decodebin ! videoconvert ! x264enc tune=zerolatency ! rtph264pay name=pay0 pt=96 )");

    // Add the factory to the mount points at the desired endpoint
    gst_rtsp_mount_points_add_factory(mounts, "/test", factory);
    g_object_unref(mounts);

    // Attach the server to the default main context
    gst_rtsp_server_attach(server, NULL);
    g_print("RTSP Server is running at rtsp://localhost:8554/test\n");
}

int main(int argc, char *argv[]) {
    // Set GST_DEBUG environment variable for this process
    setenv("GST_DEBUG", "3", 1); // Adjust the level as needed (0-9)

    // Initialize GStreamer
    gst_init(&argc, &argv);

    // Start the RTSP server
    start_rtsp_server();

    // Run the main loop to keep the application alive
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

    // Clean up
    g_main_loop_unref(loop);

    return 0;
}
