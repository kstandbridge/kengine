float4 main(float3 Color : Color) : SV_Target
{
	float4 Result = float4(Color, 1.0f);

	return Result;
}