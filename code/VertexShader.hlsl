struct VSOut
{
	float3 Color : Color;
	float4 Position : SV_Position;
};

cbuffer CBuffer
{
	matrix Transform;
};

VSOut main(float2 Position : Position, float3 Color : Color)
{
	VSOut Result;

	Result.Position = mul(float4(Position.x, Position.y, 0.0f, 1.0f), Transform);
	Result.Color = Color;

	return Result;
}