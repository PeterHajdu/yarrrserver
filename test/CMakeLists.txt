# required for igloo:
add_definitions(-Wno-deprecated-declarations)
include_directories(igloo)

set(TEST_SOURCE_FILES
    test_runner.cpp
    test_services.cpp
    test_players.cpp
    )


add_executable(test_runner EXCLUDE_FROM_ALL ${TEST_SOURCE_FILES})

set(LIB_YARRR "-Wl,--whole-archive -lyarrr -Wl,--no-whole-archive")
target_link_libraries(test_runner yarrrserverlib thelog theconf thenet thectci ${LIB_YARRR} ${LIBS} theconf)

get_target_property(TEST_RUNNER_BIN test_runner LOCATION)

add_custom_target(test DEPENDS test_runner)
add_custom_command(TARGET test COMMAND valgrind --leak-check=full --show-reachable=yes --log-file="valgrind.log" ${TEST_RUNNER_BIN} --verbose DEPENDS test_runner)
