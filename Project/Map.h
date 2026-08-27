#pragma once

#include "Object.h"
#include "QuadTree.h"

class CInstancedStandardShader;

struct MapObjectInstance
{
    int modelIndex;           // m_vLoadedModelInfo에서 참조할 모델의 인덱스 (모델 종류에 따라 인덱싱)
    std::string objectName;
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 rotation;
    DirectX::XMFLOAT3 scale;
    DirectX::XMFLOAT4 quaternion;
    float transformMatrix[16];

    MapObjectInstance(int idx, std::string name, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 rot, DirectX::XMFLOAT3 scl, DirectX::XMFLOAT4 quat, float matrix[16])
        : modelIndex(idx), objectName(name), position(pos), rotation(rot), scale(scl), quaternion(quat)
    {
        std::copy(matrix, matrix + 16, transformMatrix);
    }
};

// GPU로 전송될 순수 데이터
struct VS_INSTANCE_DATA
{
    DirectX::XMFLOAT4X4 worldMatrix;
};

// 맵에서 관리할 인스턴스 그룹 구조체
struct InstanceGroup
{
    CGameObject* pModel;                           // 원본 모델
    UINT nInstances;                               // 그릴 개수
    std::vector<VS_INSTANCE_DATA> vInstanceData;   // CPU에 모아둔 행렬들

    ID3D12Resource* pInstanceBuffer;               // GPU 메모리에 올라간 실제 버퍼 (완성품)
    D3D12_VERTEX_BUFFER_VIEW instanceBufferView;   // GPU에게 이 버퍼를 설명해주는 명세서

    std::vector<ColliderInfo> vWorldColliders;  // 인스턴스별 월드 ColliderInfo 캐시
};

class Map : public CGameObject
{
public:
	Map(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature);
	virtual ~Map();

    void ReleaseUploadBuffers() override;

	void LoadMapObjectsFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature);
	void LoadGeometryFromFile(const std::string& filePath);
    void ReloadInstances(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, const std::string& setterFilePath);
	void SetInstanceData();

    string ReadString(ifstream& inFile);

    void BuildInstanceBuffers(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);

	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = NULL);
    //virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = NULL, UINT n);

	void BuildWorldBoundingBoxes();
	//float GetHeight(float x, float z) const;

    void ReleaseInstanceBuffers(); // 인스턴스 버퍼만 따로 해제하는 헬퍼

public:
    CInstancedStandardShader* m_pInstancedShader = NULL;

	std::vector<MapObjectInstance> m_vObjectInstances;
	std::vector<CGameObject*> m_vLoadedModelInfo;
    std::map<int, InstanceGroup> m_mInstanceGroups;
    std::vector<ID3D12Resource*> m_vUploadBuffers; // 임시 보관소
};