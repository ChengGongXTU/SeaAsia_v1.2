#include "PathTracerManager.h"
#include"OptixFilePath.h"

const char* const SAMPLE_NAME = "optixPathTracer";

bool PathTracerManager::Setup(BasicManager & basicMng, LowLevelRendermanager & renderMng)
{	
	DxScene &scene = basicMng.sceneManager.sceneList[basicMng.sceneManager.currentSceneId];
	DxCamera & cam = scene.cameraList[scene.currentCameraId];

	//setup context
	context = optix::Context::create();

	//setglobal parameter
	context->setRayTypeCount(2);
	context->setEntryPointCount(1);
	context->setStackSize(1800);
	context->setMaxTraceDepth(1);

	//create global varialbe
	rr_begin_depth = 6;
	sqrt_num_samples = 128;
	context["scene_epsilon"]->setFloat(1.e-3f);
	context["rr_begin_depth"]->setUint(rr_begin_depth);
	context["sqrt_num_samples"]->setUint(sqrt_num_samples);
	context["bad_color"]->setFloat(1000000.0f, 0.0f, 1000000.0f); // Super magenta to make sure it doesn't get averaged out in the progressive rendering.
	context["bg_color"]->setFloat(optix::make_float3(0.0f, 0.0f, 0.0f));

	width = cam.cWidth;
	height = cam.cHeight;
	optix::Buffer buffer;
	buffer = context->createBuffer(RT_BUFFER_OUTPUT, RT_FORMAT_FLOAT4, width, height);
	context["output_buffer"]->set(buffer);

	ptx = getPtxString(SAMPLE_NAME, "optixPathTracer.cu", 0);
	context->setRayGenerationProgram(0, context->createProgramFromPTXString(ptx, "pathtrace_camera"));
	context->setExceptionProgram(0, context->createProgramFromPTXString(ptx, "exception"));
	context->setMissProgram(0, context->createProgramFromPTXString(ptx, "miss"));

	return true;
}

void PathTracerManager::CreateLight(BasicManager & basicMng, LowLevelRendermanager & renderMng)
{	
	int dirLightCount = renderMng.lightManager.dirLightCount;
	DxDirLight* dxDirLights = renderMng.lightManager.dxDirLights;
	DirectionalLight* dirLights = new DirectionalLight[dirLightCount];
	for (int i = 0; i < dirLightCount; i++)
	{
		dirLights[i].dir.x = dxDirLights[i].Dir.x;
		dirLights[i].dir.y = dxDirLights[i].Dir.y;
		dirLights[i].dir.z = dxDirLights[i].Dir.z;

		dirLights[i].intensity.x = dxDirLights[i].Color.x * dxDirLights[i].intensity;
		dirLights[i].intensity.y = dxDirLights[i].Color.y * dxDirLights[i].intensity;
		dirLights[i].intensity.z = dxDirLights[i].Color.z * dxDirLights[i].intensity;
	}

	optix::Buffer dirLightBuffer = context->createBuffer(RT_BUFFER_INPUT);
	dirLightBuffer->setFormat(RT_FORMAT_USER);
	dirLightBuffer->setElementSize(sizeof(DirectionalLight));
	dirLightBuffer->setSize(dirLightCount);
	memcpy(dirLightBuffer->map(), dirLights, sizeof(DirectionalLight) * dirLightCount);
	dirLightBuffer->unmap();
	context["dirLights"]->setBuffer(dirLightBuffer);
	delete []dirLights;
}

void PathTracerManager::CreateMaterial(BasicManager & basicMng, LowLevelRendermanager & renderMng)
{

	//create material for diffuse 
	diffuse = context->createMaterial();
	//create program
	//for diffuse closehit and shadow
	diffuse_ch = context->createProgramFromPTXString(ptx, "diffuse");
	diffuse_ah = context->createProgramFromPTXString(ptx, "shadow");
	diffuse->setClosestHitProgram(0, diffuse_ch);
	diffuse->setAnyHitProgram(1, diffuse_ah);
}

