struct MATERIAL
{
	float4					m_cAmbient;
	float4					m_cDiffuse;
	float4					m_cSpecular; //a = power
	float4					m_cEmissive;
};

cbuffer cbCameraInfo : register(b1)
{
	matrix					gmtxView : packoffset(c0);
	matrix					gmtxProjection : packoffset(c4);
	float3					gvCameraPosition : packoffset(c8);
};

cbuffer cbGameObjectInfo : register(b2)
{
	matrix					gmtxGameObject : packoffset(c0);
	MATERIAL				gMaterial : packoffset(c4);
	uint					gnTexturesMask : packoffset(c8);
};

//cbuffer cbShadowInfo : register(b5)
//{
//    matrix gmtxLightView : packoffset(c0); // 4 registers
//    matrix gmtxLightProj : packoffset(c4); // 4 registers
//    float gShadowBias : packoffset(c8.x); // bias
//    float3 _padShadow0 : packoffset(c8.y);
//    float2 gShadowTexel : packoffset(c9.x); // (1/width, 1/height)
//    float2 _padShadow1 : packoffset(c9.z);
//};

#include "Light.hlsl"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//#define _WITH_VERTEX_LIGHTING

#define MATERIAL_ALBEDO_MAP			0x01
#define MATERIAL_SPECULAR_MAP		0x02
#define MATERIAL_NORMAL_MAP			0x04
#define MATERIAL_METALLIC_MAP		0x08
#define MATERIAL_EMISSION_MAP		0x10
#define MATERIAL_DETAIL_ALBEDO_MAP	0x20
#define MATERIAL_DETAIL_NORMAL_MAP	0x40

Texture2D gtxtAlbedoTexture : register(t6);
Texture2D gtxtSpecularTexture : register(t7);
Texture2D gtxtNormalTexture : register(t8);
Texture2D gtxtMetallicTexture : register(t9);
Texture2D gtxtEmissionTexture : register(t10);
Texture2D gtxtDetailAlbedoTexture : register(t11);
Texture2D gtxtDetailNormalTexture : register(t12);

SamplerState gssWrap : register(s0);

//Texture2D<float> gtxtShadowMap : register(t5);
//SamplerComparisonState gssShadow : register(s3);

//float2 ClipToUV(float4 lightH)
//{
//    float3 ndc = lightH.xyz / lightH.w; // [-1,1]
//    float2 uv;
//    uv.x = ndc.x * 0.5f + 0.5f;
//    uv.y = -ndc.y * 0.5f + 0.5f;
//    return uv;
//}

//float ShadowFactor(float4 lightH)
//{
//    float3 ndc = lightH.xyz / lightH.w;

//    // 라이트 frustum 밖이면 그림자 적용 X
//    if (ndc.x < -1 || ndc.x > 1 || ndc.y < -1 || ndc.y > 1 || ndc.z < 0 || ndc.z > 1)
//        return 1.0f;

//    float2 uv = ClipToUV(lightH);

//    float currentDepth = ndc.z;
//    float depth = currentDepth - gShadowBias;

//    // 3x3 PCF
//    float sum = 0.0f;
//    [unroll]
//    for (int y = -1; y <= 1; ++y)
//    {
//        [unroll]
//        for (int x = -1; x <= 1; ++x)
//        {
//            float2 uvo = uv + float2(x, y) * gShadowTexel;
//            sum += gtxtShadowMap.SampleCmpLevelZero(gssShadow, uvo, depth);
//        }
//    }
//    return sum / 9.0f;
//}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct VS_STANDARD_INPUT
{
	float3 position : POSITION;
	float2 uv : TEXCOORD;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 bitangent : BITANGENT;
};

struct VS_STANDARD_OUTPUT
{
	float4 position : SV_POSITION;
	float3 positionW : POSITION;
	float3 normalW : NORMAL;
	float3 tangentW : TANGENT;
	float3 bitangentW : BITANGENT;
	float2 uv : TEXCOORD;
    //float4 positionLightH : TEXCOORD2;
};

