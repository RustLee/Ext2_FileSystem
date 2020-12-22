
cmake_minimum_required(VERSION 3.0.0)

project(file_system VERSION 0.1.0)


include_directories(./include)

aux_source_directory(./src DIR_SRCS)

add_executable(main ${DIR_SRCS})

target_link_libraries(main)