#include "pch_script.h"
#include "InfoPortion.h"
#include "encyclopedia_article.h"
#include "ui\uixmlinit.h"
#include "object_broker.h"

void INFO_DATA::load (IReader& stream) 
{
	load_data(info_id, stream); 
	load_data(receive_time, stream);
}

void INFO_DATA::save (IWriter& stream) 
{
	save_data(info_id, stream); 
	save_data(receive_time, stream);
}


SInfoPortionData::SInfoPortionData ()
{
}
SInfoPortionData::~SInfoPortionData ()
{
}

CInfoPortion::CInfoPortion()
{
}

CInfoPortion::~CInfoPortion ()
{
}

void CInfoPortion::Load	(shared_str info_id)
{
	m_InfoId = info_id;
	inherited_shared::load_shared(m_InfoId, NULL);
}


void CInfoPortion::load_shared(LPCSTR section)
{
	const ITEM_DATA& item_data = *id_to_index::GetById(m_InfoId);

	if (!&item_data) // Создать чистый инфо без доп свойств. (Как в ЗП)
	{
		return;
	}

	CUIXml*		pXML		= item_data._xml;
	pXML->SetLocalRoot		(pXML->GetRoot());

	//loading from XML
	XML_NODE* pNode			= pXML->NavigateToNode(id_to_index::tag_name, item_data.pos_in_file);

	THROW3(pNode, "info_portion id=", *item_data.id);

	//список названий диалогов
	int dialogs_num			= pXML->GetNodesNum(pNode, "dialog");
	info_data()->m_DialogNames.clear();
	for(int i=0; i<dialogs_num; ++i)
	{
		shared_str dialog_name = pXML->Read(pNode, "dialog", i,"");
		info_data()->m_DialogNames.push_back(dialog_name);
	}

	
	//список названий порций информации, которые деактивируются,
	//после получения этой порции
	int disable_num = pXML->GetNodesNum(pNode, "disable");
	info_data()->m_DisableInfo.clear();
	for(int i = 0; i < disable_num; ++i)
	{
		shared_str info_id		= pXML->Read(pNode, "disable", i,"");
		info_data()->m_DisableInfo.push_back(info_id);
	}

	//имена скриптовых функций
	info_data()->m_PhraseScript.Load(pXML, pNode);


	//индексы статей
	info_data()->m_Articles.clear();
	int articles_num	= pXML->GetNodesNum(pNode, "article");
	for(int i = 0; i < articles_num; ++i)
	{
		LPCSTR article_str_id = pXML->Read(pNode, "article", i, NULL);
		THROW(article_str_id);
		info_data()->m_Articles.push_back(article_str_id);
	}

	info_data()->m_ArticlesDisable.clear();
	articles_num = pXML->GetNodesNum(pNode, "article_disable");
	for(int i = 0; i < articles_num; ++i)
	{
		LPCSTR article_str_id = pXML->Read(pNode, "article_disable", i, NULL);
		THROW(article_str_id);
		info_data()->m_ArticlesDisable.push_back(article_str_id);
	}
	
	info_data()->m_GameTasks.clear();
	int task_num = pXML->GetNodesNum(pNode, "task");
	for(int i = 0; i < task_num; ++i)
	{
		LPCSTR task_str_id = pXML->Read(pNode, "task", i, NULL);
		THROW(task_str_id);
		info_data()->m_GameTasks.push_back(task_str_id);
	}

	// имена меток на карте
	info_data()->mapLocations_.clear();
	int map_locs_num = pXML->GetNodesNum(pNode, "map_location");
	for (int i = 0; i < map_locs_num; ++i)
	{
		LPCSTR article_str_id = pXML->Read(pNode, "map_location", i, NULL);
		THROW(article_str_id);
		info_data()->mapLocations_.push_back(article_str_id);
	}
}

void   CInfoPortion::InitXmlIdToIndex()
{
	if(!id_to_index::tag_name)
		id_to_index::tag_name = "info_portion";
	if(!id_to_index::file_str)
		id_to_index::file_str = pSettings->r_string("info_portions", "files");
}

bool CInfoPortion::IsComplexInfoPortion(LPCSTR info_id)
{
	return id_to_index::GetById(info_id, true) != NULL;
}