VS_STANDARD_OUTPUT VSStandard(VS_STANDARD_INPUT input)
{
	VS_STANDARD_OUTPUT output;

	output.positionW = mul(float4(input.position, 1.0f), gmtxGameObject).xyz;
	output.normalW = mul(input.normal, (float3x3)gmtxGameObject);
	output.tangentW = mul(input.tangent, (float3x3)gmtxGameObject);
	output.bitangentW = mul(input.bitangent, (float3x3)gmtxGameObject);
	output.position = mul(mul(float4(output.positionW, 1.0f), gmtxView), gmtxProjection);
	output.uv = input.uv;

    //float4 posW = float4(output.positionW, 1.0f);
    //output.positionLightH = mul(mul(posW, gmtxLightView), gmtxLightProj);

	return(output);
}

float4 PSStandard(VS_STANDARD_OUTPUT input) : SV_TARGET
{
    float4 cAlbedoColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
    if (gnTexturesMask & MATERIAL_ALBEDO_MAP)
        cAlbedoColor = gtxtAlbedoTexture.Sample(gssWrap, input.uv);
    float4 cSpecularColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
    if (gnTexturesMask & MATERIAL_SPECULAR_MAP)
        cSpecularColor = gtxtSpecularTexture.Sample(gssWrap, input.uv);
    float4 cNormalColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
    if (gnTexturesMask & MATERIAL_NORMAL_MAP)
        cNormalColor = gtxtNormalTexture.Sample(gssWrap, input.uv);
    float4 cMetallicColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
    if (gnTexturesMask & MATERIAL_METALLIC_MAP)
        cMetallicColor = gtxtMetallicTexture.Sample(gssWrap, input.uv);
    float4 cEmissionColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
    if (gnTexturesMask & MATERIAL_EMISSION_MAP)
        cEmissionColor = gtxtEmissionTexture.Sample(gssWrap, input.uv);

    float3 normalW;
    float4 cColor = cAlbedoColor + cSpecularColor + cMetallicColor + cEmissionColor;
    if (gnTexturesMask & MATERIAL_NORMAL_MAP)
    {
        float3x3 TBN = float3x3(normalize(input.tangentW), normalize(input.bitangentW), normalize(input.normalW));
        float3 vNormal = normalize(cNormalColor.rgb * 2.0f - 1.0f); //[0, 1] → [-1, 1]
        normalW = normalize(mul(vNormal, TBN));
    }
    else
    {
        normalW = normalize(input.normalW);
    }
    
    float4 cIllumination = Lighting(input.positionW, normalW);
    //float shadow = ShadowFactor(input.positionLightH);
    //cIllumination.rgb *= shadow;

    //return lerp(cColor, cIllumination, 0.5f);
    return (lerp(cColor, cIllumination, 0.5f));
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//struct VS_SHADOW_INPUT
//{
//    float3 position : POSITION;
//};

//struct VS_SHADOW_OUTPUT
//{
//    float4 position : SV_POSITION;
//};

//VS_SHADOW_OUTPUT VSShadow(VS_SHADOW_INPUT input)
//{
//    VS_SHADOW_OUTPUT output;

//    float3 posW = mul(float4(input.position, 1.0f), gmtxGameObject).xyz;
//    output.position = mul(mul(float4(posW, 1.0f), gmtxLightView), gmtxLightProj);

//    return output;
//}

//float4 PSShadow(VS_SHADOW_OUTPUT input) : SV_TARGET
//{
//    return 0;
//}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
#define MAX_VERTEX_INFLUENCES			4
#define SKINNED_ANIMATION_BONES			256

cbuffer cbBoneOffsets : register(b7)
{
	float4x4 gpmtxBoneOffsets[SKINNED_ANIMATION_BONES];
};

cbuffer cbBoneTransforms : register(b8)
{
	float4x4 gpmtxBoneTransforms[SKINNED_ANIMATION_BONES];
};

struct VS_SKINNED_STANDARD_INPUT
{
	float3 position : POSITION;
	float2 uv : TEXCOORD;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 bitangent : BITANGENT;
	int4 indices : BONEINDEX;
	float4 weights : BONEWEIGHT;
};

VS_STANDARD_OUTPUT VSSkinnedAnimationStandard(VS_SKINNED_STANDARD_INPUT input)
{
	VS_STANDARD_OUTPUT output;

	//output.positionW = float3(0.0f, 0.0f, 0.0f);
	//output.normalW = float3(0.0f, 0.0f, 0.0f);
	//output.tangentW = float3(0.0f, 0.0f, 0.0f);
	//output.bitangentW = float3(0.0f, 0.0f, 0.0f);
	//matrix mtxVertexToBoneWorld;
	//for (int i = 0; i < MAX_VERTEX_INFLUENCES; i++)
	//{
	//	mtxVertexToBoneWorld = mul(gpmtxBoneOffsets[input.indices[i]], gpmtxBoneTransforms[input.indices[i]]);
	//	output.positionW += input.weights[i] * mul(float4(input.position, 1.0f), mtxVertexToBoneWorld).xyz;
	//	output.normalW += input.weights[i] * mul(input.normal, (float3x3)mtxVertexToBoneWorld);
	//	output.tangentW += input.weights[i] * mul(input.tangent, (float3x3)mtxVertexToBoneWorld);
	//	output.bitangentW += input.weights[i] * mul(input.bitangent, (float3x3)mtxVertexToBoneWorld);
	//}
	float4x4 mtxVertexToBoneWorld = (float4x4)0.0f;
	for (int i = 0; i < MAX_VERTEX_INFLUENCES; i++)
	{
//		mtxVertexToBoneWorld += input.weights[i] * gpmtxBoneTransforms[input.indices[i]];
		mtxVertexToBoneWorld += input.weights[i] * mul(gpmtxBoneOffsets[input.indices[i]], gpmtxBoneTransforms[input.indices[i]]);
	}
	output.positionW = mul(float4(input.position, 1.0f), mtxVertexToBoneWorld).xyz;
	output.normalW = mul(input.normal, (float3x3)mtxVertexToBoneWorld).xyz;
	output.tangentW = mul(input.tangent, (float3x3)mtxVertexToBoneWorld).xyz;
	output.bitangentW = mul(input.bitangent, (float3x3)mtxVertexToBoneWorld).xyz;

    //float4 posW = float4(output.positionW, 1.0f);
    //output.positionLightH = mul(mul(posW, gmtxLightView), gmtxLightProj);
	
//	output.positionW = mul(float4(input.position, 1.0f), gmtxGameObject).xyz;

	output.position = mul(mul(float4(output.positionW, 1.0f), gmtxView), gmtxProjection);
	output.uv = input.uv;

	return(output);
}

//VS_SHADOW_OUTPUT VSSkinnedShadow(VS_SKINNED_STANDARD_INPUT input)
//{
//    VS_SHADOW_OUTPUT o;

//    float4x4 mtxVertexToBoneWorld = (float4x4) 0.0f;
//    for (int i = 0; i < MAX_VERTEX_INFLUENCES; i++)
//    {
//        mtxVertexToBoneWorld += input.weights[i] * mul(gpmtxBoneOffsets[input.indices[i]], gpmtxBoneTransforms[input.indices[i]]);
//    }

//    float3 posW = mul(float4(input.position, 1.0f), mtxVertexToBoneWorld).xyz;
//    o.position = mul(mul(float4(posW, 1.0f), gmtxLightView), gmtxLightProj);
//    return o;
//}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
Texture2D gtxtTexture : register(t0);

struct VS_TEXTURED_INPUT
{
    float3 position : POSITION;
    float2 uv : TEXCOORD;
};

struct VS_TEXTURED_OUTPUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

VS_TEXTURED_OUTPUT VSTextured(VS_TEXTURED_INPUT input)
{
    VS_TEXTURED_OUTPUT output;

    output.position = mul(mul(mul(float4(input.position, 1.0f), gmtxGameObject), gmtxView), gmtxProjection);
    output.uv = input.uv;
    return (output);
}

float4 PSTextured(VS_TEXTURED_OUTPUT input) : SV_TARGET
{
    float4 cColor = gtxtTexture.Sample(gssWrap, input.uv);
    //cColor.a = 0.25f;
    cColor.a *= gMaterial.m_cDiffuse.a;
    return (cColor);
}

VS_TEXTURED_OUTPUT VSTextureToScreen(VS_TEXTURED_INPUT input)
{
    VS_TEXTURED_OUTPUT output;

    output.position = float4(input.position, 1.0f);
    output.uv = input.uv;

    return (output);
}
// HP 바 전용 - discard 없이 그냥 출력
float4 PSTextureToScreenHP(VS_TEXTURED_OUTPUT input) : SV_TARGET
{
    float4 cColor = gtxtTexture.Sample(gssWrap, input.uv);
    return (cColor);
}
float4 PSTextureToScreen(VS_TEXTURED_OUTPUT input) : SV_TARGET
{
    float4 cColor = gtxtTexture.Sample(gssWrap, input.uv);

    float maxGB = max(cColor.g, cColor.b);
    if (cColor.r > 0.3f && cColor.r > maxGB * 3.0f)
        discard;
   //if (cColor.a < 0.1f)
   //    discard;
    
    return (cColor);
}

struct VS_FONT_INPUT
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD;
};

