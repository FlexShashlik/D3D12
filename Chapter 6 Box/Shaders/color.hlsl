cbuffer cbPerObject : register(b0)
{
	float4x4 gWorldViewProj;
	float4 gPulseColor;
	float gTime;
};

struct VertexIn
{
	float3 PosL  : POSITION;
    float4 Color : COLOR;
};

struct VertexOut
{
	float4 PosH  : SV_POSITION;
    float4 Color : COLOR;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
	vin.PosL.xy += 0.5f * sin(vin.PosL.x) * sin(3.0f * gTime);
	vin.PosL.z *= 0.6f + 0.4f * sin(2.0f * gTime);

	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);

    vout.Color = vin.Color * gTime;
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	const float pi = 3.14159;

	float s = 0.5f * sin(2 * gTime - 0.25f * pi) + 0.5f;

	float4 c = lerp(pin.Color, gPulseColor, s);

	return c;
}


