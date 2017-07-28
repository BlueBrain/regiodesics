#include "algorithm.h"

#include <boost/geometry/algorithms/distance.hpp>
#include <boost/progress.hpp>

namespace
{
template <typename T, size_t N>
PointTN<T, N> operator-(const PointTN<T, N>& p, const PointTN<T, N>& q)
{
    PointTN<T, N> t(p);
    boost::geometry::subtract_point(t, q);
    return t;
}

template <typename T, size_t N>
PointTN<T, N> operator+(const PointTN<T, N>& p, const PointTN<T, N>& q)
{
    PointTN<T, N> t(p);
    boost::geometry::add_point(t, q);
    return t;
}

template <typename T, size_t N>
PointTN<T, N>& operator+=(PointTN<T, N>& p, const PointTN<T, N>& q)
{
    boost::geometry::add_point(p, q);
    return p;
}

template <typename T, size_t N>
PointTN<T, N>& operator-=(PointTN<T, N>& p, const PointTN<T, N>& q)
{
    boost::geometry::subtract_point(p, q);
    return p;
}

template <typename T, size_t N>
PointTN<T, N> operator*(const PointTN<T, N>& p, const T& a)
{
    PointTN<T, N> t(p);
    boost::geometry::multiply_value(t, a);
    return t;
}

float _relativePositionOnSegment(const Segment& segment, Point3f c)
{
    using namespace boost::geometry;
    Point3f p = point3d_cast<float>(segment.first);
    Point3f q = point3d_cast<float>(segment.second);
    Segmentf s(p, q);
    subtract_point(q, p);
    const float totalLength = length(s);
    divide_value(q, length(s));
    subtract_point(c, p);
    multiply_value(q, dot_product(q, c));
    add_point(q, p);
    const float t = length(Segmentf(p, q)) / totalLength;
    return std::min(1.f, std::max(0.f, t));
}
}

Volume<char> annotateBoundaryVoxels(const Volume<unsigned short>& volume)
{
    Volume<char> output(volume.width(), volume.height(), volume.depth());
    output.set(0);
    output.apply([&volume](size_t x, size_t y, size_t z, const char&) {
        if (volume(x, y, z) == 0)
            return Void;

        // Border voxels are always boundary
        if (x == 0 || y == 0 || z == 0 || x == volume.width() - 1 ||
            y == volume.height() - 1 || z == volume.depth() - 1)
            return Shell;

        if (volume(x - 1, y, z) == 0 || volume(x + 1, y, z) == 0 ||
            volume(x, y - 1, z) == 0 || volume(x, y + 1, z) == 0 ||
            volume(x, y, z - 1) == 0 || volume(x, y, z + 1) == 0)
            return Shell;
        return Interior;
    });
    return output;
}

Segments findNearestVoxels(const Volume<char>& volume, char from, char to)
{
    Segments segments;
    auto index = volume.createIndex(to);
    volume.visit(
        [index, from, &segments](size_t x, size_t y, size_t z, const char& v) {
            if (v != from)
                return;
            Coords origin(x, y, z);
            std::vector<Coords> nearest;
            index.query(boost::geometry::index::nearest(origin, 1),
                        std::back_inserter(nearest));
            segments.push_back(Segment(origin, nearest[0]));
        });
    return segments;
}

SegmentIndex computeSegmentIndex(const Volume<char>& shell)
{
    auto segments = findNearestVoxels(shell, Bottom, Top);
    auto segments2 = findNearestVoxels(shell, Top, Bottom);
    // We need to reverse the second set of segments to make sure all go in
    // the same direction.
    for (const auto& segment : segments2)
        segments.push_back(Segment(segment.second, segment.first));

    return SegmentIndex(segments.begin(), segments.end());
}

Volume<float> computeRelativeDistanceField(const Volume<char>& shell,
                                           const size_t setSize,
                                           const SegmentIndex* inIndex)
{
    const SegmentIndex& index = inIndex ? *inIndex : computeSegmentIndex(shell);

    size_t width, height, depth;
    std::tie(width, height, depth) = shell.dimensions();

    Volume<float> field(width, height, depth);
    boost::progress_display progress(width * height);

    for (size_t x = 0; x < width; ++x)
    {
#pragma omp parallel for schedule(dynamic)
        for (size_t y = 0; y < height; ++y)
        {
            for (size_t z = 0; z < depth; ++z)
            {
                if (shell(x, y, z) == 0)
                {
                    field(x, y, z) = NAN;
                    continue;
                }

                Segments neighbours;

                // Computing the relative position of this voxel along a
                // virtual top bottom line.
                // For this we use this measures:
                // - The average of the relative position of its projection
                //   to the `setSize' nearest lines connecting top and bottom
                //   voxels (a)
                // - And the relative position on the nearest line (b)
                // The final result is computed as t^8 * a + (1 - t^8) * b where
                // t is 1 - (b - 0.5) * 2 if b > 0.5 or b * 2 if b < 0.5
                // The average is good for points between top and bottom but
                // tends to move distances to the center, the linear
                // interpolation with the closest distance correct this drift
                // near the outer shell.
                Coords coords(x, y, z);

                const Point3f point(x, y, z);
                index.query(boost::geometry::index::nearest(coords, 1),
                            std::back_inserter(neighbours));
                float relativeToClosest =
                    _relativePositionOnSegment(neighbours[0], point);

                neighbours.clear();
                index.query(boost::geometry::index::nearest(coords, setSize),
                            std::back_inserter(neighbours));
                float average = 0;

                for (const auto& segment : neighbours)
                    average += _relativePositionOnSegment(segment, point);

                average /= neighbours.size();

                const float t = relativeToClosest < 0.5
                                    ? 1 - relativeToClosest * 2
                                    : (relativeToClosest - 0.5) * 2;
                const float t8 = std::pow(t, 8);
                const float pos = relativeToClosest * t8 + average * (1 - t8);

                field(x, y, z) = pos;
            }
#pragma omp critical
            ++progress;
        }
    }

    return field;
}

