# Winpinator

**Winpinator** is an unofficial Windows port of [Warpinator](https://github.com/linuxmint/warpinator), rewritten from scratch in C++. It supports the gRPC based protocol of its Linux equivalent, as well as zeroconf/mDNS based service discovery. Due to the fact that Windows has a totally different filesystem, some Warpinator features have to be emulated, e.g. sending filesystem permissions along with the files.

Winpinator should integrate well into Windows ecosystem by supporting drag-and-drop and appearing in Windows Explorer's *Send to* context menu.

## Screenshots

![readme/screen1.png](readme/screen1.png?raw=true)
![readme/screen2.png](readme/screen2.png?raw=true)
![readme/screen4.png](readme/screen4.png?raw=true)
![readme/screen3.png](readme/screen3.png?raw=true)

## Features

+ Answer and process mDNS queries to be able to discover Warpinator clients running on local network
+ Support both Warpinator registration protocols (v1 and v2)
+ Send and receive files, directories or any combination of those
+ Support deflate compression of data stream during transfers
+ Optionally save zone information to received files (`This file came from another computer and might be blocked to help protect this computer.` in File Properties dialog)
+ Store transfer history as well as all transferred file paths
+ Accept files to send as command line parameters and handle the *Send to* option in Windows Explorer's context menu
+ Optionally start on system startup
+ Run in background and show toast notifications when someone sends some files

## Building

As for now, Visual Studio 2019 is used to compile binaries. Any newer version of VS should work, but is not actively tested. C++14 is used to develop the software.

### Dependencies

Dependencies of Winpinator are managed by vcpkg, but vcpkg is optional. Currently, Winpinator depends on following libraries and tools:

+ [cpp-base64](https://github.com/ReneNyffenegger/cpp-base64)
+ [grpc library](https://github.com/grpc/grpc) and protobuf compiler
+ [libsodium](https://github.com/jedisct1/libsodium)
+ [openssl](https://github.com/openssl/openssl)
+ [sqlite3](https://www.sqlite.org/index.html)
+ [wintoast](https://github.com/mohabouje/WinToast)
+ [wxWidgets 3.1.5](https://github.com/wxWidgets/wxWidgets)
+ [zlib](https://github.com/madler/zlib)

### Installer

Winpinator uses NSIS as its install system. Currently, only one additional NSIS plugin is required for the script to compile - [inetc](https://nsis.sourceforge.io/Inetc_plug-in).

## Translations

Winpinator makes use of wxWidgets' built-in gettext implementation, so it should be localizable using standard set of gettext tools (like [Poedit](https://poedit.net/)). POT template file is available at [po/winpinator.pot](po/winpinator.pot). To make language appear in preferences, an additional line is required to be added in [res/to_copy/Languages.xml](res/to_copy/Languages.xml). Its format is:
```
    <Language code="[ISO language code, e.g de_DE]" name="[Local name of the language, e.g. Deutsch]" flag="[flag filename, e.g. de.png]" 
```
After finishing a translation and adding this line to `Languages.xml`, please submit a Pull Request.

## License

GNU General Public License version 3, available [here](https://www.gnu.org/licenses/gpl-3.0.html)