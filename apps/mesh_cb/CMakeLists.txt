#
# Copyright (c) 2021 Nordic Semiconductor ASA
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
	src/lb_service_handler.c
	src/model_handler.c
	${LIB_ROOT}/mesh_model/src/storage_aa.c
	${LIB_ROOT}/mesh_model/src/vnd_unit_control_aa.c
	${LIB_ROOT}/mesh_model/src/vnd_unit_control_server_aa.c
	${LIB_ROOT}/mesh_model/src/vnd_activation_aa.c
	${LIB_ROOT}/mesh_model/src/vnd_motor_aa.c
	${LIB_ROOT}/mesh_model/src/vnd_motor_server_aa.c
	${LIB_ROOT}/common/src/uart_aa.c
	${LIB_ROOT}/common/src/check_crc_aa.c
	${LIB_ROOT}/common/src/message_format_aa.c)


target_include_directories(app PRIVATE include
	${LIB_ROOT}/mesh_model/include
	${LIB_ROOT}/common/include)

# NORDIC SDK APP END
