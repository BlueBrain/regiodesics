#ifndef LAYERS_ALGORITHM_H
#define LAYERS_ALGORITHM_H

#include "Volume.h"

#include <boost/geometry/arithmetic/arithmetic.hpp>

const char Void = 0;
const char Interior = 1;
const char Shell = 2;
const char Top = 3;
const char Bottom = 4;

using Coords = Volume<char>::Coords;
using Segment = boost::geometry::model::segment<Coords>;
using Point = boost::geometry::model::point<
    float, 3, boost::geometry::cs::cartesian>;
using SegmentF = boost::geometry::model::segment<Point>;
using Segments = std::vector<Segment>;

Volume<char> annotateBoundaryVoxels(const Volume<unsigned short>& volume);

Segments findNearestVoxels(const Volume<char>& volume, char from, char to);

Volume<float> computeRelativeDistanceField(const Volume<char>& shell,
                                           size_t lineSetSize);

Volume<char> annotateLayers(const Volume<float>& distanceField,
                            const std::vector<float>& separations);


#endif
