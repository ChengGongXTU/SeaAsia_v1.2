#include"texturemanager.h"
#include"DDSTextureLoader/DDSTextureLoader.h"

void TextureManager::CreateDxMemory(int ObjNum) {
	texViewPointer = new ID3D11ShaderResourceView*[ObjNum];
	sampDesc = new D3D11_SAMPLER_DESC[ObjNum];
	sampleStatePointer = new ID3D11SamplerState*[ObjNum];
	textures = new uint32_t*[ObjNum];
	dxTexure2D = new ID3D11Texture2D*[ObjNum];
	texNames = new const char*[ObjNum];
	for (int i = 0; i < ObjNum; i++)
	{
		texViewPointer[i] = NULL;
		sampleStatePointer[i] = NULL;
		textures[i] = NULL;
		dxTexure2D[i] = NULL;
		texNames[i] = NULL;
	}
}

void TextureManager::StartUp() {
	totalTexureNumber = 0;
	endTextureId = 0;
	currentTextureId = -1;

	totalSamnpleDescNumber = 0;
	endSampleDescId = 0;
	currentSampleDescId = -1;

	totalSamplerSateNumber = 0;
	endSamplerStateId = 0;
	currentSamplerStateId = -1;

	CreateDxMemory(500);

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

	//create texture
	FIBITMAP* img1 = FreeImage_Rescale(img, 1024, 1024);
	uint32_t* tex =  textures[endTextureId] = new uint32_t[1024 * 1024];
	int bitCount = FreeImage_GetBPP(img1);
	if (bitCount == 32)
	{
		for (int y = 0; y < 1024; y++)
			for (int x = 0; x < 1024; x++)
			{
				RGBQUAD color;
				FreeImage_GetPixelColor(img1, x, y, &color);
				tex[y * 1024 + x] = (int)((color.rgbRed & 0x000000FF)
					| ((color.rgbGreen & 0x000000FF) << 8)
					| ((color.rgbBlue & 0x000000FF) << 16)
					| ((color.rgbReserved & 0x000000FF) << 24));
			}
	}
	else
	{
		for (int y = 0; y < 1024; y++)
			for (int x = 0; x < 1024; x++)
			{
				RGBQUAD color;
				color.rgbReserved = (byte)255;
				
				FreeImage_GetPixelColor(img1, x, y, &color);
				tex[y * 1024 + x] = (int)((color.rgbRed & 0x000000FF)
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
	texDesc.Width = 1024;
	texDesc.Height = 1024;
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
	sd.SysMemPitch = 1024 * sizeof(uint32_t);
	sd.SysMemSlicePitch = 1024 * 1024 * sizeof(uint32_t);

	UINT rowPitch = (1024 * 4) * sizeof(unsigned char);
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

bool TextureManager::LoadDefeatImage(DxDevice & dxDev)
{
	DxLoadImage("Texture/defeat_albedo.png", dxDev);
	DxLoadImage("Texture/defeat_normal.png", dxDev);
	DxLoadImage("Texture/defeat_mra.png", dxDev);
	return true;
}

bool TextureManager::DxSetSamplerState(int &samplerDescId, DxDevice &dxDevice)
{
	HRESULT hr = dxDevice.device->CreateSamplerState(&sampDesc[samplerDescId], &sampleStatePointer[samplerDescId]);

	if (FAILED(hr))	return false;

	if (endSamplerStateId < samplerDescId)	endSamplerStateId = samplerDescId;

	return true;
}
