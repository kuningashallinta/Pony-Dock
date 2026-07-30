#include <Engine/PDDiagnostics.h>

#include <windows.h>

#include <algorithm>

void PDDiagnostics::write(std::string const &message)
{
	OutputDebugStringA(message.c_str());
	OutputDebugStringA("\n");

	std::lock_guard<std::mutex> const lock(m_mutex);

	m_lines.push_back(message);

	if (m_lines.size() > MaxLines)
	{
		m_lines.erase(m_lines.begin(), m_lines.begin() + static_cast<std::ptrdiff_t>(m_lines.size() - MaxLines));
	}
}

void PDDiagnostics::writeOnce(std::string const &key, std::string const &message)
{
	{
		std::lock_guard<std::mutex> const lock(m_mutex);

		int &count = m_counts[key];
		count += 1;
		m_messages[key] = message;

		if (count > 1)
		{
			return;
		}
	}

	write(message);
}

std::vector<std::string> PDDiagnostics::lines() const
{
	std::lock_guard<std::mutex> const lock(m_mutex);

	return m_lines;
}

std::vector<std::pair<std::string, int>> PDDiagnostics::repeats() const
{
	std::lock_guard<std::mutex> const lock(m_mutex);

	std::vector<std::pair<std::string, int>> result;

	for (auto const &entry : m_counts)
	{
		if (entry.second > 1)
		{
			auto const message = m_messages.find(entry.first);
			result.emplace_back(message != m_messages.end() ? message->second : entry.first, entry.second);
		}
	}

	std::sort(result.begin(), result.end(), [](auto const &a, auto const &b)
	{
		return a.second > b.second;
	});

	return result;
}

void PDDiagnostics::clear()
{
	std::lock_guard<std::mutex> const lock(m_mutex);

	m_lines.clear();
	m_counts.clear();
	m_messages.clear();
}
