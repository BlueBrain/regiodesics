#include "algorithm.h"

#include <boost/geometry/algorithms/distance.hpp>
#include <boost/progress.hpp>

namespace
{

float _relativePositionOnSegment(const Segment& segment, Point c)
{
    using namespace boost::geometry;
    Point p(segment.first.get<0>(), segment.first.get<1>(),
            segment.first.get<2>());
    Point q(segment.second.get<0>(), segment.second.get<1>(),
            segment.second.get<2>());
    SegmentF s(p, q);
    subtract_point(q, p);
    const float totalLength = length(s);
    divide_value(q, length(s));
    subtract_point(c, p);
    multiply_value(q, dot_product(q, c));
    add_point(q, p);
    const float t = length(SegmentF(p, q)) / totalLength;
    return std::min(1.f, std::max(0.f, t));

}
}

Volume<char> annotateBoundaryVoxels(const Volume<unsigned short>& volume)
{
    Volume<char> output(volume.width(), volume.height(), volume.depth());
    output.set(0);
    output.apply([&volume](size_t x, size_t y, size_t z,
                          const char&)
                 {
                     if (volume(x, y, z) == 0)
                         return Void;

                     // Border voxels are always boundary
                     if (x == 0 || y == 0 || z == 0 ||
                         x == volume.width() - 1 || y == volume.height() - 1 ||
                         z == volume.depth() - 1)
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
    volume.visit([index, from, &segments](size_t x, size_t y,
                                          size_t z, const char& v) {
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

Volume<float> computeRelativeDistanceField(const Volume<char>& shell,
                                           const size_t setSize)
{
    auto segments = findNearestVoxels(shell, Bottom, Top);
    auto segments2 = findNearestVoxels(shell, Top, Bottom);
    // We need to reverse the second set of segments to make sure all go in
    // the same direction.
    for (const auto& segment : segments2)
        segments.push_back(Segment(segment.second, segment.first));

    // Creating the index.
    using SegmentIndex =
        boost::geometry::index::rtree<Segment,
                                      boost::geometry::index::rstar<16>>;
    SegmentIndex index(segments.begin(), segments.end());

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
                auto value = shell(x, y, z);
                if (value == 0)
                {
                    field(x, y, z) = -1;
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

                const Point point(x, y, z);
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
                if (value < 0)
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
