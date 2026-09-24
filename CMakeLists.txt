cmake_minimum_required(VERSION 3.16)
project(LocalBrainIntegrationTest CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(CURL REQUIRED)

# Build options
option(BUILD_TESTING "Build the unit tests" ON)

# ---------------------------------------------------------------------------
# jsoncpp via pkg-config first (validated include dir), falling back to the
# upstream CMake config module used by Homebrew. Keeps non-Homebrew builds
# working without a hard dependency on the exported config.
# ---------------------------------------------------------------------------
pkg_check_modules(JSONCPP QUIET IMPORTED_TARGET jsoncpp)
if(JSONCPP_FOUND AND EXISTS "${JSONCPP_INCLUDE_DIRS}/json/json.h")
  set(JSONCPP_IMPORTED TRUE)
else()
  set(JSONCPP_IMPORTED FALSE)
  find_package(jsoncpp REQUIRED)
endif()

# ---------------------------------------------------------------------------
# Shared static core: provider + payload building + HTTP client
# ---------------------------------------------------------------------------
add_library(llm_core STATIC
    cOllamaProvider.cpp
    llm_payload.cpp
    ../ThreadComponent/cHTTPClient.cpp
)
target_include_directories(llm_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThreadComponent
)
if(JSONCPP_IMPORTED)
  target_include_directories(llm_core PUBLIC ${JSONCPP_INCLUDE_DIRS})
else()
  target_include_directories(llm_core PUBLIC ${jsoncpp_INCLUDE_DIRS})
endif()
target_include_directories(llm_core PUBLIC ${CURL_INCLUDE_DIRS})

set(LLM_CORE_LINK
    Threads::Threads
    ${CURL_LIBRARIES}
)
if(JSONCPP_IMPORTED)
  list(APPEND LLM_CORE_LINK PkgConfig::JSONCPP)
else()
  list(APPEND LLM_CORE_LINK jsoncpp_lib)
endif()

target_link_libraries(llm_core PUBLIC ${LLM_CORE_LINK})

# ---------------------------------------------------------------------------
# Demo executable
# ---------------------------------------------------------------------------
add_executable(${PROJECT_NAME}
    main_brain_test.cpp
    cLlmBrainWorker.cpp
)
target_link_libraries(${PROJECT_NAME} PRIVATE llm_core)

# ---------------------------------------------------------------------------
# Unit tests (pure logic: payload building + memory + base64, no network)
# ---------------------------------------------------------------------------
if(BUILD_TESTING)
  enable_testing()
  add_executable(llm_tests tests/test_llm.cpp)
  target_link_libraries(llm_tests PRIVATE llm_core)
  add_test(NAME llm_tests COMMAND llm_tests)
endif()

foreach(target ${PROJECT_NAME} llm_core llm_tests)
  if(TARGET ${target})
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endforeach()