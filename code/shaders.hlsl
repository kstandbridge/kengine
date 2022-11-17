
cbuffer constants : register(b0)
{
	float2 Offset;
	float4 UniformColor;
};

struct vs_input
{
	float3 Position : POS;
	float2 UV : TEX;
	float4 Color : COLOR_INSTANCE;
};

struct vs_output
{
	float4 Position : SV_POSITION;
	float2 UV : TEXCOORD;
	float4 Color: COLOR;
};

Texture2D MyTexture : register(t0);
SamplerState MySampler : register(s0);

vs_output vs_main(vs_input Input)
{
	vs_output Result;

	Result.Position = float4(Input.Position, 1.0f);
	Result.UV = Input.UV;

	// NOTE(kstandbridge): vs_input Color is of type COLOR_INSTANCE, which we need to map and memcpy
	Result.Color = float4(0.0f, 0.0f, 0.0f, 1.0f);

	return Result;
}

float4 ps_main(vs_output Input) : SV_Target
{
	float Smoothing = 0.1f;
    float Boldness = 0.3f;
    float4 Sample = MyTexture.Sample(MySampler, Input.UV); 

    float Distance = Sample.a;

    float Alpha = smoothstep(1.0f - Boldness, (1.0f - Boldness) + Smoothing, Distance);

    float4 Color = Alpha * Input.Color;

    Color.xyz /= Color.a;

    return Color;
}