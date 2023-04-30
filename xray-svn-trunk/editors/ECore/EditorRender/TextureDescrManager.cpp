#include "stdafx.h"
#pragma hdrstop
#include "TextureDescrManager.h"
#include "ETextureParams.h"

// eye-params
float r__dtex_range			= 50;


class cl_dt_scaler : public R_constant_setup
{
public:
	float				scale;

	cl_dt_scaler		(float s) : scale(s)	{};

	virtual void setup	(R_constant* C)
	{
		RCache.set_c	(C, scale, scale, scale, 1 / r__dtex_range);
	}
};

void fix_texture_thm_name(LPSTR fn)
{
	LPSTR _ext = strext(fn);

	if(_ext &&
	  ( 0 == stricmp(_ext, ".tga") ||
		0 == stricmp(_ext, ".thm") ||
		0 == stricmp(_ext, ".dds") ||
		0 == stricmp(_ext, ".bmp") ||
		0 == stricmp(_ext, ".ogm")))
		*_ext = 0;
}


void CTextureDescrMngr::LoadLTX()
{
	string_path fname;		
	FS.update_path(fname, "$game_textures$", "textures_associations.ltx");

	Msg(LINE_SPACER);
	Msg("* Reading Textures:LTX");

	if (FS.exist(fname))
	{
		CInifile ini(fname);

		if (ini.section_exist("association"))
		{
			CInifile::Sect& data	= ini.r_section("association");
			CInifile::SectCIt I		= data.Data.begin();
			CInifile::SectCIt E		= data.Data.end();

			for ( ; I != E; ++I)
			{
				const CInifile::Item& item = *I;

				texture_desc& texture_desc		= m_texture_details[item.first];
				cl_dt_scaler*& detais_scaler	= m_detail_scalers[item.first];

				texture_desc.m_assoc = xr_new<texture_assoc>();

				string_path T;
				float s;

				int res = sscanf(*item.second, "%[^,],%f", T, &s);

				R_ASSERT(res == 2);

				texture_desc.m_assoc->detail_name = T;

				if (detais_scaler)
					detais_scaler->scale = s;
				else
					detais_scaler = xr_new<cl_dt_scaler>(s);

				texture_desc.m_assoc->usage = 0;

				if(strstr(item.second.c_str(), "usage[diffuse_or_bump]"))
					texture_desc.m_assoc->usage = (1 << 0) | (1 << 1);
				else
				if(strstr(item.second.c_str(), "usage[bump]"))
					texture_desc.m_assoc->usage = (1 << 1);
				else
				if(strstr(item.second.c_str(), "usage[diffuse]"))
					texture_desc.m_assoc->usage = (1 << 0);

				texture_desc.m_assoc->m_tesselation_method = 32;

				if (strstr(item.second.c_str(), "tessellation_method[0]"))
					texture_desc.m_assoc->m_tesselation_method = 0;

				if (strstr(item.second.c_str(), "tessellation_method[1]"))
					texture_desc.m_assoc->m_tesselation_method = 1;

				if (strstr(item.second.c_str(), "tessellation_method[2]"))
					texture_desc.m_assoc->m_tesselation_method = 2;

				if (strstr(item.second.c_str(), "tessellation_method[3]"))
					texture_desc.m_assoc->m_tesselation_method = 3;
			}
		}
	}


	FS.update_path (fname, "$game_textures$", "textures_specifications.ltx");

	if (FS.exist(fname))
	{
		CInifile ini(fname);

		if (ini.section_exist("specification"))
		{
			CInifile::Sect& sect = ini.r_section("specification");

			for (CInifile::SectCIt I2 = sect.Data.begin(); I2 != sect.Data.end(); ++I2)	
			{
				const CInifile::Item& item = *I2;

				float gcoef = 1.0;	// поумолчанию коэфициент, чтобы глосс не пропал вообще
				float goffset = 0.0;

				texture_desc& texture_desc = m_texture_details[item.first];
				texture_desc.m_spec = xr_new <texture_spec>();

				cl_gloss_coef_and_offset*&	glossparams = texture_desc.m_spec->textureglossparams;

				string_path bmode;

				int res = sscanf(item.second.c_str(), "bump_mode[%[^]]], material[%f], gloss_coef[%f], gloss_offset[%f]", bmode, &texture_desc.m_spec->m_material, &gcoef, &goffset);
				R_ASSERT(res >= 2);

				// regular bump
				if ((bmode[0] == 'u') && (bmode[1] == 's') && (bmode[2] == 'e') && (bmode[3] == ':'))
				{
					// bump-map specified
					texture_desc.m_spec->m_bump_name = bmode + 4;
				}

				// parallaxed bump
				if ((bmode[0] == 'u') && (bmode[1] == 's') && (bmode[2] == 'e') && (bmode[3] == '_') && (bmode[4] == 'p') && (bmode[5] == ':'))
				{
					// bump-map specified with parallax
					texture_desc.m_spec->m_bump_name = bmode + 6;
					texture_desc.m_spec->m_use_steep_parallax = true;
				}

				if (glossparams)
				{
					glossparams->coef = gcoef;
					glossparams->offset = goffset;
				}
				else
					glossparams = xr_new <cl_gloss_coef_and_offset>(gcoef, goffset);
			}
		}
	}

#ifdef _EDITOR
	FS.update_path(fname, "$game_textures$", "textures_types.ltx");

	if (FS.exist(fname))
	{
		CInifile ini(fname);

		if (ini.section_exist("types"))
		{
			CInifile::Sect& data = ini.r_section("types");

			for (CInifile::SectCIt I = data.Data.begin(); I != data.Data.end(); I++)	
			{
				CInifile::Item& item	= *I;

				texture_desc& desc		= m_texture_details[item.first];
				desc.m_type				= (u16)atoi(item.second.c_str());
			}
		}
	}
#endif
}


