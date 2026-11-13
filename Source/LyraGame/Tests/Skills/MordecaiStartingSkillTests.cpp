// Project Mordecai — Starting skill catalogue recovery (US-093 round 2).

#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/Skills/MordecaiSkillComponent.h"
#include "Mordecai/Skills/MordecaiSkillDataAsset.h"
#include "Tests/Recovery/MordecaiRecoveryTestEnv.h"
#include "UObject/UnrealType.h"

namespace MordecaiStartingSkillTest
{
	struct FExpectedSkill
	{
		const TCHAR* Name;
		EMordecaiSkillCategory Category;
	};

	const FExpectedSkill ExpectedSkills[] = {
		{ TEXT("Longswords"), EMordecaiSkillCategory::Weapon },
		{ TEXT("Greatswords"), EMordecaiSkillCategory::Weapon },
		{ TEXT("Shortswords"), EMordecaiSkillCategory::Weapon },
		{ TEXT("Daggers"), EMordecaiSkillCategory::Weapon },
		{ TEXT("Axes"), EMordecaiSkillCategory::Weapon },
		{ TEXT("Maces"), EMordecaiSkillCategory::Weapon },
		{ TEXT("Spears"), EMordecaiSkillCategory::Weapon },
		{ TEXT("Quarterstaff"), EMordecaiSkillCategory::Weapon },
		{ TEXT("Unarmed"), EMordecaiSkillCategory::Weapon },
		{ TEXT("Longbow"), EMordecaiSkillCategory::Weapon },
		{ TEXT("Shortbow"), EMordecaiSkillCategory::Weapon },
		{ TEXT("Crossbows"), EMordecaiSkillCategory::Weapon },
		{ TEXT("Two-Weapon Fighting"), EMordecaiSkillCategory::Weapon },

		{ TEXT("Dodging"), EMordecaiSkillCategory::DefenseArmor },
		{ TEXT("Parrying"), EMordecaiSkillCategory::DefenseArmor },
		{ TEXT("Buckler Shield"), EMordecaiSkillCategory::DefenseArmor },
		{ TEXT("Scutum Shield"), EMordecaiSkillCategory::DefenseArmor },
		{ TEXT("Heater Shield"), EMordecaiSkillCategory::DefenseArmor },
		{ TEXT("Leather Armor"), EMordecaiSkillCategory::DefenseArmor },
		{ TEXT("Chainmail Armor"), EMordecaiSkillCategory::DefenseArmor },
		{ TEXT("Plate Armor"), EMordecaiSkillCategory::DefenseArmor },

		{ TEXT("Sneaking"), EMordecaiSkillCategory::MovementExploration },
		{ TEXT("Climbing"), EMordecaiSkillCategory::MovementExploration },
		{ TEXT("Swimming"), EMordecaiSkillCategory::MovementExploration },
		{ TEXT("Perception"), EMordecaiSkillCategory::MovementExploration },
		{ TEXT("Tracking"), EMordecaiSkillCategory::MovementExploration },
		{ TEXT("Cartography"), EMordecaiSkillCategory::MovementExploration },

		{ TEXT("Traps"), EMordecaiSkillCategory::UtilityInteraction },
		{ TEXT("Lockpicking"), EMordecaiSkillCategory::UtilityInteraction },
		{ TEXT("Pickpocket"), EMordecaiSkillCategory::UtilityInteraction },
		{ TEXT("First Aid"), EMordecaiSkillCategory::UtilityInteraction },

		{ TEXT("Fireball"), EMordecaiSkillCategory::Magic },
		{ TEXT("Cone of Cold"), EMordecaiSkillCategory::Magic },
		{ TEXT("Magic Missle"), EMordecaiSkillCategory::Magic },
		{ TEXT("Stone Skin"), EMordecaiSkillCategory::Magic },
		{ TEXT("Sleep"), EMordecaiSkillCategory::Magic },
		{ TEXT("Blink"), EMordecaiSkillCategory::Magic },
		{ TEXT("Illusion"), EMordecaiSkillCategory::Magic },
		{ TEXT("Fire Ward"), EMordecaiSkillCategory::Magic },
		{ TEXT("Frost Ward"), EMordecaiSkillCategory::Magic },
		{ TEXT("Float"), EMordecaiSkillCategory::Magic },
		{ TEXT("Counter Spell"), EMordecaiSkillCategory::Magic },
	};

	const EMordecaiSkillCategory Categories[] = {
		EMordecaiSkillCategory::Weapon,
		EMordecaiSkillCategory::DefenseArmor,
		EMordecaiSkillCategory::MovementExploration,
		EMordecaiSkillCategory::UtilityInteraction,
		EMordecaiSkillCategory::Magic,
	};

	TMap<FName, EMordecaiSkillCategory> CollectRegisteredSkills(
		const UMordecaiSkillComponent* Component)
	{
		TMap<FName, EMordecaiSkillCategory> Result;
		for (const EMordecaiSkillCategory Category : Categories)
		{
			for (const FMordecaiSkillRankInfo& Skill : Component->GetAllSkillsByCategory(Category))
			{
				Result.Add(Skill.SkillName, Category);
			}
		}
		return Result;
	}

