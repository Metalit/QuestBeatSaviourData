# credit goes to https://github.com/Lauriethefish for this

# Directory where our arbitrary asset files are stored
set(ASSETS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/assets)

# Directory to save the object files generated by llvm-objcopy
set(ASSET_BINARIES_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/binaryAssets)

# Directory to save the prepended files to
set(ASSET_HEADER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/include/assets.hpp")

# Define a macro which we will use for defining the symbols to access our asset files below
set(ASSET_HEADER_DATA
    "#pragma once
#define DECLARE_FILE(name, prefix) \\
extern \"C\" uint8_t _binary_##name##_start[]; \\
extern \"C\" uint8_t _binary_##name##_end[]; \\
struct prefix##name { \\
    static size_t getLength() { return _binary_##name##_end - _binary_##name##_start; } \\
    static uint8_t *getData() { return _binary_##name##_start; } \\
};
")

if(EXISTS ${ASSETS_DIRECTORY})
    file(MAKE_DIRECTORY ${ASSET_BINARIES_DIRECTORY})
    file(GLOB ASSETS ${ASSETS_DIRECTORY}/*.*)

    # Iterate through each file in the assets directory. TODO: This could be recursive
    foreach(FILE IN LISTS ASSETS)
        message("-- Including asset: ${FILE}")
        get_filename_component(ASSET ${FILE} NAME) # Find the asset's file name

        set(OUTPUT_FILE "${ASSET_BINARIES_DIRECTORY}/${ASSET}.o") # Save our asset in the asset binaries directory

        # Use llvm-objcopy to create an object file that stores our binary asset
        # The resulting file contains 3 symbols: _binary_<file_name>_start, _binary_<file_name>_size and _binary_<file_name>_end
        # We only use the first two
        add_custom_command(
            OUTPUT ${OUTPUT_FILE}
            COMMAND ${CMAKE_OBJCOPY} ${ASSET} ${OUTPUT_FILE} --input-target binary --output-target elf64-aarch64
            DEPENDS ${ASSETS_DIRECTORY}/${ASSET}
            WORKING_DIRECTORY ${ASSETS_DIRECTORY}
        )
        list(APPEND BINARY_ASSET_FILES ${OUTPUT_FILE})

        # Find the correct objcopy symbol name, this is always the file name with any non-alphanumeric characters replaced with _
        string(REGEX REPLACE "[^a-zA-Z0-9]" "_" FIXED_ASSET ${ASSET})

        # Add to our assets header
        set(ASSET_HEADER_DATA "${ASSET_HEADER_DATA}DECLARE_FILE(${FIXED_ASSET},)\n")
    endforeach()

    # check if at least 1 asset file, otherwise ignore
    list(LENGTH BINARY_ASSET_FILES COUNT)

    if(${COUNT} GREATER 0)
        # Generate the assets header file
        file(GENERATE OUTPUT ${ASSET_HEADER_PATH} CONTENT "${ASSET_HEADER_DATA}")

        # Add our assets files to the final SO
        add_library(asset_files OBJECT ${BINARY_ASSET_FILES})
        set_target_properties(asset_files PROPERTIES LINKER_LANGUAGE CXX)
        target_link_libraries(${COMPILE_ID} PRIVATE asset_files ${BINARY_ASSET_FILES})
    endif()
else()
    message("-- Removing '${ASSET_HEADER_PATH}' as no assets have been found in '${ASSETS_DIRECTORY}'")
    file(REMOVE ${ASSET_HEADER_PATH})
endif()
