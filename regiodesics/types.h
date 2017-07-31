#ifndef REGIODESICS_TYPES_H
#define REGIODESICS_TYPES_H

#include <boost/geometry/arithmetic/arithmetic.hpp>
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/segment.hpp>

#include <iostream>
#include <vector>

const char Void = 0;
const char Interior = 1;
const char Shell = 2;
const char Bottom = 3;
const char Top = 4;

template <typename T, size_t N>
using PointTN =
    boost::geometry::model::point<T, N, boost::geometry::cs::cartesian>;

using Point2f = PointTN<float, 2>;
using Point3f = PointTN<float, 3>;
using Point4f = PointTN<float, 4>;
using Coords = PointTN<unsigned int, 3>;

using Segment = boost::geometry::model::segment<Coords>;
using Segmentf = boost::geometry::model::segment<Point3f>;
using Segments = std::vector<Segment>;

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

template <typename T, typename U>
PointTN<T, 3> point3d_cast(const PointTN<U, 3>& point)
{
    return PointTN<T, 3>(static_cast<T>(point.template get<0>()),
                         static_cast<T>(point.template get<1>()),
                         static_cast<T>(point.template get<2>()));
}
#endif
