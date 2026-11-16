include(FetchContent)
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

set(CMAKE_POLICY_VERSION_MINIMUM 3.10)
set(GLM_BUILD_LIBRARY OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(runner_glm
  URL https://codeload.github.com/g-truc/glm/tar.gz/0af55ccecd98d4e5a8d1fad7de25ba429d60e863
  URL_HASH SHA256=e7f187d83523f505eb38dd25d297ea6c0d4ed856d733e808f18253f5a8fa88a0
)
set(FREEGLUT_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(FREEGLUT_BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
set(FREEGLUT_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(FREEGLUT_REPLACE_GLUT OFF CACHE BOOL "" FORCE)
set(INSTALL_PDB OFF CACHE BOOL "" FORCE)
FetchContent_Declare(runner_freeglut
  URL https://codeload.github.com/freeglut/freeglut/tar.gz/63c0e78d84ecbfcf8accd8bac04404427899053b
  URL_HASH SHA256=35957224dc71f06a62407ab513a06e39c18f238c8de26cedf747a4501a756d01
)
FetchContent_MakeAvailable(runner_glm runner_freeglut)
target_include_directories(freeglut_static PUBLIC
  "$<BUILD_INTERFACE:${runner_freeglut_SOURCE_DIR}/include>"
)

set(RUNNER_LICENSE_DIR "${CMAKE_BINARY_DIR}/licenses")
file(MAKE_DIRECTORY "${RUNNER_LICENSE_DIR}")
configure_file("${runner_glm_SOURCE_DIR}/copying.txt" "${RUNNER_LICENSE_DIR}/GLM.txt" COPYONLY)
configure_file("${runner_freeglut_SOURCE_DIR}/COPYING" "${RUNNER_LICENSE_DIR}/FreeGLUT.txt" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad/LICENSE" "${RUNNER_LICENSE_DIR}/GLAD.txt" COPYONLY)
