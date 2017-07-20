#ifndef LAYERS_UTIL_H
#define LAYERS_UTIL_H

#include "algorithm.h"

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Node>

#include <boost/progress.hpp>

#include <iostream>

template <typename T, typename U>
T point3d_cast(const PointT<U>& point)
{
    return T(point.template get<0>(), point.template get<1>(),
             point.template get<2>());
}

namespace std
{
istream& operator>>(istream& in, std::pair<size_t, size_t>& pair)
{
    string s;
    in >> s;
    const auto pos = s.find(':');
    pair.first = boost::lexical_cast<size_t>(s.substr(0, pos));
    if (pos == string::npos)
        pair.second = std::numeric_limits<size_t>::max();
    else
        pair.second = boost::lexical_cast<size_t>(s.substr(pos + 1));
    return in;
}
}

std::vector<float> computeSplitPoints(const std::vector<float>& thicknesses)
{
    std::vector<float> splitPoints;
    // At least two points are needed.
    if (thicknesses.size() < 2)
        return splitPoints;

    std::vector<float> cummulative({0.f});
    for (auto x : thicknesses)
        cummulative.push_back(cummulative.back() + x);
    const float total = cummulative.back();
    // The last (=total) and first (=0) values are ignored
    for (auto i = ++cummulative.begin(); i != --cummulative.end(); ++i)
    {
        std::cout << *i / total << std::endl;
        splitPoints.push_back(*i / total);
    }
    return splitPoints;
}

osg::Node* createNearestNeighbourLines(const Volume<char>& volume)
{
    size_t width, height, depth;
    std::tie(width, height, depth) = volume.dimensions();

    osg::Vec3Array* vertices = new osg::Vec3Array();
    vertices->reserve((width + 1) * (height + 1) * (depth + 1));
    for (size_t x = 0; x < width; ++x)
        for (size_t y = 0; y < height; ++y)
            for (size_t z = 0; z < depth; ++z)
                vertices->push_back(osg::Vec3(x + 0.5, y + 0.5, z + 0.5));

    osg::DrawElementsUInt* primitive1 = new osg::DrawElementsUInt(GL_LINES);
    auto segments = findNearestVoxels(volume, Top, Bottom);
#define INDEX(c) (c.get<0>() * height * depth + c.get<1>() * depth + c.get<2>())
    for (const auto& segment : segments)
    {
        primitive1->push_back(INDEX(segment.first));
        primitive1->push_back(INDEX(segment.second));
    }
    osg::Vec4Array* color1 = new osg::Vec4Array();
    color1->push_back(osg::Vec4(0, 0.5, 1, 1));

    osg::DrawElementsUInt* primitive2 = new osg::DrawElementsUInt(GL_LINES);
    segments = findNearestVoxels(volume, Bottom, Top);
#define INDEX(c) (c.get<0>() * height * depth + c.get<1>() * depth + c.get<2>())
    for (const auto& segment : segments)
    {
        primitive2->push_back(INDEX(segment.second));
        primitive2->push_back(INDEX(segment.first));
    }
    osg::Vec4Array* color2 = new osg::Vec4Array();
    color2->push_back(osg::Vec4(1, 1, 0, 1));

#undef INDEX

    osg::Geometry* geometry1 = new osg::Geometry();
    geometry1->setVertexArray(vertices);
    geometry1->setColorArray(color1);
    geometry1->setColorBinding(osg::Geometry::BIND_OVERALL);
    geometry1->addPrimitiveSet(primitive1);

    osg::Geometry* geometry2 = new osg::Geometry();
    geometry2->setVertexArray(vertices);
    geometry2->setColorArray(color2);
    geometry2->setColorBinding(osg::Geometry::BIND_OVERALL);
    geometry2->addPrimitiveSet(primitive2);

    osg::Geode* geode = new osg::Geode();
    geode->addDrawable(geometry1);
    geode->addDrawable(geometry2);

    osg::StateSet* stateSet = geode->getOrCreateStateSet();
    stateSet->setAttributeAndModes(createLitLinesProgram());

    return geode;
}

Volume<unsigned short> createVolume(unsigned int side = 128,
                                    unsigned int pad = 8)
{
    size_t width = side;
    size_t height = side;
    size_t depth = side;

    Volume<unsigned short> volume(width, height, depth);
    volume.set(0);

    const float amplitude = side / 10;
    const float period_x = 100;
    const float period_y = 90;

    for (size_t x = pad; x != width - pad; ++x)
    {
        for (size_t z = pad; z != depth - pad; ++z)
        {
            size_t y =
                std::round(std::sin(x / period_x * 2 * M_PI) *
                           std::sin(z / period_y * 2 * M_PI) * amplitude) +
                amplitude + side / 8;
            volume(x, y, z) = 1;

            size_t y2 =
                std::round(std::sin(x / period_x * 2 * M_PI + M_PI / 2) *
                           std::sin(z / period_y * 2 * M_PI + M_PI / 2) *
                           amplitude) +
                (height - amplitude - 1 - side / 8);
            volume(x, y2, z) = 1;

            for (unsigned i = y + 1; i < y2; ++i)
                volume(x, i, z) = 1;
        }
    }

    return volume;
}
#endif
