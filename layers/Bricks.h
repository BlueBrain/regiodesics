#ifndef LAYERS_BRICKS_H
#define LAYERS_BRICKS_H

#include "Volume.h"

#include <osg/Node>
#include <osg/Array>

class Bricks
{
public:
    Bricks(const Volume<char>& volume, const char value);

    osg::Node* node()
    {
        return _node.get();
    }
    void paintBrick(unsigned int x, unsigned int y, unsigned z,
                    const osg::Vec4& color);

private:
    unsigned int _width;
    unsigned int _height;
    unsigned int _depth;
    osg::ref_ptr<osg::Vec4Array> _colors;
    osg::ref_ptr<osg::Node> _node;
};

#endif
