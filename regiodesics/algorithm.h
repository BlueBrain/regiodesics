#ifndef REGIODESICS_ALGORITHM_H
#define REGIODESICS_ALGORITHM_H

#include "Volume.h"
#include "types.h"

#include <boost/geometry/arithmetic/arithmetic.hpp>

Volume<char> annotateBoundaryVoxels(const Volume<unsigned short>& volume);

Segments findNearestVoxels(const Volume<char>& volume, char from, char to);

Volume<float> computeRelativeDistanceField(const Volume<char>& shell,
                                           size_t lineSetSize);

Volume<char> annotateLayers(const Volume<float>& distanceField,
                            const std::vector<float>& separations);

template <typename T>
void clearXRange(Volume<T>& volume, const std::pair<size_t, size_t>& range,
                 const T&& value);

#endif
