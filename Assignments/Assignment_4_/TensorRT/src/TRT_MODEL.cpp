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
    auto builder = nvinfer1::createInferBuilder(logging);
}