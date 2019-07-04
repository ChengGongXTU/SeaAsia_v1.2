
// Textures
Texture2D RT0 : register(t0);
Texture2D RT1 : register(t1);
Texture2D RT2 : register(t2);
SamplerState SamLinear : register(s0);

//ibl
TextureCube iblCubeMap;

SamplerState iblSamLinear
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};

// Constants
struct LightParams
{
	float4 LightPos;
	float4 LightColor;
	float4 LightDirection;
	float LightIntensity;
	float LightRange;
	float SpotlightInnerAngles;
	float SpotlightOuterAngles;

};

StructuredBuffer<LightParams> currentLightParams : register(t3);

cbuffer LightType : register(b0)
{	
	//0 dir
	//1 point
	//2 spot
	int type;
};

cbuffer CameraParams : register(b1)
{
	float4 CameraPos;
};



// Input/Output structures
struct VS_INPUT
{
	float3 pos : POSITION;
	float2 tex : TEXCOORD0;
};

struct PS_INPUT
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD0;
};


PS_INPUT VS(VS_INPUT v)
{
	PS_INPUT o;
	o.pos = float4(v.pos,1.0);
	o.uv = v.tex;
	return o;
}

float f_JsutCause2(in float r, in float r_max)
{	
	float f = 1 - pow(r / r_max, 2);
	f = max(f, 0.0);
	return pow(f, 2);
}

float3 LightDirAndColor(inout float3 dir, in float3 worldPos)
{
	if (type == 0)
	{
		dir = currentLightParams[0].LightDirection.xyz;
		return currentLightParams[0].LightColor.xyz * currentLightParams[0].LightIntensity;

	}

	if (type == 1)
	{
		dir = worldPos - currentLightParams[0].LightPos.xyz;
		float len = length(dir);
		dir = normalize(dir);
		float f_dist = f_JsutCause2(len, currentLightParams[0].LightRange);
		return currentLightParams[0].LightColor.xyz * currentLightParams[0].LightIntensity * f_dist;

	}

	if (type == 2)
	{
		dir = worldPos - currentLightParams[0].LightPos.xyz;
		float len = length(dir);
		dir = normalize(dir);
		
		float f_dist = f_JsutCause2(len, currentLightParams[0].LightRange);

		float cos_s = dot(float4(dir, 0.0), float4(currentLightParams[0].LightDirection.xyz,0.0));
		float cos_u = cos(currentLightParams[0].SpotlightOuterAngles * 3.1415 / 180.0);
		float cos_p = cos(currentLightParams[0].SpotlightInnerAngles * 3.1415 / 180.0);

		float t = (cos_s - cos_u) / (cos_p - cos_u);
		float f_dir_smoothstep = t * t*(3 - 2 * t);

		return currentLightParams[0].LightColor.xyz * currentLightParams[0].LightIntensity * f_dist * f_dir_smoothstep;
	}

	return float3(0, 0, 0);

}

float4 PS(PS_INPUT i) : SV_TARGET
{
	//sample rt info
	float4 rt0 = RT0.Sample(SamLinear, i.uv);
	float4 rt1 = RT1.Sample(SamLinear, i.uv);
	float4 rt2 = RT2.Sample(SamLinear, i.uv);

	float metallic = rt0.w;
	float roughness = rt1.w;
	float specAO = rt2.w;
	float3 wPos = rt0.xyz;
	float3 wNormal = rt1.xyz;
	float3 albedo = rt2.xyz;

	//view
	float3 viewDir = normalize(CameraPos.xyz - wPos);

	//Directional light info
	float3 lightDir = float3(0, 0, 1);
	float3 lightCol = LightDirAndColor(lightDir, wPos);
	float dot_nl = dot(lightDir, wNormal);
	float dot_nl1 = max(dot_nl, 0);
	float dot_nv = dot(viewDir, wNormal);
	float dot_nv1 = max(dot_nv, 0);
	float3 halfDir = normalize(viewDir + lightDir);
	float dot_nh = dot(wNormal, halfDir);
	float dot_nh1 = max(dot_nh, 0.0);
	float dot_lh = dot(lightDir, halfDir);
	float dot_lh1 = max(dot_lh, 0.0);

	//disney diffuse 2014
	float3 diffuseCol = lerp(0.022, albedo, 1 - metallic);
	float FD90 = 0.5 + 2 * roughness * dot_lh1 * dot_lh1;
	float Fl = (FD90 - 1) * pow(1 - dot_nl1, 5);
	float Fd = (FD90 - 1) * pow(1 - dot_nv1, 5);
	float3 diffuse = (diffuseCol / 3.1415) * (1 + Fl) * (1 + Fd);


	//GTR D: r = 2
	float r = 2;
	float a = roughness * roughness;
	float a2 = a * a;
	float k = 0;
	if (a == 1)
	{
		k = 1;
	}
	else
	{
		k = (a2 - 1) * (r - 1) / (1 - pow(a2, 1 - r));
	}
	float D = k / pow(3.1415 * (1 + dot_nh1 * dot_nh1 * (a2 - 1)), r);

	//F schlick:
	float3 F0 = lerp(0.02, albedo, metallic);
	float3 F = F0 + (1 - F0) * pow(1 - dot_lh1, 5);

	//G2: GGX by Walter 
	float ag = pow(0.5 + roughness * 0.5, 2);
	float ag2 = ag * ag;
	float Gl = 2 * dot_nl1 / (dot_nl1 + sqrt(ag2 + (1 - ag2) * pow(dot_nl1, 2)));
	float Gv = 2 * dot_nv1 / (dot_nv1 + sqrt(ag2 + (1 - ag2) * pow(dot_nv1, 2)));
	float G2 = Gl * Gv;

	//specular: 
	float3 specular = D * G2 * F / (4 * dot_nl1 * dot_nv1);

	//f(l,v)
	float3 f_lv = diffuse + specular * specAO;

	//direct illumination
	float3 L_direct = f_lv * dot_nl1 * lightCol;
	
	//IBL£º OP2 approx publish in siggraph 2013
	half4 c0 = half4( -1.0, -0.0275, -0.572, 0.022);
	half4 c1 = half4( 1.0, 0.0425, 1.04, -0.04);
	half4 iblRoughness = roughness * c0 + c1;
	half a004 = min(iblRoughness.x * iblRoughness.x, exp2(-9.28 * dot_nv1)) * iblRoughness.x + iblRoughness.y;
	half2 AB = half2(-1.04, 1.04) * a004 + iblRoughness.zw;
	half3 envBRDF = F0 * AB.x + AB.y;
	float3 R = normalize(-viewDir - 2 * dot(-viewDir, wNormal)*wNormal);
	half4 iblCol = iblCubeMap.Sample(iblSamLinear, R);
	half3 spec_Indirect = iblCol.xyz * envBRDF;

	half3 L_indirect = spec_Indirect;

	half3 finalCol = L_direct + L_indirect;

	return half4(rt2.xyz,1.0);


}
