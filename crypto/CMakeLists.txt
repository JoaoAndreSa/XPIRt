cmake_minimum_required(VERSION 2.6.0)

add_subdirectory("prng")

set(CMAKE_CXX_FLAGS "-std=c++11 -fopenmp")
set (CMAKE_EXE_LINKER_FLAGS "-fopenmp -lnfllib")
include_directories(.. ../external_components/NFLlib/include)


add_library(pir_crypto STATIC
    AbstractPublicParameters.cpp
    FV2048_124.cpp
    Bitsplit.cpp
    HomomorphicCrypto.cpp
    HomomorphicCryptoFactory_internal.cpp
    LatticesBasedCryptosystem.cpp
    NFLLWE.cpp
    NFLLWEPublicParameters.cpp
    NFLFV.cpp
    NFLFVPublicParameters.cpp
    NFLParams.cpp
    NFLlib_old.cpp
    NoCryptography.cpp
    NoCryptographyPublicParameters.cpp
    PaillierAdapter.cpp
    PaillierKeys.cpp
    PaillierPrivateParameters.cpp
    PaillierPublicParameters.cpp)

if (APPLE)
target_link_libraries(pir_crypto pir_crypto_prng )
else()
target_link_libraries(pir_crypto pir_crypto_prng gmp)
endif()

