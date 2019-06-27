
// Textures
Texture2D RT0 : register(t0);
Texture2D RT1 : register(t1);
Texture2D RT2 : register(t2);
SamplerState SamLinear : register(s0);

// Constants
cbuffer LightParams : register(b0)
{
	float3 LightPos;
	float3 LightColor;
	float3 LightDirection;
	float2 SpotlightAngles;
	float4 LightRange;
};

cbuffer CameraParams : register(b1)
{
	float3 CameraPos;
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
	VS_INPUT o;
	o.pos = float4(v.pos,1.0);
	o.uv = v.tex;
	return o;
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
	float3 viewDir = normalize(CameraPos - wPos);

	//Directional light info
	float3 lightDir = LightDirection;
	float3 lightCol = LightColor;
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

	//indirect illumination
	float3 L_indirect = 0;

	float3 finalCol = L_direct + L_indirect;

	return finalCol;


}
