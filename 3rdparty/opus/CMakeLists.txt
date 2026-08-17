cmake_minimum_required(VERSION 3.16)
project(opus LANGUAGES C)

add_definitions(
    -DOPUS_BUILD
    -DVAR_ARRAYS
    -DFIXED_POINT
    -DCUSTOM_MODES
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/celt
    ${CMAKE_CURRENT_SOURCE_DIR}/silk
    ${CMAKE_CURRENT_SOURCE_DIR}/silk/fixed
)

file(GLOB OPUS_SOURCES
    "src/opus.c"
    "src/opus_decoder.c"
    "src/opus_encoder.c"
    "src/opus_multistream.c"
    "src/opus_multistream_decoder.c"
    "src/opus_multistream_encoder.c"
    "src/repacketizer.c"
    "src/analysis.c"
    "src/mlp.c"
    "src/mlp_data.c"
    "celt/*.c"
    "silk/*.c"
    "silk/fixed/*.c"
)

list(FILTER OPUS_SOURCES EXCLUDE REGEX ".*demo.*")
list(FILTER OPUS_SOURCES EXCLUDE REGEX ".*test.*")

add_library(opus STATIC ${OPUS_SOURCES})
