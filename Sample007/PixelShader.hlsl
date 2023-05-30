#include "ShaderHeader.hlsli"

Texture2D<float4> tex : register(t0);
SamplerState samp : register(s0);

float4 main(Output output) : SV_TARGET
{
	return float4(tex.Sample(samp, output.uv));
}
