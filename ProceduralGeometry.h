#pragma once
#include <cstdint>
#include <DirectXMath.h>
#include <vector>

class ProceduralGeometry
{
public:

	using uint16 = std::uint16_t;
	using uint32 = std::uint32_t;

	//һ������ṹ��
	struct Vertex
	{
		Vertex() {};
		Vertex(
			const DirectX::XMFLOAT3& p,
			const DirectX::XMFLOAT3& n,
			const DirectX::XMFLOAT3& t,
			const DirectX::XMFLOAT2& uv) :
			Position(p),
			Normal(n),
			TangentU(t),
			TexC(uv) {};
		Vertex(
			float px, float py, float pz,
			float nx, float ny, float nz,
			float tx, float ty, float tz,
			float u, float v) :
			Position(px, py, pz),
			Normal(nx, ny, nz),
			TangentU(tx, ty, tz),
			TexC(u, v) {};

		DirectX::XMFLOAT3 Position;			//#�������꣨����ռ䣩
		DirectX::XMFLOAT3 Normal;			//#���ߣ�����ռ䣩
		DirectX::XMFLOAT3 TangentU;			//#���ߣ�����ռ䣩
		DirectX::XMFLOAT2 TexC;				//#��������
	};

	//�������������װ
	struct MeshData
	{
		std::vector<Vertex> Vertices;	//����������Ķ�������
		std::vector<uint32> Indices32;	//�������������������

		//��32λ�������б�ת����16λ
		std::vector<uint16>& GetIndices16()
		{
			if (mIndices16.empty())
			{
				mIndices16.resize(Indices32.size());
				for (size_t i = 0; i < Indices32.size(); i++)
				{
					mIndices16[i] = static_cast<uint16>(Indices32.at(i));
				}
			}
			return mIndices16;
		}

	private:
		std::vector<uint16> mIndices16;
	};

	//���빹��һ��������
	MeshData CreateBox(float width, float height, float depth, uint32 numSubdivisions);
	//����
	MeshData CreateSphere(float radius, uint32 sliceCount, uint32 stackCount);
	//���Ǳ�������
	MeshData CreateGeosphere(float radius, uint32 numSubdivisions);
	//Բ����
	MeshData CreateCylinder(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount);
	//����
	MeshData CreateGrid(float width, float depth, uint32 m, uint32 n);
	//�ı��Σ�
	MeshData CreateQuad(float x, float y, float w, float h, float depth);


	//������֧����
	void Subdivide(MeshData& meshData);
	//��������е�
	Vertex MidPoint(const Vertex& v0, const Vertex& v1);
	//
	void BuildCylinderTopCap(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount, MeshData& meshData);
	void BuildCylinderBottomCap(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount, MeshData& meshData);
};