void CTextureDescrMngr::LoadTHM()
{
	FS_FileSet				flist;
	FS.file_list			(flist, "$game_textures$", FS_ListFiles, "*.thm");

	Msg(LINE_SPACER);
	Msg("* Reading Textures:THM: Count of .thm files: = %u", flist.size());

	FS_FileSetIt It			= flist.begin();
	FS_FileSetIt It_e		= flist.end();

	STextureParams			texture_params;
	string_path				fn;

	for(; It != It_e; ++It)
	{
		FS.update_path(fn, "$game_textures$", (*It).name.c_str());

		IReader* F = FS.r_open(fn);

		xr_strcpy(fn, (*It).name.c_str());

		fix_texture_thm_name(fn);

		R_ASSERT(F->find_chunk(THM_CHUNK_TYPE));

		F->r_u32();

		texture_params.Clear();
		texture_params.Load(*F);

		FS.r_close(F);

#ifdef _EDITOR
		texture_desc& texture_desc = m_texture_details[fn];
		texture_desc.m_type	= texture_params.type;
#endif

		if (STextureParams::ttImage == texture_params.type || STextureParams::ttTerrain == texture_params.type || STextureParams::ttNormalMap == texture_params.type)
		{

#ifndef _EDITOR
		texture_desc& texture_desc = m_texture_details[fn];
#endif
			cl_dt_scaler*&	dts = m_detail_scalers[fn];

			if (texture_params.detail_name.size() && texture_params.flags.is_any(STextureParams::flDiffuseDetail | STextureParams::flBumpDetail))
			{
				u8 bumpdiffuse_ltx = 0;
				shared_str detail_name_ltx;

				if (texture_desc.m_assoc)
				{
					bumpdiffuse_ltx = texture_desc.m_assoc->usage;
					detail_name_ltx = texture_desc.m_assoc->detail_name;

					xr_delete(texture_desc.m_assoc);
				}

				texture_desc.m_assoc = xr_new <texture_assoc>();
				texture_desc.m_assoc->detail_name = texture_params.detail_name;

				if (dts)
					dts->scale = texture_params.detail_scale;
				else
					dts = xr_new <cl_dt_scaler>(texture_params.detail_scale);

				texture_desc.m_assoc->usage = 0;

				if (texture_params.flags.is(STextureParams::flDiffuseDetail))
					texture_desc.m_assoc->usage |= (1 << 0);

				if (texture_params.flags.is(STextureParams::flBumpDetail))
					texture_desc.m_assoc->usage |= (1 << 1);

				if (bumpdiffuse_ltx != 0)
				{
					texture_desc.m_assoc->usage = bumpdiffuse_ltx;
					Msg("BumpDiffuse Flag is assigned in LTX, skipping THM (%s) BumpDiffuse Flag setting", fn);
				}

				if (detail_name_ltx.size() > 0)
				{
					texture_desc.m_assoc->detail_name = detail_name_ltx;
					Msg("Detail Name is assigned in LTX, skipping THM (%s) Detail Name setting", fn);
				}
			}

			bool use_p_ltx = false;
			shared_str bump_name_ltx;
			float material_ltx = -1.0;

			if (texture_desc.m_spec)
			{
				use_p_ltx = texture_desc.m_spec->m_use_steep_parallax;
				bump_name_ltx = texture_desc.m_spec->m_bump_name;
				material_ltx = texture_desc.m_spec->m_material;

				xr_delete(texture_desc.m_spec);
			}

			texture_desc.m_spec = xr_new <texture_spec>();
			texture_desc.m_spec->m_material = texture_params.material + texture_params.material_weight;

			texture_desc.m_spec->m_use_steep_parallax = false;

			if (texture_params.bump_mode == STextureParams::tbmUse)
			{
				texture_desc.m_spec->m_bump_name = texture_params.bump_name;
			}
			else if (texture_params.bump_mode == STextureParams::tbmUseParallax)
			{
				texture_desc.m_spec->m_bump_name = texture_params.bump_name;
				texture_desc.m_spec->m_use_steep_parallax = true;
			}

			if (material_ltx != -1.0)
			{
				texture_desc.m_spec->m_material = material_ltx;
				Msg("Material is assigned in LTX, skipping THM (%s) Material setting", fn);
			}

			if (use_p_ltx){//tatarinrafa: if already true from LTX, dont overwrite
				texture_desc.m_spec->m_use_steep_parallax = use_p_ltx;
				Msg("Parallax is assigned true in LTX, skipping THM (%s) Parallax setting", fn);
			}

			if (bump_name_ltx.size()>0){
				texture_desc.m_spec->m_bump_name = bump_name_ltx;
				Msg("Bump Name is assigned in LTX, skipping THM (%s) Bump Name setting", fn);
			}
		}
	}
}

