#pragma once
#include "database_types.hpp"

#include <atomic>
#include <chrono>
#include <mutex>
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

struct EventLock
{
    std::mutex mutex;
    std::condition_variable condVar;
    bool value;
};

struct TransferOpPub
{
    int id;
    bool outcoming;

    std::shared_ptr<std::mutex> mutex;

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

        long long sentBytes;
    } meta;

    bool useCompression;
};

struct TransferOp : public TransferOpPub
{
    struct
    {
        std::chrono::steady_clock::time_point lastProgressUpdate;
        std::shared_ptr<EventLock> pauseLock;

        std::vector<db::TransferElement> elements;

        int fileCount;
        int dirCount;

        int crawlJobId;
    } intern;
};


};
