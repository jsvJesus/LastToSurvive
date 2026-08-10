#include "VersionNo.h"

#include <cstdio>

extern const char* versionGetString();

namespace // currently unused so hide it
{
  
const	char*		_version_DATE = __DATE__;
const	char*		_version_TIME = __TIME__;

const char* versionGetCompileDate()
{
  return __DATE__;
}

const char* versionGetCompileTime()
{
  return __DATE__;
}

unsigned int versionGetBuild()
{
  return BUILD_VERSION;
}

} // namespace

const char* versionGetString()
{
  static char version[128];
  #ifdef _DEBUG
    std::snprintf(
      version,
      sizeof(version),
      "build%d(debug) (%s %s)",
      BUILD_VERSION,
      __DATE__,
      __TIME__);
  #else
    std::snprintf(
      version,
      sizeof(version),
      "build%d (%s %s)",
      BUILD_VERSION,
      __DATE__,
      __TIME__);
  #endif

  return version;
}
