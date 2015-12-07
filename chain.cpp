#include <array>
#include <cstring>
#include <iostream>
#include <map>

typedef std::array<uint8_t, 32> hash_t;

template <typename T>
void writehexln (const T& wbuf) {
	for (size_t i = 0; i < wbuf.size(); ++i) {
		fprintf(stderr, "%02x", wbuf[i]);
	}
	fprintf(stderr, "\n");
}

struct Link {
	Link* prev = nullptr;

	Link() {}
	Link(Link* prev) : prev(prev) {}
};

auto buildChains(std::map<hash_t, Link>& chains, const std::map<hash_t, hash_t>& mappings, const std::pair<hash_t, hash_t>& mapping) {
	auto prevHash = mapping.second;
	const auto prev = mappings.find(prevHash);

	// a genesis block
	if (prev == mappings.end()) return chains.emplace(hash_t(prev->first), Link(nullptr));

	auto prevChain = chains.find(prev->second);

	// is the previous chain already built?
	if (prevChain != chains.end()) return chains.emplace(hash_t(prev->first), prevChain->second);

	return buildChains(chains, mappings, *prev);
}

int main () {
	uint8_t buffer[64];

	// hash: prevHash
	std::map<hash_t, hash_t> mappings;
	std::map<hash_t, Link> chains;

	auto eof = false;
	while (!eof) {
		const auto read = fread(&buffer[0], 1, 64, stdin);
		eof = static_cast<size_t>(read) < 64;

		hash_t hash, prevHash;

		memcpy(&hash[0], &buffer[0], 32);
		memcpy(&prevHash[0], &buffer[32], 32);

		mappings.emplace(hash, prevHash);
	}

	for (auto& mapping : mappings) {
		buildChains(chains, mappings, mapping);
	}

	std::cerr << "EOF" << std::endl;

	return 0;
}
