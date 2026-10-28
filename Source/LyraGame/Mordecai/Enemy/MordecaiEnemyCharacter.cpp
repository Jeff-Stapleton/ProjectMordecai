// Project Mordecai — Enemy Character (US-050)

#include "Mordecai/Enemy/MordecaiEnemyCharacter.h"

#include "Mordecai/AbilitySystem/MordecaiAbilitySystemComponent.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/Combat/MordecaiPostureSystem.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/Combat/MordecaiAttackProfileDataAsset.h"
#include "Mordecai/UI/MordecaiEnemyHealthBarWidget.h"
#include "Mordecai/UI/MordecaiEnemyIndicatorComponent.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/MordecaiGameMode.h"
#include "Mordecai/Enemy/MordecaiEnemyAIController.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

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
		UE_LOG(LogMordecaiEnemy, Error, TEXT("enemy %s has no visible mesh: players cannot see it (set a mesh on %s)"),
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
	CreateHealthBarWidget();

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

	if (IsDead() || !EnemyAbilitySystemComponent || !AttributeSet || !PostureSystem)
	{
		return;
	}

	const float CurrentPosture = AttributeSet->GetPosture();
	const float MaxPosture = AttributeSet->GetMaxPosture();

	if (CurrentPosture < MaxPosture)
	{
		if (const UWorld* World = GetWorld())
		{
			if (PostureSystem->IsRegenAllowed(World->GetTimeSeconds()))
			{
				const float RegenAmount = PostureSystem->ComputeRegenAmount(DeltaTime);
				const float NewPosture = FMath::Min(CurrentPosture + RegenAmount, MaxPosture);
				EnemyAbilitySystemComponent->SetNumericAttributeBase(
					UMordecaiAttributeSet::GetPostureAttribute(), NewPosture);
			}
		}
	}
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
		EnterDeathState();
	}
}

void AMordecaiEnemyCharacter::OnPostureChanged(const FOnAttributeChangeData& Data)
{
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
		&& !EnemyAbilitySystemComponent->HasMatchingGameplayTag(MordecaiGameplayTags::State_PostureBroken))
	{
		EnterPostureBrokenState();
	}
}

// ---------------------------------------------------------------------------
// State Transitions
// ---------------------------------------------------------------------------

void AMordecaiEnemyCharacter::EnterDeathState()
{
	if (!EnemyAbilitySystemComponent)
	{
		return;
	}

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

	// AC-050.6: Broadcast Event_Death gameplay event
	FGameplayEventData EventData;
	EventData.EventTag = MordecaiGameplayTags::Event_Death;
	EventData.Instigator = this;
	EventData.Target = this;
	EnemyAbilitySystemComponent->HandleGameplayEvent(MordecaiGameplayTags::Event_Death, &EventData);

	// US-053: Notify game mode for kill tracking
	if (UWorld* World = GetWorld())
	{
		if (AMordecaiGameMode* GM = Cast<AMordecaiGameMode>(World->GetAuthGameMode()))
		{
			GM->NotifyEnemyKilled(this);
		}
	}
}

void AMordecaiEnemyCharacter::EnterPostureBrokenState()
{
	if (!EnemyAbilitySystemComponent || !PostureSystem)
	{
		return;
	}

	// AC-050.8: Apply PostureBroken tag (replicated via ASC, AC-050.11)
	EnemyAbilitySystemComponent->AddLooseGameplayTag(MordecaiGameplayTags::State_PostureBroken);
	PostureSystem->EnterPostureBroken();

	// Set timer to exit posture broken state after PostureBrokenDurationMs (AC-050.8)
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(PostureBrokenTimerHandle, this,
			&AMordecaiEnemyCharacter::ExitPostureBrokenState,
			PostureSystem->GetPostureBrokenDurationSeconds(), false);
	}
}

void AMordecaiEnemyCharacter::ExitPostureBrokenState()
{
	if (!EnemyAbilitySystemComponent || !PostureSystem || !AttributeSet)
	{
		return;
	}

	// Remove PostureBroken tag
	EnemyAbilitySystemComponent->RemoveLooseGameplayTag(MordecaiGameplayTags::State_PostureBroken);

	// Reset posture to MaxPosture (AC-050.8, matches UMordecaiPostureSystem pattern)
	const float MaxPosture = AttributeSet->GetMaxPosture();
	PostureSystem->ExitPostureBroken(MaxPosture);
	EnemyAbilitySystemComponent->SetNumericAttributeBase(
		UMordecaiAttributeSet::GetPostureAttribute(), MaxPosture);
}

// ---------------------------------------------------------------------------
// US-053: Arena Reset
// ---------------------------------------------------------------------------

void AMordecaiEnemyCharacter::ResetForArena()
{
	if (!EnemyAbilitySystemComponent || !AttributeSet)
	{
		return;
	}

	// Remove death and posture broken tags
	EnemyAbilitySystemComponent->RemoveLooseGameplayTag(MordecaiGameplayTags::State_Dead);
	EnemyAbilitySystemComponent->RemoveLooseGameplayTag(MordecaiGameplayTags::State_PostureBroken);

	// Cancel posture broken timer if active
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PostureBrokenTimerHandle);
	}

	// Restore health and posture to max (AC-053.9)
	const float MaxHealth = AttributeSet->GetMaxHealth();
	const float MaxPosture = AttributeSet->GetMaxPosture();
	AttributeSet->InitHealth(MaxHealth);
	AttributeSet->InitPosture(MaxPosture);

	// Reset posture system state
	if (PostureSystem)
	{
		PostureSystem->ExitPostureBroken(MaxPosture);
	}

	// Re-enable collision
	SetActorEnableCollision(true);

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

// ---------------------------------------------------------------------------
// US-054: Enemy Health Bar Widget
// ---------------------------------------------------------------------------

void AMordecaiEnemyCharacter::CreateHealthBarWidget()
{
	if (!EnemyHealthBarWidgetClass)
	{
		return;
	}

	HealthBarWidgetComponent = NewObject<UWidgetComponent>(this, TEXT("HealthBarWidget"));
	if (HealthBarWidgetComponent)
	{
		HealthBarWidgetComponent->SetupAttachment(GetRootComponent());
		HealthBarWidgetComponent->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
		HealthBarWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
		HealthBarWidgetComponent->SetDrawSize(FVector2D(150.f, 20.f));
		HealthBarWidgetComponent->SetWidgetClass(EnemyHealthBarWidgetClass);
		HealthBarWidgetComponent->RegisterComponent();

		// Bind to enemy ASC after the widget is created
		if (UMordecaiEnemyHealthBarWidget* HealthBar = Cast<UMordecaiEnemyHealthBarWidget>(HealthBarWidgetComponent->GetWidget()))
		{
			HealthBar->BindToASC(EnemyAbilitySystemComponent);
		}
	}
}
