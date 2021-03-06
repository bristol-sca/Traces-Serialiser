#[=[
    This file is part of Traces-Serialiser.

    Traces-Serialiser is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Traces-Serialiser is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with Traces-Serialiser.  If not, see <http://www.gnu.org/licenses/>.
#]=]

FIND_PACKAGE(SWIG)

set(SWIG_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(SWIG_INPUT_FILE Traces_Serialiser.i)

# Place files needed for bindings into 'output/bindings' folder
set(CMAKE_SWIG_OUTDIR ${EXECUTABLE_OUTPUT_PATH}/bindings)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SWIG_OUTDIR})

# Warnings come from swig itself... As -Werror is used this fails
# to compile. As these are not related to this project they are disabled.
# -w (Disbale all warnings) leaves -Wregister on for some reason when using
# clang
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -Wno-register")

if (SWIG_FOUND)
    option(${PROJECT_NAME}_BINDINGS_PYTHON "Generate bindings for use from \
                                            Python. Requires Swig."
        OFF)

    if(${PROJECT_NAME}_BINDINGS_PYTHON)
        # Recurse into the "python" subdirectory.
        add_subdirectory(python)
    endif()
endif()
