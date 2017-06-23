#ifndef LAYERS_UTIL_H
#define LAYERS_UTIL_H

#include "algorithm.h"

osg::Node* createLines(const Volume<char>& volume)
{
    unsigned int width, height, depth;
    std::tie(width, height, depth) = volume.dimensions();

    osg::Vec3Array* vertices = new osg::Vec3Array();
    vertices->reserve((width + 1) * (height + 1) * (depth + 1));
    for (unsigned int x = 0; x < width; ++x)
        for (unsigned int y = 0; y < height; ++y)
            for (unsigned int z = 0; z < depth; ++z)
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

Volume<char> createVolume(unsigned int side = 128, char top = Top,
                          char bottom = Bottom, char middle = Interior)
{
    unsigned int width = side;
    unsigned int height = side;
    unsigned int depth = side;

    Volume<char> volume(width, height, depth);
    volume.set(0);

    const float amplitude = 10;
    const float period_x = 100;
    const float period_y = 90;

    for (unsigned int x = 0; x != width; ++x)
    {
        for (unsigned int z = 0; z != depth; ++z)
        {
            unsigned int y =
                std::round(std::sin(x / period_x * 2 * M_PI) *
                           std::sin(z / period_y * 2 * M_PI) * amplitude) +
                amplitude + 16;
            volume(x, y, z) = top;

            unsigned int y2 =
                std::round(std::sin(x / period_x * 2 * M_PI + M_PI/2) *
                           std::sin(z / period_y * 2 * M_PI + M_PI/2) *
                           amplitude) +
                (height - amplitude - 1 - 16);
            volume(x, y2, z) = bottom;

            for (unsigned i = y + 1; i < y2; ++i)
                volume(x, i, z) = middle;
        }
    }

    return volume;
}
#endif
