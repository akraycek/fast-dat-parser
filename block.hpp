#pragma once

#include <cstdint>
#include <vector>

#include "slice.hpp"

uint64_t readVI (Slice<uint8_t>& data) {
	const auto i = data.front();
	data.popFront();

	if (i < 253) return static_cast<uint64_t>(i);
	if (i < 254) return static_cast<uint64_t>(data.read<uint16_t>());
	if (i < 255) return static_cast<uint64_t>(data.read<uint32_t>());
	return data.read<uint64_t>();
}

struct Transaction {
	struct Input {
		Slice<uint8_t> hash;
		uint32_t vout;
		Slice<uint8_t> script;
		uint32_t sequence;

		Input () {}
		Input (
			Slice<uint8_t> hash,
			uint32_t vout,
			Slice<uint8_t> script,
			uint32_t sequence
		) :
			hash(hash),
			vout(vout),
			script(script),
			sequence(sequence) {}
	};

	struct Output {
		Slice<uint8_t> script;
		uint64_t value;

		Output () {}
		Output (
			Slice<uint8_t> script,
			uint64_t value
		) :
			script(script),
			value(value) {}
	};

	Slice<uint8_t> data;
	uint32_t version;

	std::vector<Input> inputs;
	std::vector<Output> outputs;

	uint32_t locktime;

	Transaction () {}
	Transaction (
		Slice<uint8_t> data,
		uint32_t version,
		std::vector<Input> inputs,
		std::vector<Output> outputs,
		uint32_t locktime
	) :
		data(data),
		version(version),
		inputs(inputs),
		outputs(outputs),
		locktime(locktime) {}
};

namespace {
	template <typename T>
	auto readSlice (Slice<T>& data, uint64_t length) {
		auto slice = data.take(length);
		data.popFrontN(length);
		return slice;
	}
}

struct TransactionRange {
private:
	uint64_t n;
	Slice<uint8_t> data;

	Transaction current;
	bool lazy = true;

	auto readTransaction () {
		const auto source = this->data;
		const auto version = this->data.read<uint32_t>();

		auto inputs = this->current.inputs;
		inputs.resize(readVI(this->data));

		for (auto &input : inputs) {
			auto hash = readSlice(this->data, 32);
			auto vout = this->data.read<uint32_t>();
			auto script = readSlice(this->data, readVI(this->data));
			auto sequence = this->data.read<uint32_t>();

			input = Transaction::Input(hash, vout, script, sequence);
		}

		auto outputs = this->current.outputs;
		outputs.resize(readVI(this->data));

		for (auto &output : outputs) {
			auto value = this->data.read<uint64_t>();
			auto script = readSlice(this->data, readVI(this->data));

			output = Transaction::Output(script, value);
		}

		auto locktime = this->data.read<uint32_t>();
		auto tdata = source.take(source.length() - this->data.length());

		this->current = Transaction(tdata, version, std::move(inputs), std::move(outputs), locktime);
		this->lazy = false;
	}

public:
	TransactionRange(size_t n, Slice<uint8_t> data) : n(n), data(data) {}

	void popFront () {
		this->n--;

		if (this->n > 0) {
			this->readTransaction();
		}
	}

	auto empty () const { return this->n == 0; }
	auto length () const { return this->n; }
	auto& front () {
		if (this->lazy) {
			this->readTransaction();
		}

		return this->current;
	}
};

struct Block {
	struct Header {
		uint32_t version;
		uint8_t prevHash[32];
		uint8_t merkleRoot[32];
		uint32_t timestamp;
		uint32_t bits;
		uint32_t nonce;
	};

	Slice<uint8_t> headerData;
	Slice<uint8_t> data;

	Block(Slice<uint8_t> headerData, Slice<uint8_t> data) : headerData(headerData), data(data) {}

	auto header () {
		return reinterpret_cast<Header*>(&this->headerData.begin);
	}

	auto transactions () const {
		auto tdata = this->data;
		auto n = readVI(tdata);

		return TransactionRange(n, tdata);
	}
};
