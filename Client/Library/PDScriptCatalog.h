#pragma once

#include <string>
#include <vector>

struct PDScriptEntry
{
	std::string name;
	std::string relativePath;
	std::string fullPath;
	int lineCount = 0;
};

class PDScriptCatalog
{
public:
	void load(std::string const &scriptsRoot);

	std::vector<PDScriptEntry> const &entries() const
	{
		return m_entries;
	}

private:
	std::vector<PDScriptEntry> m_entries;
};
