# Copyright 2015 Jonathan Eyolfson
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# this program. If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.1.3)

project(CONFIGURATION_CHECK C)

find_package(PkgConfig)
pkg_check_modules(LIBALPM REQUIRED libalpm)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_definitions(${LIBALPM_DEFINITIONS})
include_directories(${LIBALPM_INCLUDE_DIRS})
add_compile_options(-Wall ${LIBALPM_INCLUDE_DIRS})

add_executable(configuration-check
	main.c
)
target_link_libraries(configuration-check
	${LIBALPM_LIBRARIES}
)
