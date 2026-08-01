#pragma once

#include <windows.h>

#include <string>

bool openFileDialog(HWND owner, const char *title, std::string &outPath);
bool saveFileDialog(HWND owner, const char *title, const char *suggestedName, std::string &outPath);
