//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#include "pch.h"
#include <random>
#include "Sample3DSceneRenderer.h"

#include "..\Helpers\DirectXHelper.h"

using namespace DirectXGame2;

using namespace DirectX;
using namespace Windows::Foundation;

// Loads vertex and pixel shaders from files and instantiates the cube geometry.
Sample3DSceneRenderer::Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
m_loadingComplete(false),
m_contextReady(false),
m_degreesPerSecond(45),
m_indexCount(0),
m_tracking(false),
m_deviceResources(deviceResources)
{
	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
	CreateAsteroidField();
	CreateCamera();
}

void Sample3DSceneRenderer::CreateCamera()
{
	cam.pos = XMVectorSet(0, 0, 0, 0);
	cam.ori = XMQuaternionRotationRollPitchYaw(0, 0, 0);
	laser.ori = XMQuaternionRotationRollPitchYaw(0, 0, 0);
}
void Sample3DSceneRenderer::CameraMove(float ahead)
{
	// move "ahead" amount in forward direction
	XMVECTOR aheadv = XMVectorScale(cam.forward, ahead);
	cam.pos = XMVectorAdd(cam.pos, aheadv);
}

void Sample3DSceneRenderer::CameraSpin(float roll, float pitch, float yaw)
{
	// make sure camera properties are up to date
	// NB: actually unnecessary, since they are updated every frame anyway; done here for clarity
	cam.forward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	cam.up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	cam.left = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

	cam.forward = XMVector4Transform(cam.forward, XMMatrixRotationQuaternion(cam.ori));
	cam.up = XMVector4Transform(cam.up, XMMatrixRotationQuaternion(cam.ori));
	cam.left = XMVector3Cross(cam.forward, cam.up);
	// apply camera-relative orientation changes

	XMVECTOR rollq = XMQuaternionRotationAxis(cam.forward, roll*0.05);
	XMVECTOR pitchq = XMQuaternionRotationAxis(cam.left, pitch*0.05);
	XMVECTOR yawq = XMQuaternionRotationAxis(cam.up, yaw*0.05);
	//Roll:
	cam.ori = XMQuaternionMultiply(cam.ori,rollq);
	//Pitch:
	cam.ori = XMQuaternionMultiply(cam.ori,pitchq);
	//Yaw:
	cam.ori = XMQuaternionMultiply(cam.ori,yawq);
}

void Sample3DSceneRenderer::LaserSpin(float laserPitch, float laserYaw)
{
	//note that our perspective's pitch's rotation axis is X, so it's equivalent to rolling 
	//similarly our perspective's yaw's rotation axis is Y, so it's equivalent to pitching 
	//As for the axes of roll pitch yaw, see the following link
	//http://theboredengineers.com/2012/05/the-quadcopter-basics/

	XMVECTOR pitchq = XMQuaternionRotationRollPitchYaw(laserPitch*0.01, 0, 0);
	XMVECTOR yawq = XMQuaternionRotationRollPitchYaw(0, laserYaw*0.01, 0);

	laser.ori = XMQuaternionMultiply(laser.ori, pitchq);
	laser.ori = XMQuaternionMultiply(laser.ori, yawq);
}

void Sample3DSceneRenderer::LaserFire(bool isFiring)
{
	laser.isFiring = isFiring;
}

void Sample3DSceneRenderer::LaserFireType(int type)
{
	laser.type = type;
}

