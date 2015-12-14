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
	uint32_t bits;

	Block (const hash_t& hash, const hash_t& prevBlockHash, const uint32_t bits) : hash(hash), prevBlockHash(prevBlockHash), bits(bits) {}
};

struct Chain {
	Block* block;
	Chain* previous;
	size_t work = 0;

	Chain () {}
	Chain (Block* block, Chain* previous) : block(block), previous(previous) {
		assert(block != nullptr);
		assert(previous != nullptr);
	}

	size_t determineAggregateWork () {
		this->work = this->block->bits;

		if (this->previous != nullptr) {
			this->work += this->previous->determineAggregateWork();
		}

		return this->work;
	}

	template <typename F>
	void forEach (const F f) const {
		auto link = this;

		while (link != nullptr) {
			f(*link);

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
		chains[root] = Chain(root, nullptr);

		return chains[root];
	}

	// otherwise, recurse down to the genesis block, building the chain on the way back
	const auto prevBlock = prevBlockIter->second;
	auto& prevBlockChain = buildChains(chains, hashMap, prevBlock);

	chains[root] = Chain(root, &prevBlockChain);

	return chains[root];
}

// TODO: change this to findBest using most-work over best
auto findBest(std::map<Block*, Chain>& chains) {
	auto bestChain = chains.begin()->second;
	size_t mostWork = 0;

	for (auto& chainIter : chains) {
		auto&& chain = chainIter.second;
		auto work = chain.determineAggregateWork();

		if (work > mostWork) {
			bestChain = chain;
			mostWork = work;
		}
	}

	std::vector<Block> blockchain;
	bestChain.forEach([&](const Chain& chain) {
		blockchain.push_back(*(chain.block));
	});

	return blockchain;
}

int main () {
	std::vector<Block> blocks;

	do {
		uint8_t buffer[80];
		const auto read = fread(&buffer[0], 1, sizeof(buffer), stdin);

		// EOF
		if (static_cast<size_t>(read) < sizeof(buffer)) break;

		hash_t hash, prevBlockHash;
		uint32_t bits;

		memcpy(&hash[0], &buffer[0], 32);
		memcpy(&prevBlockHash[0], &buffer[32 + 4], 32);
		memcpy(&bits, &buffer[104], 4);

		blocks.push_back(Block(hash, prevBlockHash, bits));
	} while (true);

	// build a hash map for easy referencing
	std::map<hash_t, Block*> hashMap;
	for (size_t i = 0; i < blocks.size(); ++i) {
		hashMap[blocks[i].hash] = &blocks[i];
	}

	// build all possible chains
	std::map<Block*, Chain> chains;
	for (auto& block : blocks) {
		buildChains(chains, hashMap, &block);
	}

	// find the best
	auto bestBlockChain = findBest(chains);

	// print it out
	for(auto it = bestBlockChain.rbegin(); it != bestBlockChain.rend(); ++it) {
		std::reverse(&it->hash[0], &it->hash[32]);

		fwritehexln(&it->hash[0], 32, stdout);
	}

	return 0;
}
