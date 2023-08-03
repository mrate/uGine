# Embedding files to cpp sources.
add_subdirectory(sourceEmbed)

# Material compiler -> generates .umat (ugine::SerializedMaterial) from .mat descriptor.
add_subdirectory(materialc)

# Material builder -> genearates .mat descriptor from high level material descriptor.
add_subdirectory(materialb)

# Shader compiler
add_subdirectory(shaderc)

# Mesh to vertex shader
add_subdirectory(vertify)