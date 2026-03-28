// Project Mordecai — Enemy Character (US-050)

#include "Mordecai/Enemy/MordecaiEnemyCharacter.h"

#include "Mordecai/AbilitySystem/MordecaiAbilitySystemComponent.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/Combat/MordecaiPostureSystem.h"
#include "Mordecai/UI/MordecaiEnemyHealthBarWidget.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiEnemyCharacter)

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

AMordecaiEnemyCharacter::AMordecaiEnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Enemy owns its own ASC (not on PlayerState like the player)
	EnemyAbilitySystemComponent = CreateDefaultSubobject<UMordecaiAbilitySystemComponent>(TEXT("EnemyASC"));
	EnemyAbilitySystemComponent->SetIsReplicated(true);
	// Minimal mode: tags+cues replicate to all clients, GEs do not (appropriate for AI)
	EnemyAbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	// Attribute set — will be discovered by ASC during InitAbilityActorInfo
	AttributeSet = CreateDefaultSubobject<UMordecaiAttributeSet>(TEXT("AttributeSet"));

	// Posture system for break/regen logic (reuses existing UMordecaiPostureSystem)
	PostureSystem = CreateDefaultSubobject<UMordecaiPostureSystem>(TEXT("PostureSystem"));

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

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void AMordecaiEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	InitializeAbilitySystem();
	CreateHealthBarWidget();
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
		if (AbilityClass)
		{
			FGameplayAbilitySpec Spec(AbilityClass, 1, INDEX_NONE, this);
			EnemyAbilitySystemComponent->GiveAbility(Spec);
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
