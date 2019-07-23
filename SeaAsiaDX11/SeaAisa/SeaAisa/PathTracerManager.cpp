#include "PathTracerManager.h"
/*
bool PathTracerManager::Setup(BasicManager & basicMng, LowLevelRendermanager & renderMng)
{	
	DxScene &scene = basicMng.sceneManager.sceneList[basicMng.sceneManager.currentSceneId];
	DxCamera & cam = scene.cameraList[scene.currentCameraId];

	//setup context
	context = Context::create();

	//setglobal parameter
	context->setRayTypeCount(2);
	context->setEntryPointCount(1);
	context->setStackSize(1800);
	context->setMaxTraceDepth(1);

	//create global varialbe
	rr_begin_depth = 4;
	sqrt_num_samples = 2;
	context["scene_epsilon"]->setFloat(1.e-3f);
	context["rr_begin_depth"]->setUint(rr_begin_depth);
	context["sqrt_num_samples"]->setUint(sqrt_num_samples);
	context["bad_color"]->setFloat(1000000.0f, 0.0f, 1000000.0f); // Super magenta to make sure it doesn't get averaged out in the progressive rendering.
	context["bg_color"]->setFloat(make_float3(0.0f, 0.0f, 0.0f));

	width = cam.cWidth;
	height = cam.cHeight;
	Buffer buffer;
	buffer = context->createBuffer(RT_BUFFER_OUTPUT, RT_FORMAT_FLOAT4, width, height);
	context["output_buffer"]->set(buffer);

	ptx = getPtxString(SAMPLE_NAME, "optixPathTracer.cu", 0);
	context->setRayGenerationProgram(0, context->createProgramFromPTXString(ptx, "pathtrace_camera"));
	context->setExceptionProgram(0, context->createProgramFromPTXString(ptx, "exception"));
	context->setMissProgram(0, context->createProgramFromPTXString(ptx, "miss"));

	return true;
}

const char* PathTracerManager::getPtxString(const char* sample,const char* filename,const char** log)
{
	if (log)
		*log = NULL;

	std::string *ptx, cu;
	std::string key = std::string(filename) + ";" + (sample ? sample : "");
	std::map<std::string, std::string *>::iterator elem = g_ptxSourceCache.map.find(key);

	if (elem == g_ptxSourceCache.map.end())
	{
		ptx = new std::string();
#if CUDA_NVRTC_ENABLED
		std::string location;
		getCuStringFromFile(cu, location, sample, filename);
		getPtxFromCuString(*ptx, sample, cu.c_str(), location.c_str(), log);
#else
		getPtxStringFromFile(*ptx, sample, filename);
#endif
		g_ptxSourceCache.map[key] = ptx;
	}
	else
	{
		ptx = elem->second;
	}

	return ptx->c_str();
}
*/
