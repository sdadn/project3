cmake_minimum_required(VERSION 3.6)
project(project3)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
    buddy.c
    buddy.h
    list.h
    Makefile
    simulator.c)

add_executable(project3 ${SOURCE_FILES})