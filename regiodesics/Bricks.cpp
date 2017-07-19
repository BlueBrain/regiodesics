#include "Bricks.h"

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Program>
#include <osg/Shader>

#include <boost/progress.hpp>

inline void addShader(osg::Program* program, osg::Shader::Type type,
                      const char* source)
{
    osg::Shader* shader = new osg::Shader(type);
    shader->setShaderSource(source);
    program->addShader(shader);
}

inline osg::Program* createBrickShadingProgram()
{
    const char* vertexSource = R"(
    #version 120
    varying vec3 coordsIn;
    void main()
    {
        gl_FrontColor = gl_Color;
        gl_Position = gl_ModelViewMatrix * gl_Vertex;
        coordsIn = gl_Vertex.xyz;
    })";

    const char* geomSource = R"(
    #version 120
    #extension GL_EXT_geometry_shader4 : enable
    #extension GL_EXT_gpu_shader4 : enable

    varying in vec3 coordsIn[];

    varying out vec3 normal;
    varying out vec3 eye;
    flat varying out vec3 coords;

    void main()
    {
        vec4 x = vec4(gl_NormalMatrix * vec3(1, 0, 0), 0);
        vec4 y = vec4(gl_NormalMatrix * vec3(0, 1, 0), 0);
        vec4 z = vec4(gl_NormalMatrix * vec3(0, 0, 1), 0);
        vec4 p = gl_PositionIn[0];

        vec4 px = gl_ProjectionMatrix * x;
        vec4 py = gl_ProjectionMatrix * y;
        vec4 pz = gl_ProjectionMatrix * z;
        vec4 pp = gl_ProjectionMatrix * p;

        gl_FrontColor = gl_FrontColorIn[0];
        coords = coordsIn[0];

        vec4 corners[8];
        corners[0] = p;
        corners[1] = p + x;
        corners[2] = p + x + z;
        corners[3] = p + z;
        corners[4] = p + y;
        corners[5] = p + y + x;
        corners[6] = p + y + x + z;
        corners[7] = p + y + z;

        vec4 pcorners[8];
        pcorners[0] = pp;
        pcorners[1] = pp + px;
        pcorners[2] = pp + px + pz;
        pcorners[3] = pp + pz;
        pcorners[4] = pp + py;
        pcorners[5] = pp + py + px;
        pcorners[6] = pp + py + px + pz;
        pcorners[7] = pp + py + pz;

#define VERTEX(i) \
        gl_Position = pcorners[i]; eye = -corners[i].xyz; EmitVertex();

        normal = -y.xyz;
        VERTEX(0)
        VERTEX(1)
        VERTEX(3)
        VERTEX(2)
        EndPrimitive();

        normal = y.xyz;
        VERTEX(4)
        VERTEX(7)
        VERTEX(5)
        VERTEX(6)
        EndPrimitive();

        normal = -z.xyz;
        VERTEX(0)
        VERTEX(1)
        VERTEX(4)
        VERTEX(5)
        EndPrimitive();

        normal = z.xyz;
        VERTEX(2)
        VERTEX(3)
        VERTEX(6)
        VERTEX(7)
        EndPrimitive();

        normal = -x.xyz;
        VERTEX(0)
        VERTEX(3)
        VERTEX(4)
        VERTEX(7)
        EndPrimitive();

        normal = x.xyz;
        VERTEX(1)
        VERTEX(2)
        VERTEX(5)
        VERTEX(6)
        EndPrimitive();
    })";

    const char* const fragSource = R"(
    #version 120
    #extension GL_EXT_geometry_shader4 : enable

    varying vec3 normal;
    varying vec3 eye;
    flat varying vec3 coords;

    void main()
    {
        vec3 norm_eye = normalize(eye);
        float lambert = normalize(normal).z * 0.9;
        vec4 color = gl_Color * lambert + vec4(0.05, 0.05, 0.05, 0.0);

        vec3 r = reflect(-norm_eye, normal);
        color.rgb += vec3(0.05, 0.05, 0.05) *
                     pow(max(dot(r, norm_eye) , 0.0), 16.0);

        gl_FragData[0] = color;
        // All coordinates are increased by 1 so 0, 0, 0 can be used for empty
        // pixels.
        gl_FragData[1].xyz = coords + vec3(1, 1, 1);
    })";

    osg::Program* program = new osg::Program();
    addShader(program, osg::Shader::VERTEX, vertexSource);
    addShader(program, osg::Shader::GEOMETRY, geomSource);
    addShader(program, osg::Shader::FRAGMENT, fragSource);
    program->setParameter(GL_GEOMETRY_VERTICES_OUT_EXT, 36);
    program->setParameter(GL_GEOMETRY_INPUT_TYPE_EXT, GL_POINTS);
    program->setParameter(GL_GEOMETRY_OUTPUT_TYPE_EXT, GL_TRIANGLE_STRIP);

    return program;
}

Bricks::Bricks(const Volume<char>& volume, const std::vector<char>& values,
               const ColorMap& colors)
{
    std::tie(_width, _height, _depth) = volume.dimensions();

    osg::Vec3Array* vertices = new osg::Vec3Array();
    _colors = new osg::Vec4Array();
    size_t size = _width * _height * _depth;
    vertices->reserve(size);
    _colors->reserve(size);

    boost::progress_display progress(size);
    for (size_t x = 0; x < _width; ++x)
        for (size_t y = 0; y < _height; ++y)
            for (size_t z = 0; z < _depth; ++z, ++progress)
            {
                auto i =
                    std::find(values.begin(), values.end(), volume(x, y, z));
                if (i == values.end())
                    continue;

                vertices->push_back(osg::Vec3(x, y, z));
                _coords.push_back(std::make_tuple(x, y, z));

                auto entry = colors.find(*i);
                osg::Vec4 color;
                if (entry != colors.end())
                    color = entry->second;
                else
                {
                    const float t = y / float(_height);
                    color = osg::Vec4(x / float(_width), t, 1 - t, 1);
                }
                _colors->push_back(color);
            }

    osg::DrawArrays* primitive =
        new osg::DrawArrays(GL_POINTS, 0, vertices->size());

    osg::Geometry* geometry = new osg::Geometry();
    geometry->setUseDisplayList(false);
    geometry->setVertexArray(vertices);
    geometry->setColorArray(_colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->addPrimitiveSet(primitive);

    osg::StateSet* stateSet = geometry->getOrCreateStateSet();
    stateSet->setAttributeAndModes(createBrickShadingProgram());

    osg::Geode* geode = new osg::Geode();
    geode->addDrawable(geometry);
    _node = geode;
}

void Bricks::resetBrick(size_t x, size_t y, size_t z)
{
    const float t = y / float(_height);
    paintBrick(x, y, z, osg::Vec4(x / float(_width), t, 1 - t, 1));
}

void Bricks::paintBrick(size_t x, size_t y, size_t z, const osg::Vec4& color)
{
    if (x == 0 && y == 0 && z == 0)
        abort();

    auto i = std::lower_bound(_coords.begin(), _coords.end(),
                              std::make_tuple(x, y, z));
    assert(i != _coords.end());
    (*_colors)[i - _coords.begin()] = color;
    _colors->dirty();
}