std::tuple<Volume<Point3f>, Volume<float>> computeOrientationsAndHeights(
    const Volume<char>& shell, const size_t setSize,
    const SegmentIndex* inIndex)
{
    const SegmentIndex& index = inIndex ? *inIndex : computeSegmentIndex(shell);

    size_t width, height, depth;
    std::tie(width, height, depth) = shell.dimensions();

    Volume<Point3f> orientations(width, height, depth);
    Volume<float> heights(width, height, depth);
    boost::progress_display progress(width * height);

    for (size_t i = 0; i < width; ++i)
    {
#pragma omp parallel for schedule(dynamic)
        for (size_t j = 0; j < height; ++j)
        {
            for (size_t k = 0; k < depth; ++k)
            {
                if (shell(i, j, k) == 0)
                {
                    orientations(i, j, k) = Point3f(0, 0, 0);
                    heights(i, j, k) = NAN;
                    continue;
                }

                Segments neighbours;
                Coords coords(i, j, k);
                using namespace boost::geometry;
                index.query(index::nearest(coords, setSize),
                            std::back_inserter(neighbours));

                Point3f averageDirection(0, 0, 0);
                float averageLength = 0;
                for (const auto& segment : neighbours)
                {
                    Point3f p(segment.first.get<0>(), segment.first.get<1>(),
                            segment.first.get<2>());
                    Point3f q(segment.second.get<0>(), segment.second.get<1>(),
                            segment.second.get<2>());
                    Segmentf s(p, q);
                    subtract_point(q, p);
                    const auto l = length(s);
                    averageLength += l;
                    divide_value(q, l);
                    averageDirection += q;
                }

                const auto x = averageDirection.get<0>();
                const auto y = averageDirection.get<1>();
                const auto z = averageDirection.get<2>();
                const auto l = std::sqrt(x * x + y * y + z * z);
                divide_value(averageDirection, l);

                averageLength = averageLength / neighbours.size();

                orientations(i, j, k) = averageDirection;
                heights(i, j, k) = averageLength;
            }
#pragma omp critical
            ++progress;
        }
    }

    return std::make_tuple(std::move(orientations), std::move(heights));
}

Volume<char> annotateLayers(const Volume<float>& distanceField,
                            const std::vector<float>& separations)
{
    size_t width, height, depth;
    std::tie(width, height, depth) = distanceField.dimensions();
    Volume<char> layers(width, height, depth);

    boost::progress_display progress(width * height);
    for (size_t x = 0; x < width; ++x)
    {
#pragma omp parallel for schedule(dynamic)
        for (size_t y = 0; y < height; ++y)
        {
            for (size_t z = 0; z < depth; ++z)
            {
                auto value = distanceField(x, y, z);
                if (std::isnan(value))
                {
                    layers(x, y, z) = 0;
                    continue;
                }

                // Finding out in which layer this voxel falls
                char layer = 1;
                for (auto s : separations)
                {
                    if (value < s)
                        break;
                    ++layer;
                }
                layers(x, y, z) = layer;
            }
#pragma omp critical
            ++progress;
        }
    }
    return layers;
}

template <typename T>
void clearXRange(Volume<T>& volume, const std::pair<size_t, size_t>& range,
                 const T&& value)
{
    if (range.first == 0 && range.second >= volume.width())
        return;

    for (size_t i = 0; i < range.first; ++i)
        for (size_t j = 0; j < volume.height(); ++j)
            for (size_t k = 0; k < volume.depth(); ++k)
                volume(i, j, k) = value;
    for (size_t i = std::min(size_t(range.second), volume.width() - 1) + 1;
         i < volume.width(); ++i)
        for (size_t j = 0; j < volume.height(); ++j)
            for (size_t k = 0; k < volume.depth(); ++k)
                volume(i, j, k) = 0;
}

template void clearXRange<unsigned short>(Volume<unsigned short>&,
                                          const std::pair<size_t, size_t>&,
                                          const unsigned short&&);

template void clearXRange<char>(Volume<char>&, const std::pair<size_t, size_t>&,
                                const char&&);

template void clearXRange<float>(Volume<float>&,
                                 const std::pair<size_t, size_t>&,
                                 const float&&);
