#include <gtest/gtest.h>
#include <memory>
#include <random>

#include "../src/service/unix_permissions.cpp"

using namespace srv;

class UnixPermissionsTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        instance = std::make_shared<UnixPermissions>();
    }

    void TearDown() override
    {
        instance = nullptr;
    }

    bool ComparePermissions( UnixPermissions::PermGroup& p1,
        UnixPermissions::PermGroup& p2 )
    {
        return p1.read == p2.read
            && p1.write == p2.write && p1.execute == p2.execute;
    }

    std::shared_ptr<UnixPermissions> instance;
};

TEST_F( UnixPermissionsTest, expectInitializeAllToZero )
{
    UnixPermissions::PermGroup group;
    group.read = group.write = group.execute = false;

    EXPECT_TRUE( ComparePermissions( instance->owner, group ) ) << "Owner permissions are bad";
    EXPECT_TRUE( ComparePermissions( instance->group, group ) ) << "Group permissions are bad";
    EXPECT_TRUE( ComparePermissions( instance->others, group ) ) << "Others permissions are bad";
}

TEST_F( UnixPermissionsTest, expectSafeFolderCanBeQueriedByEveryone )
{
    instance->setToFolderSafe();

    EXPECT_TRUE( instance->owner.queryFiles ) << "Owner can't query files";
    EXPECT_TRUE( instance->group.queryFiles ) << "Group can't query files";
    EXPECT_TRUE( instance->others.queryFiles ) << "Others can't query files";
}

TEST_F( UnixPermissionsTest, expectSafeFileCanBeReadByEveryone )
{
    instance->setToFileSafe();

    EXPECT_TRUE( instance->owner.read ) << "Owner can't read file";
    EXPECT_TRUE( instance->group.read ) << "Group can't read file";
    EXPECT_TRUE( instance->others.read ) << "Others can't read file";
}

TEST_F( UnixPermissionsTest, expectSafeFileCantBeWrittenByOthers )
{
    instance->setToFileSafe();

    EXPECT_FALSE( instance->others.write ) << "Others can write to safe file";
}

TEST_F( UnixPermissionsTest, expectDecimalInputEqualsOutput )
{
    std::mt19937 eng;
    std::uniform_int_distribution<int> dist( 0, 511 );

    for ( int i = 0; i < 10000; i++ )
    {
        int val = dist( eng );

        instance->loadFromDecimal( val );
        EXPECT_EQ( val, instance->convertToDecimal() ) << "Decimal values are different";
    }
}

TEST_F( UnixPermissionsTest, expectChmodInputEqualsOutput )
{
    std::mt19937 eng;
    std::uniform_int_distribution<int> dist( 0, 7 );

    for ( int i = 0; i < 10000; i++ )
    {
        int val = dist( eng ) * 100 + dist( eng ) * 10 + dist( eng );

        instance->loadFromChmod( val );
        EXPECT_EQ( val, instance->convertToChmod() ) << "CHMOD values are different";
    }
}

TEST_F( UnixPermissionsTest, expectElfHeaderDetectingWorks )
{
    std::string data;

    data = "";
    EXPECT_FALSE( UnixPermissions::checkElfHeader( data.data(), data.size() ) );

    data = "chomik dzungarski";
    EXPECT_FALSE( UnixPermissions::checkElfHeader( data.data(), data.size() ) );

    data = "fguhsdfuhghuidfhguihuisdfgh";
    EXPECT_FALSE( UnixPermissions::checkElfHeader( data.data(), data.size() ) );

    data = "JPEG";
    EXPECT_FALSE( UnixPermissions::checkElfHeader( data.data(), data.size() ) );

    data = "ELF";
    EXPECT_FALSE( UnixPermissions::checkElfHeader( data.data(), data.size() ) );

    data = '\x7f' + (std::string)"ELF";
    EXPECT_TRUE( UnixPermissions::checkElfHeader( data.data(), data.size() ) );

    data = '\x7f' + (std::string)"ELFsdfgkjhsdfhhfasduihuiasdhuiahsdfuifd";
    EXPECT_TRUE( UnixPermissions::checkElfHeader( data.data(), data.size() ) );
}
