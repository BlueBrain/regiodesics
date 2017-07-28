#ifndef REGIODESICS_VOLUME_H
#define REGIODESICS_VOLUME_H

#include "types.h"

#include "nrrd.hxx"

#include <cassert>
#include <memory>
#include <tuple>
#include <utility>

#include <boost/filesystem/path.hpp>
#include <boost/geometry/index/rtree.hpp>

template <typename T>
class Volume
{
public:
    using Index =
        boost::geometry::index::rtree<Coords,
                                      boost::geometry::index::linear<5>>;

    Volume(size_t width, size_t height, size_t depth)
        // The storage is in colume-major order
        : _data(new T[width * height * depth]),
          _width(width),
          _height(height),
          _depth(depth)
    {
    }

    Volume(Volume&& volume)
        : _data(std::move(volume._data))
        , _width(volume._width)
        , _height(volume._height)
        , _depth(volume._depth)
    {
    }

    Volume(const std::string& filename)
    {
        size_t headerSize = 0;
        std::map<std::string, std::string> dataInfo;
        headerSize = NRRD::parseHeader(filename, dataInfo);
        if (headerSize == 0)
            throw(std::runtime_error("Error parsing " + filename +
                                     ": error reading header"));

        _checkMetadata(dataInfo);

        std::string dataFile = filename;
        if (dataInfo.count("datafile") > 0)
        {
            boost::filesystem::path dataFilePath =
                boost::filesystem::path(filename).parent_path();
            dataFilePath /= dataInfo["datafile"];
            dataFile = dataFilePath.string();
        }

        if (dataInfo["endian"] != "little")
            throw(std::runtime_error("Big endian volumes are not supported"));

        std::stringstream dims(dataInfo["sizes"]);
        dims >> _width >> _height >> _depth;
        const size_t size = _width * _height * _depth;
        if (dims.fail())
            throw(std::runtime_error("Error parsing volume size"));

        std::fstream inFile;
        inFile.open(filename, std::ios::in | std::ios::binary);
        inFile.seekg(headerSize);
        _data.reset(new T[size]);
        inFile.read((char*)_data.get(), sizeof(T) * size);
    }

    Volume(const Volume&) = delete;
    Volume& operator=(const Volume&) = delete;

    Volume<T> copy() const
    {
        Volume<T> other(_width, _height, _depth);
        memcpy(other._data.get(), _data.get(),
               sizeof(T) * _width * _height * _depth);
        return other;
    }

    void save(const std::string& filename) const
    {
        int dims[] = {int(_width), int(_height), int(_depth)};
        NRRD::save<T>(filename, _data.get(), 3, dims);
    }

    std::tuple<size_t, size_t, size_t> dimensions() const
    {
        return std::make_tuple(_width, _height, _depth);
    }

    size_t width() const { return _width; }
    size_t height() const { return _height; }
    size_t depth() const { return _depth; }
    void set(const T& value)
    {
        apply([value](size_t, size_t, size_t, const T&) { return value; });
    }

    template <typename Functor>
    void visit(const Functor& functor) const
    {
        for (size_t z = 0; z != _depth; ++z)
        {
            for (size_t y = 0; y != _height; ++y)
            {
                for (size_t x = 0; x != _width; ++x)
                {
                    functor(x, y, z, operator()(x, y, z));
                }
            }
        }
    }

    template <typename Functor>
    void apply(const Functor& functor)
    {
        for (size_t z = 0; z != _depth; ++z)
        {
            for (size_t y = 0; y != _height; ++y)
            {
                for (size_t x = 0; x != _width; ++x)
                {
                    T& v = operator()(x, y, z);
                    v = functor(x, y, z, v);
                }
            }
        }
    }

    Index createIndex(T& value) const
    {
        Index idx;

        visit([&idx, value](size_t x, size_t y, size_t z, const T& v) {
            if (v != value)
                return;
            idx.insert(Coords(x, y, z));
        });
        return idx;
    }

    T& operator()(const Coords& coords)
    {
        return operator()(coords.get<0>(), coords.get<1>(), coords.get<2>());
    }

    const T& operator()(const Coords& coords) const
    {
        return operator()(coords.get<0>(), coords.get<1>(), coords.get<2>());
    }

    template <typename U>
    T operator()(const PointTN<U, 3>& point) const
    {
        return operator()(point.template get<0>(), point.template get<1>(),
                          point.template get<2>());
    }

