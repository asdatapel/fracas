@echo off

if not exist build mkdir build
pushd src
clang -g -std=c++17 ./fracas_server.cpp -o ../build/fracas_server.exe
clang -g -std=c++17 ./fracas_client.cpp -I../generated ./glad/src/glad.cpp  -I ./glad/include -I ./glfw/include .\glfw\lib-clang\glfw3.lib -o ../build/fracas_client.exe
popd