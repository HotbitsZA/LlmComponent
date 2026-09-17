cmake_minimum_required(VERSION 3.16)
project(LocalBrainIntegrationTest CXX)

# Force modern C++ standards required by cBaseWorker_V2
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Locate Native System Multi-threading libraries
find_package(Threads REQUIRED)

# 2. Locate Homebrew-installed jsoncpp cleanly via its native package config modules
find_package(jsoncpp REQUIRED)

# 3. Locate System-installed cURL libraries cleanly via its native package config modules
find_package(CURL REQUIRED)

# 4. Group your project implementation source targets together
add_executable(${PROJECT_NAME} 
    main_brain_test.cpp 
    cLlmBrainWorker.cpp
    cOllamaProvider.cpp
    ../ThreadComponent/cHTTPClient.cpp
)

# 5. Handle directory mappings automatically using found variables
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThreadComponent
    ${jsoncpp_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
)

# 6. Link all library modules together into your executable target link phase
target_link_libraries(${PROJECT_NAME} PRIVATE
    jsoncpp_lib                  # Target name provided by found Homebrew jsoncpp modules
    ${CURL_LIBRARIES}
    Threads::Threads              
)

# Enable build warning checks
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
