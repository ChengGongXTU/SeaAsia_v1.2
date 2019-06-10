#pragma once
#include"SeeAisa.h"
#include"dxobj.h"
#include"dxcamera.h"
#include"dxlight.h"

enum ObjectType
{
	TriangleType,SphereType,RectangleType,EmissiveType
};

class Unity {

public:

	ObjectType type;

	Point Pos;
	Matrix4x4 transform;

	int objId;

	int textureId;
	int samplerStateId;

	int materialNum;
	int* MaterialsIdIndex;

	Unity() {
		Pos = Point(0.f, 0.f, 0.f);
		transform = XMMatrixIdentity();

		objId = 0;
		textureId = 0;
		samplerStateId = 0;

		materialNum = 0;
	}
	
};

class DxScene {
public:

	Unity* unityList;

	int endUnityId;
	int currentUnityId;
	int maxUnityNum;

	DxCamera* cameraList;

	int endCameraId;
	int currentCameraId;
	int cameraNum;
	int maxCameraNum;

	DxPointLight* plList;
	int endPlId;
	int currentPlId;
	int maxPlNum;

	DxDirLight* dlList;
	int endDlId;
	int currentDlId;
	int maxDlNum;


	DxScene() {
		unityList = NULL; endUnityId = 0; currentUnityId = -1; maxUnityNum = 0;
		cameraList = NULL; endCameraId = 0; currentCameraId = -1; maxCameraNum = 0; cameraNum = 0;
		plList = NULL; endPlId = 0; currentPlId = -1; maxPlNum = 0;
		dlList = NULL; endDlId = 0; currentDlId = -1; maxDlNum = 0;
	}

	void setUp(int uniNum, int camNum, int plNum, int dlNum);

};
