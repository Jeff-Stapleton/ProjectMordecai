// Project Mordecai — Character Sheet Widget (US-066)

#include "Mordecai/UI/MordecaiCharacterSheetWidget.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeScaling.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/UI/MordecaiPauseMenuWidget.h"
#include "Mordecai/UI/TinTally/MordecaiUITheme.h"
#include "Mordecai/UI/TinTally/MordecaiUIThemeBinding.h"
#include "Mordecai/UI/TinTally/MordecaiUITypography.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiCharacterSheetWidget)

#define LOCTEXT_NAMESPACE "MordecaiCharacterSheet"

namespace MordecaiCharacterSheetLayout
{
	constexpr float OuterPadding = 16.f;
	constexpr float SectionGap = 12.f;
	constexpr float ColumnGap = 8.f;
	constexpr float ResourceValueWidth = 176.f;
	constexpr float AttributeValueWidth = 72.f;
	constexpr float RowMinHeight = 48.f;
}

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
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultContent();
	}
	RefreshAllText();
}

void UMordecaiCharacterSheetWidget::NativeConstruct()
{
	Super::NativeConstruct();
	MordecaiUIThemeBinding::Bind(
		this,
		BoundThemeSubsystem,
		ThemeChangedHandle,
		this,
		&ThisClass::ApplyTheme);
}

void UMordecaiCharacterSheetWidget::NativeDestruct()
{
	UnbindFromASC();
	MordecaiUIThemeBinding::Unbind(BoundThemeSubsystem, ThemeChangedHandle);
	Super::NativeDestruct();
}

