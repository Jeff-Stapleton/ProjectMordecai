// Project Mordecai — Snare Spell (US-023)

#pragma once

#include "CoreMinimal.h"
#include "MordecaiGA_SpellBase.h"

#include "MordecaiGA_Snare.generated.h"

/**
 * UMordecaiGA_Snare
 *
 *	Single-target root spell. Delivery: TraceHit (line trace in aim direction
 *	up to Range). On hit, applies a Root GE to the first enemy hit.
 *
 *	While rooted: target cannot move (movement disabled) but CAN still attack
 *	and block. Root tag and movement restriction removed when Duration expires.
 *
 *	See: US-023, AC-023.1 through AC-023.3
 */
UCLASS()
class LYRAGAME_API UMordecaiGA_Snare : public UMordecaiGA_SpellBase
{
	GENERATED_BODY()

public:
	UMordecaiGA_Snare(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// --- Public Getters (testable API) ---

	/** Get snare range from SpellDataAsset (AC-023.1). */
	float GetSnareRange() const;

	/** Get root duration from SpellDataAsset (AC-023.2). */
	float GetSnareDuration() const;

	/** Get the root status tag (AC-023.2). */
	FGameplayTag GetRootTag() const;

	/** Whether rooted targets can still attack (AC-023.2). Always true. */
	bool GetRootAllowsAttack() const;

	/** Whether rooted targets can still block (AC-023.2). Always true. */
	bool GetRootAllowsBlock() const;

protected:
	virtual void OnSpellCast() override;

private:
	/** Apply the root GE to a target actor's ASC. */
	void ApplyRootToTarget(AActor* Target);
};
