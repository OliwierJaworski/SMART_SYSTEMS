#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include "NvInfer.h"
#include "NvOnnxParser.h"

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
    class Logger logging;

};

/*************************************************
 * @brief Logger class for TensorRT
 * 
 * This class inherits from nvinfer1::ILogger and provides a logging mechanism for TensorRT.
 ************************************************/
class Logger : public nvinfer1::ILogger {
public:
    /**
     * @brief Logs a message with a given severity.
     * 
     * @param severity The severity level of the message.
     * @param msg The message to log.
     */
    void log(Severity severity, const char* msg) noexcept override {
        if (severity <= Severity::kWARNING)
            std::cout << msg << std::endl;
    }
};