void UMordecaiCharacterSheetWidget::BuildDefaultContent()
{
	using namespace MordecaiCharacterSheetLayout;

	ContentPlate = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("CharacterContentPlate"));
	ContentPlate->SetPadding(FMargin(OuterPadding));
	WidgetTree->RootWidget = ContentPlate;

	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), TEXT("CharacterContent"));
	ContentPlate->SetContent(Root);

	auto MakeText = [this](
		const FName Name,
		const FText& Text,
		const TCHAR* Style,
		const bool bMuted = false) -> UTextBlock*
	{
		UTextBlock* Result = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), Name);
		Result->SetText(Text);
		Result->SetAutoWrapText(true);
		MordecaiUITypography::Apply(Result, Style);
		(bMuted ? MutedTextWidgets : MainTextWidgets).Add(Result);
		return Result;
	};

	CharacterTitle = MakeText(
		TEXT("CharacterTitle"),
		LOCTEXT("CharacterTitle", "Character"),
		TEXT("ui_title"));
	if (UVerticalBoxSlot* TitleSlot = Root->AddChildToVerticalBox(CharacterTitle))
	{
		TitleSlot->SetHorizontalAlignment(HAlign_Left);
		TitleSlot->SetVerticalAlignment(VAlign_Center);
		TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, SectionGap));
	}

	UHorizontalBox* Resources = WidgetTree->ConstructWidget<UHorizontalBox>(
		UHorizontalBox::StaticClass(), TEXT("ResourceRow"));
	if (UVerticalBoxSlot* ResourcesSlot = Root->AddChildToVerticalBox(Resources))
	{
		ResourcesSlot->SetHorizontalAlignment(HAlign_Fill);
		ResourcesSlot->SetPadding(FMargin(0.f, 0.f, 0.f, SectionGap));
	}
	ResourceValueTexts.SetNum(Data.CoreResources.Num());
	for (int32 Index = 0; Index < Data.CoreResources.Num(); ++Index)
	{
		const FMordecaiResourceDisplay& Resource = Data.CoreResources[Index];
		USizeBox* CellSize = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			FName(*FString::Printf(TEXT("ResourceCell_%s"), *Resource.Id.ToString())));
		CellSize->SetMinDesiredWidth(ResourceValueWidth);
		CellSize->SetMinDesiredHeight(72.f);
		UBorder* Cell = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			FName(*FString::Printf(TEXT("ResourcePanel_%s"), *Resource.Id.ToString())));
		Cell->SetPadding(FMargin(12.f, 6.f));
		RaisedPanels.Add(Cell);
		CellSize->SetContent(Cell);
		UVerticalBox* CellColumn = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass());
		Cell->SetContent(CellColumn);

		UTextBlock* Label = MakeText(
			FName(*FString::Printf(TEXT("ResourceLabel_%s"), *Resource.Id.ToString())),
			Resource.DisplayName,
			TEXT("ui_label"),
			true);
		CellColumn->AddChildToVerticalBox(Label);

		UTextBlock* Value = MakeText(
			FName(*FString::Printf(TEXT("ResourceValue_%s"), *Resource.Id.ToString())),
			FText::FromString(FMordecaiCharacterSheetData::PlaceholderText()),
			TEXT("num_m"));
		Value->SetJustification(ETextJustify::Left);
		ResourceValueTexts[Index] = Value;
		if (UVerticalBoxSlot* ValueSlot = CellColumn->AddChildToVerticalBox(Value))
		{
			ValueSlot->SetVerticalAlignment(VAlign_Center);
		}
		if (UHorizontalBoxSlot* CellSlot = Resources->AddChildToHorizontalBox(CellSize))
		{
			CellSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			CellSlot->SetPadding(FMargin(
				Index > 0 ? ColumnGap * 0.5f : 0.f,
				0.f,
				Index + 1 < Data.CoreResources.Num() ? ColumnGap * 0.5f : 0.f,
				0.f));
		}
	}

	UHorizontalBox* Groups = WidgetTree->ConstructWidget<UHorizontalBox>(
		UHorizontalBox::StaticClass(), TEXT("AttributeGroups"));
	if (UVerticalBoxSlot* GroupsSlot = Root->AddChildToVerticalBox(Groups))
	{
		GroupsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		GroupsSlot->SetHorizontalAlignment(HAlign_Fill);
		GroupsSlot->SetVerticalAlignment(VAlign_Fill);
	}

	PrimaryValueTexts.SetNum(Data.PrimaryAttributes.Num());
	const FText GroupLabels[] = {
		Data.PhysicalGroupLabel,
		Data.ResilienceGroupLabel,
		Data.MagicalGroupLabel
	};
	const TCHAR* GroupNames[] = { TEXT("Physical"), TEXT("Resilience"), TEXT("Magical") };
	for (int32 GroupIndex = 0; GroupIndex < 3; ++GroupIndex)
	{
		UVerticalBox* Group = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(),
			FName(*FString::Printf(TEXT("%sGroup"), GroupNames[GroupIndex])));
		UTextBlock* Heading = MakeText(
			FName(*FString::Printf(TEXT("GroupHeading_%s"), GroupNames[GroupIndex])),
			GroupLabels[GroupIndex],
			TEXT("ui_label"),
			true);
		if (UVerticalBoxSlot* HeadingSlot = Group->AddChildToVerticalBox(Heading))
		{
			HeadingSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
			HeadingSlot->SetVerticalAlignment(VAlign_Center);
		}

		for (int32 LocalIndex = 0; LocalIndex < 3; ++LocalIndex)
		{
			const int32 AttributeIndex = GroupIndex * 3 + LocalIndex;
			const FMordecaiPrimaryAttributeDisplay& Attribute =
				Data.PrimaryAttributes[AttributeIndex];
			const FString Code = Attribute.Code.ToString();

			UBorder* Card = WidgetTree->ConstructWidget<UBorder>(
				UBorder::StaticClass(),
				FName(*FString::Printf(TEXT("AttributeCard_%s"), *Code)));
			Card->SetPadding(FMargin(12.f, 8.f));
			RaisedPanels.Add(Card);
			if (UVerticalBoxSlot* CardSlot = Group->AddChildToVerticalBox(Card))
			{
				CardSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
				CardSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			}

			UHorizontalBox* CardRow = WidgetTree->ConstructWidget<UHorizontalBox>(
				UHorizontalBox::StaticClass());
			Card->SetContent(CardRow);

			UVerticalBox* Labels = WidgetTree->ConstructWidget<UVerticalBox>(
				UVerticalBox::StaticClass());
			if (UHorizontalBoxSlot* LabelsSlot = CardRow->AddChildToHorizontalBox(Labels))
			{
				LabelsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				LabelsSlot->SetVerticalAlignment(VAlign_Center);
			}
			UTextBlock* CodeText = MakeText(
				FName(*FString::Printf(TEXT("PrimaryLabel_%s"), *Code)),
				FText::FromName(Attribute.Code),
				TEXT("ui_title"));
			Labels->AddChildToVerticalBox(CodeText);
			UTextBlock* NameText = MakeText(
				FName(*FString::Printf(TEXT("PrimaryName_%s"), *Code)),
				Attribute.DisplayName,
				TEXT("ui_label"),
				true);
			Labels->AddChildToVerticalBox(NameText);

			USizeBox* ValueBox = WidgetTree->ConstructWidget<USizeBox>(
				USizeBox::StaticClass(),
				FName(*FString::Printf(TEXT("PrimaryValueBox_%s"), *Code)));
			ValueBox->SetMinDesiredWidth(AttributeValueWidth);
			ValueBox->SetMinDesiredHeight(RowMinHeight);
			if (UHorizontalBoxSlot* ValueBoxSlot = CardRow->AddChildToHorizontalBox(ValueBox))
			{
				ValueBoxSlot->SetVerticalAlignment(VAlign_Center);
			}
			UTextBlock* Value = MakeText(
				FName(*FString::Printf(TEXT("PrimaryValue_%s"), *Code)),
				FText::FromString(FMordecaiCharacterSheetData::PlaceholderText()),
				TEXT("num_l"));
			Value->SetJustification(ETextJustify::Right);
			PrimaryValueTexts[AttributeIndex] = Value;
			if (USizeBoxSlot* ValueSlot = Cast<USizeBoxSlot>(ValueBox->AddChild(Value)))
			{
				ValueSlot->SetHorizontalAlignment(HAlign_Fill);
				ValueSlot->SetVerticalAlignment(VAlign_Center);
			}
		}

		if (UHorizontalBoxSlot* GroupSlot = Groups->AddChildToHorizontalBox(Group))
		{
			GroupSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			GroupSlot->SetPadding(FMargin(
				GroupIndex > 0 ? ColumnGap * 0.5f : 0.f,
				0.f,
				GroupIndex < 2 ? ColumnGap * 0.5f : 0.f,
				0.f));
		}
	}

	ApplyTheme(
		EMordecaiUITheme::Diorama,
		FMordecaiUIThemeColors::MakeDiorama());
}

