#pragma once

#include <iostream>
#include <string>
#include <fstream>
//#include <cuda_runtime_api.h>
#include "NvInfer.h"
#include "NvOnnxParser.h"
#include "LOGGING.h"

const std::string RED_COLOR = "\033[31m";
const std::string GREEN_COLOR = "\033[32m";
const std::string YELLOW_COLOR = "\033[33m";
const std::string RESET_COLOR = "\033[0m";



/*************************************************
 * @brief object detection class 
 * 
 * this class provides methods used for:
 * -> initialization of the necessary variables/objects.
 * -> memory management of objects created by the class.
 * -> execution of object detection.
 *
 ************************************************/
class TRT_MODEL {
public:
   
    TRT_MODEL(std::string engine_path);
    ~TRT_MODEL();
    /**
     * @brief Converts an ONNX model to a TensorRT engine.
     * 
     * @param onnxFile Path to the ONNX file.
     * @param memorySize Size of the memory allocated for the engine.
     */
    void convertOnnxToEngine(const std::string& onnxFile);

    
private:

    /**
     * @brief Initialize TensorRT components from the given engine file.
     *
     * @param engine_path Path to the serialized TensorRT engine file.
     * @param logger Reference to a TensorRT logger for error reporting.
     */
    void init(std::string engine_path, nvinfer1::ILogger& logger);

    Logger logging; //!< Logger class for TensorRT | inherits from nvinfer1::ILogger
    nvinfer1::IRuntime* runtime; //!< The TensorRT runtime used to deserialize the engine.
    nvinfer1::ICudaEngine* engine; //!< The TensorRT engine used to run the network.
    nvinfer1::IExecutionContext* context; //!< The context for executing inference using an ICudaEngine.

    float* gpu_buffers[2]; //!< The vector of device buffers needed for engine execution.
    float* cpu_output_buffer; //!< Pointer to the output buffer on the host.

    std::string engine_path;//!< where tensorRT engine is stored
    int input_w; //!< Width of the input image.
    int input_h; //!< Height of the input image.
    int num_detections; //!< Number of detections output by the model.
    int detection_attribute_size; //!< Size of each detection attribute.
    int num_classes = 80; //!< Number of object classes that can be detected.

};

#define CUDA_CHECK(callstr)\
    {\
        cudaError_t error_code = callstr;\
        if (error_code != cudaSuccess) {\
            std::cerr << "CUDA error " << error_code << " at " << __FILE__ << ":" << __LINE__;\
            assert(0);\
        }\
    }
#endif 