void PathTracerManager::CreatGeometry(BasicManager & basicMng, LowLevelRendermanager & renderMng)
{
	//get and create intersect and bounds
	ptx2 = getPtxString(SAMPLE_NAME, "optixGeometryTriangles.cu",0);
	//pgram_bounding_box = context->createProgramFromPTXString(ptx2, "bounds");
	//pgram_intersection = context->createProgramFromPTXString(ptx2, "intersect");

	//create a geometry
	Unity* unityList = basicMng.sceneManager.sceneList[basicMng.sceneManager.currentSceneId].unityList;
	int geometryCount = basicMng.sceneManager.sceneList[basicMng.sceneManager.currentSceneId].endUnityId;

	optix::float3 white = optix::make_float3(0.8f, 0.8f, 0.8f);

	shadow_group = context->createGroup();
	shadow_group->setAcceleration(context->createAcceleration("Trbvh"));

	geometry_group = context->createGroup();
	geometry_group->setAcceleration(context->createAcceleration("Trbvh"));

	for (int i = 0; i < geometryCount; i++)
	{
		Unity& unity = unityList[i];
		if (unity.empty == true)	continue;
		if (unity.objId < 0) continue;

		DxObj* mesh = basicMng.objManager.DxObjMem[unity.objId];

		unsigned num_faces = mesh->faceNum;
		unsigned num_vertices = mesh->vertexNum;

		// Define a regular tetrahedron of height 2, translated 1.5 units from the origin.
		//Tetrahedron tet(2.0f, make_float3(1.5f, 0.0f, 0.0f));

		// Create Buffers for the triangle vertices, normals, texture coordinates, and indices.
		optix::Buffer vertex_buffer = context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT3, num_vertices);
		optix::Buffer normal_buffer = context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT3, num_vertices);
		optix::Buffer texcoord_buffer = context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT2, num_vertices);
		optix::Buffer index_buffer = context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_UNSIGNED_INT3, num_faces);

		optix::float3* vertices = new optix::float3[num_vertices];
		optix::float3* normals = new optix::float3[num_vertices];
		optix::float2* texcoords = new optix::float2[num_vertices];
		unsigned* indices = new unsigned[num_faces * 3];

		for (int j = 0; j < num_vertices; j++)
		{
			vertices[j].x = mesh->vData[j].Pos.x;
			vertices[j].y = mesh->vData[j].Pos.y;
			vertices[j].z = mesh->vData[j].Pos.z;

			normals[j].x = mesh->vData[j].Normal.x;
			normals[j].y = mesh->vData[j].Normal.y;
			normals[j].z = mesh->vData[j].Normal.z;

			texcoords[j].x = mesh->vData[j].Tex.x;
			texcoords[j].y = mesh->vData[j].Tex.y;
		}

		for (int j = 0; j < num_faces * 3; j++)
		{
			indices[j] = mesh->indices[j];
		}


		// Copy the tetrahedron geometry into the device Buffers.
		memcpy(vertex_buffer->map(), vertices, sizeof(optix::float3)*num_vertices);
		memcpy(normal_buffer->map(), normals, sizeof(optix::float3)*num_vertices);
		memcpy(texcoord_buffer->map(), texcoords, sizeof(optix::float2)*num_vertices);
		memcpy(index_buffer->map(), indices, sizeof(unsigned) * num_faces * 3);

		vertex_buffer->unmap();
		normal_buffer->unmap();
		texcoord_buffer->unmap();
		index_buffer->unmap();

		// Create a GeometryTriangles object.
		optix::GeometryTriangles geom_tri = context->createGeometryTriangles();

		geom_tri->setPrimitiveCount(num_faces);
		geom_tri->setTriangleIndices(index_buffer, RT_FORMAT_UNSIGNED_INT3);
		geom_tri->setVertices(num_vertices, vertex_buffer, RT_FORMAT_FLOAT3);
		geom_tri->setBuildFlags(RTgeometrybuildflags(0));

		// Set an attribute program for the GeometryTriangles, which will compute
		// things like normals and texture coordinates based on the barycentric
		// coordindates of the intersection.
		geom_tri->setAttributeProgram(context->createProgramFromPTXString(ptx2, "triangle_attributes"));

		geom_tri["index_buffer"]->setBuffer(index_buffer);
		geom_tri["vertex_buffer"]->setBuffer(vertex_buffer);
		geom_tri["normal_buffer"]->setBuffer(normal_buffer);
		geom_tri["texcoord_buffer"]->setBuffer(texcoord_buffer);

		// Bind a Material to the GeometryTriangles.  Materials can be shared
		// between GeometryTriangles objects and other Geometry types, as long as
		// all of the attributes needed by the attached hit programs are produced in
		// the attribute program.
		optix::GeometryInstance tri_gi = context->createGeometryInstance(geom_tri, diffuse);
		tri_gi["diffuse_color"]->setFloat(white);
		
		optix::GeometryGroup gg = context->createGeometryGroup();
		gg->setAcceleration(context->createAcceleration("Trbvh"));
		gg->addChild(tri_gi);
		
		optix::Transform xform = context->createTransform();
		XMFLOAT4X4 m = unity.wolrdTransform.m.m;
		const float arr[16] = {
			m._11,m._12,m._13,m._14,
			m._21,m._22,m._23,m._24,
			m._31,m._32,m._33,m._34,
			m._41,m._42,m._43,m._44
		};
		optix::Matrix4x4 mat = optix::Matrix4x4(arr);
		//xform->setMatrix(false, arr, 0);
		xform->setMatrix(true, mat.getData(), mat.inverse().getData());
		xform->setChild(gg);

		//shadow_group->addChild(xform);
		geometry_group->addChild(xform);
		

		delete[] vertices;
		delete[] normals;
		delete[] texcoords;
		delete[] indices;
	}

	//create shadow group
	context["top_shadower"]->set(geometry_group);

	//add emission geometry and create geometry group
	context["top_object"]->set(geometry_group);

}

