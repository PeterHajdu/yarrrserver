# required for igloo:
add_definitions(-Wno-deprecated-declarations)

set(TEST_SOURCE_FILES
    test_runner.cpp
    test_services.cpp
    test_notifier.cpp
    test_world.cpp
    test_player.cpp
    test_command_handler.cpp
    test_request_ship.cpp
    test_mission.cpp
    test_login_handler.cpp
    )


add_executable(test_runner EXCLUDE_FROM_ALL ${TEST_SOURCE_FILES})

set(LIB_YARRR "-Wl,--whole-archive -lyarrr -Wl,--no-whole-archive")
target_link_libraries(test_runner yarrrserverlib thelog thenet thectci ${LIB_YARRR} ${LIBS} theconf themodel thetime lua hiredis)

get_target_property(TEST_RUNNER_BIN test_runner LOCATION)

find_program( VALGRIND_PATH valgrind )

if(VALGRIND_PATH)
  set(RUNNER
    valgrind --leak-check=full --show-reachable=yes --log-file="valgrind.log"
    )
endif()

add_custom_target(test DEPENDS test_runner)
add_custom_command(TARGET test COMMAND ${RUNNER} ${TEST_RUNNER_BIN} --verbose DEPENDS test_runner)