struct VS_FONT_OUTPUT
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

VS_FONT_OUTPUT VSFont(VS_FONT_INPUT input)
{
    VS_FONT_OUTPUT output;
    output.position = float4(input.position, 1.0f);
    output.texCoord = input.texCoord;
    return output;
}

Texture2D gFontTexture : register(t3);
SamplerState gFontSampler : register(s2);

float4 PSFont(VS_FONT_OUTPUT input) : SV_Target
{
    float4 color = gFontTexture.Sample(gFontSampler, input.texCoord);
    clip(color.a - 0.05f);
    return color;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
Texture2D gtxtTerrainBaseTexture : register(t1);
Texture2D gtxtTerrainDetailTexture : register(t2);

struct VS_TERRAIN_INPUT
{
	float3 position : POSITION;
	float4 color : COLOR;
	float2 uv0 : TEXCOORD0;
	float2 uv1 : TEXCOORD1;
};

struct VS_TERRAIN_OUTPUT
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
	float2 uv0 : TEXCOORD0;
	float2 uv1 : TEXCOORD1;
};

VS_TERRAIN_OUTPUT VSTerrain(VS_TERRAIN_INPUT input)
{
	VS_TERRAIN_OUTPUT output;

	output.position = mul(mul(mul(float4(input.position, 1.0f), gmtxGameObject), gmtxView), gmtxProjection);
	output.color = input.color;
	output.uv0 = input.uv0;
	output.uv1 = input.uv1;

	return(output);
}