void calculateCameraVariables(optix::float3 eye, optix::float3 lookat, optix::float3 up,
	float  fov, float  aspect_ratio,
	optix::float3& U, optix::float3& V, optix::float3& W, bool fov_is_vertical)
{
	float ulen, vlen, wlen;
	W = lookat - eye; // Do not normalize W -- it implies focal length

	wlen = length(W);
	U = -normalize(cross(W, up)); 
	V = -normalize(cross(U, W));

	if (fov_is_vertical) {
		vlen = wlen * tanf(0.5f * fov * M_PIf / 180.0f);
		V *= vlen;
		ulen = vlen * aspect_ratio;
		U *= ulen;
	}
	else {
		ulen = wlen * tanf(0.5f * fov * M_PIf / 180.0f);
		U *= ulen;
		vlen = ulen / aspect_ratio;
		V *= vlen;
	}
}

void PathTracerManager::updateCamera(BasicManager& basicMng)
{	
	DxScene& scene = basicMng.sceneManager.sceneList[basicMng.sceneManager.currentSceneId];
	DxCamera& cam = scene.cameraList[scene.currentCameraId];

	const float fov = cam.aspect*360/3.1415;
	const float aspect_ratio = static_cast<float>(width) / static_cast<float>(height);
	
	optix::float3 camera_eye = optix::make_float3(cam.eye.x, cam.eye.y, cam.eye.z);
	optix::float3 camera_lookat = optix::make_float3(cam.at.x, cam.at.y, cam.at.z);
	optix::float3 camera_up = optix::make_float3(cam.up.x, cam.up.y, cam.up.z);

	optix::Matrix4x4 camera_rotate = optix::Matrix4x4::identity();

	optix::float3 camera_u, camera_v, camera_w;
	calculateCameraVariables(
		camera_eye, camera_lookat, camera_up, fov, aspect_ratio,
		camera_u, camera_v, camera_w, /*fov_is_vertical*/ true);

	const optix::Matrix4x4 frame = optix::Matrix4x4::fromBasis(
		normalize(camera_u),
		normalize(camera_v),
		normalize(-camera_w),
		camera_lookat);
	const optix::Matrix4x4 frame_inv = frame.inverse();
	// Apply camera rotation twice to match old SDK behavior
	const optix::Matrix4x4 trans = frame * camera_rotate*camera_rotate*frame_inv;

	camera_eye = make_float3(trans*make_float4(camera_eye, 1.0f));
	camera_lookat = make_float3(trans*make_float4(camera_lookat, 1.0f));
	camera_up = make_float3(trans*make_float4(camera_up, 0.0f));

	calculateCameraVariables(
		camera_eye, camera_lookat, camera_up, fov, aspect_ratio,
		camera_u, camera_v, camera_w, true);

	camera_rotate = optix::Matrix4x4::identity();
	//camera_changed = false;
	context["frame_number"]->setUint(1u);
	context["eye"]->setFloat(camera_eye);
	context["U"]->setFloat(camera_u);
	context["V"]->setFloat(camera_v);
	context["W"]->setFloat(camera_w);
}