// Initializes view parameters when the window size changes.
void Sample3DSceneRenderer::CreateWindowSizeDependentResources()
{
    Size outputSize = m_deviceResources->GetOutputSize();
    float aspectRatio = outputSize.Width / outputSize.Height;
    float fovAngleY = 70.0f * XM_PI / 180.0f;

    // This is a simple example of change that can be made when the app is in
    // portrait or snapped view.
    if (aspectRatio < 1.0f)
    {
        fovAngleY *= 1.0f;
    }

    // Note that the OrientationTransform3D matrix is post-multiplied here
    // in order to correctly orient the scene to match the display orientation.
    // This post-multiplication step is required for any draw calls that are
    // made to the swap chain render target. For draw calls to other targets,
    // this transform should not be applied.

    // This sample makes use of a right-handed coordinate system using row-major matrices.
//	XMMATRIX perspectiveMatrix = XMMatrixPerspectiveRH(12, 8, 0.1, 100);
   
	
	XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovRH(
        fovAngleY,
        aspectRatio,
        0.1f,
        1000.0f
        );
		
    XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();

    XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);

    XMStoreFloat4x4(
        &m_constantBufferData.projection,
        XMMatrixTranspose(perspectiveMatrix) // * orientationMatrix)
        );

	// Eye is at (0,0.7,1.5), looking at point (0,-0.1,0) with the up-vector along the y-axis.
	static const XMVECTORF32 eye = { 0.7f, 0.7f, 1.5f, 1.0f };
	static const XMVECTORF32 at = { 0.0f, -0.1f, 0.0f, 1.0f };
	static const XMVECTORF32 up = { 0.0f, 1.0f, 0.0f, 0.0f };
	static XMVECTORF32 light = { 2.0f, 2.0f, 2.0f, 1.0f };
	static XMVECTORF32 eyee = { 0.0f, 0.7f, 1.5f, 1.0f };


	XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixLookAtRH(eye, at, up)));
	XMStoreFloat4(&m_constantBufferData.lightpos, light);
	XMStoreFloat4(&m_constantBufferData.eyepos, eyee);
	//XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixV);


}

void Sample3DSceneRenderer::CreateAsteroidField()
{
	numast = 1000;
	XMVECTOR angles;
//	debris = malloc(numast*sizeof(Asteroid));
	for (int i = 0; i < numast; i++)
	{
		angles = XMVectorSet(3.14*(rand() % 1000) / 1000.0f, 3.14*(rand() % 1000) / 1000.0f, 3.14*(rand() % 1000) / 1000.0f, 1.0f);
		debris[i].pos = XMVectorSet(600 * (rand() % 1000) / 1000.0f, 600 * (rand() % 1000) / 1000.0f, 600 * (rand() % 1000) / 1000.0f, 1.0f);
		debris[i].ori = XMQuaternionRotationRollPitchYawFromVector(angles);
		angles = XMVectorSet(0.01*3.14*(rand() % 1000) / 1000.0f, 0.01*3.14*(rand() % 1000) / 1000.0f, 0.01*3.14*(rand() % 1000) / 1000.0f, 1.0f);
		debris[i].L = XMQuaternionRotationRollPitchYawFromVector(angles);
	}
}

// Called once per frame, rotates the cube and calculates the model and view matrices.
void Sample3DSceneRenderer::Update(DX::StepTimer const& timer)
{
    if (!m_tracking)
    {
        // Convert degrees to radians, then convert seconds to rotation angle
        float radiansPerSecond = XMConvertToRadians(m_degreesPerSecond);
        double totalRotation = timer.GetTotalSeconds() * radiansPerSecond;
        float radians = static_cast<float>(fmod(totalRotation, XM_2PI));

        Rotate(radians);
    }


	m_timer = timer.GetFramesPerSecond() / 60.;

	for (int i = 0; i < numast; i++)
	{ // draw every asteroid
		if (debris[i].boolDraw==false){
			continue;
		}
		if (collisionDetection(cam.pos, debris[i].pos)){
			debris[i].hitCounter = 3;//TODO testing only
		}

		if (intersectRaySphere(cam.pos, laser.ori, debris[i].pos, 10.0f) && laser.isFiring){
			debris[i].hitCounter = 3;//TODO testing only
		}

		if (isDestroyedAsstroid(debris[i].hitCounter)){
			debris[i].boolDraw = false;
		}
		
	}

	UpdateWorld();
	UpdatePlayer();
}

bool Sample3DSceneRenderer::isDestroyedAsstroid(int hitcount){
	if (hitcount > 2){
		return true;
	}
	return false;
}

bool Sample3DSceneRenderer::collisionDetection(XMVECTOR objectOne, XMVECTOR objectTwo){

	XMVECTOR diff = XMVectorSubtract(objectOne, objectTwo);
	XMVECTOR length = XMVectorSqrt(XMVector3Dot(diff, diff));

	if (XMVectorGetX(length) < 2.5f){
		return true;
	}

	return false;
}

