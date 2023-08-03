{
    "name": "Bone",
    "category": "material",
    "defines": [
        "MATERIAL_OPACITY"
    ],
    "permutations": [
        "PASS_DEPTH",
        "MATERIAL_INSTANCE"
    ],
    "stages": [
        {
            "stage": "VertexShader",
            "shader": "bone.vert.hlsl"
        },
        {
            "stage": "FragmentShader",
            "shader": "bone.frag.hlsl"
        }
    ]
}