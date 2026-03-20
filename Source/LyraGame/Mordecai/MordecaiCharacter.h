// Project Mordecai

#pragma once

#include "Character/LyraCharacter.h"

#include "MordecaiCharacter.generated.h"

class UArrowComponent;

/**
 * AMordecaiCharacter
 *
 *	Player character for Project Mordecai.
 *	Twin-stick controls: left stick moves, right stick/mouse aims.
 *	Fixed diorama camera — character never rotates with camera.
 */
UCLASS()
class LYRAGAME_API AMordecaiCharacter : public ALyraCharacter
{
	GENERATED_BODY()

public:
	AMordecaiCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Movement tuning (AC-2.1.6)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Mordecai|Movement")
	float WalkSpeed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Mordecai|Movement")
	float SprintSpeedMultiplier;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Mordecai|Movement")
	float GroundAcceleration;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Mordecai|Movement")
	float GroundDeceleration;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Movement")
	bool bIsSprinting;

	UFUNCTION(BlueprintCallable, Category = "Mordecai|Movement")
	void SetSprinting(bool bNewSprinting);

	UFUNCTION(BlueprintCallable, Category = "Mordecai|Character")
	UArrowComponent* GetFacingArrow() const { return FacingArrowComponent; }

private:
	// Facing direction indicator (AC-2.1.5)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mordecai|Character", Meta = (AllowPrivateAccess = true))
	TObjectPtr<UArrowComponent> FacingArrowComponent;
};
