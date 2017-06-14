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
    void paintBrick(unsigned int x, unsigned int y, unsigned z,
                    const osg::Vec4& color);

    void resetBrick(unsigned int x, unsigned int y, unsigned z);

private:
    unsigned int _width;
    unsigned int _height;
    unsigned int _depth;
    osg::ref_ptr<osg::Vec4Array> _colors;
    osg::ref_ptr<osg::Node> _node;
    std::vector<std::tuple<unsigned int, unsigned int, unsigned int>> _coords;
};

#endif
