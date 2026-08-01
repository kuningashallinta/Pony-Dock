#pragma once

#include <string>

std::string ponyPackOverrideRoot();
std::string ponyPackOverridePath(std::string const &packPath);
std::string ponyPackDocumentPath(std::string const &packPath);
bool ponyPackOverrideExists(std::string const &packPath);

bool writePonyPackOverride(std::string const &packPath, std::string const &text, std::string &outError);
bool removePonyPackOverride(std::string const &packPath, std::string &outError);
