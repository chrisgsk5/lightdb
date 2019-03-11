#ifndef LIGHTDB_UNIONOPERATORS_H
#define LIGHTDB_UNIONOPERATORS_H

#include "PhysicalOperators.h"
#include "EncodeOperators.h"
#include "Rectangles.h"

namespace lightdb::physical {

class GPUTileUnion : public GPUOperator {
public:
    explicit GPUTileUnion(const LightFieldReference &logical,
                          std::vector<PhysicalLightFieldReference> &parents,
                          const unsigned int rows, const unsigned int columns)
            : GPUOperator(logical, parents,
                          parents.front().downcast<GPUOperator>().gpu(),
                          [this, rows, columns]() {
                              return create_configurationTODOremove(rows, columns); }),
              rows_(rows),
              columns_(columns),
              frames_([this] { return functional::make_union_iterator<GPUFrameReference>(
                      functional::transform<functional::flatmap_iterator<GPUFrameReference, PhysicalLightField::downcast_iterator<GPUDecodedFrameData>>>(iterators().begin(), iterators().end(), [](PhysicalLightField::iterator &it) { return functional::make_flatmap_iterator<GPUFrameReference>(it.downcast<GPUDecodedFrameData>()); })); })
    { }

    GPUTileUnion(const GPUTileUnion &) = delete;

    std::optional<physical::MaterializedLightFieldReference> read() override {
        if(!any_parent_eos()) {
            auto available_frames = std::max(frames_->available(), 1ul);
            //GPUDecodedFrameData output{configuration()};
            //std::unique_ptr<GPUDecodedFrameData> output; //{frames_.value()};
            GPUDecodedFrameData output{create_configuration2()};

            for(auto i = 0u; i < available_frames; i++) {
                auto frames = frames_.value()++;
                //auto unioned = GPUFrameReference::make<CudaFrame>(configuration().height, configuration().width, frames[0]->type());
                auto unioned = GPUFrameReference::make<CudaFrame>(
                        frames.front()->height() * rows_,
                        frames.front()->width() * columns_,
                        frames.front()->type());
                auto &cuda = unioned.downcast<CudaFrame>();

                for(auto column = 0u; column < columns_; column++)
                    for(auto row = 0u; row < rows_; row++)
                        cuda.copy(lock(), *frames[row * columns_ + column]->cuda(),
                                  create_configuration2().height / rows_,
                                  create_configuration2().width / columns_);

                //if(output == nullptr)
                  //  output = std::make_unique(frames.front()->);

                output.frames().emplace_back(GPUFrameReference{unioned});
            }

            return {output};
        } else
            return {};
    }

private:
    Configuration create_configurationTODOremove(const unsigned int rows, const unsigned int columns) {
        CHECK(!parents().empty());

        const auto &configuration = parents()[0].downcast<GPUOperator>().configuration2();
        return Configuration{configuration.width * columns, configuration.height * rows, 0, 0,
                             configuration.bitrate, configuration.framerate, configuration.offset};
    }

    Configuration create_configuration2() {
        CHECK(!parents().empty());

        printf("Broken\n");
        assert(0);
        auto &current = frames_.value().iterators();
        auto first = current.front();
        const Configuration c = (*first.iterator()).configuration();
        auto height = std::accumulate(current.begin(), current.end(), 0u,
                                      [](auto sum, auto &data) { return sum; });// + (*data.iterator()).configuration().height; });
        auto width = std::accumulate(current.begin(), current.end(), 0u,
                                     [](auto sum, auto &data) { return sum; });// + (*data.iterator()).configuration().width; });

        return Configuration{width, height, 0, 0,
                             c.bitrate,
                             c.framerate,
                             c.offset};
    }

    const unsigned int rows_, columns_;
    lazy<functional::union_iterator<
            GPUFrameReference,
            std::vector<
                    functional::flatmap_iterator<GPUFrameReference,
                                                 PhysicalLightField::downcast_iterator<GPUDecodedFrameData>>>>> frames_;
};


template<typename Transform, typename Data>
class GPUOverlayUnion : public GPUOperator {
public:
    explicit GPUOverlayUnion(const LightFieldReference &logical,
                             std::vector<PhysicalLightFieldReference> &parents)
            : GPUOperator(logical, parents,
                          parents.back().downcast<GPUOperator>().gpu(),
                          [this]() { return this->parents().back().downcast<GPUOperator>().configuration2(); }),
              transform_([this]() { return Transform(context()); }),
              groups_(lazy<PhysicalLightField::iterator*>{[this]() { return &iterators()[0]; }})
    { }

    GPUOverlayUnion(const GPUOverlayUnion &) = delete;

    std::optional<physical::MaterializedLightFieldReference> read() override {
        if(!any_parent_eos()) {
            auto video = (iterators().back()++).downcast<GPUDecodedFrameData>();
            GPUDecodedFrameData output{video.configuration()};

            for(auto &frame: video.frames()) {
                auto values = groups_++;
                auto unioned = transform_.value().nv12().draw(lock(), frame->cuda(), values);
                output.frames().emplace_back(unioned);
            }

            return {output};
        } else
            return {};
    }

private:
    class GroupById {
    public:
        explicit GroupById(lazy<PhysicalLightField::iterator*> iterator)
                : index_(0u),
                  current_id_(0u),
                  buffer_(MaterializedLightFieldReference::make<CPUDecodedFrameData>(Configuration{})),
                  iterator_(std::move(iterator))
        { }

        std::vector<Data> operator++(int) {
            std::optional<Data> value;
            std::vector<Data> values;

            while((value = peek()).has_value() && value.value().id == current_id_)
                values.emplace_back(next());

            if((value = peek()).has_value())
                current_id_ = value.value().id;

            return values;
        }

    private:
        Data next() {
            auto value = peek().value();
            index_++;
            return value;
        }

        std::optional<Data> peek() {
            if(index_ >= frames().size()) {
                buffer_ = (*iterator_.value())++;
                index_ = 0u;
            }

            if(!frames().empty())
                return *reinterpret_cast<const Data*>(frames().at(index_)->data().data());
            else
                return std::nullopt;
        }

        inline std::vector<LocalFrameReference>& frames() { return data().frames(); }
        inline CPUDecodedFrameData& data() { return buffer_.downcast<CPUDecodedFrameData>(); }

        size_t index_;
        unsigned int current_id_;
        MaterializedLightFieldReference buffer_;
        lazy<PhysicalLightField::iterator*> iterator_;
    };


    lazy<Transform> transform_;
    GroupById groups_;
};

using GPUBoxOverlayUnion = GPUOverlayUnion<video::GPURectangleOverlay, Rectangle>;

} // lightdb::physical

#endif //LIGHTDB_UNIONOPERATORS_H
