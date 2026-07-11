// Project Mordecai — Item Library (US-032)

#include "Mordecai/Items/MordecaiItemLibrary.h"
#include "Mordecai/Items/MordecaiItemDefinition.h"
#include "Mordecai/MordecaiGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiItemLibrary)

namespace
{
	/** Lower = sorts earlier. Matches AC-032.14 ordering. */
	int32 ItemTypeSortRank(EMordecaiItemType Type)
	{
		switch (Type)
		{
		case EMordecaiItemType::Weapon:        return 0;
		case EMordecaiItemType::Armor:         return 1;
		case EMordecaiItemType::Trinket:       return 2;
		case EMordecaiItemType::MagicalItem:   return 3;
		case EMordecaiItemType::UpgradeKey:    return 4;
		case EMordecaiItemType::QuestItem:     return 5;
		case EMordecaiItemType::Consumable:    return 6;
		case EMordecaiItemType::Material:      return 7;
		case EMordecaiItemType::TownResource:  return 7; // tied with Material per spec
		case EMordecaiItemType::CurrencyProxy: return 8;
		default:                               return 99;
		}
	}
}

int32 UMordecaiItemLibrary::CompareSortPriority(const UMordecaiItemDefinition* A, const UMordecaiItemDefinition* B)
{
	if (A == B) return 0;
	if (!A) return 1;   // nulls sort last
	if (!B) return -1;

	// Higher SortPriority sorts first → invert
	const int32 APri = static_cast<int32>(A->SortPriority);
	const int32 BPri = static_cast<int32>(B->SortPriority);
	if (APri != BPri)
	{
		return BPri - APri;
	}

	const int32 ARank = ItemTypeSortRank(A->ItemType);
	const int32 BRank = ItemTypeSortRank(B->ItemType);
	if (ARank != BRank)
	{
		return ARank - BRank;
	}

	return A->ItemId.Compare(B->ItemId);
}

bool UMordecaiItemLibrary::GetTypeDefaultAutoStore(EMordecaiItemType Type)
{
	return Type == EMordecaiItemType::Material || Type == EMordecaiItemType::TownResource;
}

FGameplayTag UMordecaiItemLibrary::GetTypeTag(EMordecaiItemType Type)
{
	switch (Type)
	{
	case EMordecaiItemType::Weapon:       return MordecaiGameplayTags::Item_Type_Weapon;
	case EMordecaiItemType::Armor:        return MordecaiGameplayTags::Item_Type_Armor;
	case EMordecaiItemType::Trinket:      return MordecaiGameplayTags::Item_Type_Trinket;
	case EMordecaiItemType::Consumable:   return MordecaiGameplayTags::Item_Type_Consumable;
	case EMordecaiItemType::Material:     return MordecaiGameplayTags::Item_Type_Material;
	case EMordecaiItemType::TownResource: return MordecaiGameplayTags::Item_Type_TownResource;
	case EMordecaiItemType::UpgradeKey:   return MordecaiGameplayTags::Item_Type_UpgradeKey;
	case EMordecaiItemType::QuestItem:    return MordecaiGameplayTags::Item_Type_QuestItem;
	case EMordecaiItemType::MagicalItem:  return MordecaiGameplayTags::Item_Type_MagicalItem;
	default:                              return FGameplayTag();
	}
}

FGameplayTag UMordecaiItemLibrary::GetRarityTag(EMordecaiItemRarity Rarity)
{
	switch (Rarity)
	{
	case EMordecaiItemRarity::Common: return MordecaiGameplayTags::Item_Rarity_Common;
	case EMordecaiItemRarity::Green:  return MordecaiGameplayTags::Item_Rarity_Green;
	case EMordecaiItemRarity::Blue:   return MordecaiGameplayTags::Item_Rarity_Blue;
	case EMordecaiItemRarity::Purple: return MordecaiGameplayTags::Item_Rarity_Purple;
	case EMordecaiItemRarity::Red:    return MordecaiGameplayTags::Item_Rarity_Red;
	case EMordecaiItemRarity::Gold:   return MordecaiGameplayTags::Item_Rarity_Gold;
	default:                          return FGameplayTag();
	}
}

// ---------------------------------------------------------------------------
// US-033: Identification helpers
// ---------------------------------------------------------------------------

