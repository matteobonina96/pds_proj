cmake_minimum_required(VERSION 3.17)
project(proje)

set(CMAKE_CXX_STANDARD 14)

add_executable(proje hash.h server/utility/FileManageServer.cpp client/utility/ClientStructureManage.cpp server/utility/ServerStructureManage.cpp client/ClientFS_grosso/file.cpp client/ClientFS_grosso/file.h server/users.cpp server/users.h client/ClientFS1/Slides/new_dira/yes.cpp client/ClientFS1/Slides/new_dira/yes.h client/SocketCommunication.cpp client/SocketCommunication.h client/main.cpp client/ClientC.cpp client/ClientC.h client/StructureC.cpp client/StructureC.h HashC.cpp HashC.h server/ServerStructureC.cpp server/ServerStructureC.h server/ServerC.cpp server/ServerC.h server/main.cpp server/utility/ServerTest.cpp server/Server.cpp)