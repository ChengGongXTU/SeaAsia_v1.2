#include"texturemanager.h"
#include"DDSTextureLoader/DDSTextureLoader.h"

void TextureManager::CreateDxMemory(int ObjNum) {
	texViewPointer = new ID3D11ShaderResourceView*[ObjNum];
	sampDesc = new D3D11_SAMPLER_DESC[ObjNum];
	sampleStatePointer = new ID3D11SamplerState*[ObjNum];
	textures = new uint32_t*[ObjNum];
	dxTexure2D = new ID3D11Texture2D*[ObjNum];
	texNames = new const char*[ObjNum];

	cubemapSRVs = new ID3D11ShaderResourceView*[ObjNum];
	cubemapSampDesc = new D3D11_SAMPLER_DESC[ObjNum];
	cubemapSampleStatePointer = new ID3D11SamplerState*[ObjNum];
	cubemapTextures = new uint32_t*[ObjNum*6];
	dxCubemapTexture2D = new ID3D11Texture2D*[ObjNum];
	cubemapNames = new const char*[ObjNum];
	cubemapMipmapCount = new UINT[ObjNum];

	for (int i = 0; i < ObjNum; i++)
	{
		texViewPointer[i] = NULL;
		sampleStatePointer[i] = NULL;
		textures[i] = NULL;
		dxTexure2D[i] = NULL;
		texNames[i] = NULL;

		cubemapSRVs[i] = NULL;
		cubemapSampleStatePointer[i] = NULL;
		for (int j = 0; j < 6; j++)
		{
			cubemapTextures[i*6 +j] = NULL;
		}

		dxCubemapTexture2D[i] = NULL;
		cubemapNames[i] = NULL;
	}
}

void TextureManager::StartUp(DxDevice &dxDev) {
	totalTexureNumber = 0;
	endTextureId = 0;
	currentTextureId = -1;

	totalSamnpleDescNumber = 0;
	endSampleDescId = 0;
	currentSampleDescId = -1;

	totalSamplerSateNumber = 0;
	endSamplerStateId = 0;
	currentSamplerStateId = -1;

	totalCubemapNumber = 0;
		endCubemapTexId = 0;
		currentCubemapTexId = -1;
		endCubemapId = 0;

	CreateDxMemory(500);

	LoadDefaultImage(dxDev);

}

void TextureManager::ShutUp() {
	delete texViewPointer;
	delete sampDesc;
	delete sampleStatePointer;
}

bool TextureManager::DxLoadTexture(wstring fileName, DxDevice & dxDev)
{	
	if (fileName[0] == NULL)	return 0;
	HRESULT hr = CreateDDSTextureFromFile(dxDev.device, fileName.c_str(), nullptr, &texViewPointer[endTextureId]);
	 //HRESULT hr = D3DX11CreateShaderResourceViewFromFile(dxDev.device, fileName.c_str(), NULL, NULL, &texViewPointer[endTextureId], NULL);
	 if (FAILED(hr))	return 0;

	 currentTextureId = endTextureId;
	 endTextureId++;
	 totalTexureNumber++;
	 return true;
}

