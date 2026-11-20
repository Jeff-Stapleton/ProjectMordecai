// Project Mordecai — Enemy Character (US-050)

#include "Mordecai/Enemy/MordecaiEnemyCharacter.h"

#include "Mordecai/AbilitySystem/MordecaiAbilitySystemComponent.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/Combat/MordecaiPostureSystem.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/Combat/MordecaiAttackProfileDataAsset.h"
#include "Mordecai/Combat/MordecaiHitDetectionSubsystem.h"
#include "Mordecai/UI/MordecaiEnemyIndicatorComponent.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/MordecaiGameMode.h"
#include "Mordecai/Enemy/MordecaiEnemyAIController.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameplayEffectExtension.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiEnemyCharacter)

DEFINE_LOG_CATEGORY(LogMordecaiEnemy);

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

AMordecaiEnemyCharacter::AMordecaiEnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Use our custom AI controller (AC-054.1, AC-054.13)
	AIControllerClass = AMordecaiEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// Enemy owns its own ASC (not on PlayerState like the player)
	EnemyAbilitySystemComponent = CreateDefaultSubobject<UMordecaiAbilitySystemComponent>(TEXT("EnemyASC"));
	EnemyAbilitySystemComponent->SetIsReplicated(true);
	// Minimal mode: tags+cues replicate to all clients, GEs do not (appropriate for AI)
	EnemyAbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	// Attribute set — will be discovered by ASC during InitAbilityActorInfo
	AttributeSet = CreateDefaultSubobject<UMordecaiAttributeSet>(TEXT("AttributeSet"));

	// Posture system for break/regen logic (reuses existing UMordecaiPostureSystem)
	PostureSystem = CreateDefaultSubobject<UMordecaiPostureSystem>(TEXT("PostureSystem"));

	// Enemy indicator component for world-space health/status display (US-064)
	EnemyIndicatorComponent = CreateDefaultSubobject<UMordecaiEnemyIndicatorComponent>(TEXT("EnemyIndicator"));

	// Issue #17: the same temporary green facing indicator as the player. It is purely visual:
	// no collision/overlaps, navigation, ticking or component replication.
	FacingArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("FacingArrow"));
	FacingArrowComponent->SetupAttachment(RootComponent);
	FacingArrowComponent->ArrowColor = FColor::Green;
	FacingArrowComponent->ArrowSize = 1.5f;
	FacingArrowComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));
	FacingArrowComponent->SetHiddenInGame(false);
	FacingArrowComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FacingArrowComponent->SetGenerateOverlapEvents(false);
	FacingArrowComponent->SetCanEverAffectNavigation(false);
	FacingArrowComponent->PrimaryComponentTick.bCanEverTick = false;
	FacingArrowComponent->SetIsReplicated(false);

	// US-085 (issue #19): the body players see. Left empty, so an enemy Blueprint without a body still logs
	// "no visible mesh"; it never collides, so blocking and hits stay the capsule's
	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(GetCapsuleComponent());
	BodyMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	BodyMesh->SetGenerateOverlapEvents(false);
	BodyMesh->SetCanEverAffectNavigation(false);

	// Enable tick for posture regeneration
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

// ---------------------------------------------------------------------------
// IAbilitySystemInterface
// ---------------------------------------------------------------------------

UAbilitySystemComponent* AMordecaiEnemyCharacter::GetAbilitySystemComponent() const
{
	return EnemyAbilitySystemComponent;
}

// ---------------------------------------------------------------------------
// State Queries
// ---------------------------------------------------------------------------

bool AMordecaiEnemyCharacter::IsDead() const
{
	if (EnemyAbilitySystemComponent)
	{
		return EnemyAbilitySystemComponent->HasMatchingGameplayTag(MordecaiGameplayTags::State_Dead);
	}
	return false;
}

bool AMordecaiEnemyCharacter::HasVisibleMesh() const
{
	if (IsHidden())
	{
		return false;
	}

	TInlineComponentArray<UPrimitiveComponent*> Primitives(this);
	for (const UPrimitiveComponent* Primitive : Primitives)
	{
		if (!Primitive || !Primitive->IsVisible() || Primitive->bHiddenInGame)
		{
			continue;
		}
		if (const UStaticMeshComponent* StaticMesh = Cast<UStaticMeshComponent>(Primitive))
		{
			if (StaticMesh->GetStaticMesh())
			{
				return true;
			}
		}
		else if (const USkinnedMeshComponent* Skinned = Cast<USkinnedMeshComponent>(Primitive))
		{
			if (Skinned->GetSkinnedAsset())
			{
				return true;
			}
		}
	}
	return false;
}

