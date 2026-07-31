#include <Library/PDScriptCatalog.h>

#include <algorithm>
#include <filesystem>
#include <fstream>

void PDScriptCatalog::load(std::string const &scriptsRoot)
{
	m_entries.clear();

	std::filesystem::path const root(scriptsRoot);
	std::error_code error;

	for (std::filesystem::recursive_directory_iterator iterator(root, error), end; iterator != end and not error; iterator.increment(error))
	{
		if (not iterator->is_regular_file() or iterator->path().extension() != ".lua")
		{
			continue;
		}

		PDScriptEntry entry;
		entry.fullPath = iterator->path().lexically_normal().string();
		entry.relativePath = std::filesystem::relative(iterator->path(), root).generic_string();
		entry.name = iterator->path().filename().string();

		std::ifstream stream(iterator->path(), std::ios::binary);

		if (stream)
		{
			std::string line;

			while (std::getline(stream, line))
			{
				entry.lineCount += 1;
			}
		}

		m_entries.push_back(std::move(entry));
	}

	std::sort(m_entries.begin(), m_entries.end(), [](PDScriptEntry const &a, PDScriptEntry const &b)
	{
		return a.relativePath < b.relativePath;
	});
}