bool TextureManager::DxLoadImage(const char * fileName, DxDevice & dxDev)
{	
	//load image by freeimage
	FREE_IMAGE_FORMAT fif = FreeImage_GetFIFFromFilename(fileName);
	FIBITMAP* img = FreeImage_Load(fif, fileName);
	int texW = FreeImage_GetWidth(img);
	int texH = FreeImage_GetHeight(img);

	//create texture
	FIBITMAP* img1 = FreeImage_Rescale(img, texW, texH);
	uint32_t* tex =  textures[endTextureId] = new uint32_t[texW * texH];
	int bitCount = FreeImage_GetBPP(img1);
	if (bitCount == 32)
	{
		for (int y = 0; y < texH; y++)
			for (int x = 0; x < texW; x++)
			{
				RGBQUAD color;
				FreeImage_GetPixelColor(img1, x, texH - y - 1, &color);
				tex[y * texW + x] = (int)((color.rgbRed & 0x000000FF)
					| ((color.rgbGreen & 0x000000FF) << 8)
					| ((color.rgbBlue & 0x000000FF) << 16)
					| ((color.rgbReserved & 0x000000FF) << 24));
			}
	}
	else
	{
		for (int y = 0; y < texH; y++)
			for (int x = 0; x < texW; x++)
			{
				RGBQUAD color;
				color.rgbReserved = (byte)255;
				
				FreeImage_GetPixelColor(img1, x, texH - y - 1, &color);
				tex[y * texW + x] = (int)((color.rgbRed & 0x000000FF)
					| ((color.rgbGreen & 0x000000FF) << 8)
					| ((color.rgbBlue & 0x000000FF) << 16)
					| ((color.rgbReserved & 0x000000FF) << 24));
			}
	}

	FreeImage_Unload(img);
	FreeImage_Unload(img1);

	//create texture2D Desc
	//D3D11_TEXTURE2D_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(texDesc));
	texDesc.Width = texW;
	texDesc.Height = texH;
	texDesc.MipLevels = 0;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
	HRESULT hr =  dxDev.device->CreateTexture2D(&texDesc,NULL, &dxTexure2D[endTextureId]);

	//create resource_data desc
	//D3D11_SUBRESOURCE_DATA sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.pSysMem = textures[endTextureId];
	sd.SysMemPitch = texW * sizeof(uint32_t);
	sd.SysMemSlicePitch = texW * texH * sizeof(uint32_t);

	UINT rowPitch = (texW * 4) * sizeof(unsigned char);
	dxDev.context->UpdateSubresource(dxTexure2D[endTextureId], 0, NULL, textures[endTextureId], rowPitch, 0);

	//create texture2D

	ID3D11Texture2D* tex2 = dxTexure2D[endTextureId];
	//create shader resource view desc
	//D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = -1;
	srvDesc.Texture2D.MostDetailedMip = 0;

	//create shader resource view
	hr = dxDev.device->CreateShaderResourceView(dxTexure2D[endTextureId], &srvDesc, &texViewPointer[endTextureId]);
	dxDev.context->GenerateMips(texViewPointer[endTextureId]);

	
	texNames[endTextureId] = fileName;

	endTextureId++;
	totalTexureNumber++;

	return true;
}