void PathTracerManager::DestroyContext()
{
	if (context)
	{
		context->destroy();
		context = 0;
	}
}



void  PathTracerManager::SavePPM(const unsigned char *Pix, const char *fname, int wid, int hgt, int chan)
{
	if (Pix == NULL || wid < 1 || hgt < 1)
		throw optix::Exception("Image is ill-formed. Not saving");

	if (chan != 1 && chan != 3 && chan != 4)
		throw optix::Exception("Attempting to save image with channel count != 1, 3, or 4.");

	std::ofstream OutFile(fname, std::ios::out | std::ios::binary);
	if (!OutFile.is_open())
		throw optix::Exception("Could not open file for SavePPM");

	bool is_float = false;
	OutFile << 'P';
	OutFile << ((chan == 1 ? (is_float ? 'Z' : '5') : (chan == 3 ? (is_float ? '7' : '6') : '8'))) << std::endl;
	OutFile << wid << " " << hgt << std::endl << 255 << std::endl;

	OutFile.write(reinterpret_cast<char*>(const_cast<unsigned char*>(Pix)), wid * hgt * chan * (is_float ? 4 : 1));

	OutFile.close();
}

void  PathTracerManager::displayBufferPPM(const char* filename, RTbuffer buffer, bool disable_srgb_conversion)
{
	int width, height;
	RTsize buffer_width, buffer_height;

	void* imageData;
	rtBufferMap(buffer, &imageData);

	rtBufferGetSize2D(buffer, &buffer_width, &buffer_height);
	width = static_cast<int>(buffer_width);
	height = static_cast<int>(buffer_height);

	std::vector<unsigned char> pix(width * height * 3);

	RTformat buffer_format;
	rtBufferGetFormat(buffer, &buffer_format);

	const float gamma_inv = 1.0f / 2.2f;

	switch (buffer_format) {
	case RT_FORMAT_UNSIGNED_BYTE4:
		// Data is BGRA and upside down, so we need to swizzle to RGB
		for (int j = height - 1; j >= 0; --j) {
			unsigned char *dst = &pix[0] + (3 * width*(height - 1 - j));
			unsigned char *src = ((unsigned char*)imageData) + (4 * width*j);
			for (int i = 0; i < width; i++) {
				*dst++ = *(src + 2);
				*dst++ = *(src + 1);
				*dst++ = *(src + 0);
				src += 4;
			}
		}
		break;

	case RT_FORMAT_FLOAT:
		// This buffer is upside down
		for (int j = height - 1; j >= 0; --j) {
			unsigned char *dst = &pix[0] + width * (height - 1 - j);
			float* src = ((float*)imageData) + (3 * width*j);
			for (int i = 0; i < width; i++) {
				int P;
				if (disable_srgb_conversion)
					P = static_cast<int>((*src++) * 255.0f);
				else
					P = static_cast<int>(std::pow(*src++, gamma_inv) * 255.0f);
				unsigned int Clamped = P < 0 ? 0 : P > 0xff ? 0xff : P;

				// write the pixel to all 3 channels
				*dst++ = static_cast<unsigned char>(Clamped);
				*dst++ = static_cast<unsigned char>(Clamped);
				*dst++ = static_cast<unsigned char>(Clamped);
			}
		}
		break;

	case RT_FORMAT_FLOAT3:
		// This buffer is upside down
		for (int j = height - 1; j >= 0; --j) {
			unsigned char *dst = &pix[0] + (3 * width*(height - 1 - j));
			float* src = ((float*)imageData) + (3 * width*j);
			for (int i = 0; i < width; i++) {
				for (int elem = 0; elem < 3; ++elem) {
					int P;
					if (disable_srgb_conversion)
						P = static_cast<int>((*src++) * 255.0f);
					else
						P = static_cast<int>(std::pow(*src++, gamma_inv) * 255.0f);
					unsigned int Clamped = P < 0 ? 0 : P > 0xff ? 0xff : P;
					*dst++ = static_cast<unsigned char>(Clamped);
				}
			}
		}
		break;

	case RT_FORMAT_FLOAT4:
		// This buffer is upside down
		for (int j = height - 1; j >= 0; --j) {
			unsigned char *dst = &pix[0] + (3 * width*(height - 1 - j));
			float* src = ((float*)imageData) + (4 * width*j);
			for (int i = 0; i < width; i++) {
				for (int elem = 0; elem < 3; ++elem) {
					int P;
					if (disable_srgb_conversion)
						P = static_cast<int>((*src++) * 255.0f);
					else
						P = static_cast<int>(std::pow(*src++, gamma_inv) * 255.0f);
					unsigned int Clamped = P < 0 ? 0 : P > 0xff ? 0xff : P;
					*dst++ = static_cast<unsigned char>(Clamped);
				}

				// skip alpha
				src++;
			}
		}
		break;

	default:
		fprintf(stderr, "Unrecognized buffer data type or format.\n");
		exit(2);
		break;
	}

	SavePPM(&pix[0], filename, width, height, 3);

	// Now unmap the buffer
	rtBufferUnmap(buffer);
}

