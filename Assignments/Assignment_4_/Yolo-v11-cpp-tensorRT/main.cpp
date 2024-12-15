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


#define IMAGE_H 1200
#define IMAGE_W 1200
#define CHUNK_SIZE IMAGE_H * IMAGE_W * 3

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

typedef struct _CustomData {
    GstElement  *pipeline, 
                *source, 
                *jpeg_parser,
                *decoder,
                *to_bgr_conv,
                *app_sink,
                *app_source,
                *encoder,
                *sink;

    guint64 frame_count;   /* Number of samples generated so far (for timestamp generation) */
    guint sourceid;        /* To control the GSource */
}CustomData;

typedef struct RTSP_CustomData {
    GstElement  *pipeline, 
                *source, 
                *depay,
                *queue,
                *decoder,
                *convert,
                *encoder,
                *rtppay;

    guint64 frame_count;   /* Number of samples generated so far (for timestamp generation) */
    guint sourceid;        /* To control the GSource */
}RTSP_ELEMENTS;


static void on_pad_added(GstElement *src, GstPad *new_pad, GstElement *depay);
static void on_pad_added_image(GstElement *decoder, GstPad *pad, GstElement *converter);
static gboolean on_bus_message(GstBus *bus, GstMessage *message, GMainLoop *loop);
static GstFlowReturn on_new_sample_from_sink(GstAppSink *appsink, gpointer user_data);
static void start_feed (GstElement *source, guint size, CustomData *data);
static void stop_feed (GstElement *source, CustomData *data);
static gboolean push_data (CustomData *data);
static GstFlowReturn new_sample (GstElement *sink, CustomData *data);
static GstPadProbeReturn event_probe_callback(GstPad *pad, GstPadProbeInfo *info, gpointer user_data);
static GstPadProbeReturn buffer_probe_callback(GstPad *pad, GstPadProbeInfo *info, gpointer user_data); 


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
                
            
            RTSP_ELEMENTS rtsp_data;
            memset (&rtsp_data, 0, sizeof (rtsp_data));

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
            printf("reached infer_image\n");

            GstBus *bus;
            GstMessage *msg;
            GMainLoop *loop;
            CustomData data;

            memset (&data, 0, sizeof (data));

            gst_init(&argc, &argv);

            loop = g_main_loop_new(NULL, FALSE);

            data.pipeline = gst_pipeline_new("image-inference");

            //get image and process it
            data.source = gst_element_factory_make("filesrc","file-source");
            data.jpeg_parser = gst_element_factory_make("jpegparse","jpeg-parser");
            //data.decoder = gst_element_factory_make("jpegdec","jpeg_decoder");
            data.decoder = gst_element_factory_make("jpegdec","jpeg_decoder");
            data.to_bgr_conv = gst_element_factory_make("videoconvert","bgr_convert");
            data.app_sink = gst_element_factory_make("appsink","infer_begin");

            //get processed image into file
            //app_set = gst_element_factory_make("appsink","infer_end");
            //encoder = gst_element_factory_make("nvjpegenc","jpeg_encoder");
            //sink = gst_element_factory_make("filesink","sink_to_file");
        
            if (!data.pipeline || !data.source || !data.decoder  || !data.to_bgr_conv || !data.app_sink){
                g_printerr("One element could not be created, exiting.\n");
            }
            
            g_object_set(G_OBJECT(data.source),"location",argv[2],NULL);
            //g_object_set(G_OBJECT(decoder),"DeepStream",TRUE, NULL); //!<- error no caps if using nvjpegdec
            //g_object_set(G_OBJECT(sink),"location","./output/output_image.jpg", NULL);
            
            //configure appsink
            /*
            GstCaps *fixed_image_caps = gst_caps_new_simple(
                "video/x-raw",
                "format", G_TYPE_STRING, "BGR",
                "width", G_TYPE_INT, IMAGE_H,
                "height", G_TYPE_INT, IMAGE_W,
                "framerate", GST_TYPE_FRACTION, 1, 1,
                "parsed", G_TYPE_BOOLEAN, true,
                NULL
            );
            */

           GstCaps *fixed_image_caps = gst_caps_new_simple(
                "video/x-raw",
                "format", G_TYPE_STRING, "BGR",
                NULL
            );

            GstCaps *example = gst_caps_new_simple(
                "video/x-raw",
                "format", G_TYPE_STRING, "BGR",
                "width", G_TYPE_INT, IMAGE_H,
                "height", G_TYPE_INT, IMAGE_W,
                "framerate", GST_TYPE_FRACTION, 0, 1,
                "interlace-mode",G_TYPE_STRING,"progressive", 
                "multiview-mode",G_TYPE_STRING,"mono",
                "pixel-aspect-ratio",GST_TYPE_FRACTION, 1, 1,
                "chroma-site",G_TYPE_STRING,"mpeg2",
                "colorimetry",G_TYPE_STRING,"1:4:0:0",
                //"multiview-flags",
                NULL
            );

            //g_signal_connect(data.app_sink,"need-data", G_CALLBACK( start_feed ), &data);
            //g_signal_connect(data.app_sink,"enough-data", G_CALLBACK( stop_feed ), &data);
            //g_object_set (data.decoder,"DeepStream", TRUE, NULL);
            g_object_set (data.app_sink, "emit-signals", TRUE, "caps", fixed_image_caps, NULL);
            g_object_set (data.app_sink, "emit-signals", TRUE, NULL);
            g_signal_connect(data.app_sink, "new-sample", G_CALLBACK( new_sample ), &data);
            gst_caps_unref(fixed_image_caps);


            bus = gst_pipeline_get_bus(GST_PIPELINE(data.pipeline));
            gst_bus_add_watch(bus,(GstBusFunc)on_bus_message,loop);
            gst_object_unref (bus);

            gst_bin_add_many(GST_BIN(data.pipeline), data.source, data.decoder, data.to_bgr_conv, data.app_sink, NULL);
        
            if (gst_element_link_many (data.source,  data.decoder, data.to_bgr_conv ,data.app_sink, NULL) != TRUE) 
            {
                g_printerr ("Elements could not be linked.\n");
                gst_object_unref (data.pipeline);
                return -1;
            }

            //GstPad *src_pad_jpeg_parser = gst_element_get_static_pad(data.jpeg_parser, "src");
            GstPad *src_pad_decoder = gst_element_get_static_pad(data.app_sink, "sink");
            GstPad *bgr_conv_pad = gst_element_get_static_pad(data.to_bgr_conv,"sink");
            //gst_pad_add_probe(src_pad_jpeg_parser, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, event_probe_callback, NULL, NULL);
            gst_pad_add_probe(src_pad_decoder, GST_PAD_PROBE_TYPE_BUFFER, buffer_probe_callback, NULL, NULL);
            gst_pad_add_probe(bgr_conv_pad, GST_PAD_PROBE_TYPE_BUFFER, buffer_probe_callback, NULL, NULL);
            

            gst_element_set_state(data.pipeline,GstState::GST_STATE_PLAYING);
            g_main_loop_run (loop);
            gst_element_set_state(data.pipeline, GST_STATE_NULL);

            gst_object_unref(GST_OBJECT(data.pipeline));
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

