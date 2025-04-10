cmake_minimum_required(VERSION 3.14)
project(MyApp)

# ——————————————————————————————
# 1) C++ Standard
# ——————————————————————————————
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ——————————————————————————————
# 2) Enable Qt automoc/uic/rcc
# ——————————————————————————————
set(CMAKE_AUTOMOC ON)

# ——————————————————————————————
# 3) Manually Specify OpenCV Paths
# ——————————————————————————————
set(OpenCV_DIR "C:/opencv_build")
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# ——————————————————————————————
# 4) Locate Qt Libraries
# ——————————————————————————————
find_package(Qt6 COMPONENTS Widgets REQUIRED)

# ——————————————————————————————
# 5) Define Executable and Sources
# ——————————————————————————————
set(SOURCES
    src/main.cpp
)

set(HEADERS
    src/mainwindow.h
    src/node_framework.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# ——————————————————————————————
# 6) Include Directories for Libraries
# ——————————————————————————————


# ——————————————————————————————
# 7) Link Against Libraries
# ——————————————————————————————
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt6::Widgets
        ${OpenCV_LIBS}
)



