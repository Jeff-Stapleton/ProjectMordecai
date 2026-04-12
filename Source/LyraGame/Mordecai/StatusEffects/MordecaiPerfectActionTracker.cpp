// Project Mordecai — Perfect Action Tracker Component (US-018)

#include "Mordecai/StatusEffects/MordecaiPerfectActionTracker.h"
#include "Mordecai/StatusEffects/Effects/MordecaiGE_Focused.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"

UMordecaiPerfectActionTracker::UMordecaiPerfectActionTracker(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UMordecaiPerfectActionTracker::BeginPlay()
{
	Super::BeginPlay();
	StartTracking();
}

void UMordecaiPerfectActionTracker::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopTracking();
	Super::EndPlay(EndPlayReason);
}

void UMordecaiPerfectActionTracker::SetAbilitySystemComponentOverride(UAbilitySystemComponent* InASC)
{
	ASCOverride = InASC;
}

UAbilitySystemComponent* UMordecaiPerfectActionTracker::GetAbilitySystemComponent() const
{
	if (ASCOverride)
	{
		return ASCOverride;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Owner))
	{
		return ASI->GetAbilitySystemComponent();
	}

	return Owner->FindComponentByClass<UAbilitySystemComponent>();
}

bool UMordecaiPerfectActionTracker::IsFocused() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return false;
	}
	return ASC->HasMatchingGameplayTag(MordecaiGameplayTags::Status_Focused);
}

void UMordecaiPerfectActionTracker::StartTracking()
{
	if (bTracking)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	bTracking = true;

	// AC-018.9: Listen for perfect action events
	{
		FGameplayEventMulticastDelegate& Delegate = ASC->GenericGameplayEventCallbacks.FindOrAdd(
			MordecaiGameplayTags::Event_PerfectAction);
		PerfectActionDelegateHandle = Delegate.AddUObject(
			this, &UMordecaiPerfectActionTracker::OnPerfectAction);
	}

	// AC-018.12/14: Listen for damage taken → reset counter + remove Focused
	{
		FGameplayEventMulticastDelegate& Delegate = ASC->GenericGameplayEventCallbacks.FindOrAdd(
			MordecaiGameplayTags::Event_DamageTaken);
		DamageTakenDelegateHandle = Delegate.AddUObject(
			this, &UMordecaiPerfectActionTracker::OnDamageTaken);
	}

	// AC-018.14: Listen for non-perfect actions → reset counter
	{
		FGameplayEventMulticastDelegate& Delegate = ASC->GenericGameplayEventCallbacks.FindOrAdd(
			MordecaiGameplayTags::Event_NonPerfectAction);
		NonPerfectActionDelegateHandle = Delegate.AddUObject(
			this, &UMordecaiPerfectActionTracker::OnNonPerfectAction);
	}
}

void UMordecaiPerfectActionTracker::StopTracking()
{
	if (!bTracking)
	{
		return;
	}

	bTracking = false;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	if (PerfectActionDelegateHandle.IsValid())
	{
		ASC->GenericGameplayEventCallbacks.FindOrAdd(
			MordecaiGameplayTags::Event_PerfectAction).Remove(PerfectActionDelegateHandle);
		PerfectActionDelegateHandle.Reset();
	}

	if (DamageTakenDelegateHandle.IsValid())
	{
		ASC->GenericGameplayEventCallbacks.FindOrAdd(
			MordecaiGameplayTags::Event_DamageTaken).Remove(DamageTakenDelegateHandle);
		DamageTakenDelegateHandle.Reset();
	}

	if (NonPerfectActionDelegateHandle.IsValid())
	{
		ASC->GenericGameplayEventCallbacks.FindOrAdd(
			MordecaiGameplayTags::Event_NonPerfectAction).Remove(NonPerfectActionDelegateHandle);
		NonPerfectActionDelegateHandle.Reset();
	}
}

void UMordecaiPerfectActionTracker::OnPerfectAction(const FGameplayEventData* Payload)
{
	ConsecutivePerfectActions++;

	if (ConsecutivePerfectActions >= FocusedStreakThreshold && !IsFocused())
	{
		ApplyFocused();
	}
}

void UMordecaiPerfectActionTracker::OnDamageTaken(const FGameplayEventData* Payload)
{
	// AC-018.12: Remove Focused on any damage
	if (IsFocused())
	{
		RemoveFocused();
	}

	// AC-018.14: Reset counter on hit
	ResetStreak();
}

void UMordecaiPerfectActionTracker::OnNonPerfectAction(const FGameplayEventData* Payload)
{
	// AC-018.14: Non-perfect action resets the counter
	ResetStreak();
}

void UMordecaiPerfectActionTracker::ApplyFocused()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UMordecaiGE_Focused::StaticClass(), 1.0f, Context);
	if (Spec.IsValid())
	{
		ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
}

void UMordecaiPerfectActionTracker::RemoveFocused()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	FGameplayTagContainer TagsToRemove;
	TagsToRemove.AddTag(MordecaiGameplayTags::Status_Focused);
	ASC->RemoveActiveEffectsWithGrantedTags(TagsToRemove);
}

void UMordecaiPerfectActionTracker::ResetStreak()
{
	ConsecutivePerfectActions = 0;
}

void UMordecaiPerfectActionTracker::SimulatePerfectAction()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (ASC)
	{
		FGameplayEventData EventData;
		ASC->HandleGameplayEvent(MordecaiGameplayTags::Event_PerfectAction, &EventData);
	}
}

void UMordecaiPerfectActionTracker::SimulateDamageTaken()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (ASC)
	{
		FGameplayEventData EventData;
		ASC->HandleGameplayEvent(MordecaiGameplayTags::Event_DamageTaken, &EventData);
	}
}

void UMordecaiPerfectActionTracker::SimulateNonPerfectAction()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (ASC)
	{
		FGameplayEventData EventData;
		ASC->HandleGameplayEvent(MordecaiGameplayTags::Event_NonPerfectAction, &EventData);
	}
}
