// Project Mordecai — Recovery: pause-menu Character, Skills, and Feats content (US-093).

#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "HAL/PlatformMisc.h"
#include "Internationalization/Text.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/Feats/MordecaiFeatComponent.h"
#include "Mordecai/Feats/MordecaiFeatDataAsset.h"
#include "Mordecai/MordecaiPlayerState.h"
#include "Mordecai/Skills/MordecaiSkillComponent.h"
#include "Mordecai/Skills/MordecaiSkillDataAsset.h"
#include "Mordecai/UI/MordecaiCharacterSheetWidget.h"
#include "Mordecai/UI/MordecaiFeatDisplayWidget.h"
#include "Mordecai/UI/MordecaiPauseMenuWidget.h"
#include "Mordecai/UI/MordecaiSkillTreeWidget.h"
#include "Mordecai/UI/MordecaiTabButton.h"
#include "Mordecai/UI/TinTally/MordecaiUITheme.h"
#include "Mordecai/UI/TinTally/MordecaiUIThemeSubsystem.h"
#include "Tests/Recovery/MordecaiRecoveryTestEnv.h"

namespace MordecaiRecoveryPauseMenuContentTest
{
	using namespace MordecaiRecoveryTest;

	template <typename T>
	T* Find(const UUserWidget* Owner, const TCHAR* Name)
	{
		return Owner && Owner->WidgetTree
			? Cast<T>(Owner->WidgetTree->FindWidget(FName(Name)))
			: nullptr;
	}

	bool HasText(const UUserWidget* Owner, const TCHAR* Name, const FString& Expected)
	{
		const UTextBlock* Text = Find<UTextBlock>(Owner, Name);
		return Text && Text->GetText().ToString() == Expected;
	}

	bool HasThemeTextColor(
		const UUserWidget* Owner,
		const TCHAR* Name,
		const FLinearColor& Expected)
	{
		const UTextBlock* Text = Find<UTextBlock>(Owner, Name);
		return Text
			&& Text->GetColorAndOpacity().GetSpecifiedColor().Equals(Expected);
	}

	bool HasThemeBrushColor(
		const UUserWidget* Owner,
		const TCHAR* Name,
		const FLinearColor& Expected)
	{
		const UBorder* Border = Find<UBorder>(Owner, Name);
		return Border && Border->GetBrushColor().Equals(Expected);
	}

	struct FThemedWidgetEnv : FRecoveryControlledTestEnv
	{
		bool SetupThemed()
		{
			if (!SetupWithController())
			{
				return false;
			}
			LocalPlayer->PlayerAdded(nullptr, FPlatformMisc::GetPlatformUserForUserIndex(0));
			return LocalPlayer->GetSubsystem<UMordecaiUIThemeSubsystem>() != nullptr;
		}

		void TeardownThemed()
		{
			LocalPlayer->PlayerRemoved();
			TeardownWithController();
		}
	};

	UMordecaiSkillDataAsset* MakeSkill(
		UObject* Outer,
		const FName SkillName,
		const EMordecaiSkillCategory Category)
	{
		UMordecaiSkillDataAsset* Skill = NewObject<UMordecaiSkillDataAsset>(Outer);
		Skill->SkillName = SkillName;
		Skill->DisplayName = FText::FromName(SkillName);
		Skill->Category = Category;
		return Skill;
	}

	UMordecaiFeatDataAsset* MakeFeat(UObject* Outer, const FName FeatName)
	{
		UMordecaiFeatDataAsset* Feat = NewObject<UMordecaiFeatDataAsset>(Outer);
		Feat->FeatName = FeatName;
		Feat->DisplayName = FText::FromName(FeatName);
		Feat->Description = FText::FromString(TEXT("A recovery-test feat."));
		Feat->Tier = EMordecaiFeatTier::Common;

		FMordecaiFeatCondition Condition;
		Condition.ConditionType = EMordecaiFeatConditionType::EventCount;
		Condition.StatName = TEXT("RecoveryFeatEvents");
		Condition.ThresholdValue = 1;
		Feat->UnlockConditions.Add(Condition);
		return Feat;
	}
}

