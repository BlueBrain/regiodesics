#ifndef REGIODESICS_TYPES_H
#define REGIODESICS_TYPES_H

#include <boost/geometry/arithmetic/arithmetic.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/segment.hpp>
#include <boost/geometry/core/cs.hpp>

#include <vector>

const char Void = 0;
const char Interior = 1;
const char Shell = 2;
const char Top = 3;
const char Bottom = 4;

using Coords = boost::geometry::model::point<unsigned int, 3,
                                             boost::geometry::cs::cartesian>;

template <typename T>
using PointT = boost::geometry::model::point<
    T, 3, boost::geometry::cs::cartesian>;
using Point = PointT<float>;

using Segment = boost::geometry::model::segment<Coords>;
using SegmentF = boost::geometry::model::segment<Point>;
using Segments = std::vector<Segment>;

#endif
