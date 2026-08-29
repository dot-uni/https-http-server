#ifndef UUID_INCLUDED
#define UUID_INCLUDED

#include <random>
#include <sstream>
#include <iomanip>

namespace uuid {

std::string generate_uuid_v4();

} // namespace uuid

#endif