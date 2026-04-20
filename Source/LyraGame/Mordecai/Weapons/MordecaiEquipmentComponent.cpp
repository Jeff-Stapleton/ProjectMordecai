// Project Mordecai — Equipment Component (US-024)

#include "Mordecai/Weapons/MordecaiEquipmentComponent.h"
#include "Mordecai/Weapons/MordecaiWeaponDataAsset.h"
#include "Mordecai/Combat/MordecaiAttackProfileDataAsset.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/Items/MordecaiItemLibrary.h"
#include "Mordecai/Items/MordecaiItemInstance.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiEquipmentComponent)

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMordecaiEquipmentComponent::UMordecaiEquipmentComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

// ---------------------------------------------------------------------------
// Replication
// ---------------------------------------------------------------------------

void UMordecaiEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

// ---------------------------------------------------------------------------
// Dependency Injection
// ---------------------------------------------------------------------------

void UMordecaiEquipmentComponent::SetASCOverride(UAbilitySystemComponent* InASC)
{
	ASCOverride = InASC;
}

bool UMordecaiEquipmentComponent::IsInstanceEquippable(const FMordecaiItemInstance& Instance)
{
	return UMordecaiItemLibrary::CanEquipInstance(Instance);
}

UAbilitySystemComponent* UMordecaiEquipmentComponent::FindAbilitySystemComponent() const
{
	if (ASCOverride)
	{
		return ASCOverride;
	}

	AActor* Owner = GetOwner();
	return Owner ? Owner->FindComponentByClass<UAbilitySystemComponent>() : nullptr;
}

// ---------------------------------------------------------------------------
// Equip / Unequip
// ---------------------------------------------------------------------------

bool UMordecaiEquipmentComponent::EquipWeapon(FMordecaiWeaponInstance& Weapon, EMordecaiEquipSlot Slot)
{
	if (!Weapon.IsValid())
	{
		return false;
	}

	const UMordecaiWeaponDataAsset* WeaponData = Weapon.WeaponDataAsset;
	if (!WeaponData)
	{
		return false;
	}

	// AC-024.8: TwoHand clears both MainHand and OffHand
	if (Slot == EMordecaiEquipSlot::TwoHand)
	{
		if (MainHandWeapon.IsValid())
		{
			UnequipWeapon(EMordecaiEquipSlot::MainHand);
		}
		if (OffHandWeapon.IsValid())
		{
			UnequipWeapon(EMordecaiEquipSlot::OffHand);
		}
	}
	// AC-024.8: MainHand while TwoHand is equipped — unequip TwoHand first
	else if (Slot == EMordecaiEquipSlot::MainHand && MainHandWeapon.IsValid()
		&& MainHandWeapon.WeaponDataAsset && MainHandWeapon.WeaponDataAsset->EquipSlot == EMordecaiEquipSlot::TwoHand)
	{
		UnequipWeapon(EMordecaiEquipSlot::MainHand);
	}
	// Unequip whatever is currently in the target slot
	else if (Slot == EMordecaiEquipSlot::MainHand && MainHandWeapon.IsValid())
	{
		UnequipWeapon(EMordecaiEquipSlot::MainHand);
	}
	else if (Slot == EMordecaiEquipSlot::OffHand && OffHandWeapon.IsValid())
	{
		UnequipWeapon(EMordecaiEquipSlot::OffHand);
	}

	// Store the weapon in the slot
	// TwoHand weapons are stored in MainHand slot
	EMordecaiEquipSlot StorageSlot = (Slot == EMordecaiEquipSlot::TwoHand) ? EMordecaiEquipSlot::MainHand : Slot;

	if (StorageSlot == EMordecaiEquipSlot::MainHand)
	{
		MainHandWeapon = Weapon;
		MainHandWeapon.IsEquipped = true;
	}
	else
	{
		OffHandWeapon = Weapon;
		OffHandWeapon.IsEquipped = true;
	}

	// Mark the caller's copy as equipped too
	Weapon.IsEquipped = true;

	// Apply GE, abilities, tags
	ApplyWeaponEffects(WeaponData, StorageSlot);

	// AC-077.10: Broadcast weapon change
	OnWeaponChanged.Broadcast(Slot, WeaponData);

	return true;
}

bool UMordecaiEquipmentComponent::UnequipWeapon(EMordecaiEquipSlot Slot)
{
	// TwoHand unequip acts on MainHand storage
	EMordecaiEquipSlot StorageSlot = (Slot == EMordecaiEquipSlot::TwoHand) ? EMordecaiEquipSlot::MainHand : Slot;

	FMordecaiWeaponInstance* WeaponSlot = (StorageSlot == EMordecaiEquipSlot::MainHand) ? &MainHandWeapon : &OffHandWeapon;

	if (!WeaponSlot->IsValid())
	{
		return false;
	}

	// Remove GE, abilities, tags
	RemoveWeaponEffects(StorageSlot);

	// Clear the slot
	WeaponSlot->IsEquipped = false;
	*WeaponSlot = FMordecaiWeaponInstance();

	// AC-077.10: Broadcast weapon change (nullptr on unequip)
	OnWeaponChanged.Broadcast(Slot, nullptr);

	return true;
}

