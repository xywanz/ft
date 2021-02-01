#!/bin/bash
# 请确保在ft项目根目录下执行

mkdir update && cd update

git clone https://github.com/jbeder/yaml-cpp.git --depth=1
git clone https://github.com/gabime/spdlog.git --depth=1
git clone https://github.com/redis/hiredis.git --depth=1

mkdir -p yaml-cpp/build && cd yaml-cpp/build
cmake -DCMAKE_POSITION_INDEPENDENT_CODE=ON .. && make -j8
cd ../..
cp -rf yaml-cpp/include ../third_party/yaml-cpp/
cp -rf yaml-cpp/build/libyaml-cpp.a ../third_party/yaml-cpp/lib/

mkdir -p spdlog/build && cd spdlog/build
cmake -DCMAKE_POSITION_INDEPENDENT_CODE=ON .. && make -j8
cd ../..
cp -rf spdlog/include ../third_party/spdlog/
cp -rf spdlog/build/libspdlog.a ../third_party/spdlog/lib/

mkdir -p hiredis/build && cd hiredis/build
cmake -DCMAKE_POSITION_INDEPENDENT_CODE=ON .. && make -j8
cd ../..
cp -rf hiredis/*.h ../third_party/hiredis/include
cp -rf hiredis/build/libhiredis.so* ../third_party/hiredis/lib/

cd ..
rm -rf update