bool AMordecaiEnemyCharacter::ValidateContent() const
{
	bool bValid = true;

	// The granted melee instance holds the profiles the attack uses (EnemyAttackProfiles, or the ability's own)
	bool bHasAttackProfile = false;
	if (EnemyAbilitySystemComponent)
	{
		for (const FGameplayAbilitySpec& Spec : EnemyAbilitySystemComponent->GetActivatableAbilities())
		{
			if (const UMordecaiGA_MeleeAttack* Melee = Cast<UMordecaiGA_MeleeAttack>(Spec.GetPrimaryInstance()))
			{
				bHasAttackProfile |= Melee->HasAnyAttackProfile();
			}
		}
	}
	if (!bHasAttackProfile)
	{
		UE_LOG(LogMordecaiEnemy, Error, TEXT("enemy %s has no attack profiles: its melee attack can never hit (set EnemyAttackProfiles on %s)"),
			*GetName(), *GetClass()->GetName());
		bValid = false;
	}

	if (!HasVisibleMesh())
	{
		UE_LOG(LogMordecaiEnemy, Error, TEXT("enemy %s has no visible mesh: players cannot see it (set BodyMesh's mesh on %s)"),
			*GetName(), *GetClass()->GetName());
		bValid = false;
	}

	// US-085 (issue #18): the base stats are EditDefaultsOnly, so only the class sets them. A level that saved another
	// value on a placed enemy overrides its Blueprint without a trace (DevTestMap's Frontliners: 100 vs the BP's 200).
	const AMordecaiEnemyCharacter* ClassDefaults = GetDefault<AMordecaiEnemyCharacter>(GetClass());
	auto CheckClassStat = [&](const TCHAR* StatName, float Value, float ClassValue)
	{
		if (Value != ClassValue)
		{
			UE_LOG(LogMordecaiEnemy, Error, TEXT("enemy %s overrides %s (%.1f on the instance, %.1f on %s): its level saved a stale value on the placed actor"),
				*GetName(), StatName, Value, ClassValue, *GetClass()->GetName());
			bValid = false;
		}
	};
	CheckClassStat(TEXT("DefaultMaxHealth"), DefaultMaxHealth, ClassDefaults->DefaultMaxHealth);
	CheckClassStat(TEXT("DefaultMaxPosture"), DefaultMaxPosture, ClassDefaults->DefaultMaxPosture);
	CheckClassStat(TEXT("DefaultMoveSpeed"), DefaultMoveSpeed, ClassDefaults->DefaultMoveSpeed);

	return bValid;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void AMordecaiEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	InitializeAbilitySystem();

	// US-084: missing content is an error in every log, not a silent enemy (abilities are granted on authority only)
	if (HasAuthority())
	{
		ValidateContent();
	}

	// Register with GameMode for arena reset tracking (AC-054.13)
	if (HasAuthority())
	{
		if (UWorld* World = GetWorld())
		{
			if (AMordecaiGameMode* GM = Cast<AMordecaiGameMode>(World->GetAuthGameMode()))
			{
				GM->RegisterEnemy(this, GetActorTransform());
			}
		}
	}
}

void AMordecaiEnemyCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitializeAbilitySystem();
}

void AMordecaiEnemyCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);
	UMordecaiHitDetectionSubsystem::SyncAirborneTag(EnemyAbilitySystemComponent, this);
}

