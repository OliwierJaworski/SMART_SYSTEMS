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
    GstElement  *src_pipeline, 
                *sink_pipeline,
                *source, 
                *jpeg_parser,
                *decoder,
                *to_bgr_conv,
                *app_sink,
                *app_source,
                *from_bgr_conv,
                *encoder,
                *sink;

    guint64 frame_count;   /* Number of samples generated so far (for timestamp generation) */
    guint sourceid;        /* To control the GSource */
}CustomData;

typedef struct RTSP_CustomData {
    GstElement  *src_pipeline, 
                *sink_pipeline, 
                *source, 
                *depay,
                *queue,
                *decoder,
                *convert_to_sink,
                *app_sink,

                *app_source,
                *convert_to_source,
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
static GstFlowReturn new_sample_rtsp (GstElement *sink, RTSP_CustomData *data);
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
                
            
            }else if (mode == "infer_image"){
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
static GstFlowReturn new_sample_rtsp (GstElement *sink, RTSP_CustomData *data){
    g_print ("sample received\n");
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
            int num_bytes_to_print = 100;  // Print first 30 bytes for example
            printf("Buffer data (first %d bytes):\n", num_bytes_to_print);
            for (int i = 0; i < num_bytes_to_print; i++) {
                printf("%02x ", map.data[i]);
                if ((i + 1) % 10 == 0) {
                    printf("\n");
                }
            }
            printf("\n");
        
        cv::Mat bgr_img(IMAGE_H, IMAGE_W, CV_8UC3, map.data);
        std::vector<Detection> detections;
    
        yolov11.preprocess(bgr_img);
        yolov11.infer();
        
        
        yolov11.postprocess(detections);
        yolov11.draw(bgr_img, detections);

        GstBuffer *new_buffer = gst_buffer_new_allocate(NULL, bgr_img.total() * bgr_img.elemSize(), NULL);
        GstMapInfo new_map;
        if (gst_buffer_map(new_buffer, &new_map, GST_MAP_WRITE)) {
            memcpy(new_map.data, bgr_img.data, bgr_img.total() * bgr_img.elemSize());
            gst_buffer_unmap(new_buffer, &new_map);
        }
        
        GstFlowReturn ret;
        g_signal_emit_by_name(data->app_source, "push-buffer", new_buffer, &ret);

        if (ret != GST_FLOW_OK) {
            g_printerr("Failed to push buffer into appsrc\n");
            gst_buffer_unref(new_buffer);
            return GST_FLOW_ERROR;
        }
        gst_buffer_unref(new_buffer);

        if (ret != GST_FLOW_OK) {
            g_printerr("Failed to push buffer into appsrc\n");
            gst_buffer_unref(new_buffer);
            return GST_FLOW_ERROR;
        }
        gst_buffer_unmap(new_buffer, &new_map);
    }
        /*
        bool success = cv::imwrite("output_image.jpg", bgr_img);
        } else {
            std::cerr << "Failed to map buffer." << std::endl;
        }
        */
        
        gst_buffer_unmap(buffer, &map);
        gst_sample_unref (sample);

        return GST_FLOW_OK;
  }
  return GST_FLOW_ERROR;
}

static GstFlowReturn new_sample (GstElement *sink, CustomData *data) {
    g_print ("sample received\n");
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
            int num_bytes_to_print = 100;  // Print first 30 bytes for example
            printf("Buffer data (first %d bytes):\n", num_bytes_to_print);
            for (int i = 0; i < num_bytes_to_print; i++) {
                printf("%02x ", map.data[i]);
                if ((i + 1) % 10 == 0) {
                    printf("\n");
                }
            }
            printf("\n");
        
        cv::Mat bgr_img(IMAGE_H, IMAGE_W, CV_8UC3, map.data);
        std::vector<Detection> detections;
    
        yolov11.preprocess(bgr_img);
        yolov11.infer();
        
        
        yolov11.postprocess(detections);
        yolov11.draw(bgr_img, detections);

        GstBuffer *new_buffer = gst_buffer_new_allocate(NULL, bgr_img.total() * bgr_img.elemSize(), NULL);
        GstMapInfo new_map;
        if (gst_buffer_map(new_buffer, &new_map, GST_MAP_WRITE)) {
            memcpy(new_map.data, bgr_img.data, bgr_img.total() * bgr_img.elemSize());
        }
        
        g_signal_emit_by_name (data->app_source, "push-buffer", new_buffer, &ret);

        if (ret != GST_FLOW_OK) {
            g_printerr("Failed to push buffer into appsrc\n");
            gst_buffer_unref(new_buffer);
            return GST_FLOW_ERROR;
        }
        gst_buffer_unmap(new_buffer, &new_map);
    }
        /*
        bool success = cv::imwrite("output_image.jpg", bgr_img);
        } else {
            std::cerr << "Failed to map buffer." << std::endl;
        }
        */
        
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