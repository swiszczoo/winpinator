#pragma once
#include <string>
#include <vector>

namespace srv
{

namespace db
{

enum class TransferElementType
{
    FILE,
    FOLDER,
    UNKNOWN
};

struct TransferElement
{
    int id;

    TransferElementType elementType;
    std::wstring elementName; // File or folder name
    std::wstring relativePath; // Path relative to root of the transfer
    std::wstring absolutePath; // Absolute path of the element or empty
};

enum class TransferStatus
{
    SUCCEEDED,
    CANCELLED,
    FAILED,
    UNKNOWN
};

enum class TransferType
{
    SINGLE_FILE,
    MULTIPLE_FILES,
    SINGLE_DIRECTORY,
    MULTIPLE_DIRECTORIES,
    MIXED,
    UNFINISHED_INCOMING,
    UNKNOWN
};

struct Transfer
{
    int id;

    std::wstring targetId;
    std::wstring singleElementName;
    TransferType transferType;
    long long transferTimestamp;
    int fileCount;
    int folderCount;
    long long totalSizeBytes;
    bool outgoing;
    TransferStatus status;

    std::vector<TransferElement> elements;
};

struct TargetInfo
{
    std::wstring targetId;
    std::wstring fullName;
    std::wstring hostname;
    std::wstring ip;
    std::wstring os;
};

struct TargetInfoData : public TargetInfo
{
    long long transferCount;
};

};

};
