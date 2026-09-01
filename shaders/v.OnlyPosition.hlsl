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
	output.pos = float4(input.pos, 1.0f);
	output.color = input.color;
	return output;
}
