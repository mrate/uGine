{
  "name": "Color shader",
  "category": "material",
  "permutations": [ "PASS_DEPTH", "MATERIAL_INSTANCE" ],
  "stages": [
    {
      "stage": "VertexShader",
      "shader": "color.vert.hlsl"
    },
    {
      "stage": "FragmentShader",
      "shader": "color.frag.hlsl"
    }
  ],
  "values": {
    "materialColor": [ 1.0, 1.0, 0.0, 1.0 ]
  }
}