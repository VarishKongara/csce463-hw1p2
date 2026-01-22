#pragma once

#include <unordered_set>
#include <mutex>
#include <string>

struct ThreadSafeSet {
	std::unordered_set<std::string> set;
	std::mutex m;

	bool seen(std::string str) {
		std::lock_guard<std::mutex> lock(m);
		return !set.insert(str).second;
	}
};
