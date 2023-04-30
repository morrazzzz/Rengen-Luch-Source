#include "stdafx.h"

#define LODDING_SUGGESTED_SIZE 128 * 3 * 1024 // in bytes

void GenerateTextureLodList()
{
	Msg(LINE_SPACER);
	Msg("- Generating List of textures for reduced quality option");
	Msg(LINE_SPACER);

	FS_FileSet flist;
	FS.file_list(flist, "$game_textures$", FS_ListFiles, "*.dds");

	FS_FileSetIt It = flist.begin();
	FS_FileSetIt It_e = flist.end();

	u32 reduced_count = 0;
	u32 total_count = flist.size();

	for (; It != It_e; ++It)
	{
		if ((*It).size > LODDING_SUGGESTED_SIZE)
		{
			string_path	fn;
			xr_sprintf(fn, "%s", (*It).name.c_str());

			LPSTR _ext = strext(fn);
			if (_ext)
				*_ext = 0;

			Msg("%s", fn);

			reduced_count++;
		}
	}

	Msg(LINE_SPACER);
	Msg("-- This is the raw list of textures, that are big enough (size is more than %i KB) for putting it in the list of textures", LODDING_SUGGESTED_SIZE / 1024);
	Msg("-- which will have generated reduced qulity at the texture loading stage, if the 'texture quality' option is lower, than maximum.");
	Msg("-- Put this list into section [reduce_lod_texture_list] and adjust it manualy, if needed. Total count of reduced list is = %i. Total count of DDS textures in folder = %i", reduced_count, total_count);
}