#include "regiodesics/algorithm.h"
#include "regiodesics/programs.h"
#include "regiodesics/util.h"
#include "regiodesics/version.h"

#include <boost/program_options.hpp>

#include <iostream>

void saveOrientations(const Volume<Point3f>& orientations,
                      const Volume<char>& shell);
void saveDistances(const Volume<float>& heights,
                   const Volume<float>& relatives);

int main(int argc, char* argv[])
{
    size_t averageSize = 1000;
    std::pair<size_t, size_t> cropX{0, std::numeric_limits<size_t>::max()};

    namespace po = boost::program_options;
    // clang-format off
    po::options_description options("Options");
    options.add_options()
        ("help,h", "Produce help message")
        ("version,v", "Show program name/version banner and exit")
        ("average-size,a", po::value<size_t>(&averageSize)->value_name("lines"),
         "Size of k-nearest neighbour query of top to shell lines used to"
         " approximate orientations")
        ("crop-x,x", po::value<std::pair<size_t, size_t>>(&cropX)->
                         value_name("<min>[:<max>]"),
         "Optional crop range for the input volume. Values outside this "
         "range will be cleared to 0 in the input volumes.");

    po::options_description hidden;
    hidden.add_options()
        ("shell,s", po::value<std::string>()->required(), "Shell volume");
    hidden.add_options()
        ("distances,d", po::value<std::string>()->required(), "Relative distance field");
    // clang-format on

    po::options_description allOptions;
    allOptions.add(hidden).add(options);

    po::positional_options_description positional;
    positional.add("shell", 1);
    positional.add("distances", 1);

    po::variables_map vm;

    auto parser = po::command_line_parser(argc, argv);
    po::store(parser.options(allOptions).positional(positional).run(), vm);

    if (vm.count("version"))
    {
        std::cout << "Brain region geodesics" << std::endl;
        return 0;
    }
    if (vm.count("shell") == 0 || vm.count("distances") == 0 ||
        vm.count("help"))
    {
        std::cout << "Usage: " << argv[0]
                  << " shell relative-distances [options]" << std::endl
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
    clearXRange(shell, cropX, '\0');
    Volume<float> relatives(vm["distances"].as<std::string>());

    const auto index = computeSegmentIndex(shell);
    std::cout << "Computing orientations and absolute distances" << std::endl;
    auto result = computeOrientationsAndHeights(shell, averageSize, &index);
    std::cout << "Saving" << std::endl;
    auto& orientations = std::get<0>(result);
    auto& heights = std::get<1>(result);
    saveOrientations(orientations, shell);
    // For correcting the distances from voxel space to volume space we take
    // into account that the volume is isotropic.
    const auto axis = orientations.volumeAxis(0);
    const auto correction = std::sqrt(boost::geometry::dot_product(axis, axis));
    heights.apply([correction](size_t, size_t, size_t, const float x) {
        return x * correction;
    });

    saveDistances(heights, relatives);
}

void saveOrientations(const Volume<Point3f>& orientations,
                      const Volume<char>& shell)
{
    using Point4c = PointTN<char, 4>;
    Volume<Point4c> output(orientations.width(), orientations.height(),
                           orientations.depth(), orientations.metadata());
    // Overriding the original metadata to provide what the consumer tools
    // expect
    auto& metadata = output.metadata();
    metadata["kinds"] = "quaternion domain domain domain";
    metadata["space directions"] = "none " + metadata["space directions"];

    output.apply([&orientations, &shell](size_t i, size_t j, size_t k,
                                         const Point4c&) {

        Point4c p;
        if (shell(i, j, k) == 0)
        {
            p.set<0>(0);
            p.set<1>(0);
            p.set<2>(0);
            p.set<3>(0);
        }
        else
        {
            // This code ignores the possible mirrorings being applied by
            // by the NRRD space directions
            auto orientation = orientations(i, j, k);
            const auto vx = orientations.volumeAxis(0);
            const auto vy = orientations.volumeAxis(1);
            const auto vz = orientations.volumeAxis(2);
            orientation = vx * orientation.get<0>() +
                          vy * orientation.get<1>() +
                          vz * orientation.get<2>();
            osg::Vec3 v(orientation.get<0>(), orientation.get<1>(),
                        orientation.get<2>());
            v.normalize();

            osg::Vec3 up(0, 1, 0);
            osg::Quat q(std::acos(up * v), up ^ v);
            q /= q.length();
            // According to nrrd specs a quaternion is w x y z
            p.set<0>(static_cast<int8_t>(std::round(q[3] * 127)));
            p.set<1>(static_cast<int8_t>(std::round(q[0] * 127)));
            p.set<2>(static_cast<int8_t>(std::round(q[1] * 127)));
            p.set<3>(static_cast<int8_t>(std::round(q[2] * 127)));
        }
        return p;
    });
    output.save("orientations.nrrd");
}

void saveDistances(const Volume<float>& heights, const Volume<float>& relatives)
{
    Volume<float> output(heights.width(), heights.height(), heights.depth(),
                         heights.metadata());
    output.apply(
        [&heights, &relatives](size_t i, size_t j, size_t k, const float&) {
            if (std::isnan(heights(i, j, k)))
                return NAN;
            return heights(i, j, k) * relatives(i, j, k);
        });
    heights.save("heights.nrrd");
    output.save("distances.nrrd");
}
