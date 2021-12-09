#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <Windows.h>
#include <plugin.hpp>
#include <farcolor.hpp>

#include <guiddef.h>
#include <assert.h>
#include <vector>
#include <string>
#include <list>
#include <map>
#include <set>
#include <algorithm>
#include <experimental/filesystem>

extern const GUID			_FPG;
extern const GUID			_MenuGuid;
extern PluginStartupInfo	_PSI;
extern FarStandardFunctions	_FSF;
