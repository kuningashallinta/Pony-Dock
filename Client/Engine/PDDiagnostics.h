#pragma once

#include <cstddef>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class PDDiagnostics
{
public:
	void write(std::string const &message);
	void writeOnce(std::string const &key, std::string const &message);

	std::vector<std::string> lines() const;
	std::vector<std::pair<std::string, int>> repeats() const;
	void clear();

private:
	static constexpr std::size_t MaxLines = 512;

	mutable std::mutex m_mutex;
	std::vector<std::string> m_lines;
	std::unordered_map<std::string, int> m_counts;
	std::unordered_map<std::string, std::string> m_messages;
};
