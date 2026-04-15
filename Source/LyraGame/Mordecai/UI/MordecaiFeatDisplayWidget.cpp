// Project Mordecai — Feat Display Widget (US-068)

#include "Mordecai/UI/MordecaiFeatDisplayWidget.h"
#include "Mordecai/Feats/MordecaiFeatComponent.h"
#include "Mordecai/Feats/MordecaiFeatDataAsset.h"
#include "Mordecai/Skills/MordecaiSkillComponent.h"
#include "Mordecai/UI/MordecaiPauseMenuWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiFeatDisplayWidget)

#define LOCTEXT_NAMESPACE "MordecaiFeatDisplay"

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

FText UMordecaiFeatDisplayWidget::GetFeatsTabDisplayName()
{
	return LOCTEXT("FeatsTabName", "Feats");
}

void UMordecaiFeatDisplayWidget::RegisterWithPauseMenu(UMordecaiPauseMenuWidget* PauseMenu, TSubclassOf<UMordecaiFeatDisplayWidget> WidgetClass)
{
	if (!PauseMenu)
	{
		return;
	}
	TSubclassOf<UUserWidget> ContentClass = WidgetClass ? *WidgetClass : UMordecaiFeatDisplayWidget::StaticClass();
	PauseMenu->RegisterTab(GetFeatsTabId(), GetFeatsTabDisplayName(), ContentClass);
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void UMordecaiFeatDisplayWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// Initialize default tier colors per spec
	TierColors.Add(EMordecaiFeatTier::Common, FLinearColor(0.8f, 0.8f, 0.8f));
	TierColors.Add(EMordecaiFeatTier::Rare, FLinearColor(0.3f, 0.5f, 1.0f));
	TierColors.Add(EMordecaiFeatTier::Legendary, FLinearColor(1.0f, 0.75f, 0.0f));
}

void UMordecaiFeatDisplayWidget::NativeDestruct()
{
	UnbindFromFeatComponent();
	Super::NativeDestruct();
}

// ---------------------------------------------------------------------------
// Binding
// ---------------------------------------------------------------------------

void UMordecaiFeatDisplayWidget::BindToFeatComponent(UMordecaiFeatComponent* InComponent)
{
	UnbindFromFeatComponent();
	if (!InComponent)
	{
		return;
	}

	BoundComponent = InComponent;
	InComponent->OnFeatUnlockedBP.AddDynamic(this, &UMordecaiFeatDisplayWidget::HandleFeatUnlocked);
}

void UMordecaiFeatDisplayWidget::UnbindFromFeatComponent()
{
	if (UMordecaiFeatComponent* Comp = BoundComponent.Get())
	{
		Comp->OnFeatUnlockedBP.RemoveDynamic(this, &UMordecaiFeatDisplayWidget::HandleFeatUnlocked);
	}
	BoundComponent.Reset();
}

void UMordecaiFeatDisplayWidget::HandleFeatUnlocked(FName FeatName)
{
	LastUnlockedFeatName = FeatName;
}

// ---------------------------------------------------------------------------
// Unlocked feats
// ---------------------------------------------------------------------------

TArray<FMordecaiFeatDisplayRow> UMordecaiFeatDisplayWidget::GetUnlockedFeatRows() const
{
	TArray<FMordecaiFeatDisplayRow> Result;

	UMordecaiFeatComponent* Comp = BoundComponent.Get();
	if (!Comp)
	{
		return Result;
	}

	const TSet<FName>& UnlockedNames = Comp->GetUnlockedFeats();
	const auto& RegisteredFeats = Comp->GetRegisteredFeats();

	Result.Reserve(UnlockedNames.Num());
	for (const FName& FeatName : UnlockedNames)
	{
		const TObjectPtr<UMordecaiFeatDataAsset>* AssetPtr = RegisteredFeats.Find(FeatName);
		if (!AssetPtr || !(*AssetPtr))
		{
			continue;
		}

		const UMordecaiFeatDataAsset* Asset = *AssetPtr;
		FMordecaiFeatDisplayRow Row;
		Row.FeatName = Asset->FeatName;
		Row.DisplayName = Asset->DisplayName;
		Row.Description = Asset->Description;
		Row.Tier = Asset->Tier;
		Row.bUnlocked = true;
		Row.AppliedRecord = Comp->GetFeatAppliedRecord(FeatName);
		Result.Add(Row);
	}

	return Result;
}

