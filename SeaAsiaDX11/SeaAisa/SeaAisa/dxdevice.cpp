#include"dxdevice.h"
#include"windowsDevice.h"

HRESULT DxDevice::CreateDevice() {
	HRESULT hr = NULL;
	hr = D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 0, creatDeviceFlag, 0, 0,
		D3D11_SDK_VERSION, &device, &featureLevel, &context);
	return hr;
}

HRESULT DxDevice::CreateDeviceAndSwap() {
	HRESULT hr = NULL;
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_0,
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);
	hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, creatDeviceFlag, featureLevels, numFeatureLevels, D3D11_SDK_VERSION, &sd,
		&swapChain, &device, &featureLevel, &context);
	return hr;
}

bool DxDevice::Init(WindowsDevice &Dev)
{
	// create device and context
	creatDeviceFlag = 0;//D3D11_CREATE_DEVICE_DEBUG;
	featureLevel = D3D_FEATURE_LEVEL_11_0;

	ZeroMemory(&sd, sizeof(sd));
	sd.BufferDesc.Width = Dev.w;
	sd.BufferDesc.Height = Dev.h;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;   //set the pixel's type
																 //mainDev.sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;      // raster uses to create an image on a surface (unspecified)
																 //mainDev.sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;              //how an image is stretched to fit a given monitor's resolution
	sd.SampleDesc.Count = 1;											// not use 4x MSAA
	sd.SampleDesc.Quality = 0;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;							//Use the surface or resource as a back buffer
	sd.BufferCount = 2;													// use one backbuffer for double buffering
	sd.OutputWindow = Dev.hwnd;									//NOTICE!!! get a window's handle for this swapchain
	sd.Windowed = true;											// true for window, false for full-scene;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;                           // the contents of the back buffer are discarded after calling Present().
	
	if (FAILED(CreateDeviceAndSwap())) {
		return 0;
	}


	// create render target view array and RT textures
	D3D11_TEXTURE2D_DESC textureDesc;
	ZeroMemory(&textureDesc, sizeof(textureDesc));
	textureDesc.Width = Dev.w;
	textureDesc.Height = Dev.h;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;

	device->CreateTexture2D(&textureDesc, NULL, &rtt[1]);
	device->CreateTexture2D(&textureDesc, NULL, &rtt[2]);
	device->CreateTexture2D(&textureDesc, NULL, &rtt[3]);

	rtv[0] = rtv[1] = rtv[2] = rtv[3] = 0;

	ID3D11Texture2D* backbuffer;
	swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backbuffer));  //backbuffer get data from mSwapChain
	device->CreateRenderTargetView(backbuffer, 0, &rtv[0]);				// use backbufer as render target
	Rel(backbuffer);

	//swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&rtt[1]));  //backbuffer get data from mSwapChain
	device->CreateRenderTargetView(rtt[1], 0, &rtv[1]);				// use backbufer as render target
	//swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&rtt[2]));  //backbuffer get data from mSwapChain
	device->CreateRenderTargetView(rtt[2], 0, &rtv[2]);				// use backbufer as render target
	//swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&rtt[3]));  //backbuffer get data from mSwapChain
	device->CreateRenderTargetView(rtt[3], 0, &rtv[3]);				// use backbufer as render target

	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	shaderResourceViewDesc.Format = textureDesc.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;
	device->CreateShaderResourceView(rtt[1], &shaderResourceViewDesc, &rtsrv[1]);
	device->CreateShaderResourceView(rtt[2], &shaderResourceViewDesc, &rtsrv[2]);
	device->CreateShaderResourceView(rtt[3], &shaderResourceViewDesc, &rtsrv[3]);

	// create depth/stencil buffer and view
	// set the parameter for buffer
	D3D11_TEXTURE2D_DESC dsDesc;
	ZeroMemory(&dsDesc, sizeof(dsDesc));
	dsDesc.Width = Dev.w;
	dsDesc.Height = Dev.h;
	dsDesc.MipLevels = 1;
	dsDesc.ArraySize = 1;
	dsDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;				// the format of element in array
	dsDesc.SampleDesc.Count = 1;								// MSAA
	dsDesc.SampleDesc.Quality = 0;
	dsDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;				// this texture is bound as depth and stencil
	dsDesc.Usage = D3D11_USAGE_DEFAULT;							// determine if CPU and GPU can read or write the resource 
	dsDesc.CPUAccessFlags = 0;									// how the CPU access the resource, set with "Usage" method
	dsDesc.MiscFlags = 0;										//Identifies other, less common options for resources.
																// create buffer and view
	ID3D11Texture2D* mDepthStencilBuffer = 0;
	HRESULT hr = device->CreateTexture2D(&dsDesc, NULL, &mDepthStencilBuffer);				// get point to depth buffer

	ID3D11DepthStencilState* depthState = NULL;
	D3D11_DEPTH_STENCIL_DESC depthDesc;
	ZeroMemory(&depthDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
	depthDesc.DepthEnable = true;
	depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthDesc.DepthFunc = D3D11_COMPARISON_LESS;

	depthDesc.StencilEnable = true;
	depthDesc.StencilReadMask = 0xFF;
	depthDesc.StencilWriteMask = 0xFF;

	depthDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	depthDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	hr = device->CreateDepthStencilState(&depthDesc, &depthState);
	context->OMSetDepthStencilState(depthState, 1);

	D3D11_DEPTH_STENCIL_VIEW_DESC dsDSV;
	ZeroMemory(&dsDSV, sizeof(dsDSV));
	dsDSV.Format = dsDesc.Format;
	dsDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsDSV.Texture2D.MipSlice = 0;
	hr = device->CreateDepthStencilView(mDepthStencilBuffer, &dsDSV, &dsv);  //get a view for depth buffer resource
																			 // bind RTV and DSV to Output Merge Stage																						// bind render target and depth/stencil view to output merger stage
	context->OMSetRenderTargets(1, &rtv[0], dsv);
	mDepthStencilBuffer->Release();

	//set the viewport
	//D3D11_VIEWPORT vp;
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	vp.Width = float(Dev.w);
	vp.Height = float(Dev.h);
	vp.MinDepth = 0.f;
	vp.MaxDepth = 1.0f;
	context->RSSetViewports(1, &vp);

	//clear the RTV and DSV
	float color[4] = { 1.0f,0.0f,0.0f,1.0f };
	context->ClearRenderTargetView(rtv[0], color);
	context->ClearRenderTargetView(rtv[1], color);
	context->ClearRenderTargetView(rtv[2], color);
	context->ClearRenderTargetView(rtv[3], color);
	context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH| D3D11_CLEAR_STENCIL, 1.0f, 0);
	
	swapChain->Present(0, 0);

	return true;
}

