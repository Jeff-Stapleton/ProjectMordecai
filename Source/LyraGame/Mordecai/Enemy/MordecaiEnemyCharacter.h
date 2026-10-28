// Project Mordecai — Enemy Character (US-050)

#pragma once

#include "Character/LyraCharacter.h"

#include "MordecaiEnemyCharacter.generated.h"

/** Enemy content and behaviour problems (US-084). Errors here surface in every playtest's log_errors check. */
LYRAGAME_API DECLARE_LOG_CATEGORY_EXTERN(LogMordecaiEnemy, Log, All);

class UMordecaiAbilitySystemComponent;
class UMordecaiAttributeSet;
class UMordecaiAttackProfileDataAsset;
class UMordecaiPostureSystem;
class UMordecaiEnemyHealthBarWidget;
class UMordecaiEnemyIndicatorComponent;
class UStaticMeshComponent;
class UWidgetComponent;
class UGameplayAbility;
struct FOnAttributeChangeData;

/**
 * AMordecaiEnemyCharacter
 *
 * Base enemy character that can receive damage, die, and have its posture broken.
 * Owns its own ASC directly (not on PlayerState like the player character).
 * This is a "training dummy" — AI behavior is added in US-051.
 *
 * See: US-050, enemy_archetypes_v1.md, combat_system_v1.md
 */
UCLASS()
class LYRAGAME_API AMordecaiEnemyCharacter : public ALyraCharacter
{
	GENERATED_BODY()

public:
	AMordecaiEnemyCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// IAbilitySystemInterface — returns the enemy's own ASC (bypasses PawnExtComponent)
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// --- Configurable Base Stats (AC-050.10) ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Enemy")
	float DefaultMaxHealth = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Enemy")
	float DefaultMaxPosture = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Enemy")
	float DefaultMoveSpeed = 400.f;

	/** Abilities to grant when this enemy spawns. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Enemy")
	TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

	/** Attack profiles for the enemy's melee attack (set on instanced ability after granting). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Enemy")
	TArray<TObjectPtr<UMordecaiAttackProfileDataAsset>> EnemyAttackProfiles;

	/** Widget class for the world-space health bar above the enemy's head. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Enemy|UI")
	TSubclassOf<UMordecaiEnemyHealthBarWidget> EnemyHealthBarWidgetClass;

	// --- State Queries ---

	/** Returns true if the enemy has the Mordecai.State.Dead tag. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Enemy")
	bool IsDead() const;

	/** True when a visible skeletal or static mesh component has a mesh asset, so a player can see this enemy. */
	bool HasVisibleMesh() const;

	/**
	 * US-084 (AC-084.3): log a LogMordecaiEnemy error for content a playable enemy needs and lacks: attack
	 * profiles, a visible mesh. US-085 (issue #18): also for base stats that differ from its class's (a placed
	 * instance override). Called from BeginPlay. Returns true when nothing is wrong.
	 */
	bool ValidateContent() const;

	// --- Accessors ---

	UFUNCTION(BlueprintCallable, Category = "Mordecai|Enemy")
	UMordecaiAttributeSet* GetMordecaiAttributeSet() const { return AttributeSet; }

	UMordecaiPostureSystem* GetPostureSystem() const { return PostureSystem; }

	// --- State Transitions ---

	/** Enter death state: apply State_Dead tag, disable movement/collision, broadcast Event_Death.
	 *  Called automatically when Health <= 0. Safe to call externally (e.g., instakill). */
	void EnterDeathState();

	/** Enter posture broken state: apply State_PostureBroken tag, start recovery timer.
	 *  Called automatically when Posture <= 0. */
	void EnterPostureBrokenState();

	/** Exit posture broken state: remove tag, reset posture to MaxPosture. */
	void ExitPostureBrokenState();

	/** Initialize ASC, attributes, tags, and delegates. Idempotent — safe to call multiple times. */
	void InitializeAbilitySystem();

	/** Reset this enemy for arena replay. Restores health/posture, removes death state,
	 *  re-enables collision and movement. Called by GameMode::ResetArena(). */
	void ResetForArena();

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void Tick(float DeltaTime) override;

	// Attribute change handlers (bound to ASC delegates in InitializeAbilitySystem)
	void OnHealthChanged(const FOnAttributeChangeData& Data);
	void OnPostureChanged(const FOnAttributeChangeData& Data);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mordecai|Enemy", Meta = (AllowPrivateAccess = true))
	TObjectPtr<UMordecaiAbilitySystemComponent> EnemyAbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UMordecaiAttributeSet> AttributeSet;

	UPROPERTY()
	TObjectPtr<UMordecaiPostureSystem> PostureSystem;

	UPROPERTY()
	TObjectPtr<UWidgetComponent> HealthBarWidgetComponent;

	/** Enemy indicator component for world-space health/status display (US-064). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mordecai|Enemy", Meta = (AllowPrivateAccess = true))
	TObjectPtr<UMordecaiEnemyIndicatorComponent> EnemyIndicatorComponent;

	/**
	 * The body players see (US-085, issue #19). Empty on the native class: each enemy Blueprint sets its mesh. It never
	 * collides; the capsule alone blocks and takes hits. Hidden while the enemy is dead.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mordecai|Enemy", Meta = (AllowPrivateAccess = true))
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	bool bAbilitySystemInitialized = false;
	FTimerHandle PostureBrokenTimerHandle;

	void GrantDefaultAbilities();
	void CreateHealthBarWidget();
};
