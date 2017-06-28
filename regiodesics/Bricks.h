#ifndef LAYERS_BRICKS_H
#define LAYERS_BRICKS_H

#include "Volume.h"

#include <osg/Array>
#include <osg/Node>

#include <map>



class Bricks
{
public:
    using ColorMap = std::map<char, osg::Vec4>;

    Bricks(const Volume<char>& volume, const std::vector<char>& values,
           const ColorMap& colors = ColorMap());

    osg::Node* node()
    {
        return _node.get();
    }
    void paintBrick(size_t x, size_t y, size_t z,
                    const osg::Vec4& color);

    void resetBrick(size_t x, size_t y, size_t z);

private:
    size_t _width;
    size_t _height;
    size_t _depth;
    osg::ref_ptr<osg::Vec4Array> _colors;
    osg::ref_ptr<osg::Node> _node;
    std::vector<std::tuple<size_t, size_t, size_t>> _coords;
};

#endif
