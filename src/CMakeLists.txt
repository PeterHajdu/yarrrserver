
set(SOURCE_FILES
  main.cpp
    )

add_executable(yarrrserver ${SOURCE_FILES})
target_link_libraries(yarrrserver yarrr pthread ${LIBS})
target_link_libraries(yarrrserver thenet pthread ${LIBS})
install(TARGETS yarrrserver DESTINATION ${deploy_directory}/bin)

