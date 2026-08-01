#include <Library/PDMonitor.h>

#include <windows.h>

#include <algorithm>
#include <cstdio>

static BOOL CALLBACK collectMonitor(HMONITOR monitor, HDC, LPRECT, LPARAM payload)
{
	MONITORINFOEXA info = {};
	info.cbSize = sizeof(MONITORINFOEXA);

	if (GetMonitorInfoA(monitor, &info) == 0)
	{
		return TRUE;
	}

	std::vector<PDMonitor> &monitors = *reinterpret_cast<std::vector<PDMonitor> *>(payload);

	PDMonitor entry;
	entry.device = info.szDevice;
	entry.x = info.rcMonitor.left;
	entry.y = info.rcMonitor.top;
	entry.width = info.rcMonitor.right - info.rcMonitor.left;
	entry.height = info.rcMonitor.bottom - info.rcMonitor.top;
	entry.primary = (info.dwFlags & MONITORINFOF_PRIMARY) != 0;

	monitors.push_back(std::move(entry));

	return TRUE;
}

std::vector<PDMonitor> enumerateMonitors()
{
	std::vector<PDMonitor> monitors;
	EnumDisplayMonitors(nullptr, nullptr, collectMonitor, reinterpret_cast<LPARAM>(&monitors));

	std::sort(monitors.begin(), monitors.end(), [](PDMonitor const &a, PDMonitor const &b)
	{
		if (a.primary != b.primary)
		{
			return a.primary;
		}

		if (a.x != b.x)
		{
			return a.x < b.x;
		}

		return a.y < b.y;
	});

	for (std::size_t index = 0; index < monitors.size(); index += 1)
	{
		PDMonitor &entry = monitors[index];

		char label[128];
		std::snprintf(
			label,
			sizeof(label),
			"Display %zu  -  %dx%d%s",
			index + 1,
			entry.width,
			entry.height,
			entry.primary ? "  (primary)" : "");

		entry.label = label;
	}

	return monitors;
}
