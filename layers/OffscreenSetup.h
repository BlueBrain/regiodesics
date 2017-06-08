#ifndef LAYERS_OFFSCREENSETUP_H
#define LAYERS_OFFSCREENSETUP_H

#include <osg/GL>

#include <osg/Camera>
#include <osg/FrameBufferObject>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Program>
#include <osg/Shader>
#include <osg/Texture2D>

class OffscreenSetup
{
public:
    OffscreenSetup(osg::Camera* camera)
        : _width(camera->getViewport()->width())
        , _height(camera->getViewport()->height())
        , _screenQuad(new osg::Geode)
        , _orthoCamera(new osg::Camera)
        , _camera(camera)
    {
        _screenQuad->addDrawable(
            osg::createTexturedQuadGeometry(osg::Vec3(-1, -1, 0),
                                            osg::Vec3(2, 0, 0),
                                            osg::Vec3(0, 2, 0), 0, 0, 1, 1));

        // Creating the depth and color textures were the original camera
        // will render into.
        _textures[0] = new osg::Texture2D;
        _textures[0]->setTextureSize(_width, _height);
        _textures[0]->setInternalFormat(GL_DEPTH_COMPONENT24);
        _textures[0]->setSourceFormat(GL_DEPTH_COMPONENT);
        _textures[0]->setSourceType(GL_FLOAT);
        _textures[0]->setFilter(osg::Texture2D::MIN_FILTER,
                                osg::Texture2D::LINEAR);
        _textures[0]->setFilter(osg::Texture2D::MAG_FILTER,
                                osg::Texture2D::LINEAR);

        _textures[1] = new osg::Texture2D;
        _textures[1]->setTextureSize(_width, _height);
        _textures[1]->setInternalFormat(GL_RGBA);

        // Creating the screen quad for the final render pass.
        osg::StateSet* stateset = _screenQuad->getOrCreateStateSet();
        stateset->setTextureAttribute(0, _textures[1]);
        stateset->addUniform(new osg::Uniform("color", 0));

        osg::Program* program =new osg::Program();
        program->addShader(new osg::Shader(osg::Shader::VERTEX, R"(
            varying vec2 uv;
            void main()
            {
                gl_Position = gl_Vertex;
                uv = gl_MultiTexCoord0.st;
            }
            )"));
        program->addShader(new osg::Shader(osg::Shader::FRAGMENT, R"(
            uniform sampler2D color;
            varying vec2 uv;
            void main()
            {
                gl_FragColor = texture2D(color, uv);
            }
            )"));

        stateset->setAttributeAndModes(program, osg::StateAttribute::ON);

        // Set orthographic camera to render the quad
        _orthoCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
        _orthoCamera->setViewport(0, 0, _width, _height);
        _orthoCamera->setRenderOrder(osg::Camera::POST_RENDER);
        _orthoCamera->addChild(_screenQuad);

        // Setting up the camera original camera to do render to texture
        _camera->attach(osg::Camera::DEPTH_BUFFER, _textures[0].get());
        _camera->attach(osg::Camera::COLOR_BUFFER, _textures[1].get());
        _camera->setClearMask(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        _camera->setRenderTargetImplementation(
            osg::Camera::FRAME_BUFFER_OBJECT);
        _camera->setRenderOrder(osg::Camera::PRE_RENDER);

        _camera->addChild(_orthoCamera.get());
    }

private:
    int _width;
    int _height;

    osg::ref_ptr<osg::Geode> _screenQuad;
    osg::ref_ptr<osg::Camera> _orthoCamera;
    osg::ref_ptr<osg::Camera> _camera;
    osg::ref_ptr<osg::Texture2D> _textures[3];
};

#endif