bool dirExists(const char* path)
{
#if defined(_WIN32)
	DWORD attrib = GetFileAttributesA(path);
	return (attrib != INVALID_FILE_ATTRIBUTES) && (attrib & FILE_ATTRIBUTE_DIRECTORY);
#else
	DIR* dir = opendir(path);
	if (dir == NULL)
		return false;

	closedir(dir);
	return true;
#endif
}

const char* samplesDir()
{
	static char s[512];
	/*
	// Allow for overrides.
	const char* dir = "D:\\WorkSoftware\\GitHub\\SeaAsia_v1.2\\SeaAsiaDX11\\SeaAisa\\SeaAisa\\OptiX SDK 6.0.0";
	if (dir) {
		strcpy_s(s, dir);
		return s;
	}
	*/

	// Return hardcoded path if it exists.
	if (dirExists(SAMPLES_DIR))
		return SAMPLES_DIR;

	// Last resort.
	return ".";
}

static bool readSourceFile(std::string &str, const std::string &filename)
{
	// Try to open file
	std::ifstream file(filename.c_str());
	if (file.good())
	{
		// Found usable source file
		std::stringstream source_buffer;
		source_buffer << file.rdbuf();
		str = source_buffer.str();
		return true;
	}
	return false;
}

static void getCuStringFromFile(std::string &cu, std::string& location, const char* sample_name, const char* filename)
{
	std::vector<std::string> source_locations;

	std::string base_dir = std::string(samplesDir());

	// Potential source locations (in priority order)
	if (sample_name)
		source_locations.push_back(base_dir + "/" + sample_name + "/" + filename);
	source_locations.push_back(base_dir + "/cuda/" + filename);

	for (std::vector<std::string>::const_iterator it = source_locations.begin(); it != source_locations.end(); ++it) {
		// Try to get source code from file
		if (readSourceFile(cu, *it))
		{
			location = *it;
			return;
		}

	}

	// Wasn't able to find or open the requested file
	throw optix::Exception("Couldn't open source file " + std::string(filename));
}

