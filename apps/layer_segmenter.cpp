#include "Bricks.h"
#include "OffscreenSetup.h"
#include "algorithm.h"
#include "programs.h"
#include "util.h"

#include <osgViewer/Viewer>
#include <osgViewer/Renderer>
#include <osgGA/GUIEventAdapter>

#include <cmath>

#include <boost/program_options.hpp>
#include <boost/progress.hpp>
#include <boost/signals2/signal.hpp>


osg::Vec4 TopColor(1, 1, 0, 1);
osg::Vec4 BottomColor(1, 0, 0, 1);

class Painter : public osgGA::GUIEventHandler
{
public:
    typedef boost::signals2::signal<void()> DoneSignal;

    DoneSignal done;

    class PostDrawCallback : public osg::Camera::DrawCallback
    {
    public:
        PostDrawCallback(Painter* painter)
            : _painter(painter)
            , _image(new osg::Image)
        {
        }

        void operator()(osg::RenderInfo& renderInfo) const
        {
            if (!_painter->_paint)
                return;
            _painter->_paint = false;

            auto& state = *renderInfo.getState();
            auto camera = renderInfo.getCurrentCamera();
            auto renderer = (osgViewer::Renderer*)camera->getRenderer();
            auto renderStage = renderer->getSceneView(0)->getRenderStage();
            auto fbo = renderStage->getFrameBufferObject();
            if (!fbo)
                return;

            fbo->apply(state);
            glReadBuffer(GL_COLOR_ATTACHMENT1);

            auto viewport = camera->getViewport();
            const int posx = std::max(0, _painter->_x - 20);
            const int posy = std::max(0, _painter->_y - 20);
            const int width = std::min(40, int(viewport->width()) - posx);
            const int height = std::min(40, int(viewport->height()) - posy);

            _image->readPixels(posx, posy, width, height, GL_RGB, GL_FLOAT);

            auto& bricks = _painter->_bricks;
            auto& volume = _painter->_volume;
            for (int i = 0; i != width; ++i)
            {
                for (int j = 0; j != height; ++j)
                {
                    float* coords = (float*)(_image->data(i, j));
                    // There's no easy way to specify a separate clear color
                    // per buffer, so the coord buffer get's cleared to the
                    // default color (2, 0.2, 0.4). In general we will assume
                    // that if red < 1, this pixel has the clear color values,
                    // so it's empty.
                    if (coords[0] < 1)
                        continue;

                    size_t x = coords[0] - 1;
                    size_t y = coords[1] - 1;
                    size_t z = coords[2] - 1;
                    switch(_painter->_state)
                    {
                    case State::paintTop:
                        bricks.paintBrick(x, y, z, TopColor);
                        volume(x, y, z) = Top;
                        break;
                    case State::paintBottom:
                        bricks.paintBrick(x, y, z, BottomColor);
                        volume(x, y, z) = Bottom;
                        break;
                    case State::erase:
                        bricks.resetBrick(x, y, z);
                        volume(x, y, z) = Shell;
                        break;
                    default:;
                    }
                }
            }
        }

    private:
        Painter* _painter;
        osg::ref_ptr<osg::Image> _image;
    };

    Painter(Volume<char>& volume, Bricks& bricks, osg::Camera* camera)
        : _volume(volume)
        , _bricks(bricks)
        , _state(State::off)
        , _paint(false)
    {
        camera->setPostDrawCallback(new PostDrawCallback(this));
    }

