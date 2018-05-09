#ifndef LIGHTDB_GPUCONTEXT_H
#define LIGHTDB_GPUCONTEXT_H

#include "errors.h"
#include <glog/logging.h>
#include <stdexcept>
#include <dynlink_nvcuvid.h>

class GPUContext {
public:
    explicit GPUContext(const unsigned int deviceId): device_(0), owned_(true) {
        CUresult result;

        if(!ensureInitialized())
            throw GpuRuntimeError("GPU context initialization failed");
        else if((result = cuCtxGetCurrent(&context_)) != CUDA_SUCCESS)
            throw GpuCudaRuntimeError("Call to cuCtxGetCurrent failed", result);
        else if(context_ != nullptr)
            owned_ = false;
        else if((result = cuDeviceGet(&device_, deviceId)) != CUDA_SUCCESS)
            throw GpuCudaRuntimeError(std::string("Call to cuDeviceGet failed for device ") + std::to_string(deviceId),
                                      result);
        else if((result = cuCtxCreate(&context_, CU_CTX_SCHED_AUTO, device_)) != CUDA_SUCCESS)
            throw GpuCudaRuntimeError(std::string("Call to cuCtxCreate failed for device ") + std::to_string(deviceId),
                                      result);
    }

    GPUContext(const GPUContext& other)
            : device_(other.device_),
              context_(other.context_),
              owned_(false)
    { }
    GPUContext(GPUContext&& other) noexcept
            : device_(other.device_),
              context_(other.context_),
              owned_(other.owned_) {
        other.device_ = 0;
        other.context_ = nullptr;
    }

    ~GPUContext() {
        CUresult result;

        if(owned_ && context_ != nullptr && (result = cuCtxDestroy(context_)) != CUDA_SUCCESS)
            LOG(ERROR) << "Swallowed failure to destroy CUDA context (CUresult " << result << ") in destructor";
    }


    CUdevice device() const noexcept { return device_; }
    CUcontext get() const { return context_; }

    void AttachToThread() const {
        CUresult result;

        if((result = cuCtxSetCurrent(context_)) != CUDA_SUCCESS) {
            throw GpuCudaRuntimeError("Call to cuCtxSetCurrent failed", result);
        }
    }

private:
    static bool isInitialized;
    static bool ensureInitialized() {
            CUresult result;

            if(isInitialized)
                return true;
            else if((result = cuInit(0, __CUDA_API_VERSION, nullptr)) != CUDA_SUCCESS)
                throw GpuCudaRuntimeError("Call to cuInit failed", result);
            else if(cuvidInit(0) != CUDA_SUCCESS)
                throw GpuCudaRuntimeError("Call to cuvidInit failed", result);
            else
                return (isInitialized = true);
        }

    CUdevice device_;
    CUcontext context_ = nullptr;
    bool owned_;
};

#endif //LIGHTDB_GPUCONTEXT_H