void AMordecaiEnemyCharacter::InitializeAbilitySystem()
{
	if (bAbilitySystemInitialized || !EnemyAbilitySystemComponent)
	{
		return;
	}

	// Initialize ASC with self as both owner and avatar (enemy ASC lives on the actor)
	EnemyAbilitySystemComponent->InitAbilityActorInfo(this, this);

	// Initialize attributes to configurable defaults (AC-050.3)
	if (AttributeSet)
	{
		AttributeSet->InitHealth(DefaultMaxHealth);
		AttributeSet->InitMaxHealth(DefaultMaxHealth);
		AttributeSet->InitPosture(DefaultMaxPosture);
		AttributeSet->InitMaxPosture(DefaultMaxPosture);
	}

	// Apply enemy team tag at spawn (AC-050.4)
	EnemyAbilitySystemComponent->AddLooseGameplayTag(MordecaiGameplayTags::Team_Enemy);

	// US-101: Airborne from the current movement mode; OnMovementModeChanged keeps it current from here
	UMordecaiHitDetectionSubsystem::SyncAirborneTag(EnemyAbilitySystemComponent, this);

	// Set movement speed from configurable default (AC-050.10)
	if (UCharacterMovementComponent* CMC = GetCharacterMovement())
	{
		CMC->MaxWalkSpeed = DefaultMoveSpeed;
	}

	// Bind attribute change delegates for death and posture break detection
	EnemyAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		UMordecaiAttributeSet::GetHealthAttribute()).AddUObject(
			this, &AMordecaiEnemyCharacter::OnHealthChanged);

	EnemyAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		UMordecaiAttributeSet::GetPostureAttribute()).AddUObject(
			this, &AMordecaiEnemyCharacter::OnPostureChanged);

	// Grant default abilities (US-054)
	GrantDefaultAbilities();

	bAbilitySystemInitialized = true;
}

// ---------------------------------------------------------------------------
// Tick — Posture Regeneration (AC-050.9)
// ---------------------------------------------------------------------------

void AMordecaiEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority() || IsDead() || !EnemyAbilitySystemComponent || !AttributeSet || !PostureSystem)
	{
		return;
	}

	RegeneratePosture(DeltaTime);
}

// ---------------------------------------------------------------------------
// Attribute Change Handlers
// ---------------------------------------------------------------------------

void AMordecaiEnemyCharacter::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	// US-064: Notify indicator component when enemy takes damage (AC-064.2)
	if (Data.NewValue < Data.OldValue && EnemyIndicatorComponent)
	{
		EnemyIndicatorComponent->NotifyDamageReceived();
	}

	// AC-050.6: When Health reaches <= 0, enter death state
	if (Data.NewValue <= 0.f && !IsDead())
	{
		EnterDeathState(FindPlayerKiller(Data));
		return;
	}

	// Count only health actually taken from a player while the authoritative target is staggered.
	if (HasAuthority()
		&& Data.NewValue < Data.OldValue
		&& FindPlayerKiller(Data)
		&& PostureSystem
		&& EnemyAbilitySystemComponent
		&& EnemyAbilitySystemComponent->HasMatchingGameplayTag(
			MordecaiGameplayTags::State_PostureBroken))
	{
		PostureSystem->AccumulateStaggerDamage(Data.OldValue - Data.NewValue);
		if (PostureSystem->ShouldKnockDown())
		{
			EnterKnockDownState();
		}
	}
}

APlayerState* AMordecaiEnemyCharacter::FindPlayerKiller(const FOnAttributeChangeData& Data)
{
	// Set on the server for changes a GameplayEffect makes, periodic ticks included; null for a direct set or a
	// replicated value. Player damage (melee, projectiles, spells, DoTs) uses the context of the player's ASC, which
	// lives on its PlayerState: instigator = the PlayerState, effect causer = the pawn. Enemies have no PlayerState.
	if (!Data.GEModData)
	{
		return nullptr;
	}
	const FGameplayEffectContextHandle& Context = Data.GEModData->EffectSpec.GetEffectContext();
	for (AActor* Actor : { Context.GetOriginalInstigator(), Context.GetEffectCauser() })
	{
		if (APlayerState* PlayerState = Cast<APlayerState>(Actor))
		{
			return PlayerState;
		}
		if (const APawn* Pawn = Cast<APawn>(Actor))
		{
			if (APlayerState* PlayerState = Pawn->GetPlayerState())
			{
				return PlayerState;
			}
		}
	}
	return nullptr;
}

void AMordecaiEnemyCharacter::OnPostureChanged(const FOnAttributeChangeData& Data)
{
	if (!HasAuthority())
	{
		return;
	}

	// Track posture damage for regen delay (AC-050.9)
	if (Data.NewValue < Data.OldValue && PostureSystem)
	{
		if (const UWorld* World = GetWorld())
		{
			PostureSystem->NotifyPostureDamageApplied(World->GetTimeSeconds());
		}
	}

	// AC-050.8: When Posture reaches <= 0, enter posture broken state
	if (Data.NewValue <= 0.f && !IsDead()
		&& EnemyAbilitySystemComponent
		&& !EnemyAbilitySystemComponent->HasMatchingGameplayTag(MordecaiGameplayTags::State_PostureBroken)
		&& !EnemyAbilitySystemComponent->HasMatchingGameplayTag(MordecaiGameplayTags::State_KnockedDown))
	{
		EnterPostureBrokenState();
	}
}

