# required for igloo:
add_definitions(-Wno-deprecated-declarations)
include_directories(igloo)

set(TEST_SOURCE_FILES
    test_runner.cpp
    test_services.cpp
    test_notifier.cpp
    test_object_factory.cpp
    test_shape_creation.cpp
    test_lua_engine.cpp
    test_lua_setup.cpp
    test_world.cpp
    )


add_executable(test_runner EXCLUDE_FROM_ALL ${TEST_SOURCE_FILES})

set(LIB_YARRR "-Wl,--whole-archive -lyarrr -Wl,--no-whole-archive")
target_link_libraries(test_runner yarrrserverlib thelog theconf thenet thectci ${LIB_YARRR} ${LIBS} theconf lua)

get_target_property(TEST_RUNNER_BIN test_runner LOCATION)

add_custom_target(test DEPENDS test_runner)
add_custom_command(TARGET test COMMAND valgrind --leak-check=full --show-reachable=yes --log-file="valgrind.log" ${TEST_RUNNER_BIN} --verbose DEPENDS test_runner)