bool Sample3DSceneRenderer::intersectRaySphere(XMVECTOR rO, XMVECTOR rV, XMVECTOR sO, double sR) {

	XMVECTOR Q = XMVectorSubtract(sO, rO);
	double c = XMVectorGetX(XMVector3Normalize(Q));// sqrt(XMVectorGetX(Q) * XMVectorGetX(Q) + XMVectorGetY(Q) *XMVectorGetY(Q) + XMVectorGetZ(Q)*XMVectorGetZ(Q));
	double v = XMVectorGetX(XMVector3Dot(Q, rV));
	double d = sR*sR - (c*c - v*v);

	// If there was no intersection, return -1
	if (d < 0.0) return false;

	// Return the distance to the [first] intersecting point
	return true;
}


// Rotate the 3D cube model a set amount of radians.
void Sample3DSceneRenderer::Rotate(float radians)
{
    // Prepare to pass the updated model matrix to the shader
    XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixTranspose(XMMatrixRotationY(radians)));
}

void Sample3DSceneRenderer::DrawOne(ID3D11DeviceContext2 *context, XMMATRIX *thexform) 
{
	/* Draw the object set up in the current context. Use the input transformation matrix.
	(i.e. first send matrix to shader using constant buffer, then draw)
	*/
	
	if (context == NULL)
		return; // don't try anything if no context set

	XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixTranspose(*thexform));

	context->UpdateSubresource(
		m_constantBuffer.Get(),
		0,
		NULL,
		&m_constantBufferData,
		0,
		0
		);

	context->VSSetConstantBuffers(
		0,
		1,
		m_constantBuffer.GetAddressOf()
		);

	// Draw the objects.
	context->DrawIndexed(
		m_indexCount,
		0,
		0
		);

}


void Sample3DSceneRenderer::StartTracking()
{
    m_tracking = true;
}

// When tracking, the 3D cube can be rotated around its Y axis by tracking pointer position relative to the output screen width.
void Sample3DSceneRenderer::TrackingUpdate(float positionX)
{
    if (m_tracking)
    {
        float radians = XM_2PI * 2.0f * positionX / m_deviceResources->GetOutputSize().Width;
        Rotate(radians);
    }
}

void Sample3DSceneRenderer::StopTracking()
{
    m_tracking = false;
}

void Sample3DSceneRenderer::UpdatePlayer()
{
	
	// update player: rebuild coordinate frame using current orientation,
	// plus remake and reset camera transformation
	XMVECTOR L;
	
	// canonical coordinate frame
	cam.forward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	cam.up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	cam.left = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

	// rotate frame with current camera quaternion
	cam.forward = XMVector4Transform(cam.forward, XMMatrixRotationQuaternion(cam.ori));
	cam.up = XMVector4Transform(cam.up, XMMatrixRotationQuaternion(cam.ori));
	cam.left = XMVector3Cross(cam.forward, cam.up);

	// tiny extra rotation even when there is no input
	//L = XMQuaternionRotationAxis(cam.forward, 0.001);
	//cam.ori = XMQuaternionMultiply(cam.ori, L);
	
	// camera advances slowly even without input
	//cam.pos = XMVectorAdd(cam.pos, XMVectorScale(cam.forward, 0.05));

	// remake view matrix, store in constant buffer data
	XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixLookToRH(cam.pos,cam.forward,cam.up)));
	// constant buffer data is headed to card on per-object basis, so no need to reset here
}

void Sample3DSceneRenderer::UpdateWorld()
{
	// asteroids rotate, here; could make them move also, not done now
	for (int i = 0; i < numast; i++)
	{
		debris[i].ori = XMQuaternionMultiply(debris[i].ori, debris[i].L);
	}
}