// ---------------------------------------------------------------------------
// State Transitions
// ---------------------------------------------------------------------------

void AMordecaiEnemyCharacter::EnterDeathState(APlayerState* Killer)
{
	// One death per life: a dead enemy is not killed (or counted) again until ResetForArena
	if (!EnemyAbilitySystemComponent || IsDead())
	{
		return;
	}

	ClearPostureRuntimeStates();

	// AC-050.6: Apply State_Dead tag (replicated via ASC, AC-050.11)
	EnemyAbilitySystemComponent->AddLooseGameplayTag(MordecaiGameplayTags::State_Dead);

	// US-064: Unregister indicator on death (AC-064.3)
	if (EnemyIndicatorComponent)
	{
		EnemyIndicatorComponent->NotifyDeath();
	}

	// AC-050.6: Disable movement
	if (UCharacterMovementComponent* CMC = GetCharacterMovement())
	{
		CMC->StopMovementImmediately();
		CMC->SetMovementMode(MOVE_None);
	}

	// AC-050.6: Change collision to NoCollision
	SetActorEnableCollision(false);

	// Issue #19: the body goes with the collision, so no standing enemy is left to walk through.
	// TODO(DECISION): corpse visuals (death animation, ragdoll, fade-out); until then the dead enemy is hidden.
	if (BodyMesh)
	{
		BodyMesh->SetVisibility(false);
	}

	// AC-050.6: Broadcast Event_Death gameplay event
	FGameplayEventData EventData;
	EventData.EventTag = MordecaiGameplayTags::Event_Death;
	EventData.Instigator = this;
	EventData.Target = this;
	EnemyAbilitySystemComponent->HandleGameplayEvent(MordecaiGameplayTags::Event_Death, &EventData);

	// US-053, issue #6: the game mode counts the kill when a player made it
	if (UWorld* World = GetWorld())
	{
		if (AMordecaiGameMode* GM = Cast<AMordecaiGameMode>(World->GetAuthGameMode()))
		{
			GM->NotifyEnemyKilled(this, Killer);
		}
	}
}

void AMordecaiEnemyCharacter::EnterPostureBrokenState()
{
	if (!HasAuthority() || !EnemyAbilitySystemComponent || !PostureSystem)
	{
		return;
	}

	SetPostureRuntimeState(MordecaiGameplayTags::State_KnockedDown, false);
	SetPostureRuntimeState(MordecaiGameplayTags::State_PostureBroken, true);
	PostureSystem->EnterPostureBroken();
	CancelInFlightMelee();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PostureBrokenTimerHandle);
		World->GetTimerManager().SetTimer(PostureBrokenTimerHandle, this,
			&AMordecaiEnemyCharacter::ExitPostureBrokenState,
			PostureSystem->GetPostureBrokenDurationSeconds(), false);
	}
}

void AMordecaiEnemyCharacter::ExitPostureBrokenState()
{
	if (!HasAuthority() || !EnemyAbilitySystemComponent || !PostureSystem || !AttributeSet)
	{
		return;
	}

	SetPostureRuntimeState(MordecaiGameplayTags::State_PostureBroken, false);

	// Reset posture to MaxPosture (AC-050.8, matches UMordecaiPostureSystem pattern)
	const float MaxPosture = AttributeSet->GetMaxPosture();
	PostureSystem->ExitPostureBroken(MaxPosture);
	EnemyAbilitySystemComponent->SetNumericAttributeBase(
		UMordecaiAttributeSet::GetPostureAttribute(), MaxPosture);
}

