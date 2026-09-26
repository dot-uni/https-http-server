#ifndef CONFIGURATOR_INCLUDED
#define CONFIGURATOR_INCLUDED

#include <fcntl.h>
#include <fstream>
#include <filesystem>
#include <string_view>
#include <thread>
#include <functional>

#ifdef __APPLE__
#include <sys/event.h>
#endif

#include "project/logging/logging.h"


namespace uni::logging::config {

using SignalHandler = std::function<bool(std::filesystem::path)>;


class ConfigWatcher 
{
private:
    class ConfigWatcherImpl;
    std::unique_ptr<ConfigWatcherImpl> impl_;

public:
    ConfigWatcher(std::filesystem::path path, SignalHandler sh);
    virtual ~ConfigWatcher();

    ConfigWatcher(const ConfigWatcher&) = delete;
    ConfigWatcher(ConfigWatcher&&) = default;

    ConfigWatcher& operator=(const ConfigWatcher&) = delete;
    ConfigWatcher& operator=(ConfigWatcher&&) = default;

    void Start();
    void Stop();
};

} // namespace uni::logging::config

#endif