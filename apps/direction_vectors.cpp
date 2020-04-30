#include "regiodesics/algorithm.h"
#include "regiodesics/version.h"
#include <boost/program_options.hpp>

#include <iostream>

int main(int argc, char* argv[])
{
    size_t averageSize = 1000;

    namespace po = boost::program_options;
    po::options_description options("Options");
    // clang-format off
    options.add_options()
        ("help,h", "Produce this help message.")
        ("version,v", "Show program description and exit.")
        ("shells,s", po::value<std::string>()->required(), "Shells volume, that is, a 3D array"
        " of type char (int8), where bottom voxels are labeled with 3 and top voxels are labeled"
        " with 4.")
        ("average-size,a", po::value<size_t>(&averageSize)->value_name("lines")->default_value(averageSize),
         "Size of k-nearest neighbour query of top to bottom lines used to"
         " approximate direction vectors.")
         ("output-path,o", po::value<std::string>()->default_value("direction_vectors.nrrd"),
        "File path of the 3D unit vector field to save.");
    // clang-format on

    po::variables_map vm;

    auto parser = po::command_line_parser(argc, argv);
    po::store(parser.options(options).run(), vm);

    if (vm.count("version"))
    {
        std::cout << "direction_vectors (Regiodesics "
                  << regiodesics::Version::getString() << ")" << std::endl;
    }
    if (vm.count("shells") == 0 || vm.count("help"))
    {
        // clang-format off
        std::cout << "Generate and save brain region direction vectors.\n\n"
            "Unit direction vectors are obtained as an 'average' of the directions\n"
            "of straight line segments joining the bottom shell to the top shell.\n"
            "These segments join every voxel of the bottom shell to one of its \n"
            "closest voxels in the top shell, and vice versa. The direction vector\n"
            "of a voxel is the sum of the unit direction vectors of its first\n"
            "`average-size` closest segments divided by its Euclidean norm.\n\n"
            "Usage: " << argv[0] << " --shells SHELLS \n\n"
            "where SHELLS is the path to a nrrd file containing the definitions\n"
            "of a bottom and a top shell. SHELLS holds a 3D array of type char\n (int8)"\
            " where bottom voxels are labeled with 3 and top voxels\n"\
            "are labeled with 4.\n\n";
        std::cout << options << std::endl;
        return 0;
        // clang-format on
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

    const Volume<char> shell(vm["shells"].as<std::string>());

    std::cout << "Computing direction vectors.\n";
    const auto index = computeSegmentIndex(shell);
    auto result = computeOrientationsAndHeights(shell, averageSize, &index);
    const auto& direction_vectors = std::get<0>(result);

    std::cout << "Saving.\n";
    direction_vectors.save(vm["output-path"].as<std::string>());
}
