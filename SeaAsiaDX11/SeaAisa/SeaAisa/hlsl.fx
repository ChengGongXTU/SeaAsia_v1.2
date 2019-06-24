//--------------------------------------------------------------------------------------
// File: Tutorial07.fx
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
Texture2D txDiffuse : register( t0 );
SamplerState samLinear : register( s0 );

cbuffer wolrdTransform : register( b0 )
{
	matrix mWorld;
};

cbuffer viewTransform : register( b1 )
{
	matrix mView;
    matrix mView2;
};


cbuffer projectionTransform : register( b2 )
{
	matrix mProjection;
};

cbuffer DirLight : register( b3 )
{
    float4 Dir;
    float4 DirLightColor;
};


cbuffer MaterialParameter:register(b4)
{
    float4 Ka;
    float4 Kd;
    float4 Ks;
    float4 Ke;
    float alpha;
    int illum;
    int Ns;
    float Ni;
};


//--------------------------------------------------------------------------------------
struct VS_INPUT
{	
	float4 color : COLOR;
    float3 vertex : POSITION;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD0;
	float3 tangent : TANGENT;
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float4 normal : NORMAL;
    float2 tex : TEXCOORD0;
    matrix view : MATRIX;
    
};

//static matrix viewTran = mView;
//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT VS( VS_INPUT v )
{
    //viewTran = mView;
    PS_INPUT o = (PS_INPUT)0;
	
	o.pos = mul(float4(v.vertex, 1.f),mWorld);
	o.pos = mul(o.pos, mView);
	o.pos = mul(o.pos, mProjection);
    o.normal = normalize(mul(float4(v.normal,0.f), mWorld));
    //o.Tex = v.Tex;
    //o.view = mView;
    return o;
}


//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS( PS_INPUT i) : SV_Target
{	/*
    float4 lightdir = -normalize(mul(Dir, input.view));

    float cosA = dot(lightdir, input.Normal);
    if (cosA < 0.f)
        cosA = 0.f;
    if (cosA > 1.f)
        cosA = 1.f;

    float4 eyeVec = normalize(float4(0, 0, 0, 1) - input.Pos);
    float4 H = normalize(lightdir + eyeVec);
    float cosB = dot(H, input.Normal);
    if (cosB < 0)
        cosB = 0;
    if (cosB > 1.f)
        cosB = 1.f;


    if (input.Tex.x == 0.f && input.Tex.y == .0f)
    {
        return Ka + Kd * cosA * DirLightColor + Ks * pow(cosB, Ns) * DirLightColor;
    }
    else
    {
        return Ka + txDiffuse.Sample(samLinear, input.Tex) * cosA * DirLightColor + Ks * pow(cosB, Ns) * DirLightColor;
        

    }
	*/
	return i.normal;
}
