#pragma once
#include <string>
#include <vector>

namespace gui
{

class NetworkInterfaceAdapter
{
public:
    struct InetInfo
    {
        int id;
        std::string name;
        std::wstring displayName;
        bool defaultInterface;

        int metricV4;
        int metricV6;
    };

    NetworkInterfaceAdapter();
    std::vector<InetInfo> getAllInterfaces();
    std::string getDefaultInterfaceName();

private:
    bool m_dataReady;
    std::vector<InetInfo> m_data;

    void loadData();
};

};
