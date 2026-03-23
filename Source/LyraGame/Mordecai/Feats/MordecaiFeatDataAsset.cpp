// Project Mordecai — Feat Data Asset (US-012)

#include "MordecaiFeatDataAsset.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiFeatDataAsset)

FPrimaryAssetId UMordecaiFeatDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("MordecaiFeat")), FeatName);
}
