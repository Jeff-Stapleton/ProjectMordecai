// Project Mordecai — Skill Data Asset (US-011)

#include "Mordecai/Skills/MordecaiSkillDataAsset.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiSkillDataAsset)

FPrimaryAssetId UMordecaiSkillDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("MordecaiSkill")), SkillName);
}
