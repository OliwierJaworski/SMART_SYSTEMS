#include "../include/TRT_MODEL.h"


int main(int argc, char* argv[]){


     if (argc < 4 || argc > 5) {
        std::cerr << RED_COLOR << "Usage: " << RESET_COLOR << argv[0] << " <mode> <input_path> <engine_path> [onnx_path]" << std::endl;
        std::cerr << YELLOW_COLOR << "  <mode> - Mode of operation: 'convert', 'infer_video', or 'infer_image'" << RESET_COLOR << std::endl;
        std::cerr << YELLOW_COLOR << "  <input_path> - Path to the input video/image or ONNX model" << RESET_COLOR << std::endl;
        std::cerr << YELLOW_COLOR << "  <engine_path> - Path to the TensorRT engine file" << RESET_COLOR << std::endl;
        std::cerr << YELLOW_COLOR << "  [onnx_path] - Path to the ONNX model (only for 'convert' mode)" << RESET_COLOR << std::endl;
        return 1;
    }
    std::string mode = argv[1];
    std::string inputPath = argv[2];
    std::string enginePath = argv[3];
    std::string onnxPath;

    if (mode == "convert") {
        if (argc != 4) {
            std::cerr << "Usage for conversion: " << argv[0] << " convert <onnx_path> <engine_path>" << std::endl;
            return 1;
        }
        onnxPath = inputPath;  // Using inputPath as ONNX path for conversion mode
    } else if (mode == "infer_video") {
        if (argc != 4) {
            std::cerr << "Usage for video inference: " << argv[0] << " infer_video <video_path> <engine_path>" << std::endl;
            return 1;
        }
    } else if (mode == "infer_image") {
        if (argc != 4) {
            std::cerr << "Usage for image inference: " << argv[0] << " infer_image <image_path> <engine_path>" << std::endl;
            return 1;
        }
    } else {
        std::cerr << "Invalid mode. Use 'convert' to convert ONNX model, 'infer_video' to perform inference on video, or 'infer_image' to perform inference on image." << std::endl;
        return 1;
    }

    class TRT_MODEL obj_det_model;

    if (mode == "convert") {
       obj_det_model.convertOnnxToEngine(onnxPath);
    } else if (mode == "infer_video") {
        
    } else if (mode == "infer_image") {
        
    }

    return 0;
}
