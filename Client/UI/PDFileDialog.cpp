#include <UI/PDFileDialog.h>

#include <Core/PDString.h>

#include <commdlg.h>

#include <filesystem>
#include <vector>

bool openFileDialog(HWND owner, const char *title, std::string &outPath)
{
	std::vector<wchar_t> buffer(MAX_PATH * 4, L'\0');
	std::wstring const caption = toWide(title);

	OPENFILENAMEW dialog = {};
	dialog.lStructSize = sizeof(OPENFILENAMEW);
	dialog.hwndOwner = owner;
	dialog.lpstrFilter = L"Pony Dock config (*.json)\0*.json\0All files (*.*)\0*.*\0";
	dialog.nFilterIndex = 1;
	dialog.lpstrFile = buffer.data();
	dialog.nMaxFile = static_cast<DWORD>(buffer.size());
	dialog.lpstrTitle = caption.c_str();
	dialog.lpstrDefExt = L"json";
	dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;

	if (GetOpenFileNameW(&dialog) == 0)
	{
		return false;
	}

	outPath = std::filesystem::path(buffer.data()).string();

	return true;
}

bool saveFileDialog(HWND owner, const char *title, const char *suggestedName, std::string &outPath)
{
	std::vector<wchar_t> buffer(MAX_PATH * 4, L'\0');
	std::wstring const caption = toWide(title);
	std::wstring const suggested = toWide(suggestedName);

	for (std::size_t index = 0; index < suggested.size() and index + 1 < buffer.size(); index += 1)
	{
		buffer[index] = suggested[index];
	}

	OPENFILENAMEW dialog = {};
	dialog.lStructSize = sizeof(OPENFILENAMEW);
	dialog.hwndOwner = owner;
	dialog.lpstrFilter = L"Pony Dock config (*.json)\0*.json\0All files (*.*)\0*.*\0";
	dialog.nFilterIndex = 1;
	dialog.lpstrFile = buffer.data();
	dialog.nMaxFile = static_cast<DWORD>(buffer.size());
	dialog.lpstrTitle = caption.c_str();
	dialog.lpstrDefExt = L"json";
	dialog.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;

	if (GetSaveFileNameW(&dialog) == 0)
	{
		return false;
	}

	outPath = std::filesystem::path(buffer.data()).string();

	return true;
}
