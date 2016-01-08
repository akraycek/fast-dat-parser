#pragma once

#include <array>
#include <atomic>
#include <condition_variable>
#include <list>
#include <mutex>
#include <thread>

template <typename F>
struct ThreadPool {
private:
	std::vector<std::thread> threads;
	std::vector<F> vector;

	std::atomic_bool joined;
	std::atomic_int running;
	std::condition_variable idle_signal;
	std::condition_variable go_signal;
	std::mutex vector_mutex;

	void run() {
		++this->running;

		while (!this->joined) {
			F job;

			{
				std::unique_lock<std::mutex> vector_lock(this->vector_mutex);

				if (this->vector.empty()) {
					--this->running;

					this->idle_signal.notify_one();

					// releases vector_lock, waits for job or join event
					this->go_signal.wait(vector_lock, [this] {
						// aquires vector_lock
						return !this->vector.empty() || this->joined;
					});
					// vector_lock is re-aquired

					if (this->joined) break;
					++this->running;
				}

				job = vector.back();
				this->vector.pop_back();
			}

			// execute
			job();
		}
	}

public:
	ThreadPool(size_t n) : running(0), joined(false) {
		this->threads.reserve(n);

		for (size_t i = 0; i < n; ++i) {
			this->threads.emplace_back(std::thread([this] { this->run(); }));
		}
	}

	~ThreadPool() {
		if (this->joined) return;
		this->join();
	}

	void push(const F f) {
		assert(!this->joined);
		std::lock_guard<std::mutex> vector_guard(this->vector_mutex);

		this->vector.push_back(f);
		this->go_signal.notify_one();
	}

	void join() {
		assert(!this->joined);
		this->wait();
		this->joined = true;

		// wake any waiting threads
		this->go_signal.notify_all();

		// join joinable threads
		for (auto &x : this->threads) {
			if (!x.joinable()) continue;

			x.join();
		}
	}

	void wait() {
		assert(!this->joined);
		if (this->running == 0) return;

		std::unique_lock<std::mutex> vector_lock(this->vector_mutex);

		this->idle_signal.wait(vector_lock, [this]() {
			return this->running == 0;
		});
	}
};
