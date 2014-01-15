set(LIBSRC fips.c fips.h log.c log.h util.c util.h)

add_library(rtlentropylib ${LIBSRC})

if(LIBRTLSDR_FOUND)
  add_executable(rtl_entropy rtl_entropy.c)
  target_link_libraries(rtl_entropy rtlentropylib ${OPENSSL_LIBRARIES} ${LibCAP_LIBRARY} ${LIBRTLSDR_LIBRARIES})
  set(INSTALL_TARGETS rtl_entropy)
endif(LIBRTLSDR_FOUND)

if(LIBBLADERF_FOUND)
  add_executable(brf_entropy brf_entropy.c)
  add_executable(brf2 brf2.c)
  target_link_libraries(brf_entropy rtlentropylib ${OPENSSL_LIBRARIES} ${LibCAP_LIBRARY} ${LIBBLADERF_LIBRARIES})
  target_link_libraries(brf2 rtlentropylib ${OPENSSL_LIBRARIES} ${LIBBLADERF_LIBRARIES})
  set(INSTALL_TARGETS brf_entropy)
endif(LIBBLADERF_FOUND)


if (LIBBLADERF_FOUND AND LIBRTLSDR_FOUND)
  set(INSTALL_TARGETS rtl_entropy brf_entropy)
endif()



install(TARGETS ${INSTALL_TARGETS}
      LIBRARY DESTINATION ${LIB_INSTALL_DIR} # .so/.dylib file
    ARCHIVE DESTINATION ${LIB_INSTALL_DIR} # .lib file
    RUNTIME DESTINATION bin              # .dll file
)
