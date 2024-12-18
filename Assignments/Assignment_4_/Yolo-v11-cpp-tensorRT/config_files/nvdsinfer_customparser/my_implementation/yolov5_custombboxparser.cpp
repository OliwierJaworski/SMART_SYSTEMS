/*
 * Copyright (c) 2021-2024, NVIDIA CORPORATION. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <cstring>
#include <iostream>
#include "nvdsinfer_custom_impl.h"
#include <cassert>
#include <cmath>
#include <algorithm>
#include <map>

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define CLIP(a,min,max) (MAX(MIN(a, max), min))
#define DIVIDE_AND_ROUND_UP(a, b) ((a + b - 1) / b)

struct MrcnnRawDetection {
    float y1, x1, y2, x2, class_id, score;
};

#define BOLD_RED "\033[1;31m"
#define RESET_COLOR "\033[0m"

/* This is a sample bounding box parsing function for the sample FasterRCNN
 *
 * detector model provided with the SDK. */

/* C-linkage to prevent name-mangling */
extern "C"
bool NvDsInferParseCustomBatchedYoloV5NMSTLT (
         std::vector<NvDsInferLayerInfo> const &outputLayersInfo,
         NvDsInferNetworkInfo  const &networkInfo,
         NvDsInferParseDetectionParams const &detectionParams,
         std::vector<NvDsInferObjectDetectionInfo> &objectList);

/**
 * Type definition for the custom bounding box parsing function.
 *
 * @param[in]  outputLayersInfo A vector containing information on the output
 *                              layers of the model.
 * @param[in]  networkInfo      Network information.
 * @param[in]  detectionParams  Detection parameters required for parsing
 *                              objects.
 * @param[out] objectList       A reference to a vector in which the function
 *                              is to add parsed objects.
 */
extern "C"
bool NvDsInferParseCustomBatchedYoloV5NMSTLT (
         std::vector<NvDsInferLayerInfo> const &outputLayersInfo,
         NvDsInferNetworkInfo  const &networkInfo,
         NvDsInferParseDetectionParams const &detectionParams,
         std::vector<NvDsInferObjectDetectionInfo> &objectList) {

    
    std::cout << BOLD_RED 
              << "========== CUSTOM YOLOv5 PARSER FUNCTION STARTED =========="
              << RESET_COLOR << std::endl;
     if(outputLayersInfo.size() != 1)
    {
        std::cerr << "Mismatch in the number of output buffers."
                  << "Expected 1 output buffer, detected in the network :"
                  << outputLayersInfo.size() << std::endl;
        return false;
    }
    
//--------------------
int centerX = networkInfo.width / 2; // X coordinate of the center
int centerY = networkInfo.height / 2; // Y coordinate of the center
int boxWidth = 100; // Width of the bounding box
int boxHeight = 100; // Height of the bounding box

// Assuming detectionParams and objectList are set up
float threshold = detectionParams.perClassThreshold[0];

// Assuming a bounding box at the center of the image
int left = centerX - boxWidth / 2;
int top = centerY - boxHeight / 2;

// Ensure the bounding box is within image bounds
left = std::max(0, std::min<int>(left, networkInfo.width - 1));
top = std::max(0, std::min<int>(top, networkInfo.height - 1));
int right = std::min<int>(left + boxWidth, networkInfo.width);
int bottom = std::min<int>(top + boxHeight, networkInfo.height);

// Simulate a classId and confidence score for this bounding box
int simulatedClassId = 0; // Example classId
float simulatedConfidence = 0.9f; // Example confidence score

// Display the bounding box info
std::cout << "Displaying bounding box at the center: " << std::endl;
std::cout << "Class: " << simulatedClassId << ", Confidence: " << simulatedConfidence << std::endl;
std::cout << "Bounding Box: (Left: " << left << ", Top: " << top << ", Right: " << right << ", Bottom: " << bottom << ")" << std::endl;

// Assuming you're storing objects in objectList
NvDsInferObjectDetectionInfo object;
object.classId = simulatedClassId;
object.detectionConfidence = simulatedConfidence;
object.left = left;
object.top = top;
object.width = right - left;
object.height = bottom - top;

// Add this simulated object to the list
objectList.push_back(object);

//--------------------
    /* Host memory for "BatchedNMS"
       BatchedNMS has 4 output bindings, the order is:
       keepCount, bboxes, scores, classes
    */
/*
    int* p_keep_count = (int *) outputLayersInfo[0].buffer;
    float* p_bboxes = (float *) outputLayersInfo[1].buffer;
    float* p_scores = (float *) outputLayersInfo[2].buffer;
    float* p_classes = (float *) outputLayersInfo[3].buffer;

    const float threshold = detectionParams.perClassThreshold[0];

    const int keep_top_k = 200;
    const char* log_enable = std::getenv("ENABLE_DEBUG");

    if(log_enable != NULL && std::stoi(log_enable)) {
        std::cout <<"keep cout"
              <<p_keep_count[0] << std::endl;
    }

    for (int i = 0; i < p_keep_count[0] && objectList.size() <= keep_top_k; i++) {

        if ( p_scores[i] < threshold) continue;

        if(log_enable != NULL && std::stoi(log_enable)) {
            std::cout << "label/conf/ x/y x/y -- "
                      << p_classes[i] << " " << p_scores[i] << " "
                      << p_bboxes[4*i] << " " << p_bboxes[4*i+1] << " " << p_bboxes[4*i+2] << " "<< p_bboxes[4*i+3] << " " << std::endl;
        }

        if((unsigned int) p_classes[i] >= detectionParams.numClassesConfigured) continue;
        if(p_bboxes[4*i+2] < p_bboxes[4*i] || p_bboxes[4*i+3] < p_bboxes[4*i+1]) continue;

        NvDsInferObjectDetectionInfo object;
        object.classId = (int) p_classes[i];
        object.detectionConfidence = p_scores[i];

        object.left = CLIP(p_bboxes[4*i], 0, networkInfo.width - 1);
        object.top = CLIP(p_bboxes[4*i+1], 0, networkInfo.height - 1);
        object.width = CLIP(p_bboxes[4*i+2], 0, networkInfo.width - 1) - object.left;
        object.height = CLIP(p_bboxes[4*i+3], 0, networkInfo.height - 1) - object.top;

        if(object.height < 0 || object.width < 0)
            continue;
        objectList.push_back(object);
    }
    return true;
    */
   std::cout << "========== CUSTOM YOLOv11 PARSER FUNCTION COMPLETED ==========" << std::endl;
   return true;
}

/* Check that the custom function has been defined correctly */
CHECK_CUSTOM_PARSE_FUNC_PROTOTYPE(NvDsInferParseCustomBatchedYoloV5NMSTLT);
 