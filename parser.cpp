#include <cstdio>
#include <cstring>
#include <functional>
#include <iostream>

#include "block.hpp"
#include "hash.hpp"
#include "slice.hpp"
#include "threadpool.h"
#include "utils.hpp"

// BLOCK_HEADER
void dumpHeaders (Slice<uint8_t> data) {
	fwrite(data.begin, 80, 1, stdout);
// 	fwritehexln(data.begin, 80, stdout);
}

// BLOCK_HASH | TX_HASH | SCRIPT_HASH
void dumpScriptShas (Slice<uint8_t> data) {
	const auto block = Block(data.take(80), data.drop(80));
	uint8_t wbuf[32 + 32 + 20] = {0};

	hash256(&wbuf[0], block.header);

	auto transactions = block.transactions();
	while (!transactions.empty()) {
		const auto& transaction = transactions.front();

		hash256(&wbuf[32], transaction.data);

		for (const auto& input : transaction.inputs) {
			sha1(&wbuf[64], input.script);

			// no locking, 84 bytes < PIPE_BUF (4096 bytes)
			fwrite(wbuf, sizeof(wbuf), 1, stdout);
// 			fwritehexln(wbuf, sizeof(wbuf), stdout);
		}

		for (const auto& output : transaction.outputs) {
			sha1(&wbuf[64], output.script);

			// no locking, 84 bytes < PIPE_BUF (4096 bytes)
			fwrite(wbuf, sizeof(wbuf), 1, stdout);
// 			fwritehexln(wbuf, sizeof(wbuf), stdout);
		}

		transactions.popFront();
	}
}

typedef void(*processFunction_t)(Slice<uint8_t>);
processFunction_t FUNCTIONS[] = {
	&dumpHeaders,
	&dumpScriptShas
};

static size_t bufferSize = 100 * 1024 * 1024;
static size_t nThreads = 1;
static size_t functionIndex = 0;

auto parseArg (char* argv) {
	if (sscanf(argv, "-b=%lu", &bufferSize) == 1) return true;
	if (sscanf(argv, "-n=%lu", &nThreads) == 1) return true;
	if (sscanf(argv, "-f=%lu", &functionIndex) == 1) return true;
	return false;
}

int main (int argc, char** argv) {
	for (auto i = 1; i < argc; ++i) {
		if (parseArg(argv[i])) continue;

		return 1;
	}

	const auto delegate = FUNCTIONS[functionIndex];
	const auto backbuffer = std::unique_ptr<uint8_t>(new uint8_t[bufferSize]);

	auto iobuffer = Slice<uint8_t>(backbuffer.get(), backbuffer.get() + bufferSize / 2);
	auto buffer = Slice<uint8_t>(backbuffer.get() + bufferSize / 2, backbuffer.get() + bufferSize);
	ThreadPool<std::function<void(void)>> pool(nThreads);

// 	std::cerr << "Initialized buffer (" << bufferSize << " bytes)" << std::endl;
// 	std::cerr << "Initialized " << nThreads << " threads in the thread pool" << std::endl;

	uint64_t remainder = 0;
	bool dirty = false;
	size_t count = 0;

	while (true) {
		const auto rdbuf = iobuffer.drop(remainder);
		const auto read = fread(rdbuf.begin, 1, rdbuf.length(), stdin);
		const auto eof = static_cast<size_t>(read) < rdbuf.length();

		// wait for all workers before overwrite
		pool.wait();

		// swap the buffers
		std::swap(buffer, iobuffer);

		auto slice = buffer.take(remainder + read);
		std::cerr << "-- " << count << " Blocks (processing " << slice.length() / 1024 << " KiB)" << std::endl;

		while (slice.length() >= 88) {
			// skip bad data (e.g bitcoind zero pre-allocations)
			if (slice.peek<uint32_t>() != 0xd9b4bef9) {
				slice.popFrontN(4);
				dirty = true;

				continue;
			}

			const auto header = Block(slice.drop(8).take(80));

			// skip bad data cont. (only verify if it was dirty)
			if (dirty && !header.verify()) {
				slice.popFrontN(4);

				continue;
			}

			// do we have enough data?
			const auto length = slice.drop(4).peek<uint32_t>();
			const auto needed = 8 + length;
			if (needed > slice.length()) break;

			// process the data
			const auto data = slice.drop(8).take(needed - 8);
			pool.push([data, delegate]() { delegate(data); });
			count++;

			slice.popFrontN(needed);
		}

		if (eof) break;

		// assign remainder to front of iobuffer (rdbuf is offset to avoid overwrite on rawRead)
		remainder = slice.length();
		memcpy(&iobuffer[0], &slice[0], remainder);
	}

	return 0;
}