bool TextureManager::DxLoadNonHDRCubemap(const char * fileName, DxDevice & dxDev)
{
	//load cubemap image by freeimage
	FREE_IMAGE_FORMAT fif = FreeImage_GetFIFFromFilename(fileName);
	FIBITMAP* cubeImg = FreeImage_Load(fif, fileName);

	//split cubemap image to six images
	int cubeW = FreeImage_GetWidth(cubeImg) / 4;
	int cubeH = FreeImage_GetHeight(cubeImg) / 3;
	int bitCount = FreeImage_GetBPP(cubeImg);

	//X+
	uint32_t* tex0 = cubemapTextures[endCubemapTexId] = new uint32_t[cubeW * cubeH];
	for (int y = 0; y < cubeH; y++)
		for (int x = 0 ; x < cubeW; x++)
		{
			RGBQUAD color;
			if (bitCount != 32) color.rgbReserved = (byte)255;
			FreeImage_GetPixelColor(cubeImg, x + cubeW * 2, cubeH * 2 - y - 1, &color);
			tex0[y * cubeW + x] = (int)((color.rgbRed & 0x000000FF)
				| ((color.rgbGreen & 0x000000FF) << 8)
				| ((color.rgbBlue & 0x000000FF) << 16)
				| ((color.rgbReserved & 0x000000FF) << 24));
		}

	//X-
	uint32_t* tex1 = cubemapTextures[endCubemapTexId + 1] = new uint32_t[cubeW * cubeH];
	for (int y = 0; y < cubeH; y++)
		for (int x = 0; x < cubeW; x++)
		{
			RGBQUAD color;
			if (bitCount != 32) color.rgbReserved = (byte)255;
			FreeImage_GetPixelColor(cubeImg, x, cubeH * 2 - y - 1, &color);
			tex1[y * cubeW + x] = (int)((color.rgbRed & 0x000000FF)
				| ((color.rgbGreen & 0x000000FF) << 8)
				| ((color.rgbBlue & 0x000000FF) << 16)
				| ((color.rgbReserved & 0x000000FF) << 24));
		}


	//Y+
	uint32_t* tex2 = cubemapTextures[endCubemapTexId + 2] = new uint32_t[cubeW * cubeH];
	for (int y = 0; y < cubeH; y++)
		for (int x = 0; x < cubeW; x++)
		{
			RGBQUAD color;
			if (bitCount != 32) color.rgbReserved = (byte)255;
			FreeImage_GetPixelColor(cubeImg, x + cubeW, cubeH * 3 - y - 1, &color);
			tex2[y * cubeW + x] = (int)((color.rgbRed & 0x000000FF)
				| ((color.rgbGreen & 0x000000FF) << 8)
				| ((color.rgbBlue & 0x000000FF) << 16)
				| ((color.rgbReserved & 0x000000FF) << 24));
		}

	//Y-
	uint32_t* tex3 = cubemapTextures[endCubemapTexId + 3] = new uint32_t[cubeW * cubeH];
	for (int y =0; y < cubeH; y++)
		for (int x = 0; x < cubeW; x++)
		{
			RGBQUAD color;
			if (bitCount != 32) color.rgbReserved = (byte)255;
			FreeImage_GetPixelColor(cubeImg, x + cubeW, cubeH - y - 1, &color);
			tex3[y * cubeW + x] = (int)((color.rgbRed & 0x000000FF)
				| ((color.rgbGreen & 0x000000FF) << 8)
				| ((color.rgbBlue & 0x000000FF) << 16)
				| ((color.rgbReserved & 0x000000FF) << 24));
		}

	//Z+
	uint32_t* tex4 = cubemapTextures[endCubemapTexId + 4] = new uint32_t[cubeW * cubeH];
	for (int y = 0; y < cubeH; y++)
		for (int x = 0; x < cubeW; x++)
		{
			RGBQUAD color;
			if (bitCount != 32) color.rgbReserved = (byte)255;
			FreeImage_GetPixelColor(cubeImg, x + cubeH, cubeH * 2 - y - 1, &color);
			tex4[y * cubeW + x] = (int)((color.rgbRed & 0x000000FF)
				| ((color.rgbGreen & 0x000000FF) << 8)
				| ((color.rgbBlue & 0x000000FF) << 16)
				| ((color.rgbReserved & 0x000000FF) << 24));
		}

	//Z-
	uint32_t* tex5 = cubemapTextures[endCubemapTexId + 5] = new uint32_t[cubeW * cubeH];
	for (int y = 0; y < cubeH; y++)
		for (int x = 0; x < cubeW; x++)
		{
			RGBQUAD color;
			if (bitCount != 32) color.rgbReserved = (byte)255;
			FreeImage_GetPixelColor(cubeImg, cubeW * 3 + x, cubeH * 2 - y - 1, &color);
			tex5[y * cubeW + x] = (int)((color.rgbRed & 0x000000FF)
				| ((color.rgbGreen & 0x000000FF) << 8)
				| ((color.rgbBlue & 0x000000FF) << 16)
				| ((color.rgbReserved & 0x000000FF) << 24));
		}

	FreeImage_Unload(cubeImg);


	ZeroMemory(&texDesc, sizeof(texDesc));
	texDesc.Width = cubeW;
	texDesc.Height = cubeH;
	texDesc.MipLevels = 0;
	texDesc.ArraySize = 6;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS | D3D11_RESOURCE_MISC_TEXTURECUBE;
	HRESULT hr = dxDev.device->CreateTexture2D(&texDesc, nullptr, &dxCubemapTexture2D[endCubemapId]);

	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;// D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.TextureCube.MipLevels = -1;
	srvDesc.Texture2D.MipLevels = -1;
	//srvDesc.TextureCube.MostDetailedMip = 0;
	hr = dxDev.device->CreateShaderResourceView(dxCubemapTexture2D[endCubemapId], &srvDesc, &cubemapSRVs[endCubemapId]);

	D3D11_SUBRESOURCE_DATA pData[6];

	for (int i = 0; i < 6; i++)
	{
		pData[i].pSysMem = cubemapTextures[endCubemapTexId + i];
		pData[i].SysMemPitch = cubeW * sizeof(uint32_t);
	}
	int MipMapCount = 1 + floor(log10((float)max(cubeW, cubeH)) / log10(2.0));
	cubemapMipmapCount[endCubemapId] = MipMapCount;
	for (int i = 0; i < 6; i++)
	{
		for (int j = 0; j < MipMapCount; j++)
		{
			dxDev.context->UpdateSubresource(dxCubemapTexture2D[endCubemapId], D3D11CalcSubresource(j, i, MipMapCount), 0, pData[i].pSysMem, pData[i].SysMemPitch, 0);
		}
	}

	dxDev.context->GenerateMips(cubemapSRVs[endCubemapId]);


	UINT rowPitch = (cubeW * 4) * sizeof(unsigned char);

	texNames[endCubemapId] = fileName;

	bool key = DxSetCubeSamplerState(D3D11_FILTER_MIN_MAG_MIP_LINEAR,
		D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP,
		D3D11_COMPARISON_NEVER, 0, D3D11_FLOAT32_MAX, 4,dxDev);

	endCubemapTexId += 6;
	endCubemapId++;
	totalCubemapNumber++;

	return true;
}

