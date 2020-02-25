set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(server test-server.cpp thread-pool.cpp)
target_link_libraries(server -lpthread)
add_executable(client test-client.cpp)
