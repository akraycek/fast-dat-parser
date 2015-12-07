#pragma once

#include "sha1.h"
#include "sha256.h"

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
