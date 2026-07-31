add_executable(d2xx_send
  d2xx_send.cpp)
target_link_libraries(d2xx_send ftdi_d2xx_lib shared_lib d2xx_shared_lib)
