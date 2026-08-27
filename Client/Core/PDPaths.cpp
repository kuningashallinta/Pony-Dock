#include <Core/PDPaths.h>

#include <windows.h>

#include <filesystem>

static std::filesystem::path executableDirectory()
{
	std::wstring buffer(UNICODE_STRING_MAX_CHARS, L'\0');
	buffer.resize(GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size())));

	return std::filesystem::path(buffer).parent_path();
}

std::string packsRoot()
{
	return (executableDirectory() / "Packs").lexically_normal().string();
}

std::string scriptsRoot()
{
	return (executableDirectory() / "Scripts").lexically_normal().string();
}
