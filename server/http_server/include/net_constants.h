#ifndef NET_CONSTANTS_INCLUDED
#define NET_CONSTANTS_INCLUDED


namespace http {

inline constexpr const char* kHttpPort = "8080";
inline constexpr uint8_t kMaxConnections = 20;
inline constexpr uint16_t kReceptionBufSize = 1024;
inline constexpr uint16_t kReceptionBufLimit = 8*kReceptionBufSize;
inline constexpr bool kInvalidSocket = -1;
inline constexpr bool kEmptyDescriptor = 0;
inline constexpr size_t kNumHTTPMethods = 5;

} // namespace http

#endif 