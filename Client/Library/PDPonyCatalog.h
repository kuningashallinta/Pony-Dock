#pragma once

#include <string>
#include <vector>

struct PDPonyPack
{
	std::string id;
	std::string packPath;
	std::string previewPath;
};

struct PDPonyGroup
{
	std::string displayName;
	std::vector<PDPonyPack> variants;
};

class PDPonyCatalog
{
public:
	void load(std::string const &packsRoot);

	std::vector<PDPonyGroup> const &groups() const
	{
		return m_groups;
	}

private:
	std::vector<PDPonyGroup> m_groups;
};
