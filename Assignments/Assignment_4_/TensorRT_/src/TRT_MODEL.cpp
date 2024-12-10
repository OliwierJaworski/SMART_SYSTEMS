#include "../include/TRT_MODEL.h"

TRT_MODEL::TRT_MODEL(std::string engine_path) : engine_path{engine_path}{
    init(engine_path,logging);
}
TRT_MODEL::~TRT_MODEL(){

}

void TRT_MODEL::init(std::string engine_path, nvinfer1::ILogger& logger)
{
    // Open the engine file in binary mode
    std::ifstream engineStream(engine_path, std::ios::binary);
    // Move to the end to determine file size
    engineStream.seekg(0, std::ios::end);
    const size_t modelSize = engineStream.tellg();
    // Move back to the beginning of the file
    engineStream.seekg(0, std::ios::beg);
    // Allocate memory to read the engine data
    std::unique_ptr<char[]> engineData(new char[modelSize]);
    // Read the engine data into memory
    engineStream.read(engineData.get(), modelSize);
    engineStream.close();

    // Create a TensorRT runtime instance
    runtime = nvinfer1::createInferRuntime(logger);
    // Deserialize the CUDA engine from the engine data
    engine = runtime->deserializeCudaEngine(engineData.get(), modelSize);
    // Create an execution context for the engine
    context = engine->createExecutionContext();

    // Retrieve input dimensions from the engine
    input_h = engine->getBindingDimensions(0).d[2];
    input_w = engine->getBindingDimensions(0).d[3];
    // Retrieve detection attributes and number of detections
    detection_attribute_size = engine->getBindingDimensions(1).d[1];
    num_detections = engine->getBindingDimensions(1).d[2];
    // Calculate the number of classes based on detection attributes
    num_classes = detection_attribute_size - 4;

    // Allocate CPU memory for output buffer
    cpu_output_buffer = new float[detection_attribute_size * num_detections];
    // Allocate GPU memory for input buffer (assuming 3 channels: RGB)
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(gpu_buffers[0]), 3 * input_w * input_h * sizeof(float)));
    // Allocate GPU memory for output buffer
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(gpu_buffers[1]), detection_attribute_size * num_detections * sizeof(float)));

    // Initialize CUDA preprocessing with maximum image size
    cuda_preprocess_init(MAX_IMAGE_SIZE);

    // Create a CUDA stream for asynchronous operations
    CUDA_CHECK(cudaStreamCreate(&stream));
}

void TRT_MODEL::convertOnnxToEngine(const std::string& onnxFile){
    //check if onnx file exists
    std::ifstream onnx_file(onnxFile);
    if(!onnx_file){
        std::cerr << RED_COLOR << "Could not open Onnx." << std::endl << "relative file path: " << onnxFile << std::endl;
        return;
    }
    nvinfer1::IBuilder *builder = nvinfer1::createInferBuilder(logging);
    uint32_t explicitBatch = 1U << static_cast<uint32_t>(nvinfer1::NetworkDefinitionCreationFlag::kEXPLICIT_BATCH);

    
    nvinfer1::INetworkDefinition *network = builder->createNetworkV2(explicitBatch);
    nvinfer1::IBuilderConfig *config = builder->createBuilderConfig();

    //CREATE AN ONNX PARSER
    nvonnxparser::IParser *parser = nvonnxparser::createParser(*network, logging);

    bool parsed= parser->parseFromFile(onnxFile.c_str(),static_cast<int>(nvinfer1::ILogger::Severity::kINFO));
    nvinfer1::IHostMemory *plan{ builder->buildSerializedNetwork(*network, *config)};

    //CREATE TENSORRT RUNTIME
    runtime = nvinfer1::createInferRuntime(logging);

    //DESERIALIZE CUDA ENGINE FROM SERIALIZED PLAN
    engine = runtime->deserializeCudaEngine(plan->data(),plan->size());

    context = engine->createExecutionContext();
//--------------------------------------------------------------------------------------

    // Generate the engine file path by replacing the extension with ".engine"
    std::string engine_path;
    size_t dotIndex = onnxFile.find_last_of(".");
    if (dotIndex != std::string::npos) {
        engine_path = onnxFile.substr(0, dotIndex) + ".engine";
    }
    else
    {
        return; // Return false if no extension is found
    }
    if(engine){
        nvinfer1::IHostMemory *data = engine->serialize();
        std::ofstream file;

        file.open(engine_path, std::ios::binary | std::ios::out);
        if (!file.is_open())
        {
            std::cout << "Create engine file " << engine_path << " failed" << std::endl;
            return;
        }
        file.write((const char*)data->data(), data->size());
        file.close();
        delete data;
    }
    delete network;
    delete config;
    delete parser;
    delete plan;
}