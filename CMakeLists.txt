cmake_minimum_required(VERSION 3.16)
project(DeciBeliever VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------- third-party headers ----------
# dr_wav  – single-header WAV parser
add_library(dr_wav INTERFACE)
target_include_directories(dr_wav INTERFACE ${CMAKE_SOURCE_DIR}/third_party/dr_wav)

# kissfft – small, portable FFT
add_library(kissfft STATIC
    ${CMAKE_SOURCE_DIR}/third_party/kissfft/kiss_fft.c
    ${CMAKE_SOURCE_DIR}/third_party/kissfft/kiss_fftr.c
)
target_include_directories(kissfft PUBLIC ${CMAKE_SOURCE_DIR}/third_party/kissfft)

# nlohmann/json is fetched via FetchContent (header-only)
include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(json)

# ---------- main library (shared between app and tests) ----------
add_library(DeciBeliever_lib STATIC
    src/audio_io.cpp
    src/metrics.cpp
    src/effects.cpp
    src/verifier.cpp
    src/report.cpp
    src/config.cpp
    src/generator.cpp

)
target_include_directories(audio_verifier_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(DeciBeliever_lib PUBLIC dr_wav kissfft nlohmann_json::nlohmann_json)

# ---------- executable ----------
add_executable(DeciBeliever src/main.cpp)
target_link_libraries(DeciBeliever PRIVATE DeciBeliever_lib)

# ---------- optional PortAudio ----------
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
  pkg_check_modules(PORTAUDIO QUIET portaudio-2.0)
endif()
if(PORTAUDIO_FOUND)
  target_compile_definitions(audio_verifier_lib PUBLIC HAS_PORTAUDIO)
  target_include_directories(audio_verifier_lib PUBLIC ${PORTAUDIO_INCLUDE_DIRS})
  target_link_libraries(audio_verifier_lib PUBLIC ${PORTAUDIO_LIBRARIES})
  message(STATUS "PortAudio found – live capture enabled")
else()
  message(STATUS "PortAudio not found – live capture disabled")
endif()

# ---------- tests ----------
enable_testing()
add_subdirectory(tests)
