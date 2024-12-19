﻿#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <iostream>
#include <string>
#include "YOLOv11.h"
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/gstbuffer.h> 
#include <gst/rtsp-server/rtsp-server.h>


#define MUXER_OUTPUT_WIDTH 640
#define MUXER_OUTPUT_HEIGHT 640
#define MUXER_BATCH_TIMEOUT_USEC 40000

/**
 * @brief Setting up Tensorrt logger
*/
class Logger : public nvinfer1::ILogger {
    void log(Severity severity, const char* msg) noexcept override {
        // Only output logs with severity greater than warning
        if (severity <= Severity::kWARNING)
            std::cout << msg << std::endl;
    }
}logger;



typedef struct RTSP_CustomData {
    GMainLoop *loop;
    GstRTSPMediaFactory *factory;
    GstRTSPServer *server;
    GstElement *transform;
    GstBus *bus;
    guint bus_watch_id;
    GstElement  *pipeline, 
                *source, //!<- input rtsp stream
                *depay, //!<- extract H264 from RTP packets
                *queue, //!<- queue the H264 packets 
                *decoder, //!<- converts compressed video into raw video frames = H264 -> video/x-raw(memory:NVMM)
                *streammux, //!<- multiplex video stream with the metadata streams generated by other elements in pipeline
                *pgie, //!<- element responsible for object detection | uses config file 
                *tracker, //!<- tracks objects detected by the nvinfer element | uses ll-lib-file attribute to specify library file for multi-object tracking
                *nvvidconv, //!<- converts convert video frames between different video format
                *nvosd, //!<- element for bounding boxes overlay and labels | based on metadata generated by nvtracker and nvinfer elements
                *nvvidconv2, //!<- Use convertor to convert from NV12 to RGBA
                *encoder, //!<- Use convertor to convert from NV12 to H264
                *rtppay; //!<- Payload-encode H264 video into RTP packets

}RTSP_ELEMENTS;


static gboolean on_bus_message(GstBus *bus, GstMessage *message, GMainLoop *loop); 
static GstPadProbeReturn event_probe_callback(GstPad *pad, GstPadProbeInfo *info, gpointer user_data);
static GstPadProbeReturn buffer_probe_callback(GstPad *pad, GstPadProbeInfo *info, gpointer user_data); 
static void on_pad_added(GstElement *src, GstPad *new_pad, GstElement *depay);