void CTextureDescrMngr::CheckAndCreate_Assoc(texture_desc*& desc)
{
	// Create description, if needed
	if (!desc->m_assoc)
		desc->m_assoc = xr_new<texture_assoc>();
}

void CTextureDescrMngr::CheckAndCreate_Spec(texture_desc*& desc)
{
	// Create specificator, if needed
	if (!desc->m_spec)
		desc->m_spec = xr_new<texture_spec>();
}

void CTextureDescrMngr::LoadMiniLTX()
{
	FS_FileSet				flist;
	FS.file_list(flist, "$game_textures$", FS_ListFiles, "*.ltx");

	Msg(LINE_SPACER);
	Msg("* Reading Textures:MINI LTX: Count of .ltx files in Textures: = %u", flist.size());

	FS_FileSetIt It = flist.begin();
	FS_FileSetIt It_e = flist.end();

	STextureParams			texture_params;
	string_path				fn;
	string_path				stored_ltx_path;

	for (; It != It_e; ++It)
	{
		FS.update_path(fn, "$game_textures$", (*It).name.c_str());

		xr_strcpy(stored_ltx_path, fn);

		// check if there is a corresponding dds file
		LPSTR _ext = strext(fn);
		if (_ext)
			*_ext = 0;

#ifndef _EDITOR
		xr_sprintf(fn, "%s.dds", fn);
#else
		sprintf(fn, "%s.dds", fn);
#endif

		if (FS.exist(fn))
		{
			// remove the extension
			xr_strcpy(fn, (*It).name.c_str());
			_ext = strext(fn);

			if (_ext)
				*_ext = 0;

			texture_desc* texture_desc = &m_texture_details[fn];

			R_ASSERT(texture_desc);

			// Load the min LTX cfg
			R_ASSERT2(FS.exist(stored_ltx_path), stored_ltx_path);
			CInifile texture_ltx(stored_ltx_path);

			string128 config_section = "config";

			if (texture_ltx.section_exist(config_section)) // Check the section existance
			{

				//THE PARAMETRES

				if (texture_ltx.line_exist(config_section, "detail_texture"))
				{
					CheckAndCreate_Assoc(texture_desc);

					texture_desc->m_assoc->detail_name = texture_ltx.r_string(config_section, "detail_texture");
				}


				if (texture_ltx.line_exist(config_section, "detail_texture_scaler"))
				{
					cl_dt_scaler*& detais_scaler = m_detail_scalers[fn];

					if (!detais_scaler)
						detais_scaler = xr_new<cl_dt_scaler>(1.f);

					R_ASSERT(m_detail_scalers[fn]);

					detais_scaler->scale = texture_ltx.r_float(config_section, "detail_texture_scaler");
				}


				if (texture_ltx.line_exist(config_section, "usage"))
				{
					CheckAndCreate_Assoc(texture_desc);

					LPCSTR ussage_type = texture_ltx.r_string(config_section, "usage");

					if (xr_strcmp(ussage_type, "diffuse_or_bump") == 0)
						texture_desc->m_assoc->usage = (1 << 0) | (1 << 1);
					else

						if (xr_strcmp(ussage_type, "bump") == 0)
							texture_desc->m_assoc->usage = (1 << 1);
						else

							if (xr_strcmp(ussage_type, "diffuse") == 0)
								texture_desc->m_assoc->usage = (1 << 0);

					R_ASSERT2((texture_desc->m_assoc->usage & (1 << 0) || texture_desc->m_assoc->usage & (1 << 1)), make_string("Variable 'usage' is specified but not set in mini ltx of texture %s", fn));
					
				}


				if (texture_ltx.line_exist(config_section, "tessellation_method"))
				{
					CheckAndCreate_Assoc(texture_desc);

					LPCSTR ussage_type = texture_ltx.r_string(config_section, "tessellation_method");

					if (xr_strcmp(ussage_type, "none") == 0)
						texture_desc->m_assoc->m_tesselation_method = 0;
					else

						if (xr_strcmp(ussage_type, "hight_map") == 0)
							texture_desc->m_assoc->m_tesselation_method = 1;
						else

							if (xr_strcmp(ussage_type, "smoothing") == 0)
								texture_desc->m_assoc->m_tesselation_method = 2;
							else

								if (xr_strcmp(ussage_type, "both") == 0)
									texture_desc->m_assoc->m_tesselation_method = 3;
				}


				if (texture_ltx.line_exist(config_section, "bump_texture"))
				{
					CheckAndCreate_Spec(texture_desc);

					texture_desc->m_spec->m_bump_name = texture_ltx.r_string(config_section, "bump_texture");
				}


				if (texture_ltx.line_exist(config_section, "parallax"))
				{
					CheckAndCreate_Spec(texture_desc);

					BOOL parallax = texture_ltx.r_bool(config_section, "parallax");
					texture_desc->m_spec->m_use_steep_parallax = (parallax == TRUE) ? true : false;
				}

				if (texture_ltx.line_exist(config_section, "material_type"))
				{
					CheckAndCreate_Spec(texture_desc);

					LPCSTR mat_type = texture_ltx.r_string(config_section, "material_type");

					if (xr_strcmp(mat_type, "orennayar_blin") == 0)
						texture_desc->m_spec->m_material += 0;
					else

						if (xr_strcmp(mat_type, "blin_phong") == 0)
							texture_desc->m_spec->m_material += 1;
						else

							if (xr_strcmp(mat_type, "phong_metal") == 0)
								texture_desc->m_spec->m_material += 2;
							else

								if (xr_strcmp(mat_type, "metal_orennayar") == 0)
									texture_desc->m_spec->m_material += 3;
				}

				if (texture_ltx.line_exist(config_section, "material_weight"))
				{
					CheckAndCreate_Spec(texture_desc);

					texture_desc->m_spec->m_material += texture_ltx.r_float(config_section, "material_weight");
				}

				if (texture_ltx.line_exist(config_section, "gloss_coef") || texture_ltx.line_exist(config_section, "gloss_offset"))
				{
					CheckAndCreate_Spec(texture_desc);

					cl_gloss_coef_and_offset*&	glossparams = texture_desc->m_spec->textureglossparams;

					if (!glossparams)
						glossparams = xr_new <cl_gloss_coef_and_offset>(1.0f, 0.0f);

					R_ASSERT(texture_desc->m_spec->textureglossparams);

					if (texture_ltx.line_exist(config_section, "gloss_coef"))
						glossparams->coef = texture_ltx.r_float(config_section, "gloss_coef");

					if (texture_ltx.line_exist(config_section, "gloss_offset"))
						glossparams->offset = texture_ltx.r_float(config_section, "gloss_offset");

				}

				//Msg("%s %f %u %u %s %d %f %f %f", texture_desc->m_assoc->detail_name.c_str(), m_detail_scalers[fn] ? m_detail_scalers[fn]->scale : 0.0f, texture_desc->m_assoc->usage,
				//	texture_desc->m_assoc->m_tesselation_method, texture_desc->m_spec->m_bump_name.c_str(), texture_desc->m_spec->m_use_steep_parallax, texture_desc->m_spec->m_material,
				//	texture_desc->m_spec->textureglossparams ? texture_desc->m_spec->textureglossparams->coef : 0.f, texture_desc->m_spec->textureglossparams ? texture_desc->m_spec->textureglossparams->offset : 0.f);

			}
		}
	}
}


