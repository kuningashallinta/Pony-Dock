#include <Library/PDPonyPackOverride.h>

#include <windows.h>

#include <shlobj.h>

#include <filesystem>
#include <fstream>

static std::string packFolderName(std::string const &packPath)
{
	return std::filesystem::path(packPath).lexically_normal().filename().string();
}

std::string ponyPackOverrideRoot()
{
	PWSTR folder = nullptr;

	if (FAILED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &folder)))
	{
		return "overrides";
	}

	std::filesystem::path path(folder);
	CoTaskMemFree(folder);

	path /= "Pony Dock";
	path /= "overrides";

	return path.string();
}

std::string ponyPackOverridePath(std::string const &packPath)
{
	std::string const folder = packFolderName(packPath);

	if (folder.empty())
	{
		return std::string();
	}

	return (std::filesystem::path(ponyPackOverrideRoot()) / (folder + ".json")).string();
}

bool ponyPackOverrideExists(std::string const &packPath)
{
	std::string const path = ponyPackOverridePath(packPath);

	if (path.empty())
	{
		return false;
	}

	std::error_code error;

	return std::filesystem::is_regular_file(path, error);
}

std::string ponyPackDocumentPath(std::string const &packPath)
{
	if (ponyPackOverrideExists(packPath))
	{
		return ponyPackOverridePath(packPath);
	}

	return (std::filesystem::path(packPath) / "pony.json").string();
}

bool writePonyPackOverride(std::string const &packPath, std::string const &text, std::string &outError)
{
	std::string const path = ponyPackOverridePath(packPath);

	if (path.empty())
	{
		outError = "cannot derive an override name for " + packPath;

		return false;
	}

	std::filesystem::path const target(path);
	std::error_code error;
	std::filesystem::create_directories(target.parent_path(), error);

	std::filesystem::path temporary = target;
	temporary += ".tmp";

	{
		std::ofstream stream(temporary, std::ios::binary);

		if (not stream)
		{
			outError = "cannot write " + temporary.string();

			return false;
		}

		stream << text;
	}

	std::filesystem::rename(temporary, target, error);

	if (error)
	{
		outError = "cannot replace " + target.string();

		return false;
	}

	return true;
}

bool removePonyPackOverride(std::string const &packPath, std::string &outError)
{
	std::string const path = ponyPackOverridePath(packPath);

	if (path.empty())
	{
		outError = "cannot derive an override name for " + packPath;

		return false;
	}

	std::error_code error;
	std::filesystem::remove(path, error);

	if (error)
	{
		outError = "cannot remove " + path;

		return false;
	}

	return true;
}
