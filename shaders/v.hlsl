cbuffer MatrixBuffer : register(b0, space1) {
	float4x4 mvp;
};

struct Input {
	float3 pos: TEXCOORD0;
	float4 color: TEXCOORD1;
};

struct Output {
    float4 pos: SV_Position;
    float4 color: TEXCOORD1;
};

Output main(Input input) {
	Output output;
	output.pos = mul(mvp, float4(input.pos, 1.0f));
	output.color = input.color;
	return output;
}
