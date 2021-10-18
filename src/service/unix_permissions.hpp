#pragma once

namespace srv
{

class UnixPermissions
{
public:
    struct PermGroup
    {
        bool read, write;
        union
        {
            bool execute;
            bool queryFiles;
        };
    };

    PermGroup owner;
    PermGroup group;
    PermGroup others;
   
    explicit UnixPermissions();

    void setToFileSafe();
    void setToFolderSafe();

    void loadFromDecimal( short decMode );
    void loadFromChmod( short specifier );
    short convertToDecimal();
    short convertToChmod();

    static bool checkElfHeader( const void* data, int length );

private:
    static void setPermGroupToDigit( PermGroup& group, unsigned char digit );
    static unsigned char getDigitFromPermGroup( const PermGroup& group );
};

};
