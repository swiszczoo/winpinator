@echo off

REM 
REM generate-protobuf.bat
REM 
REM This script regenerates all protobuf c++ sources
REM and headers based on contents of warp.proto file
REM
REM This batch file outputs to src/proto-gen directory
REM

rmdir /s /q "../src/proto-gen"
mkdir "../src/proto-gen"
..\vcpkg_installed\x64-windows\x64-windows\tools\protobuf\protoc.exe -I . --grpc_out ../src/proto-gen --plugin=protoc-gen-grpc=../vcpkg_installed/x64-windows/x64-windows/tools/grpc/grpc_cpp_plugin.exe warp.proto
..\vcpkg_installed\x64-windows\x64-windows\tools\protobuf\protoc.exe -I . --cpp_out ../src/proto-gen warp.proto
echo All done...