// ---------------------------------------------------------------------------
// Locked feats
// ---------------------------------------------------------------------------

TArray<FMordecaiFeatDisplayRow> UMordecaiFeatDisplayWidget::GetLockedFeatRows() const
{
	TArray<FMordecaiFeatDisplayRow> Result;

	UMordecaiFeatComponent* Comp = BoundComponent.Get();
	if (!Comp)
	{
		return Result;
	}

	const TSet<FName>& UnlockedNames = Comp->GetUnlockedFeats();
	const auto& RegisteredFeats = Comp->GetRegisteredFeats();

	for (const auto& Pair : RegisteredFeats)
	{
		if (UnlockedNames.Contains(Pair.Key))
		{
			continue; // Skip unlocked feats
		}

		const UMordecaiFeatDataAsset* Asset = Pair.Value;
		if (!Asset)
		{
			continue;
		}

		FMordecaiFeatDisplayRow Row;
		Row.FeatName = Asset->FeatName;
		Row.DisplayName = Asset->DisplayName;
		Row.Description = LOCTEXT("LockedFeatDescription", "???");
		Row.Tier = Asset->Tier;
		Row.bUnlocked = false;
		Row.ConditionProgress = BuildConditionProgress(Asset->UnlockConditions);
		Result.Add(Row);
	}

	return Result;
}

// ---------------------------------------------------------------------------
// Summary
// ---------------------------------------------------------------------------

FMordecaiFeatDisplaySummary UMordecaiFeatDisplayWidget::GetSummary() const
{
	FMordecaiFeatDisplaySummary Summary;

	UMordecaiFeatComponent* Comp = BoundComponent.Get();
	if (!Comp)
	{
		return Summary;
	}

	const TSet<FName>& UnlockedNames = Comp->GetUnlockedFeats();
	const auto& RegisteredFeats = Comp->GetRegisteredFeats();

	Summary.TotalUnlocked = UnlockedNames.Num();

	for (const FName& FeatName : UnlockedNames)
	{
		const TObjectPtr<UMordecaiFeatDataAsset>* AssetPtr = RegisteredFeats.Find(FeatName);
		if (!AssetPtr || !(*AssetPtr))
		{
			continue;
		}

		switch ((*AssetPtr)->Tier)
		{
		case EMordecaiFeatTier::Common:    Summary.CommonCount++;    break;
		case EMordecaiFeatTier::Rare:      Summary.RareCount++;      break;
		case EMordecaiFeatTier::Legendary: Summary.LegendaryCount++; break;
		}
	}

	return Summary;
}

// ---------------------------------------------------------------------------
// Detail panel
// ---------------------------------------------------------------------------

void UMordecaiFeatDisplayWidget::SetSelectedFeat(FName FeatName)
{
	SelectedFeat = FeatName;
}

