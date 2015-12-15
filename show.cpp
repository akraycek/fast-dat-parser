#include <algorithm>
#include <cstdio>

void writehex (char* dst, const char* src, const size_t n) {
	for (size_t i = 0; i < n; ++i) snprintf(&dst[i * 2], 3, "%02x", src[i]);
}

int main () {
	do {
		char buffer[84];
		const auto read = fread(buffer, sizeof(buffer), 1, stdin);

		// EOF?
		if (read == 0) break;

		std::reverse(&buffer[0], &buffer[32]); // BLOCK_HASH -> BLOCK_ID
		std::reverse(&buffer[32], &buffer[64]); // TX_HASH -> TX_ID

		char wbuf[171];
		writehex(&wbuf[0], &buffer[0], 32);
		wbuf[64] = ' ';
		writehex(&wbuf[65], &buffer[32], 32);
		wbuf[129] = ' ';
		writehex(&wbuf[130], &buffer[64], 20);
		wbuf[170] = '\n';

		fwrite(wbuf, sizeof(wbuf), 1, stdout);
	} while (true);

	return 0;
}
