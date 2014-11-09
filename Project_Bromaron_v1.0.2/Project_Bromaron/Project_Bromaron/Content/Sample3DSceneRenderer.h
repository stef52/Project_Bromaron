﻿//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
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

namespace Project_Bromaron
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
        void StartTracking();
        void TrackingUpdate(float positionX);
        void StopTracking();
        bool IsTracking() { return m_tracking; }
		bool collisionDetectionShotA(XMVECTOR objectOne, XMVECTOR objectTwo);
		bool collisionDetectionAA(XMVECTOR objectOne, XMVECTOR objectTwo);
		bool collisionDetectionShipA(XMVECTOR objectOne, XMVECTOR objectTwo);
		void setFirering(bool fire);


    private:
        void Rotate(float radians);

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
        float   m_degreesPerSecond;
        bool    m_tracking;
    };
}
