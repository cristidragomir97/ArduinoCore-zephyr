# SPDX-License-Identifier: Apache-2.0
zephyr_include_directories(.)

if(NOT DEFINED ARDUINO_BUILD_PATH)
	zephyr_sources_ifdef(CONFIG_FILE_SYSTEM
		StorageError.cpp
		StorageFile.cpp
		StorageFolder.cpp
	)
endif()
