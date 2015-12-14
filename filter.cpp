#include <array>
#include <cstring>
#include <iostream>
#include <set>
#include <vector>

#include "utils.hpp"

typedef std::array<uint8_t, 32> hash_t;

static size_t chunkLength = 84;
static size_t hashOffset = 0;
static std::string filterFileName;

auto parseArg (char* argv) {
	if (sscanf(argv, "-l=%lu", &chunkLength) == 1) return true;
	if (sscanf(argv, "-o=%lu", &hashOffset) == 1) return true;
	filterFileName = std::string(argv);

	return false;
}

int main (int argc, char** argv) {
	for (auto i = 1; i < argc; ++i) {
		if (parseArg(argv[i])) continue;

		return 1;
	}

	std::set<hash_t> filter;

	auto filterFile = fopen(filterFileName.c_str(), "r");
	if (filterFile == nullptr) return 1;

	// build the filter set from a dump
	do {
		uint8_t buffer[32];
		const auto read = fread(&buffer[0], sizeof(buffer), 1, filterFile);

		// EOF?
		if (read == 0) break;

		hash_t hash;
		memcpy(&hash[0], &buffer[0], 32);

		filter.emplace(hash);
	} while (true);

	std::cerr << "Initialized set (" << filter.size() << " entries)" << std::endl;

	// now filter stdin
	std::vector<uint8_t> chunkBuffer(chunkLength);

	do {
		const auto read = fread(&chunkBuffer[0], chunkLength, 1, stdin);

		// EOF?
		if (read == 0) break;

		hash_t hash;
		memcpy(&hash[0], &chunkBuffer[hashOffset], chunkLength);

		// not in the filter? skip it
		if (filter.find(hash) == filter.end()) {
			std::cerr << "Filtered ";
			fwritehexln(&hash[0], 32, stderr);

			continue;
		}

		fwrite(&chunkBuffer[0], chunkLength, 1, stdout);
	} while (true);

	return 0;
}
