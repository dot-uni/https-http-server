#include "project/logging/config/configurator.h"

#ifdef __APPLE__
#include "project/logging/config/impl/macos.h"
#endif

namespace uni::logging::config {

ConfigWatcher::ConfigWatcher(std::filesystem::path path, SignalHandler sh) : impl_(std::make_unique<ConfigWatcherImpl>(std::move(path), std::move(sh))) {}

ConfigWatcher::~ConfigWatcher() = default;

void ConfigWatcher::Start() { impl_->Start(); }
void ConfigWatcher::Stop() { impl_->Stop(); }

} // namespace uni::logging::config