// ---------------------------------------------------------------------------
// Weapon Cycling (US-077)
// ---------------------------------------------------------------------------

int32 UMordecaiEquipmentComponent::AddAvailableWeapon(const UMordecaiWeaponDataAsset* WeaponAsset)
{
	if (!WeaponAsset)
	{
		return INDEX_NONE;
	}

	FMordecaiWeaponInstance Inst;
	Inst.InstanceId = FGuid::NewGuid();
	Inst.WeaponDataAsset = const_cast<UMordecaiWeaponDataAsset*>(WeaponAsset);
	Inst.IsEquipped = false;

	return AvailableWeapons.Add(Inst);
}

int32 UMordecaiEquipmentComponent::GetCurrentWeaponIndex() const
{
	if (!MainHandWeapon.IsValid())
	{
		return INDEX_NONE;
	}

	for (int32 Idx = 0; Idx < AvailableWeapons.Num(); ++Idx)
	{
		if (AvailableWeapons[Idx].InstanceId == MainHandWeapon.InstanceId)
		{
			return Idx;
		}
	}
	return INDEX_NONE;
}

void UMordecaiEquipmentComponent::CycleNextWeapon()
{
	if (AvailableWeapons.Num() == 0)
	{
		return;
	}

	const int32 CurrentIdx = GetCurrentWeaponIndex();
	int32 NextIdx;
	if (CurrentIdx == INDEX_NONE)
	{
		NextIdx = 0;
	}
	else
	{
		NextIdx = (CurrentIdx + 1) % AvailableWeapons.Num();
	}

	const EMordecaiEquipSlot TargetSlot = AvailableWeapons[NextIdx].WeaponDataAsset
		? AvailableWeapons[NextIdx].WeaponDataAsset->EquipSlot
		: EMordecaiEquipSlot::MainHand;

	EquipWeapon(AvailableWeapons[NextIdx], TargetSlot);
}

