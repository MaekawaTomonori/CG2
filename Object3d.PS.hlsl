#include "Object3d.hlsli"

struct DirectionalLight{
    float32_t4 color;
    float32_t3 direction;
    float intensity;
};
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);

struct Material{
    float32_t4 color;
    int32_t enableLighting;
};
ConstantBuffer<Material> gMaterial : register(b0); //"g"Material = global

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    float32_t4 texColor = gTexture.Sample(gSampler, input.texcoord);
    if(gMaterial.enableLighting != 0){
        //Lambertain Reflectance
    	//float cos = saturate(dot(normalize(input.normal), -gDirectionalLight.direction));

        //Half Lambert
        float NdotL = dot(normalize(input.normal), -gDirectionalLight.direction);
        float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);

    	output.color = gMaterial.color * texColor * gDirectionalLight.color * cos * gDirectionalLight.intensity;
    } else{
		output.color = gMaterial.color * texColor;
    }
    return output;
}