#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)

set(LIB_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../lib)

# NORDIC SDK APP START
target_sources(app PRIVATE
	src/main.c
	src/model_handler.c
	src/model_level_cli_aa.c
	src/model_sensor_cli_aa.c
	${LIB_ROOT}/src/vnd_unit_control_aa.c
	${LIB_ROOT}/src/storage_aa.c
	${LIB_ROOT}/src/vnd_activation_aa.c)


target_include_directories(app PRIVATE include
	${LIB_ROOT}/include)
# NORDIC SDK APP END
