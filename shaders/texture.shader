{
  "name": "Texture Material",
  "category": "material",
  "permutations": [ "PASS_DEPTH", "MATERIAL_INSTANCE" ],
  "stages": [
    {
      "stage": "VertexShader",
      "shader": "texture.vert.hlsl"
    },
    {
      "stage": "FragmentShader",
      "shader": "texture.frag.hlsl"
    }
  ],
  "values": {
    "textureID": -1
  }
}