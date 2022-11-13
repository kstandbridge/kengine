
cbuffer constants : register(b0)
{
	float2 Offset;
	float4 UniformColor;
};

struct vs_output
{
	float4 Position : SV_POSITION;
	float4 Color : COL;
};

vs_output vs_main(float2 Position : POS)
{
	vs_output Result;

	Result.Position = float4(Position + Offset, 0.0f, 1.0f);
	Result.Color = UniformColor;

	return Result;
}

float4 ps_main(vs_output Input) : SV_Target
{
	float4 Result = Input.Color;
	
	return Result;
}