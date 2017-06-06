#include <utility>
#include <tuple>
#include <memory>
#include <cassert>

using VoxelID = std::tuple<unsigned int, unsigned int, unsigned int>;

template <typename T>
class Volume
{
public:

    Volume(unsigned int width, unsigned int height, unsigned int depth)
        // The storage is in colume-major order
        : _data(new T[width * height * depth])
        , _width(width)
        , _height(height)
        , _depth(depth)
    {
    }

    Volume(Volume&& volume)
        : _data(std::move(volume))
        , _width(volume.width)
        , _height(volume.height)
        , _depth(volume.depth)
    {
    }

    Volume(const Volume&) = delete;
    Volume& operator=(const Volume&) = delete;

    std::tuple<unsigned int, unsigned int, unsigned> dimensions() const
    {
        return std::make_tuple(_width, _height, _depth);
    }

    unsigned int width() const
    {
        return _width;
    }

    unsigned int height() const
    {
        return _height;
    }

    unsigned int depth() const
    {
        return _depth;
    }

    void set(const T& value)
    {
        apply([value](unsigned int, unsigned int, unsigned int, const T&)
              { return value; });
    }

    template<typename Functor>
    void visit(const Functor& functor)
    {
        for (unsigned int x = 0; x != _width; ++x)
        {
            for (unsigned int y = 0; y != _height; ++y)
            {
                for (unsigned int z = 0; z != _depth; ++z)
                {
                    functor(x, y, z, operator()(x, y, z));
                }
            }
        }
    }

    template<typename Functor>
    void apply(const Functor& functor)
    {
        for (unsigned int x = 0; x != _width; ++x)
        {
            for (unsigned int y = 0; y != _height; ++y)
            {
                for (unsigned int z = 0; z != _depth; ++z)
                {
                    T& v = operator()(x, y, z);
                    v = functor(x, y, z, v);
                }
            }
        }
    }

    T& operator()(const VoxelID& voxel)
    {
        unsigned int x, y, z;
        std::tie(x, y, z) = voxel;
        return operator()(x, y, z);
    }

    const T& operator()(const VoxelID& voxel) const
    {
        unsigned int x, y, z;
        std::tie(x, y, z) = voxel;
        return operator()(x, y, z);
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
};
