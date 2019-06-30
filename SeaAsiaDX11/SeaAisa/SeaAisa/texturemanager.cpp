#include"texturemanager.h"
#include"DDSTextureLoader/DDSTextureLoader.h"

void TextureManager::CreateDxMemory(int ObjNum) {
	texViewPointer = new ID3D11ShaderResourceView*[ObjNum];
	sampDesc = new D3D11_SAMPLER_DESC[ObjNum];
	sampleStatePointer = new ID3D11SamplerState*[ObjNum];
	textures = new uint32_t*[ObjNum];
	dxTexure2D = new ID3D11Texture2D*[ObjNum];
	for (int i = 0; i < ObjNum; i++)
	{
		texViewPointer[i] = NULL;
		sampleStatePointer[i] = NULL;
		textures[i] = NULL;
		dxTexure2D[i] = NULL;
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
	FIBITMAP* img = NULL;

	//create texture
	FIBITMAP* img1 = FreeImage_Rescale(img, 1024, 1024);
	uint32_t* tex =  textures[endTextureId] = new uint32_t[1024 * 1024];
	int bitCount = FreeImage_GetBPP(img1);
	if (bitCount == 32)
	{
		for (int y = 0; y <= 1024; y++)
			for (int x = 0; x <= 1024; x++)
			{
				RGBQUAD *color = NULL;
				FreeImage_GetPixelColor(img1, x, y, color);
				tex[y * 1024 + x] = (int)((color->rgbRed & 0xFF)
					| ((color->rgbGreen & 0xFF) << 8)
					| ((color->rgbBlue & 0xFF) << 16)
					| ((color->rgbReserved & 0xFF) << 24));
			}
	}
	else
	{
		for (int y = 0; y <= 1024; y++)
			for (int x = 0; x <= 1024; x++)
			{
				RGBQUAD *color = NULL;
				color->rgbReserved = (byte)1;
				FreeImage_GetPixelColor(img1, x, y, color);
				tex[y * 1024 + x] = (int)((color->rgbRed & 0xFF)
					| ((color->rgbGreen & 0xFF) << 8)
					| ((color->rgbBlue & 0xFF) << 16)
					| ((color->rgbReserved & 0xFF) << 24));
			}
	}

	//create texture2D Desc
	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = 1024;
	texDesc.Height = 1024;
	texDesc.MipLevels = 7;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	//create resource_data desc
	D3D11_SUBRESOURCE_DATA sd;
	sd.pSysMem = textures[endTextureId];
	sd.SysMemPitch = 1024 * sizeof(uint32_t);
	sd.SysMemSlicePitch = 1024 * 1024 * sizeof(uint32_t);

	//create texture2D
	dxDev.device->CreateTexture2D(&texDesc, &sd, &dxTexure2D[endTextureId]);

	//create shader resource view desc
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 7;
	srvDesc.Texture2D.MostDetailedMip = 0;

	//create shader resource view
	dxDev.device->CreateShaderResourceView(dxTexure2D[endTextureId], &srvDesc, &texViewPointer[endTextureId]);
	
	endTextureId++;

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

bool TextureManager::DxSetSamplerState(int &samplerDescId, DxDevice &dxDevice)
{
	HRESULT hr = dxDevice.device->CreateSamplerState(&sampDesc[samplerDescId], &sampleStatePointer[samplerDescId]);

	if (FAILED(hr))	return false;

	if (endSamplerStateId < samplerDescId)	endSamplerStateId = samplerDescId;

	return true;
}
