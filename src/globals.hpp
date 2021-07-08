#pragma once
#include "service/winpinator_service.hpp"

#include <memory>

/*
 This singleton holds references to globally available objects
 */
class Globals
{
    friend std::unique_ptr<Globals> std::make_unique<Globals>();

private:
    static std::unique_ptr<Globals> s_inst;

    srv::WinpinatorService* m_service;

    Globals();

public:
    static Globals* get();

    void setWinpinatorServiceInstance( srv::WinpinatorService* srv );
    srv::WinpinatorService* getWinpinatorServiceInstance() const;
};
