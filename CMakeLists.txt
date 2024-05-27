cmake_minimum_required(VERSION 3.5)
project(vc_project)

set(OpenCV_DIR C:/opencv/build/x64/vc16/lib)

#Encontrar a lib do opencv
find_package(OpenCV REQUIRED)

#Incluir os diretorios do openCV
include_directories(${OpenCV_INCLUDE_DIRS})

#Crirar o executavel
add_executable(vc_project main.cpp vc.c)

#Linkar o opencv
target_link_libraries(vc_project ${OpenCV_LIBS})