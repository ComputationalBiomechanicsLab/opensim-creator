#version 330 core

uniform sampler2D uDiffuseMap;
uniform sampler2D uNormalMap;
uniform sampler2D uDisplacementMap;
uniform bool uEnableMapping = true;
uniform float uHeightScale;

in vec3 FragWorldPos;
in vec2 TexCoord;
in vec3 NormalTangentDir;  // from the vertex data, for non-normal-mapped comparison
in vec3 LightTangentPos;
in vec3 ViewTangentPos;
in vec3 FragTangentPos;

out vec4 FragColor;


// calculates parallax mapping texture offset using "Parallax Occlusion Mapping" where,
// like "steep mapping", you step along the view vector in a fixed number of steps
// (layers) until you find the first layer that's less deep than the previous one (implying
// you hit an upward hill) and then you LERP between that layer and the previous one to
// approximate where on the hill you hit
//
// (in steep mapping, you just take the layer with no LERPing, which results in
//  stratification - LERPing here smooths the result)
vec2 CalcParallaxOcclusionMapping(vec2 texCoord, vec3 tangentFrag2ViewDir)
{
    // adjust the number of layers to sample based on how straight-on the fragment is being viewed
    //
    // (i.e. if being viewed straight-on, you don't need as many samples as when viewing from the side)
    const float minLayers = 8.0;
    const float maxLayers = 32.0;
    float numLayers = mix(maxLayers, minLayers, max(dot(vec3(0.0, 0.0, 1.0), tangentFrag2ViewDir), 0.0));

    // step along the view vector to find an appropriate texture offset
    //
    // (see LearnOpenGL's parallax mapping guide for "steep parallax mapping")
    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;
    vec2 deltaTexCoord = (uHeightScale * tangentFrag2ViewDir.xy)/numLayers;

    vec2 currentTexCoord = texCoord;
    float currentDepthMapValue = texture(uDisplacementMap, currentTexCoord).r;
    while(currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoord -= deltaTexCoord;

        // get depthmap value at current texture coordinates
        currentDepthMapValue = texture(uDisplacementMap, currentTexCoord).r;

        // get depth of next layer
        currentLayerDepth += layerDepth;
    }

    // (the rest of this is the "Occlusion Mapping" bit, where you LERP between the chosen
    //  point and the previous one to "smooth out" the problems that steep, non-LERPed mapping
    //  produces)

    // get texture coordinates before collision (reverse operations)
    vec2 prevTexCoord = currentTexCoord + deltaTexCoord;

    // get depth after and before collision for linear interpolation
    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(uDisplacementMap, prevTexCoord).r - currentLayerDepth + layerDepth;

    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoord * weight + currentTexCoord * (1.0 - weight);

    return finalTexCoords;
}

// calculates parallax mapping texture offset using "steep parallax mapping", where
// you step along the view vector in a fixed number of steps (layers) until you find
// the first layer that's less deep than the previous one (implying you hit an upward
// hill) and take that one (which might be somewhat embedded in the hill)
vec2 CalcSteepParallaxMapping(vec2 texCoord, vec3 tangentFrag2ViewDir)
{
    // adjust the number of layers to sample based on how straight-on the fragment is being viewed
    //
    // (i.e. if being viewed straight-on, you don't need as many samples as when viewing from the side)
    const float minLayers = 8.0;
    const float maxLayers = 32.0;
    float numLayers = mix(maxLayers, minLayers, max(dot(vec3(0.0, 0.0, 1.0), tangentFrag2ViewDir), 0.0));

    // step along the view vector to find an appropriate texture offset
    //
    // (see LearnOpenGL's parallax mapping guide for "steep parallax mapping")
    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;
    vec2 deltaTexCoord = (uHeightScale * tangentFrag2ViewDir.xy)/numLayers;

    vec2 currentTexCoord = texCoord;
    float currentDepthMapValue = texture(uDisplacementMap, currentTexCoord).r;
    while(currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoord -= deltaTexCoord;

        // get depthmap value at current texture coordinates
        currentDepthMapValue = texture(uDisplacementMap, currentTexCoord).r;

        // get depth of next layer
        currentLayerDepth += layerDepth;
    }

    return currentTexCoord;
}

// calculates parallax mapping texture offset the "basic" way by projecting the
// view vector a bit based on the depth read from the initial texture coordinate
vec2 CalcParallaxMappingTexCoord(vec2 texCoord, vec3 tangentFrag2ViewDir)
{ 
    // the texture stores *depth*, not *height*
    float depth = texture(uDisplacementMap, texCoord).r;

    // because it stores depth subtract, rather than add, the offset
    return texCoord - (uHeightScale * depth * tangentFrag2ViewDir.xy/tangentFrag2ViewDir.z);
}

// calculates the output fragment color incl. parallax mapping
vec3 CalcFragColorWithMapping()
{
    // offset texture coordinates with Parallax Mapping
    vec3 tangentFrag2ViewDir = normalize(ViewTangentPos - FragTangentPos);

    vec2 displacedTexCoord = CalcParallaxOcclusionMapping(TexCoord, tangentFrag2ViewDir);

    // if parallax mapping results in an out-of-bounds texture coordinate, then skip rendering
    // this fragment altogether
    if (displacedTexCoord.x > 1.0 ||
        displacedTexCoord.y > 1.0 ||
        displacedTexCoord.x < 0.0 ||
        displacedTexCoord.y < 0.0)
    {
        discard;
    }

    // read normal in range [0, 1] from normal map
    vec3 tangentNormalDir = texture(uNormalMap, displacedTexCoord).rgb;
    // and remap it to [-1, 1]
    tangentNormalDir = normalize(2.0*tangentNormalDir - 1.0);

    // perform shading calculations (phong)
    vec3 tangentFrag2LightDir = normalize(LightTangentPos - FragTangentPos);
    vec3 tangentLight2FragReflectionDir = reflect(-tangentFrag2LightDir, tangentNormalDir);
    vec3 tangentHalfwayDir = 0.5 * (tangentFrag2LightDir + tangentFrag2ViewDir);

    float ambientStrength = 0.1;
    float diffuseStrength = max(dot(tangentFrag2LightDir, tangentNormalDir), 0.0);
    float specularStrength = 0.2 * pow(max(dot(tangentNormalDir, tangentHalfwayDir), 0.0), 32.0);
    float strength = ambientStrength + diffuseStrength + specularStrength;

    return strength * texture(uDiffuseMap, displacedTexCoord).rgb;
}

// calculates the output fragment color without applying any parallax mapping
//
// (used for comparison)
vec3 CalcFragColorWithoutMapping()
{
    // perform shading calculations (phong)
    vec3 lightTangentDir = normalize(LightTangentPos - FragTangentPos);
    vec3 viewTangentDir = normalize(ViewTangentPos - FragTangentPos);
    vec3 reflectTangentDir = reflect(-lightTangentDir, NormalTangentDir);
    vec3 halfwayTangentDir = normalize(lightTangentDir + viewTangentDir);

    float ambientStrength = 0.1;
    float diffuseStrength = max(dot(lightTangentDir, NormalTangentDir), 0.0);
    float specularStrength = 0.2 * pow(max(dot(NormalTangentDir, halfwayTangentDir), 0.0), 32.0);

    float strength = ambientStrength + diffuseStrength + specularStrength;

    return strength*texture(uDiffuseMap, TexCoord).rgb;
}

void main()
{
    vec3 color = uEnableMapping ? CalcFragColorWithMapping() : CalcFragColorWithoutMapping();
    FragColor = vec4(color, 1.0);
}