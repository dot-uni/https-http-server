#include "project/logging/config_watcher_impl/macos.h"


namespace uni {
namespace logrr {


ConfigWatcher::ConfigWatcherImpl::ConfigWatcherImpl(std::filesystem::path path, SignalHandler sh) : sh_(std::move(sh))
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
        fd_ = -1;
        throw std::runtime_error("kqueue failed to return a descriptor");
    }

    EV_SET(&kev[0], fd_, EVFILT_VNODE, EV_ADD | EV_CLEAR, NOTE_WRITE, 0, nullptr);
    EV_SET(&kev[1], 1, EVFILT_USER, EV_ADD | EV_CLEAR, 0, 0, nullptr);
    
    int ret = kevent(kq_, kev, 2, nullptr, 0, nullptr);
    if (ret == -1) {
        close(kq_);
        close(fd_);
        kq_ = -1;
        fd_ = -1;
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
    worker_ = std::thread(&ConfigWatcher::ConfigWatcherImpl::Run, this);
}


void ConfigWatcher::ConfigWatcherImpl::Stop()
{
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
        struct kevent triggerd;
        const int n = kevent(kq_, nullptr, 0, &triggerd, 1, nullptr);
        if (n == -1) {
            // log плохой
            return;
        }
        else if (n > 0 && (triggerd.flags & EV_ERROR)) {
            // log плохой
            return;
        }
        switch (triggerd.filter) {
            case EVFILT_VNODE: {
                if (!(triggerd.fflags & NOTE_WRITE)) {
                    // log и вывод triggerd.fflags
                    break;
                }

                char path[PATH_MAX];
                if (fcntl(fd_, F_GETPATH, path) == -1) {
                    // log плохой
                    break;
                }
                try {
                    sh_(path);
                } catch (const std::exception& error) {
                    syslog("Config watcher callback failed: ", error.what());
                } catch (...) {
                    syslog("Config watcher callback failed with an unknown exception");
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
        syslog("Config watcher thread failed: ", error.what());
    } catch (...) {
        syslog("Config watcher thread failed with an unknown exception");
    }
}

} // namespace logrr 
} // namespace uni 
