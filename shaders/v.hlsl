cbuffer MatrixBuffer : register(b0, space1) {
	float4x4 mvp;
};

struct Input {
	float3 pos: TEXCOORD0;
	float4 color: TEXCOORD1;
	float2 uv: TEXCOORD2;
};

struct Output {
	float4 pos: SV_Position;
	float4 color: TEXCOORD1;
	float2 uv: TEXCOORD2;
};

Output main(Input input) {
	Output output;
	output.pos = mul(mvp, float4(input.pos, 1.0f));
	output.color = input.color;
	output.uv = input.uv;
	return output;
}
