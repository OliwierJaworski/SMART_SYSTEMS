#ifdef _WIN32
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


static void on_pad_added(GstElement *src, GstPad *new_pad, GstElement *depay);
static void on_pad_added_image(GstElement *decoder, GstPad *pad, GstElement *converter);
static gboolean on_bus_message(GstBus *bus, GstMessage *message, GMainLoop *loop);
static GstFlowReturn on_new_sample_from_sink(GstAppSink *appsink, gpointer user_data);

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
//----------------------------------------------------------------------------------------------------
    // Handle inference modes
    else if (mode == "infer_video" || mode == "infer_image") {
        try {
            if (mode == "infer_video") {
                
            gst_init(&argc, &argv);

            GstElement *pipeline, *source, *depay, *queue, *decoder, *convert, *encoder, *rtppay;
            GstRTSPServer *server = gst_rtsp_server_new();
            GstRTSPMediaFactory *factory = gst_rtsp_media_factory_new();

            // Create elements for receiving RTSP input
            source = gst_element_factory_make("rtspsrc", "source");
            depay = gst_element_factory_make("rtph264depay", "depay");
            queue = gst_element_factory_make("queue", "queue"); // New queue element
            decoder = gst_element_factory_make("nvv4l2decoder", "decoder");
            convert = gst_element_factory_make("nvvidconv", "convert");
            encoder = gst_element_factory_make("nvv4l2h264enc", "encoder");
            rtppay = gst_element_factory_make("rtph264pay", "rtppay");

            if (!source || !depay || !queue || !decoder || !convert || !encoder || !rtppay) {
                g_printerr("Element creation failed.\n");
                return -1;
            }

            // Set the properties of the RTSP source
            g_object_set(source, "location", "rtsp://192.168.0.145:8554/xx", "timeout", (guint64)30 * GST_SECOND, 
                        "do-timestamp", TRUE, "latency", 2000, NULL); // Increase latency to 500ms
            g_object_set(queue, "max-size-time", 10000000000L, "max-size-buffers", 4000, "max-size-bytes", 10000000, NULL);
            g_object_set(rtppay, "pt", 96, NULL);
            g_object_set(encoder, "bitrate", 2000000, "control-rate", 2, "iframeinterval", 30, NULL);

            // Create pipeline and add elements
            pipeline = gst_pipeline_new("rtsp-in-out-pipeline");
            gst_bin_add_many(GST_BIN(pipeline), source, depay, queue, decoder, convert, encoder, rtppay, NULL); // Add queue element
            g_signal_connect(source, "pad-added", G_CALLBACK(on_pad_added), depay);

            // Link elements
            if (!gst_element_link_many(depay, queue, decoder, convert, encoder, rtppay, NULL)) {
                g_printerr("Failed to link elements.\n");
                return -1;
            }

            // Setup bus watch for error handling
            GstBus *bus = gst_element_get_bus(pipeline);
            gst_bus_add_watch(bus, (GstBusFunc)on_bus_message, NULL);
            gst_object_unref(bus);

            // Start playing the pipeline
            gst_element_set_state(pipeline, GST_STATE_PLAYING);

            // Set up RTSP media factory
            gst_rtsp_media_factory_set_launch(factory, 
                "( rtspsrc location=rtsp://192.168.0.145:8554/xx ! rtph264depay ! queue ! nvv4l2decoder ! nvvidconv ! nvv4l2h264enc bitrate=4000000 control-rate=1 preset-level=1 ! rtph264pay name=pay0 pt=96 )");
            GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);
            gst_rtsp_mount_points_add_factory(mounts, "/output", factory);

            // Start the RTSP server
            g_print("RTSP server started at rtsp://127.0.0.1:8554/output\n");
            gst_rtsp_server_attach(server, NULL);

            // Main loop
            GMainLoop *loop = g_main_loop_new(NULL, FALSE);
            g_main_loop_run(loop);

            // Cleanup
            gst_element_set_state(pipeline, GST_STATE_NULL);
            g_object_unref(mounts);
            g_object_unref(server);
            g_object_unref(pipeline);
//----------------------------------------------------------------------------------------------------
            }else if (mode == "infer_image"){
            printf("%sreached infer_image%s\n",GREEN_COLOR,RESET_COLOR);
            GstElement *pipeline, 
                       *source, 
                       *jpeg_parser,
                       *decoder,
                       *encoder,
                       *sink;

            GstBus *bus;
            GstMessage *msg;
            GstStateChangeReturn ret;
            GMainLoop *loop;

            gst_init(&argc, &argv);

            loop = g_main_loop_new(NULL, FALSE);

            pipeline = gst_pipeline_new("image-inference");
            source = gst_element_factory_make("filesrc","file-source");
            jpeg_parser = gst_element_factory_make("jpegparse","jpeg-parser");
            decoder = gst_element_factory_make("jpegdec","jpeg_decoder");
            encoder = gst_element_factory_make("nvjpegenc","jpeg_encoder");
            sink = gst_element_factory_make("filesink","sink_to_file");
        
            if (!pipeline || !source || !jpeg_parser || !decoder || !encoder || !sink){
                g_printerr("One element could not be created, exitin.\n");
            }

            g_object_set(G_OBJECT(source),"location",argv[2],NULL);
            //g_object_set(G_OBJECT(decoder),"DeepStream",TRUE, NULL); //!<- error no caps if using nvjpegdec
            g_object_set(G_OBJECT(sink),"location","./output_image_jpeg.jpg", NULL);

            bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
            gst_bus_add_watch(bus,(GstBusFunc)on_bus_message,loop);
            gst_object_unref (bus);

            gst_bin_add_many(GST_BIN(pipeline), source, jpeg_parser, decoder, encoder, sink, NULL);
            //!<- linking of elements happens here 

            gst_element_link_many(source, jpeg_parser, decoder, encoder, sink, NULL);

            gst_element_set_state(pipeline,GstState::GST_STATE_PLAYING);
            g_main_loop_run (loop);
            gst_element_set_state(pipeline, GST_STATE_NULL);

            gst_object_unref(GST_OBJECT(pipeline));
            g_main_loop_unref(loop);            
        }
        }
        catch (const std::exception& e) {
            std::cerr << RED_COLOR << "Error during inference: " << e.what() << RESET_COLOR << std::endl;
            return 1;
        }
    } 

    return 0;
}

// Callback for handling dynamic pad-added signal
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

// Bus message handler (error and EOS handling)
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
