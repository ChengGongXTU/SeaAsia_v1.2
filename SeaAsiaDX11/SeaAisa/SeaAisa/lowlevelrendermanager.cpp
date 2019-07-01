#include "lowlevelrendermanager.h"
using namespace fbxsdk;

bool LowLevelRendermanager::StartUp()
{
	shaderManager.StartUp();
	primitiveManager.StartUp();
	lightManager.StartUp();
	cameraManager.StartUp();

	if (primitiveManager.objId != -1)	return false;

	return true;
}

void LowLevelRendermanager::ShutUp()
{
	shaderManager.ShutUp();
	primitiveManager.ShutUp();
	lightManager.ShutUp();
	cameraManager.ShutUp();
}

bool LowLevelRendermanager::SetScene(DxDevice &dev,DxScene & scene)
{
	if(!cameraManager.LoadCamera(dev, scene.cameraList[0]))	return false;
	if (!lightManager.SetDirLight(dev, scene.dlList[0]))	return false;

	return true;

}

bool LowLevelRendermanager::DrawUnity(BasicManager & basicMng, Unity & unity)
{	
	//vertex
	if (!primitiveManager.LoadUnity(basicMng.dxDevice, basicMng.objManager, unity))	return false;
	
	if (!primitiveManager.InputVertexBuffer(basicMng.dxDevice, unity, shaderManager))	return false;
	
	//camera
	XMMATRIX m = XMLoadFloat4x4(&unity.wolrdTransform.m.m);
	m = XMMatrixTranspose(m);
	basicMng.dxDevice.context->UpdateSubresource(cameraManager.worldjTransformBuffer, 0, NULL, &m, 0, 0);
	basicMng.dxDevice.context->VSSetConstantBuffers(0, 1, &cameraManager.worldjTransformBuffer);

	if (unity.textureId != -1)
	{
		basicMng.dxDevice.context->PSSetShaderResources(0, 1, &basicMng.textureManager.texViewPointer[unity.textureId]);
		basicMng.dxDevice.context->PSSetSamplers(0, 1, &basicMng.textureManager.sampleStatePointer[unity.samplerStateId]);
	}

	ID3D11Buffer* materialConstant = NULL;
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(MaterialParameter);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	HRESULT hr = basicMng.dxDevice.device->CreateBuffer(&bd, NULL, &materialConstant);
	if (FAILED(hr))	return false;

	for (int i = 0; i < unity.materialNum; i++) {

		// update material
		basicMng.dxDevice.context->UpdateSubresource(materialConstant, 0, 0,
			&basicMng.materialsManager.dxMaterial[unity.MaterialsIdIndex[i]].parameter, 0, 0);

		//input material to constant buffer slot 4th
		basicMng.dxDevice.context->PSSetConstantBuffers(4, 1, &materialConstant);

		//draw mesh which belong to this material
		//basicMng.dxDevice.context->DrawIndexed(basicMng.objManager.DxObjMem[unity.objId]->FaceNumInEachMtl[i] * 3,
			//basicMng.objManager.DxObjMem[unity.objId]->beginFaceInEachMtl[i]*3, 0);
		basicMng.dxDevice.context->Draw(basicMng.objManager.DxObjMem[unity.objId]->vertexNum, 0);
	}
	if(materialConstant != NULL) materialConstant->Release();
	//basicMng.dxDevice.context->UpdateSubresource(materialConstant, 0, 0,
	//	&basicMng.materialsManager.dxMaterial[unity.MaterialsIdIndex[0]].parameter, 0, 0);
	//basicMng.dxDevice.context->PSSetConstantBuffers(4, 1, &materialConstant);

	//basicMng.dxDevice.context->DrawIndexed(basicMng.objManager.DxObjMem[unity.objId]->faceNum*3, 0, 0);
	
	return true;

}

bool LowLevelRendermanager::DrawSceneUnity(BasicManager & basicMng, int SceneId,int cameraId, int dlightId)
{	
	int unityNum = basicMng.sceneManager.sceneList[SceneId].endUnityId;
	DxScene& scene = basicMng.sceneManager.sceneList[SceneId];

	if (unityNum <= 0)	return false;
	
	for (int i = 0; i < unityNum; i++)
	{
		if (scene.unityList[i].objId == -1)	continue;

		if (!DrawUnity(basicMng, scene.unityList[i])) return false;

	}

	return true;
}

bool LowLevelRendermanager::DeferredDrawGeometry(BasicManager & basicMng, Unity & unity)
{
	//vertex
	if (!primitiveManager.LoadUnity(basicMng.dxDevice, basicMng.objManager, unity))	return false;

	if (!primitiveManager.InputVertexBufferGeometryShading(basicMng.dxDevice, unity, shaderManager))	return false;

	//camera
	XMMATRIX m = XMLoadFloat4x4(&unity.wolrdTransform.m.m);
	m = XMMatrixTranspose(m);
	if (cameraManager.worldjTransformBuffer == NULL) cameraManager.CreateWorldBuffer(basicMng.dxDevice);
	basicMng.dxDevice.context->UpdateSubresource(cameraManager.worldjTransformBuffer, 0, NULL, &m, 0, 0);
	basicMng.dxDevice.context->VSSetConstantBuffers(0, 1, &cameraManager.worldjTransformBuffer);

	if (unity.textureId != -1)
	{
		basicMng.dxDevice.context->PSSetShaderResources(0, 1, &basicMng.textureManager.texViewPointer[unity.textureId]);
		basicMng.dxDevice.context->PSSetSamplers(0, 1, &basicMng.textureManager.sampleStatePointer[unity.samplerStateId]);
	}

	ID3D11Buffer* materialConstant = NULL;
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(MaterialParameter);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	HRESULT hr = basicMng.dxDevice.device->CreateBuffer(&bd, NULL, &materialConstant);
	if (FAILED(hr))	return false;

	for (int i = 0; i < unity.materialNum; i++) {

		// update material
		basicMng.dxDevice.context->UpdateSubresource(materialConstant, 0, 0,
			&basicMng.materialsManager.dxMaterial[unity.MaterialsIdIndex[i]].parameter, 0, 0);

		//input material to constant buffer slot 4th
		basicMng.dxDevice.context->PSSetConstantBuffers(4, 1, &materialConstant);

		//draw mesh which belong to this material
		basicMng.dxDevice.context->DrawIndexed(basicMng.objManager.DxObjMem[unity.objId]->FaceNumInEachMtl[i] * 3,
			basicMng.objManager.DxObjMem[unity.objId]->beginFaceInEachMtl[i]*3, 0);
		//basicMng.dxDevice.context->Draw(basicMng.objManager.DxObjMem[unity.objId]->vertexNum, 0);
	}
	if (materialConstant != NULL) materialConstant->Release();
	return true;

}

