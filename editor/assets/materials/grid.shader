{
    "name": "Grid",
    "category": "material",
    "defines": [
        "MATERIAL_MASK"
    ],
    "permutations": [ "PASS_DEPTH" ],
    "stages": [
        {
            "stage": "VertexShader",
            "shader": "grid.vert.hlsl"
        },
        {
            "stage": "FragmentShader",
            "shader": "grid.frag.hlsl"
        }
    ]
}