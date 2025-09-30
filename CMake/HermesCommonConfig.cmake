# -----------------------------------------------------------------------------
# Find all packages needed by Hermes
# -----------------------------------------------------------------------------
# This is for compatability with SPACK
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Chimaera
find_package(Chimaera CONFIG REQUIRED)
message(STATUS "found chimaera at ${CHIMAERA_INCLUDE_DIR}")

# Catch2
find_package(Catch2 3.0.1 REQUIRED)
message(STATUS "found catch2.h at ${Catch2_CXX_INCLUDE_DIRS}")

# MPICH
if(BUILD_MPI_TESTS)
  find_package(MPI REQUIRED COMPONENTS C CXX)
  message(STATUS "found mpi.h at ${MPI_CXX_INCLUDE_DIRS}")
endif()

# OpenMP
if(BUILD_OpenMP_TESTS)
  find_package(OpenMP REQUIRED COMPONENTS C CXX)
  message(STATUS "found omp.h at ${OpenMP_CXX_INCLUDE_DIRS}")
endif()

# Boost
find_package(Boost REQUIRED COMPONENTS regex system filesystem fiber REQUIRED)
message(STATUS "found boost at ${Boost_INCLUDE_DIRS}")

# Libelf
pkg_check_modules(libelf REQUIRED libelf)
message(STATUS "found libelf at ${libelf_INCLUDE_DIRS}")

# LIBAIO
# find_library(LIBAIO_LIBRARY NAMES aio)
# if(LIBAIO_LIBRARY)
# message(STATUS "found libaio at ${LIBAIO_LIBRARY}")
# else()
# set(LIBAIO_LIBRARY aio)
# message(STATUS "Assuming it was installed with our aio spack")
# endif()
