cbuffer cbPerObject: register(b0)
{
	matrix WVP;
};

TextureCube skyboxCubemap : register(t0);
SamplerState skyboxCubemapSampleState: register(s0);

struct VS_INPUT
{
	float3 vertex : POSITION;
	//float2 uv : TEXCOORD;
	//float3 normal : NORMAL;
};

struct PS_INPUT    //output structure for skymap vertex shader
{
	float4 pos : SV_POSITION;
	float3 texcoord : TEXCOORD;
};

PS_INPUT VS(VS_INPUT v)
{
	PS_INPUT o = (PS_INPUT)0;

	//Set Pos to xyww instead of xyzw, so that z will always be 1 (furthest from camera)
	o.pos =  mul(float4(v.vertex, 1.0f), WVP).xyww;
	o.texcoord = v.vertex;

	return o;
}

float4 PS(PS_INPUT i) : SV_Target
{  
	return skyboxCubemap.Sample(skyboxCubemapSampleState, i.texcoord);
}

