cmake_minimum_required(VERSION 3.17)
project(proje)

set(CMAKE_CXX_STANDARD 14)

add_executable(proje main.cpp hash.h server/FileManage.cpp client/ClientStructureManage.cpp server/ServerStructureManage.cpp client/ClientFS_grosso/file.cpp client/ClientFS_grosso/file.h server/users.cpp server/users.h client/ClientFS1/Slides/new_dira/yes.cpp client/ClientFS1/Slides/new_dira/yes.h)