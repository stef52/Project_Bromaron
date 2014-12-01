//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

#include "..\Helpers\DeviceResources.h"
#include "ShaderStructures.h"
#include "..\Helpers\StepTimer.h"

using namespace DirectX;

namespace DirectXGame2
{
    // This sample renderer instantiates a basic rendering pipeline.
    class Sample3DSceneRenderer
    {
    public:
        Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources);
        void CreateDeviceDependentResources();
        void CreateWindowSizeDependentResources();
        void ReleaseDeviceDependentResources();
        void Update(DX::StepTimer const& timer);
        void Render();
		void UpdatePlayer();
		void UpdateWorld();
        void StartTracking();
        void TrackingUpdate(float positionX);
        void StopTracking();
        bool IsTracking() { return m_tracking; }
		void CameraSpin(float roll, float pitch, float yaw);
		void CameraMove(float forr);
		void LaserSpin(float laserPitch, float laserYaw);
		void LaserFire(bool isFiring);
		void LaserFireType(int type);
    private:
        void Rotate(float radians);
		void DrawOne(ID3D11DeviceContext2 *context, XMMATRIX *thexform);
		void CreateAsteroidField();
		void CreateCamera();
    private:
        // Cached pointer to device resources.
        std::shared_ptr<DX::DeviceResources> m_deviceResources;

        // Direct3D resources for cube geometry.
        Microsoft::WRL::ComPtr<ID3D11InputLayout>   m_inputLayout;
        Microsoft::WRL::ComPtr<ID3D11Buffer>        m_vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer>        m_indexBuffer;
        Microsoft::WRL::ComPtr<ID3D11VertexShader>  m_vertexShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>   m_pixelShader;
        Microsoft::WRL::ComPtr<ID3D11Buffer>        m_constantBuffer;

        // System resources for cube geometry.
        ModelViewProjectionConstantBuffer    m_constantBufferData;
        uint32    m_indexCount;

        // Variables used with the rendering loop.
        bool    m_loadingComplete;
		bool	m_contextReady;
        float   m_degreesPerSecond;
        bool    m_tracking;

		// Variables for player
		typedef struct Camera
		{
			XMVECTOR pos;
			XMVECTOR ori;
			XMVECTOR forward;
			XMVECTOR up;
			XMVECTOR left; // player coordinate frame, rebuilt from orientation
		};

		Camera cam;
		// Variables for asteroid field
		typedef struct Asteroid {
			XMVECTOR pos; // position
			XMVECTOR ori; // orientation
			XMVECTOR L; // angular momentum (use as velocity)
			XMVECTOR vel; // linear velocity
		};

		int numast;
		Asteroid debris[1500];

		typedef struct Laser
		{
			XMVECTOR ori;
			bool isFiring;
			int type;
		};

		Laser laser;
    };
}