bool LowLevelRendermanager::DeferredDrawSceneGeometry(BasicManager & basicMng, int SceneId, int cameraId, int dlightId)
{
	int unityNum = basicMng.sceneManager.sceneList[SceneId].endUnityId;
	DxScene& scene = basicMng.sceneManager.sceneList[SceneId];

	if (unityNum <= 0)	return false;

	for (int i = 0; i < unityNum; i++)
	{
		if (scene.unityList[i].objId == -1)	continue;

		if (!DeferredDrawGeometry(basicMng, scene.unityList[i])) return false;

	}

	return true;
}

bool LowLevelRendermanager::DeferredDrawSceneLighting(BasicManager & basicMng, int SceneId, int cameraId, int dlightId)
{
	int unityNum = basicMng.sceneManager.sceneList[SceneId].endUnityId;
	DxScene& scene = basicMng.sceneManager.sceneList[SceneId];

	//pp vertex; two triangles
	if (!primitiveManager.LoadPPVertex(basicMng.dxDevice, basicMng.objManager))	return false;
	if (!primitiveManager.InputVertexBufferLightShading(basicMng.dxDevice, shaderManager))	return false;

	//G buffer RT
	basicMng.dxDevice.context->PSSetShaderResources(0, 1, &basicMng.dxDevice.rtsrv[1]);
	basicMng.dxDevice.context->PSSetSamplers(0, 1, &basicMng.dxDevice.rtSampler[1]);
	basicMng.dxDevice.context->PSSetShaderResources(1,1, &basicMng.dxDevice.rtsrv[2]);
	basicMng.dxDevice.context->PSSetSamplers(1, 1, &basicMng.dxDevice.rtSampler[2]);
	basicMng.dxDevice.context->PSSetShaderResources(2, 1, &basicMng.dxDevice.rtsrv[3]);
	basicMng.dxDevice.context->PSSetSamplers(2, 1, &basicMng.dxDevice.rtSampler[3]);

	basicMng.dxDevice.context->DrawIndexed(6, 0, 0);

	return true;
}

void LowLevelRendermanager::RenderScene(BasicManager & basicMng, WindowsDevice &wnDev,int SceneId)
{	
	DxScene& scene = basicMng.sceneManager.sceneList[SceneId];

	/*ResizeRenderpipeline(basicMng, wnDev);*/

	float color[4] = { 0.5f,0.5f,0.5f,1.0f };
	float black[4] = { 0.0f,0.0f,0.0f,1.0f };
	basicMng.dxDevice.context->ClearRenderTargetView(basicMng.dxDevice.rtv[0], color);
	basicMng.dxDevice.context->ClearRenderTargetView(basicMng.dxDevice.rtv[1], black);
	basicMng.dxDevice.context->ClearRenderTargetView(basicMng.dxDevice.rtv[2], black);
	basicMng.dxDevice.context->ClearRenderTargetView(basicMng.dxDevice.rtv[3], black);
	basicMng.dxDevice.context->ClearDepthStencilView(basicMng.dxDevice.dsv, D3D11_CLEAR_DEPTH| D3D11_CLEAR_STENCIL, 1.0f,0);
	
	shaderManager.InputVertexShader(basicMng.dxDevice, 0);

	shaderManager.InputPixelShader(basicMng.dxDevice, 0);
	
	if (cameraManager.viewTransformBuffer != NULL)
	{
		cameraManager.LoadCamera(basicMng.dxDevice, scene.cameraList[scene.currentCameraId]);
		cameraManager.InputCamera(basicMng.dxDevice);
	}

	if (lightManager.DirLightBuffer != NULL)
	{	
		lightManager.SetDirLight(basicMng.dxDevice, scene.dlList[scene.currentDlId]);
		DrawSceneUnity(basicMng, SceneId,0, scene.currentDlId);		
	}

}

