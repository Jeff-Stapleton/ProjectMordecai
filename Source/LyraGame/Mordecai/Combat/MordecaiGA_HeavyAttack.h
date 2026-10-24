// Project Mordecai — Heavy Attack Gameplay Ability (US-083)

#pragma once

#include "CoreMinimal.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"

#include "MordecaiGA_HeavyAttack.generated.h"

/**
 * UMordecaiGA_HeavyAttack
 *
 * The player's heavy (charged) melee attack: the melee ability with AttackSlot = Heavy, so it performs the equipped
 * weapon's heavy attack profile (UMordecaiEquipmentComponent::GetActiveHeavyAttackProfile) instead of its light combo.
 * Granted as its own spec so LMB/RT hold and LMB/RT tap can activate different abilities.
 *
 * See: US-083, control_bindings_v1.1.md (Heavy / Charged Attack: RT hold / LMB hold)
 */
UCLASS()
class LYRAGAME_API UMordecaiGA_HeavyAttack : public UMordecaiGA_MeleeAttack
{
	GENERATED_BODY()

public:
	UMordecaiGA_HeavyAttack(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
