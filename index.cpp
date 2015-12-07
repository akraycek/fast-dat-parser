#include <cstdio>
#include <cstring>
#include <functional>
#include <iostream>

#include "sha1.h"
#include "sha256.h"
#include "slice.hpp"
#include "block.hpp"
#include "threadpool.h"

void sha1 (uint8_t* dest, Slice<uint8_t> src) {
	CSHA1 hash;
	hash.Write(src.begin, src.length());
	hash.Finalize(dest);
}

void hash256 (uint8_t* dest, Slice<uint8_t> src) {
	CSHA256 hash;
	hash.Write(src.begin, src.length());
	hash.Finalize(dest);
	hash.Reset();
	hash.Write(dest, 32);
	hash.Finalize(dest);
}

void writehexln (Slice<uint8_t> wbuf) {
	for (size_t i = 0; i < wbuf.length(); ++i) {
		fprintf(stderr, "%02x", wbuf[i]);
	}
	fprintf(stderr, "\n");
}

void processBlocks (Slice<uint8_t> data) {
	auto block = Block(data.take(80), data.drop(80));
	uint8_t wbuf[32 + 32] = {0};

	hash256(&wbuf[0], block.headerData);
	memcpy(&wbuf[32], &block.header()->prevHash[0], 32);

	fwrite(wbuf, sizeof(wbuf), 1, stdout);
}

void processScriptShas (Slice<uint8_t> data) {
	const auto block = Block(data.take(80), data.drop(80));
	uint8_t wbuf[32 + 32 + 20] = {0};

	hash256(&wbuf[0], block.headerData);

	auto transactions = block.transactions();
	while (!transactions.empty()) {
		const auto& transaction = transactions.front();

		hash256(&wbuf[32], transaction.data);

		for (const auto& input : transaction.inputs) {
			sha1(&wbuf[64], input.script);

			// no locking, 84 bytes < PIPE_BUF (4096 bytes)
			fwrite(wbuf, sizeof(wbuf), 1, stdout);
// 			writehexln(Slice<uint8_t>(wbuf, wbuf + 84));
		}

		for (const auto& output : transaction.outputs) {
			sha1(&wbuf[64], output.script);

			// no locking, 84 bytes < PIPE_BUF (4096 bytes)
			fwrite(wbuf, sizeof(wbuf), 1, stdout);
// 			writehexln(Slice<uint8_t>(wbuf, wbuf + 84));
		}

		transactions.popFront();
	}
}

static size_t bufferSize = 100 * 1024 * 1024;
static size_t nThreads = 2;
static unsigned blocksOnly = 0;

auto parseArg (char* argv) {
	if (sscanf(argv, "-b=%lu", &bufferSize) == 1) return true;
	if (sscanf(argv, "-bo=%u", &blocksOnly) == 1) return true;
	if (sscanf(argv, "-n=%lu", &nThreads) == 1) return true;
	return false;
}

int main (int argc, char** argv) {
	for (auto i = 1; i < argc; ++i) {
		if (parseArg(argv[i])) continue;

		std::cerr << "index -b=BUFFER_SIZE -n=N_THREADS" << std::endl;
		return 1;
	}

	const auto delegate = blocksOnly ? &processBlocks : &processScriptShas;
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

		while (slice.length() >= 8) {
			// skip mis-allocated zero pre-allocations
			if (slice.peek<uint32_t>() != 0xd9b4bef9) {
				size_t zeros = 0;
				while (slice.length() > 0 && slice.peek<uint8_t>() == 0x00) {
					slice.popFront();
					++zeros;
				}

				if (zeros == 0) {
					std::cerr << "--!! Unexpected data " << static_cast<uint32_t>(slice.peek<uint8_t>()) << std::endl;

					pool.join();
					writehexln(slice);
					return 1;
				}

				std::cerr << "--() Gap: " << zeros << " bytes (" << slice.length() << " bytes remaining)" << std::endl;
				continue;
			}

			const auto length = slice.drop(4).peek<uint32_t>();

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