void LowLevelRendermanager::DeferredRenderScene(BasicManager & basicMng, WindowsDevice &wnDev, int SceneId)
{
	DxScene& scene = basicMng.sceneManager.sceneList[SceneId];

	/*ResizeRenderpipeline(basicMng, wnDev);*/

	float color[4] = { 0.5f,0.5f,0.5f,1.0f };
	float black[4] = { 0.0f,0.0f,0.0f,1.0f };
	float red[4] = { 1.0f,0.0f,0.0f,1.0f };
	float green[4] = { 0.0f,1.0f,0.0f,1.0f };
	float blue[4] = { 0.0f,0.0f,1.0f,1.0f };
	basicMng.dxDevice.context->ClearRenderTargetView(basicMng.dxDevice.rtv[0], color);
	basicMng.dxDevice.context->ClearRenderTargetView(basicMng.dxDevice.rtv[1], red);
	basicMng.dxDevice.context->ClearRenderTargetView(basicMng.dxDevice.rtv[2], green);
	basicMng.dxDevice.context->ClearRenderTargetView(basicMng.dxDevice.rtv[3], blue);
	basicMng.dxDevice.context->ClearDepthStencilView(basicMng.dxDevice.dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

//------------------------compute G-buffer---------------------------------------------------------------------------
 	//------set 3 G-buffer rtv-----------
	basicMng.dxDevice.context->OMSetRenderTargets(3, &basicMng.dxDevice.rtv[1], NULL);
	//------bind vs and ps -----------
	shaderManager.InputVertexShader(basicMng.dxDevice, 0);
	shaderManager.InputPixelShader(basicMng.dxDevice, 0);
	//-----input camera matrix constant-------
	if (cameraManager.viewTransformBuffer != NULL)
	{
		cameraManager.LoadCamera(basicMng.dxDevice, scene.cameraList[scene.currentCameraId]);
		cameraManager.InputCamera(basicMng.dxDevice);
	}
	//------------draw Geometry buffer------------ 
	DeferredDrawSceneGeometry(basicMng, SceneId, 0, scene.currentDlId);

//------------------------light shading---------------------------------------------------------------------------
	//------set first rtv-----------
	basicMng.dxDevice.context->OMSetRenderTargets(1, &basicMng.dxDevice.rtv[0], basicMng.dxDevice.dsv);

	shaderManager.InputVertexShader(basicMng.dxDevice, 1);
	shaderManager.InputPixelShader(basicMng.dxDevice, 1);
	//-----input camera matrix constant-------
	if (cameraManager.viewTransformBuffer != NULL)
	{
		cameraManager.LoadCamera(basicMng.dxDevice, scene.cameraList[scene.currentCameraId]);
		cameraManager.InputCamera(basicMng.dxDevice);
	}
	//-------- input light constant--------------
	if (lightManager.DirLightBuffer != NULL)
	{
		lightManager.SetDirLight(basicMng.dxDevice, scene.dlList[scene.currentDlId]);
	}
	DeferredDrawSceneLighting(basicMng, SceneId, 0, scene.currentDlId);

}



void LowLevelRendermanager::SetViewPort(BasicManager & basicMng, float x, float y, float w, float h, float mind, float maxd)
{
	basicMng.dxDevice.vp.TopLeftX = x;
	basicMng.dxDevice.vp.TopLeftY = y;
	basicMng.dxDevice.vp.Width = w;
	basicMng.dxDevice.vp.Height = h;
	basicMng.dxDevice.vp.MinDepth = mind;
	basicMng.dxDevice.vp.MaxDepth = maxd;
	basicMng.dxDevice.context->RSSetViewports(1, &basicMng.dxDevice.vp);
}

void LowLevelRendermanager::ResizeRenderpipeline(BasicManager &basicMng, WindowsDevice &wnDev)
{
	RECT rect;
	GetClientRect(wnDev.hwnd, &rect);
	float w = (float)(rect.right - rect.left);
	float h = (float)(rect.bottom - rect.top);

	if (w == NULL)	return;

	if (wnDev.w != w || wnDev.h != h)
	{
		wnDev.w = w;
		wnDev.h = h;
		DxDevice &dxdev = basicMng.dxDevice;		

		// create render target view array and RT textures
		D3D11_TEXTURE2D_DESC textureDesc;
		ZeroMemory(&textureDesc, sizeof(textureDesc));
		textureDesc.Width = w;
		textureDesc.Height = h;
		textureDesc.MipLevels = 1;
		textureDesc.ArraySize = 1;
		textureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.Usage = D3D11_USAGE_DEFAULT;
		textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		textureDesc.CPUAccessFlags = 0;
		textureDesc.MiscFlags = 0;

		basicMng.dxDevice.device->CreateTexture2D(&textureDesc, NULL, &basicMng.dxDevice.rtt[1]);
		basicMng.dxDevice.device->CreateTexture2D(&textureDesc, NULL, &basicMng.dxDevice.rtt[2]);
		basicMng.dxDevice.device->CreateTexture2D(&textureDesc, NULL, &basicMng.dxDevice.rtt[3]);

		basicMng.dxDevice.rtv[1] = basicMng.dxDevice.rtv[2] = basicMng.dxDevice.rtv[3] = 0;

		//swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&rtt[1]));  //backbuffer get data from mSwapChain
		basicMng.dxDevice.device->CreateRenderTargetView(basicMng.dxDevice.rtt[1], 0, &basicMng.dxDevice.rtv[1]);				// use backbufer as render target
		//swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&rtt[2]));  //backbuffer get data from mSwapChain
		basicMng.dxDevice.device->CreateRenderTargetView(basicMng.dxDevice.rtt[2], 0, &basicMng.dxDevice.rtv[2]);				// use backbufer as render target
		//swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&rtt[3]));  //backbuffer get data from mSwapChain
		basicMng.dxDevice.device->CreateRenderTargetView(basicMng.dxDevice.rtt[3], 0, &basicMng.dxDevice.rtv[3]);				// use backbufer as render target

		D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
		shaderResourceViewDesc.Format = textureDesc.Format;
		shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
		shaderResourceViewDesc.Texture2D.MipLevels = 1;
		basicMng.dxDevice.device->CreateShaderResourceView(basicMng.dxDevice.rtt[1], &shaderResourceViewDesc, &basicMng.dxDevice.rtsrv[1]);
		basicMng.dxDevice.device->CreateShaderResourceView(basicMng.dxDevice.rtt[2], &shaderResourceViewDesc, &basicMng.dxDevice.rtsrv[2]);
		basicMng.dxDevice.device->CreateShaderResourceView(basicMng.dxDevice.rtt[3], &shaderResourceViewDesc, &basicMng.dxDevice.rtsrv[3]);


		// create the sampler
		D3D11_SAMPLER_DESC samplerDescription;
		samplerDescription.Filter = D3D11_FILTER_ANISOTROPIC;
		samplerDescription.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDescription.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDescription.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDescription.MipLODBias = 0.0f;
		//samplerDescription.MaxAnisotropy = m_featureLevel > D3D_FEATURE_LEVEL_9_1 ? 4 : 2;
		samplerDescription.ComparisonFunc = D3D11_COMPARISON_NEVER;
		samplerDescription.BorderColor[0] = 0.0f;
		samplerDescription.BorderColor[1] = 0.0f;
		samplerDescription.BorderColor[2] = 0.0f;
		samplerDescription.BorderColor[3] = 0.0f;
		samplerDescription.MinLOD = 0;      // This allows the use of all mip levels
		samplerDescription.MaxLOD = D3D11_FLOAT32_MAX;
		basicMng.dxDevice.device->CreateSamplerState(&samplerDescription, &basicMng.dxDevice.rtSampler[0]);
		basicMng.dxDevice.device->CreateSamplerState(&samplerDescription, &basicMng.dxDevice.rtSampler[1]);
		basicMng.dxDevice.device->CreateSamplerState(&samplerDescription, &basicMng.dxDevice.rtSampler[2]);
		basicMng.dxDevice.device->CreateSamplerState(&samplerDescription, &basicMng.dxDevice.rtSampler[3]);

		// create depth/stencil buffer and view
		// set the parameter for buffer
		D3D11_TEXTURE2D_DESC dsDesc;
		ZeroMemory(&dsDesc, sizeof(dsDesc));
		dsDesc.Width = w;
		dsDesc.Height = h;
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
		HRESULT hr = dxdev.device->CreateTexture2D(&dsDesc, NULL, &mDepthStencilBuffer);				// get point to depth buffer

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

		hr = dxdev.device->CreateDepthStencilState(&depthDesc, &depthState);
		dxdev.context->OMSetDepthStencilState(depthState, 1);

		D3D11_DEPTH_STENCIL_VIEW_DESC dsDSV;
		ZeroMemory(&dsDSV, sizeof(dsDSV));
		dsDSV.Format = dsDesc.Format;
		dsDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		dsDSV.Texture2D.MipSlice = 0;
		hr = dxdev.device->CreateDepthStencilView(mDepthStencilBuffer, &dsDSV, &dxdev.dsv);  //get a view for depth buffer resource
																				 // bind RTV and DSV to Output Merge Stage																						// bind render target and depth/stencil view to output merger stage
		dxdev.context->OMSetRenderTargets(1, &dxdev.rtv[0], dxdev.dsv);
		mDepthStencilBuffer->Release();

		

		//set the viewport
		//D3D11_VIEWPORT vp;
		dxdev.vp.TopLeftX = 0.0f;
		dxdev.vp.TopLeftY = 0.0f;
		dxdev.vp.Width = float(w);
		dxdev.vp.Height = float(h);
		dxdev.vp.MinDepth = 0.f;
		dxdev.vp.MaxDepth = 1.0f;
		dxdev.context->RSSetViewports(1, &dxdev.vp);

		//reseize camera
		if (basicMng.sceneManager.scenenNum > 0)
		{
		DxScene scene = basicMng.sceneManager.sceneList[basicMng.sceneManager.currentSceneId];
		if (scene.cameraNum > 0)
		{
			DxCamera cam = scene.cameraList[scene.currentCameraId];
			cam.cWidth = w;
			cam.cHeight = h;
			cam.ResizeCamera();
		}

		}


	}
}

void LowLevelRendermanager::ReverseUnityNormal(BasicManager & basicMng, Unity & unity)
{	
	DxObj* obj = basicMng.objManager.DxObjMem[unity.objId];
	int normalNum = obj->vertexNum;

	for (int i = 0; i < normalNum; i++)
	{
		obj->vData[i].Normal.x = -obj->vData[i].Normal.x;
		obj->vData[i].Normal.y = -obj->vData[i].Normal.y;
		obj->vData[i].Normal.z = -obj->vData[i].Normal.z;
	}

	ZeroMemory(&primitiveManager.bd, sizeof(primitiveManager.bd));
	ZeroMemory(&primitiveManager.data, sizeof(primitiveManager.data));

	primitiveManager.bd.Usage = D3D11_USAGE_DEFAULT;
	primitiveManager.bd.ByteWidth = sizeof(vertexData)*(basicMng.objManager.DxObjMem[unity.objId]->vertexNum);
	primitiveManager.bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	primitiveManager.bd.CPUAccessFlags = 0;
	primitiveManager.data.pSysMem = basicMng.objManager.DxObjMem[unity.objId]->vData;
	HRESULT hr =basicMng.dxDevice.device->CreateBuffer(&primitiveManager.bd, &primitiveManager.data, &primitiveManager.vertexBuffer);
}

void LowLevelRendermanager::ReverseUnityNormalZaxis(BasicManager & basicMng, Unity & unity)
{
	DxObj* obj = basicMng.objManager.DxObjMem[unity.objId];
	int normalNum = obj->vertexNum;

	for (int i = 0; i < normalNum; i++)
	{
		obj->vData[i].Normal.z = -obj->vData[i].Normal.z;
	}

	ZeroMemory(&primitiveManager.bd, sizeof(primitiveManager.bd));
	ZeroMemory(&primitiveManager.data, sizeof(primitiveManager.data));

	primitiveManager.bd.Usage = D3D11_USAGE_DEFAULT;
	primitiveManager.bd.ByteWidth = sizeof(vertexData)*(basicMng.objManager.DxObjMem[unity.objId]->vertexNum);
	primitiveManager.bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	primitiveManager.bd.CPUAccessFlags = 0;
	primitiveManager.data.pSysMem = basicMng.objManager.DxObjMem[unity.objId]->vData;
	HRESULT hr = basicMng.dxDevice.device->CreateBuffer(&primitiveManager.bd, &primitiveManager.data, &primitiveManager.vertexBuffer);
}

void LowLevelRendermanager::RenderMaterialChange(BasicManager & basicMng, int materialID, MaterialParameter & para,int type)
{	/*
	MaterialParameter& currentPara = basicMng.materialsManager.dxMaterial[materialID].parameter;
	currentPara.Ka = para.Ka;
	currentPara.Kd = para.Kd;
	currentPara.Ks = para.Ks;
	currentPara.Ke = para.Ke;
	currentPara.alpha = para.alpha;
	currentPara.illum = para.illum;
	currentPara.Ns = para.Ns;
	currentPara.Ni = para.Ni;

	if (type == 0)	basicMng.materialsManager.dxMaterial[materialID].mtlType == matte;
	if (type == 1)	basicMng.materialsManager.dxMaterial[materialID].mtlType == phong;
	if (type == 2)	basicMng.materialsManager.dxMaterial[materialID].mtlType == emissive;
	*/
}


bool LowLevelRendermanager::LoadUnityFromObjFile(wstring objName, wstring mtlName, wstring textureName, DxScene &scene,
	BasicManager &basicMng, ObjectType type)
{	/*
	//load mesh 
	if (!basicMng.objManager.LoadTriangelMeshObj(objName)) return false;
	scene.unityList[scene.endUnityId].objId = basicMng.objManager.endObjId - 1;
	for (int i = 0; i < basicMng.objManager.DxObjMem[scene.unityList[scene.endUnityId].objId]->vertexNum; i++)
	{
		basicMng.objManager.DxObjMem[scene.unityList[scene.endUnityId].objId]->vData[i].Pos.z = -basicMng.objManager.DxObjMem[scene.unityList[scene.endUnityId].objId]->vData[i].Pos.z;
		basicMng.objManager.DxObjMem[scene.unityList[scene.endUnityId].objId]->vData[i].Normal.z = -basicMng.objManager.DxObjMem[scene.unityList[scene.endUnityId].objId]->vData[i].Normal.z;
	}


	//load mmaterials
	int current1MtlId = basicMng.materialsManager.endMtlId;
	basicMng.materialsManager.LoadMtlFile(mtlName);
	scene.unityList[scene.endUnityId].materialNum = basicMng.materialsManager.endMtlId - current1MtlId;
	if (scene.unityList[scene.endUnityId].materialNum == 0)
	{	
		basicMng.materialsManager.endMtlId += 1;
		basicMng.materialsManager.mtlNumber += 1;
		scene.unityList[scene.endUnityId].materialNum = 1;
		scene.unityList[scene.endUnityId].MaterialsIdIndex = new int(1);
		scene.unityList[scene.endUnityId].MaterialsIdIndex[0] = current1MtlId;
		if (type == EmissiveType)
			basicMng.materialsManager.dxMaterial[scene.unityList[scene.endUnityId].MaterialsIdIndex[0]].mtlType = emissive;
	}
	else
	{
		scene.unityList[scene.endUnityId].MaterialsIdIndex = new int[scene.unityList[scene.endUnityId].materialNum];

		for (int i = 0; i < scene.unityList[scene.endUnityId].materialNum; i++) 
		{
			scene.unityList[scene.endUnityId].MaterialsIdIndex[i] = current1MtlId + i;

			if (type == EmissiveType)
				basicMng.materialsManager.dxMaterial[scene.unityList[scene.endUnityId].MaterialsIdIndex[i]].mtlType = emissive;

		}
	}


	//load texture, no set samplerstate!
	if (basicMng.textureManager.DxLoadTexture(textureName, basicMng.dxDevice))
	{
		scene.unityList[scene.endUnityId].textureId = basicMng.textureManager.endTextureId - 1;
		scene.unityList[scene.endUnityId].samplerStateId = basicMng.textureManager.endSamplerStateId;
		basicMng.textureManager.endSamplerStateId++;
	}
	else
	{
		scene.unityList[scene.endUnityId].textureId = -1;
		scene.unityList[scene.endUnityId].samplerStateId = -1;
	}

	//set type
	scene.unityList[scene.endUnityId].type = type;

	//name and id
	const wchar_t *wchar = objName.c_str();
	char * m_char = NULL;
	int len = WideCharToMultiByte(CP_ACP, 0, wchar, wcslen(wchar), NULL, 0, NULL, NULL);
	if (len == 0)
	{
		m_char = "Unity";
	}
	else
	{
		m_char = new char[len + 1];
		WideCharToMultiByte(CP_ACP, 0, wchar, wcslen(wchar), m_char, len, NULL, NULL);
	}
	scene.unityList[scene.endUnityId].name = m_char;
	scene.unityList[scene.endUnityId].UnityId = scene.endUnityId;
	scene.unityList[scene.endUnityId].empty = false;

	scene.currentUnityId = scene.endUnityId;
	scene.endUnityId++;
	*/
	return true;
}

bool LowLevelRendermanager::LoadUnityFromFBXFile(const char* fbxName, DxScene & scene, BasicManager & basicMng)
{
	//initialize manager
	FbxManager* pSdkManager = FbxManager::Create();
	FbxIOSettings* ios = FbxIOSettings::Create(pSdkManager, IOSROOT);
	pSdkManager->SetIOSettings(ios);

	FbxString lExtension = "dll";
	FbxString lPath = FbxGetApplicationDirectory();
	pSdkManager->LoadPluginsDirectory(lPath.Buffer(), lExtension.Buffer());

	FbxScene* pScene = FbxScene::Create(pSdkManager, "");

	//loadscene
	int lFileMajor, lFileMinor, lFileRevision;
	int lSDKMajor, lSDKMinor, lSDKRevision;
	int lAnimStackCount;
	bool lStatus;
	char lPassword[1024];
	FbxImporter* lImporter = NULL;
	bool lImportStatus = false;

	/*
	const wchar_t *wchar = fbxName.c_str();
	char * m_char = NULL;
	int len = WideCharToMultiByte(CP_ACP, 0, wchar, wcslen(wchar), NULL, 0, NULL, NULL);
	if (len == 0)
	{	
		return false;

	}
	else
	{
		m_char = new char[len + 1];
		WideCharToMultiByte(CP_ACP, 0, wchar, wcslen(wchar), m_char, len, NULL, NULL);
	}
	*/
	if (pSdkManager == NULL)
	{
		return false;
	}
	else
	{
		// Get the file version number generate by the FBX SDK.
		FbxManager::GetFileFormatVersion(lSDKMajor, lSDKMinor, lSDKRevision);
		lImporter = FbxImporter::Create(pSdkManager, "");
		lImportStatus = lImporter->Initialize(fbxName, -1, pSdkManager->GetIOSettings());
		lImporter->GetFileVersion(lFileMajor, lFileMinor, lFileRevision);
	}
	pScene->Clear();
	lStatus = lImporter->Import(pScene);
	lImporter->Destroy();

	//FbxSystemUnit::km. ConvertScene(pScene);
	FbxNode* pRootNode = pScene->GetRootNode();
	//read fbx mesh
	LoadFbxNode(pRootNode, NULL, -1, scene, basicMng);
	
	pScene->Destroy();
	return true;

}

void LowLevelRendermanager::LoadFbxNode(FbxNode* pNode, Unity* pParentUnity, int childInex, DxScene & scene, BasicManager & basicMng)
{

	Unity* currentUnity = NULL;
	int currentUnityID = NULL;
	bool NoEmptyUnity = true;

	//find empty unity, and fill into it
	if (scene.endUnityId > 0)
	{
		for (int i = 0; i < (scene.endUnityId - 0); i++)
		{

			if (scene.unityList[i].empty == true)
			{
				currentUnity = &scene.unityList[i];
				currentUnity->UnityId = i;
				currentUnity->empty = false;
				NoEmptyUnity = false;
				currentUnityID = i;
				break;
			}
		}
	}

	//create new unity
	if (NoEmptyUnity)
	{
		currentUnity = &scene.unityList[scene.endUnityId];
		currentUnity->UnityId = scene.endUnityId;
		currentUnityID = scene.endUnityId;
		currentUnity->empty = false;
		scene.endUnityId++;
	}

	//find parent unity
	if (pParentUnity != NULL && childInex >= 0)
	{
		currentUnity->parent = pParentUnity;
		pParentUnity->childs[childInex] = &scene.unityList[currentUnityID];

	}

	//get name info
	const char* name = pNode->GetName();
	int charLen = strlen(name);
	if (charLen > 0)
	{
		currentUnity->name = new char[charLen];
		strcpy_s(currentUnity->name, charLen+1,name);
		
	}
	else
	{
		currentUnity->name = "unity";
	}

	//get transform

	if (pNode->GetNodeAttribute())
	{
		if (&currentUnity->transform == NULL || &currentUnity->transform == nullptr)
		{
			currentUnity->transform = Transform();


		}

		if (&currentUnity->wolrdTransform == NULL || &currentUnity->wolrdTransform == nullptr)
		{
			currentUnity->wolrdTransform = Transform();


		}

		//geometry transform
		FbxVector4 translate1 = pNode->GetGeometricTranslation(FbxNode::eSourcePivot);
		FbxVector4 rotate1 = pNode->GetGeometricRotation(FbxNode::eSourcePivot);
		FbxVector4 scale1 = pNode->GetGeometricScaling(FbxNode::eSourcePivot);

		FbxMatrix geoM;
		geoM.SetIdentity();
		geoM.SetTRS(translate1, rotate1, scale1);
		
		//global transform
		FbxMatrix globalM;
		globalM.SetIdentity();
		globalM = pNode->EvaluateGlobalTransform();
		globalM *= geoM;
		currentUnity->wolrdTransform = currentUnity->transform = Transform(Matrix4x4(globalM.Get(0, 0), globalM.Get(0, 1), globalM.Get(0, 2), globalM.Get(0, 3),
			globalM.Get(1, 0), globalM.Get(1, 1), globalM.Get(1, 2), globalM.Get(1, 3),
			globalM.Get(2, 0), globalM.Get(2, 1), globalM.Get(2, 2), globalM.Get(2, 3),
			globalM.Get(3, 0), globalM.Get(3, 1), globalM.Get(3, 2), globalM.Get(3, 3)));
		currentUnity->Pos = Point(globalM.Get(3, 0), globalM.Get(3, 1), globalM.Get(3, 2));

		//local transform
		FbxMatrix localM;
		localM.SetIdentity();
		if (pNode->EvaluateLocalTransform())
		{
			localM = pNode->EvaluateLocalTransform();
		}
		localM *= geoM;
		currentUnity->transform = Transform(Matrix4x4(localM.Get(0,0), localM.Get(0,1), localM.Get(0,2), localM.Get(0,3),
			localM.Get(1, 0), localM.Get(1, 1), localM.Get(1, 2), localM.Get(1, 3),
			localM.Get(2, 0), localM.Get(2, 1), localM.Get(2, 2), localM.Get(2, 3),
			localM.Get(3, 0), localM.Get(3, 1), localM.Get(3, 2), localM.Get(3, 3)));

	}
	else
	{
		if (&currentUnity->transform == NULL || &currentUnity->transform == nullptr)
		{
			currentUnity->transform = Transform();


		}

		if (&currentUnity->wolrdTransform == NULL || &currentUnity->wolrdTransform == nullptr)
		{
			currentUnity->wolrdTransform = Transform();


		}
	}
		
	//get mesh info
	if (pNode->GetNodeAttribute())
	{
		FbxNodeAttribute::EType type = pNode->GetNodeAttribute()->GetAttributeType();

		switch (type)
		{
		case fbxsdk::FbxNodeAttribute::eUnknown:
			break;
		case fbxsdk::FbxNodeAttribute::eNull:
			break;
		case fbxsdk::FbxNodeAttribute::eMarker:
			break;
		case fbxsdk::FbxNodeAttribute::eSkeleton:
			break;
		case fbxsdk::FbxNodeAttribute::eMesh:
			LoadFBXMesh(pNode, scene, basicMng, &scene.unityList[currentUnityID]);
			break;
		case fbxsdk::FbxNodeAttribute::eNurbs:
			break;
		case fbxsdk::FbxNodeAttribute::ePatch:
			break;
		case fbxsdk::FbxNodeAttribute::eCamera:
			break;
		case fbxsdk::FbxNodeAttribute::eCameraStereo:
			break;
		case fbxsdk::FbxNodeAttribute::eCameraSwitcher:
			break;
		case fbxsdk::FbxNodeAttribute::eLight:
			break;
		case fbxsdk::FbxNodeAttribute::eOpticalReference:
			break;
		case fbxsdk::FbxNodeAttribute::eOpticalMarker:
			break;
		case fbxsdk::FbxNodeAttribute::eNurbsCurve:
			break;
		case fbxsdk::FbxNodeAttribute::eTrimNurbsSurface:
			break;
		case fbxsdk::FbxNodeAttribute::eBoundary:
			break;
		case fbxsdk::FbxNodeAttribute::eNurbsSurface:
			break;
		case fbxsdk::FbxNodeAttribute::eShape:
			break;
		case fbxsdk::FbxNodeAttribute::eLODGroup:
			break;
		case fbxsdk::FbxNodeAttribute::eSubDiv:
			break;
		case fbxsdk::FbxNodeAttribute::eCachedEffect:
			break;
		case fbxsdk::FbxNodeAttribute::eLine:
			break;
		default:
			break;
		}
	}

	// get child unity
	if (pNode->GetChildCount() > 0)
	{	
		currentUnity->childCount = pNode->GetChildCount();
		currentUnity->childs = new Unity*[currentUnity->childCount];
		for (int i = 0; i < pNode->GetChildCount(); i++)
		{	
			
			LoadFbxNode(pNode->GetChild(i), &scene.unityList[currentUnityID], i, scene, basicMng);
		}
	}
	return;

}

void LowLevelRendermanager::LoadFBXMesh(FbxNode *pNode, DxScene &scene, BasicManager &basicMng, Unity *unity)
{	
	//get mesh
	FbxMesh *pMesh = NULL;
	pMesh = pNode->GetMesh();
	if (pMesh == NULL)
	{
		return;
	}

	int polygonCount = pMesh->GetPolygonCount();
	if (polygonCount > 0)
	{
		FbxVector4* pControlPoint = pMesh->GetControlPoints();
		vector<FbxVector4> vertices;
		vector<FbxVector4> normals;
		vector<FbxVector2> uvs;
		vector<int> index;
		vector<FbxVector4> tangents;
		vector<FbxColor> colors;

		//load vertex
		for (int i = 0; i < polygonCount; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				//load vertex position
				int controlPoinIndex = pMesh->GetPolygonVertex(i, j);
				vertices.push_back(pControlPoint[controlPoinIndex]);
				index.push_back(i * 3 + j);
			}
		}

		//load vertex normal
		FbxGeometryElementNormal* pFbxNormals = pMesh->GetElementNormal(0);
		if (pFbxNormals)
		{
			switch (pFbxNormals->GetMappingMode())
			{
				case FbxGeometryElement::eByControlPoint:
				{
					switch (pFbxNormals->GetReferenceMode())
					{
						case FbxGeometryElement::eDirect:
						{	
							for (int i = 0; i < polygonCount; i++)
							{
								for (int j = 0; j < 3; j++)
								{
									int controlPoinIndex = pMesh->GetPolygonVertex(i, j);
									normals.push_back(pFbxNormals->GetDirectArray().GetAt(controlPoinIndex));
								}
							}
						}		
						break;

						case FbxGeometryElement::eIndexToDirect:
						{	
							for (int i = 0; i < polygonCount; i++)
							{
								for (int j = 0; j < 3; j++)
								{
									int controlPoinIndex = pMesh->GetPolygonVertex(i, j);
									normals.push_back(pFbxNormals->GetDirectArray().GetAt(pFbxNormals->GetIndexArray().GetAt(controlPoinIndex)));
								}
							}
						}
						break;

						default:
							break;
					}
				}
				break;

				case FbxGeometryElement::eByPolygonVertex:
				{
					switch (pFbxNormals->GetReferenceMode())
					{
						case FbxGeometryElement::eDirect:
						{
							for (int i = 0; i < polygonCount; i++)
							{
								for (int j = 0; j < 3; j++)
								{
									normals.push_back(pFbxNormals->GetDirectArray().GetAt(i * 3 + j));
								}
							}
						}
						break;

						case FbxGeometryElement::eIndexToDirect:
						{
							for (int i = 0; i < polygonCount; i++)
							{
								for (int j = 0; j < 3; j++)
								{
									normals.push_back(pFbxNormals->GetDirectArray().GetAt(pFbxNormals->GetIndexArray().GetAt(i * 3 + j)));
								}
							}
						}
						break;

						default:
							break;
					}
				}
				break;

			}

		}

		//load tangent
		FbxGeometryElementTangent* pFbxTangents = pMesh->GetElementTangent(0);
		if (!pFbxTangents)
		{
			if (pMesh->GenerateTangentsData(0))
			{	
				pFbxTangents = pMesh->GetElementTangent(0);
			}
		}
		if (pFbxTangents)
		{
			switch (pFbxTangents->GetMappingMode())
			{
			case FbxGeometryElement::eByControlPoint:
			{
				switch (pFbxTangents->GetReferenceMode())
				{
				case FbxGeometryElement::eDirect:
				{
					for (int i = 0; i < polygonCount; i++)
					{
						for (int j = 0; j < 3; j++)
						{
							int controlPoinIndex = pMesh->GetPolygonVertex(i, j);
							tangents.push_back(pFbxTangents->GetDirectArray().GetAt(controlPoinIndex));
						}
					}
				}
				break;

				case FbxGeometryElement::eIndexToDirect:
				{
					for (int i = 0; i < polygonCount; i++)
					{
						for (int j = 0; j < 3; j++)
						{
							int controlPoinIndex = pMesh->GetPolygonVertex(i, j);
							tangents.push_back(pFbxTangents->GetDirectArray().GetAt(pFbxTangents->GetIndexArray().GetAt(controlPoinIndex)));
						}
					}
				}
				break;

				default:
					break;
				}
			}
			break;

			case FbxGeometryElement::eByPolygonVertex:
			{
				switch (pFbxTangents->GetReferenceMode())
				{
				case FbxGeometryElement::eDirect:
				{
					for (int i = 0; i < polygonCount; i++)
					{
						for (int j = 0; j < 3; j++)
						{
							tangents.push_back(pFbxTangents->GetDirectArray().GetAt(i * 3 + j));
						}
					}
				}
				break;

				case FbxGeometryElement::eIndexToDirect:
				{
					for (int i = 0; i < polygonCount; i++)
					{
						for (int j = 0; j < 3; j++)
						{
							tangents.push_back(pFbxTangents->GetDirectArray().GetAt(pFbxTangents->GetIndexArray().GetAt(i * 3 + j)));
						}
					}
				}
				break;

				default:
					break;
				}
			}
			break;

			}

		}

		//load vertx color
		FbxGeometryElementVertexColor* pFbxColors = pMesh->GetElementVertexColor(0);
		if (pFbxColors)
		{
			switch (pFbxColors->GetMappingMode())
			{
			case FbxGeometryElement::eByControlPoint:
			{
				switch (pFbxColors->GetReferenceMode())
				{
				case FbxGeometryElement::eDirect:
				{
					for (int i = 0; i < polygonCount; i++)
					{
						for (int j = 0; j < 3; j++)
						{
							int controlPoinIndex = pMesh->GetPolygonVertex(i, j);
							colors.push_back(pFbxColors->GetDirectArray().GetAt(controlPoinIndex));
						}
					}
				}
				break;

				case FbxGeometryElement::eIndexToDirect:
				{
					for (int i = 0; i < polygonCount; i++)
					{
						for (int j = 0; j < 3; j++)
						{
							int controlPoinIndex = pMesh->GetPolygonVertex(i, j);
							colors.push_back(pFbxColors->GetDirectArray().GetAt(pFbxColors->GetIndexArray().GetAt(controlPoinIndex)));
						}
					}
				}
				break;

				default:
					break;
				}
			}
			break;

			case FbxGeometryElement::eByPolygonVertex:
			{
				switch (pFbxColors->GetReferenceMode())
				{
				case FbxGeometryElement::eDirect:
				{
					for (int i = 0; i < polygonCount; i++)
					{
						for (int j = 0; j < 3; j++)
						{
							colors.push_back(pFbxColors->GetDirectArray().GetAt(i * 3 + j));
						}
					}
				}
				break;

				case FbxGeometryElement::eIndexToDirect:
				{
					for (int i = 0; i < polygonCount; i++)
					{
						for (int j = 0; j < 3; j++)
						{
							colors.push_back(pFbxColors->GetDirectArray().GetAt(pFbxColors->GetIndexArray().GetAt(i * 3 + j)));
						}
					}
				}
				break;

				default:
					break;
				}
			}
			break;

			}
			 
		}

		//load uv0
		FbxGeometryElementUV* pFbxUVs = pMesh->GetElementUV(0);
		if (pFbxUVs)
		{
			switch (pFbxUVs->GetMappingMode())
			{
				case FbxGeometryElement::eByControlPoint:
				{
					switch (pFbxUVs->GetReferenceMode())
					{
					case FbxGeometryElement::eDirect:
					{
						for (int i = 0; i < polygonCount; i++)
						{
							for (int j = 0; j < 3; j++)
							{
								int controlPoinIndex = pMesh->GetPolygonVertex(i, j);
								uvs.push_back(pFbxUVs->GetDirectArray().GetAt(controlPoinIndex));
							}
						}
					}
					break;

					case FbxGeometryElement::eIndexToDirect:
					{
						for (int i = 0; i < polygonCount; i++)
						{
							for (int j = 0; j < 3; j++)
							{
								int controlPoinIndex = pMesh->GetPolygonVertex(i, j);
								uvs.push_back(pFbxUVs->GetDirectArray().GetAt(pFbxUVs->GetIndexArray().GetAt(controlPoinIndex)));
							}
						}
					}
					break;

					default:
						break;
					}
				}
				break;

				case FbxGeometryElement::eByPolygonVertex:
				{
					switch (pFbxUVs->GetReferenceMode())
					{
					case FbxGeometryElement::eDirect:
					{
						for (int i = 0; i < polygonCount; i++)
						{
							for (int j = 0; j < 3; j++)
							{
								uvs.push_back(pFbxUVs->GetDirectArray().GetAt(i * 3 + j));
							}
						}
					}
					break;

					case FbxGeometryElement::eIndexToDirect:
					{
						for (int i = 0; i < polygonCount; i++)
						{
							for (int j = 0; j < 3; j++)
							{
								uvs.push_back(pFbxUVs->GetDirectArray().GetAt(pFbxUVs->GetIndexArray().GetAt(i * 3 + j)));
							}
						}
					}
					break;

					default:
						break;
					}
				}
				break;

			}

		}
		
		//create DxObj
		ObjManager& objMng = basicMng.objManager;
		int currentObjId = NULL;
		DxObj* currentObj = NULL;
		bool NoEmptyUnity = true;

		if (objMng.endObjId > 0)
		{
			for (int i = 0; i < (objMng.endObjId - 0); i++)
			{

				if (objMng.DxObjMem[i]->empty == true)
				{	
					currentObj = objMng.DxObjMem[i];
					currentObj->empty = false;
					currentObj->objId = i;
					NoEmptyUnity = false;
					currentObjId = i;
					unity->objId = currentObjId;
					objMng.ObjNumber++;
					break;
				}
			}
		}

		if (NoEmptyUnity)
		{
			objMng.DxObjMem[objMng.endObjId] = new DxTriangleMesh();
			currentObj = objMng.DxObjMem[objMng.endObjId];
			currentObj->objId = objMng.endObjId;
			currentObjId = objMng.endObjId;
			unity->objId = currentObjId;
			currentObj->empty = false;
			objMng.endObjId++;
			objMng.ObjNumber++;
		}

		//create mesh info in the DxObj
		currentObj->vertexNum = vertices.size();
		currentObj->faceNum = pMesh->GetPolygonCount();
		currentObj->vData = new vertexData[vertices.size()];
		currentObj->indices = new WORD[vertices.size()];

		for (size_t i = 0; i < vertices.size(); i++)
		{
			currentObj->vData[i].Pos = XMFLOAT3(vertices[i].mData[0],
												vertices[i].mData[1], 
												vertices[i].mData[2]);
			if (normals.size() == vertices.size())
			{
				currentObj->vData[i].Normal = XMFLOAT3(normals[i].mData[0],
														normals[i].mData[1],
														normals[i].mData[2]);
			}
			else
			{
				currentObj->vData[i].Normal = XMFLOAT3(0.0, 0.0, 0.0);
			}

			if (tangents.size() == vertices.size())
			{
				currentObj->vData[i].Tangent = XMFLOAT3(tangents[i].mData[0],
					tangents[i].mData[1],
					tangents[i].mData[2]);
			}
			else
			{
				currentObj->vData[i].Tangent = XMFLOAT3(0.0, 0.0, 0.0);
			}

			if (colors.size() == vertices.size())
			{
				currentObj->vData[i].Color = XMFLOAT4(colors[i].mRed,
					colors[i].mGreen,
					colors[i].mBlue,
					colors[i].mAlpha);
			}
			else
			{
				currentObj->vData[i].Color = XMFLOAT4(1.0, 1.0, 1.0, 1.0);
			}

			if (uvs.size() == vertices.size())
			{
				currentObj->vData[i].Tex = XMFLOAT2(uvs[i].mData[0],
													uvs[i].mData[1]);
			}
			else
			{
				currentObj->vData[i].Tex = XMFLOAT2(0.0, 0.0);
			}



			currentObj->indices[i] = index[i];
		}


		//create material index for mesh
		vector<int> matIndices;
		FbxLayerElementMaterial* pFbxMat = pMesh->GetElementMaterial();

		if (pFbxMat)
		{
			if (&pFbxMat->GetIndexArray())
			{	
				//load material indices
				switch (pFbxMat->GetMappingMode())
				{
				case FbxGeometryElement::eByPolygon:
				{
					if (pFbxMat->GetIndexArray().GetCount() == polygonCount)
					{
						for (int i = 0; i < polygonCount; i++)
						{
							int matIndex = pFbxMat->GetIndexArray().GetAt(i);
							matIndices.push_back(matIndex);
						}
					}
				}
				break;

				case FbxGeometryElement::eAllSame:
				{

					for (int i = 0; i < polygonCount; i++)
					{
						int matIndex = pFbxMat->GetIndexArray().GetAt(i);
						matIndices.push_back(matIndex);
					}

				}
				break;

				default:
					break;
				}

				//save material indices
				currentObj->materialNum = pMesh->GetElementMaterialCount();
				currentObj->faceMaterialIndices = new int[polygonCount];

				for (int i = 0; i < polygonCount; i++)
				{
					currentObj->faceMaterialIndices[i] = matIndices[i];
				}
	
				//optimize material and mesh
				if (currentObj->materialNum > 1)
				{
					vector<int> opIndices;
					vector<int> opMatIndices;
					vector<int> eachMatFaceCount;

					for (int matIndex =0; matIndex < currentObj->materialNum; matIndex++)
					{	
						int faceCount = 0;
						for (int i = 0; i < polygonCount; i++)
						{
							if (currentObj->faceMaterialIndices[i] == matIndex)
							{	
								//optimize face-material index
								opMatIndices.push_back(currentObj->faceMaterialIndices[i]);
								faceCount++;

								//optimize vertex index
								for (int j = 0; j < 3; j++)
								{
									opIndices.push_back(currentObj->indices[3 * i + j]);
								}
							}
						}
						eachMatFaceCount.push_back(faceCount);

					}

					for (int i = 0; i < polygonCount*3; i++)
					{
						currentObj->indices[i] = opIndices[i];
					}

					for (int i = 0; i < polygonCount; i++)
					{
						currentObj->faceMaterialIndices[i] = opMatIndices[i];
					}

					//save face begin index
					currentObj->FaceNumInEachMtl = new int[currentObj->materialNum];
					currentObj->beginFaceInEachMtl = new int[currentObj->materialNum];
					for (int i = 0; i < currentObj->materialNum; i++)
					{
						currentObj->FaceNumInEachMtl[i] = eachMatFaceCount[i];
						for (int j = 0; j < polygonCount; j++)
						{
							if (currentObj->faceMaterialIndices[j] == i)
							{
								currentObj->beginFaceInEachMtl[i] = j;
								break;
							}
						}
					}
					
					opIndices.clear();
					opIndices.shrink_to_fit();
					opMatIndices.clear();
					opMatIndices.shrink_to_fit();
					eachMatFaceCount.clear();
					eachMatFaceCount.shrink_to_fit();
				}
				else if(currentObj->materialNum  == 1)
				{
					currentObj->beginFaceInEachMtl = new int[1];
					currentObj->beginFaceInEachMtl[0] = 0;
					currentObj->FaceNumInEachMtl = new int[1];
					currentObj->FaceNumInEachMtl[0] = polygonCount;
				}
				

				unity->materialNum = currentObj->materialNum;
				unity->MaterialsIdIndex = new int[unity->materialNum];
				MaterialsManager& matMng = basicMng.materialsManager;
				bool NoEmptyMat = true;

				for (int i = 0; i < unity->materialNum; i++)
				{
					//create material in material manager:
					if (matMng.endMtlId > 0)
					{
						for (int j = 0; j < (matMng.endMtlId - 0); j++)
						{

							if (matMng.dxMaterial[j].empty == true)
							{
								matMng.dxMaterial[j].empty = false;
								NoEmptyMat = false;
								unity->MaterialsIdIndex[i] = j;
								matMng.mtlNumber++;
								break;
							}
						}
					}

					if (NoEmptyMat)
					{
						matMng.dxMaterial[matMng.endMtlId].empty = false;
						unity->MaterialsIdIndex[i] = matMng.endMtlId;
						matMng.endMtlId++;
						matMng.mtlNumber++;
					}
				}		
			}
		}
		else
		{
			if (currentObj->vertexNum > 0)
			{
				currentObj->materialNum = 1;
				currentObj->beginFaceInEachMtl = new int[1];
				currentObj->beginFaceInEachMtl[0] = 0;
				currentObj->FaceNumInEachMtl = new int[1];
				currentObj->FaceNumInEachMtl[0] = polygonCount;


				unity->materialNum = currentObj->materialNum;
				unity->MaterialsIdIndex = new int[unity->materialNum];
				MaterialsManager& matMng = basicMng.materialsManager;
				bool NoEmptyMat = true;

				for (int i = 0; i < unity->materialNum; i++)
				{
					//create material in material manager:
					if (matMng.endMtlId > 0)
					{
						for (int j = 0; j < (matMng.endMtlId - 0); j++)
						{

							if (matMng.dxMaterial[j].empty == true)
							{
								matMng.dxMaterial[j].empty = false;
								NoEmptyMat = false;
								unity->MaterialsIdIndex[i] = j;
								matMng.mtlNumber++;
								break;
							}
						}
					}

					if (NoEmptyMat)
					{
						matMng.dxMaterial[matMng.endMtlId].empty = false;
						unity->MaterialsIdIndex[i] = matMng.endMtlId;
						matMng.endMtlId++;
						matMng.mtlNumber++;
					}
				}
			}
		}

		//set texture defeat
		for (int matCount = 0; matCount < unity->materialNum; matCount++)
		{
			DxMaterials &mat =  basicMng.materialsManager.dxMaterial[unity->MaterialsIdIndex[matCount]];
			mat.albedoID = 0;
			mat.albedoSRV = &basicMng.textureManager.texViewPointer[0];
			mat.albedoSampleState = &basicMng.textureManager.sampleStatePointer[0];
			mat.normalID = 1;
			mat.normalSRV = &basicMng.textureManager.texViewPointer[1];
			mat.normalSampleState = &basicMng.textureManager.sampleStatePointer[1];
			mat.mraID = 0;
			mat.mraSRV = &basicMng.textureManager.texViewPointer[0];
			mat.mraSampleState = &basicMng.textureManager.sampleStatePointer[0];
		}

		vertices.clear();
		vertices.shrink_to_fit();
		normals.clear();
		normals.shrink_to_fit();
		tangents.clear();
		tangents.shrink_to_fit();
		colors.clear();
		colors.shrink_to_fit();
		uvs.clear();
		uvs.shrink_to_fit();
		index.clear();
		index.shrink_to_fit();
		matIndices.clear();
		matIndices.shrink_to_fit();

	}
	else
	{
		return;
	}

}


