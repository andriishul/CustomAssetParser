# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.10)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("CustomAssetParser")

add_subdirectory(external/assimp)


add_executable (CustomAssetParser "main.cpp" "ModelWriter.cpp")
target_link_libraries(CustomAssetParser PRIVATE assimp)

file(GLOB DLL_FILENAME "${CMAKE_BINARY_DIR}/assimp/bin/*.dll")

add_custom_command(
TARGET CustomAssetParser POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy "${DLL_FILENAME}" "${CMAKE_CURRENT_BINARY_DIR}"
)
message("${DLL_FILENAME}")
if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET CustomAssetParser PROPERTY CXX_STANDARD 20)
endif()
