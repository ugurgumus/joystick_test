
#include <stdio.h>
#include <unistd.h>
#include <thread>
#include <memory>

#include "PttManager.h"

class PttTest : public IPttListener
{
public:

    PttTest() {}
    virtual ~PttTest(){}

    void initializePttManager(char const* device)
    {
        m_PttManager.reset(PttManager::initialize(device));

        if (m_PttManager.get() != 0)
        {
            m_PttManager->addPttListener(this);
        }
        else {
            printf("Error in line %d: PTT Manager not initialized!!!\n", __LINE__);
        }
    }

    virtual void pttPressed(PttEvent const& event)
    {
        printf("pttPressed: %d\n", event.SourceId);
        fflush(stdout);
    }

    virtual void pttReleased(PttEvent const& event)
    {
        printf("pttReleased: %d\n", event.SourceId);
        fflush(stdout);
    }

private:

    std::shared_ptr<PttManager> m_PttManager;
};

int main(int argc, char* argv[])
{
    if (argc < 2) {
        puts("Enter device name as arg!");
        return -1;
    }
    printf("Device: %s\n", argv[1]);

    PttTest test;
    test.initializePttManager(argv[1]);

    while (true)
    {
        sleep(1);
    }

    return 0;
}
