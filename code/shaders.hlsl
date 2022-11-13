
struct vs_input
{
	float2 Position : POS;
	float4 Color : COL;
};


struct vs_output
{
	float4 Position : SV_POSITION;
	float4 Color : COL;
};

vs_output 
vs_main(vs_input Input)
{
	vs_output Result;

	Result.Position = float4(Input.Position, 0.0f, 1.0f);
	Result.Color = Input.Color;

	return Result;
}

float4 
ps_main(vs_output Input) : SV_TARGET
{
	float4 Result = Input.Color;
	
	return Result;
}