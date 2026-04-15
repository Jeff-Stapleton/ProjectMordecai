// Project Mordecai — Weapon Data Asset (US-024)

#include "Mordecai/Weapons/MordecaiWeaponDataAsset.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiWeaponDataAsset)

FPrimaryAssetId UMordecaiWeaponDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("MordecaiWeapon"), GetFName());
}
