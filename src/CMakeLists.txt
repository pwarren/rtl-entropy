set(LIBSRC fips.c fips.h log.c log.h util.c util.h)

add_library(rtlentropylib ${LIBSRC})

add_executable(rtl_entropy rtl_entropy.c)
add_executable(brf_entropy brf_entropy.c)

target_link_libraries(rtl_entropy rtlentropylib ${OPENSSL_LIBRARIES} ${LibCAP_LIBRARY} ${LIBRTLSDR_LIBRARIES})

target_link_libraries(brf_entropy rtlentropylib ${OPENSSL_LIBRARIES} ${LibCAP_LIBRARY} ${LIBBLADERF_LIBRARIES})

set(INSTALL_TARGETS rtl_entropy brf_entropy)