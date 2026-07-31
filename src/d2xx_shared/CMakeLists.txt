add_library(d2xx_shared_lib
  d2xx_shared.cpp d2xx_shared.h d2xx_shared.inl)
target_include_directories(d2xx_shared_lib PUBLIC .)
target_link_libraries(d2xx_shared_lib PUBLIC ftdi_d2xx_lib shared_lib)