void UMordecaiEquipmentComponent::CyclePrevWeapon()
{
	if (AvailableWeapons.Num() == 0)
	{
		return;
	}

	const int32 CurrentIdx = GetCurrentWeaponIndex();
	int32 PrevIdx;
	if (CurrentIdx == INDEX_NONE)
	{
		PrevIdx = 0;
	}
	else
	{
		PrevIdx = (CurrentIdx - 1 + AvailableWeapons.Num()) % AvailableWeapons.Num();
	}

	const EMordecaiEquipSlot TargetSlot = AvailableWeapons[PrevIdx].WeaponDataAsset
		? AvailableWeapons[PrevIdx].WeaponDataAsset->EquipSlot
		: EMordecaiEquipSlot::MainHand;

	EquipWeapon(AvailableWeapons[PrevIdx], TargetSlot);
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

const FMordecaiWeaponInstance* UMordecaiEquipmentComponent::GetEquippedWeapon(EMordecaiEquipSlot Slot) const
{
	EMordecaiEquipSlot StorageSlot = (Slot == EMordecaiEquipSlot::TwoHand) ? EMordecaiEquipSlot::MainHand : Slot;

	const FMordecaiWeaponInstance* WeaponSlot = (StorageSlot == EMordecaiEquipSlot::MainHand) ? &MainHandWeapon : &OffHandWeapon;

	return WeaponSlot->IsValid() ? WeaponSlot : nullptr;
}

bool UMordecaiEquipmentComponent::HasEquippedWeapon(EMordecaiEquipSlot Slot) const
{
	return GetEquippedWeapon(Slot) != nullptr;
}

FMordecaiWeaponInstance* UMordecaiEquipmentComponent::GetEquippedWeaponMutable(EMordecaiEquipSlot Slot)
{
	EMordecaiEquipSlot StorageSlot = (Slot == EMordecaiEquipSlot::TwoHand) ? EMordecaiEquipSlot::MainHand : Slot;

	FMordecaiWeaponInstance* WeaponSlot = (StorageSlot == EMordecaiEquipSlot::MainHand) ? &MainHandWeapon : &OffHandWeapon;

	return WeaponSlot->IsValid() ? WeaponSlot : nullptr;
}

TArray<UMordecaiAttackProfileDataAsset*> UMordecaiEquipmentComponent::GetActiveLightAttackProfiles() const
{
	if (MainHandWeapon.IsValid() && MainHandWeapon.WeaponDataAsset)
	{
		TArray<UMordecaiAttackProfileDataAsset*> Result;
		for (const TObjectPtr<UMordecaiAttackProfileDataAsset>& Profile : MainHandWeapon.WeaponDataAsset->LightAttackProfiles)
		{
			Result.Add(Profile);
		}
		return Result;
	}

	// Unarmed fallback
	TArray<UMordecaiAttackProfileDataAsset*> Result;
	for (const TObjectPtr<UMordecaiAttackProfileDataAsset>& Profile : UnarmedAttackProfiles)
	{
		Result.Add(Profile);
	}
	return Result;
}

UMordecaiAttackProfileDataAsset* UMordecaiEquipmentComponent::GetActiveHeavyAttackProfile() const
{
	if (MainHandWeapon.IsValid() && MainHandWeapon.WeaponDataAsset)
	{
		return MainHandWeapon.WeaponDataAsset->HeavyAttackProfile;
	}
	return nullptr;
}

float UMordecaiEquipmentComponent::GetWeaponBaseDamage() const
{
	if (MainHandWeapon.IsValid() && MainHandWeapon.WeaponDataAsset)
	{
		return MainHandWeapon.WeaponDataAsset->BaseDamage;
	}
	return 0.f;
}

float UMordecaiEquipmentComponent::GetWeaponAttackSpeedMultiplier() const
{
	if (MainHandWeapon.IsValid() && MainHandWeapon.WeaponDataAsset)
	{
		return MainHandWeapon.WeaponDataAsset->AttackSpeedMultiplier;
	}
	return 1.f;
}

float UMordecaiEquipmentComponent::GetWeaponPostureDamageBonus() const
{
	if (MainHandWeapon.IsValid() && MainHandWeapon.WeaponDataAsset)
	{
		return MainHandWeapon.WeaponDataAsset->PostureDamageBonus;
	}
	return 0.f;
}

// ---------------------------------------------------------------------------
// Internal: Apply / Remove Effects
// ---------------------------------------------------------------------------

void UMordecaiEquipmentComponent::ApplyWeaponEffects(const UMordecaiWeaponDataAsset* WeaponData, EMordecaiEquipSlot Slot)
{
	if (!WeaponData)
	{
		return;
	}

	UAbilitySystemComponent* ASC = FindAbilitySystemComponent();
	FSlotEffectState& State = SlotEffects.FindOrAdd(Slot);

	if (ASC)
	{
		// AC-024.6: Apply StatModifiers as an infinite-duration GE
		if (WeaponData->StatModifiers.Num() > 0)
		{
			UGameplayEffect* StatGE = NewObject<UGameplayEffect>(GetTransientPackage(), TEXT("GE_MordecaiWeaponStatMod"));
			StatGE->DurationPolicy = EGameplayEffectDurationType::Infinite;

			for (const FMordecaiStatModifier& StatMod : WeaponData->StatModifiers)
			{
				if (StatMod.Attribute.IsValid())
				{
					FGameplayModifierInfo& Mod = StatGE->Modifiers.AddDefaulted_GetRef();
					Mod.Attribute = StatMod.Attribute;

					switch (StatMod.Operation)
					{
					case EMordecaiModifierOp::Add:
						Mod.ModifierOp = EGameplayModOp::Additive;
						break;
					case EMordecaiModifierOp::Multiply:
						Mod.ModifierOp = EGameplayModOp::Multiplicitive;
						break;
					case EMordecaiModifierOp::PercentAdd:
						Mod.ModifierOp = EGameplayModOp::Additive;
						break;
					}

					Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(StatMod.Value));
				}
			}

			FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
			FGameplayEffectSpec Spec(StatGE, Context, 1.0f);
			State.StatModifierGEHandle = ASC->ApplyGameplayEffectSpecToSelf(Spec);
		}

		// AC-024.6: Grant abilities
		for (const TSubclassOf<UGameplayAbility>& AbilityClass : WeaponData->GrantedAbilities)
		{
			if (AbilityClass)
			{
				FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1));
				State.AbilityHandles.Add(Handle);
			}
		}

		// AC-024.6: Add GrantedTags
		State.AppliedTags = WeaponData->GrantedTags;
		for (const FGameplayTag& Tag : State.AppliedTags)
		{
			ASC->AddLooseGameplayTag(Tag);
		}
	}
}

void UMordecaiEquipmentComponent::RemoveWeaponEffects(EMordecaiEquipSlot Slot)
{
	FSlotEffectState* State = SlotEffects.Find(Slot);
	if (!State)
	{
		return;
	}

	UAbilitySystemComponent* ASC = FindAbilitySystemComponent();
	if (ASC)
	{
		// AC-024.7: Remove stat modifier GE
		if (State->StatModifierGEHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(State->StatModifierGEHandle);
		}

		// AC-024.7: Remove granted abilities
		for (const FGameplayAbilitySpecHandle& Handle : State->AbilityHandles)
		{
			ASC->ClearAbility(Handle);
		}

		// AC-024.7: Remove granted tags
		for (const FGameplayTag& Tag : State->AppliedTags)
		{
			ASC->RemoveLooseGameplayTag(Tag);
		}
	}

	SlotEffects.Remove(Slot);
}
