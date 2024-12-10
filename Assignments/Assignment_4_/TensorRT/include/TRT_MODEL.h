#pragma once

#include <iostream>
#include <string>
#include <fstream>
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
   
    TRT_MODEL();
    ~TRT_MODEL();
    /**
     * @brief Converts an ONNX model to a TensorRT engine.
     * 
     * @param onnxFile Path to the ONNX file.
     * @param memorySize Size of the memory allocated for the engine.
     */
    void convertOnnxToEngine(const std::string& onnxFile);

private:

    Logger logging; //!< Logger class for TensorRT | inherits from nvinfer1::ILogger
    nvinfer1::IRuntime* runtime; //!< The TensorRT runtime used to deserialize the engine.
    nvinfer1::ICudaEngine* engine; //!< The TensorRT engine used to run the network.
    nvinfer1::IExecutionContext* context; //!< The context for executing inference using an ICudaEngine.
};

