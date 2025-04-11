####
# Include file with macros for adding tests
####

include(${CMAKE_CURRENT_SOURCE_DIR}/../CMake/my_test.cmake)

#
# Copyright (C) 2021-2024 Chair of Electronic Design Automation, TUM.
# Copyright (C) 2025 TUWien.
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the License); you may
# not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an AS IS BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# add_definitions(-fno-rtti)          #Needed for GCC to resolve virtual delete function scope issue
# add_definitions(-fno-exceptions)    #Needed for GCC to resolve virtual delete function scope issue

add_bin(vector_add ${CMAKE_CURRENT_SOURCE_DIR} ${BUILD_DIR}/custom_sources)
add_bin(scalar_add ${CMAKE_CURRENT_SOURCE_DIR} ${BUILD_DIR}/custom_sources)