void AMordecaiEnemyCharacter::EnterKnockDownState()
{
	if (!HasAuthority() || !EnemyAbilitySystemComponent || !PostureSystem)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(PostureBrokenTimerHandle);
	SetPostureRuntimeState(MordecaiGameplayTags::State_KnockedDown, true);
	SetPostureRuntimeState(MordecaiGameplayTags::State_PostureBroken, false);
	PostureSystem->EnterKnockDown();
	CancelInFlightMelee();

	GetWorldTimerManager().ClearTimer(KnockDownTimerHandle);
	GetWorldTimerManager().SetTimer(
		KnockDownTimerHandle,
		this,
		&ThisClass::ExitKnockDownState,
		PostureSystem->GetKnockDownDurationSeconds(),
		false);
}

void AMordecaiEnemyCharacter::ExitKnockDownState()
{
	if (!HasAuthority() || !EnemyAbilitySystemComponent || !AttributeSet || !PostureSystem)
	{
		return;
	}

	SetPostureRuntimeState(MordecaiGameplayTags::State_KnockedDown, false);
	const float MaxPosture = AttributeSet->GetMaxPosture();
	PostureSystem->ExitKnockDown(MaxPosture);
	EnemyAbilitySystemComponent->SetNumericAttributeBase(
		UMordecaiAttributeSet::GetPostureAttribute(), MaxPosture);
}

void AMordecaiEnemyCharacter::EnterParriedState()
{
	if (!HasAuthority() || !EnemyAbilitySystemComponent || !PostureSystem)
	{
		return;
	}

	SetPostureRuntimeState(MordecaiGameplayTags::State_Parried, true);
	CancelInFlightMelee();
	GetWorldTimerManager().ClearTimer(ParriedTimerHandle);
	GetWorldTimerManager().SetTimer(
		ParriedTimerHandle,
		this,
		&ThisClass::ExitParriedState,
		PostureSystem->GetParriedDurationSeconds(),
		false);
}

void AMordecaiEnemyCharacter::ExitParriedState()
{
	SetPostureRuntimeState(MordecaiGameplayTags::State_Parried, false);
}

void AMordecaiEnemyCharacter::SetPostureRuntimeState(const FGameplayTag StateTag, const bool bEnabled)
{
	if (!HasAuthority() || !EnemyAbilitySystemComponent || !StateTag.IsValid())
	{
		return;
	}

	EnemyAbilitySystemComponent->SetLooseGameplayTagCount(
		StateTag,
		bEnabled ? 1 : 0,
		EGameplayTagReplicationState::TagOnly);
}

void AMordecaiEnemyCharacter::CancelInFlightMelee()
{
	if (!HasAuthority() || !EnemyAbilitySystemComponent)
	{
		return;
	}

	for (const FGameplayAbilitySpec& Spec : EnemyAbilitySystemComponent->GetActivatableAbilities())
	{
		if (Spec.IsActive() && Spec.Ability && Spec.Ability->IsA<UMordecaiGA_MeleeAttack>())
		{
			EnemyAbilitySystemComponent->CancelAbilityHandle(Spec.Handle);
		}
	}
}

void AMordecaiEnemyCharacter::RegeneratePosture(const float DeltaTime)
{
	if (!HasAuthority() || !EnemyAbilitySystemComponent || !AttributeSet || !PostureSystem
		|| IsDead()
		|| EnemyAbilitySystemComponent->HasMatchingGameplayTag(MordecaiGameplayTags::State_PostureBroken)
		|| EnemyAbilitySystemComponent->HasMatchingGameplayTag(MordecaiGameplayTags::State_KnockedDown))
	{
		return;
	}

	const float CurrentPosture = AttributeSet->GetPosture();
	const float MaxPosture = AttributeSet->GetMaxPosture();
	const UWorld* World = GetWorld();
	if (CurrentPosture >= MaxPosture || !World
		|| !PostureSystem->IsRegenAllowed(World->GetTimeSeconds()))
	{
		return;
	}

	const float RecoveryMultiplier = FMath::Max(
		0.f,
		EnemyAbilitySystemComponent->GetNumericAttribute(
			UMordecaiAttributeSet::GetPostureRecoveryMultiplierAttribute()));
	const float RegenAmount = PostureSystem->ComputeRegenAmount(DeltaTime) * RecoveryMultiplier;
	EnemyAbilitySystemComponent->SetNumericAttributeBase(
		UMordecaiAttributeSet::GetPostureAttribute(),
		FMath::Min(CurrentPosture + RegenAmount, MaxPosture));
}