float4 PSTerrain(VS_TERRAIN_OUTPUT input) : SV_TARGET
{
	float4 cBaseTexColor = gtxtTerrainBaseTexture.Sample(gssWrap, input.uv0);
	float4 cDetailTexColor = gtxtTerrainDetailTexture.Sample(gssWrap, input.uv1);
//	float4 cColor = saturate((cBaseTexColor * 0.5f) + (cDetailTexColor * 0.5f));
	float4 cColor = input.color * saturate((cBaseTexColor * 0.5f) + (cDetailTexColor * 0.5f));

	return(cColor);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
struct VS_SKYBOX_CUBEMAP_INPUT
{
	float3 position : POSITION;
};

struct VS_SKYBOX_CUBEMAP_OUTPUT
{
	float3	positionL : POSITION;
	float4	position : SV_POSITION;
};

VS_SKYBOX_CUBEMAP_OUTPUT VSSkyBox(VS_SKYBOX_CUBEMAP_INPUT input)
{
	VS_SKYBOX_CUBEMAP_OUTPUT output;

	output.position = mul(mul(mul(float4(input.position, 1.0f), gmtxGameObject), gmtxView), gmtxProjection);
	output.positionL = input.position;

	return(output);
}

TextureCube gtxtSkyCubeTexture : register(t13);
SamplerState gssClamp : register(s1);

float4 PSSkyBox(VS_SKYBOX_CUBEMAP_OUTPUT input) : SV_TARGET
{
	float4 cColor = gtxtSkyCubeTexture.Sample(gssClamp, input.positionL);

	return(cColor);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
struct VS_INSTANCED_STANDARD_INPUT
{
    float3 position : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
    
    matrix mtxInstanceTransform : INSTANCE_TRANSFORM;
};

VS_STANDARD_OUTPUT VSInstancedStandard(VS_INSTANCED_STANDARD_INPUT input)
{
    VS_STANDARD_OUTPUT output;
	
    float4 localPos = mul(float4(input.position, 1.0f), gmtxGameObject);
    float3 localNormal = mul(input.normal, (float3x3) gmtxGameObject);
    float3 localTangent = mul(input.tangent, (float3x3) gmtxGameObject);
    float3 localBitangent = mul(input.bitangent, (float3x3) gmtxGameObject);
    
    output.positionW = mul(localPos, input.mtxInstanceTransform).xyz;
    output.normalW = mul(localNormal, (float3x3) input.mtxInstanceTransform);
    output.tangentW = mul(localTangent, (float3x3) input.mtxInstanceTransform);
    output.bitangentW = mul(localBitangent, (float3x3) input.mtxInstanceTransform);
    
    output.position = mul(mul(float4(output.positionW, 1.0f), gmtxView), gmtxProjection);
    output.uv = input.uv;

    return (output);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
#define SPRITE_COLS  4
#define SPRITE_ROWS  4

struct FireballParticle
{
    float3 position;
    float size;
    float3 velocity;
    float lifetime;
    float maxLifetime;
    float uvOffset;
    uint active;
    float pad;
};

StructuredBuffer<FireballParticle> gFireballParticles : register(t4);

struct VS_FIREBALL_IN
{
    float3 posL : POSITION;
    float2 uv : TEXCOORD;
};

struct VS_FIREBALL_OUT
{
    float4 posH : SV_POSITION;
    float2 uv : TEXCOORD0;
    float lifeRatio : TEXCOORD1;
};

VS_FIREBALL_OUT VSFireball(VS_FIREBALL_IN input, uint instanceID : SV_InstanceID)
{
    VS_FIREBALL_OUT output;

    FireballParticle p = gFireballParticles[instanceID];

    if (!p.active)
    {
        output.posH = float4(0.0f, 0.0f, 2.0f, 1.0f);
        output.uv = float2(0.0f, 0.0f);
        output.lifeRatio = 0.0f;
        return output;
    }

    float3 camRight = normalize(float3(gmtxView[0][0], gmtxView[1][0], gmtxView[2][0]));
    float3 camUp = normalize(float3(gmtxView[0][1], gmtxView[1][1], gmtxView[2][1]));

    float3 worldPos = p.position
                    + camRight * input.posL.x * p.size
                    + camUp * input.posL.y * p.size;

    float4 posV = mul(float4(worldPos, 1.0f), gmtxView);
    output.posH = mul(posV, gmtxProjection);

    float frameF = fmod(p.uvOffset * (SPRITE_COLS * SPRITE_ROWS), (float) (SPRITE_COLS * SPRITE_ROWS));
    uint frameI = (uint) frameF % (SPRITE_COLS * SPRITE_ROWS);
    uint col = frameI % SPRITE_COLS;
    uint row = frameI / SPRITE_COLS;

    float cellW = 1.0f / SPRITE_COLS;
    float cellH = 1.0f / SPRITE_ROWS;

    float2 spriteUV;
    spriteUV.x = ((float) col + input.uv.x) * cellW;
    spriteUV.y = ((float) row + input.uv.y) * cellH;

    spriteUV.x += sin(input.uv.y * 6.0f + p.uvOffset * 3.14f) * 0.015f;

    output.uv = spriteUV;
    output.lifeRatio = saturate(p.lifetime / p.maxLifetime);

    return output;
}

float4 PSFireball(VS_FIREBALL_OUT input) : SV_TARGET
{
    float4 texColor = gtxtTexture.Sample(gssWrap, input.uv);
    float life = input.lifeRatio;

    float3 cYoung = float3(1.0f, 0.95f, 0.55f);
    float3 cMid = float3(1.0f, 0.40f, 0.05f);
    float3 cOld = float3(0.55f, 0.05f, 0.0f);

    float3 fireColor = lerp(cYoung, cMid, saturate(life * 2.0f));
    fireColor = lerp(fireColor, cOld, saturate((life - 0.5f) * 2.0f));
    texColor.rgb *= fireColor;

    float2 center = input.uv - float2(0.5f, 0.5f);

    float2 cellUV = frac(input.uv * float2(SPRITE_COLS, SPRITE_ROWS));
    float2 c2 = cellUV - float2(0.5f, 0.5f);
    float edgeFade = 1.0f - smoothstep(0.35f, 0.5f, length(c2));

    float lifeFade = 1.0f - smoothstep(0.55f, 1.0f, life);

    texColor.a *= edgeFade * lifeFade;

    clip(texColor.a - 0.01f);

    return texColor;
}

float4 PSGreenSpirit(VS_FIREBALL_OUT input) : SV_TARGET
{
    float4 texColor = gtxtTexture.Sample(gssWrap, input.uv);
    float life = input.lifeRatio;

    float2 cellUV = frac(input.uv * float2(SPRITE_COLS, SPRITE_ROWS));
    float2 c2 = cellUV - float2(0.5f, 0.5f);
    float edgeFade = 1.0f - smoothstep(0.35f, 0.5f, length(c2));
    float lifeFade = 1.0f - smoothstep(0.55f, 1.0f, life);
    texColor.a *= edgeFade * lifeFade;

    clip(texColor.a - 0.01f);

    float3 cYoung = float3(0.1f, 1.5f, 0.2f); // 밝은 초록
    float3 cMid = float3(0.05f, 0.9f, 0.1f); // 중간 초록
    float3 cOld = float3(0.0f, 0.4f, 0.05f); // 어두운 초록

    float3 spiritColor = lerp(cYoung, cMid, saturate(life * 2.0f));
    spiritColor = lerp(spiritColor, cOld, saturate((life - 0.5f) * 2.0f));
    texColor.rgb *= spiritColor;

    return texColor;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct VS_GROUNDCRACK_INPUT
{
    float3 position : POSITION;
    float2 uv : TEXCOORD;
    float alpha : COLOR; 
};

struct VS_GROUNDCRACK_OUTPUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    float alpha : COLOR;
};

VS_GROUNDCRACK_OUTPUT VSGroundCrack(VS_GROUNDCRACK_INPUT input)
{
    VS_GROUNDCRACK_OUTPUT output;

    float4 posW = float4(input.position, 1.0f);
    output.position = mul(mul(posW, gmtxView), gmtxProjection);
    output.uv = input.uv;
    output.alpha = input.alpha;

    return output;
}

float4 PSGroundCrack(VS_GROUNDCRACK_OUTPUT input) : SV_TARGET
{
    float centerFactor = 1.0f - abs(input.uv.y * 2.0f - 1.0f);
    centerFactor = pow(abs(centerFactor), 0.6f);
    
    float tipFade = 1.0f - input.uv.x * 0.75f;

    float alpha = centerFactor * tipFade * input.alpha;

    float3 coreColor = float3(1.2f, 0.55f, 0.08f);
    float3 edgeColor = float3(0.08f, 0.05f, 0.02f);
    float3 crackColor = lerp(edgeColor, coreColor, centerFactor);

    crackColor *= tipFade;

    clip(alpha - 0.005f);

    return float4(crackColor, alpha);
}