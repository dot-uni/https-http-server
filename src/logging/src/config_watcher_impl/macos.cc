#include "project/logging/config_watcher_impl/macos.h"


namespace uni {
namespace logrr {


ConfigWatcher::ConfigWatcherImpl::ConfigWatcherImpl(std::filesystem::path path, SignalHandler sh) : sh_(std::move(sh)), started_(false)
{
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("File not found: " + path.string());
    }
    fd_ = open(path.c_str(), O_EVTONLY);
    if (fd_ == -1) {
        throw std::runtime_error("Unable to open file for watching: " + path.string());
    }

    struct kevent kev[2];

    if ((kq_ = kqueue()) == -1) {
        close(fd_);
        throw std::runtime_error("kqueue failed to return a descriptor");
    }

    EV_SET(&kev[0], fd_, EVFILT_VNODE, EV_ADD | EV_CLEAR, NOTE_WRITE, 0, nullptr);
    EV_SET(&kev[1], 1, EVFILT_USER, EV_ADD | EV_CLEAR, 0, 0, nullptr);
    
    int ret = kevent(kq_, kev, 2, nullptr, 0, nullptr);
    if (ret == -1) {
        close(kq_);
        close(fd_);
        throw std::runtime_error("kevent registration error");
    }
}


ConfigWatcher::ConfigWatcherImpl::~ConfigWatcherImpl()
{
    Stop();
    if (fd_ != -1) {
        close(fd_);
    }
    if (kq_ != -1) {
        close(kq_);
    }
}


void ConfigWatcher::ConfigWatcherImpl::Start()
{
    std::lock_guard<std::mutex> lock(mtx_);
    started_.store(true);
    worker_ = std::thread(&ConfigWatcher::ConfigWatcherImpl::Run, this);
}


void ConfigWatcher::ConfigWatcherImpl::Stop()
{
    if (!started_.exchange(false)) {
        sys_warn("Warning regarding an attempt to call Stop() again on the ConfigWatcher object");
        return;
    }

    struct kevent kev;
    EV_SET(&kev, 1, EVFILT_USER, 0, NOTE_TRIGGER, 0, nullptr);
    kevent(kq_, &kev, 1, nullptr, 0, nullptr);
    
    std::lock_guard<std::mutex> lock(mtx_);
    if (worker_.joinable()) {
        worker_.join();
    }
}


void ConfigWatcher::ConfigWatcherImpl::Run()
{
    try {
        while (true) {
        struct kevent triggered;
        const int n = kevent(kq_, nullptr, 0, &triggered, 1, nullptr);
        if (n == -1) {
            sys_error("Config watcher kevent() failed: {}", std::strerror(errno));
            return;
        }

        if (!n) {
            sys_info("Config watcher kevent() returned no events");
            continue;
        }

        if (triggered.flags & EV_ERROR) {
            sys_error(
                "Config watcher received EV_ERROR: filter={}, ident={}, error={}", 
                triggered.filter, triggered.ident, std::strerror(static_cast<int>(triggered.data))
            );
            return;
        }

        switch (triggered.filter) {
            case EVFILT_VNODE: {
                sys_info(
                    "Config watcher received vnode event: ident={}, flags={}", 
                    triggered.ident, triggered.fflags
                );

                if (!(triggered.fflags & NOTE_WRITE)) {
                    sys_info(
                        "Config watcher ignored vnode event without NOTE_WRITE: fflags={}",
                        triggered.fflags
                    );
                    break;
                }

                char path[PATH_MAX];
                if (fcntl(fd_, F_GETPATH, path) == -1) {
                    sys_error(
                        "Config watcher failed to resolve watched file path: {}",
                        std::strerror(errno)
                    );
                    break;
                }

                sys_info("Config watcher detected configuration update: {}", path);

                try {
                    sh_(path);
                    sys_info("Config watcher callback completed: {}", path);
                } catch (const std::exception& error) {
                    sys_error(
                        "Config watcher callback failed for {}: {}", 
                        path, error.what()
                    );
                } catch (...) {
                    sys_error("Config watcher callback failed for {}: unknown exception", path);
                }
                break;
            }
            case EVFILT_USER: {
                // log
                return;
            }
        }
    }
    } catch (const std::exception& error) {
        sys_error("Config watcher thread failed: {}", error.what());
    } catch (...) {
        sys_error("Config watcher thread failed with an unknown exception");
    }
}

} // namespace logrr 
} // namespace uni 
