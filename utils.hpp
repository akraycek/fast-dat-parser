#pragma once

template <typename T, typename F>
void fwritehexln (const T* rbuf, size_t n, F f) {
	uint8_t wbuf[(n * 2) + 1];

	for (size_t i = 0; i < n; ++i) {
		snprintf((char*) &wbuf[i * 2], 3, "%02x", rbuf[i]);
	}

	wbuf[n * 2] = '\n';
	fwrite(wbuf, sizeof(wbuf), 1, f);
}
