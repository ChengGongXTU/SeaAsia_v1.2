#include "primitivemanager.h"


void PrimitiveManager::StartUp()
{
	objId = -1;
	layout[0] = { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,   0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
	layout[1] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,   16, D3D11_INPUT_PER_VERTEX_DATA, 0 };
	layout[2] = { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  28, D3D11_INPUT_PER_VERTEX_DATA, 0 };
	layout[3] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,  40, D3D11_INPUT_PER_VERTEX_DATA, 0 };
	layout[4] = { "TANGENT",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  48, D3D11_INPUT_PER_VERTEX_DATA, 0 };

	numElements = ARRAYSIZE(layout);
	vertexlayout = NULL;

	ZeroMemory(&bd, sizeof(bd));
	ZeroMemory(&data, sizeof(data));
	vertexBuffer = NULL;
	indexBuffer = NULL;
	vertexlayout = NULL;

	topologyType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;


}

void PrimitiveManager::ShutUp()
{	
	objId = -1;
	delete layout;
	numElements = 0;
	vertexlayout = NULL;

	ZeroMemory(&bd, sizeof(bd));
	ZeroMemory(&data, sizeof(data));
	vertexBuffer = NULL;
	indexBuffer = NULL;

	topologyType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

bool PrimitiveManager::LoadUnity(DxDevice &dev,ObjManager &objMng,Unity & unity)
{	
	if (vertexBuffer != NULL) vertexBuffer->Release();
	if (indexBuffer != NULL) indexBuffer->Release();

	if (objId == -1) {

		ZeroMemory(&bd, sizeof(bd));
		ZeroMemory(&data, sizeof(data));

		objId = unity.objId;
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(vertexData)*(objMng.DxObjMem[unity.objId]->vertexNum);
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;
		data.pSysMem = objMng.DxObjMem[unity.objId]->vData;
		HRESULT hr = dev.device->CreateBuffer(&bd, &data, &vertexBuffer);

		if (FAILED(hr))	return false;

		bd.ByteWidth = sizeof(WORD)*(objMng.DxObjMem[unity.objId]->faceNum) * 3;
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		data.pSysMem = objMng.DxObjMem[unity.objId]->indices;
		hr = dev.device->CreateBuffer(&bd, &data, &indexBuffer);

		if (FAILED(hr))	return false;

		return true;

	}

	if (objId == unity.objId) {
		return true;
	}

	if (objId != unity.objId && objId != -1) {
		
		ZeroMemory(&bd, sizeof(bd));
		ZeroMemory(&data, sizeof(data));

		objId = unity.objId;

		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(vertexData)*(objMng.DxObjMem[unity.objId]->vertexNum);
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;
		data.pSysMem = objMng.DxObjMem[unity.objId]->vData;
		HRESULT hr = dev.device->CreateBuffer(&bd, &data, &vertexBuffer);

		if (FAILED(hr))	return false;

		bd.ByteWidth = sizeof(WORD)*(objMng.DxObjMem[unity.objId]->faceNum) * 3;
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		data.pSysMem = objMng.DxObjMem[unity.objId]->indices;
		hr = dev.device->CreateBuffer(&bd, &data, &indexBuffer);

		if (FAILED(hr))	return false;

		return true;
	}
	
	return false;
	
}

bool PrimitiveManager::InputVertexBuffer(DxDevice & dev,Unity &unity, ShaderManager &shaderMng)
{	
	if (vertexlayout != NULL)	vertexlayout->Release();
	HRESULT hr = dev.device->CreateInputLayout(layout, numElements, shaderMng.vsBlob[shaderMng.endVSId-1]->GetBufferPointer(),
		shaderMng.vsBlob[shaderMng.endVSId-1]->GetBufferSize(), &vertexlayout);

	if (FAILED(hr)) return false;

	dev.context->IASetInputLayout(vertexlayout);

	UINT stride = sizeof(vertexData);
	UINT offset = 0;
	dev.context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
	dev.context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R16_UINT, 0);
	dev.context->IASetPrimitiveTopology(topologyType);
	
	return true;
}

bool PrimitiveManager::InputVertexBufferGeometryShading(DxDevice & dev, Unity &unity, ShaderManager &shaderMng)
{
	if (vertexlayout != NULL)	vertexlayout->Release();
	HRESULT hr = dev.device->CreateInputLayout(layout, numElements, shaderMng.vsBlob[0]->GetBufferPointer(),
		shaderMng.vsBlob[0]->GetBufferSize(), &vertexlayout);

	if (FAILED(hr)) return false;

	dev.context->IASetInputLayout(vertexlayout);

	UINT stride = sizeof(vertexData);
	UINT offset = 0;
	dev.context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
	dev.context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R16_UINT, 0);
	dev.context->IASetPrimitiveTopology(topologyType);

	return true;
}

bool PrimitiveManager::InputVertexBufferLightShading(DxDevice & dev, Unity &unity, ShaderManager &shaderMng)
{
	if (vertexlayout != NULL)	vertexlayout->Release();
	HRESULT hr = dev.device->CreateInputLayout(layout, numElements, shaderMng.vsBlob[1]->GetBufferPointer(),
		shaderMng.vsBlob[1]->GetBufferSize(), &vertexlayout);

	if (FAILED(hr)) return false;

	dev.context->IASetInputLayout(vertexlayout);

	UINT stride = sizeof(vertexData);
	UINT offset = 0;
	dev.context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
	dev.context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R16_UINT, 0);
	dev.context->IASetPrimitiveTopology(topologyType);

	return true;
}