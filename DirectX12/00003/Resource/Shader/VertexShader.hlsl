#include "Header.hlsli"

Output main(float4 position : POSITION, float2 uv : TEXCOORD)
{
    Output output;
	output.position = position;
	output.uv = uv;
	return output;
}
