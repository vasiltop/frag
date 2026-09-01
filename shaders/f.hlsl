Texture2D tex: register(t0, space2);
SamplerState tex_sampler: register(s0, space2);

struct Input {
	float4 pos: SV_Position;
	float4 color: TEXCOORD1;
	float2 uv: TEXCOORD2;
};

float4 main(Input input): SV_Target0 {
	float4 tex_color = tex.Sample(tex_sampler, input.uv);
	return input.color * tex_color;
}
