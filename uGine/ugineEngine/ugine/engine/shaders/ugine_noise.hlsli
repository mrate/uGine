// https://www.shadertoy.com/view/DdBGDh?utm_campaign=Shader%20Tutorials&utm_medium=email&utm_source=Revue%20newsletter

//Typical pseudo-random hash (white noise)
float hash1(float2 p) {
    //Generate a pseudo random number from 'p'.
    return frac(sin(p.x * 0.129898 + p.y * 0.78233) * 43758.5453);
}

//float2 version of the hash function
float2 hash2(float2 p) {
    //Generate a pseudo random float2 from 'p'
    return frac(sin(mul(p, float2x2(0.129898, 0.81314, 0.78233, 0.15926))) * 43758.5453);
}
//float2 unit-vector version of the hash function
float2 hash2_norm(float2 p) {
    //Returns a random normalized direction vector
    return normalize(hash2(p) - 0.5);
}
//Standard value noise
float value_noise(float2 p) {
    //Cell (whole number) coordinates
    float2 cell = floor(p);
    //Sub-cell (fracional) coordinates
    float2 sub = p - cell;
    //Cubic interpolation (use sub for linear interpolation)
    float2 cube = sub * sub * (3. - 2. * sub);
    //Offset vector
    const float2 off = float2(0, 1);

    //Sample cell corners and interpolate between them.
    return lerp(lerp(hash1(cell + off.xx), hash1(cell + off.yx), cube.x), lerp(hash1(cell + off.xy), hash1(cell + off.yy), cube.x), cube.y);
}
//Standard Perlin noise
float perlin_noise(float2 p) {
    //Cell (whole number) coordinates
    float2 cell = floor(p);
    //Sub-cell (fracional) coordinates
    float2 sub = p - cell;
    //Quintic interpolation
    float2 quint = sub * sub * sub * (10.0 + sub * (-15.0 + 6.0 * sub));
    //Offset vector
    const float2 off = float2(0, 1);

    //Compute corner hashes and gradients
    float grad_corner00 = dot(hash2_norm(cell + off.xx), off.xx - sub);
    float grad_corner10 = dot(hash2_norm(cell + off.yx), off.yx - sub);
    float grad_corner01 = dot(hash2_norm(cell + off.xy), off.xy - sub);
    float grad_corner11 = dot(hash2_norm(cell + off.yy), off.yy - sub);

    //Interpolate between the gradient values them and map to 0 - 1 range
    return lerp(lerp(grad_corner00, grad_corner10, quint.x), lerp(grad_corner01, grad_corner11, quint.x), quint.y) * 0.7 + 0.5;
}
//Standard Worley noise function
float worley_noise(float2 p) {
    //Cell (whole number) coordinates
    float2 cell = floor(p);
    //Initialize distance at a high-number
    float dist = 9.0;

    //Iterate through [3,3] neighbor cells
    for (int x = -1; x <= 1; x++)
        for (int y = -1; y <= 1; y++) {
            //Get sample cell coordinates
            float2 sample_cell = cell + float2(x, y);
            //Compute difference from pixel to worley cell
            float2 worley_dif = hash2(sample_cell) + sample_cell - p;
            //Save the nearest distance
            dist = min(dist, length(worley_dif));
        }
    return dist;
}
//Standard Voronoi noise function
float voronoi_noise(float2 p) {
    //Cell (whole number) coordinates
    float2 cell = floor(p);
    //Initialize distance at a high-number
    float dist = 9.0;
    //Store the nearest voronoi cell
    float2 voronoi_cell = cell;

    //Iterate through [3,3] neighbor cells
    for (int x = -1; x <= 1; x++)
        for (int y = -1; y <= 1; y++) {
            //Get sample cell coordintaes
            float2 sample_cell = cell + float2(x, y);
            //Compute difference from pixel to worley cell
            float2 worley_dif = hash2(sample_cell) + sample_cell - p;
            //Compute the worley distance
            float new_dist = length(worley_dif);
            //If the new distance is the nearest
            if (dist > new_dist) {
                //Store the new distance and cell coordinates
                dist = new_dist;
                voronoi_cell = sample_cell;
            }
        }
    //Get a random value using cell coordinates
    return hash1(voronoi_cell);
}
//Generate fractal value noise from multiple octaves
//oct - The number of octave passes
//per - Octave persistence value (should be between 0 and 1)
float fractal_noise(float2 p, int oct, float per) {
    float noise_sum = 0.0;  //Noise total
    float weight_sum = 0.0; //Weight total
    float weight = 1.0;     //Octave weight

    for (int i = 0; i < oct; i++) //Iterate through octaves
    {
        //Add noise octave to total
        noise_sum += value_noise(p) * weight;
        //Add octave weight to total
        weight_sum += weight;
        //Reduce octave amplitude with persistence value
        weight *= per;
        //Rotate and scale for next octave
        p = mul(p, float2x2(1.6, 1.2, -1.2, 1.6));
    }
    //Compute weighted average
    return noise_sum / weight_sum;
}