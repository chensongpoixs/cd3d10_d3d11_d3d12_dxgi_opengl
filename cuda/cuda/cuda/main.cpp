//1 C++使用cuda api获取当前GPU显卡的显存容量、已使用显存、剩余显存
//在一些使用场景下，当我们的应用程序需要从显卡中开辟显存时，为了增加程序的健壮性，常常需要在开启程序的时候获取当前电脑是否有足够的显存，
//在这一层面，所有的系统都没有提供太好的公用api可供调用，所以需要使用N卡的cuda库获取显卡的可用显存。
//这里使用的主要是cudaMemGetInfo的这个api，这个api会在出参返回显卡可用显存的字节数与显卡总共的显存字节数。


 
//以下代码通过该api返回了当前显卡的总共的显存容量、已使用显存容量、剩余显存容量，需要在项目配置cuda的include目录与lib目录，
//并链接cuda.lib、cudadevrt.lib、cudart.lib、cudart_static.lib、cublas.lib、cublasLt.lib库。

#include <iostream>
#include "cuda_runtime_api.h"

static void GetGPUMemory()
{
    int deviceCount = 0;
    cudaError_t error_id = cudaGetDeviceCount(&deviceCount);
    if (deviceCount == 0)
    {
        std::cout << "当前PC没有支持CUDA的显卡硬件设备" << std::endl;
    }

    size_t gpu_total_size;
    size_t gpu_free_size;

    cudaError_t cuda_status = cudaMemGetInfo(&gpu_free_size, &gpu_total_size);

    if (cudaSuccess != cuda_status) 
    {
        std::cout << "Error: cudaMemGetInfo fails : " << cudaGetErrorString(cuda_status) << std::endl;
        exit(1);
    }

    double total_memory = double(gpu_total_size) / (1024.0 * 1024.0);
    double free_memory = double(gpu_free_size) / (1024.0 * 1024.0);
    double used_memory = total_memory - free_memory;

    std::cout << "\n"
        << "当前显卡总共有显存" << total_memory << "m \n"
        << "已使用显存" << used_memory << "m \n"
        << "剩余显存" << free_memory << "m \n" << std::endl;

}


int test_main()
{
    GetGPUMemory();
    return 0;
}