// Renders one frame using the vertex and pixel shaders.
void Sample3DSceneRenderer::Render()
{

	m_contextReady = false;
	// Loading is asynchronous. Only draw geometry after it's loaded.
	if (!m_loadingComplete)
	{
		return;
	}

	auto context = m_deviceResources->GetD3DDeviceContext();

    // Set render targets to the screen.
    ID3D11RenderTargetView *const targets[1] = { m_deviceResources->GetBackBufferRenderTargetView() };
    context->OMSetRenderTargets(1, targets, m_deviceResources->GetDepthStencilView());

    // Prepare the constant buffer to send it to the graphics device.
	context->UpdateSubresource(
		m_constantBuffer.Get(),
		0,
		NULL,
		&m_constantBufferData,
		0,
		0
		);

    // Each vertex is one instance of the VertexPositionColor struct.
    UINT stride = sizeof(VertexPositionColor);
    UINT offset = 0;
    context->IASetVertexBuffers(
        0,
        1,
        m_vertexBuffer.GetAddressOf(),
        &stride,
        &offset
        );

    context->IASetIndexBuffer(
        m_indexBuffer.Get(),
        DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
        0
        );

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    context->IASetInputLayout(m_inputLayout.Get());

    // Attach our vertex shader.
    context->VSSetShader(
        m_vertexShader.Get(),
        nullptr,
        0
        );

    // Send the constant buffer to the graphics device.
	context->VSSetConstantBuffers(
		0,
		1,
		m_constantBuffer.GetAddressOf()
		);

    // Attach our pixel shader.
    context->PSSetShader(
        m_pixelShader.Get(),
        nullptr,
        0
        );

	// Draw the objects.
	// DM: used for original demo only, skip this
	/*
	context->DrawIndexed(
		m_indexCount,
		0,
		0
		);
	*/

	XMMATRIX thexform;
	
	for (int i = 0; i < numast; i++)
	{ // draw every asteroid
		if (debris[i].boolDraw == true){
			thexform = XMMatrixRotationQuaternion(debris[i].ori);
			thexform = XMMatrixMultiply(thexform, XMMatrixTranslationFromVector(debris[i].pos));
			XMVECTOR move = {- rand() % 100 /95, - rand() % 100/95, - rand() % 100/95 };
			//DEMO XMVECTOR move = {- rand() % 100 /10, - rand() % 100/10, - rand() % 100/10 };
			debris[i].pos += move;
			DrawOne(context, &thexform);
		}
	}

	//camera transform, here i consider camera as root
	XMMATRIX cameraXform, laserXform, targetXform;
	cameraXform = XMMatrixRotationQuaternion(cam.ori);
	cameraXform = XMMatrixMultiply(cameraXform, XMMatrixTranslationFromVector(cam.pos));

	//laser's local xform
	laserXform = XMMatrixIdentity();
	laserXform = XMMatrixRotationQuaternion(laser.ori);
	laserXform = XMMatrixMultiply(laserXform, XMMatrixTranslation(0.0f, -0.2f, 0.0f));

	//fire laser 
	if (laser.isFiring){
		if (laser.type == 0){// This does the weaker laser shot
			//hierarchical xform from camera
			thexform = XMMatrixMultiply(laserXform, cameraXform);
			//thexform = XMMatrixMultiply(thexform, XMMatrixRotationNormal());//Apply rotation for camera
			thexform = XMMatrixMultiply(thexform, XMMatrixTranslation(0.5f, 0.0f, 0.0f));
			thexform = XMMatrixMultiply(XMMatrixScaling(0.1, 0.1, 1000), thexform);
			DrawOne(context, &thexform);
			thexform = XMMatrixMultiply(thexform, XMMatrixTranslation(-1.0f, 0.0f, 0.0f));
			laser.draw = 1;
			laser.power = 1;
		}
		else if (laser.type == 1)//TODO possibly sphere shot?
		{
			thexform = XMMatrixMultiply(laserXform, cameraXform);
			thexform = XMMatrixMultiply(thexform, XMMatrixTranslation(0.5f, 0.0f, 0.0f));
			thexform = XMMatrixMultiply(XMMatrixScaling(0.1, 0.1, 1000), thexform);
			laser.draw = 1;
			laser.power = 3;
		}
		else //This does the Stronger single shot
		{
			if (laser.count < 7){
				thexform = XMMatrixMultiply(laserXform, cameraXform);
				thexform = XMMatrixMultiply(XMMatrixScaling(0.1, 0.1, 1000), thexform);
				laser.count++;
				laser.draw = 1;
				laser.power = 2;
			}
			else if (laser.count > 30)
			{
				laser.count = 0;
				laser.power = 0;
			}
			else{
				laser.count++;
			}
		}
		if (laser.draw == 1){
			DrawOne(context, &thexform);
		}
		laser.isFiring = false;
	}
	else{
		laser.count = 0;
	}

	//target box's local xform
	targetXform = XMMatrixIdentity();
	targetXform = XMMatrixMultiply(targetXform, XMMatrixTranslation(0.0f, 0.0f, 70.0f));

	//inherit laser xform
	targetXform = XMMatrixMultiply(targetXform, laserXform);
	//inherit camera xform
	targetXform = XMMatrixMultiply(targetXform, cameraXform);
	thexform = XMMatrixMultiply(XMMatrixScaling(2, 2, 2), targetXform);
	//DrawOne(context, &thexform);

	//spline pathing
	float circtime, t, p1w, p2w, p3w, p4w;

	circtime = fmodf((m_timer / 100), 16);
	t = circtime - floor(circtime);	

	p1w = (1 - t)*(1 - t)*(1 - t);
	p2w = 3 * t*(1 - t)*(1 - t);
	p3w = 3 * t*t*(1 - t);
	p4w = t*t*t;

	int wsec = ((int)floor(circtime)) * 4; //picks which set of control points are used 

	XMFLOAT4* P;
	P = createSpline();

	XMFLOAT4 Bt;
	Bt.x = p1w*P[0 + wsec].x + p2w*P[1 + wsec].x + p3w*P[2 + wsec].x + p4w*P[3 + wsec].x;
	Bt.y = p1w*P[0 + wsec].y + p2w*P[1 + wsec].y + p3w*P[2 + wsec].y + p4w*P[3 + wsec].y;
	Bt.z = p1w*P[0 + wsec].z + p2w*P[1 + wsec].z + p3w*P[2 + wsec].z + p4w*P[3 + wsec].z;
	Bt.w = 0;

	XMMATRIX temp;
	temp = XMMatrixIdentity();
	temp = XMMatrixTranslation(Bt.x, Bt.y, Bt.z);
	DrawOne(context, &temp);

	//Skybox


	m_contextReady = true;

}

