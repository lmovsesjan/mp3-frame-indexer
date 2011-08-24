#include <iostream>
#include <set>
#include <stdexcept>

#include <cstdlib>

#include "mp3.hpp"

int
main(int argc, char **argv) {
	try {
		if (argc < 3) {
			throw std::runtime_error("not enought params");
		}

		std::set<std::string> flags;
		for (int i = 1; i < argc; ++i) {
			flags.insert(argv[i]);
		}

		bool index_only = (flags.find("-index-only") != flags.end());

		std::string ifile = argv[argc - 2];
		std::string ofile = argv[argc - 1];

		mp3::mp3_t test(ifile);

		test.dump(ofile, index_only);

		return EXIT_SUCCESS;
	}
	catch (const std::exception &e) {
		std::cerr << "error: " << e.what() << std::endl;
	}
	catch (...) {
		std::cerr << "unknown error" << std::endl;
	}

	return EXIT_FAILURE;
}

