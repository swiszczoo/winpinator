#include "zeroconf/mdns_service.hpp"
#include "zeroconf/mdns_client.hpp"

int main()
{
    //{
        WORD versionWanted = MAKEWORD( 1, 1 );
        WSADATA wsaData;
        if ( WSAStartup( versionWanted, &wsaData ) )
        {
            printf( "Failed to initialize WinSock\n" );
            return -1;
        }

        zc::MdnsService service( "_warpinator._tcp.local." );
        service.setHostname( "Testhost" );
        service.setPort( 42000 );
        service.setTxtRecord( "hostname", "Test device" );
        service.setTxtRecord( "type", "real" );
        service.registerService();

        Sleep( 2000 );
    //}

    //Sleep( 2000 );

    zc::MdnsClient client( "_warpinator._tcp.local." );
    client.startListening();

    Sleep( 600000 );

    return 0;
}
