add_executable(d2xx_list
  d2xx_list.cpp)
target_link_libraries(d2xx_list ftdi_d2xx_lib shared_lib d2xx_shared_lib)
