// Project Mordecai — Character Sheet Widget (US-066)

#include "Mordecai/UI/MordecaiCharacterSheetWidget.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeScaling.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/UI/MordecaiPauseMenuWidget.h"
#include "AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiCharacterSheetWidget)

#define LOCTEXT_NAMESPACE "MordecaiCharacterSheet"

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

FText UMordecaiCharacterSheetWidget::GetCharacterTabDisplayName()
{
	return LOCTEXT("CharacterTabName", "Character");
}

void UMordecaiCharacterSheetWidget::RegisterWithPauseMenu(UMordecaiPauseMenuWidget* PauseMenu, TSubclassOf<UMordecaiCharacterSheetWidget> WidgetClass)
{
	if (!PauseMenu)
	{
		return;
	}
	TSubclassOf<UUserWidget> ContentClass = WidgetClass ? *WidgetClass : UMordecaiCharacterSheetWidget::StaticClass();
	PauseMenu->RegisterTab(GetCharacterTabId(), GetCharacterTabDisplayName(), ContentClass);
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void UMordecaiCharacterSheetWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	Data.InitDefaults();
}

void UMordecaiCharacterSheetWidget::NativeDestruct()
{
	UnbindFromASC();
	Super::NativeDestruct();
}

// ---------------------------------------------------------------------------
// ASC Binding
// ---------------------------------------------------------------------------

// Helper: register a delegate and track its handle for cleanup.
void UMordecaiCharacterSheetWidget::RegisterAttributeDelegate(const FGameplayAttribute& Attribute, TFunction<void(const FOnAttributeChangeData&)> Callback)
{
	UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC)
	{
		return;
	}
	FDelegateHandle Handle = ASC->GetGameplayAttributeValueChangeDelegate(Attribute).AddLambda(MoveTemp(Callback));
	DelegateHandles.Emplace(Attribute, Handle);
}

void UMordecaiCharacterSheetWidget::BindToASC(UAbilitySystemComponent* ASC)
{
	UnbindFromASC();
	if (!ASC)
	{
		return;
	}

	// Ensure defaults are initialized (widgets can call BindToASC before NativeOnInitialized runs in headless tests).
	if (Data.PrimaryAttributes.Num() == 0)
	{
		Data.InitDefaults();
	}

	BoundASC = ASC;

	// --- Primary attributes (9) ---
	struct FPrimaryBind { FGameplayAttribute Attr; int32 Index; };
	const FPrimaryBind Primaries[] = {
		{ UMordecaiAttributeSet::GetStrengthAttribute(),     0 },
		{ UMordecaiAttributeSet::GetDexterityAttribute(),    1 },
		{ UMordecaiAttributeSet::GetEnduranceAttribute(),    2 },
		{ UMordecaiAttributeSet::GetConstitutionAttribute(), 3 },
		{ UMordecaiAttributeSet::GetResistanceAttribute(),   4 },
		{ UMordecaiAttributeSet::GetDisciplineAttribute(),   5 },
		{ UMordecaiAttributeSet::GetIntelligenceAttribute(), 6 },
		{ UMordecaiAttributeSet::GetWisdomAttribute(),       7 },
		{ UMordecaiAttributeSet::GetCharismaAttribute(),     8 },
	};
	for (const FPrimaryBind& B : Primaries)
	{
		const int32 Idx = B.Index;
		RegisterAttributeDelegate(B.Attr, [this, Idx](const FOnAttributeChangeData& D)
		{
			OnPrimaryAttributeChanged(D, Idx);
		});
	}

	// --- Primary-derived multipliers (5) ---
	const FPrimaryBind PrimaryDerived[] = {
		{ UMordecaiAttributeSet::GetPhysicalDamageMultiplierAttribute(),   0 },
		{ UMordecaiAttributeSet::GetAttackSpeedMultiplierAttribute(),      1 },
		{ UMordecaiAttributeSet::GetAfflictionResistMultiplierAttribute(), 2 },
		{ UMordecaiAttributeSet::GetMagicDamageMultiplierAttribute(),      3 },
		{ UMordecaiAttributeSet::GetCastSpeedMultiplierAttribute(),        4 },
	};
	for (const FPrimaryBind& B : PrimaryDerived)
	{
		const int32 Idx = B.Index;
		RegisterAttributeDelegate(B.Attr, [this, Idx](const FOnAttributeChangeData& D)
		{
			OnPrimaryDerivedChanged(D, Idx);
		});
	}

	// --- Secondary-derived (9) ---
	const FPrimaryBind SecondaryDerived[] = {
		{ UMordecaiAttributeSet::GetArmorPenetrationMultiplierAttribute(),     0 },
		{ UMordecaiAttributeSet::GetPhysicalCritChanceAttribute(),             1 },
		{ UMordecaiAttributeSet::GetStaminaRegenMultiplierAttribute(),         2 },
		{ UMordecaiAttributeSet::GetHealthRegenMultiplierAttribute(),          3 },
		{ UMordecaiAttributeSet::GetAfflictionRecoveryMultiplierAttribute(),   4 },
		{ UMordecaiAttributeSet::GetPostureRecoveryMultiplierAttribute(),      5 },
		{ UMordecaiAttributeSet::GetSpellPointsRegenMultiplierAttribute(),     6 },
		{ UMordecaiAttributeSet::GetResistancePenetrationMultiplierAttribute(),7 },
		{ UMordecaiAttributeSet::GetMagicCritChanceAttribute(),                8 },
	};
	for (const FPrimaryBind& B : SecondaryDerived)
	{
		const int32 Idx = B.Index;
		RegisterAttributeDelegate(B.Attr, [this, Idx](const FOnAttributeChangeData& D)
		{
			OnSecondaryDerivedChanged(D, Idx);
		});
	}

	// --- Resources (4 pools × current+max = 8 delegates) ---
	struct FResourceBind { FGameplayAttribute Current; FGameplayAttribute Max; int32 Index; };
	const FResourceBind Resources[] = {
		{ UMordecaiAttributeSet::GetHealthAttribute(),      UMordecaiAttributeSet::GetMaxHealthAttribute(),      0 },
		{ UMordecaiAttributeSet::GetStaminaAttribute(),     UMordecaiAttributeSet::GetMaxStaminaAttribute(),     1 },
		{ UMordecaiAttributeSet::GetSpellPointsAttribute(), UMordecaiAttributeSet::GetMaxSpellPointsAttribute(), 2 },
		{ UMordecaiAttributeSet::GetPostureAttribute(),     UMordecaiAttributeSet::GetMaxPostureAttribute(),     3 },
	};
	for (const FResourceBind& R : Resources)
	{
		const int32 Idx = R.Index;
		RegisterAttributeDelegate(R.Current, [this, Idx](const FOnAttributeChangeData& D)
		{
			OnResourceChanged(D, Idx, /*bMax=*/false);
		});
		RegisterAttributeDelegate(R.Max, [this, Idx](const FOnAttributeChangeData& D)
		{
			OnResourceChanged(D, Idx, /*bMax=*/true);
		});
	}

	Data.bIsBound = true;
	RefreshAllFromASC();
}

