cbuffer Constants : register(b0)
{
	matrix Transform;
};

struct VS_INPUT
{
	float2 Position : POSITION;
	float2 UV : TEX;
	float2 Offset : OFFSET;
	float2 Size : SIZE;
	float4 Color : COLOR;
	float4 GlyphUV : GLYPHUV;
};

struct PS_INPUT
{
	float4 Position : SV_POSITION;
	float2 UV : TEXCOORD;
	float4 Color : COLOR;
};

Texture2D MyTexture : register(t0);
SamplerState MySampler : register(s0);

PS_INPUT vs_main(VS_INPUT Input)
{
	PS_INPUT Result;

	Input.Offset.y *= -1;

	float3 Pos = float3(Input.Position, 0.0f);

	Pos.x *= Input.Size.x;
	Pos.y *= Input.Size.y;

	Pos += float3(Input.Offset, 0.0f);

	Result.Position = mul(Transform, float4(Pos, 1.0f));

	Result.UV.x = lerp(Input.GlyphUV.x, Input.GlyphUV.z, Input.UV.x);
	Result.UV.y = lerp(Input.GlyphUV.y, Input.GlyphUV.w, Input.UV.y);

	Result.Color = Input.Color;

	return Result;
}

float4 ps_main(PS_INPUT Input) : SV_Target
{
	float Smoothing = 0.1f;
    float Boldness = 0.3f;
    float4 Sample = MyTexture.Sample(MySampler, Input.UV); 

    float Distance = Sample.a;

    float Alpha = smoothstep(1.0f - Boldness, (1.0f - Boldness) + Smoothing, Distance);

    float4 Result = Alpha * Input.Color;

    Result.xyz /= Result.a;

	return Result;
}