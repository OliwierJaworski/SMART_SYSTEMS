#include "../include/TRT_MODEL.h"

TRT_MODEL::TRT_MODEL(){

}
TRT_MODEL::~TRT_MODEL(){

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