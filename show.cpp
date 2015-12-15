#include <algorithm>
#include <cstdio>

#include <iostream>
#include <iomanip>

int main () {
	std::cout << std::hex;

	do {
		uint8_t buffer[84];
		const auto read = fread(buffer, sizeof(buffer), 1, stdin);

		// EOF?
		if (read == 0) break;

		std::reverse(&buffer[0], &buffer[32]); // BLOCK_HASH -> BLOCK_ID
		std::reverse(&buffer[32], &buffer[64]); // TX_HASH -> TX_ID

		for (int i = 0; i < 32; ++i) std::cout << std::setw(2) << std::setfill('0') << (uint32_t) buffer[i];
		std::cout << ' ';
		for (int i = 32; i < 64; ++i) std::cout << std::setw(2) << std::setfill('0') << (uint32_t) buffer[i];
		std::cout << ' ';
		for (int i = 64; i < 84; ++i) std::cout << std::setw(2) << std::setfill('0') << (uint32_t) buffer[i];
		std::cout << std::endl;

	} while (true);

	return 0;
}
