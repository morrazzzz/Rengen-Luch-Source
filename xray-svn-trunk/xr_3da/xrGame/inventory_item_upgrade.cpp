////////////////////////////////////////////////////////////////////////////
//	Module 		: inventory_item_upgrade.cpp
//	Created 	: 27.11.2007
//  Modified 	: 27.11.2007
//	Author		: Sokolov Evgeniy
//	Description : Inventory item upgrades class impl
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "inventory_item.h"
#include "object_broker.h"

#include "ai_space.h"
#include "alife_simulator.h"
#include "inventory_upgrade_manager.h"
#include "inventory_upgrade.h"
#include "Level.h"
#include "WeaponMagazinedWGrenade.h"
#include "alife_object_registry.h"

bool CInventoryItem::has_upgrade_group( const shared_str& upgrade_group_id )
{
	Upgrades_type::iterator it	= m_upgrades.begin();
	Upgrades_type::iterator it_e	= m_upgrades.end();

	for(; it!=it_e; ++it)
	{
		inventory::upgrade::Upgrade* upgrade = ai().alife().inventory_upgrade_manager().get_upgrade( *it );
		if(upgrade->parent_group_id()==upgrade_group_id)
			return true;
	}
	return false;
}

bool CInventoryItem::has_upgrade( const shared_str& upgrade_id )
{
	if ( m_section_id == upgrade_id )
	{
		return true;
	}
	return ( std::find( m_upgrades.begin(), m_upgrades.end(), upgrade_id ) != m_upgrades.end() );
}

void CInventoryItem::add_upgrade( const shared_str& upgrade_id, bool loading )
{
	if ( !has_upgrade( upgrade_id ) )
	{
		m_upgrades.push_back( upgrade_id );

		if (!loading)
		{
			CSE_ALifeDynamicObject* se_obj = Alife()->objects().object(object().ID());
			CSE_ALifeInventoryItem* se_item = smart_cast<CSE_ALifeInventoryItem*>(se_obj);

			se_item->add_upgrade(upgrade_id);
		}
	}
}

bool CInventoryItem::get_upgrades_str( string2048& res ) const
{
	int prop_count = 0;
	res[0] = 0;
	Upgrades_type::const_iterator ib = m_upgrades.begin();
	Upgrades_type::const_iterator ie = m_upgrades.end();
	inventory::upgrade::Upgrade* upgr;
	for ( ; ib != ie; ++ib )
	{
		upgr = ai().alife().inventory_upgrade_manager().get_upgrade( *ib );
		if ( !upgr ) { continue; }

		LPCSTR upgr_section = upgr->section();
		if ( prop_count > 0 )
		{
			xr_strcat( res, sizeof(res), ", " );
		}
		xr_strcat( res, sizeof(res), upgr_section );
		++prop_count;
	}
	if ( prop_count > 0 )
	{
		return true;
	}
	return false;
}

bool CInventoryItem::equal_upgrades( Upgrades_type const& other_upgrades ) const
{
	if ( m_upgrades.size() != other_upgrades.size() )
	{
		return false;
	}
	
	Upgrades_type::const_iterator ib = m_upgrades.begin();
	Upgrades_type::const_iterator ie = m_upgrades.end();
	for ( ; ib != ie; ++ib )
	{
		shared_str const& name1 = (*ib);
		bool upg_equal = false;
		Upgrades_type::const_iterator ib2 = other_upgrades.begin();
		Upgrades_type::const_iterator ie2 = other_upgrades.end();
		for ( ; ib2 != ie2; ++ib2 )
		{
			if ( name1.equal( (*ib2) ) )
			{
				upg_equal = true;
				break;//from for2, in for1
			}
		}
		if ( !upg_equal )
		{
			return false;
		}
	}
	return true;
}

#ifdef DEBUG	
void CInventoryItem::log_upgrades()
{
	Msg( "* all upgrades of item = %s", m_section_id.c_str() );
	Upgrades_type::const_iterator ib = m_upgrades.begin();
	Upgrades_type::const_iterator ie = m_upgrades.end();
	for ( ; ib != ie; ++ib )
	{
		Msg( "    %s", (*ib).c_str() );
	}
	Msg( "* finish - upgrades of item = %s", m_section_id.c_str() );
}
#endif // DEBUG

void CInventoryItem::ImportInstalledUbgrades( Upgrades_type saved_upgrades ) // SpawnAndImportSOData
{
	m_upgrades.clear_not_free();

	if ( !ai().get_alife() )
	{
		return;
	}
	
	ai().alife().inventory_upgrade_manager().init_install( *this ); // from pSettings

	Upgrades_type::iterator ib = saved_upgrades.begin();
	Upgrades_type::iterator ie = saved_upgrades.end();
	for ( ; ib != ie ; ++ib )
	{
		ai().alife().inventory_upgrade_manager().upgrade_install( *this, (*ib), true );
	}
}

