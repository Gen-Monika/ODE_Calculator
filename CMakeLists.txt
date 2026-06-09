cmake_minimum_required(VERSION 3.21)

project(ODE_Calculator VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets WebEngineWidgets)

qt_add_executable(ODE_Calculator WIN32
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/ExpressionParser.h
    src/ExpressionParser.cpp
    src/ExactNumber.h
    src/ExactNumber.cpp
    src/OdeSolver.h
    src/OdeSolver.cpp
)

target_link_libraries(ODE_Calculator PRIVATE Qt6::Widgets Qt6::WebEngineWidgets)

set(KATEX_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/katex")
file(GLOB KATEX_FONT_FILES CONFIGURE_DEPENDS "${KATEX_DIR}/fonts/*.woff2")
qt_add_resources(ODE_Calculator "katex_assets"
    PREFIX "/katex"
    BASE "${KATEX_DIR}"
    FILES
        "${KATEX_DIR}/katex.min.css"
        "${KATEX_DIR}/katex.min.js"
        ${KATEX_FONT_FILES}
)

if (MSVC)
    target_compile_options(ODE_Calculator PRIVATE /permissive- /Zc:__cplusplus /utf-8)
else()
    target_compile_options(ODE_Calculator PRIVATE -finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()
