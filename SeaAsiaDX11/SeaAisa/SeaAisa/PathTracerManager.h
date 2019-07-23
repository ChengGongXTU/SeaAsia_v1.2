#pragma once
#include"SeeAisa.h"
#include"basicmanager.h"
#include"lowlevelrendermanager.h"
using namespace optix;
struct PtxSourceCache
{
	std::map<std::string, std::string *> map;
	~PtxSourceCache()
	{
		for (std::map<std::string, std::string *>::const_iterator it = map.begin(); it != map.end(); ++it)
			delete it->second;
	}
};

struct DirectionalLight
{
	optix::float3 dir;
	optix::float3 intensity;
};

struct PointLight
{	
	optix::float3 corner;
	optix::float3 intensity;
	optix::float1 range;
};

struct SpotLight
{
	optix::float3 corner;
	optix::float3 dir;
	optix::float3 intensity;
	optix::float1 range;
	optix::float1 angle_u;
	optix::float1 angle_p;
};

class PathTracerManager
{
public:
	//context
	Context context;
	Buffer buffer;
	const char *ptx;

	int rr_begin_depth;
	int sqrt_num_samples;
	uint32_t       width;
	uint32_t       height;


	//light
	Buffer directionalLightBuffer;
	Buffer pointLightBuffer;
	Buffer spotLightBuffer;

	//material
	const char *ptx2;
	Material diffuse;
	Program diffuse_ch;
	Program diffuse_ah;

	//geometry
	Program pgram_bounding_box;
	Program pgram_intersection;

	std::vector<GeometryInstance> gis;
	GeometryGroup shadow_group;
	GeometryGroup geometry_group;

	//ptx
	static PtxSourceCache g_ptxSourceCache;

	bool Setup(BasicManager &basicMng, LowLevelRendermanager &renderMng);
	const char* getPtxString(const char* sample, const char* filename, const char** log);
};