void Sample3DSceneRenderer::CreateDeviceDependentResources()
{
    // Load shaders asynchronously.
    auto loadVSTask = DX::ReadDataAsync(L"SampleVertexShader.cso");
    auto loadPSTask = DX::ReadDataAsync(L"SamplePixelShader.cso");

    // After the vertex shader file is loaded, create the shader and input layout.
    auto createVSTask = loadVSTask.then([this](const std::vector<byte>& fileData) {
        DX::ThrowIfFailed(
            m_deviceResources->GetD3DDevice()->CreateVertexShader(
                &fileData[0],
                fileData.size(),
                nullptr,
                &m_vertexShader
                )
            );

        static const D3D11_INPUT_ELEMENT_DESC vertexDesc [] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };

        DX::ThrowIfFailed(
            m_deviceResources->GetD3DDevice()->CreateInputLayout(
                vertexDesc,
                ARRAYSIZE(vertexDesc),
                &fileData[0],
                fileData.size(),
                &m_inputLayout
                )
            );
    });

    // After the pixel shader file is loaded, create the shader and constant buffer.
    auto createPSTask = loadPSTask.then([this](const std::vector<byte>& fileData) {
        DX::ThrowIfFailed(
            m_deviceResources->GetD3DDevice()->CreatePixelShader(
                &fileData[0],
                fileData.size(),
                nullptr,
                &m_pixelShader
                )
            );

        CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer) , D3D11_BIND_CONSTANT_BUFFER);
        DX::ThrowIfFailed(
            m_deviceResources->GetD3DDevice()->CreateBuffer(
                &constantBufferDesc,
                nullptr,
                &m_constantBuffer
                )
            );
    });

    // Once both shaders are loaded, create the mesh.
    auto createCubeTask = (createPSTask && createVSTask).then([this] () {

        // Load mesh vertices. Each vertex has a position and a color.
        static const VertexPositionColor cubeVertices[] = 
        {
			{ XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT2(0.0f, 1.0f) },
			{ XMFLOAT3(-0.5f, -0.5f, 0.5f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(-0.5f, -0.5f, 0.5f), XMFLOAT2(0.0f, 1.0f) },
			{ XMFLOAT3(-0.5f, 0.5f, -0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(-0.5f, 0.5f, -0.5f), XMFLOAT2(0.0f, 1.0f) },
			{ XMFLOAT3(-0.5f, 0.5f, 0.5f), XMFLOAT3(0.0f, 1.0f, 1.0f), XMFLOAT3(-0.5f, 0.5f, 0.5f), XMFLOAT2(0.0f, 1.0f) },
			{ XMFLOAT3(0.5f, -0.5f, -0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(0.5f, -0.5f, -0.5f), XMFLOAT2(0.0f, 1.0f) },
			{ XMFLOAT3(0.5f, -0.5f, 0.5f), XMFLOAT3(1.0f, 0.0f, 1.0f), XMFLOAT3(0.5f, -0.5f, 0.5f), XMFLOAT2(0.0f, 1.0f) },
			{ XMFLOAT3(0.5f, 0.5f, -0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT3(0.5f, 0.5f, -0.5f), XMFLOAT2(0.0f, 1.0f) },
			{ XMFLOAT3(0.5f, 0.5f, 0.5f), XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.5f, 0.5f, 0.5f), XMFLOAT2(0.0f, 1.0f) },
        };

		// Load mesh indices. Each trio of indices represents
		// a triangle to be rendered on the screen.
		// For example: 0,2,1 means that the vertices with indexes
		// 0, 2 and 1 from the vertex buffer compose the 
		// first triangle of this mesh.
		static const unsigned short cubeIndices[] =
		{
			0, 2, 1, // -x
			1, 2, 3,

			4, 5, 6, // +x
			5, 7, 6,

			0, 1, 5, // -y
			0, 5, 4,

			2, 6, 7, // +y
			2, 7, 3,

			0, 4, 6, // -z
			0, 6, 2,

			1, 3, 7, // +z
			1, 7, 5,
		};

		VertexPositionColor *particledata;
		VertexPositionColor thisone;
		XMFLOAT3 thisnor;
		numParticles = 7500;

		particledata = (VertexPositionColor *)malloc(numParticles*sizeof(VertexPositionColor));

		float u, v, w, theta, phi, spray;
		float trad = 0.4;
		float maxspray = 0.5;

		std::random_device rd;
		std::mt19937 lotto(rd());
		std::uniform_real_distribution<> distro(0, 1);

		for (int i = 0; i < numParticles; i++)
		{
			u = static_cast<float>(distro(lotto));
			v = static_cast<float>(distro(lotto));
			w = static_cast<float>(distro(lotto));

			theta = u * 2 * 3.1416;

			phi = acos(2 * v - 1.0);
			//			phi = v * 2 * 3.1416;
			spray = maxspray*cbrt(w);// *cbrt(cbrt(cbrt(w))); // cbrt(w);
			thisnor = // normal direction
				XMFLOAT3(spray*cos(theta)*sin(phi), spray*sin(theta)*sin(phi), spray*cos(phi));

			thisone = // position + color of this vertex, now added normal and texcoord
			{
				XMFLOAT3(thisnor.x*trad, thisnor.y*trad, thisnor.z*trad),
				XMFLOAT3(i / (float)numParticles, 1, 0.05),
				XMFLOAT3(thisnor.x, thisnor.y, thisnor.z),
				XMFLOAT2(0, 0)
			};

			particledata[i] = thisone; // add to vertex
		}

		int circle = 30;
		float crad = 2.0f;
		int loop;
		int segment = 30;
		loop = 3 * segment;
		int numvertices = circle*loop;
		int numindices = circle*loop * 6;
		VertexPositionColor *vertices;

		vertices = (VertexPositionColor *)malloc(numvertices*sizeof(VertexPositionColor));


		// a circle now
		for (int i = 0; i < loop; i++)
		{
			theta = 2 * i*3.1416 / loop; // large loop

			for (int j = 0; j < circle; j++) // small circle
			{
				phi = 2 * j*3.1416 / circle; // from 0 to 2PI

				thisnor = // normal direction
					XMFLOAT3(cos(theta)*sin(phi), cos(phi), sin(theta)*sin(phi));
				thisone = // position + color of this vertex, now added normal and texcoord
				{
					XMFLOAT3(thisnor.x*crad, thisnor.y*crad, thisnor.z*crad),
					XMFLOAT3(i / (segment), j / (circle), 0.05),
					XMFLOAT3(thisnor.x, thisnor.y, thisnor.z),
					XMFLOAT2((float)i / loop, (float)j / circle)
				};
				vertices[i*circle + j] = thisone; // add to vertex array
			}
		}

		WORD *indices;
		indices = (WORD *)malloc(sizeof(WORD)*numindices);
		int count = 0;
		for (int i = 0; i < loop; i++)
		{
			for (int j = 0; j < circle; j++)
			{
				// two triangles per quad

				// proper order:

				indices[count++] = WORD(((i + 1) % loop)*circle + j);
				indices[count++] = WORD(i*circle + ((j + 1) % circle));
				indices[count++] = WORD((i*circle + j));

				indices[count++] = WORD(((i + 1) % loop)*circle + j);
				indices[count++] = WORD(((i + 1) % loop)*circle + ((j + 1) % circle));
				indices[count++] = WORD(i*circle + ((j + 1) % circle));
			}
		}

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = vertices;
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(numvertices*sizeof(VertexPositionColor), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
			&vertexBufferDesc,
			&vertexBufferData,
			&m_vertexBuffer
			)
			);

		D3D11_SUBRESOURCE_DATA vertexBufferData2 = { 0 };
		vertexBufferData2.pSysMem = particledata;
		vertexBufferData2.SysMemPitch = 0;
		vertexBufferData2.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc2(numParticles*sizeof(VertexPositionColor), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
			&vertexBufferDesc2,
			&vertexBufferData2,
			&m_particleVB
			)
			);

		m_indexCount = numindices; // ARRAYSIZE(cubeIndices);

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
		indexBufferData.pSysMem = indices;
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(numindices*sizeof(WORD), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
			&indexBufferDesc,
			&indexBufferData,
			&m_indexBuffer
			)
			);
    });

	auto context = m_deviceResources->GetD3DDeviceContext();

	ID3D11Resource *pp;
	ID3D11ShaderResourceView *textureView;

	CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), context,
		L"Assets/2365.dds",
		&pp,
		&textureView,
		0);

	// Create the sampler state
	ID3D11SamplerState*                 mySampler = NULL;

	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = 2 * D3D11_FLOAT32_MAX;
	m_deviceResources->GetD3DDevice()->CreateSamplerState(&sampDesc, &mySampler);

	// add the texture and sampler to the pixel shader:
	context->PSSetShaderResources(0, 1, &textureView);
	context->PSSetSamplers(0, 1, &mySampler);

    // Once the cube is loaded, the object is ready to be rendered.
    createCubeTask.then([this] () {
        m_loadingComplete = true;
    });
}

