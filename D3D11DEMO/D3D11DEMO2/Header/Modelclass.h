////////////////////////////////////////////////////////////////////////////////
// Filename: modelclass.h
////////////////////////////////////////////////////////////////////////////////
#pragma once

//////////////
// INCLUDES //
//////////////
#include <d3d11.h>
#include <DirectXMath.h>

///////////////////////
// MY CLASS INCLUDES //
///////////////////////
#include "textureclass.h"
#include "Mydefines.h"


using namespace DirectX;

////////////////////////////////////////////////////////////////////////////////
// Class name: ModelClass
////////////////////////////////////////////////////////////////////////////////
class ModelClass
{
private:
#ifdef TEXTURE_SHADER
	struct VertexType 
	{
		XMFLOAT3 position;
		XMFLOAT2 texture;
	};
#else
	struct VertexType
	{
		XMFLOAT3 position;
		XMFLOAT4 color;
	};
#endif

public:
	ModelClass();
	ModelClass(const ModelClass&);
	~ModelClass();

#ifdef TEXTURE_SHADER
	bool Initialize(ID3D11Device*, ID3D11DeviceContext*, char*);
#else
	bool Initialize(ID3D11Device*);
#endif

	void Shutdown();
	void Render(ID3D11DeviceContext*);

	int GetIndexCount();

#ifdef TEXTURE_SHADER
	ID3D11ShaderResourceView* GetTexture();
#endif

private:
	bool InitializeBuffers(ID3D11Device*);
	void ShutdownBuffers();
	void RenderBuffers(ID3D11DeviceContext*);

#ifdef TEXTURE_SHADER
	bool LoadTexture(ID3D11Device*, ID3D11DeviceContext*, char*);
	void ReleaseTexture();
#endif

private:
	ID3D11Buffer * m_vertexBuffer, *m_indexBuffer;
	int m_vertexCount, m_indexCount;
	 
#ifdef TEXTURE_SHADER
	TextureClass* m_Texture;
#endif
};