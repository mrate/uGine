{
  "name": "Cubemap Material",
  "category": "material",
  "permutations": [ "PASS_DEPTH" ],
  "stages": [
    {
      "stage": "VertexShader",
      "shader": "cubemap.vert.hlsl"
    },
    {
      "stage": "FragmentShader",
      "shader": "cubemap.frag.hlsl"
    }
  ],
  "pipeline": {
    "depthStencil": {
      "depthTestEnable": true
    }
  },
  "values": {
    "textureID": -1
  }
}