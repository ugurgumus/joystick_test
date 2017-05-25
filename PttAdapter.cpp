
#include "PttAdapter.h"

PttAdapter::PttAdapter()
{
}

PttAdapter::~PttAdapter()
{
}

void PttAdapter::pttPressed(PttEvent const& event)
{
    std::vector<IPttListener*>::const_iterator it = m_PttListeners.begin();
    for (; it != m_PttListeners.end(); ++it)
    {
        (*it)->pttPressed(event);
    }
}

void PttAdapter::pttReleased(PttEvent const& event)
{
    std::vector<IPttListener*>::const_iterator it = m_PttListeners.begin();
    for (; it != m_PttListeners.end(); ++it)
    {
        (*it)->pttReleased(event);
    }
}

void PttAdapter::addPttListener(IPttListener* listener)
{
    m_PttListeners.push_back(listener);
}
