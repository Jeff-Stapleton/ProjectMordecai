// Project Mordecai — Item Definition (US-032)

#include "Mordecai/Items/MordecaiItemDefinition.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiItemDefinition)

bool UMordecaiItemDefinition::IsEquippable() const
{
	switch (ItemType)
	{
	case EMordecaiItemType::Weapon:
	case EMordecaiItemType::Armor:
	case EMordecaiItemType::Trinket:
	case EMordecaiItemType::MagicalItem:
		return true;
	default:
		return false;
	}
}
