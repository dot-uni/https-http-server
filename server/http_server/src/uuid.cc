#include "uuid.h"


namespace uuid {

std::string generate_uuid_v4() 
{
    static thread_local std::mt19937_64 gen{std::random_device{}()};
    std::uniform_int_distribution<uint64_t> dist;

    uint64_t a = dist(gen), b = dist(gen);

    a = (a & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
    b = (b & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;

    std::ostringstream ss;
    ss << std::hex << std::setfill('0')
       << std::setw(8) << ((a >> 32) & 0xFFFFFFFF) << "-"
       << std::setw(4) << ((a >> 16) & 0xFFFF) << "-"
       << std::setw(4) << (a & 0xFFFF) << "-"
       << std::setw(4) << ((b >> 48) & 0xFFFF) << "-"
       << std::setw(12) << (b & 0xFFFFFFFFFFFFULL);
    return ss.str();
}

} // namespace uuid