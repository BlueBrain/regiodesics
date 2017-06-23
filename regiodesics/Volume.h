#ifndef LAYERS_VOLUME_H
#define LAYERS_VOLUME_H

#include "nrrd.hxx"

#include <cassert>
#include <memory>
#include <tuple>
#include <utility>

#include <boost/geometry/index/rtree.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/filesystem/path.hpp>

template <typename T>
class Volume
{
public:
    using Coords =
        boost::geometry::model::point<unsigned int, 3,
                                      boost::geometry::cs::cartesian>;
    using Index =
        boost::geometry::index::rtree<Coords,
                                      boost::geometry::index::linear<5>>;

    Volume(unsigned int width, unsigned int height, unsigned int depth)
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

    std::tuple<unsigned int, unsigned int, unsigned> dimensions() const
    {
        return std::make_tuple(_width, _height, _depth);
    }

    unsigned int width() const { return _width; }
    unsigned int height() const { return _height; }
    unsigned int depth() const { return _depth; }
    void set(const T& value)
    {
        apply([value](unsigned int, unsigned int, unsigned int, const T&) {
            return value;
        });
    }

    template <typename Functor>
    void visit(const Functor& functor) const
    {
        for (unsigned int z = 0; z != _depth; ++z)
        {
            for (unsigned int y = 0; y != _height; ++y)
            {
                for (unsigned int x = 0; x != _width; ++x)
                {
                    functor(x, y, z, operator()(x, y, z));
                }
            }
        }
    }

    template <typename Functor>
    void apply(const Functor& functor)
    {
        for (unsigned int z = 0; z != _depth; ++z)
        {
            for (unsigned int y = 0; y != _height; ++y)
            {
                for (unsigned int x = 0; x != _width; ++x)
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

        visit([&idx, value](
                  unsigned int x, unsigned int y, unsigned int z, const T& v)
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

    T& operator()(unsigned int x, unsigned int y, unsigned int z)
    {
            assert(x < _width);
            assert(y < _height);
            assert(z < _depth);
            return _data[z * _width * _height + y * _width + x];
    }

    const T& operator()(unsigned int x, unsigned int y, unsigned int z) const
    {
            assert(x < _width);
            assert(y < _height);
            assert(z < _depth);
            return _data[z * _width * _height + y * _width + x];
    }

private:
    std::unique_ptr<T[]> _data;
    unsigned int _width;
    unsigned int _height;
    unsigned int _depth;

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

#endif
