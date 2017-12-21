#ifndef REGIODESICS_ALGORITHM_H
#define REGIODESICS_ALGORITHM_H

#include "Volume.h"
#include "types.h"

#include <boost/geometry/arithmetic/arithmetic.hpp>

Volume<char> annotateBoundaryVoxels(const Volume<unsigned short>& volume);

Segments findNearestVoxels(const Volume<char>& volume, char from, char to);

using SegmentIndex =
    boost::geometry::index::rtree<Segment, boost::geometry::index::rstar<16>>;

SegmentIndex computeSegmentIndex(const Volume<char>& shell);

Volume<float> computeRelativeDistanceField(const Volume<char>& shell,
                                           size_t lineSetSize,
                                           const SegmentIndex* index = 0);
std::tuple<Volume<Point3f>, Volume<float>> computeOrientationsAndHeights(
    const Volume<char>& shell, size_t lineSetSize,
    const SegmentIndex* index = 0);

Volume<char> annotateLayers(const Volume<float>& distanceField,
                            const std::vector<float>& separations);

template <typename T, int axis>
void clearOutsideRange(Volume<T>& volume,
                       const std::pair<size_t, size_t>& range, T&& value);

template <typename T>
void clearOutsideXRange(Volume<T>& volume,
                        const std::pair<size_t, size_t>& range, T&& value)
{
    clearOutsideRange<T, 0>(volume, range, std::forward<T>(value));
}

template <typename T>
void clearOutsideYRange(Volume<T>& volume,
                        const std::pair<size_t, size_t>& range, T&& value)
{
    clearOutsideRange<T, 1>(volume, range, std::forward<T>(value));
}

template <typename T>
void clearOutsideZRange(Volume<T>& volume,
                        const std::pair<size_t, size_t>& range, T&& value)
{
    clearOutsideRange<T, 2>(volume, range, std::forward<T>(value));
}

#endif
