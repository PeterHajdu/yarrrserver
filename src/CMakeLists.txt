set(LIBRARY_SOURCE_FILES
  network_service.cpp
  player.cpp
  )

set(EXECUTABLE_SOURCE_FILES
  main.cpp
  services.cpp
  )

add_library(yarrrserverlib STATIC ${LIBRARY_SOURCE_FILES})

add_executable(yarrrserver ${EXECUTABLE_SOURCE_FILES})
target_link_libraries(yarrrserver yarrrserverlib yarrr thelog thenet thetime thectci pthread ${LIBS})
install(TARGETS yarrrserver DESTINATION bin)

