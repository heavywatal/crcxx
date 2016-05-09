// -*- mode: c++; coding: utf-8 -*-
/*! @file simulation.cpp
    @brief Inplementation of Simulation class
*/
#include "simulation.hpp"

#include <cstdlib>

#include <cxxwtils/iostr.hpp>
#include <cxxwtils/getopt.hpp>
#include <cxxwtils/prandom.hpp>
#include <cxxwtils/os.hpp>
#include <cxxwtils/gz.hpp>

#include "tissue.hpp"
#include "cell.hpp"

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////
// functions

boost::program_options::options_description& Simulation::opt_description() {HERE;
    namespace po = boost::program_options;
    static po::options_description description("Simulation");
    description.add_options()
        ("help,h", po::value<bool>()->default_value(false)->implicit_value(true), "produce help")
        ("verbose,v", po::value<bool>(&VERBOSE)
            ->default_value(VERBOSE)->implicit_value(true), "verbose output")
        ("test", po::value<int>()->default_value(0)->implicit_value(1))
        ("out_dir,o", po::value<std::string>()->default_value(OUT_DIR.string()))
        ("seed", po::value<unsigned int>(&SEED)->default_value(SEED))
        ("nsam", po::value<size_t>(&NSAM)->default_value(NSAM))
        ("howmany", po::value<size_t>(&HOWMANY)->default_value(HOWMANY))
    ;
    return description;
}

//! Unit test for each class
inline void test() {HERE;
    Cell::unit_test();
    Tissue::unit_test();
}

Simulation::Simulation(int argc, char* argv[]) {HERE;
    std::vector<std::string> arguments(argv, argv + argc);
    COMMAND_ARGS = wtl::str_join(arguments, " ") ;
    OUT_DIR = wtl::strftime("tumopp_%Y%m%d_%H%M_") + std::to_string(::getpid());

    namespace po = boost::program_options;
    po::options_description description;
    description.add(opt_description());
    description.add(Cell::opt_description());
    description.add(Tissue::opt_description());

    po::positional_options_description positional;
    positional.add("nsam", 1);
    positional.add("howmany", 1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
              options(description).
              positional(positional).run(), vm);
    po::notify(vm);

    if (vm["help"].as<bool>()) {
        description.print(std::cout);
        std::exit(EXIT_SUCCESS);
    }
    wtl::sfmt().seed(SEED);
    CONFIG_STRING = wtl::flags_into_string(vm);
    if (VERBOSE) {
        std::cerr << wtl::iso8601datetime() << std::endl;
        std::cerr << CONFIG_STRING << std::endl;
    }
    switch (vm["test"].as<int>()) {
      case 0:
        break;
      case 1:
        test();
        std::exit(EXIT_SUCCESS);
      default:
        std::abort();
    }
    if (NSAM > vm["max"].as<size_t>()) {
        std::cout.precision(1);
        std::cout
            << "\nERROR: NSAM=" << NSAM
            << " is larger than tumor size "
            << std::fixed << vm["max"].as<size_t>() << std::endl;
        std::abort();
    }
    OUT_DIR = fs::path(vm["out_dir"].as<std::string>());
    OUT_DIR = fs::system_complete(OUT_DIR);
}

void Simulation::run() const {HERE;
    std::cout << COMMAND_ARGS << "\n" << SEED << "\n";
    Tissue tissue;
    while (!tissue.grow()) {
        tissue = Tissue();
    }

    if (Tissue::DIMENSIONS() == 3) {
        for (size_t i=0; i<HOWMANY; ++i) {
            tissue.write_segsites(std::cout, tissue.sample_section(NSAM));
        }
    } else {
        for (size_t i=0; i<HOWMANY; ++i) {
            tissue.write_segsites(std::cout, tissue.sample_random(NSAM));
        }
    }

    if (VERBOSE) {
        derr("mkdir && cd to " << OUT_DIR << std::endl);
        fs::create_directory(OUT_DIR);
        wtl::cd(OUT_DIR.string());
        wtl::Fout{"program_options.conf"} << CONFIG_STRING;
        wtl::gzip{wtl::Fout{"population.tsv.gz"}}
            << tissue.header() << tissue.specimens();
        wtl::gzip{wtl::Fout{"snapshots.tsv.gz"}}
            << tissue.header() << tissue.snapshots();
        std::cerr << wtl::iso8601datetime() << std::endl;
    }
}
