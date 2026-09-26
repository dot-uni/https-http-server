#ifndef HTTP_CONSTANTS_INCLUDED
#define HTTP_CONSTANTS_INCLUDED


namespace uni::server::http {

inline constexpr const char* kHttpPort = "8080";
inline constexpr uint8_t kMaxConnections = 20;
inline constexpr uint16_t kReceptionBufSize = 1024;
inline constexpr uint16_t kReceptionBufLimit = 8*kReceptionBufSize;
inline constexpr int kInvalidSocket = -1;
inline constexpr int kEmptyDescriptor = 0;

} // namespace uni::server::http

#endif 