FMordecaiFeatDisplayRow UMordecaiFeatDisplayWidget::GetSelectedFeatDetail() const
{
	FMordecaiFeatDisplayRow Detail;

	UMordecaiFeatComponent* Comp = BoundComponent.Get();
	if (!Comp || SelectedFeat.IsNone())
	{
		return Detail;
	}

	const auto& RegisteredFeats = Comp->GetRegisteredFeats();
	const TObjectPtr<UMordecaiFeatDataAsset>* AssetPtr = RegisteredFeats.Find(SelectedFeat);
	if (!AssetPtr || !(*AssetPtr))
	{
		return Detail;
	}

	const UMordecaiFeatDataAsset* Asset = *AssetPtr;
	Detail.FeatName = Asset->FeatName;
	Detail.DisplayName = Asset->DisplayName;
	Detail.Tier = Asset->Tier;

	if (Comp->HasFeat(SelectedFeat))
	{
		// Unlocked: show full description and applied record
		Detail.bUnlocked = true;
		Detail.Description = Asset->Description;
		Detail.AppliedRecord = Comp->GetFeatAppliedRecord(SelectedFeat);
	}
	else
	{
		// Locked: show teaser and condition progress
		Detail.bUnlocked = false;
		Detail.Description = LOCTEXT("LockedFeatDescription", "???");
		Detail.ConditionProgress = BuildConditionProgress(Asset->UnlockConditions);
	}

	return Detail;
}

// ---------------------------------------------------------------------------
// Tier colors
// ---------------------------------------------------------------------------

FLinearColor UMordecaiFeatDisplayWidget::GetTierColor(EMordecaiFeatTier Tier) const
{
	const FLinearColor* Color = TierColors.Find(Tier);
	if (Color)
	{
		return *Color;
	}

	// Fallback defaults (same as spec, in case TierColors wasn't initialized via NativeOnInitialized)
	switch (Tier)
	{
	case EMordecaiFeatTier::Common:    return FLinearColor(0.8f, 0.8f, 0.8f);
	case EMordecaiFeatTier::Rare:      return FLinearColor(0.3f, 0.5f, 1.0f);
	case EMordecaiFeatTier::Legendary: return FLinearColor(1.0f, 0.75f, 0.0f);
	default:                           return FLinearColor::White;
	}
}

// ---------------------------------------------------------------------------
// Condition progress
// ---------------------------------------------------------------------------

TArray<FMordecaiFeatConditionProgress> UMordecaiFeatDisplayWidget::BuildConditionProgress(
	const TArray<FMordecaiFeatCondition>& Conditions) const
{
	TArray<FMordecaiFeatConditionProgress> Result;

	UMordecaiFeatComponent* Comp = BoundComponent.Get();
	if (!Comp)
	{
		return Result;
	}

	Result.Reserve(Conditions.Num());
	for (const FMordecaiFeatCondition& Cond : Conditions)
	{
		FMordecaiFeatConditionProgress Prog;
		Prog.ConditionType = Cond.ConditionType;
		Prog.ThresholdValue = Cond.ThresholdValue;

		switch (Cond.ConditionType)
		{
		case EMordecaiFeatConditionType::StatThreshold:
		case EMordecaiFeatConditionType::EventCount:
		{
			Prog.StatName = Cond.StatName;
			Prog.CurrentValue = Comp->GetFeatStatValue(Cond.StatName);
			Prog.bMet = (Prog.CurrentValue >= Cond.ThresholdValue);
			break;
		}

		case EMordecaiFeatConditionType::SkillRank:
		{
			Prog.StatName = Cond.StatName;
			UMordecaiSkillComponent* SkillComp = FindSkillComponent();
			Prog.CurrentValue = SkillComp ? SkillComp->GetSkillRank(Cond.StatName) : 0;
			Prog.bMet = (Prog.CurrentValue >= Cond.ThresholdValue);
			break;
		}

		case EMordecaiFeatConditionType::TagPresent:
		{
			Prog.RequiredTag = Cond.RequiredTag;
			Prog.CurrentValue = Comp->HasGrantedFeatTag(Cond.RequiredTag) ? 1 : 0;
			Prog.ThresholdValue = 1;
			Prog.bMet = (Prog.CurrentValue >= 1);
			break;
		}
		}

		Result.Add(Prog);
	}

	return Result;
}

UMordecaiSkillComponent* UMordecaiFeatDisplayWidget::FindSkillComponent() const
{
	UMordecaiFeatComponent* Comp = BoundComponent.Get();
	return Comp ? Comp->GetSkillComponent() : nullptr;
}

#undef LOCTEXT_NAMESPACE
