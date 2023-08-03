{
    "name": "Standard",
    "category": "material",
    "defines": [],
    "permutations": [ "MATERIAL_MASKED", "MATERIAL_OPACITY", "MATERIAL_INSTANCE", "PASS_DEPTH" ],
    "stages": [
        {
            "stage": "VertexShader",
            "shader": "standard.vert.hlsl"
        },
        {
            "stage": "FragmentShader",
            "shader": "standard.frag.hlsl"
        }
    ],
    "values": {
        "albedoColor": [1.0, 1.0, 1.0],
        "albedoTexture": -1,
        "emissiveColor": [0.0, 0.0, 0.0],
        "emissiveTexture": -1,
        "normalTexture": -1,
        "alpha": 1.0
    }
}