struct VS_INPUT
{
	float2 Position : POSITION;
	float3 Color : COLOR0;
};

struct PS_INPUT
{
	float4 Position : SV_POSITION;
	float3 Color : COLOR0;
};

PS_INPUT vs_main(VS_INPUT Input)
{
	PS_INPUT Result;

	Result.Position = float4(Input.Position.xy, 0.0f, 1.0f);
	Result.Color = Input.Color;

	return Result;
}

float4 ps_main(PS_INPUT Input) : SV_Target
{
	float4 Result = float4(Input.Color, 1.0f);
	return Result;
}