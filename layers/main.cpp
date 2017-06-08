#include "Bricks.h"
#include "Volume.h"
#include "programs.h"

#include <osgViewer/Viewer>

#include <cmath>

#include <boost/geometry/algorithms/distance.hpp>
#include <boost/geometry/arithmetic/arithmetic.hpp>
#include <boost/program_options.hpp>

const char Void = 0;
const char Interior = 1;
const char Shell = 2;

Volume<char> createVolume(unsigned int side = 128, char top = 1,
                          char bottom = 1, char middle = 1)
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

Volume<char> annotateBoundaryVoxels(const Volume<char>& volume)
{
    Volume<char> output(volume.width(), volume.height(), volume.depth());
    output.set(0);
    output.apply([&volume](unsigned int x, unsigned int y, unsigned int z,
                          const char&)
                 {
                     if (volume(x, y, z) == 0)
                         return Void;

                     // Border voxels are always boundary
                     if (x == 0 || y == 0 || z == 0 ||
                         x == volume.width() - 1 || y == volume.height() - 1 ||
                         z == volume.depth() - 1)
                         return Shell;

                     if (volume(x - 1, y, z) == 0 || volume(x + 1, y, z) == 0 ||
                         volume(x, y - 1, z) == 0 || volume(x, y + 1, z) == 0 ||
                         volume(x, y, z - 1) == 0 || volume(x, y, z + 1) == 0)
                         return Shell;
                     return Interior;
                 });
    return output;
}

using Coords = Volume<char>::Coords;
using Segment = boost::geometry::model::segment<Coords>;
using Point = boost::geometry::model::point<
    float, 3, boost::geometry::cs::cartesian>;
using SegmentF = boost::geometry::model::segment<Point>;
using Segments = std::vector<Segment>;

Segments findNearestVoxels(const Volume<char>& volume, char from, char to)
{
    Segments segments;
    auto index = volume.createIndex(to);
    volume.visit([index, from, &segments](unsigned int x, unsigned int y,
                                          unsigned int z, const char& v) {
        if (v != from)
            return;
        Coords origin(x, y, z);
        std::vector<Coords> nearest;
        index.query(boost::geometry::index::nearest(origin, 1),
                    std::back_inserter(nearest));
        segments.push_back(Segment(origin, nearest[0]));
    });
    return segments;
}

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
    auto segments = findNearestVoxels(volume, 1, 2);
#define INDEX(c) (c.get<0>() * height * depth + c.get<1>() * depth + c.get<2>())
    for (const auto& segment : segments)
    {
        primitive1->push_back(INDEX(segment.first));
        primitive1->push_back(INDEX(segment.second));
    }
    osg::Vec4Array* color1 = new osg::Vec4Array();
    color1->push_back(osg::Vec4(0, 0.5, 1, 1));

    osg::DrawElementsUInt* primitive2 = new osg::DrawElementsUInt(GL_LINES);
    segments = findNearestVoxels(volume, 2, 1);
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

osg::Node* createLayer(const Volume<char>& volume, float a, float b)
{
    auto segments = findNearestVoxels(volume, 1, 2);
    auto segments2 = findNearestVoxels(volume, 2, 1);
    // We need to reverse the second set of segments to make sure all go in
    // the same direction.
    for (const auto& segment : segments2)
        segments.push_back(Segment(segment.second, segment.first));

    // Creating the index.
    using SegmentIndex =
        boost::geometry::index::rtree<Segment,
                                      boost::geometry::index::rstar<16>>;
    SegmentIndex index(segments.begin(), segments.end());

    unsigned int width, height, depth;
    std::tie(width, height, depth) = volume.dimensions();

    osg::Vec3Array* vertices = new osg::Vec3Array();
    osg::Vec4Array* colors = new osg::Vec4Array();
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
                auto value = volume(x, y, z);
                if (value != 3)
                    continue;
                Segments neighbours;

                Coords coords(x, y, z);
                index.query(boost::geometry::index::nearest(coords, 1000),
                            std::back_inserter(neighbours));
                float average = 0;
                for (const auto& segment : neighbours)
                {
                    using namespace boost::geometry;

                    Point p(segment.first.get<0>(),
                            segment.first.get<1>(),
                            segment.first.get<2>());
                    Point q(segment.second.get<0>(),
                            segment.second.get<1>(),
                            segment.second.get<2>());
                    SegmentF s(p, q);
                    Point c(x, y, z);
                    subtract_point(q, p);
                    float totalLength = length(s);
                    divide_value(q, length(s));
                    subtract_point(c, p);
                    multiply_value(q, dot_product(q, c));
                    add_point(q, p);
                    float t = length(SegmentF(p, q)) / totalLength;
                    average += std::min(1.f, std::max(0.f, t));
                }

                average /= neighbours.size();
                if (average < a || average > b)
                    continue;
#define INDEX(i, j, k) \
    ((i) * (height + 1) * (depth + 1) + (j) * (depth + 1) + (k))
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
#undef INDEX
            }
        }
    }

    osg::Geometry* geometry = new osg::Geometry();
    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
    geometry->addPrimitiveSet(primitive);

    osg::StateSet* stateSet = geometry->getOrCreateStateSet();
    stateSet->setAttributeAndModes(createFlatShadingProgram());

    osg::Geode* geode = new osg::Geode();
    geode->addDrawable(geometry);
    return geode;
}

int main(int argc, char* argv[])
{
    namespace po = boost::program_options;
    // clang-format off
    po::options_description options("Options");
    options.add_options()
        ( "help,h", "Produce help message" )
        ( "version,v", "Show program name/version banner and exit" );

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
        std::cout
            << "Usage: " << argv[0] << " input [options]" << std::endl
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

    Volume<char> volume(filename);

    Volume<char> inOut = annotateBoundaryVoxels(volume);

    Bricks bricks(inOut, Shell);

    osgViewer::Viewer viewer;
    viewer.setSceneData(bricks.node());
    viewer.run();

    

    //osg::ref_ptr<osg::Node> topBottom = createTopBottomLayers(volume);
    //osg::ref_ptr<osg::Node> lines = createLines(volume);
    //osg::ref_ptr<osg::Node> layer = createLayer(volume, 0.45, 0.5);
    //osg::ref_ptr<osg::Group> scene = new osg::Group();
    //scene->addChild(shell);
    //scene->addChild(layer);
    //scene->addChild(lines);

}
