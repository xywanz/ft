// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <ctime>
#include <cstring>
#include <iostream>


int main(int argc, char** argv) {
  struct tm _tm;
  strptime("20200414 20:02:38", "%Y%m%d %H:%M:%S", &_tm);
  std::cout << _tm.tm_hour << " " << _tm.tm_min << " " << _tm.tm_sec << std::endl;
}