XMFLOAT4* Sample3DSceneRenderer::createSpline()
{
	//Spline Control point set up
	DirectX::XMFLOAT4 m_splineArray[64];
	float u, v, w;

	std::random_device rd;
	std::mt19937 lotto(rd());
	std::uniform_real_distribution<> distro(0, 1);

	//Randomize X, Y, Z
	u = static_cast<float>(distro(lotto)) * 200 - 100;
	v = static_cast<float>(distro(lotto)) * 200 - 100;
	w = static_cast<float>(distro(lotto)) * 200 - 100;

	m_splineArray[0] = XMFLOAT4(0, 0, 0, 0);
	m_splineArray[1] = XMFLOAT4(u, v, w, 0);
	m_splineArray[63] = m_splineArray[0];

	for (int i = 2; i < 63; i++)
	{
		u = static_cast<float>(distro(lotto)) * 200 - 100;
		v = static_cast<float>(distro(lotto)) * 200 - 100;
		w = static_cast<float>(distro(lotto)) * 200 - 100;

		if (i % 4 == 0)		m_splineArray[i] = m_splineArray[i - 1];
		if (i % 4 == 1)		m_splineArray[i] = XMFLOAT4(2 * m_splineArray[i - 1].x - m_splineArray[i - 3].x,
														2 * m_splineArray[i - 1].y - m_splineArray[i - 3].y,
														2 * m_splineArray[i - 1].z - m_splineArray[i - 3].z, 0);
		if (i % 4 == 2)		m_splineArray[i] = XMFLOAT4(u, v, w, 0);
		if (i % 4 == 3)		m_splineArray[i] = XMFLOAT4(u, v, w, 0);
	}

	return m_splineArray;
}

void Sample3DSceneRenderer::ReleaseDeviceDependentResources()
{
    m_loadingComplete = false;
    m_vertexShader.Reset();
    m_inputLayout.Reset();
    m_pixelShader.Reset();
    m_constantBuffer.Reset();
    m_vertexBuffer.Reset();
    m_indexBuffer.Reset();
	m_particleVB.Reset();
}