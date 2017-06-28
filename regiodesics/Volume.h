#ifndef REGIODESICS_VOLUME_H
#define REGIODESICS_VOLUME_H

#include "types.h"

#include "nrrd.hxx"

#include <cassert>
#include <memory>
#include <tuple>
#include <utility>

#include <boost/geometry/index/rtree.hpp>
#include <boost/filesystem/path.hpp>

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
            throw(std::runtime_error("NRRD file's header is empty"));

        _checkType(dataInfo["type"]);

        std::string dataFile = filename;
        if (dataInfo.count("datafile") > 0)
        {
            boost::filesystem::path dataFilePath =
                boost::filesystem::path(filename).parent_path();
            dataFilePath /= dataInfo["datafile"];
            dataFile = dataFilePath.string();
        }

        if (dataInfo["endian"] != "little")
            throw(std::runtime_error(
                "Big endian volumes are not supported"));

        std::stringstream dims(dataInfo["sizes"]);
        dims >> _width >> _height >> _depth;
        const size_t size = _width * _height * _depth;
        if (dims.fail())
            throw(std::runtime_error(
                "Error parsing volume size"));

        std::fstream inFile;
        inFile.open(filename, std::ios::in | std::ios::binary);
        inFile.seekg(headerSize);
        _data.reset(new T[size]);
        inFile.read((char*)_data.get(), sizeof(T) * size);
    }

    Volume(const Volume&) = delete;
    Volume& operator=(const Volume&) = delete;

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
        apply([value](size_t, size_t, size_t, const T&) {
            return value;
        });
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

        visit([&idx, value](size_t x, size_t y, size_t z, const T& v)
              {
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
    T operator()(const PointT<U>& point) const
    {
        return operator()(point.template get<0>(), point.template get<1>(),
                          point.template get<2>());
    }

    template <typename U>
    T operator()(const U x, const U y, const U z) const
    {
        static_assert(!std::is_integral<T>::value,
                      "Interpolation of integer volumes is not valid");

        size_t i = x;
        size_t j = y;
        size_t k = z;

        U a = x - i;
        U b = y - j;
        U c = z - k;

        T p[] = {(*this)(i + 1, j, k) * a + (*this)(i, j, k) * (1 - a),
                 (*this)(i + 1, j + 1, k) * a + (*this)(i, j + 1, k) * (1 - a),
                 (*this)(i + 1, j, k + 1) * a + (*this)(i, j, k + 1) * (1 - a),
                 (*this)(i + 1, j + 1, k + 1) * a +
                     (*this)(i, j + 1, k + 1) * (1 - a)};
        T q[] = {p[1] * b + p[0] * (1 - b), p[3] * b + p[2] * (1 - b)};
        return q[1] * c + q[0] * (1 - c);
    }

    T & operator()(size_t x, size_t y, size_t z)
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

    void _checkType(const std::string& type);
};


template<>
inline void Volume<char>::_checkType(const std::string& type)
{
    if (type != "char")
        throw(std::runtime_error("Unexpected volume type: " + type));
}

template<>
inline void Volume<unsigned short>::_checkType(const std::string& type)
{
    if (type != "unsigned short")
        throw(std::runtime_error("Unexpected volume type: " + type));
}

template<>
inline void Volume<float>::_checkType(const std::string& type)
{
    if (type != "float")
        throw(std::runtime_error("Unexpected volume type: " + type));
}

#endif
