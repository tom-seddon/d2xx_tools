add_executable(d2xx_recv
  d2xx_recv.cpp)
target_link_libraries(d2xx_recv ftdi_d2xx_lib shared_lib d2xx_shared_lib)
