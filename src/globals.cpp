#include "globals.hpp"

std::unique_ptr<Globals> Globals::s_inst = nullptr;

Globals::Globals()
{
}

Globals* Globals::get()
{
    if ( !Globals::s_inst )
    {
        Globals::s_inst = std::make_unique<Globals>();
    }

    return Globals::s_inst.get();
}

void Globals::setWinpinatorServiceInstance( srv::WinpinatorService* inst )
{
    m_service = inst;
}

srv::WinpinatorService* Globals::getWinpinatorServiceInstance() const
{
    return m_service;
}