static void start_feed (GstElement *source, guint size, CustomData *data) {
  if (data->sourceid == 0) {
    g_print ("Start feeding\n");
    data->sourceid = g_idle_add ((GSourceFunc) push_data, data);
  }
}

static void stop_feed (GstElement *source, CustomData *data) {
  if (data->sourceid != 0) {
    g_print ("Stop feeding\n");
    g_source_remove (data->sourceid);
    data->sourceid = 0;
  }
}

static gboolean push_data (CustomData *data) {
    printf("pushing data\n");
    GstBuffer *buffer;
    GstFlowReturn ret;
    GstMapInfo map;
    
    /* Create a new empty buffer */
    buffer = gst_buffer_new_and_alloc (CHUNK_SIZE);

    /* Set its timestamp and duration */
    GST_BUFFER_TIMESTAMP (buffer) = gst_util_uint64_scale (data->frame_count, GST_SECOND, 60);
    GST_BUFFER_DURATION (buffer) = gst_util_uint64_scale (1, GST_SECOND, 60);

    data->frame_count++;  
    /* create samples here */
    
    /* Map the buffer to read its contents */
    if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        printf("Buffer size: %zu bytes\n", map.size);

        // Print the first few bytes of the buffer
        printf("Buffer content (first 10 bytes): ");
        for (size_t i = 0; i < map.size && i < (CHUNK_SIZE); i++) {
            printf("%02X ", map.data[i]);
        }
        printf("\n");

        gst_buffer_unmap(buffer, &map);
    } else {
        printf("Failed to map buffer\n");
    }
    /* Push the buffer into the appsrc */

    g_signal_emit_by_name (data->app_source, "push-buffer", buffer, &ret);

    /* Free the buffer now that we are done with it */
    gst_buffer_unref (buffer);

    if (ret != GST_FLOW_OK) {
        /* We got some error, stop sending data */
        return FALSE;
    }

    return TRUE;
}

static GstFlowReturn new_sample (GstElement *sink, CustomData *data) {
    printf("sample received\n");
    GstSample *sample;
    GstBuffer *buffer;
    GstFlowReturn ret;
    GstMapInfo map;

    /* Retrieve the buffer */
    g_signal_emit_by_name (sink, "pull-sample", &sample);
    if (sample) {
        YOLOv11 yolov11("best.engine", logger);
        g_print ("\n*\n");
        buffer = gst_sample_get_buffer(sample);

        if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
            // Print the first few bytes of the buffer
            int num_bytes_to_print = 2000;  // Print first 30 bytes for example
            printf("Buffer data (first %d bytes):\n", num_bytes_to_print);
            for (int i = 0; i < num_bytes_to_print; i++) {
                printf("%02x ", map.data[i]);
                if ((i + 1) % 10 == 0) {
                    printf("\n");
                }
            }
            printf("\n");
        
        //cv::Mat img(IMAGE_H,IMAGE_W,CV_8UC3,map.data);
        // Assuming `map.data` points to your YV12 data
        //cv::Mat yv12_img(IMAGE_H + IMAGE_H / 2, IMAGE_W, CV_8UC1, map.data);
        cv::Mat bgr_img(IMAGE_H, IMAGE_W, CV_8UC3, map.data);

        // Convert YV12 (YUV) to BGR format using cvtColor
        //cv::Mat bgr_img;
        //cv::cvtColor(yv12_img, bgr_img, cv::COLOR_YUV2BGR_YV12);
        yolov11.preprocess(bgr_img);
        yolov11.infer();
        
        std::vector<Detection> detections;
        yolov11.postprocess(detections);

        // Draw detections on the image
        yolov11.draw(bgr_img, detections);

        bool success = cv::imwrite("output_image.jpg", bgr_img);
        } else {
            std::cerr << "Failed to map buffer." << std::endl;
        }
        gst_buffer_unmap(buffer, &map);
        gst_sample_unref (sample);

        return GST_FLOW_OK;
  }
  return GST_FLOW_ERROR;
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

/* Functions below print the Capabilities in a human-friendly format */
static gboolean print_field (GQuark field, const GValue * value, gpointer pfx) {
  gchar *str = gst_value_serialize (value);

  g_print ("%s  %15s: %s\n", (gchar *) pfx, g_quark_to_string (field), str);
  g_free (str);
  return TRUE;
}


