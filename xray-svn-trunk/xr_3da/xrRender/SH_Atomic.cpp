#include "stdafx.h"
#pragma hdrstop

#include "sh_atomic.h"
#include "ResourceManager.h"
#include "dxRenderDeviceRender.h"

//	SVS
SVS::SVS() : vs(0)
//, signature(0)
{
	;
}


SVS::~SVS()
{
	DEV->_DeleteVS(this);
	_RELEASE(vs);
}


SPS::~SPS()			{ _RELEASE(ps); DEV->_DeletePS(this); }
SGS::~SGS()			{ _RELEASE(gs); DEV->_DeleteGS(this); }
SHS::~SHS()			{ _RELEASE(sh); DEV->_DeleteHS(this); }
SDS::~SDS()			{ _RELEASE(sh); DEV->_DeleteDS(this); }
SCS::~SCS()			{ _RELEASE(sh); DEV->_DeleteCS(this); }


SInputSignature::SInputSignature		(ID3DBlob* pBlob)	{ VERIFY(pBlob); signature=pBlob; signature->AddRef(); };
SInputSignature::~SInputSignature		()					{ _RELEASE(signature); DEV->_DeleteInputSignature(this); }


SState::~SState							()					{ _RELEASE(state);	DEV->_DeleteState(this);	}


SDeclaration::~SDeclaration()
{	
	DEV->_DeleteDecl(this);	

	xr_map<ID3DBlob*, ID3DInputLayout*>::iterator iLayout;

	iLayout = vs_to_layout.begin();

	for( ; iLayout != vs_to_layout.end(); ++iLayout)
	{
		//	Release vertex layout
		_RELEASE(iLayout->second);
	}
}
