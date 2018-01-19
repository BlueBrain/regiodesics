#include "regiodesics/Bricks.h"
#include "regiodesics/util.h"

#include <boost/geometry/algorithms/distance.hpp>
#include <boost/program_options.hpp>

#include <osg/Geometry>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

osg::Vec4 TopColor(1, 1, 0, 1);
osg::Vec4 BottomColor(0, 0.5, 1, 1);

using Point4c = PointTN<char, 4>;

std::vector<Point3f> findIsosurfaceSamples(const Volume<float>& distances,
                                           const float isovalue)
{
    std::vector<Point3f> points;
    const auto width = distances.width();
    const auto height = distances.height();
    const auto depth = distances.depth();
    assert(width > 1 && height > 1 && depth > 1);
    for (size_t i = 0; i != width - 1; ++i)
    {
        for (size_t j = 0; j != height - 1; ++j)
        {
            for (size_t k = 0; k != depth - 1; ++k)
            {
                float corners[] = {
                    distances(i, j, k),         distances(i, j, k + 1),
                    distances(i, j + 1, k),     distances(i, j + 1, k + 1),
                    distances(i + 1, j, k),     distances(i + 1, j, k + 1),
                    distances(i + 1, j + 1, k), distances(i + 1, j + 1, k + 1)};
                // Simple first approximation, we will just find out if this
                // voxel intersects the isosurface and add a seed point at
                // its center.
                if (std::isnan(corners[0]))
                    continue;
                const bool above = corners[0] > isovalue;
                for (int n = 1; n < 8; ++n)
                {
                    if (std::isnan(corners[n]))
                        break;

                    if ((corners[n] > isovalue) != above)
                    {
                        points.push_back(Point3f(i + 0.5, j + 0.5, k + 0.5));
                        break;
                    }
                }
            }
        }
    }
    return points;
}

Volume<Point4f> convertOrientations(const Volume<char>& shell,
                                    const Volume<Point4c>& orientations)
{
    auto depth = orientations.depth();
    auto height = orientations.height();
    auto width = orientations.width();
    auto metadata = orientations.metadata();
    Volume<Point4f> output(width, height, depth, orientations.metadata());
    output.apply(
        [&orientations, &shell](size_t i, size_t j, size_t k, const Point4f&) {
            if (shell(i, j, k) == 0)
                return Point4f(0, 0, 0);

            const auto o = orientations(i, j, k);
            Point4f p;
            // NRRD quaternions are stored w x y z
            p.set<0>(o.get<1>() / 127.f);
            p.set<1>(o.get<2>() / 127.f);
            p.set<2>(o.get<3>() / 127.f);
            p.set<3>(o.get<0>() / 127.f);
            return p;
        });
    return output;
}

osg::Node* createDistanceLines(const std::vector<Point3f>& points,
                               const Volume<Point4f>& orientations,
                               const Volume<float>& heights,
                               const Volume<float>& distances)
{
    osg::Vec3Array* vertices = new osg::Vec3Array;
    osg::Vec4Array* colors = new osg::Vec4Array;
    const auto vx = orientations.volumeAxis(0);
    const auto vy = orientations.volumeAxis(1);
    const auto vz = orientations.volumeAxis(2);
    osg::Matrix transform(vx.get<0>(), vy.get<0>(), vz.get<0>(), 0, vx.get<1>(),
                          vy.get<1>(), vz.get<1>(), 0, vx.get<2>(), vy.get<2>(),
                          vz.get<2>(), 0, 0, 0, 0, 1);
    for (auto point : points)
    {
        const auto o = orientations(point);
        osg::Quat orientation(o.get<0>(), o.get<1>(), o.get<2>(), o.get<3>());
        osg::Vec3 direction = orientation * osg::Vec3(0, 1, 0);
        const auto height = heights(point);
        const auto distance = distances(point);
        if (std::isnan(height) || std::isnan(distance))
            continue;
        const osg::Vec3 p =
            osg::Vec3(point.get<0>(), point.get<1>(), point.get<2>()) *
            transform;
        vertices->push_back(p);
        osg::Vec4 topColor(1, 0.5, 0, 1);
        osg::Vec4 bottomColor(0.5, 0, 1, 1);
        colors->push_back(osg::Vec4(bottomColor));
        vertices->push_back(p - direction * distance);
        colors->push_back(osg::Vec4(bottomColor));
        vertices->push_back(p);
        colors->push_back(osg::Vec4(topColor));
        vertices->push_back(p + direction * (height - distance));
        colors->push_back(osg::Vec4(topColor));
    }
    osg::DrawArrays* primitive =
        new osg::DrawArrays(GL_LINES, 0, vertices->size());

    osg::Geometry* geometry = new osg::Geometry();
    geometry->addPrimitiveSet(primitive);
    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);

    osg::Geode* geode = new osg::Geode();
    geode->addDrawable(geometry);

    osg::StateSet* stateSet = geode->getOrCreateStateSet();
    stateSet->setAttributeAndModes(createLitLinesProgram());

    return geode;
}