static std::string g_nvrtcLog;

static void getPtxFromCuString(std::string &ptx, const char* sample_name, const char* cu_source, const char* name, const char** log_string)
{
	// Create program
	nvrtcProgram prog = 0;
	nvrtcCreateProgram(&prog, cu_source, name, 0, NULL, NULL);

	// Gather NVRTC options
	std::vector<const char *> options;

	std::string base_dir = std::string(samplesDir());

	// Set sample dir as the primary include path
	std::string sample_dir;
	if (sample_name)
	{
		sample_dir = std::string("-I") + base_dir + "/" + sample_name;
		options.push_back(sample_dir.c_str());
	}

	// Collect include dirs
	std::vector<std::string> include_dirs;
	const char *abs_dirs[] = { SAMPLES_ABSOLUTE_INCLUDE_DIRS };
	const char *rel_dirs[] = { SAMPLES_RELATIVE_INCLUDE_DIRS };

	const size_t n_abs_dirs = sizeof(abs_dirs) / sizeof(abs_dirs[0]);
	for (size_t i = 0; i < n_abs_dirs; i++)
		include_dirs.push_back(std::string("-I") + abs_dirs[i]);
	const size_t n_rel_dirs = sizeof(rel_dirs) / sizeof(rel_dirs[0]);
	for (size_t i = 0; i < n_rel_dirs; i++)
		include_dirs.push_back(std::string("-I") + base_dir + rel_dirs[i]);
	for (std::vector<std::string>::const_iterator it = include_dirs.begin(); it != include_dirs.end(); ++it)
		options.push_back(it->c_str());

	// Collect NVRTC options
	const char *compiler_options[] = { CUDA_NVRTC_OPTIONS };
	const size_t n_compiler_options = sizeof(compiler_options) / sizeof(compiler_options[0]);
	for (size_t i = 0; i < n_compiler_options - 1; i++)
		options.push_back(compiler_options[i]);

	// JIT compile CU to PTX
	const nvrtcResult compileRes = nvrtcCompileProgram(prog, (int)options.size(), options.data());

	// Retrieve log output
	size_t log_size = 0;
	nvrtcGetProgramLogSize(prog, &log_size);
	g_nvrtcLog.resize(log_size);
	if (log_size > 1)
	{
		nvrtcGetProgramLog(prog, &g_nvrtcLog[0]);
		if (log_string)
			*log_string = g_nvrtcLog.c_str();
	}
	if (compileRes != NVRTC_SUCCESS)
		throw optix::Exception("NVRTC Compilation failed.\n" + g_nvrtcLog);

	// Retrieve PTX code
	size_t ptx_size = 0;
	nvrtcGetPTXSize(prog, &ptx_size);
	ptx.resize(ptx_size);
	nvrtcGetPTX(prog, &ptx[0]);

	// Cleanup
	nvrtcDestroyProgram(&prog);
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

void PathTracerManager::Bake(BasicManager &basicMng, LowLevelRendermanager &renderMng)
{
	Setup(basicMng, renderMng);
	CreateLight(basicMng, renderMng);
	CreateMaterial(basicMng, renderMng);
	CreatGeometry(basicMng, renderMng);
	updateCamera(basicMng);
	context->launch(0, width, height);
	displayBufferPPM("D:/WorkSoftware/GitHub/SeaAsia_v1.2/SeaAsiaDX11/SeaAisa/SeaAisa/OptiX SDK 6.0.0/pathtrace.ppm" , context["output_buffer"]->getBuffer()->get(), false);
	DestroyContext();
}