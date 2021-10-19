#include <gtest/gtest.h>
#include <memory>
#include <random>

#include "../src/service/zlib_deflate.cpp"

using namespace srv;

class ZlibDeflateTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        instance = std::make_shared<ZlibDeflate>();
    }

    void TearDown() override
    {
        instance = nullptr;
    }

    void RepeatText( std::string& inout, int times )
    {
        std::string tmp = inout;

        for ( int i = 1; i < times; i++ )
        {
            inout += tmp;
        }
    }

    std::shared_ptr<ZlibDeflate> instance;
};

TEST_F( ZlibDeflateTest, expectDeflateShrinksDownEnglishText )
{
    std::string text;
    std::string output;

    text = "a rose by any other name would smell as sweet.";
    RepeatText( text, 50 );
    output = instance->compress( text, 5 );
    EXPECT_LT( output.size(), text.size() );

    text = "genius is one percent inspiration and ninety-nine percent perspiration.";
    RepeatText( text, 100 );
    output = instance->compress( text, 5 );
    EXPECT_LT( output.size(), text.size() );

    text = "nothing is certain except for death and taxes.";
    RepeatText( text, 200 );
    output = instance->compress( text, 5 );
    EXPECT_LT( output.size(), text.size() );

    text = "that’s one small step for a man, a giant leap for mankind.";
    RepeatText( text, 300 );
    output = instance->compress( text, 5 );
    EXPECT_LT( output.size(), text.size() );

    text = "you must be the change you wish to see in the world.";
    RepeatText( text, 400 );
    output = instance->compress( text, 5 );
    EXPECT_LT( output.size(), text.size() );
}

TEST_F( ZlibDeflateTest, expectDeflateToBeReversible )
{
    std::mt19937 eng;
    std::uniform_int_distribution<int> charDist( 0, 26 );
    std::uniform_int_distribution<int> sizeDist( 1000, 10000 );

    for ( int i = 0; i < 500; i++ )
    {
        int textSize = sizeDist( eng );

        std::string text;
        text.resize( textSize );

        for ( int j = 0; j < textSize; j++ )
        {
            text[j] = 'a' + charDist( eng );
        }

        std::string compressed = instance->compress( text, i % 9 + 1 );
        std::string decompressed = instance->decompress( compressed );
        
        EXPECT_EQ( text, decompressed );
    }
}

