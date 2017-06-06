#include "Volume.h"

#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Program>
#include <osg/Shader>

#include <osgViewer/Viewer>

#include <cmath>


const char *vertexSource = R"(
#version 120
void main()
{
    float p = min(1, gl_Vertex.y / 64);
    gl_FrontColor = vec4(p, p, 1 - p, 1);
    gl_Position = gl_ModelViewMatrix * gl_Vertex;
}
)";

const char *geomSource = R"(
#version 120
#extension GL_EXT_geometry_shader4 : enable
#extension GL_EXT_gpu_shader4 : enable

varying out vec3 normal;
varying out vec3 eye;

void main()
{
    normal = cross(gl_PositionIn[1].xyz - gl_PositionIn[0].xyz,
                   gl_PositionIn[2].xyz - gl_PositionIn[0].xyz);
    if (normal.z < 0)
        normal = -normal;

    for (int i = 0; i < 3; ++i)
    {
        gl_FrontColor = gl_FrontColorIn[i];
        gl_Position = gl_ProjectionMatrix * gl_PositionIn[i];
        eye = -gl_PositionIn[i].xyz;
        EmitVertex();
    }
    EndPrimitive();
}
)";

const char *fragSource = R"(
#version 120
#extension GL_EXT_geometry_shader4 : enable

varying vec3 normal;
varying vec3 eye;

void main()
{
    vec3 norm_eye = normalize(eye);
    float lambert = normalize(normal).z * 0.9;
    vec4 color = gl_Color * lambert + vec4(0.05, 0.05, 0.05, 0.0);

    vec3 r = reflect(-norm_eye, normal);
    color.rgb += vec3(0.05, 0.05, 0.05) * pow(max(dot(r, norm_eye) , 0.0), 16.0);

    gl_FragColor = color;
}
)";

void addShader(osg::Program* program, osg::Shader::Type type, const char* source)
{
    osg::Shader* shader = new osg::Shader(type);
    shader->setShaderSource(source);
    program->addShader(shader);
}

osg::Node* createScene()
{
    unsigned int side = 64;
    unsigned int width = side;
    unsigned int height = side;
    unsigned int depth = side;

    Volume<char> volume(width, height, depth);
    volume.set(0);

    const float amplitude = 5;
    const float period_x = 40;
    const float period_y = 30;

    for (unsigned int x = 0; x != volume.width(); ++x)
    {
        for (unsigned int z = 0; z != volume.depth(); ++z)
        {
            unsigned int y = std::round(
                std::sin(x / period_x * 2 * M_PI) *
                std::sin(z / period_y * 2 * M_PI) * amplitude) + amplitude + 16;
            volume(x, y, z) = 1;

            unsigned int y2 = std::round(
                std::sin(x / period_x * 2 * M_PI + M_PI) *
                std::sin(z / period_y * 2 * M_PI + M_PI) * amplitude) +
                (height - amplitude - 1 - 16);
            volume(x, y2, z) = 2;
        }
    }

    osg::Vec3Array *vertices = new osg::Vec3Array();

    vertices->reserve((width + 1) * (height + 1) * (depth + 1));
    for (unsigned int x = 0; x <= width; ++x)
        for (unsigned int y = 0; y <= height; ++y)
            for (unsigned int z = 0; z <= depth; ++z)
                vertices->push_back(osg::Vec3(x, y, z));

    osg::DrawElementsUInt* primitive = new osg::DrawElementsUInt(GL_TRIANGLES);
    for (unsigned int x = 0; x < width; ++x)
    {
        for (unsigned int y = 0; y < height; ++y)
        {
            for (unsigned int z = 0; z < depth; ++z)
            {
                if (volume(x, y, z) == 0)
                    continue;

#define INDEX(i, j, k) ((i) * (height + 1) * (depth + 1) + (j) * (depth + 1) + (k))
                primitive->push_back(INDEX(x, y, z));
                primitive->push_back(INDEX(x + 1, y, z));
                primitive->push_back(INDEX(x, y, z + 1));
                primitive->push_back(INDEX(x + 1, y, z));
                primitive->push_back(INDEX(x + 1, y, z + 1));
                primitive->push_back(INDEX(x, y, z + 1));

                primitive->push_back(INDEX(x, y + 1, z));
                primitive->push_back(INDEX(x, y + 1, z + 1));
                primitive->push_back(INDEX(x + 1, y + 1, z));
                primitive->push_back(INDEX(x + 1, y + 1, z));
                primitive->push_back(INDEX(x, y + 1, z + 1));
                primitive->push_back(INDEX(x + 1, y + 1, z + 1));

                primitive->push_back(INDEX(x, y, z));
                primitive->push_back(INDEX(x + 1, y, z));
                primitive->push_back(INDEX(x, y + 1, z));
                primitive->push_back(INDEX(x + 1, y, z));
                primitive->push_back(INDEX(x + 1, y + 1, z));
                primitive->push_back(INDEX(x, y + 1, z));

                primitive->push_back(INDEX(x, y, z + 1));
                primitive->push_back(INDEX(x, y + 1, z + 1));
                primitive->push_back(INDEX(x + 1, y, z + 1));
                primitive->push_back(INDEX(x + 1, y, z + 1));
                primitive->push_back(INDEX(x, y + 1, z + 1));
                primitive->push_back(INDEX(x + 1, y + 1, z + 1));

                primitive->push_back(INDEX(x, y, z));
                primitive->push_back(INDEX(x, y, z + 1));
                primitive->push_back(INDEX(x, y + 1, z));
                primitive->push_back(INDEX(x, y + 1, z));
                primitive->push_back(INDEX(x, y, z + 1));
                primitive->push_back(INDEX(x, y + 1, z + 1));

                primitive->push_back(INDEX(x + 1, y, z));
                primitive->push_back(INDEX(x + 1, y, z + 1));
                primitive->push_back(INDEX(x + 1, y + 1, z));
                primitive->push_back(INDEX(x + 1, y + 1, z));
                primitive->push_back(INDEX(x + 1, y, z + 1));
                primitive->push_back(INDEX(x + 1, y + 1, z + 1));
            }
        }
    }

    osg::Geometry* geometry = new osg::Geometry();
    geometry->setVertexArray(vertices);
    geometry->addPrimitiveSet(primitive);

    osg::Program* program = new osg::Program();
    addShader(program, osg::Shader::VERTEX, vertexSource);
    addShader(program, osg::Shader::GEOMETRY, geomSource);
    addShader(program, osg::Shader::FRAGMENT, fragSource);
    program->setParameter(GL_GEOMETRY_VERTICES_OUT_EXT, 3);
    program->setParameter(GL_GEOMETRY_INPUT_TYPE_EXT, GL_TRIANGLES);
    program->setParameter(GL_GEOMETRY_OUTPUT_TYPE_EXT, GL_TRIANGLE_STRIP);

    osg::StateSet* stateSet = geometry->getOrCreateStateSet();
    stateSet->setAttributeAndModes(program);

    osg::Geode* geode = new osg::Geode();
    geode->addDrawable(geometry);
    return geode;
}

int main()
{
    osgViewer::Viewer viewer;
    osg::ref_ptr<osg::Node> scene = createScene();
    viewer.setSceneData(scene);
    viewer.run();
}
