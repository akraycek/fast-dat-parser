#include <cstdio>
#include <cstring>
#include <functional>
#include <iostream>

#include "block.hpp"
#include "hash.hpp"
#include "slice.hpp"
#include "threadpool.h"
#include "utils.hpp"

void processBlocks (Slice<uint8_t> data) {
	auto block = Block(data.take(80), data.drop(80));
	uint8_t wbuf[32 + 32];

	hash256(&wbuf[0], block.header);
	memcpy(&wbuf[32], &block.header[4], 32);

// 	fwrite(wbuf, sizeof(wbuf), 1, stdout);
	fwritehexln(wbuf, sizeof(wbuf), stderr);
}

void processScriptShas (Slice<uint8_t> data) {
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
// 			fwritehexln(wbuf, sizeof(wbuf), stderr);
		}

		for (const auto& output : transaction.outputs) {
			sha1(&wbuf[64], output.script);

			// no locking, 84 bytes < PIPE_BUF (4096 bytes)
			fwrite(wbuf, sizeof(wbuf), 1, stdout);
// 			fwritehexln(wbuf, sizeof(wbuf), stderr);
		}

		transactions.popFront();
	}
}

static size_t bufferSize = 100 * 1024 * 1024;
static size_t nThreads = 1;
static size_t function = 0;

auto parseArg (char* argv) {
	if (sscanf(argv, "-b=%lu", &bufferSize) == 1) return true;
	if (sscanf(argv, "-n=%lu", &nThreads) == 1) return true;
	if (sscanf(argv, "-f=%lu", &function) == 1) return true;
	return false;
}

int main (int argc, char** argv) {
	for (auto i = 1; i < argc; ++i) {
		if (parseArg(argv[i])) continue;

		return 1;
	}

	const auto delegate = function == 0 ? &processBlocks : &processScriptShas;
	const auto backbuffer = std::unique_ptr<uint8_t>(new uint8_t[bufferSize]);

	auto iobuffer = Slice<uint8_t>(backbuffer.get(), backbuffer.get() + bufferSize / 2);
	auto buffer = Slice<uint8_t>(backbuffer.get() + bufferSize / 2, backbuffer.get() + bufferSize);
	std::cerr << "Initialized buffer (" << bufferSize << " bytes)" << std::endl;

	uint64_t remainder = 0;

	ThreadPool<std::function<void(void)>> pool(nThreads);
	std::cerr << "Initialized " << nThreads << " threads in the thread pool" << std::endl;

	while (true) {
		const auto rdbuf = iobuffer.drop(remainder);
		std::cerr << "> Reading: " << rdbuf.length() / 1024 << "/" << iobuffer.length() / 1024 << " KiB" << std::endl;

		const auto read = fread(rdbuf.begin, 1, rdbuf.length(), stdin);
		const auto eof = static_cast<size_t>(read) < rdbuf.length();
		std::cerr << "< Read: " << read / 1024 << "/" << rdbuf.length() / 1024 << " KiB" << std::endl;

		// wait for all workers to finish before overwriting memory
		pool.wait();

		// swap the buffers
		std::swap(buffer, iobuffer);

		auto slice = buffer.take(remainder + read);
		std::cerr << "-- Processing " << slice.length() / 1024 << " KiB" << std::endl;

		while (slice.length() >= 80) {
			// skip bad data (includes bitcoind zero pre-allocations)
			if (slice.peek<uint32_t>() != 0xd9b4bef9) {
				while (slice.length() >= 4 && slice.peek<uint32_t>() != 0xd9b4bef9) {
					slice.popFrontN(4);
				}

				continue;
			}

			const auto length = slice.drop(4).peek<uint32_t>();
			const auto header = Block(slice.drop(8).take(80));

			if (!header.verify()) {
				slice.popFrontN(80);
				std::cerr << "Skipping invalid block" << std::endl;
				break;
			}

			// do we have enough data?
			const auto needed = 8 + length;
			if (needed > slice.length()) break;

			const auto data = slice.drop(8).take(needed - 8);
			pool.push([data, delegate]() { delegate(data); });

			slice.popFrontN(needed);
		}

		if (eof) break;

		// assign remainder to front of iobuffer (rdbuf is offset to avoid overwrite on rawRead)
		remainder = slice.length();

		for (size_t i = 0; i < remainder; ++i) {
			iobuffer[i] = slice[i];
		}
	}

	std::cerr << "EOF" << std::endl;

	return 0;
}
