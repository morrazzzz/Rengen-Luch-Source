#include "stdafx.h"
#pragma hdrstop

#include "ResourceManager.h"
#include "blenders\blender.h"


void CResourceManager::OnDeviceDestroy(BOOL )
{
	if (RDEVICE.b_is_Ready)
		return;

	m_textures_description.UnLoad();

	// Matrices
	for (map_Matrix::iterator m = m_matrices.begin(); m != m_matrices.end(); m++)
	{
		R_ASSERT(1 == m->second->dwReference);
		xr_delete(m->second);
	}

	m_matrices.clear();
    
	// Constants
	for (map_Constant::iterator c = m_constants.begin(); c != m_constants.end(); c++)
	{
		R_ASSERT(1 == c->second->dwReference);
		xr_delete(c->second);
	}

	m_constants.clear();

   	// Release blenders
	for (map_BlenderIt b = m_blenders.begin(); b != m_blenders.end(); b++)
	{
		xr_free((char*&)b->first);
		IBlender::Destroy(b->second);
	}

	m_blenders.clear();

	// destroy TD
	for (map_TDIt _t = m_td.begin(); _t != m_td.end(); _t++)
	{
		xr_free((char*&)_t->first);
		xr_free((char*&)_t->second.T);

		xr_delete(_t->second.cs);
	}

	m_td.clear();

	// scripting
	LS_Unload();
}

void CResourceManager::OnDeviceCreate(IReader* F)
{
	if (!RDEVICE.b_is_Ready)
		return;

	Msg("* RM: Reading shaders xr...");

	string256 name;

	// scripting
	LS_Load();

	IReader* fs = 0;
	// Load constants
 	fs = F->open_chunk(0);

	if (fs)
	{
		while (!fs->eof())
		{
			fs->r_stringZ(name, sizeof(name));
			CConstant*	C = _CreateConstant(name);
			C->Load(fs);
		}

		fs->close();
	}

	// Load matrices
	fs = F->open_chunk(1);

	if (fs)
	{
		while (!fs->eof())
		{
			fs->r_stringZ(name, sizeof(name));
			CMatrix* M = _CreateMatrix(name);

			M->Load(fs);
		}

		fs->close();
	}

	// Load blenders
	fs = F->open_chunk(2);

	if (fs)
	{
		IReader* chunk = NULL;
		int chunk_id = 0;

		while ((chunk = fs->open_chunk(chunk_id)) != NULL)
		{
			CBlender_DESC desc;
			chunk->r(&desc, sizeof(desc));
			IBlender* B = IBlender::Create(desc.CLS);

			if (!B)
			{
				//				Msg				("! Renderer doesn't support blender '%s'",desc.cName);
			}
			else
			{
				if (B->getDescription().version != desc.version)
				{
					//					Msg			("! Version conflict in shader '%s'",desc.cName);
				}

				chunk->seek(0);
				B->Load(*chunk, desc.version);

				std::pair<map_BlenderIt, bool> I = m_blenders.insert(mk_pair(xr_strdup(desc.cName), B));

				R_ASSERT2(I.second, "shader.xr - found duplicate name!!!");
			}

			chunk->close();
			chunk_id += 1;
		}

		fs->close();
	}

	m_textures_description.Load();
}

void CResourceManager::OnDeviceCreate(LPCSTR shName)
{
	// Check if file is compressed already
	string32 ID = "shENGINE";
	string32 id;
	IReader* F = FS.r_open(shName);

	R_ASSERT2(F, shName);

	F->r(&id, 8);

	if (0 == strncmp(id, ID, 8))
	{
		FATAL("Unsupported blender library. Compressed?");
	}

	OnDeviceCreate(F);

	FS.r_close(F);
}

void CResourceManager::StoreNecessaryTextures()
{
	if (!m_necessary.empty())
		return;

	map_TextureIt it = m_textures.begin();
	map_TextureIt it_e = m_textures.end();

	for (; it != it_e; ++it)
	{
		LPCSTR texture_name = it->first;

		if (strstr(texture_name, "\\levels\\"))
			continue;

		if (!strchr(texture_name, '\\'))
			continue;

		ref_texture T;

		T.create(texture_name);
		m_necessary.push_back(T);

	}
}

void CResourceManager::DestroyNecessaryTextures()
{
	m_necessary.clear();
}

void CResourceManager::_DeleteTexture(CTexture*& T)
{
	protectTexturesToDelete_.Enter();

	VERIFY(T);

	if (!T->alreadyInDestroyQueue_)
	{
		texturesToDelete_.push_back(T);

		T->PreDelete();
		T->alreadyInDestroyQueue_ = true;
	}

	protectTexturesToDelete_.Leave();

	T = nullptr; // should do it, in case further code is using this pointer!
}

struct SRemoveText
{
	bool operator() (const CTexture* texture) const
	{
		if (!texture)
			return true;

		return false;
	}
};

u32 CResourceManager::DeleteTextureQueue(bool forced)
{
	if (forced)
		Msg("* Deleting textutres");

	u32 cnt = 0;

	protectTexturesToDelete_.Enter();

	for (u32 i = 0; i < texturesToDelete_.size(); i++)
	{
		if (!forced)
		{
			if (texturesToDelete_[i] && !texturesToDelete_[i]->GetIsProcessingMT()) // don't touch textures, that are mt loading now
			{
				texturesToDelete_[i]->SetReadyToDestroy(true); // set flag for assertion check
				xr_delete(texturesToDelete_[i]);

				cnt++;
			}
		}
		else
		{
			if (texturesToDelete_[i])
			{
				// We are forced to delete all textures, so need to wait if for some reason mt is loading texture now
				while (texturesToDelete_[i]->GetIsProcessingMT())
					Sleep(1);

				texturesToDelete_[i]->SetReadyToDestroy(true); // set flag for assertion check
				xr_delete(texturesToDelete_[i]);

				cnt++;
			}
		}
	}

	// Now delete nulled pointers
	texturesToDelete_.erase(std::remove_if(texturesToDelete_.begin(), texturesToDelete_.end(), SRemoveText()), texturesToDelete_.end());

	protectTexturesToDelete_.Leave();

	if (forced)
		Msg("* Deleted textutres cnt: %u", cnt);

#ifdef DEBUG
	if(cnt)
		Msg("* Post frame textutres deleting: %u textures deleted", cnt);
#endif

	return cnt;
}

