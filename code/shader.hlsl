cbuffer Constants : register(b0)
{
	matrix Transform;
	float4 UniformColor;
};

struct VS_INPUT
{
	float2 Position : POSITION;
};

struct PS_INPUT
{
	float4 Position : SV_POSITION;
	float4 Color : COLOR;
};

PS_INPUT vs_main(VS_INPUT Input)
{
	PS_INPUT Result;

	// Result.Position = mul(float4(Input.Position, 0.0f, 1.0f), Transform);
	Result.Position = float4(Input.Position, 0.0f, 1.0f);
	Result.Color = UniformColor;

	return Result;
}

float4 ps_main(PS_INPUT Input) : SV_Target
{
	float4 Result = float4(Input.Color);
	return Result;
}