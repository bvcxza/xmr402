cmake_minimum_required(VERSION 3.30)
project(xmr402 CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(MONERO_DIR "${CMAKE_SOURCE_DIR}/external/monero" CACHE STRING "Monero project directory")
set(MONERO_BUILD "${CMAKE_SOURCE_DIR}/build_xmr" CACHE STRING "Monero project build directory")
list(APPEND CMAKE_MODULE_PATH "${MONERO_DIR}/cmake")

if(NOT MSVC)
	add_compile_options("-Wall" "-Wextra" "$<$<CONFIG:RELEASE>:-O3>")
endif()

message(STATUS "Build environment:
	System processor: ${CMAKE_SYSTEM_PROCESSOR}
	CXX compiler:     ${CMAKE_CXX_COMPILER} (${CMAKE_CXX_COMPILER_ID})
")

find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
	if(UNIX)
		set(OPENSSL_LIBRARIES "${OPENSSL_LIBRARIES};${CMAKE_DL_LIBS};${CMAKE_THREAD_LIBS_INIT}")
	endif()
	message(STATUS "Using OpenSSL include dir at ${OPENSSL_INCLUDE_DIR}")
	message(STATUS "Using OpenSSL libs: ${OPENSSL_LIBRARIES}")
endif()

find_library(SODIUM_LIBRARY sodium REQUIRED)
if(SODIUM_LIBRARY)
	message(STATUS "Using libsodium library at ${SODIUM_LIBRARY}")
	find_path(SODIUM_INCLUDE_PATH sodium/crypto_verify_32.h)
	if (SODIUM_INCLUDE_PATH)
		message(STATUS "SODIUM_INCLUDE_PATH: ${SODIUM_INCLUDE_PATH}")
	else()
		message(FATAL_ERROR "Could not find required sodium/crypto_verify_32.h")
	endif()
endif()

find_package(Unbound)
if(NOT UNBOUND_INCLUDE_DIR)
	message(FATAL_ERROR "Could not find libunbound")
else()
	message(STATUS "Found libunbound include (unbound.h) in ${UNBOUND_INCLUDE_DIR}")
	if(UNBOUND_LIBRARIES)
		message(STATUS "Found libunbound library")
		if (WIN32)
			add_library(unbound STATIC IMPORTED)
		else()
			add_library(unbound SHARED IMPORTED)
		endif()
		set_target_properties(unbound PROPERTIES IMPORTED_LOCATION ${UNBOUND_LIBRARIES})
	else()
		message(FATAL_ERROR "Found libunbound includes, but could not find libunbound library. Please make sure you have installed libunbound or libunbound-dev or the equivalent")
	endif()
endif()

find_package(Threads REQUIRED)

cmake_policy(SET CMP0167 NEW)
set(Boost_VERBOSE ON)
set(Boost_USE_MULTITHREADED ON)
find_package(Boost 1.73 REQUIRED chrono date_time filesystem program_options regex serialization system thread json)
message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")

set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")

find_library(XMR_WALLET wallet)
if(XMR_WALLET)
	message(STATUS "Using monero wallet lib: ${XMR_WALLET}")
endif()

# libwallet-crypto.a provides x86_64 asm for some wallet functions
if (EXISTS ${MONERO_BUILD}/src/crypto/wallet/libwallet-crypto.a)
	add_library(wallet_crypto_lib STATIC IMPORTED)
	set_target_properties(wallet_crypto_lib PROPERTIES IMPORTED_LOCATION
		${MONERO_BUILD}/src/crypto/wallet/libwallet-crypto.a)
	set(wallet_crypto wallet_crypto_lib)
endif()

add_library(lmdb STATIC IMPORTED)
set_target_properties(lmdb PROPERTIES IMPORTED_LOCATION
	${MONERO_BUILD}/external/db_drivers/liblmdb/liblmdb.a)

add_library(epee STATIC IMPORTED)
set_target_properties(epee PROPERTIES IMPORTED_LOCATION
	${MONERO_BUILD}/contrib/epee/src/libepee.a)

add_library(rpc_base STATIC IMPORTED)
set_target_properties(rpc_base PROPERTIES IMPORTED_LOCATION
	${MONERO_BUILD}/src/rpc/librpc_base.a)

add_library(net STATIC IMPORTED)
set_target_properties(net PROPERTIES IMPORTED_LOCATION
	${MONERO_BUILD}/src/net/libnet.a)

add_library(hardforks STATIC IMPORTED)
set_target_properties(hardforks PROPERTIES IMPORTED_LOCATION
	${MONERO_BUILD}/src/hardforks/libhardforks.a)

add_library(easylogging STATIC IMPORTED)
set_target_properties(easylogging PROPERTIES IMPORTED_LOCATION
	${MONERO_BUILD}/external/easylogging++/libeasylogging.a)

add_library(cryptonote_core STATIC IMPORTED)
set_target_properties(cryptonote_core PROPERTIES IMPORTED_LOCATION
	${MONERO_BUILD}/src/cryptonote_core/libcryptonote_core.a)

add_library(cryptonote_basic STATIC IMPORTED)
set_target_properties(cryptonote_basic PROPERTIES IMPORTED_LOCATION
	${MONERO_BUILD}/src/cryptonote_basic/libcryptonote_basic.a)

add_library(cryptonote_format_utils_basic STATIC IMPORTED)
set_target_properties(cryptonote_format_utils_basic PROPERTIES IMPORTED_LOCATION
	${MONERO_BUILD}/src/cryptonote_basic/libcryptonote_format_utils_basic.a)

add_library(mnemonics STATIC IMPORTED)
set_target_properties(mnemonics PROPERTIES IMPORTED_LOCATION
	${MONERO_BUILD}/src/mnemonics/libmnemonics.a)

add_library(common STATIC IMPORTED)
set_target_properties(common PROPERTIES IMPORTED_LOCATION
	${MONERO_BUILD}/src/common/libcommon.a)

add_library(cncrypto STATIC IMPORTED)
set_target_properties(cncrypto PROPERTIES IMPORTED_LOCATION
	${MONERO_BUILD}/src/crypto/libcncrypto.a)

add_library(ringct STATIC IMPORTED)
set_target_properties(ringct PROPERTIES IMPORTED_LOCATION
	${MONERO_BUILD}/src/ringct/libringct.a)

add_library(ringct_basic STATIC IMPORTED)
set_target_properties(ringct_basic PROPERTIES IMPORTED_LOCATION
	${MONERO_BUILD}/src/ringct/libringct_basic.a)

add_library(blockchain_db STATIC IMPORTED)
set_target_properties(blockchain_db PROPERTIES IMPORTED_LOCATION
	${MONERO_BUILD}/src/blockchain_db/libblockchain_db.a)

add_library(blocks STATIC IMPORTED)
set_target_properties(blocks PROPERTIES IMPORTED_LOCATION
	${MONERO_BUILD}/src/blocks/libblocks.a)

add_library(checkpoints STATIC IMPORTED)
set_target_properties(checkpoints PROPERTIES IMPORTED_LOCATION
	${MONERO_BUILD}/src/checkpoints/libcheckpoints.a)

add_library(device STATIC IMPORTED)
set_target_properties(device PROPERTIES IMPORTED_LOCATION
	${MONERO_BUILD}/src/device/libdevice.a)

add_library(device_trezor STATIC IMPORTED)
set_target_properties(device_trezor PROPERTIES IMPORTED_LOCATION
	${MONERO_BUILD}/src/device_trezor/libdevice_trezor.a)

add_library(multisig STATIC IMPORTED)
set_target_properties(multisig PROPERTIES IMPORTED_LOCATION
	${MONERO_BUILD}/src/multisig/libmultisig.a)

add_library(version STATIC IMPORTED)
set_target_properties(version PROPERTIES IMPORTED_LOCATION
	${MONERO_BUILD}/src/libversion.a)

add_library(randomx STATIC IMPORTED)
set_target_properties(randomx PROPERTIES IMPORTED_LOCATION
	${MONERO_BUILD}/external/randomx/librandomx.a)

add_executable(${CMAKE_PROJECT_NAME}
	src/main.cpp
	src/validate.cpp
)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
	Boost::headers
	${MONERO_DIR}/src
	${MONERO_DIR}/contrib/epee/include
	${MONERO_DIR}/external
	${MONERO_DIR}/external/easylogging++
	${MONERO_DIR}/src/crypto
	${OPENSSL_INCLUDE_DIR}
)
target_link_libraries(${CMAKE_PROJECT_NAME}
	Threads::Threads
	${XMR_WALLET}
	rpc_base
	net
	lmdb
	unbound
	easylogging
	cryptonote_core
	cryptonote_basic
	cryptonote_format_utils_basic
	mnemonics
	ringct
	ringct_basic
	common
	cncrypto
	blockchain_db
	blocks
	checkpoints
	device
	device_trezor
	multisig
	version
	randomx
	epee
	hardforks
	${wallet_crypto}
	${OPENSSL_LIBRARIES}
	${SODIUM_LIBRARY}
	Boost::chrono
	Boost::date_time
	Boost::filesystem
	Boost::program_options
	Boost::regex
	Boost::serialization
	Boost::system
	Boost::thread
	Boost::json
)

# https://stackoverflow.com/questions/73435637/how-can-i-fix-usr-bin-ld-warning-trap-o-missing-note-gnu-stack-section-imp
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	target_link_options(${CMAKE_PROJECT_NAME} PRIVATE "-z" "noexecstack")
endif()

# Install
install(TARGETS ${CMAKE_PROJECT_NAME})

message(STATUS "Summary of build options:
	CXX compiler:                 ${CMAKE_CXX_COMPILER} (${CMAKE_CXX_COMPILER_ID})
	C_FLAGS:                      ${CMAKE_C_FLAGS}
	CXX_FLAGS:                    ${CMAKE_CXX_FLAGS}
	CMAKE_EXE_LINKER_FLAGS:       ${CMAKE_EXE_LINKER_FLAGS}
	CMAKE_THREAD_LIBS_INIT        ${CMAKE_THREAD_LIBS_INIT}
	CMAKE_INSTALL_PREFIX          ${CMAKE_INSTALL_PREFIX}
	MONERO_DIR                    ${MONERO_DIR}
	MONERO_BUILD                  ${MONERO_BUILD}
")
