#include "unix_permissions.hpp"

namespace srv
{

UnixPermissions::UnixPermissions()
    : owner( { 0, 0, 0 } )
    , group( { 0, 0, 0 } )
    , others( { 0, 0, 0 } )
{
}

void UnixPermissions::setToFileSafe()
{
    loadFromChmod( 664 );
}

void UnixPermissions::setToFolderSafe()
{
    loadFromChmod( 775 );
}

void UnixPermissions::loadFromDecimal( short decMode )
{
    short ownerPart = ( decMode >> 6 ) & 0b111;
    short groupPart = ( decMode >> 3 ) & 0b111;
    short othersPart = ( decMode >> 0 ) & 0b111;

    setPermGroupToDigit( owner, (unsigned char)ownerPart );
    setPermGroupToDigit( group, (unsigned char)groupPart );
    setPermGroupToDigit( others, (unsigned char)othersPart );
}

void UnixPermissions::loadFromChmod( short specifier )
{
    short ownerPart = ( specifier / 100 ) % 10;
    short groupPart = ( specifier / 10 ) % 10;
    short othersPart = ( specifier / 1 ) % 10;

    setPermGroupToDigit( owner, (unsigned char)ownerPart );
    setPermGroupToDigit( group, (unsigned char)groupPart );
    setPermGroupToDigit( others, (unsigned char)othersPart );
}

short UnixPermissions::convertToDecimal()
{
    short ownerPart = getDigitFromPermGroup( owner );
    short groupPart = getDigitFromPermGroup( group );
    short othersPart = getDigitFromPermGroup( others );

    return ( ownerPart << 6 ) + ( groupPart << 3 ) + ( othersPart << 0 );
}

short UnixPermissions::convertToChmod()
{
    short ownerPart = getDigitFromPermGroup( owner );
    short groupPart = getDigitFromPermGroup( group );
    short othersPart = getDigitFromPermGroup( others );

    return ( ownerPart * 100 ) + ( groupPart * 10 ) + ( othersPart * 1 );
}

bool UnixPermissions::checkElfHeader( const void* data, int length )
{
    static const int ELF_MAGIC_LENGTH = 4;

    if ( length < ELF_MAGIC_LENGTH )
    {
        return false;
    }

    const unsigned char* chr = (const unsigned char*)data;

    return chr[0] == 0x7F && chr[1] == 'E' && chr[2] == 'L' && chr[3] == 'F';
}

void UnixPermissions::setPermGroupToDigit(
    PermGroup& group, unsigned char digit )
{
    group.read = ( digit & ( 1 << 2 ) ) ? true : false;
    group.write = ( digit & ( 1 << 1 ) ) ? true : false;
    group.execute = ( digit & ( 1 << 0 ) ) ? true : false;
}

unsigned char UnixPermissions::getDigitFromPermGroup(
    const PermGroup& group )
{
    unsigned char digit = 0;

    if ( group.read )
    {
        digit += 1 << 2;
    }

    if ( group.write )
    {
        digit += 1 << 1;
    }

    if ( group.execute )
    {
        digit += 1 << 0;
    }

    return digit;
}

};
