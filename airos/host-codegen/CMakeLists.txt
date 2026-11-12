# The code generators Telegram's build runs (codegen_emoji, codegen_lang,
# codegen_style), built for the build machine: a cross build cannot run the
# ones it makes. airos/build.sh builds this with a host Qt of the target's Qt
# version and hands the programs to the cross build (TDESKTOP_HOST_CODEGEN).
cmake_minimum_required(VERSION 3.16)
project(tdesktop_host_codegen LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC OFF)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Svg)

get_filename_component(tg ${CMAKE_CURRENT_SOURCE_DIR}/../../Telegram REALPATH)
set(cg ${tg}/codegen/codegen)

add_library(codegen_host_common STATIC
    ${cg}/common/basic_tokenized_file.cpp
    ${cg}/common/checked_utf8_string.cpp
    ${cg}/common/clean_file.cpp
    ${cg}/common/cpp_file.cpp
    ${cg}/common/logging.cpp
    ${tg}/lib_base/base/crc32hash.cpp
)
target_include_directories(codegen_host_common PUBLIC
    ${tg}/codegen
    ${tg}/lib_base
    ${tg}/lib_crl
    ${tg}/lib_rpl
    ${tg}/ThirdParty/GSL/include
    ${tg}/ThirdParty/range-v3/include
    ${tg}/ThirdParty/expected/include
)
target_compile_definitions(codegen_host_common PUBLIC
    QT_NO_KEYWORDS QT_NO_CAST_FROM_BYTEARRAY)
target_link_libraries(codegen_host_common PUBLIC Qt6::Core Qt6::Gui Qt6::Svg)

foreach(tool emoji lang style)
    file(GLOB sources ${cg}/${tool}/*.cpp)
    add_executable(codegen_${tool} ${sources})
    target_link_libraries(codegen_${tool} PRIVATE codegen_host_common)
endforeach()
install(TARGETS codegen_emoji codegen_lang codegen_style RUNTIME DESTINATION bin)
