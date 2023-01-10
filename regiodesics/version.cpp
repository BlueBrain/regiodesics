#include <regiodesics/version.h>

namespace regiodesics
{

int Version::getMajor()
{
  return REGIODESICS_VERSION_MAJOR;
}

int Version::getMinor()
{
  return REGIODESICS_VERSION_MINOR;
}

int Version::getPatch()
{
  return REGIODESICS_VERSION_PATCH;
}

std::string Version::getString()
{
  return REGIODESICS_VERSION_STRING;
}
}
