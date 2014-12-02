//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

// A constant buffer that stores the three basic column-major matrices for composing geometry.
cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
    matrix model;
    matrix view;
    matrix projection;
	float4 lightpos;
	float4 eyepos;
};

// Per-vertex data used as input to the vertex shader.
struct VertexShaderInput
{
    float3 pos : POSITION;
    float3 color : COLOR0;
	float3 normal : NORMAL0;
	float2 texcoord : TEXCOORD0;
};

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 color : COLOR0;
	float3 normal : NORMAL0;
	float4 surfpos : POSITION0;
	float2 texcoord : TEXCOORD0;
};

// Simple shader to do vertex processing on the GPU.
PixelShaderInput main(VertexShaderInput input)
{
	PixelShaderInput output;
	float4 pos = float4(input.pos, 1.0f);
		float4 norm = float4(input.normal, 0.0f);
		// Transform the vertex position into projected space.
		pos = mul(pos, model);
	output.surfpos = pos;

	pos = mul(pos, view);
	pos = mul(pos, projection);
	output.pos = pos;

	// transform the surface normal -- model xform only
	output.normal = mul(norm, model);

	// Pass the color through without modification.
	output.color = input.color;
	// pass through texture coordinates:
	output.texcoord = input.texcoord;

	return output;
}
