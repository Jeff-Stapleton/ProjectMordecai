// Project Mordecai — Heavy Attack Gameplay Ability (US-083)

#include "Mordecai/Combat/MordecaiGA_HeavyAttack.h"
#include "Mordecai/MordecaiGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiGA_HeavyAttack)

UMordecaiGA_HeavyAttack::UMordecaiGA_HeavyAttack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AttackSlot = EMordecaiInputSlot::Heavy;

	// Marks the attack as heavy while it runs (the tag Weakened's heavy bypass is specified against, MordecaiGE_Weakened.h)
	ActivationOwnedTags.AddTag(MordecaiGameplayTags::Attack_Heavy);
}
