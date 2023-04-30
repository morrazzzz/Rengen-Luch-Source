#include "stdafx.h"
#include "dx10BufferUtils.h"

namespace dx10BufferUtils
{

HRESULT	IC CreateBuffer( ID3DBuffer** ppBuffer, const void* pData, UINT DataSize, bool bImmutable, bool bIndexBuffer)
{
	D3D_BUFFER_DESC desc;
	desc.ByteWidth = DataSize;
	//desc.Usage = bImmutable ? D3D_USAGE_IMMUTABLE : D3D_USAGE_DEFAULT;
	desc.Usage = D3D_USAGE_DEFAULT;
	desc.BindFlags = bIndexBuffer ? D3D_BIND_INDEX_BUFFER : D3D_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	D3D_SUBRESOURCE_DATA subData;
	subData.pSysMem = pData;

	HRESULT res = HW.pDevice->CreateBuffer( &desc, &subData, ppBuffer);
	R_CHK(res);
	return res;
}

HRESULT	CreateVertexBuffer( ID3DVertexBuffer** ppBuffer, const void* pData, UINT DataSize, bool bImmutable)
{
	return CreateBuffer( ppBuffer, pData, DataSize, bImmutable, false);
}

HRESULT	CreateIndexBuffer( ID3DIndexBuffer** ppBuffer, const void* pData, UINT DataSize, bool bImmutable)
{
	return CreateBuffer( ppBuffer, pData, DataSize, bImmutable, true);
} 

HRESULT	CreateConstantBuffer( ID3DBuffer** ppBuffer, UINT DataSize)
{
	D3D_BUFFER_DESC desc;
	desc.ByteWidth = DataSize;
	desc.Usage = D3D_USAGE_DYNAMIC;
	desc.BindFlags = D3D_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = D3D_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;

	HRESULT res = HW.pDevice->CreateBuffer( &desc, 0, ppBuffer);
	R_CHK(res);
	return res;
}

struct VertexFormatPairs
{
	D3DDECLTYPE	m_dx9FMT;
	DXGI_FORMAT	m_dx10FMT;
};

VertexFormatPairs	VertexFormatList[] = 
{
	{ D3DDECLTYPE_FLOAT1,	DXGI_FORMAT_R32_FLOAT },
	{ D3DDECLTYPE_FLOAT2,	DXGI_FORMAT_R32G32_FLOAT },
	{ D3DDECLTYPE_FLOAT3,	DXGI_FORMAT_R32G32B32_FLOAT },
	{ D3DDECLTYPE_FLOAT4,	DXGI_FORMAT_R32G32B32A32_FLOAT },
	{ D3DDECLTYPE_D3DCOLOR,	DXGI_FORMAT_R8G8B8A8_UNORM },	// Warning. Explicit RGB component swizzling is nesessary	//	Not available 
	{ D3DDECLTYPE_UBYTE4,	DXGI_FORMAT_R8G8B8A8_UINT },	// Note: Shader gets UINT values, but if Direct3D 9 style integral floats are needed (0.0f, 1.0f... 255.f), UINT can just be converted to float32 in shader. 
	{ D3DDECLTYPE_SHORT2,	DXGI_FORMAT_R16G16_SINT },		// Note: Shader gets SINT values, but if Direct3D 9 style integral floats are needed, SINT can just be converted to float32 in shader. 
	{ D3DDECLTYPE_SHORT4,	DXGI_FORMAT_R16G16B16A16_SINT },// Note: Shader gets SINT values, but if Direct3D 9 style integral floats are needed, SINT can just be converted to float32 in shader. 
	{ D3DDECLTYPE_UBYTE4N,	DXGI_FORMAT_R8G8B8A8_UNORM },
	{ D3DDECLTYPE_SHORT2N,	DXGI_FORMAT_R16G16_SNORM },
	{ D3DDECLTYPE_SHORT4N,	DXGI_FORMAT_R16G16B16A16_SNORM },
	{ D3DDECLTYPE_USHORT2N,	DXGI_FORMAT_R16G16_UNORM },
	{ D3DDECLTYPE_USHORT4N,	DXGI_FORMAT_R16G16B16A16_UNORM },
	{ D3DDECLTYPE_UDEC3,    DXGI_FORMAT_R10G10B10A2_UINT },
	{ D3DDECLTYPE_DEC3N,    DXGI_FORMAT_R10G10B10A2_UNORM },
	{ D3DDECLTYPE_FLOAT16_2,DXGI_FORMAT_R16G16_FLOAT },
	{ D3DDECLTYPE_FLOAT16_4,DXGI_FORMAT_R16G16B16A16_FLOAT },
	{ D3DDECLTYPE_UNUSED,   DXGI_FORMAT_UNKNOWN }
};

DXGI_FORMAT	ConvertVertexFormat(D3DDECLTYPE dx9FMT)
{
	int arrayLength = sizeof(VertexFormatList)/sizeof(VertexFormatList[0]);
	for (int i=0; i<arrayLength; ++i)
	{
		if (VertexFormatList[i].m_dx9FMT==dx9FMT)
			return VertexFormatList[i].m_dx10FMT;
	}

	VERIFY(!"ConvertVertexFormat didn't find appropriate dx10 vertex format!");
	return DXGI_FORMAT_UNKNOWN;
}

struct VertexSemanticPairs
{
	D3DDECLUSAGE	m_dx9Semantic;
	LPCSTR			m_dx10Semantic;
};

VertexSemanticPairs	VertexSemanticList[] = 
{
	{ D3DDECLUSAGE_POSITION, "POSITION"},
	{ D3DDECLUSAGE_BLENDWEIGHT, "BLENDWEIGHT"},
	{ D3DDECLUSAGE_BLENDINDICES, "BLENDINDICES"},
	{ D3DDECLUSAGE_NORMAL, "NORMAL"},
	{ D3DDECLUSAGE_PSIZE, "PSIZE"},
	{ D3DDECLUSAGE_TEXCOORD, "TEXCOORD"},
	{ D3DDECLUSAGE_TANGENT, "TANGENT"},
	{ D3DDECLUSAGE_BINORMAL, "BINORMAL"},
	{ D3DDECLUSAGE_TESSFACTOR, "TESSFACTOR"},
	{ D3DDECLUSAGE_POSITIONT, "POSITIONT"},
	{ D3DDECLUSAGE_COLOR, "COLOR"},
	{ D3DDECLUSAGE_FOG, "FOG"},
	{ D3DDECLUSAGE_DEPTH, "DEPTH"},
	{ D3DDECLUSAGE_SAMPLE, "SAMPLE"}
};

LPCSTR	ConvertSemantic(D3DDECLUSAGE Semantic)
{
	int arrayLength = sizeof(VertexSemanticList)/sizeof(VertexSemanticList[0]);
	for (int i=0; i<arrayLength; ++i)
	{
		if (VertexSemanticList[i].m_dx9Semantic==Semantic)
			return VertexSemanticList[i].m_dx10Semantic;
	}

	VERIFY(!"ConvertSemantic didn't find appropriate dx10 input semantic!");
	return 0;
}

#ifdef USE_DX11
void ConvertVertexDeclaration( const xr_vector<D3DVERTEXELEMENT9> &declIn, xr_vector<D3D11_INPUT_ELEMENT_DESC> &declOut)
{
	int iDeclSize = declIn.size()-1;
	declOut.resize(iDeclSize+1);

	for (int i=0; i<iDeclSize; ++i)
	{
		const D3DVERTEXELEMENT9		&descIn = declIn[i];
		D3D11_INPUT_ELEMENT_DESC		&descOut = declOut[i];
		
		descOut.SemanticName = ConvertSemantic((D3DDECLUSAGE)descIn.Usage);
		descOut.SemanticIndex = descIn.UsageIndex;
		descOut.Format = ConvertVertexFormat((D3DDECLTYPE)descIn.Type);
		descOut.InputSlot = descIn.Stream;
		descOut.AlignedByteOffset = descIn.Offset;
		descOut.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		descOut.InstanceDataStepRate = 0;
	}

	ZeroMemory( &declOut[iDeclSize], sizeof(declOut[iDeclSize]) );
}

#else //USE_DX11

void ConvertVertexDeclaration( const xr_vector<D3DVERTEXELEMENT9> &declIn, xr_vector<D3D10_INPUT_ELEMENT_DESC> &declOut)
{
	int iDeclSize = declIn.size()-1;
	declOut.resize(iDeclSize+1);

	for (int i=0; i<iDeclSize; ++i)
	{
		const D3DVERTEXELEMENT9		&descIn = declIn[i];
		D3D10_INPUT_ELEMENT_DESC		&descOut = declOut[i];
		
		descOut.SemanticName = ConvertSemantic((D3DDECLUSAGE)descIn.Usage);
		descOut.SemanticIndex = descIn.UsageIndex;
		descOut.Format = ConvertVertexFormat((D3DDECLTYPE)descIn.Type);
		descOut.InputSlot = descIn.Stream;
		descOut.AlignedByteOffset = descIn.Offset;
		descOut.InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;
		descOut.InstanceDataStepRate = 0;
	}

	ZeroMemory( &declOut[iDeclSize], sizeof(declOut[iDeclSize]) );
}

#endif //USE_DX11
};
