Texture2D tex: register(t0, space2);
SamplerState tex_sampler: register(s0, space2);

struct Input {
	float4 pos: SV_Position;
	float4 color: TEXCOORD1;
	float2 uv: TEXCOORD2;
	float3 normal: TEXCOORD3;
};

float4 main(Input input): SV_Target0 {
	float4 tex_color = tex.Sample(tex_sampler, input.uv);
	float3 n = normalize(input.normal);
	float3 light_dir = normalize(float3(0.3, 1.0, 0.2)); // direction *to* the light
	float ndotl = max(dot(n, light_dir), 0.0);
	float ambient = 0.2;
	float diffuse = 0.8 * ndotl;
	float3 lit = tex_color.rgb * (ambient + diffuse);
	return float4(input.color.rgb * lit, tex_color.a);
}
