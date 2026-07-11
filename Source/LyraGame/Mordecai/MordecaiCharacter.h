// Project Mordecai

#pragma once

#include "Character/LyraCharacter.h"

#include "MordecaiCharacter.generated.h"

class UArrowComponent;
class ULyraCameraMode;
class UMordecaiHeroComponent;
class UMordecaiEquipmentComponent;
class UMordecaiWeaponDataAsset;
class UMordecaiEquippedWeaponWidget;
class UMordecaiInventoryComponent;
class UMordecaiPickupInteractionComponent;
class UMordecaiResourceLedger;
class UStaticMeshComponent;
struct FOnAttributeChangeData;

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

	UFUNCTION(BlueprintCallable, Category = "Mordecai|Character")
	UStaticMeshComponent* GetPlaceholderBody() const { return PlaceholderBodyComponent; }

	UFUNCTION(BlueprintCallable, Category = "Mordecai|Character")
	UStaticMeshComponent* GetPlaceholderHead() const { return PlaceholderHeadComponent; }

	// --- Death / Respawn (US-053) ---

	/** Handle player death: apply State_Dead, disable movement/input, broadcast Event_PlayerDeath. */
	void HandlePlayerDeath();

	/** Handle player respawn: restore attributes, remove Dead tag, re-enable movement/input. */
	void HandlePlayerRespawn();

	/** Returns true if this character is dead. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Character")
	bool IsDead() const;

	// --- US-078: Weapon System Integration ---

	/** Equipment component for weapon management. Exposed for HeroComponent input wiring. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|Equipment")
	UMordecaiEquipmentComponent* GetEquipmentComponent() const { return EquipmentComponent; }

	// --- US-031: Inventory & Ledger ---

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|Inventory")
	UMordecaiInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|Inventory")
	UMordecaiResourceLedger* GetResourceLedger() const { return ResourceLedger; }

	// --- US-079: Press-to-pickup interaction ---

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|Inventory")
	UMordecaiPickupInteractionComponent* GetPickupInteractionComponent() const { return PickupInteractionComponent; }

	/** Cycle to next weapon in AvailableWeapons (bound to Tab / Gamepad Y via HeroComponent). */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Equipment")
	void CycleNextWeapon();

	/** Cycle to previous weapon in AvailableWeapons (bound to Shift+Tab via HeroComponent). */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Equipment")
	void CyclePrevWeapon();

	/** Designer-configurable starting weapon set. Auto-populated into AvailableWeapons on BeginPlay. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Equipment")
	TArray<TSoftObjectPtr<UMordecaiWeaponDataAsset>> StartingWeapons;

	/** Widget class for the equipped weapon HUD display. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Equipment")
	TSubclassOf<UMordecaiEquippedWeaponWidget> EquippedWeaponWidgetClass;

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	// Hero component — handles input binding and camera mode from PawnData
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mordecai|Character", Meta = (AllowPrivateAccess = true))
	TObjectPtr<UMordecaiHeroComponent> HeroComponent;

	// Equipment component for weapon management (US-078)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mordecai|Equipment", Meta = (AllowPrivateAccess = true))
	TObjectPtr<UMordecaiEquipmentComponent> EquipmentComponent;

	// Inventory (flat-list, unlimited carry) and resource ledger (auto-stored materials) — US-031
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mordecai|Inventory", Meta = (AllowPrivateAccess = true))
	TObjectPtr<UMordecaiInventoryComponent> InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mordecai|Inventory", Meta = (AllowPrivateAccess = true))
	TObjectPtr<UMordecaiResourceLedger> ResourceLedger;

	// Press-to-pickup focus tracking (US-079)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mordecai|Inventory", Meta = (AllowPrivateAccess = true))
	TObjectPtr<UMordecaiPickupInteractionComponent> PickupInteractionComponent;

	// Runtime: equipped-weapon HUD widget (created on BeginPlay for local player)
	UPROPERTY(Transient)
	TObjectPtr<UMordecaiEquippedWeaponWidget> EquippedWeaponWidget;

	void InitializeWeaponLoadout();

	// Facing direction indicator (AC-2.1.5)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mordecai|Character", Meta = (AllowPrivateAccess = true))
	TObjectPtr<UArrowComponent> FacingArrowComponent;

	// Placeholder visual mesh (replace with skeletal mesh when art is ready)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mordecai|Character", Meta = (AllowPrivateAccess = true))
	TObjectPtr<UStaticMeshComponent> PlaceholderBodyComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mordecai|Character", Meta = (AllowPrivateAccess = true))
	TObjectPtr<UStaticMeshComponent> PlaceholderHeadComponent;

	// Diorama camera — always returns MordecaiCameraMode_Diorama
	TSubclassOf<ULyraCameraMode> DetermineDioramaCameraMode() const;

	// Death handling (US-053)
	void BindToASC();
	void OnHealthChanged(const FOnAttributeChangeData& Data);
	bool bDeathHandled = false;
};