int main(int argc, char* argv[])
{
    std::pair<size_t, size_t> cropX{0, std::numeric_limits<size_t>::max()};
    std::pair<size_t, size_t> cropY{0, std::numeric_limits<size_t>::max()};
    std::pair<size_t, size_t> cropZ{0, std::numeric_limits<size_t>::max()};
    float seedHeight = 0.5;

    namespace po = boost::program_options;
    // clang-format off
    po::options_description options("Options");
    options.add_options()
        ("help,h", "Produce help message")
        ("version,v", "Show program name/version banner and exit")
        ("crop-x,x", po::value<std::pair<size_t, size_t>>(&cropX)->
                         value_name("<min>[:<max>]"),
         "Optional crop range in the x axis.")
        ("crop-y,y", po::value<std::pair<size_t, size_t>>(&cropY)->
                         value_name("<min>[:<max>]"),
         "Optional crop range in the y axis.")
        ("crop-z,z", po::value<std::pair<size_t, size_t>>(&cropZ)->
                         value_name("<min>[:<max>]"),
         "Optional crop range in the z axis.")
        ("roi,r", po::value<std::string>(), "Region of interest")
        ("seed-height,s", po::value<float>(&seedHeight)->value_name("[0..1]"),
         "Relative height of the plane where field lines will be seeded");

    po::options_description hidden;
    hidden.add_options()
        ("shell", po::value<std::string>()->required(), "Shell volume");
    hidden.add_options()
        ("orientations", po::value<std::string>()->required(),
         "Orientation field");
    hidden.add_options()
        ("heights", po::value<std::string>()->required(),
         "Top to bottom Distance field");
    hidden.add_options()
        ("distances", po::value<std::string>()->required(),
         "Voxel to bottom Distance field");
    hidden.add_options()
        ("relatives", po::value<std::string>()->required(),
         "Relative distance to bottom field");
    // clang-format on

    po::options_description allOptions;
    allOptions.add(hidden).add(options);

    po::positional_options_description positional;
    positional.add("shell", 1);
    positional.add("orientations", 1);
    positional.add("heights", 1);
    positional.add("distances", 1);
    positional.add("relatives", 1);

    po::variables_map vm;

    auto parser = po::command_line_parser(argc, argv);
    po::store(parser.options(allOptions).positional(positional).run(), vm);

    if (vm.count("version"))
    {
        std::cout << "Brain region geodesics" << std::endl;
        return 0;
    }
    if (vm.count("shell") == 0 || vm.count("orientations") == 0 ||
        vm.count("heights") == 0 || vm.count("distances") == 0 ||
        vm.count("relatives") == 0 || vm.count("help"))
    {
        std::cout << "Usage: " << argv[0]
                  << " shell orientations heights distances relatives [options]"
                  << std::endl
                  << options << std::endl;
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

    Volume<char> shell(vm["shell"].as<std::string>());
    Volume<Point4c> origOrientations(vm["orientations"].as<std::string>());
    Volume<float> heights(vm["heights"].as<std::string>());
    Volume<float> distances(vm["distances"].as<std::string>());
    Volume<float> relatives(vm["relatives"].as<std::string>());
    clearOutsideXRange(shell, cropX, '\0');
    clearOutsideXRange(relatives, cropX, NAN);
    clearOutsideXRange(heights, cropX, NAN);
    clearOutsideYRange(shell, cropY, '\0');
    clearOutsideYRange(heights, cropY, NAN);
    clearOutsideYRange(relatives, cropY, NAN);
    clearOutsideZRange(shell, cropZ, '\0');
    clearOutsideZRange(heights, cropZ, NAN);
    clearOutsideZRange(relatives, cropZ, NAN);

    if (vm.count("roi"))
    {
        Volume<unsigned short> roi(vm["roi"].as<std::string>());
        roi.visit([&shell, &relatives, &heights](size_t i, size_t j, size_t k,
                                                 const unsigned short x) {
            if (x == 0)
            {
                shell(i, j, k) = '\0';
                relatives(i, j, k) = NAN;
                heights(i, j, k) = NAN;
            }
        });
    }

    auto orientations = convertOrientations(shell, origOrientations);

    osg::Group* scene = new osg::Group();

    const std::vector<Point3f> grid =
        findIsosurfaceSamples(relatives, seedHeight);
    scene->addChild(
        createDistanceLines(grid, orientations, heights, distances));

    Bricks::ColorMap colors;
    colors[Top] = TopColor;
    colors[Bottom] = BottomColor;
    Bricks bricks(shell, {Top, Bottom}, colors, true);
    scene->addChild(bricks.node());

    osgViewer::Viewer viewer;
    viewer.setSceneData(scene);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.run();
}