	TMap<FName, float> SnapshotAttributes(const UMordecaiAttributeSet* Attributes)
	{
		TMap<FName, float> Result;
		for (TFieldIterator<FStructProperty> It(UMordecaiAttributeSet::StaticClass()); It; ++It)
		{
			if (It->Struct == FGameplayAttributeData::StaticStruct())
			{
				const FGameplayAttributeData* Value =
					It->ContainerPtrToValuePtr<FGameplayAttributeData>(Attributes);
				Result.Add(It->GetFName(), Value->GetCurrentValue());
			}
		}
		return Result;
	}
}

using namespace MordecaiStartingSkillTest;
using namespace MordecaiRecoveryTest;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_Skills_StartingSetMatchesSheet,
	"Mordecai.Recovery.Skills.StartingSetMatchesSheet",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Skills_StartingSetMatchesSheet::RunTest(const FString& Parameters)
{
	UMordecaiSkillComponent* Component = NewObject<UMordecaiSkillComponent>();
	Component->RegisterStartingSkillDataAssets();

	const TMap<FName, EMordecaiSkillCategory> Actual = CollectRegisteredSkills(Component);
	TestEqual("registered set count matches all nonblank skill-sheet rows",
		Actual.Num(), UE_ARRAY_COUNT(ExpectedSkills));

	for (const FExpectedSkill& Expected : ExpectedSkills)
	{
		const FName Name(Expected.Name);
		const EMordecaiSkillCategory* Category = Actual.Find(Name);
		TestNotNull(*FString::Printf(TEXT("%s is registered"), Expected.Name), Category);
		if (Category)
		{
			TestTrue(*FString::Printf(TEXT("%s has its exact sheet category"), Expected.Name),
				*Category == Expected.Category);
		}

		const UMordecaiSkillDataAsset* Data = Component->GetRegisteredSkillDataAsset(Name);
		TestNotNull(*FString::Printf(TEXT("%s uses a skill data asset"), Expected.Name), Data);
		TestEqual(*FString::Printf(TEXT("%s starts at rank 0"), Expected.Name),
			Component->GetSkillRank(Name), 0);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_Skills_StartingRegistrationHasNoGameplayEffects,
	"Mordecai.Recovery.Skills.StartingRegistrationHasNoGameplayEffects",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Skills_StartingRegistrationHasNoGameplayEffects::RunTest(
	const FString& Parameters)
{
	FRecoveryTestEnv Env;
	if (!Env.Setup(FVector::ZeroVector))
	{
		AddError(TEXT("Recovery test environment setup failed"));
		return false;
	}
	ON_SCOPE_EXIT { Env.Teardown(); };

	UMordecaiSkillComponent* Component = Env.PlayerState->GetSkillComponent();
	UAbilitySystemComponent* ASC = Env.PlayerState->GetAbilitySystemComponent();
	const UMordecaiAttributeSet* Attributes = Env.PlayerState->GetMordecaiAttributeSet();
	if (!TestNotNull("SkillComponent exists", Component)
		|| !TestNotNull("ASC exists", ASC)
		|| !TestNotNull("Mordecai attributes exist", Attributes))
	{
		return false;
	}

	const int32 AbilitiesBefore = ASC->GetActivatableAbilities().Num();
	const int32 SkillPointsBefore = Component->GetAvailableSkillPoints();
	const TMap<FName, float> AttributesBefore = SnapshotAttributes(Attributes);

	Component->RegisterStartingSkillDataAssets();

	TestEqual("the complete starting catalogue is registered",
		CollectRegisteredSkills(Component).Num(), UE_ARRAY_COUNT(ExpectedSkills));
	TestEqual("registration grants no abilities",
		ASC->GetActivatableAbilities().Num(), AbilitiesBefore);
	TestEqual("registration changes no skill points",
		Component->GetAvailableSkillPoints(), SkillPointsBefore);

	const TMap<FName, float> AttributesAfter = SnapshotAttributes(Attributes);
	TestEqual("the same attributes exist after registration",
		AttributesAfter.Num(), AttributesBefore.Num());
	for (const TPair<FName, float>& Before : AttributesBefore)
	{
		const float* After = AttributesAfter.Find(Before.Key);
		TestNotNull(*FString::Printf(TEXT("%s still exists"), *Before.Key.ToString()), After);
		if (After)
		{
			TestEqual(*FString::Printf(TEXT("%s is unchanged"), *Before.Key.ToString()),
				*After, Before.Value);
		}
	}

	for (const FExpectedSkill& Expected : ExpectedSkills)
	{
		const FName Name(Expected.Name);
		TestEqual(*FString::Printf(TEXT("%s remains rank 0"), Expected.Name),
			Component->GetSkillRank(Name), 0);
		TestFalse(*FString::Printf(TEXT("%s grants no rank-1 ability"), Expected.Name),
			Component->HasGrantedMilestoneAbility(Name, 1));
	}

	return true;
}

#endif // WITH_AUTOMATION_TESTS
