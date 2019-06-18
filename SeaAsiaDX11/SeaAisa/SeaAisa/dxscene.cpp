#include"dxscene.h"

void DxScene::setUp(int uniNum, int camNum, int plNum, int dlNum) {

	endUnityId = 0;
	currentUnityId = -1;
	maxUnityNum = uniNum;

	endCameraId = 0;
	currentCameraId = -1;
	cameraNum = 0;
	maxCameraNum = camNum;

	endPlId = 0;
	currentPlId = -1;
	maxPlNum = plNum;

	endDlId = 0;
	currentDlId = -1;
	maxDlNum = dlNum;

	unityList = new Unity[uniNum];
	for (int i = 0; i < uniNum; i++)
	{
		unityList[i].objId = -1;
		unityList[i].materialNum = 0;
		unityList[i].samplerStateId = -1;
		unityList[i].textureId = -1;
		unityList[i].MaterialsIdIndex = NULL;
		unityList[i].Pos = Point(0.f, 0.f, 0.f);
		unityList[i].transform = Transform();
		unityList[i].wolrdTransform = Transform();
		unityList[i].UnityId = -1;
		unityList[i].parent = NULL;
		unityList[i].childs = NULL;
		unityList[i].childCount = 0;
		unityList[i].name = NULL;
	}
	cameraList = new DxCamera[camNum];
	for (int i = 0; i < maxCameraNum; i++)
	{
		cameraList[i].eye = Point(0, 0, 0);
		cameraList[i].at = Point(0, 0, 0);
		cameraList[i].up = Vector(0, 1, 0);
		cameraList[i].aspect = 0.f;
		cameraList[i].cNear = 0.f;
		cameraList[i].cFar = 0.f;
		cameraList[i].cWidth = 1.f;
		cameraList[i].cHeight = 1.f;
		cameraList[i].mWorld = MatrixIdentity().m;
		cameraList[i].mView = MatrixIdentity().m;
		cameraList[i].mProjection = MatrixIdentity().m;

	}

	plList = new DxPointLight[plNum];
	for (int i = 0; i < maxPlNum; i++)
	{
		plList[i].Pos = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
		plList[i].Color = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
	}
	dlList = new DxDirLight[dlNum];
	for (int i = 0; i < maxPlNum; i++)
	{
		dlList[i].Dir = XMFLOAT4(0.f, 0.f, 0.f, 0.f);
		dlList[i].Color = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
	}
}



//bool unity::LoadObjFile(wstring obj, wstring mtl)
//{
//	
//}

