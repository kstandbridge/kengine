cbuffer CBuf
{
	float4 FaceColors[6];
};

float4 main(uint TriangleId : SV_PrimitiveID) : SV_Target
{
	float4 Result = FaceColors[TriangleId / 2];

	return Result;
}