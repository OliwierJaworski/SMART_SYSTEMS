## Introduction
How will tensorrt be incorporated into the project

## what is it for 
## installation help
-> before running sudo apt-get install tensorrt  
run -> sudo apt-get update 
otherways it might not find any tensorrt package
if any error shows up try installing different version of cuda for example 12.2 was broken i installed 12.6 and i was able to install
## usefull links:

### installation
- [medium installation tutorial](https://medium.com/@moshiur.faisal01/install-tensorrt-with-command-line-wrapper-trtexec-on-ununtu-20-04-lts-3e44f4f36a2b)
- [nvidia installation tutorial](https://docs.nvidia.com/deeplearning/tensorrt/quick-start-guide/index.html)
- [tensort download page](https://developer.nvidia.com/tensorrt/download/10x)
- [cuda toolkit](https://developer.nvidia.com/cuda-downloads?target_os=Linux&target_arch=x86_64&Distribution=Ubuntu&target_version=22.04&target_type=deb_local)
- [example deployment using onnx](https://docs.nvidia.com/deeplearning/tensorrt/quick-start-guide/index.html#ex-deploy-onnx)
-[cli command to create tensorRT engine](https://docs.nvidia.com/deeplearning/tensorrt/quick-start-guide/index.html#convert-model)
- [trtexec installation](https://forums.developer.nvidia.com/t/where-is-trtexec/73514)
- [can't find trtexec after installation](https://forums.developer.nvidia.com/t/bash-trtexec-command-not-found/127302/5)
- [tensorrt api documentation](https://docs.nvidia.com/deeplearning/tensorrt/index.html)
- [reinstall cuda thread](https://forums.developer.nvidia.com/t/tensorrt-installation-problem/265125/8)
- [cudnn install](https://developer.nvidia.com/cudnn-downloads?target_os=Linux&target_arch=x86_64&Distribution=Ubuntu&target_version=22.04&target_type=deb_network)
### running inference
- [NVIDIA/TensoRT github](https://github.com/NVIDIA/TensorRT/tree/c468d67760e066f4d85c3e7f2fa89aa221fac83f)
- [nvidia developer blog](https://developer.nvidia.com/blog/speed-up-inference-tensorrt/)
- [classification inference guide](https://github.com/NVIDIA-developer-blog/code-samples/tree/master/posts/TensorRT-introduction)
- [updated inference guide](https://developer.nvidia.com/blog/speeding-up-deep-learning-inference-using-tensorrt-updated/)
- [nvidia tensorrt documentation](https://docs.nvidia.com/deeplearning/tensorrt/sample-support-guide/index.html#onnx_mnist_sample)

- [youtube video tensorRT tutorial](https://www.youtube.com/watch?v=Z0n5aLmcRHQ)
- [youtube video github page](https://github.com/cyrusbehr/tensorrt-cpp-api/blob/main/src/main.cpp)

### first attempt 
- [spdlog](https://github.com/gabime/spdlog?tab=readme-ov-file)
- [fmt](https://github.com/fmtlib/fmt/releases/tag/11.0.2)

- [alternative simple onnx->trt->inference in c++ <- weird example](https://github.com/ggluo/TensorRT-Cpp-Example)
- [medium tutorial yolo10 onnx tensorRT object detection](https://medium.com/@boukamchahamdi/yolov10-c-tensorrt-project-27f66c163de1)

### maybe usefull oneday 
- [cross compilation x86_64 for arm64](https://forums.developer.nvidia.com/t/how-to-across-compile-the-tensorrt-sample-code-in-x86-64-for-aarch64/180932/2)

## usefull will be placed somewhere else later
- onnx to tensorrt engine : 'trtexec --onnx=resnet50/model.onnx --saveEngine=resnet_engine_intro.engine'

## jetson nano stuff

### dependencies download
-[jetson nano nvidia specifications](https://developer.nvidia.com/embedded/jetpack-sdk-461)
- cuda version : -- cat /usr/local/cuda/version.txt -- -> 10.2
- tensorRT version : -- dpkg -l | grep nvinfer -- -> 8.2
- OS version : ubuntu -v1804

### issues fixed:
1. -  //usr/lib/aarch64-linux-gnu/tegra/libnvmedia.so: undefined reference to `TVMRLDCUpdateTNR2Params' -
-> sudo apt-get install --reinstall nvidia-l4t-jetson-multimedia-api

### data parsing
- [DATACAMP: yolo model explained](https://www.datacamp.com/blog/yolo-object-detection-explained?utm_source=google&utm_medium=paid_search&utm_campaignid=19589720821&utm_adgroupid=152984011334&utm_device=c&utm_keyword=&utm_matchtype=&utm_network=g&utm_adpostion=&utm_creative=684592139693&utm_targetid=aud-1964540900041:dsa-2222697811358&utm_loc_interest_ms=&utm_loc_physical_ms=9197485&utm_content=DSA~blog~Data-Science&utm_campaign=230119_1-sea~dsa~tofu_2-b2c_3-row-p1_4-prc_5-na_6-na_7-le_8-pdsh-go_9-nb-e_10-na_11-na&gad_source=1&gclid=Cj0KCQiAgdC6BhCgARIsAPWNWH1hOvU7uaViPjwTZ32YlG6DmhF56tnSTTLAJAJ7lSaL89dN0sPWUqYaAiDBEALw_wcB)
- [ENCORD: yolo model explained](https://encord.com/blog/yolo-object-detection-guide/)
- [DATACAMP: CNN explained](https://www.datacamp.com/tutorial/introduction-to-convolutional-neural-networks-cnns?utm_source=google&utm_medium=paid_search&utm_campaignid=19589720821&utm_adgroupid=157156374951&utm_device=c&utm_keyword=&utm_matchtype=&utm_network=g&utm_adpostion=&utm_creative=684592139651&utm_targetid=aud-1964540900041:dsa-2218886984380&utm_loc_interest_ms=&utm_loc_physical_ms=9197485&utm_content=&utm_campaign=230119_1-sea~dsa~tofu_2-b2c_3-row-p1_4-prc_5-na_6-na_7-le_8-pdsh-go_9-nb-e_10-na_11-na&gad_source=1&gclid=Cj0KCQiAgdC6BhCgARIsAPWNWH26c4S1bXY6IZuRk6nnz8oWmPS7jvQ4PgVscG7n4hNTWopI-3dnPpgaArVTEALw_wcB)
- [Ultralytics: ONNX export YOLOV11](https://docs.ultralytics.com/integrations/onnx/)
- [nvidia docs](https://docs.nvidia.com/deeplearning/tensorrt/archives/tensorrt-825/api/index.html#api)

### issues noted
- [onnx(int64 model)-> transformed to int32](https://forums.developer.nvidia.com/t/onnx-model-int64-weights/124248/9)
- 