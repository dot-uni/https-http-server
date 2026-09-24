#ifndef CONFIG_WATCHER_IMPL_MAC_INCLUDED
#define CONFIG_WATCHER_IMPL_MAC_INCLUDED

#include <unistd.h>
#include <atomic>
#include <mutex>

#include "project/logging/config_watcher.h"


namespace uni {
namespace logrr {

class ConfigWatcher::ConfigWatcherImpl final
{
public:
    ConfigWatcherImpl(std::filesystem::path path, SignalHandler sh);
    ~ConfigWatcherImpl();

    ConfigWatcherImpl(const ConfigWatcherImpl&) = delete;
    ConfigWatcherImpl(ConfigWatcherImpl&&) = delete;

    ConfigWatcherImpl& operator=(const ConfigWatcherImpl&) = delete;
    ConfigWatcherImpl& operator=(ConfigWatcherImpl&&) = delete;
public:
    void Start();
    void Stop();
private:
    void Run();
private:
    int fd_ = -1;
    int kq_ = -1;
    SignalHandler sh_;
    std::thread worker_;
    std::atomic<bool> stopped_;
    std::mutex mtx_;
};

} // namespace logrr
} // namespace uni

#endif
