// same constant buffer as used for other shaders
cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
	float4 timer;
	float4 up;
};

// vertex data obtained from the vertex shader.
struct Geo_In
{
	float4 wPos : SV_POSITION;
	float2 texcoord : TEXCOORD0;
	float id : TEXCOORD1;
	float4 color : COLOR0;
};

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float4 color : COLOR0;
	float4 surfpos : POSITION0;
	float2 texcoord : TEXCOORD0;
};

// Geo shader, makes quad
[maxvertexcount(4)]
void GS(point Geo_In gin[1], inout TriangleStream<PixelShaderInput> triStream)
{
	float4 v[4];

	float hHeight = 0.08;
	float hWidth = 0.08;

	v[0] = float4(gin[0].wPos.x - hWidth, gin[0].wPos.y + hHeight, gin[0].wPos.z, 1);
	v[1] = float4(gin[0].wPos.x + hWidth, gin[0].wPos.y + hHeight, gin[0].wPos.z, 1);
	v[2] = float4(gin[0].wPos.x - hWidth, gin[0].wPos.y - hHeight, gin[0].wPos.z, 1);
	v[3] = float4(gin[0].wPos.x + hWidth, gin[0].wPos.y - hHeight, gin[0].wPos.z, 1);

	PixelShaderInput gout;
	int fid = (int)floor(gin[0].id * 4); // 0-3, used to pick sector from flame 2x2 drawing
	for (uint i = 0; i < 4; i++)
	{
		gout.surfpos = v[i];
		gout.pos = mul(v[i], view);
		gout.pos = mul(gout.pos, projection);
		gout.texcoord = float2(floor(i / 2)*0.5 + 0.5*(fid/2), (float)(i % 2)*0.5 + 0.5*(fid%2));
		gout.color = float4(0.8,0.3,0.04, gin[0].color.w);
		triStream.Append(gout);
	}
}
