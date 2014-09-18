set(LIBRARY_SOURCE_FILES
  network_service.cpp
  player.cpp
  notifier.cpp
  duck_hunt.cpp
  )

set(EXECUTABLE_SOURCE_FILES
  main.cpp
  services.cpp
  )

add_library(yarrrserverlib STATIC ${LIBRARY_SOURCE_FILES})
add_executable(yarrrserver ${EXECUTABLE_SOURCE_FILES})

set(LIB_YARRR "-Wl,--whole-archive -lyarrr -Wl,--no-whole-archive")
target_link_libraries(yarrrserver yarrrserverlib thelog thenet thetime thectci pthread ${LIB_YARRR} ${LIBS} theconf )

install(TARGETS yarrrserver DESTINATION bin)