using namespace MordecaiRecoveryPauseMenuContentTest;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_CharacterSheet_BuildsDefaultContent,
	"Mordecai.Recovery.CharacterSheet.BuildsDefaultContent",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_CharacterSheet_BuildsDefaultContent::RunTest(const FString& Parameters)
{
	FThemedWidgetEnv Env;
	if (!Env.SetupThemed())
	{
		AddError(TEXT("Themed controlled test environment setup failed"));
		return false;
	}
	ON_SCOPE_EXIT { Env.TeardownThemed(); };

	UMordecaiCharacterSheetWidget* Widget = Env.MakeWidget<UMordecaiCharacterSheetWidget>();
	TestNotNull("Character sheet widget", Widget);
	if (!Widget)
	{
		return false;
	}

	Widget->BindToASC(Env.PlayerState->GetAbilitySystemComponent());
	TSharedRef<SWidget> SlateWidget = Widget->TakeWidget();

	UAbilitySystemComponent* ASC = Env.PlayerState->GetAbilitySystemComponent();
	const FGameplayAttribute PrimaryAttributes[] = {
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
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(PrimaryAttributes); ++Index)
	{
		const float Current = ASC->GetNumericAttribute(PrimaryAttributes[Index]);
		ASC->ApplyModToAttribute(
			PrimaryAttributes[Index],
			EGameplayModOp::Additive,
			static_cast<float>(Index + 11) - Current);
	}

	const TCHAR* GroupHeadings[] = {
		TEXT("GroupHeading_Physical"),
		TEXT("GroupHeading_Resilience"),
		TEXT("GroupHeading_Magical")
	};
	const TCHAR* GroupLabels[] = { TEXT("Physical"), TEXT("Resilience"), TEXT("Magical") };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(GroupHeadings); ++Index)
	{
		TestTrue(
			*FString::Printf(TEXT("%s heading is rendered"), GroupLabels[Index]),
			HasText(Widget, GroupHeadings[Index], GroupLabels[Index]));
	}

	const TCHAR* AttributeCodes[] = {
		TEXT("STR"), TEXT("DEX"), TEXT("END"),
		TEXT("CON"), TEXT("RES"), TEXT("DIS"),
		TEXT("INT"), TEXT("WIS"), TEXT("CHA")
	};
	const TCHAR* AttributeLookupKeys[] = {
		TEXT("strength"), TEXT("dexterity"), TEXT("endurance"),
		TEXT("constitution"), TEXT("resistance"), TEXT("discipline"),
		TEXT("intelligence"), TEXT("wisdom"), TEXT("charisma")
	};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(AttributeCodes); ++Index)
	{
		const FString LabelName = FString::Printf(
			TEXT("PrimaryLabel_%s"), AttributeLookupKeys[Index]);
		const FString ValueName = FString::Printf(
			TEXT("PrimaryValue_%s"), AttributeLookupKeys[Index]);
		TestTrue(
			*FString::Printf(TEXT("%s label uses the stable lookup key"), AttributeCodes[Index]),
			Find<UTextBlock>(Widget, *LabelName)
				&& Find<UTextBlock>(Widget, *LabelName)->GetText().ToString().Equals(
					AttributeCodes[Index],
					ESearchCase::CaseSensitive));
		TestNull(
			*FString::Printf(TEXT("%s localized code is not a widget identifier"), AttributeCodes[Index]),
			Find<UTextBlock>(
				Widget,
				*FString::Printf(TEXT("PrimaryLabel_%s"), AttributeCodes[Index])));
		TestTrue(
			*FString::Printf(TEXT("%s distinct live value is rendered"), AttributeCodes[Index]),
			HasText(
				Widget,
				*ValueName,
				FString::FromInt(Index + 11)));
		const FString EffectiveModName =
			FString::Printf(TEXT("PrimaryEffectiveMod_%s"), AttributeLookupKeys[Index]);
		TestTrue(
			*FString::Printf(TEXT("%s effective modifier is rendered"), AttributeCodes[Index]),
			HasText(
				Widget,
				*EffectiveModName,
				FMordecaiCharacterSheetData::FormatEffectiveModBonus(
					Widget->GetData().PrimaryAttributes[Index].EffectiveMod)));
	}

	const TCHAR* ResourceIds[] = {
		TEXT("health"), TEXT("stamina"), TEXT("spellpoints"), TEXT("posture")
	};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(ResourceIds); ++Index)
	{
		const FMordecaiResourceDisplay& Resource = Widget->GetData().CoreResources[Index];
		const FString ValueName = FString::Printf(TEXT("ResourceValue_%s"), ResourceIds[Index]);
		TestTrue(
			*FString::Printf(TEXT("%s current / max text is rendered"), ResourceIds[Index]),
			HasText(
				Widget,
				*ValueName,
				FMordecaiCharacterSheetData::FormatResource(Resource.Current, Resource.Max)));
	}

	UTextBlock* HealthText = Find<UTextBlock>(Widget, TEXT("ResourceValue_health"));
	const FString HealthBefore = HealthText ? HealthText->GetText().ToString() : FString();
	ASC->ApplyModToAttribute(
		UMordecaiAttributeSet::GetHealthAttribute(),
		EGameplayModOp::Additive,
		-7.f);
	const FString HealthExpected = FMordecaiCharacterSheetData::FormatResource(
		Widget->GetData().CoreResources[0].Current,
		Widget->GetData().CoreResources[0].Max);
	TestTrue(
		"Health text refreshes from the ASC delegate",
		HealthText
			&& HealthText->GetText().ToString() == HealthExpected
			&& HealthText->GetText().ToString() != HealthBefore);

	UMordecaiUIThemeSubsystem* Themes =
		Env.LocalPlayer->GetSubsystem<UMordecaiUIThemeSubsystem>();
	Themes->SetActiveTheme(EMordecaiUITheme::HighContrast);
	TestTrue(
		"Character title follows the live High Contrast text token",
		HasThemeTextColor(
			Widget,
			TEXT("CharacterTitle"),
			Themes->GetActiveColors().Text));
	TestTrue(
		"Character content plate follows the live High Contrast plate token",
		HasThemeBrushColor(
			Widget,
			TEXT("CharacterContentPlate"),
			Themes->GetActiveColors().Plate));
	TestTrue(
		"Character attribute row follows the live High Contrast raised-plate token",
		HasThemeBrushColor(
			Widget,
			TEXT("AttributeCard_strength"),
			Themes->GetActiveColors().PlateRaised));
	TestTrue(
		"Character muted name follows the live High Contrast muted-text token",
		HasThemeTextColor(
			Widget,
			TEXT("PrimaryName_strength"),
			Themes->GetActiveColors().TextMuted));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_PauseMenu_SkillsAndFeatsTabsRegisteredAndBound,
	"Mordecai.Recovery.PauseMenu.SkillsAndFeatsTabsRegisteredAndBound",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_PauseMenu_SkillsAndFeatsTabsRegisteredAndBound::RunTest(
	const FString& Parameters)
{
	FRecoveryControlledTestEnv Env;
	if (!Env.SetupWithController())
	{
		AddError(TEXT("Controlled test environment setup failed"));
		return false;
	}
	ON_SCOPE_EXIT { Env.TeardownWithController(); };

	UMordecaiPauseMenuWidget* Menu = Env.MakeWidget<UMordecaiPauseMenuWidget>();
	TestNotNull("Pause menu widget", Menu);
	if (!Menu)
	{
		return false;
	}

	const FMordecaiPauseMenuTabEntry* Skills =
		Menu->GetTabModel().FindTab(UMordecaiSkillTreeWidget::GetSkillsTabId());
	const FMordecaiPauseMenuTabEntry* Feats =
		Menu->GetTabModel().FindTab(UMordecaiFeatDisplayWidget::GetFeatsTabId());
	TestTrue(
		"Skills tab registers the real skill tree class",
		Skills && Skills->ContentWidgetClass == UMordecaiSkillTreeWidget::StaticClass());
	TestTrue(
		"Feats tab registers the real feat display class",
		Feats && Feats->ContentWidgetClass == UMordecaiFeatDisplayWidget::StaticClass());

	Menu->SetActiveTab(UMordecaiSkillTreeWidget::GetSkillsTabId());
	const UMordecaiSkillTreeWidget* SkillContent =
		Cast<UMordecaiSkillTreeWidget>(Menu->GetCurrentContentWidget());
	TestTrue(
		"Skills content is bound to the player's SkillComponent",
		SkillContent && SkillContent->IsBoundToComponent());

	Menu->SetActiveTab(UMordecaiFeatDisplayWidget::GetFeatsTabId());
	const UMordecaiFeatDisplayWidget* FeatContent =
		Cast<UMordecaiFeatDisplayWidget>(Menu->GetCurrentContentWidget());
	TestTrue(
		"Feats content is bound to the player's FeatComponent",
		FeatContent && FeatContent->IsBoundToComponent());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_SkillTree_BuildsDefaultContent,
	"Mordecai.Recovery.SkillTree.BuildsDefaultContent",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_SkillTree_BuildsDefaultContent::RunTest(const FString& Parameters)
{
	FThemedWidgetEnv Env;
	if (!Env.SetupThemed())
	{
		AddError(TEXT("Themed controlled test environment setup failed"));
		return false;
	}
	ON_SCOPE_EXIT { Env.TeardownThemed(); };

	UMordecaiSkillComponent* Skills = Env.PlayerState->GetSkillComponent();
	Skills->SetSkillRank(TEXT("Longswords"), 7);

	UMordecaiSkillTreeWidget* Widget = Env.MakeWidget<UMordecaiSkillTreeWidget>();
	Widget->BindToSkillComponent(Skills);
	TSharedRef<SWidget> SlateWidget = Widget->TakeWidget();

	TestTrue(
		"Weapon category is rendered",
		HasText(Widget, TEXT("Category_Weapon"), TEXT("Weapon")));
	TestTrue(
		"Longswords row is built from the bound SkillComponent",
		Widget->FindSkillNameText(TEXT("Longswords"))
			&& Widget->FindSkillNameText(TEXT("Longswords"))
				->GetText().ToString().Equals(
					TEXT("Longswords"),
					ESearchCase::CaseSensitive));
	TestTrue(
		"Longswords row renders rank / 20",
		Widget->FindSkillRankText(TEXT("Longswords"))
			&& Widget->FindSkillRankText(TEXT("Longswords"))
				->GetText().ToString() == TEXT("7 / 20"));
	TestTrue(
		"Skill points label does not auto-wrap",
		Find<UTextBlock>(Widget, TEXT("SkillPointsText"))
			&& !Find<UTextBlock>(Widget, TEXT("SkillPointsText"))->GetAutoWrapText());
	TestTrue(
		"Rank value does not auto-wrap",
		Widget->FindSkillRankText(TEXT("Longswords"))
			&& !Widget->FindSkillRankText(TEXT("Longswords"))->GetAutoWrapText());
	TestTrue(
		"Rank value box has a fixed width override",
		Widget->FindSkillRankBox(TEXT("Longswords"))
			&& Widget->FindSkillRankBox(TEXT("Longswords"))->GetWidthOverride() > 0.f);
	USizeBox* CategoryRailBox = Find<USizeBox>(Widget, TEXT("CategoryRailBox"));
	UVerticalBox* CategoryRail = Find<UVerticalBox>(Widget, TEXT("CategoryRail"));
	TestTrue(
		"Category rail is the specified 198px vertical rail",
		CategoryRailBox
			&& CategoryRailBox->GetWidthOverride() == 198.f
			&& CategoryRail
			&& CategoryRail->GetChildrenCount() == 5);
	const TCHAR* CategoryNames[] = {
		TEXT("Weapon"),
		TEXT("DefenseArmor"),
		TEXT("MovementExploration"),
		TEXT("UtilityInteraction"),
		TEXT("Magic")
	};
	for (const TCHAR* CategoryName : CategoryNames)
	{
		const FString SizeName = FString::Printf(TEXT("CategorySize_%s"), CategoryName);
		const FString TextName = FString::Printf(TEXT("Category_%s"), CategoryName);
		const USizeBox* CategorySize = Find<USizeBox>(Widget, *SizeName);
		const UTextBlock* CategoryText = Find<UTextBlock>(Widget, *TextName);
		TestTrue(
			*FString::Printf(TEXT("%s category has equal 68px height"), CategoryName),
			CategorySize && CategorySize->GetHeightOverride() == 68.f);
		TestTrue(
			*FString::Printf(TEXT("%s category label deliberately wraps"), CategoryName),
			CategoryText && CategoryText->GetAutoWrapText());
		TestTrue(
			*FString::Printf(TEXT("%s category label carries localization metadata"), CategoryName),
			CategoryText
				&& FTextInspector::GetNamespace(CategoryText->GetText()).IsSet()
				&& FTextInspector::GetKey(CategoryText->GetText()).IsSet());
	}

	UMordecaiTabButton* MagicButton =
		Find<UMordecaiTabButton>(Widget, TEXT("CategoryButton_Magic"));
	TestNotNull("Magic category is a real button", MagicButton);
	if (MagicButton)
	{
		MagicButton->OnClicked.Broadcast();
	}
	TestTrue(
		"Clicking Magic switches the active category",
		Widget->GetActiveCategory() == EMordecaiSkillCategory::Magic);
	TestTrue(
		"Magic category renders its real sheet count",
		HasText(Widget, TEXT("CategoryMetaText"), TEXT("Magic \u00b7 11 skills")));
	TestTrue(
		"Magic category renders Fireball",
		Widget->FindSkillNameText(TEXT("Fireball"))
			&& Widget->FindSkillNameText(TEXT("Fireball"))
				->GetText().ToString().Equals(
					TEXT("Fireball"),
					ESearchCase::CaseSensitive));

	Widget->SetActiveCategory(EMordecaiSkillCategory::Weapon);
	Skills->SetSkillRank(TEXT("Longswords"), 8);
	Skills->SetAvailableSkillPoints(12);
	TestTrue(
		"Non-milestone rank changes refresh the visible row",
		Widget->FindSkillRankText(TEXT("Longswords"))
			&& Widget->FindSkillRankText(TEXT("Longswords"))
				->GetText().ToString() == TEXT("8 / 20"));
	TestTrue(
		"Available skill-point changes refresh the header",
		HasText(Widget, TEXT("SkillPointsText"), TEXT("Skill Points: 12")));

	UTextBlock* RowBeforeRedundantBind =
		Widget->FindSkillRankText(TEXT("Longswords"));
	const int32 RebuildsBeforeRedundantBind = Widget->GetContentRebuildCount();
	Widget->BindToSkillComponent(Skills);
	TestTrue(
		"Binding the same component does not rebuild rows",
		Widget->FindSkillRankText(TEXT("Longswords")) == RowBeforeRedundantBind
			&& Widget->GetContentRebuildCount() == RebuildsBeforeRedundantBind);
	UTextBlock* RowBeforeUnbind =
		Widget->FindSkillRankText(TEXT("Longswords"));
	const int32 RebuildsBeforeUnbind = Widget->GetContentRebuildCount();
	Widget->UnbindFromSkillComponent();
	TestTrue(
		"Unbind only removes subscriptions and does not rebuild rows",
		Widget->FindSkillRankText(TEXT("Longswords")) == RowBeforeUnbind
			&& Widget->GetContentRebuildCount() == RebuildsBeforeUnbind);

	UMordecaiUIThemeSubsystem* Themes =
		Env.LocalPlayer->GetSubsystem<UMordecaiUIThemeSubsystem>();
	Widget->BindToSkillComponent(Skills);
	TestTrue(
		"Reopening and rebinding the same PlayerState component reuses rows",
		Widget->FindSkillRankText(TEXT("Longswords")) == RowBeforeUnbind
			&& Widget->GetContentRebuildCount() == RebuildsBeforeUnbind);
	Themes->SetActiveTheme(EMordecaiUITheme::HighContrast);
	Widget->SetActiveCategory(EMordecaiSkillCategory::Magic);
	Widget->SetActiveCategory(EMordecaiSkillCategory::Weapon);
	TestTrue(
		"Skills title follows the live High Contrast text token",
		HasThemeTextColor(
			Widget,
			TEXT("SkillsTitle"),
			Themes->GetActiveColors().Text));
	TestTrue(
		"Skills content plate follows the live High Contrast plate token",
		HasThemeBrushColor(
			Widget,
			TEXT("SkillContentPlate"),
			Themes->GetActiveColors().Plate));
	TestTrue(
		"Selected category uses the cream enamel fill token",
		Find<UMordecaiTabButton>(Widget, TEXT("CategoryButton_Weapon"))
			&& Find<UMordecaiTabButton>(Widget, TEXT("CategoryButton_Weapon"))
				->GetStyle().Normal.TintColor.GetSpecifiedColor().Equals(
					Themes->GetActiveColors().Enamel)
			&& Find<UMordecaiTabButton>(Widget, TEXT("CategoryButton_Weapon"))
				->GetStyle().Hovered.TintColor.GetSpecifiedColor().Equals(
					Themes->GetActiveColors().Enamel)
			&& Find<UMordecaiTabButton>(Widget, TEXT("CategoryButton_Weapon"))
				->GetStyle().Pressed.TintColor.GetSpecifiedColor().Equals(
					Themes->GetActiveColors().Enamel));
	TestTrue(
		"Skills row keeps the live High Contrast raised-plate token after category rebuild",
		Widget->FindSkillRowPanel(TEXT("Longswords"))
			&& Widget->FindSkillRowPanel(TEXT("Longswords"))
				->GetBrushColor().Equals(
					Themes->GetActiveColors().PlateRaised));
	TestTrue(
		"Skills muted category metadata keeps the live High Contrast muted-text token after category rebuild",
		HasThemeTextColor(
			Widget,
			TEXT("CategoryMetaText"),
			Themes->GetActiveColors().TextMuted));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_FeatDisplay_BuildsDefaultContent,
	"Mordecai.Recovery.FeatDisplay.BuildsDefaultContent",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_FeatDisplay_BuildsDefaultContent::RunTest(const FString& Parameters)
{
	FThemedWidgetEnv Env;
	if (!Env.SetupThemed())
	{
		AddError(TEXT("Themed controlled test environment setup failed"));
		return false;
	}
	ON_SCOPE_EXIT { Env.TeardownThemed(); };

	UMordecaiFeatComponent* Feats = Env.PlayerState->GetFeatComponent();
	UMordecaiFeatDisplayWidget* Widget = Env.MakeWidget<UMordecaiFeatDisplayWidget>();
	Widget->BindToFeatComponent(Feats);
	TSharedRef<SWidget> SlateWidget = Widget->TakeWidget();
	TestTrue(
		"Initial summary matches the component's unlocked count",
		HasText(
			Widget,
			TEXT("FeatSummaryText"),
			TEXT("Feats: 0 unlocked \u2014 0 Common, 0 Rare, 0 Legendary")));

	UMordecaiFeatDataAsset* IronWill = MakeFeat(Feats, TEXT("IronWill"));
	Feats->RegisterFeatDataAsset(IronWill);
	Feats->IncrementFeatStat(TEXT("RecoveryFeatEvents"), 1);
	TestEqual("Component unlocked count is one", Feats->GetUnlockedFeats().Num(), 1);
	TestTrue(
		"Delegate refreshes summary to the component's unlocked count",
		HasText(
			Widget,
			TEXT("FeatSummaryText"),
			TEXT("Feats: 1 unlocked \u2014 1 Common, 0 Rare, 0 Legendary")));
	TestTrue(
		"Unlocked feat row is rendered",
		HasText(Widget, TEXT("FeatName_IronWill"), TEXT("IronWill")));

	UMordecaiUIThemeSubsystem* Themes =
		Env.LocalPlayer->GetSubsystem<UMordecaiUIThemeSubsystem>();
	Themes->SetActiveTheme(EMordecaiUITheme::HighContrast);
	TestTrue(
		"Feats title follows the live High Contrast text token",
		HasThemeTextColor(
			Widget,
			TEXT("FeatsTitle"),
			Themes->GetActiveColors().Text));
	TestTrue(
		"Feats content plate follows the live High Contrast plate token",
		HasThemeBrushColor(
			Widget,
			TEXT("FeatContentPlate"),
			Themes->GetActiveColors().Plate));
	TestTrue(
		"Feat row follows the live High Contrast raised-plate token",
		HasThemeBrushColor(
			Widget,
			TEXT("FeatRow_IronWill"),
			Themes->GetActiveColors().PlateRaised));
	TestTrue(
		"Feat description follows the live High Contrast muted-text token",
		HasThemeTextColor(
			Widget,
			TEXT("FeatDescription_IronWill"),
			Themes->GetActiveColors().TextMuted));
	TestTrue(
		"Feat tier follows the live High Contrast common-tier token",
		HasThemeTextColor(
			Widget,
			TEXT("FeatTier_IronWill"),
			Themes->GetActiveColors().FeatCommon));
	return true;
}

#endif // WITH_AUTOMATION_TESTS
