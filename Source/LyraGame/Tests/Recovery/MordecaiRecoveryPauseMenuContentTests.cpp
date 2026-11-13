// Project Mordecai — Recovery: pause-menu Character, Skills, and Feats content (US-093).

#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "HAL/PlatformMisc.h"
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
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(AttributeCodes); ++Index)
	{
		const FString LabelName = FString::Printf(TEXT("PrimaryLabel_%s"), AttributeCodes[Index]);
		const FString ValueName = FString::Printf(TEXT("PrimaryValue_%s"), AttributeCodes[Index]);
		TestTrue(
			*FString::Printf(TEXT("%s label is rendered"), AttributeCodes[Index]),
			HasText(Widget, *LabelName, AttributeCodes[Index]));
		TestTrue(
			*FString::Printf(TEXT("%s live value is rendered"), AttributeCodes[Index]),
			HasText(
				Widget,
				*ValueName,
				FString::FromInt(FMath::RoundToInt(
					Widget->GetData().PrimaryAttributes[Index].BaseValue))));
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
	UAbilitySystemComponent* ASC = Env.PlayerState->GetAbilitySystemComponent();
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
		"Character title follows the live High Contrast ink token",
		HasThemeTextColor(
			Widget,
			TEXT("CharacterTitle"),
			Themes->GetActiveColors().Ink));
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
	UMordecaiSkillDataAsset* Longswords =
		MakeSkill(Skills, TEXT("Longswords"), EMordecaiSkillCategory::Weapon);
	Skills->RegisterSkillDataAsset(Longswords);
	Skills->SetSkillRank(TEXT("Longswords"), 7);

	UMordecaiSkillTreeWidget* Widget = Env.MakeWidget<UMordecaiSkillTreeWidget>();
	Widget->BindToSkillComponent(Skills);
	TSharedRef<SWidget> SlateWidget = Widget->TakeWidget();

	TestTrue(
		"Weapon category is rendered",
		HasText(Widget, TEXT("Category_Weapon"), TEXT("Weapon")));
	TestTrue(
		"Longswords row is built from the bound SkillComponent",
		HasText(Widget, TEXT("SkillName_Longswords"), TEXT("Longswords")));
	TestTrue(
		"Longswords row renders rank / 20",
		HasText(Widget, TEXT("SkillRank_Longswords"), TEXT("7 / 20")));

	UMordecaiUIThemeSubsystem* Themes =
		Env.LocalPlayer->GetSubsystem<UMordecaiUIThemeSubsystem>();
	Themes->SetActiveTheme(EMordecaiUITheme::HighContrast);
	TestTrue(
		"Skills title follows the live High Contrast ink token",
		HasThemeTextColor(
			Widget,
			TEXT("SkillsTitle"),
			Themes->GetActiveColors().Ink));
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
		HasText(Widget, TEXT("FeatSummaryText"), TEXT("Feats: 0 unlocked")));

	UMordecaiFeatDataAsset* IronWill = MakeFeat(Feats, TEXT("IronWill"));
	Feats->RegisterFeatDataAsset(IronWill);
	Feats->IncrementFeatStat(TEXT("RecoveryFeatEvents"), 1);
	TestEqual("Component unlocked count is one", Feats->GetUnlockedFeats().Num(), 1);
	TestTrue(
		"Delegate refreshes summary to the component's unlocked count",
		HasText(Widget, TEXT("FeatSummaryText"), TEXT("Feats: 1 unlocked")));
	TestTrue(
		"Unlocked feat row is rendered",
		HasText(Widget, TEXT("FeatName_IronWill"), TEXT("IronWill")));

	UMordecaiUIThemeSubsystem* Themes =
		Env.LocalPlayer->GetSubsystem<UMordecaiUIThemeSubsystem>();
	Themes->SetActiveTheme(EMordecaiUITheme::HighContrast);
	TestTrue(
		"Feats title follows the live High Contrast ink token",
		HasThemeTextColor(
			Widget,
			TEXT("FeatsTitle"),
			Themes->GetActiveColors().Ink));
	return true;
}

#endif // WITH_AUTOMATION_TESTS