void CTextureDescrMngr::Load()
{

#ifdef DEBUG
	CTimer					TT;
	TT.Start				();
#endif

	LockTxDecsMutex();

	LoadLTX					();
	LoadTHM					();
	LoadMiniLTX				();

	UnLockTxDecsMutex();
#ifdef DEBUG
	Msg("load time=%d ms",TT.GetElapsed_ms());
#endif
}


void CTextureDescrMngr::UnLoad()
{
	LockTxDecsMutex();

	map_TD::iterator I = m_texture_details.begin();
	map_TD::iterator E = m_texture_details.end();

	for(; I != E; ++I)
	{
		xr_delete(I->second.m_assoc);
		xr_delete(I->second.m_spec);
	}

	m_texture_details.clear	();

	UnLockTxDecsMutex();
}

CTextureDescrMngr::~CTextureDescrMngr()
{
	map_CS::iterator I = m_detail_scalers.begin();
	map_CS::iterator E = m_detail_scalers.end();

	for(; I != E; ++I)
		xr_delete(I->second);

	m_detail_scalers.clear();
}

shared_str CTextureDescrMngr::GetBumpName(const shared_str& tex_name) const
{
	map_TD::const_iterator I = FindInTxDetailsPool(tex_name);

	if (I != GetTxDetailsPoolEndIter())
	{
		if(I->second.m_spec)
		{
			return I->second.m_spec->m_bump_name;
		}	
	}

	return "";
}

