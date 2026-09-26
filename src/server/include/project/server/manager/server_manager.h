#ifndef SERVER_MANAGER_INCLUDED
#define SERVER_MANAGER_INCLUDED

namespace uni::server::manager {

class ServerManager 
{
public:
    ServerManager() = default;
    virtual ~ServerManager() = default;

    ServerManager(const ServerManager&) = default;
    ServerManager(ServerManager&&) = default;

    ServerManager& operator=(const ServerManager&) = default;
    ServerManager& operator=(ServerManager&&) = default;
public:
    virtual bool Launch() = 0;
    virtual bool ShutDown() = 0;

    virtual bool Continue() = 0;
    virtual bool Suspension() = 0;
};

} // namespace uni::server::manager

#endif