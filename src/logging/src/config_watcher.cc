#include "project/logging/config_watcher.h"

#ifdef __APPLE__
#include "project/logging/config_watcher_impl/macos.h"
#endif

namespace uni {
namespace logrr {

ConfigWatcher::ConfigWatcher(std::filesystem::path path, SignalHandler sh) : impl_(std::make_unique<ConfigWatcherImpl>(std::move(path), std::move(sh))) {}

ConfigWatcher::~ConfigWatcher() = default;

void ConfigWatcher::Start() { impl_->Start(); }
void ConfigWatcher::Stop() { impl_->Stop(); }

} // namespace logrr
} // namespace uni