bool TextureManager::DxSetSamplerDesc(D3D11_FILTER filterType,
	D3D11_TEXTURE_ADDRESS_MODE adddU,
	D3D11_TEXTURE_ADDRESS_MODE adddV, 
	D3D11_TEXTURE_ADDRESS_MODE adddW,
	D3D11_COMPARISON_FUNC comparType,
	FLOAT minLod,FLOAT maxLod,
	UINT maxAni,
	FLOAT mipLodBias)
{
	sampDesc[endSampleDescId];
	ZeroMemory(&sampDesc[endSampleDescId], sizeof(sampDesc[endSampleDescId]));

	if (sampDesc[endSampleDescId].Filter != 0) return false;

	sampDesc[endSampleDescId].Filter = filterType;
	sampDesc[endSampleDescId].AddressU = adddU;
	sampDesc[endSampleDescId].AddressV = adddV;
	sampDesc[endSampleDescId].AddressW = adddW;
	sampDesc[endSampleDescId].ComparisonFunc = comparType;
	sampDesc[endSampleDescId].MinLOD = minLod;
	sampDesc[endSampleDescId].MaxLOD = maxLod;
	sampDesc[endSampleDescId].MaxAnisotropy = maxAni;
	sampDesc[endSampleDescId].MipLODBias = mipLodBias;

	endSampleDescId++;

	return true;

}

bool TextureManager::LoadDefaultImage(DxDevice & dxDev)
{
	DxLoadImage("Texture/Default_albedo.png", dxDev);
	DxLoadImage("Texture/Default_normal.png", dxDev);
	DxLoadImage("Texture/Default_mra.png", dxDev);
	DxLoadNonHDRCubemap("Texture/Default_skybox.bmp", dxDev);
	return true;
}

bool TextureManager::DxSetSamplerState(int &samplerDescId, DxDevice &dxDevice)
{
	HRESULT hr = dxDevice.device->CreateSamplerState(&sampDesc[samplerDescId], &sampleStatePointer[samplerDescId]);

	if (FAILED(hr))	return false;

	if (endSamplerStateId < samplerDescId)	endSamplerStateId = samplerDescId;

	return true;
}

bool TextureManager::DxSetCubeSamplerState(D3D11_FILTER filterType,
	D3D11_TEXTURE_ADDRESS_MODE adddU,
	D3D11_TEXTURE_ADDRESS_MODE adddV,
	D3D11_TEXTURE_ADDRESS_MODE adddW,
	D3D11_COMPARISON_FUNC comparType,
	FLOAT minLod, FLOAT maxLod,
	UINT maxAni,
	DxDevice &dxDevice)
{
	cubemapSampDesc[endCubemapId];
	ZeroMemory(&cubemapSampDesc[endSampleDescId], sizeof(cubemapSampDesc[endSampleDescId]));



	cubemapSampDesc[endCubemapId].Filter = filterType;
	cubemapSampDesc[endCubemapId].AddressU = adddU;
	cubemapSampDesc[endCubemapId].AddressV = adddV;
	cubemapSampDesc[endCubemapId].AddressW = adddW;
	cubemapSampDesc[endCubemapId].ComparisonFunc = comparType;
	cubemapSampDesc[endCubemapId].MinLOD = minLod;
	cubemapSampDesc[endCubemapId].MaxLOD = maxLod;
	cubemapSampDesc[endCubemapId].MaxAnisotropy = maxAni;

	HRESULT hr = dxDevice.device->CreateSamplerState(&cubemapSampDesc[endCubemapId], &cubemapSampleStatePointer[endCubemapId]);

	if (FAILED(hr))	return false;

	return true;

}
