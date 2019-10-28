cmake_minimum_required(VERSION 3.14)
project(ass4 C)

set(CMAKE_C_STANDARD 99)

add_executable(ass4 main.c network.c network.h linkedLists.h linkedLists.c util.c util.h messaging.c messaging.h channel.c channel.h)