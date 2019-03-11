#ifndef LIGHTDB_ENCODEOPERATORS_H
#define LIGHTDB_ENCODEOPERATORS_H

#include "LightField.h"
#include "PhysicalOperators.h"
#include "Codec.h"
#include "Configuration.h"
#include "VideoEncoder.h"
#include "EncodeWriter.h"
#include "VideoEncoderSession.h"
#include "MaterializedLightField.h"
#include "lazy.h"
#include <cstdio>
#include <utility>

namespace lightdb::physical {

class GPUEncode : public GPUUnaryOperator<GPUDecodedFrameData> /*, public EncodedVideoInterface*/ {
public:
    static constexpr size_t kDefaultGopSize = 30;

    explicit GPUEncode(const LightFieldReference &logical,
                       PhysicalLightFieldReference &parent,
                       Codec codec)
            : GPUUnaryOperator(logical, parent),
              codec_(std::move(codec)),
              encodeConfiguration_([this]() {
                  //return EncodeConfiguration{configuration(), codec_.nvidiaId().value(), gop()}; }),
                  return EncodeConfiguration{new_encode_configuration_, codec_.nvidiaId().value(), gop()}; }),
              encoder_([this]() { return VideoEncoder(context(), encodeConfiguration_, lock()); }),
              writer_([this]() { return MemoryEncodeWriter(encoder_->api()); }),
              encodeSession_([this]() { return VideoEncoderSession(encoder_, writer_); }) {
        if(!codec.nvidiaId().has_value())
            throw GpuRuntimeError("Requested codec does not have an Nvidia encode id");
    }

    std::optional<physical::MaterializedLightFieldReference> read() override {
        if (iterator() != iterator().eos()) {
            auto decoded = iterator()++;
            new_encode_configuration_ = decoded.configuration(); //TODO

            for (const auto &frame: decoded.frames())
                encodeSession_->Encode(*frame, decoded.configuration().offset.top, decoded.configuration().offset.left);

            //TODO think this should move down just above nullopt
            // Did we just reach the end of the decode stream?
            if (iterator() == iterator().eos())
                // If so, flush the encode queue and end this op too
                encodeSession_->Flush();

            return {CPUEncodedFrameData(codec_, decoded.configuration(), writer_->dequeue())};
        } else
            return std::nullopt;
    }

    const Codec &codec() const /*override */ { return codec_; }
    //const Configuration &configuration() override {
    //    return GPUUnaryOperator<GPUDecodedFrameData>::configuration();
    //}

private:
    const Codec codec_;
    lazy <EncodeConfiguration> encodeConfiguration_;
    lazy <VideoEncoder> encoder_;
    lazy <MemoryEncodeWriter> writer_;
    lazy <VideoEncoderSession> encodeSession_;
    Configuration new_encode_configuration_;

    unsigned int gop() const {
        auto option = logical().is<OptionContainer<>>()
                ? logical().downcast<OptionContainer<>>().get_option(EncodeOptions::GOPSize)
                : std::nullopt;

        if(option.has_value() && option.value().type() != typeid(unsigned int))
            throw InvalidArgumentError("Invalid GOP option specified", EncodeOptions::GOPSize);
        else
            return std::any_cast<unsigned int>(option.value_or(
                    std::make_any<unsigned int>(kDefaultGopSize)));
    }
};

}; // namespace lightdb::physical

#endif //LIGHTDB_ENCODEOPERATORS_H
