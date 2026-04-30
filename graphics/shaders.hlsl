// Both vertex and fragment shaders are written here in HLSL,
 // they are compiled as a C-style header using dxc with the "-spirv" flag
 // You might get an error saying that spirv is not compatible,
 // in this case, use the Vulkan SDK's dxc tool and not the one from the Windows SDK
//
// dxc -spirv -Fh fs.h -T ps_6_0 -E PSmain shaders.hlsl
// dxc -spirv -Fh vs.h -T vs_6_0 -E VSmain shaders.hlsl

struct PSinput
{
    float4 pos: SV_POSITION;
    float4 color: COLOR;
};

typedef PSinput VSoutput;

struct PCdata
{
    float angle;
    float aspect;
};

[[vk::push_constant]]
PCdata pc_data;

VSoutput VSmain(uint viviviviii: SV_VERTEXID)
{
    static const float3 COLORS[3] = {
        float3(1.0f, 0.0f, 0.0f),
        float3(0.0f, 1.0f, 0.0f),
        float3(0.0f, 0.0f, 1.0f)
    };
    static const float2 VERTICES[3] = {
        float2(-0.5f, 0.5f),
        float2(0.0f, -0.5f),
        float2(0.5f, 0.5f)
    };

    float angle = pc_data.angle;
    //angle = 0;
    const float ca = cos(angle);
    //const float sa = sin(angle);
    const float2x2 rotmat = float2x2(
        ca, 0.0f, // -sa,
        0.0f, 1.0f // 0.0f,
    //  sa, 0.0f, ca
    );

    const float2x2 aspectmat = float2x2(
        /*pc_data.aspect*/ 1.0f, 0.0f,
        0.0f, 1.0f 
    );

    float2x2 mp = mul(rotmat, aspectmat);

    VSoutput output;
    output.pos = float4(mul(VERTICES[viviviviii], mp), 0.0f, 1.0f);
    output.color = float4(COLORS[viviviviii], 1.0f);

    return output;
}

float4 PSmain(PSinput input): SV_TARGET
{
    return input.color;
}