namespace
{
	FString RarityDisplayString(EMordecaiItemRarity R)
	{
		switch (R)
		{
		case EMordecaiItemRarity::Common: return TEXT("Common");
		case EMordecaiItemRarity::Green:  return TEXT("Green");
		case EMordecaiItemRarity::Blue:   return TEXT("Blue");
		case EMordecaiItemRarity::Purple: return TEXT("Purple");
		case EMordecaiItemRarity::Red:    return TEXT("Red");
		case EMordecaiItemRarity::Gold:   return TEXT("Gold");
		default:                          return TEXT("");
		}
	}

	FString ItemTypeDisplayString(EMordecaiItemType T)
	{
		switch (T)
		{
		case EMordecaiItemType::Weapon:        return TEXT("Weapon");
		case EMordecaiItemType::Armor:         return TEXT("Armor");
		case EMordecaiItemType::Trinket:       return TEXT("Trinket");
		case EMordecaiItemType::Consumable:    return TEXT("Consumable");
		case EMordecaiItemType::Material:      return TEXT("Material");
		case EMordecaiItemType::TownResource:  return TEXT("Resource");
		case EMordecaiItemType::UpgradeKey:    return TEXT("Upgrade Key");
		case EMordecaiItemType::QuestItem:     return TEXT("Quest Item");
		case EMordecaiItemType::MagicalItem:   return TEXT("Magical Item");
		case EMordecaiItemType::CurrencyProxy: return TEXT("Currency");
		default:                               return TEXT("Item");
		}
	}
}

FText UMordecaiItemLibrary::GetDisplayName(const FMordecaiItemInstance& Instance)
{
	const UMordecaiItemDefinition* Def = Instance.ItemDefinition;
	if (!Def)
	{
		return FText::GetEmpty();
	}

	if (Instance.IsIdentified())
	{
		return Def->DisplayName;
	}

	if (!Def->ShowPartialInfoBeforeIdentify)
	{
		return FText::FromString(TEXT("Unknown Item"));
	}

	const FString RarityLabel = RarityDisplayString(Def->Rarity);
	const FString TypeLabel = Def->Subtype.IsNone()
		? ItemTypeDisplayString(Def->ItemType)
		: Def->Subtype.ToString();

	return FText::FromString(FString::Printf(TEXT("Unidentified %s %s"), *RarityLabel, *TypeLabel));
}

FText UMordecaiItemLibrary::GetDescription(const FMordecaiItemInstance& Instance)
{
	const UMordecaiItemDefinition* Def = Instance.ItemDefinition;
	if (!Def)
	{
		return FText::GetEmpty();
	}

	if (Instance.IsIdentified())
	{
		return Def->Description;
	}

	if (Def->ShowPartialInfoBeforeIdentify)
	{
		return Def->ShortDescription;
	}

	return FText::GetEmpty();
}

FGameplayTagContainer UMordecaiItemLibrary::GetVisibleTags(const FMordecaiItemInstance& Instance)
{
	const UMordecaiItemDefinition* Def = Instance.ItemDefinition;
	if (!Def)
	{
		return FGameplayTagContainer();
	}

	if (Instance.IsIdentified())
	{
		return Def->Tags;
	}

	// Filter: keep only Mordecai.Item.Rarity.* and Mordecai.Item.Type.* tags
	static const FGameplayTag RarityRoot = FGameplayTag::RequestGameplayTag(FName(TEXT("Mordecai.Item.Rarity")), false);
	static const FGameplayTag TypeRoot = FGameplayTag::RequestGameplayTag(FName(TEXT("Mordecai.Item.Type")), false);

	FGameplayTagContainer Result;
	for (const FGameplayTag& Tag : Def->Tags)
	{
		if (Tag.MatchesTag(RarityRoot) || Tag.MatchesTag(TypeRoot))
		{
			Result.AddTag(Tag);
		}
	}
	return Result;
}

bool UMordecaiItemLibrary::CanEquipInstance(const FMordecaiItemInstance& Instance)
{
	const UMordecaiItemDefinition* Def = Instance.ItemDefinition;
	if (!Def || !Def->IsEquippable())
	{
		return false;
	}
	if (Def->UsesIdentification && Def->RequiresIdentificationToEquip && !Instance.IsIdentified())
	{
		return false;
	}
	return true;
}

FGameplayTag UMordecaiItemLibrary::GetIdentificationTag(EMordecaiIdentificationState State)
{
	switch (State)
	{
	case EMordecaiIdentificationState::Identified:   return MordecaiGameplayTags::Item_Identification_Identified;
	case EMordecaiIdentificationState::Unidentified: return MordecaiGameplayTags::Item_Identification_Unidentified;
	default:                                         return FGameplayTag();
	}
}
