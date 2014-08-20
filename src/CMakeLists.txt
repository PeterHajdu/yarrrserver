
set(SOURCE_FILES
  main.cpp
  local_event_dispatcher.cpp
  network_service.cpp
  player.cpp
  services.cpp
    )

add_executable(yarrrserver ${SOURCE_FILES})
target_link_libraries(yarrrserver yarrr thelog thenet thetime thectci pthread ${LIBS})
install(TARGETS yarrrserver DESTINATION bin)

