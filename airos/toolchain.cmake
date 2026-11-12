# Cross toolchain for Telegram on air/OS. airos/build.sh passes
# AIROS_CROSS (the compiler prefix of the air/OS SDK), AIROS_SYSROOT
# (airos/setup-sysroot.sh's tree) and AIROS_ARCH.
set(CMAKE_SYSTEM_NAME Haiku)
set(CMAKE_SYSTEM_PROCESSOR ${AIROS_ARCH})
set(CMAKE_SYSROOT ${AIROS_SYSROOT})
set(CMAKE_C_COMPILER ${AIROS_CROSS}gcc)
set(CMAKE_CXX_COMPILER ${AIROS_CROSS}g++)
set(CMAKE_ASM_COMPILER ${AIROS_CROSS}gcc)
set(CMAKE_AR ${AIROS_CROSS}ar CACHE FILEPATH "")
set(CMAKE_RANLIB ${AIROS_CROSS}ranlib CACHE FILEPATH "")
set(CMAKE_STRIP ${AIROS_CROSS}strip CACHE FILEPATH "")
set(CMAKE_FIND_ROOT_PATH ${AIROS_SYSROOT}/boot/system ${AIROS_SYSROOT})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
# Haiku keeps libraries in develop/lib and headers in develop/headers
list(APPEND CMAKE_SYSTEM_PREFIX_PATH /boot/system)
set(CMAKE_SYSTEM_LIBRARY_PATH /boot/system/develop/lib /boot/system/lib)
set(CMAKE_SYSTEM_INCLUDE_PATH /boot/system/develop/headers)
