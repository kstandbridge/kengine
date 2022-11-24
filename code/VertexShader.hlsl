cbuffer CBuffer
{
	matrix Transform;
};

float4 main(float3 Position : Position) : SV_Position
{
	float4 Result = mul(float4(Position, 1.0f), Transform);
	
	return Result;
}