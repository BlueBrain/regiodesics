#include "regiodesics/algorithm.h"
#include "regiodesics/programs.h"
#include "regiodesics/util.h"
#include "regiodesics/version.h"

#include <boost/program_options.hpp>

#include <iostream>

void saveOrientations(const Volume<Point>& gradients, Volume<char>& shell);

int main(int argc, char* argv[])
{
    std::pair<size_t, size_t> cropX{0, std::numeric_limits<size_t>::max()};

    namespace po = boost::program_options;
    // clang-format off
    po::options_description options("Options");
    options.add_options()
        ("help,h", "Produce help message")
        ("version,v", "Show program name/version banner and exit")
        ("crop-x,x", po::value<std::pair<size_t, size_t>>(&cropX)->
                         value_name("<min>[:<max>]"),
         "Optional crop range for the input volume. Values outside this "
         "range will be cleared to 0 in the input volumes.");

    po::options_description hidden;
    hidden.add_options()
        ("shell,s", po::value<std::string>()->required(), "Shell volume");
    // clang-format on

    po::options_description allOptions;
    allOptions.add(hidden).add(options);

    po::positional_options_description positional;
    positional.add("shell", 1);

    po::variables_map vm;

    auto parser = po::command_line_parser(argc, argv);
    po::store(parser.options(allOptions).positional(positional).run(), vm);

    if (vm.count("version"))
    {
        std::cout << "Brain region geodesics" << std::endl;
        return 0;
    }
    if (vm.count("shell") == 0 || vm.count("help"))
    {
        std::cout << "Usage: " << argv[0] << " distance_map shell [options]"
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

    osg::ref_ptr<osg::Group> scene(new osg::Group());

    clearXRange(shell, cropX, '\0');

    auto orientations = computeOrientations(shell, 1000);
    saveOrientations(orientations, shell);
}


void saveOrientations(const Volume<Point>& gradients, Volume<char>& shell)
{
    auto depth = gradients.depth();
    auto height = gradients.height();
    auto width = gradients.width();
    Volume<Point4> orientations(width, height, depth);

    for (size_t i = 0; i != width; ++i)
    {
        for (size_t j = 0; j != height; ++j)
        {
            for (size_t k = 0; k != depth; ++k)
            {
                Point4 p;
                if (shell(i, j, k) != 0)
                {
                    auto gradient = gradients(i, j, k);
                    auto x = gradient.get<0>();
                    auto y = gradient.get<1>();
                    auto z = gradient.get<2>();
                    auto l = std::sqrt(x * x + y * y + z * z);
                    p.set<0>(x / l);
                    p.set<1>(y / l);
                    p.set<2>(z / l);
                    p.set<3>(0);
                }
                else
                {
                    p.set<0>(0);
                    p.set<1>(0);
                    p.set<2>(0);
                    p.set<3>(0);
                }
                orientations(i, j, k) = p;
            }
        }
    }
    orientations.save("orientations.nrrd");
}