void UMordecaiCharacterSheetWidget::UnbindFromASC()
{
	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		for (const TPair<FGameplayAttribute, FDelegateHandle>& Pair : DelegateHandles)
		{
			ASC->GetGameplayAttributeValueChangeDelegate(Pair.Key).Remove(Pair.Value);
		}
	}
	DelegateHandles.Reset();
	BoundASC.Reset();
	Data.bIsBound = false;

	// Reset per-row live value flags so UI falls back to placeholders.
	for (FMordecaiPrimaryAttributeDisplay& Row : Data.PrimaryAttributes) { Row.bHasValue = false; }
	for (FMordecaiDerivedStatDisplay&      Row : Data.PrimaryDerivedStats) { Row.bHasValue = false; }
	for (FMordecaiDerivedStatDisplay&      Row : Data.SecondaryDerivedStats) { Row.bHasValue = false; }
	for (FMordecaiResourceDisplay&         Row : Data.CoreResources) { Row.bHasValue = false; }
}

// ---------------------------------------------------------------------------
// Data refresh
// ---------------------------------------------------------------------------

void UMordecaiCharacterSheetWidget::RefreshAllFromASC()
{
	UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC)
	{
		return;
	}

	auto Read = [ASC](const FGameplayAttribute& Attr) -> float
	{
		bool bFound = false;
		const float Value = ASC->GetGameplayAttributeValue(Attr, bFound);
		return bFound ? Value : 0.f;
	};

	// Primaries
	const FGameplayAttribute Primaries[] = {
		UMordecaiAttributeSet::GetStrengthAttribute(),
		UMordecaiAttributeSet::GetDexterityAttribute(),
		UMordecaiAttributeSet::GetEnduranceAttribute(),
		UMordecaiAttributeSet::GetConstitutionAttribute(),
		UMordecaiAttributeSet::GetResistanceAttribute(),
		UMordecaiAttributeSet::GetDisciplineAttribute(),
		UMordecaiAttributeSet::GetIntelligenceAttribute(),
		UMordecaiAttributeSet::GetWisdomAttribute(),
		UMordecaiAttributeSet::GetCharismaAttribute(),
	};
	for (int32 i = 0; i < UE_ARRAY_COUNT(Primaries); ++i)
	{
		const float Base = Read(Primaries[i]);
		Data.PrimaryAttributes[i].BaseValue    = Base;
		Data.PrimaryAttributes[i].EffectiveMod = FMordecaiAttributeScaling::CalculateEffectiveMod(Base);
		Data.PrimaryAttributes[i].bHasValue    = true;
	}

	// Primary-derived
	const FGameplayAttribute PrimaryDerived[] = {
		UMordecaiAttributeSet::GetPhysicalDamageMultiplierAttribute(),
		UMordecaiAttributeSet::GetAttackSpeedMultiplierAttribute(),
		UMordecaiAttributeSet::GetAfflictionResistMultiplierAttribute(),
		UMordecaiAttributeSet::GetMagicDamageMultiplierAttribute(),
		UMordecaiAttributeSet::GetCastSpeedMultiplierAttribute(),
	};
	for (int32 i = 0; i < UE_ARRAY_COUNT(PrimaryDerived); ++i)
	{
		Data.PrimaryDerivedStats[i].MultiplierValue = Read(PrimaryDerived[i]);
		Data.PrimaryDerivedStats[i].bHasValue       = true;
	}

	// Secondary-derived
	const FGameplayAttribute SecondaryDerived[] = {
		UMordecaiAttributeSet::GetArmorPenetrationMultiplierAttribute(),
		UMordecaiAttributeSet::GetPhysicalCritChanceAttribute(),
		UMordecaiAttributeSet::GetStaminaRegenMultiplierAttribute(),
		UMordecaiAttributeSet::GetHealthRegenMultiplierAttribute(),
		UMordecaiAttributeSet::GetAfflictionRecoveryMultiplierAttribute(),
		UMordecaiAttributeSet::GetPostureRecoveryMultiplierAttribute(),
		UMordecaiAttributeSet::GetSpellPointsRegenMultiplierAttribute(),
		UMordecaiAttributeSet::GetResistancePenetrationMultiplierAttribute(),
		UMordecaiAttributeSet::GetMagicCritChanceAttribute(),
	};
	for (int32 i = 0; i < UE_ARRAY_COUNT(SecondaryDerived); ++i)
	{
		Data.SecondaryDerivedStats[i].MultiplierValue = Read(SecondaryDerived[i]);
		Data.SecondaryDerivedStats[i].bHasValue       = true;
	}

	// Resources
	struct FResourcePair { FGameplayAttribute Current; FGameplayAttribute Max; };
	const FResourcePair Resources[] = {
		{ UMordecaiAttributeSet::GetHealthAttribute(),      UMordecaiAttributeSet::GetMaxHealthAttribute() },
		{ UMordecaiAttributeSet::GetStaminaAttribute(),     UMordecaiAttributeSet::GetMaxStaminaAttribute() },
		{ UMordecaiAttributeSet::GetSpellPointsAttribute(), UMordecaiAttributeSet::GetMaxSpellPointsAttribute() },
		{ UMordecaiAttributeSet::GetPostureAttribute(),     UMordecaiAttributeSet::GetMaxPostureAttribute() },
	};
	for (int32 i = 0; i < UE_ARRAY_COUNT(Resources); ++i)
	{
		Data.CoreResources[i].Current   = Read(Resources[i].Current);
		Data.CoreResources[i].Max       = Read(Resources[i].Max);
		Data.CoreResources[i].bHasValue = true;
	}
}

