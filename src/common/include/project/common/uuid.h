#ifndef UUID_INCLUDED
#define UUID_INCLUDED

#include <random>
#include <sstream>
#include <iomanip>

namespace uni::common::uuid  {

std::string generate_uuid_v4();

} // namespace uni::common::uuid 

#endif