void UMordecaiCharacterSheetWidget::ApplyTheme(
	EMordecaiUITheme Theme,
	const FMordecaiUIThemeColors& Colors)
{
	if (ContentPlate)
	{
		ContentPlate->SetBrushColor(Colors.Plate);
	}
	for (UBorder* Panel : RaisedPanels)
	{
		if (Panel)
		{
			Panel->SetBrushColor(Colors.PlateRaised);
		}
	}
	for (UTextBlock* Text : MainTextWidgets)
	{
		if (Text)
		{
			Text->SetColorAndOpacity(FSlateColor(Colors.Text));
		}
	}
	for (UTextBlock* Text : MutedTextWidgets)
	{
		if (Text)
		{
			Text->SetColorAndOpacity(FSlateColor(Colors.TextMuted));
		}
	}
}

void UMordecaiCharacterSheetWidget::RefreshAllText()
{
	for (int32 Index = 0; Index < PrimaryValueTexts.Num(); ++Index)
	{
		RefreshPrimaryText(Index);
	}
	for (int32 Index = 0; Index < ResourceValueTexts.Num(); ++Index)
	{
		RefreshResourceText(Index);
	}
}

void UMordecaiCharacterSheetWidget::RefreshPrimaryText(const int32 PrimaryIndex)
{
	if (!Data.PrimaryAttributes.IsValidIndex(PrimaryIndex)
		|| !PrimaryValueTexts.IsValidIndex(PrimaryIndex)
		|| !PrimaryValueTexts[PrimaryIndex])
	{
		return;
	}
	const FMordecaiPrimaryAttributeDisplay& Row = Data.PrimaryAttributes[PrimaryIndex];
	PrimaryValueTexts[PrimaryIndex]->SetText(FText::FromString(
		Row.bHasValue
			? FString::FromInt(FMath::RoundToInt(Row.BaseValue))
			: FMordecaiCharacterSheetData::PlaceholderText()));
}

void UMordecaiCharacterSheetWidget::RefreshResourceText(const int32 ResourceIndex)
{
	if (!Data.CoreResources.IsValidIndex(ResourceIndex)
		|| !ResourceValueTexts.IsValidIndex(ResourceIndex)
		|| !ResourceValueTexts[ResourceIndex])
	{
		return;
	}
	const FMordecaiResourceDisplay& Row = Data.CoreResources[ResourceIndex];
	ResourceValueTexts[ResourceIndex]->SetText(FText::FromString(
		Row.bHasValue
			? FMordecaiCharacterSheetData::FormatResource(Row.Current, Row.Max)
			: FMordecaiCharacterSheetData::PlaceholderText()));
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
	RefreshAllText();
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
	RefreshAllText();
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
	RefreshPrimaryText(PrimaryIndex);
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
	RefreshResourceText(ResourceIndex);
}

#undef LOCTEXT_NAMESPACE
