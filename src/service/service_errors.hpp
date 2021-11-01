#pragma once

namespace srv
{

enum class ServiceError
{
    KEIN_ERROR, // Can't be NO_ERROR because windows.h has stolen it...
    UNKNOWN_ERROR,
    ZEROCONF_SERVER_FAILED,
    REGISTRATION_V1_SERVER_FAILED,
    REGISTRATION_V2_SERVER_FAILED,
    WARP_SERVER_FAILED,
    CANT_CREATE_OUTPUT_DIR
};

};