    template <typename U>
    T operator()(const U x, const U y, const U z) const
    {
        static_assert(!std::is_integral<T>::value,
                      "Interpolation of integer volumes is not valid");

        U dx = x == _width - 1 ? 1 : x - std::floor(x);
        U dy = y == _height - 1 ? 1 : y - std::floor(y);
        U dz = z == _depth - 1 ? 1 : z - std::floor(z);

        size_t i = x - dx;
        size_t j = y - dy;
        size_t k = z - dz;

        T p[] = {(*this)(i + 1, j, k) * dx + (*this)(i, j, k) * (1 - dx),
                 (*this)(i + 1, j + 1, k) * dx +
                     (*this)(i, j + 1, k) * (1 - dx),
                 (*this)(i + 1, j, k + 1) * dx +
                     (*this)(i, j, k + 1) * (1 - dx),
                 (*this)(i + 1, j + 1, k + 1) * dx +
                     (*this)(i, j + 1, k + 1) * (1 - dx)};
        T q[] = {p[1] * dy + p[0] * (1 - dy), p[3] * dy + p[2] * (1 - dy)};
        return q[1] * dz + q[0] * (1 - dz);
    }

    T& operator()(size_t x, size_t y, size_t z)
    {
        assert(x < _width);
        assert(y < _height);
        assert(z < _depth);
        return _data[z * _width * _height + y * _width + x];
    }

    const T& operator()(size_t x, size_t y, size_t z) const
    {
        assert(x < _width);
        assert(y < _height);
        assert(z < _depth);
        return _data[z * _width * _height + y * _width + x];
    }

private:
    std::unique_ptr<T[]> _data;
    size_t _width;
    size_t _height;
    size_t _depth;

    void _checkMetadata(std::map<std::string, std::string>& metadata);
};

template <>
inline void Volume<char>::_checkMetadata(
    std::map<std::string, std::string>& metadata)
{
    const auto& type = metadata["type"];
    if (type != "char")
        throw(std::runtime_error("Unexpected volume type: " + type));
    const auto& dims = metadata["dimension"];
    if (std::atoi(dims.c_str()) != 3)
        throw(std::runtime_error("Invalid dimensions: " + dims));
}

template <>
inline void Volume<unsigned short>::_checkMetadata(
    std::map<std::string, std::string>& metadata)
{
    const auto& type = metadata["type"];
    if (type != "unsigned short")
        throw(std::runtime_error("Unexpected volume type: " + type));
    const auto& dims = metadata["dimension"];
    if (std::atoi(dims.c_str()) != 3)
        throw(std::runtime_error("Invalid dimensions: " + dims));
}

template <>
inline void Volume<float>::_checkMetadata(
    std::map<std::string, std::string>& metadata)
{
    const auto& type = metadata["type"];
    if (type != "float")
        throw(std::runtime_error("Unexpected volume type: " + type));
    const auto& dims = metadata["dimension"];
    if (std::atoi(dims.c_str()) != 3)
        throw(std::runtime_error("Invalid dimensions: " + dims));
}

#define CHECK_VECTOR_FIELD_METADATA(T, N)                                 \
    template <>                                                           \
    inline void Volume<PointTN<T, N>>::_checkMetadata(                    \
        std::map<std::string, std::string>& metadata)                     \
    {                                                                     \
        const auto& type = metadata["type"];                              \
        if (type != #T)                                                   \
            throw(std::runtime_error("Unexpected volume type: " + type)); \
        const auto& dims = metadata["dimension"];                         \
        if (std::atoi(dims.c_str()) != 4)                                 \
            throw(std::runtime_error("Invalid dimensions: " + dims));     \
        std::stringstream sizes(metadata["sizes"]);                       \
        size_t first;                                                     \
        sizes >> first;                                                   \
        if (first != N)                                                   \
            throw(std::runtime_error("Invalid size in first dimension")); \
        std::getline(sizes, metadata["sizes"]);                           \
    }

CHECK_VECTOR_FIELD_METADATA(char, 4)
CHECK_VECTOR_FIELD_METADATA(float, 3)


// Partial template specialization of member functions in template classes is
// not allowed by the standard, hence we have to enumerate the cases needed
// with full template specialization.
#define VOLUME_POINT_SAVE(T, N)                                                \
    template <>                                                                \
    inline void Volume<PointTN<T, N>>::save(const std::string& filename) const \
    {                                                                          \
        int dims[] = {N, int(_width), int(_height), int(_depth)};              \
        assert(sizeof(PointTN<T, N>) == sizeof(T) * N);                        \
        NRRD::save<T>(filename, (T*)_data.get(), 4, dims);                     \
    }

VOLUME_POINT_SAVE(float, 2)
VOLUME_POINT_SAVE(float, 3)
VOLUME_POINT_SAVE(float, 4)
VOLUME_POINT_SAVE(char, 4)

#undef VOLUME_POINT_SAVE

#endif
