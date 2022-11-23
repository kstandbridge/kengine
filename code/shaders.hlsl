
cbuffer constants : register(b0)
{
	float4 ConstColor;	
	float4x4 OrthoMatrix;
};

struct vs_input
{
	float3 Position : POS;
	float2 UV : TEX;
	float4 Color : COLOR_INSTANCE;
	float4 GlyphUV : TEX_INSTANCE;
	float4 Size : SIZE_INSTANCE;
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

	float3 Position = Input.Position;
	
	Position.x *= Input.Size.x;
	Position.y *= Input.Size.y;

	Result.Position = mul(OrthoMatrix, float4(Position, 1.0f));

	// Result.UV = Input.UV;
	Result.UV.x = lerp(Input.GlyphUV.x, Input.GlyphUV.z, Input.UV.x);
	Result.UV.y = lerp(Input.GlyphUV.y, Input.GlyphUV.w, Input.UV.y);

	//Result.Color = Input.Color;
	Result.Color = ConstColor;

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