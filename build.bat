@echo off

python .\pipeline\assets.py
python .\pipeline\scene.py
python .\src\net\rpc_parse.py .\src\net\messages.rpc .\src\net
if not exist build mkdir build
pushd src
clang -g -std=c++17 ./fracas_server.cpp -o ../build/fracas_server.exe
popd

clang -g -std=c++17 ./src/fracas_client.cpp ^
    -I./generated -I ./thirdparty/glad/include -I ./thirdparty/glfw/include -I ./thirdparty ^
    ./thirdparty/glad/src/glad.cpp  ./thirdparty/glfw/lib-clang/glfw3.lib -o ./build/fracas_client.exe