int main(int argc, char* argv[]) {

    // Define color codes for terminal output
    const std::string RED_COLOR = "\033[31m";
    const std::string GREEN_COLOR = "\033[32m";
    const std::string YELLOW_COLOR = "\033[33m";
    const std::string RESET_COLOR = "\033[0m";
    
    // Check for valid number of arguments
    if (argc < 4 || argc > 5) {
        std::cerr << RED_COLOR << "Usage: " << RESET_COLOR << argv[0]
            << " <mode> <input_path> <engine_path> [onnx_path]" << std::endl;
        std::cerr << YELLOW_COLOR << "  <mode> - Mode of operation: 'convert', 'infer_video', or 'infer_image'" << RESET_COLOR << std::endl;
        std::cerr << YELLOW_COLOR << "  <input_path> - Path to the input video/image or ONNX model" << RESET_COLOR << std::endl;
        std::cerr << YELLOW_COLOR << "  <engine_path> - Path to the TensorRT engine file" << RESET_COLOR << std::endl;
        std::cerr << YELLOW_COLOR << "  [onnx_path] - Path to the ONNX model (only for 'convert' mode)" << RESET_COLOR << std::endl;
        return 1;
    }

    // Parse command-line arguments
    std::string mode = argv[1];
    std::string inputPath = argv[2];
    std::string enginePath = argv[3];
    std::string onnxPath;

    // Validate mode and arguments
    if (mode == "convert") {
        if (argc != 5) {  // 'convert' requires onnx_path
            std::cerr << RED_COLOR << "Usage for conversion: " << RESET_COLOR << argv[0]
                << " convert <onnx_path> <engine_path>" << std::endl;
            return 1;
        }
        onnxPath = inputPath;  // In 'convert' mode, inputPath is actually onnx_path
    }
    else if (mode == "infer_video" || mode == "infer_image") {
        if (argc != 4) {
            std::cerr << RED_COLOR << "Usage for " << mode << ": " << RESET_COLOR << argv[0]
                << " " << mode << " <input_path> <engine_path>" << std::endl;
            return 1;
        }
    }
    else {
        std::cerr << RED_COLOR << "Invalid mode. Use 'convert', 'infer_video', or 'infer_image'." << RESET_COLOR << std::endl;
        return 1;
    }

    // Initialize the Logger
    Logger logger;

    // Handle 'convert' mode
    if (mode == "convert") {
        try {
            // Initialize YOLOv11 with the ONNX model path
            YOLOv11 yolov11(onnxPath, logger);
            std::cout << GREEN_COLOR << "Model conversion successful. Engine saved." << RESET_COLOR << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << RED_COLOR << "Error during model conversion: " << e.what() << RESET_COLOR << std::endl;
            return 1;
        }
    }
    // Handle inference modes
    else if (mode == "infer_video" || mode == "infer_image") {
        try {
            RTSP_ELEMENTS element_data;
            memset(&element_data, 0, sizeof (element_data)); //!<- SET THE memory so the optimizer doesn't fuck with it

            gst_init (&argc, &argv);
            //env setup
            element_data.loop = g_main_loop_new (NULL, FALSE);
            element_data.server = gst_rtsp_server_new();
            element_data.factory = gst_rtsp_media_factory_new();
            element_data.pipeline = gst_pipeline_new("rtsp-in-out-pipeline");
            //data receiving
            element_data.source = gst_element_factory_make("rtspsrc", "source");
            element_data.depay = gst_element_factory_make("rtph264depay", "depay");
            element_data.queue = gst_element_factory_make("queue", "queue"); 
            element_data.decoder = gst_element_factory_make("nvv4l2decoder", "decoder");
            //data inference
            element_data.streammux = gst_element_factory_make ("nvstreammux", "stream-muxer");
            element_data.pgie = gst_element_factory_make ("nvinfer", "primary-nvinference-engine");
            element_data.tracker = gst_element_factory_make ("nvtracker", "tracker");
            element_data.nvvidconv = gst_element_factory_make ("nvvideoconvert", "nvvideo-converter");
            element_data.nvosd = gst_element_factory_make ("nvdsosd", "nv-onscreendisplay");
            //data sending
            element_data.nvvidconv2 = gst_element_factory_make ("nvvideoconvert", "nvvideo-converter2");
            element_data.encoder = gst_element_factory_make ("nvv4l2h264enc", "nvv4l2h264enc");
            element_data.rtppay = gst_element_factory_make("rtph264pay", "rtppay");

            if( !element_data.pipeline || !element_data.source || !element_data.depay || !element_data.queue || !element_data.decoder || !element_data.streammux ||
                !element_data.pgie || !element_data.tracker || !element_data.nvvidconv || !element_data.nvosd || !element_data.nvvidconv2 || !element_data.encoder || !element_data.rtppay){
                    
                    g_printerr ("One element could not be created. Exiting.\n");
                    return -1;
                }
            g_object_set(G_OBJECT (element_data.source), 
                "location", "rtsp://192.168.0.145:8554/xx", 
                "timeout", (guint64)30 * GST_SECOND, 
                "latency", 5000, 
                NULL);
            g_signal_connect(element_data.source, "pad-added", G_CALLBACK(on_pad_added), element_data.depay);

            g_object_set(G_OBJECT (element_data.queue), 
                "max-size-time", 10000000000L, 
                "max-size-buffers", 4000, 
                "max-size-bytes", 10000000,
                NULL);

            g_object_set(G_OBJECT (element_data.decoder), 
                "skip-frames", 0,
                //"drop-frame-interval", 5,
                "disable-dpb", TRUE,
                "enable-full-frame", TRUE,
                "enable-max-performance", TRUE,
                NULL);

            g_object_set (G_OBJECT (element_data.streammux), 
                "batch-size", 1, 
                "live-source",1,
                "width", MUXER_OUTPUT_WIDTH, 
                "height", MUXER_OUTPUT_HEIGHT,
                "batched-push-timeout", MUXER_BATCH_TIMEOUT_USEC, 
                NULL);

            g_object_set (G_OBJECT (element_data.pgie),
                "config-file-path", "/opt/nvidia/deepstream/deepstream/sources/apps/sample_apps/deepstream-test1/dstest1_pgie_config.txt", 
                NULL);
            
            g_object_set (G_OBJECT (element_data.tracker),
                "ll-lib-file", "/opt/nvidia/deepstream/deepstream/lib/libnvds_nvmultiobjecttracker.so", 
                NULL);
            
            g_object_set(G_OBJECT(element_data.encoder),
                "bitrate", 4000000, 
                "iframeinterval", 30, 
                "preset-level", 1,
                "maxperf-enable", true,
                NULL);

            g_object_set(G_OBJECT(element_data.rtppay), 
                "pt", 96, 
                NULL);

            element_data.bus = gst_pipeline_get_bus(GST_PIPELINE(element_data.pipeline));
            element_data.bus_watch_id = gst_bus_add_watch(element_data.bus, GstBusFunc(on_bus_message), element_data.loop); 
            gst_object_unref(element_data.bus);   

            gst_bin_add_many( 
                GST_BIN(element_data.pipeline),
                element_data.source,
                element_data.depay,
                element_data.queue,
                element_data.decoder,
                element_data.streammux,
                element_data.pgie,
                element_data.tracker,
                element_data.nvvidconv,
                element_data.nvosd,
                element_data.nvvidconv2,
                element_data.encoder,
                element_data.rtppay,
                NULL
                );

            //decoder pad | creation | linking | to streammux
            GstPad *sinkpad, *srcpad;
            gchar pad_name_sink[16] = "sink_0";
            gchar pad_name_src[16] = "src";

            /* Dynamically created pad */
            sinkpad = gst_element_get_request_pad (element_data.streammux, pad_name_sink);
            if (!sinkpad) {
                g_printerr ("Streammux request sink pad failed. Exiting.\n");
                return -1;
            }

            /* Statically created pad */
            srcpad = gst_element_get_static_pad (element_data.decoder, pad_name_src);
            if (!srcpad) {
                g_printerr ("Decoder request src pad failed. Exiting.\n");
                return -1;
            }

            /* Linking the pads */
            if (gst_pad_link (srcpad, sinkpad) != GST_PAD_LINK_OK) {
                g_printerr ("Failed to link decoder to stream muxer. Exiting.\n");
                return -1;
            }

            gst_object_unref (sinkpad);
            gst_object_unref (srcpad);
            
            if (!gst_element_link_many (element_data.depay, 
                                        element_data.queue, 
                                        element_data.decoder, 
                                        NULL)){
                g_printerr( "first linking session | error | could not be linked: 1. Exiting.\n");
                return -1;
            }
            if (!gst_element_link_many (element_data.streammux, 
                                        element_data.pgie, 
                                        element_data.tracker,
                                        element_data.nvvidconv,
                                        element_data.nvosd,
                                        element_data.nvvidconv2,
                                        element_data.encoder,
                                        element_data.rtppay,
                                        NULL)){
                g_printerr( "first linking session | error | could not be linked: 1. Exiting.\n");
                return -1;
            }

            gst_element_set_state (element_data.pipeline, GST_STATE_PLAYING);
            g_print ("Running...\n");

            // Set up RTSP media factory
            gst_rtsp_media_factory_set_launch(element_data.factory, 
                "( rtspsrc location=rtsp://192.168.0.145:8554/xx ! rtph264depay ! queue ! nvv4l2decoder ! nvvidconv ! nvv4l2h264enc bitrate=4000000 control-rate=1 preset-level=1 ! rtph264pay name=pay0 pt=96 )");
            GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(element_data.server);
            gst_rtsp_mount_points_add_factory(mounts, "/output", element_data.factory);

            // Start the RTSP server
            g_print("RTSP server started at rtsp://127.0.0.1:8554/output\n");
            gst_rtsp_server_attach(element_data.server, NULL);

            g_main_loop_run (element_data.loop);


            g_print ("Returned, stopping playback\n");
            gst_element_set_state (element_data.pipeline, GST_STATE_NULL);

            g_print ("Deleting pipeline\n");
            gst_object_unref (GST_OBJECT (element_data.pipeline));
            g_source_remove (element_data.bus_watch_id);
            g_object_unref(mounts);
            g_object_unref(element_data.server);
            g_main_loop_unref (element_data.loop);
        }
        catch (const std::exception& e) {
            std::cerr << RED_COLOR << "Error during inference: " << e.what() << RESET_COLOR << std::endl;
            return 1;
        }
    } 

    return 0;
}