BOOL CTextureDescrMngr::UseSteepParallax(const shared_str& tex_name) const
{
	map_TD::const_iterator I = FindInTxDetailsPool(tex_name);

	if (I != GetTxDetailsPoolEndIter())
	{
		if(I->second.m_spec)
		{
			return I->second.m_spec->m_use_steep_parallax;
		}	
	}

	return FALSE;
}

void CTextureDescrMngr::GetGlossParams(const shared_str& tex_name, R_constant_setup* &GlossParams) const
{
	map_TD::const_iterator I = FindInTxDetailsPool(tex_name);

	if (I != GetTxDetailsPoolEndIter())
	{
		if (I->second.m_spec)
		{
			GlossParams = I->second.m_spec->textureglossparams;
		}
	}
}

u8 CTextureDescrMngr::TessMethod(const shared_str& tex_name) const
{
	map_TD::const_iterator I = FindInTxDetailsPool(tex_name);

	if (I != GetTxDetailsPoolEndIter())
	{
		if (I->second.m_assoc)
		{
			return I->second.m_assoc->m_tesselation_method;
		}
	}

	return 32;
}

float CTextureDescrMngr::GetMaterial(const shared_str& tex_name) const
{
	map_TD::const_iterator I = FindInTxDetailsPool(tex_name);

	if (I != GetTxDetailsPoolEndIter())
	{
		if(I->second.m_spec)
		{
			return I->second.m_spec->m_material;
		}
	}

	return 1.0f;
}

