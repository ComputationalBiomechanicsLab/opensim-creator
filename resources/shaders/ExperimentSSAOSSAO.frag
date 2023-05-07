#version 330 core

in vec2 TexCoords;

out float FragColor;

uniform sampler2D uPositionTex;
uniform sampler2D uNormalTex;
uniform sampler2D uNoiseTex;
uniform vec3 uSamples[64];
uniform mat4 uProjMat;
uniform vec2 uNoiseScale;
uniform int uKernelSize;
uniform float uRadius;
uniform float uBias;

void main()
{
    // get input for SSAO algorithm
    vec3 fragPos = texture(uPositionTex, TexCoords).xyz;
    vec3 normal = normalize(texture(uNormalTex, TexCoords).rgb);
    vec3 randomVec = normalize(texture(uNoiseTex, TexCoords * uNoiseScale).xyz);

    // create TBN change-of-basis matrix: from tangent-space to view-space
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    // iterate over the sample kernel and calculate occlusion factor
    float occlusion = 0.0;
    for(int i = 0; i < uKernelSize; ++i)
    {
        // get sample position
        vec3 samplePos = TBN * uSamples[i];  // from tangent to view-space
        samplePos = fragPos + samplePos * uRadius;
        
        // project sample position (to sample texture) (to get position on screen/texture)
        vec4 offset = vec4(samplePos, 1.0);
        offset = uProjMat * offset; // from view to clip-space
        offset.xyz /= offset.w; // perspective divide
        offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0
        
        // get sample depth
        float sampleDepth = texture(uPositionTex, offset.xy).z; // get depth value of kernel sample
        
        // range check & accumulate
        float rangeCheck = smoothstep(0.0, 1.0, uRadius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + uBias ? 1.0 : 0.0) * rangeCheck;
    }
    occlusion = 1.0 - (occlusion / uKernelSize);
    
    FragColor = occlusion;
}
