@echo off

python .\pipeline\assets.py
python .\pipeline\scene.py
python .\src\net\rpc_parse.py .\src\net\messages.rpc .\src\net
if not exist build mkdir build
pushd src
clang -g -std=c++17 ./fracas_server.cpp -o ../build/fracas_server.exe
clang -g -std=c++17 ./fracas_client.cpp -I../generated ./glad/src/glad.cpp  -I ./glad/include -I ./glfw/include .\glfw\lib-clang\glfw3.lib -o ../build/fracas_client.exe
popd