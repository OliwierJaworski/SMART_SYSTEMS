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

### running inference
- [NVIDIA/TensoRT github](https://github.com/NVIDIA/TensorRT/tree/c468d67760e066f4d85c3e7f2fa89aa221fac83f)
- [nvidia developer blog](https://developer.nvidia.com/blog/speed-up-inference-tensorrt/)
- [classification inference guide](https://github.com/NVIDIA-developer-blog/code-samples/tree/master/posts/TensorRT-introduction)

## usefull will be placed somewhere else later
- onnx to tensorrt engine : 'trtexec --onnx=resnet50/model.onnx --saveEngine=resnet_engine_intro.engine'