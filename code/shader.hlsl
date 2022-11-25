cbuffer Constants : register(b0)
{
	matrix Transform;
};

struct VS_INPUT
{
	float2 Position : POSITION;
	float2 Offset : OFFSET;
	float2 Size : SIZE;
	float4 Color : COLOR;
};

struct PS_INPUT
{
	float4 Position : SV_POSITION;
	float4 Color : COLOR;
};

PS_INPUT vs_main(VS_INPUT Input)
{
	PS_INPUT Result;

	Input.Offset.y *= -1;

	float3 Pos = float3(Input.Position, 0.0f);

	Pos.x *= Input.Size.x;
	Pos.y *= Input.Size.y;

	Pos += float3(Input.Offset, 0.0f);

	Result.Position = mul(Transform, float4(Pos, 1.0f));
	Result.Color = Input.Color;

	return Result;
}

float4 ps_main(PS_INPUT Input) : SV_Target
{
	float4 Result = float4(Input.Color);
	return Result;
}