    /** Handle events, return true if handled, false otherwise. */
    virtual bool handle(const osgGA::GUIEventAdapter& ea,
                        osgGA::GUIActionAdapter& aa,
                        osg::Object*, osg::NodeVisitor*)
    {
        if (_state == State::done)
            return false;

        switch (ea.getEventType())
        {
        case osgGA::GUIEventAdapter::PUSH:
            if (ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
            {
                auto modifier = ea.getModKeyMask();
                if (modifier & osgGA::GUIEventAdapter::MODKEY_SHIFT &&
                    modifier & osgGA::GUIEventAdapter::MODKEY_CTRL)
                {
                    _state = State::erase;
                }
                else if (modifier & osgGA::GUIEventAdapter::MODKEY_SHIFT)
                    _state = State::paintTop;
                else if (modifier & osgGA::GUIEventAdapter::MODKEY_CTRL)
                    _state = State::paintBottom;

                if (_state != State::off)
                    _paintVoxels(aa, ea.getX(), ea.getY());
            }
            break;
        case osgGA::GUIEventAdapter::RELEASE:
            if (ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
                _state = State::off;
            break;
        case osgGA::GUIEventAdapter::DRAG:
            if (_state != State::off)
            {
                _paintVoxels(aa, ea.getX(), ea.getY());
                return true;
            }
            break;
        case osgGA::GUIEventAdapter::KEYDOWN:
            if (ea.getKey() == osgGA::GUIEventAdapter::KEY_Return)
            {
                _state = State::done;
                done();
                return true;
            }
            else if (ea.getKey() == 's')
            {
                _volume.save("shell.nrrd");
                return true;
            }
            break;
        default:
            ;
        }
        return false;
    }

private:
    void _paintVoxels(osgGA::GUIActionAdapter& aa, int x, int y)
    {
        _x = x;
        _y = y;
        _paint = true;
        aa.requestRedraw();
    }

    enum class State {off, erase, paintTop, paintBottom, done};

    Volume<char>& _volume;
    Bricks& _bricks;
    State _state;
    int _x;
    int _y;
    bool _paint;
};

namespace std
{
istream& operator>>(istream& in, std::pair<unsigned int, unsigned int>& pair)
{
    string s;
    in >> s;
    const auto pos = s.find(':');
    pair.first = boost::lexical_cast<unsigned int>(s.substr(0, pos));
    if (pos == string::npos)
        pair.second = std::numeric_limits<unsigned int>::max();
    else
        pair.second = boost::lexical_cast<unsigned int>(s.substr(pos + 1));
    return in;
}
}

int main(int argc, char* argv[])
{
    std::pair<unsigned int, unsigned int> cropX{
        0, std::numeric_limits<unsigned int>::max()};

    namespace po = boost::program_options;
    // clang-format off
    po::options_description options("Options");
    options.add_options()
        ("help,h", "Produce help message")
        ("version,v", "Show program name/version banner and exit")
        ("shell,s", po::value<std::string>(), "Load a saved painted shell")
        ("thickness,t", po::value<std::vector<float>>()->multitoken(),
         "Layer thicknesses (absolute or relative). Must contain at least"
         " two values")
        ("crop-x,x", po::value<std::pair<unsigned int, unsigned int>>(&cropX)->
         value_name("<min>[:<max>]"),
         "Optional crop range for the input volume. Values outside this "
         "range will be cleared to 0 in the input volumes.");

    po::options_description hidden;
    hidden.add_options()
        ("input,i", po::value<std::string>()->required(), "Input volume");
    // clang-format on

    po::options_description allOptions;
    allOptions.add(hidden).add(options);

    po::positional_options_description positional;
    positional.add("input", 1);

    po::variables_map vm;

    auto parser = po::command_line_parser(argc, argv);
    po::store(parser.options(allOptions).positional(positional).run(), vm);

    if (vm.count("help") || vm.count("input") == 0)
    {
        std::cout << "Usage: " << argv[0] << " input [options]" << std::endl
                  << options << std::endl;
        return 0;
    }
    if (vm.count("version"))
    {
        std::cout << "Layer labelling 0.0.0" << std::endl;
        return 0;
    }

    try
    {
        po::notify(vm);
    }
    catch (const po::error& e)
    {
        std::cerr << "Command line parse error: " << e.what() << std::endl
                  << options << std::endl;
        return -1;
    }

    std::string filename = vm["input"].as<std::string>();
    std::string shellFile;
    if (vm.count("shell"))
        shellFile = vm["shell"].as<std::string>();

    std::vector<float> splitPoints{0.0792355711331, 0.150750191634,
                                   0.320282553918, 0.411345964278,
                                   0.663563684133};
    if (vm.count("thickness"))
    {
        const auto& thicknesses = vm["thickness"].as<std::vector<float>>();

        if (thicknesses.size() < 2)
        {
            std::cerr << "The thickness option must include at least two values"
                      << std::endl;
            return -1;
        }
        splitPoints.clear();
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
    }

    Volume<unsigned short> inVolume(filename);
    std::cout << "Input volume dimensions: " << inVolume.width() << " "
              << inVolume.height() << " " << inVolume.depth() << std::endl;

    Volume<char> shell = shellFile.empty() ? annotateBoundaryVoxels(inVolume)
                                           : Volume<char>(shellFile);

    if (inVolume.dimensions() != shell.dimensions())
    {
        std::cerr << "Invalid shell volume" << std::endl;
        return -1;
    }
    if (cropX.first > 0 || cropX.second < inVolume.width())
    {
        for (size_t i = 0; i < cropX.first; ++i)
            for (size_t j = 0; j < inVolume.height(); ++j)
                for (size_t k = 0; k < inVolume.depth(); ++k)
                {
                    inVolume(i, j , k) = 0;
                    shell(i, j , k) = 0;
                }
        for (size_t i =
                 std::min(size_t(cropX.second), inVolume.width() - 1) + 1;
             i < inVolume.width(); ++i)
        {
            for (size_t j = 0; j < inVolume.height(); ++j)
            {
                for (size_t k = 0; k < inVolume.depth(); ++k)
                {
                    inVolume(i, j , k) = 0;
                    shell(i, j , k) = 0;
                }
            }
        }
    }

    Bricks::ColorMap colors;
    colors[Top] = TopColor;
    colors[Bottom] = BottomColor;
    Bricks bricks(shell, {Shell, Top, Bottom}, colors);

    osg::ref_ptr<osg::Group> scene(new osg::Group());
    scene->addChild(bricks.node());

    osgViewer::Viewer viewer;
    viewer.setSceneData(scene);
    viewer.realize();

    osgViewer::Viewer::Cameras cameras;
    viewer.getCameras(cameras);
    OffscreenSetup indirect(cameras[0]);

    osg::ref_ptr<Painter> painter = new Painter(shell, bricks, cameras[0]);
    viewer.addEventHandler(painter);

    painter->done.connect(
        [scene, &shell, splitPoints]{

            scene->removeChild(0, scene->getNumChildren());

            auto distances = computeRelativeDistanceField(shell, 1000);
            distances.save("distanceMap.nrrd");
            auto layers = annotateLayers(distances, splitPoints);
            layers.save("layers.nrrd");

            Bricks::ColorMap layerColors;
            layerColors[1] = osg::Vec4(1.0, 0, 0, 1);
            layerColors[2] = osg::Vec4(1.0, 0.5, 0, 1);
            layerColors[3] = osg::Vec4(1.0, 1.0, 0, 1);
            layerColors[4] = osg::Vec4(0.5, 1.0, 0.5, 1);
            layerColors[5] = osg::Vec4(0, 1.0, 1.0, 1);
            layerColors[6] = osg::Vec4(0, 0.5, 0.5, 1);
            Bricks layerBricks(layers, {1, 2, 3, 4, 5, 6}, layerColors);
            scene->addChild(layerBricks.node());
        });

    viewer.run();
}