void AMordecaiEnemyCharacter::ClearPostureRuntimeStates()
{
	if (!HasAuthority())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PostureBrokenTimerHandle);
		World->GetTimerManager().ClearTimer(KnockDownTimerHandle);
		World->GetTimerManager().ClearTimer(ParriedTimerHandle);
	}
	SetPostureRuntimeState(MordecaiGameplayTags::State_PostureBroken, false);
	SetPostureRuntimeState(MordecaiGameplayTags::State_KnockedDown, false);
	SetPostureRuntimeState(MordecaiGameplayTags::State_Parried, false);
	SetPostureRuntimeState(MordecaiGameplayTags::State_Riposte, false);
	if (PostureSystem)
	{
		PostureSystem->Reset();
	}
}

// ---------------------------------------------------------------------------
// US-053: Arena Reset
// ---------------------------------------------------------------------------

void AMordecaiEnemyCharacter::ResetForArena()
{
	if (!HasAuthority() || !EnemyAbilitySystemComponent || !AttributeSet)
	{
		return;
	}

	ClearPostureRuntimeStates();

	// Remove death tag
	EnemyAbilitySystemComponent->RemoveLooseGameplayTag(MordecaiGameplayTags::State_Dead);

	// Restore health and posture to max (AC-053.9)
	const float MaxHealth = AttributeSet->GetMaxHealth();
	const float MaxPosture = AttributeSet->GetMaxPosture();
	if (EnemyAbilitySystemComponent->GetOwnerActor())
	{
		EnemyAbilitySystemComponent->SetNumericAttributeBase(
			UMordecaiAttributeSet::GetHealthAttribute(), MaxHealth);
		EnemyAbilitySystemComponent->SetNumericAttributeBase(
			UMordecaiAttributeSet::GetPostureAttribute(), MaxPosture);
	}
	else
	{
		// NewObject-only automation fixtures have no initialized GAS owner.
		AttributeSet->InitHealth(MaxHealth);
		AttributeSet->InitPosture(MaxPosture);
	}

	if (EnemyIndicatorComponent)
	{
		EnemyIndicatorComponent->NotifyArenaReset();
	}

	// Re-enable collision, and show the body again (issue #19)
	SetActorEnableCollision(true);
	if (BodyMesh)
	{
		BodyMesh->SetVisibility(true);
	}

	// Re-enable movement
	if (UCharacterMovementComponent* CMC = GetCharacterMovement())
	{
		CMC->SetMovementMode(MOVE_Walking);
		CMC->MaxWalkSpeed = DefaultMoveSpeed;
	}

	// Reset AI state to Idle (AC-053.9)
	if (AMordecaiEnemyAIController* AI = Cast<AMordecaiEnemyAIController>(GetController()))
	{
		AI->ResetState();
	}
}

// ---------------------------------------------------------------------------
// US-054: Ability Granting
// ---------------------------------------------------------------------------

void AMordecaiEnemyCharacter::GrantDefaultAbilities()
{
	if (!EnemyAbilitySystemComponent || !HasAuthority())
	{
		return;
	}

	for (const TSubclassOf<UGameplayAbility>& AbilityClass : DefaultAbilities)
	{
		if (!AbilityClass)
		{
			continue;
		}

		FGameplayAbilitySpec Spec(AbilityClass, 1, INDEX_NONE, this);
		FGameplayAbilitySpecHandle Handle = EnemyAbilitySystemComponent->GiveAbility(Spec);

		// Configure attack profiles on instanced melee abilities (AC-054.13)
		if (AbilityClass->IsChildOf(UMordecaiGA_MeleeAttack::StaticClass()))
		{
			// Use EnemyAttackProfiles if configured (set in Blueprint); otherwise no profiles
			TArray<TObjectPtr<UMordecaiAttackProfileDataAsset>>& ProfilesToUse = EnemyAttackProfiles;

			if (ProfilesToUse.Num() > 0)
			{
				FGameplayAbilitySpec* GrantedSpec = EnemyAbilitySystemComponent->FindAbilitySpecFromHandle(Handle);
				if (GrantedSpec && GrantedSpec->GetPrimaryInstance())
				{
					UMordecaiGA_MeleeAttack* MeleeInstance = Cast<UMordecaiGA_MeleeAttack>(GrantedSpec->GetPrimaryInstance());
					if (MeleeInstance)
					{
						MeleeInstance->AttackProfiles = ProfilesToUse;
					}
				}
			}
		}
	}
}
