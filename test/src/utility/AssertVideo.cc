#include "AssertVideo.h"

Frame CREATE_BLACK_FRAME(const Configuration &configuration)
{
    CUdeviceptr handle;
    size_t pitch;

    EXPECT_EQ(cuMemAllocPitch(
            &handle,
            &pitch,
            configuration.width,
            configuration.height * 3 / 2,
            8), CUDA_SUCCESS);

    // Set frame to black
    //   Chroma planes
    EXPECT_EQ(cuMemsetD2D8(handle, pitch, 128, configuration.width, configuration.height * 3 / 2), CUDA_SUCCESS);
    //   Luma plane
    EXPECT_EQ(cuMemsetD2D8(handle, pitch, 16, configuration.width, configuration.height), CUDA_SUCCESS);

    return Frame(handle, static_cast<unsigned int>(pitch), configuration, NV_ENC_PIC_STRUCT_FRAME);
}

void ASSERT_BLACK_FRAME(const Frame &frame)
{
    auto local = LocalFrame(frame);

    for(auto y = 0u; y < local.height(); y++)
        for(auto x = 0u; x < local.width(); x++)
            ASSERT_EQ(local(x, y), 16); // Black Y luma

    for(auto y = local.height(); y < local.height() * 3 / 2; y++)
        for(auto x = 0u; x < local.width(); x++)
            ASSERT_EQ(local(x, y), 128); // Black UV chroma
}