// Bus message handler (error and EOS handling)
static gboolean on_bus_message(GstBus *bus, GstMessage *message, GMainLoop *loop) {
    GError *err;
    gchar *debug_info;

    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_ERROR:
            gst_message_parse_error(message, &err, &debug_info);
            g_printerr("Error received from element %s: %s\n",
                       GST_OBJECT_NAME(message->src), err->message);
            if (debug_info) {
                g_printerr("Debug info: %s\n", debug_info);
            } else {
                g_printerr("No additional debug info available.\n");
            }
            g_error_free(err);
            g_free(debug_info);
            break;
        default:
            break;
    }
    return TRUE;
}

static void on_pad_added(GstElement *src, GstPad *new_pad, GstElement *depay) {
    GstPad *sink_pad = gst_element_get_static_pad(depay, "sink");
    if (!sink_pad) {
        g_printerr("Failed to get sink pad of depay element. Exiting.\n");
        return;
    }
    if (gst_pad_is_linked(sink_pad)) {
        g_object_unref(sink_pad);
        return;
    }
    if (gst_pad_link(new_pad, sink_pad) != GST_PAD_LINK_OK) {
        g_printerr("Failed to link new pad to depay sink pad.\n");
    }
    g_object_unref(sink_pad);
}

static GstPadProbeReturn event_probe_callback(GstPad *pad, GstPadProbeInfo *info, gpointer user_data) {
    g_print("Event probe triggered\n");
    GstCaps *caps = gst_pad_get_current_caps(pad);
    gchar *caps_str = gst_caps_to_string(caps);
    printf("Caps for pad of jpeg_parse : %s\n", caps_str);

    gst_caps_unref(caps);
    g_free(caps_str);
    return GST_PAD_PROBE_PASS;
}

static GstPadProbeReturn buffer_probe_callback(GstPad *pad, GstPadProbeInfo *info, gpointer user_data) {
    GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    if (buffer) {
        GstCaps *caps = gst_pad_get_current_caps(pad);
        gchar *caps_str = gst_caps_to_string(caps);
        printf("Caps for pad of appsink: %s\n", caps_str);
        g_print("Buffer probe triggered: Buffer size = %zu\n", gst_buffer_get_size(buffer));

        gst_caps_unref(caps);
        g_free(caps_str);
    }
    return GST_PAD_PROBE_PASS;
}
