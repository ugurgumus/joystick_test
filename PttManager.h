#ifndef PTT_MANAGER_H
#define PTT_MANAGER_H

#include <string>

#include <boost/atomic.hpp>
#include <boost/thread.hpp>

#include "PttAdapter.h"
#include "IPttListener.h"

class PttManager : public PttAdapter
{
public:

    ~PttManager();

    static PttManager* initialize(std::string const& pttDevice);

private:

    PttManager(std::string const& deviceNodePath);

    PttManager();

    static bool findPttDev(std::string const& pttDevice, std::string& deviceNodePath);

    void runThread();

    void printDevInfo(int fd);

    bool openDevice(char const* const dev);

    bool tryToReOpenDevice();


private:

    std::string m_DeviceNodePath;

    boost::atomic_bool m_IsRunning;

    boost::shared_ptr<boost::thread> m_ThreadInstance;

    PttEvent m_PttEvent;

    static std::string m_PttDevice;

    int m_FD;
};

#endif
