#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include <map>
#include <vector>
#include "utils.hpp"

typedef std::array<uint8_t, 32> hash_t;

struct Block {
	hash_t hash;
	hash_t prevBlockHash;

	Block (const hash_t& hash, const hash_t& prevBlockHash) : hash(hash), prevBlockHash(prevBlockHash) {}
};

struct Chain {
	Block* block;
	Chain* previous;
	size_t depth = 0;

	Chain () {}
	Chain (Block* block) : block(block), previous(nullptr) {
		assert(block != nullptr);
	}
	Chain (Block* block, Chain* previous) : block(block), previous(previous) {
		assert(block != nullptr);
		assert(previous != nullptr);
	}

	template <typename F>
	void every (const F f) const {
		auto link = this;

		while (link != nullptr) {
			if (!f(*link)) break;

			link = (*link).previous;
		}
	}
};

auto& buildChains(std::map<Block*, Chain>& chains, const std::map<hash_t, Block*>& hashMap, Block* root) {
	const auto blockChainIter = chains.find(root);

	// already built this?
	if (blockChainIter != chains.end()) return blockChainIter->second;

	// not yet built, what about the previous block?
	const auto prevBlockIter = hashMap.find(root->prevBlockHash);

	// if prevBlock is unknown, it must be a genesis block
	if (prevBlockIter == hashMap.end()) {
		chains[root] = Chain(root);

		return chains[root];
	}

	// otherwise, recurse to the genesis block, then build the chain on the way back
	const auto prevBlock = prevBlockIter->second;
	auto& prevBlockChain = buildChains(chains, hashMap, prevBlock);

	chains[root] = Chain(root, &prevBlockChain);

	return chains[root];
}

auto findDeepest(std::map<Block*, Chain>& chains) {
	auto bestChain = chains.begin()->second;
	size_t bestDepth = 0;

	for (auto& chainIter : chains) {
		auto&& chain = chainIter.second;

		if (chain.depth == 0) {
			chain.every([&](const Chain& subChain) {
				if (subChain.depth > 0) {
					chain.depth += subChain.depth;
					return false;
				}

				chain.depth++;
				return true;
			});
		}

		if (chain.depth > bestDepth) {
			bestChain = chain;
			bestDepth = chain.depth;
		}
	}

	std::vector<Block> blockchain;
	bestChain.every([&](const Chain& chain) {
		blockchain.push_back(*(chain.block));
		return true;
	});

	return blockchain;
}

int main () {
	std::vector<Block> blocks;

	do {
		uint8_t buffer[64];
		const auto read = fread(&buffer[0], 1, 64, stdin);

		// EOF
		if (static_cast<size_t>(read) < 64) break;

		hash_t hash, prevBlockHash;
		memcpy(&hash[0], &buffer[0], 32);
		memcpy(&prevBlockHash[0], &buffer[32], 32);

		blocks.push_back(Block(hash, prevBlockHash));
	} while (true);

	// build a hash map for easy referencing
	std::map<hash_t, Block*> hashMap;
	for (size_t i = 0; i < blocks.size(); ++i) {
		hashMap[blocks[i].hash] = &blocks[i];
	}

	std::cerr << "EOF" << std::endl;
	std::cerr << "-[] Building chains: " << blocks.size() << " candidates" << std::endl;

	std::map<Block*, Chain> chains;
	for (auto& block : blocks) {
		buildChains(chains, hashMap, &block);
	}

	std::cerr << "-[]-[] Finding deepest chain ..." << std::endl;
	auto bestBlockChain = findDeepest(chains);

	std::cerr << "-[]-[]-[] Found chain with length " << bestBlockChain.size() << std::endl;
	for(auto it = bestBlockChain.rbegin(); it != bestBlockChain.rend(); ++it) {
		std::reverse(&it->hash[0], &it->hash[32]);

		fwritehexln(&it->hash[0], 32, stdout);
	}

	return 0;
}