// ---------------------------------------------------------------------------
// Per-attribute update callbacks
// ---------------------------------------------------------------------------

void UMordecaiCharacterSheetWidget::OnPrimaryAttributeChanged(const FOnAttributeChangeData& ChangeData, int32 PrimaryIndex)
{
	if (!Data.PrimaryAttributes.IsValidIndex(PrimaryIndex))
	{
		return;
	}
	FMordecaiPrimaryAttributeDisplay& Row = Data.PrimaryAttributes[PrimaryIndex];
	Row.BaseValue    = ChangeData.NewValue;
	Row.EffectiveMod = FMordecaiAttributeScaling::CalculateEffectiveMod(ChangeData.NewValue);
	Row.bHasValue    = true;
}

void UMordecaiCharacterSheetWidget::OnPrimaryDerivedChanged(const FOnAttributeChangeData& ChangeData, int32 DerivedIndex)
{
	if (!Data.PrimaryDerivedStats.IsValidIndex(DerivedIndex))
	{
		return;
	}
	Data.PrimaryDerivedStats[DerivedIndex].MultiplierValue = ChangeData.NewValue;
	Data.PrimaryDerivedStats[DerivedIndex].bHasValue       = true;
}

void UMordecaiCharacterSheetWidget::OnSecondaryDerivedChanged(const FOnAttributeChangeData& ChangeData, int32 SecondaryIndex)
{
	if (!Data.SecondaryDerivedStats.IsValidIndex(SecondaryIndex))
	{
		return;
	}
	Data.SecondaryDerivedStats[SecondaryIndex].MultiplierValue = ChangeData.NewValue;
	Data.SecondaryDerivedStats[SecondaryIndex].bHasValue       = true;
}

void UMordecaiCharacterSheetWidget::OnResourceChanged(const FOnAttributeChangeData& ChangeData, int32 ResourceIndex, bool bMax)
{
	if (!Data.CoreResources.IsValidIndex(ResourceIndex))
	{
		return;
	}
	if (bMax)
	{
		Data.CoreResources[ResourceIndex].Max = ChangeData.NewValue;
	}
	else
	{
		Data.CoreResources[ResourceIndex].Current = ChangeData.NewValue;
	}
	Data.CoreResources[ResourceIndex].bHasValue = true;
}

#undef LOCTEXT_NAMESPACE
