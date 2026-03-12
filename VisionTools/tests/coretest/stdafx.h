// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include <Windows.h>
#include <Shlwapi.h>

#include "WinUnitAdaptors.h"

#include "vtcore.h"
#include "vtfileio.h"


#pragma warning(push)
#pragma warning(disable:4995)
#include <stddef.h>
#include <vector>
#include <string>
#include <sstream>
#include <regex>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <numeric>
#include <map>
#include <tuple>
#include <memory>
#pragma warning(pop)