void CTextureDescrMngr::GetTextureUsage(const shared_str& tex_name, BOOL& bDiffuse, BOOL& bBump) const
{
	map_TD::const_iterator I = FindInTxDetailsPool(tex_name);

	if (I != GetTxDetailsPoolEndIter())
	{
		if(I->second.m_assoc)
		{
			u8 usage	= I->second.m_assoc->usage;
			bDiffuse	= !!(usage & (1<<0));
			bBump		= !!(usage & (1<<1));
		}	
	}
}

BOOL CTextureDescrMngr::GetDetailTexture(const shared_str& tex_name, LPCSTR& res, R_constant_setup* &CS) const
{
	map_TD::const_iterator I = FindInTxDetailsPool(tex_name);

	if (I != GetTxDetailsPoolEndIter())
	{
		if(I->second.m_assoc)
		{
            texture_assoc* TA = I->second.m_assoc;
			res	= TA->detail_name.c_str();

			map_CS::const_iterator It2 = m_detail_scalers.find(tex_name);
			CS	= It2==m_detail_scalers.end()?0:It2->second;//TA->cs;

			return TRUE;
		}
	}

	return FALSE;
}

#ifdef _EDITOR
u32 CTextureDescrMngr::GetTextureType(const shared_str& tex_name) const
{
	map_TD::const_iterator I = m_texture_details.find(tex_name);
	if (I != m_texture_details.end())
	{
		return I->second.m_type;
	}
	return u32(-1);
}
#endif

IC CTextureDescrMngr::map_TD::const_iterator CTextureDescrMngr::FindInTxDetailsPool(const shared_str& tex_name) const
{
	protectTxDescPool_.Enter();
	map_TD::const_iterator iter = m_texture_details.find(tex_name);
	protectTxDescPool_.Leave();

	return iter;
}

IC CTextureDescrMngr::map_TD::const_iterator CTextureDescrMngr::GetTxDetailsPoolEndIter() const
{
	protectTxDescPool_.Enter();
	map_TD::const_iterator iter = m_texture_details.end();
	protectTxDescPool_.Leave();

	return iter;
}

IC void CTextureDescrMngr::LockTxDecsMutex()
{
	protectTxDescPool_.Enter();
}

IC void CTextureDescrMngr::UnLockTxDecsMutex()
{
	protectTxDescPool_.Leave();
}

