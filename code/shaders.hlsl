
cbuffer constants : register(b0)
{
	float2 Offset;
	float4 UniformColor;
};

struct vs_input
{
	float2 Position : POS;
	float2 UV : TEX;
};

struct vs_output
{
	float4 Position : SV_POSITION;
	float2 UV : TEXCOORD;
};

Texture2D MyTexture : register(t0);
SamplerState MySampler : register(s0);

vs_output vs_main(vs_input Input)
{
	vs_output Result;

	Result.Position = float4(Input.Position, 0.0f, 1.0f);
	Result.UV = Input.UV;

	return Result;
}

float4 ps_main(vs_output Input) : SV_Target
{
	float4 Result = MyTexture.Sample(MySampler, Input.UV);
	
	return Result;
}