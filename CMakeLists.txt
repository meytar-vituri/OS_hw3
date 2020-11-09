cmake_minimum_required(VERSION 3.17)
project(OS3 C)

set(CMAKE_C_STANDARD 11)

include_directories(/usr/src/linux-headers-5.0.0-32/include)

add_executable(OS3 message_slot.c message_reader.c message_sender.c message_slot.h)
