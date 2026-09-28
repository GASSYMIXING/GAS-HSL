cmake_minimum_required(VERSION 3.21)
project(gas-hsl VERSION 0.5.0 LANGUAGES C)

find_package(libobs REQUIRED)

add_library(gas-hsl MODULE src/plugin-main.c)
target_link_libraries(gas-hsl PRIVATE OBS::libobs)
if(WIN32)
  target_link_libraries(gas-hsl PRIVATE user32 gdi32)
endif()
set_target_properties(gas-hsl PROPERTIES PREFIX "" OUTPUT_NAME "gas-hsl" C_STANDARD 11)

install(TARGETS gas-hsl RUNTIME DESTINATION gas-hsl/bin/64bit LIBRARY DESTINATION gas-hsl/bin/64bit)
install(DIRECTORY data/ DESTINATION gas-hsl/data)
