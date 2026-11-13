// Project Mordecai — Starting skill catalogue recovery (US-093 round 2).

#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "Internationalization/Text.h"
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
		{ TEXT("Magic Missile"), EMordecaiSkillCategory::Magic },
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

	TArray<FName> CollectRegisteredSkillsInSheetOrder(
		const UMordecaiSkillComponent* Component)
	{
		TArray<FName> Result;
		for (const EMordecaiSkillCategory Category : Categories)
		{
			for (const FMordecaiSkillRankInfo& Skill : Component->GetAllSkillsByCategory(Category))
			{
				Result.Add(Skill.SkillName);
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
	FRecoveryTestEnv Env;
	if (!Env.Setup(FVector::ZeroVector))
	{
		AddError(TEXT("Recovery test environment setup failed"));
		return false;
	}
	ON_SCOPE_EXIT { Env.Teardown(); };

	const UMordecaiSkillComponent* Component = Env.PlayerState->GetSkillComponent();
	if (!TestNotNull("PlayerState SkillComponent exists", Component))
	{
		return false;
	}
	const TMap<FName, EMordecaiSkillCategory> Actual = CollectRegisteredSkills(Component);
	const TArray<FName> Ordered = CollectRegisteredSkillsInSheetOrder(Component);
	TestEqual("registered set count matches all nonblank skill-sheet rows",
		Actual.Num(), UE_ARRAY_COUNT(ExpectedSkills));
	TestEqual("ordered catalogue count matches the sheet",
		Ordered.Num(), UE_ARRAY_COUNT(ExpectedSkills));

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(ExpectedSkills); ++Index)
	{
		const FExpectedSkill& Expected = ExpectedSkills[Index];
		const FName Name(Expected.Name);
		if (Ordered.IsValidIndex(Index))
		{
			TestTrue(
				*FString::Printf(TEXT("catalogue row %d matches sheet spelling and order"), Index),
				Ordered[Index].ToString().Equals(Expected.Name, ESearchCase::CaseSensitive));
		}
		const EMordecaiSkillCategory* Category = Actual.Find(Name);
		TestNotNull(*FString::Printf(TEXT("%s is registered"), Expected.Name), Category);
		if (Category)
		{
			TestTrue(*FString::Printf(TEXT("%s has its exact sheet category"), Expected.Name),
				*Category == Expected.Category);
		}

		const UMordecaiSkillDataAsset* Data = Component->GetRegisteredSkillDataAsset(Name);
		TestNotNull(*FString::Printf(TEXT("%s uses a skill data asset"), Expected.Name), Data);
		if (Data)
		{
			TestTrue(
				*FString::Printf(TEXT("%s runtime data is transient"), Expected.Name),
				Data->HasAnyFlags(RF_Transient));
			TestTrue(
				*FString::Printf(TEXT("%s localized display name matches exactly"), Expected.Name),
				Data->DisplayName.ToString().Equals(
					Expected.Name,
					ESearchCase::CaseSensitive));
			TestTrue(
				*FString::Printf(TEXT("%s display name carries a localization namespace"), Expected.Name),
				FTextInspector::GetNamespace(Data->DisplayName).IsSet());
			TestTrue(
				*FString::Printf(TEXT("%s display name carries a localization key"), Expected.Name),
				FTextInspector::GetKey(Data->DisplayName).IsSet());
		}
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

	TestEqual("the complete starting catalogue is registered",
		CollectRegisteredSkills(Component).Num(), UE_ARRAY_COUNT(ExpectedSkills));
	TestEqual("production starting registration grants no abilities",
		ASC->GetActivatableAbilities().Num(), 0);
	TestEqual("production starting registration grants no skill points",
		Component->GetAvailableSkillPoints(), 0);
	TestEqual("production starting registration creates no milestone names",
		Component->GetActiveMilestoneTagNames().Num(), 0);

	FGameplayTagContainer OwnedTags;
	ASC->GetOwnedGameplayTags(OwnedTags);
	for (const FGameplayTag& Tag : OwnedTags)
	{
		TestFalse(
			*FString::Printf(TEXT("production starting registration adds no %s gameplay tag"),
				*Tag.ToString()),
			Tag.ToString().StartsWith(TEXT("Mordecai.Skill."), ESearchCase::CaseSensitive));
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_Skills_ClientMutatorsAreRejected,
	"Mordecai.Recovery.Skills.ClientMutatorsAreRejected",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Skills_ClientMutatorsAreRejected::RunTest(
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
	if (!TestNotNull("PlayerState SkillComponent exists", Component))
	{
		return false;
	}

	Env.PlayerState->SetRole(ROLE_SimulatedProxy);
	const int32 RankBefore = Component->GetSkillRank(TEXT("Longswords"));
	const int32 PointsBefore = Component->GetAvailableSkillPoints();

	const bool bSetPoints = Component->SetAvailableSkillPoints(12);
	const bool bSetRank = Component->SetSkillRank(TEXT("Longswords"), 8);
	const bool bAllocated = Component->TryAllocateSkillPoint(TEXT("Longswords"));

	TestFalse("client SetAvailableSkillPoints returns false", bSetPoints);
	TestFalse("client SetSkillRank returns false", bSetRank);
	TestFalse("client TryAllocateSkillPoint returns false", bAllocated);
	TestEqual("client SetSkillRank leaves rank unchanged",
		Component->GetSkillRank(TEXT("Longswords")), RankBefore);
	TestEqual("client SetAvailableSkillPoints leaves points unchanged",
		Component->GetAvailableSkillPoints(), PointsBefore);
	return true;
}

#endif // WITH_AUTOMATION_TESTS
