# CustomAssetParser

An offline asset pipeline tool for [tungol-gfx](https://github.com/andriishul/tungol-gfx). 

This tool bridges the gap between standard 3D assets and the custom rendering engine. It is designed specifically around the **`.gltf`** format to take advantage of its flexibility, reading the file and packing the geometry, materials, and animation data into a closed, custom binary **`.model`** format. 

This eliminates runtime JSON parsing in the engine, while explicitly extracting and managing texture file paths so the runtime never fails to resolve them.

## Features
*   **`.gltf`-Centric Design:** Built to leverage the structural flexibility of glTF files via Assimp.
*   **Custom `.model` Format:** Packs meshes, materials, bones, and animation keyframes into a single, closed binary blob for fast runtime loading.
*   **Texture Path Extraction:** Automatically resolves texture references from the source, copies the raw image files to the output directory, and embeds the normalized file paths directly into the `.model` binary.

## Status & Disclaimer
**This is an early-stage working prototype.** 

The tool was built specifically to serve the needs of a custom Vulkan renderer. To accelerate the proof-of-concept phase, parts of the boilerplate and binary parsing logic were developed with AI assistance. 

Because this is a V1 prototype, it prioritizes functionality over robustness. You may encounter missing edge cases or incomplete error handling

Repository has submodules, so **clone --recursive**