bool CInventoryItem::install_upgrade( LPCSTR section )
{
	return install_upgrade_impl( section, false );
}

bool CInventoryItem::verify_upgrade( LPCSTR section )
{
	return install_upgrade_impl( section, true );
}

static float multiply_control_inertion(float base, float mult)
{
	return (base - 1.f) * mult;
}

bool CInventoryItem::install_upgrade_impl( LPCSTR section, bool test )
{
	bool result = process_if_exists( section, "cost",   &CInifile::r_u32,   m_cost,   test );
	result |= process_if_exists( section, "inv_weight", &CInifile::r_float, m_weight, test );

	bool result2 = false;
//	if ( BaseSlot() != NO_ACTIVE_SLOT )
	{
		BOOL value = m_flags.test( FRuckDefault );
		result2 = process_if_exists_set( section, "default_to_ruck", &CInifile::r_bool, value, test );
		if ( result2 && !test )
		{
			m_flags.set( FRuckDefault, value );
		}
		result |= result2;

		value = m_flags.test( FAllowSprint );
		result2 = process_if_exists_set( section, "sprint_allowed", &CInifile::r_bool, value, test );
		if ( result2 && !test )
		{
			m_flags.set( FAllowSprint, value );
		}
		result |= result2;

		result |= process_if_exists( section, "control_inertion_factor", m_fControlInertionFactor, test, nullptr, multiply_control_inertion);
	}

	LPCSTR str;
	result2 = process_if_exists_set( section, "immunities_sect", &CInifile::r_string, str, test );
	if ( result2 && !test )
		CHitImmunity::LoadImmunities( str, pSettings );

	result2 = process_if_exists_set( section, "immunities_sect_add", &CInifile::r_string, str, test );
	if ( result2 && !test )
		CHitImmunity::AddImmunities( str, pSettings );

	return result;
}

void CInventoryItem::pre_install_upgrade()
{
	CWeaponMagazined* wm = smart_cast<CWeaponMagazined*>( this );
	if ( wm )
	{
		wm->UnloadMagazine();

		CWeaponMagazinedWGrenade* wg = smart_cast<CWeaponMagazinedWGrenade*>( this );
		if ( wg )
		{
			if ( wg->IsGrenadeLauncherAttached() ) 
			{
				wg->PerformSwitchGL();
				wg->UnloadMagazine();
				wg->PerformSwitchGL(); // restore state
			}
		}
	}

	CWeapon* weapon = smart_cast<CWeapon*>( this );
	if ( weapon )
	{
		if ( weapon->ScopeAttachable() && weapon->IsScopeAttached() )
		{
			weapon->Detach( weapon->GetScopeName().c_str(), true );
		}
		if ( weapon->SilencerAttachable() && weapon->IsSilencerAttached() )
		{
			weapon->Detach( weapon->GetSilencerName().c_str(), true );
		}
		if ( weapon->GrenadeLauncherAttachable() && weapon->IsGrenadeLauncherAttached() )
		{
			weapon->Detach( weapon->GetGrenadeLauncherName().c_str(), true );
		}
	}


}

//template <>
//IC bool CInventoryItem::process_if_exists(LPCSTR section, LPCSTR name, float(CInifile::*method)(LPCSTR, LPCSTR) const, float& value, bool test)
//{
//	return process_if_exists(section, name, value, test, nullptr, nullptr);
//}

bool CInventoryItem::process_if_exists(LPCSTR section, LPCSTR name, float& value, bool test, float(*getter)(float), float(*multiply)(float,float))
{
	bool result = false;
	float delta = 0.f;
	string128 name_mult;
	strconcat(sizeof(name_mult), name_mult, name, "@mult");
	if (pSettings->line_exist(section, name_mult) && pSettings->line_exist(m_section_id, name))
	{
		if (!test)
		{
			float base = pSettings->r_float(m_section_id, name);
			float mult = pSettings->r_float(section, name_mult);
			float mult2 = READ_IF_EXISTS(pSettings, r_float, section, "mult", 1.f);

			delta = multiply
				? multiply(base, mult * mult2)
				: base * mult * mult2;

			// Msg("Upgrade %s: %s. Base: %.2f, mult: %.2f, mult2: %.2f, delta: %.2f", section, name, base, mult, mult2, delta);
		}
		result = true;
	}
	else if (pSettings->line_exist(section, name))
	{
		LPCSTR str = pSettings->r_string(section, name);
		if (str && xr_strlen(str))
		{
			if (!test)
			{
				delta = pSettings->r_float(section, name); // add
			}
			result = true;
		}
	}

	if (!test && result)
	{
		value += getter
			? getter(delta)
			: delta;
	}
	return result;
}