void DxDevice::CleanupRenderTarget()
{
	if (rtv[0]) { rtv[0]->Release(); rtv[0] = NULL; }
	if (rtv[1]) { rtv[1]->Release(); rtv[1] = NULL; }
	if (rtv[2]) { rtv[2]->Release(); rtv[2] = NULL; }
	if (rtv[3]) { rtv[3]->Release(); rtv[3] = NULL; }
	if (rtt[0]) { rtt[0]->Release(); rtt[0] = NULL; }
	if (rtt[1]) { rtt[1]->Release(); rtt[1] = NULL; }
	if (rtt[2]) { rtt[2]->Release(); rtt[2] = NULL; }
	if (rtt[3]) { rtt[3]->Release(); rtt[3] = NULL; }
	if (rtsrv[0]) { rtsrv[0]->Release(); rtsrv[0] = NULL; }
	if (rtsrv[1]) { rtsrv[1]->Release(); rtsrv[1] = NULL; }
	if (rtsrv[2]) { rtsrv[2]->Release(); rtsrv[2] = NULL; }
	if (rtsrv[3]) { rtsrv[3]->Release(); rtsrv[3] = NULL; }
}

void DxDevice::CreateRenderTarget()
{
	DXGI_SWAP_CHAIN_DESC sd;
	swapChain->GetDesc(&sd);


	D3D11_RENDER_TARGET_VIEW_DESC rtv_desc;
	ZeroMemory(&rtv_desc, sizeof(rtv_desc));
	rtv_desc.Format = sd.BufferDesc.Format;
	rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

	ID3D11Texture2D* pBackBuffer;
	swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	device->CreateRenderTargetView(pBackBuffer, &rtv_desc, &rtv[0]);
	pBackBuffer->Release();

	context->OMSetRenderTargets(1, &rtv[0], NULL);


}


