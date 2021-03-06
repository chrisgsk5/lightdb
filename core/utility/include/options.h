#ifndef LIGHTDB_OPTIONS_H
#define LIGHTDB_OPTIONS_H

#include <any>
#include <unordered_map>

namespace lightdb {
    template<typename TKey=std::string, typename TValue=std::any>
    class options: public std::unordered_map<TKey, TValue> {
    public:
        using std::unordered_map<TKey, TValue>::unordered_map;

        struct Encoding {
            static constexpr const char* GOPSize = "GOP";
        };
        struct Geometry {
            static constexpr const char* Volume = "Volume";
            static constexpr const char* Projection = "Projection";
        };


        std::optional<TValue> get(const TKey& key) const {
            auto value = this->find(key);
            return value != this->end()
                ? std::optional<TValue>{(*value).second}
                : std::optional<TValue>{};
        }

        template<typename TOptional>
        std::optional<TOptional> get(const TKey& key) const {
            auto value = this->find(key);
            return value != this->end()
                   ? std::optional<TOptional>{std::any_cast<TOptional>((*value).second)}
                   : std::optional<TOptional>{};
        }
    };

    template<typename TKey=std::string, typename TValue=std::any>
    class OptionContainer {
    public:
        virtual const lightdb::options<TKey, TValue> &options() const = 0;
        const std::optional<TValue> get_option(const TKey& key) const { return options().get(key); }
    };

    using EncodeOptions = options<>::Encoding;
    using GeometryOptions = options<>::Geometry;
} // namespace lightdb

#endif //LIGHTDB_OPTIONS_H
