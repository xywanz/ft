#!/bin/bash
# 请确保在ft项目根目录下执行

mkdir update && cd update

git clone https://github.com/jbeder/yaml-cpp.git --depth=1
git clone https://github.com/gabime/spdlog.git --depth=1
git clone https://github.com/redis/hiredis.git --depth=1

mkdir -p yaml-cpp/build && cd yaml-cpp/build && cmake .. && make -j8
cd ../..
cp -rf yaml-cpp/include ../dependencies/yaml-cpp/
cp -rf yaml-cpp/build/libyaml-cpp.a ../dependencies/yaml-cpp/lib/

mkdir -p spdlog/build && cd spdlog/build && cmake .. && make -j8
cd ../..
cp -rf spdlog/include ../dependencies/spdlog/
cp -rf spdlog/build/libspdlog.a ../dependencies/spdlog/lib/

mkdir -p hiredis/build && cd hiredis/build && cmake .. && make -j8
cd ../..
cp -rf hiredis/*.h ../dependencies/hiredis/include
cp -rf hiredis/build/libhiredis.so* ../dependencies/hiredis/lib/

cd ..
rm -rf update
