#pragma once
#include <chrono>
#include <string>
#include <vector>

namespace srv
{

enum class OpStatus
{
    INVALID_OP,
    INIT,
    CALCULATING,
    WAITING_PERMISSION,
    CANCELLED_PERMISSION_BY_SENDER,
    CANCELLED_PERMISSION_BY_RECEIVER,
    TRANSFERRING,
    PAUSED,
    STOPPED_BY_SENDER,
    STOPPED_BY_RECEIVER,
    FAILED,
    FAILED_UNRECOVERABLE,
    FILE_NOT_FOUND,
    FINISHED
};

enum class FileType
{
    INVALID,
    REGULAR_FILE,
    DIRECTORY,
    SYMBOLIC_LINK
};

struct TransferOp
{
    int id;
    bool outcoming;

    std::time_t startTime;
    std::string senderNameUtf8;
    std::string receiverUtf8;
    std::string receiverNameUtf8;
    OpStatus status;
    long long totalSize;
    long long totalCount;
    std::string mimeIfSingleUtf8;
    std::string nameIfSingleUtf8;
    std::vector<std::string> topDirBasenamesUtf8;

    struct
    {
        bool notEnoughSpace;
        bool mustOverwrite;
        std::time_t localTimestamp;
    } meta;

    bool useCompression;
};


};
