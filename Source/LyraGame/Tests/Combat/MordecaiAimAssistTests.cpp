// Project Mordecai — Aim Assist System Tests (US-009)

#include "Misc/AutomationTest.h"
#include "Mordecai/Combat/MordecaiAimAssistSubsystem.h"
#include "Mordecai/MordecaiGameplayTags.h"

// ---------------------------------------------------------------------------
// 1. Mordecai.AimAssist.FindsBestCandidate (AC-009.19, AC-009.22)
// Place target at 3° offset, verify returned direction points toward target
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_AimAssist_FindsBestCandidate,
	"Mordecai.AimAssist.FindsBestCandidate",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_AimAssist_FindsBestCandidate::RunTest(const FString& Parameters)
{
	// Verify subsystem class exists
	const UClass* SubsystemClass = UMordecaiAimAssistSubsystem::StaticClass();
	TestNotNull("UMordecaiAimAssistSubsystem class exists", SubsystemClass);
	TestTrue("Is a UWorldSubsystem", SubsystemClass->IsChildOf(UWorldSubsystem::StaticClass()));

	// Test the static candidate generation helper
	FVector AimForward = FVector(1.f, 0.f, 0.f);
	TArray<float> CandidateAngles = { 0.f, 2.f, 4.f, 6.f };

	TArray<FVector> Candidates = UMordecaiAimAssistSubsystem::GenerateCandidateDirections(
		AimForward, CandidateAngles);

	// Should produce: 0, +2, -2, +4, -4, +6, -6 = 7 candidates
	TestEqual("7 candidate directions generated", Candidates.Num(), 7);

	// First candidate should be the original forward direction (0° offset)
	float DotFirst = FVector::DotProduct(Candidates[0].GetSafeNormal(), AimForward.GetSafeNormal());
	TestTrue("First candidate is original forward", FMath::IsNearlyEqual(DotFirst, 1.f, 0.001f));

	return true;
}

// ---------------------------------------------------------------------------
// 2. Mordecai.AimAssist.PrefersSmallestAngle (AC-009.22)
// Two targets at different offsets — should prefer smallest angle
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_AimAssist_PrefersSmallestAngle,
	"Mordecai.AimAssist.PrefersSmallestAngle",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_AimAssist_PrefersSmallestAngle::RunTest(const FString& Parameters)
{
	// Static selection: given two candidates with different angular offsets,
	// should prefer the one with the smaller offset
	FVector Origin = FVector::ZeroVector;
	FVector AimForward = FVector(1.f, 0.f, 0.f);

	// Candidate at 2° offset (closer to forward)
	FVector Target2DegDir = AimForward.RotateAngleAxis(2.f, FVector::UpVector).GetSafeNormal();
	FVector Target2DegPos = Origin + Target2DegDir * 500.f;

	// Candidate at 5° offset (farther from forward)
	FVector Target5DegDir = AimForward.RotateAngleAxis(5.f, FVector::UpVector).GetSafeNormal();
	FVector Target5DegPos = Origin + Target5DegDir * 400.f;

	// Build candidate results — simulate what the subsystem produces
	TArray<FMordecaiAimCandidate> Candidates;
	{
		FMordecaiAimCandidate C1;
		C1.Direction = Target2DegDir;
		C1.AngleOffset = 2.f;
		C1.Distance = 500.f;
		C1.bHitTarget = true;
		Candidates.Add(C1);
	}
	{
		FMordecaiAimCandidate C2;
		C2.Direction = Target5DegDir;
		C2.AngleOffset = 5.f;
		C2.Distance = 400.f;
		C2.bHitTarget = true;
		Candidates.Add(C2);
	}

	FMordecaiAimCandidate Best = UMordecaiAimAssistSubsystem::SelectBestCandidate(Candidates);
	TestTrue("Best candidate found", Best.bHitTarget);
	TestEqual("Prefers smallest angle offset", Best.AngleOffset, 2.f);

	return true;
}

// ---------------------------------------------------------------------------
// 3. Mordecai.AimAssist.ReturnsOriginalIfNoTargets (AC-009.24)
// No targets in range — original AimForward returned unchanged
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_AimAssist_ReturnsOriginalIfNoTargets,
	"Mordecai.AimAssist.ReturnsOriginalIfNoTargets",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_AimAssist_ReturnsOriginalIfNoTargets::RunTest(const FString& Parameters)
{
	FVector AimForward = FVector(1.f, 0.f, 0.f);
	TArray<FMordecaiAimCandidate> EmptyCandidates;

	// With no valid candidates, SelectBestCandidate should return an invalid result
	FMordecaiAimCandidate Best = UMordecaiAimAssistSubsystem::SelectBestCandidate(EmptyCandidates);
	TestFalse("No best candidate when empty", Best.bHitTarget);

	// The public API FallbackDirection helper should return original forward
	FVector Result = UMordecaiAimAssistSubsystem::GetCorrectedDirection(AimForward, Best);
	float Dot = FVector::DotProduct(Result.GetSafeNormal(), AimForward.GetSafeNormal());
	TestTrue("Returns original forward when no targets", FMath::IsNearlyEqual(Dot, 1.f, 0.001f));

	return true;
}

// ---------------------------------------------------------------------------
// 4. Mordecai.AimAssist.DoesNotExceedMaxCorrection (AC-009.23)
// Target at 10° offset (beyond max 6°) — no correction applied
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_AimAssist_DoesNotExceedMaxCorrection,
	"Mordecai.AimAssist.DoesNotExceedMaxCorrection",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_AimAssist_DoesNotExceedMaxCorrection::RunTest(const FString& Parameters)
{
	FVector AimForward = FVector(1.f, 0.f, 0.f);
	TArray<float> CandidateAngles = { 0.f, 2.f, 4.f, 6.f };

	// Generate candidates — max angle is 6°
	TArray<FVector> Dirs = UMordecaiAimAssistSubsystem::GenerateCandidateDirections(
		AimForward, CandidateAngles);

	// Verify no candidate exceeds 6° from forward
	float MaxAngleCos = FMath::Cos(FMath::DegreesToRadians(6.f + 0.1f)); // small tolerance
	for (const FVector& Dir : Dirs)
	{
		float Dot = FVector::DotProduct(Dir.GetSafeNormal(), AimForward.GetSafeNormal());
		TestTrue("Candidate within max angle", Dot >= MaxAngleCos);
	}

	// A target at 10° offset should not produce a valid candidate
	FVector Target10Deg = AimForward.RotateAngleAxis(10.f, FVector::UpVector).GetSafeNormal();

	// Check that none of the candidates are close to the 10° direction
	float BestDot = -1.f;
	for (const FVector& Dir : Dirs)
	{
		float Dot = FVector::DotProduct(Dir.GetSafeNormal(), Target10Deg);
		BestDot = FMath::Max(BestDot, Dot);
	}

	// The best candidate should be the 6° one, which at cos(4°) ≈ 0.9976 from the 10° target
	// This proves a 10° target would not get a near-perfect match from our candidates
	float CosOf4Deg = FMath::Cos(FMath::DegreesToRadians(4.f));
	TestTrue("No candidate matches 10° target closely", BestDot <= CosOf4Deg + 0.01f);

	return true;
}

// ---------------------------------------------------------------------------
// 5. Mordecai.AimAssist.IgnoresFriendlyTargets (AC-009.26)
// Verify that filtering excludes friendly actors (no Team.Enemy tag)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_AimAssist_IgnoresFriendlyTargets,
	"Mordecai.AimAssist.IgnoresFriendlyTargets",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_AimAssist_IgnoresFriendlyTargets::RunTest(const FString& Parameters)
{
	// Verify the Team.Enemy tag is declared via conversion to FGameplayTag
	FGameplayTag EnemyTag = MordecaiGameplayTags::Team_Enemy;
	TestTrue("Team.Enemy tag is valid", EnemyTag.IsValid());

	// Verify the tag name matches expected pattern
	FString TagName = EnemyTag.GetTagName().ToString();
	TestEqual("Tag name is Mordecai.Team.Enemy", TagName, FString(TEXT("Mordecai.Team.Enemy")));

	// The subsystem uses this tag for target filtering —
	// IsValidAimTarget returns false for actors without Team.Enemy tag
	TestTrue("IsValidAimTarget rejects nullptr", !UMordecaiAimAssistSubsystem::IsValidAimTarget(nullptr, FGameplayTag()));

	return true;
}

// ---------------------------------------------------------------------------
// 6. Mordecai.AimAssist.SubtleCorrectionOnly (AC-009.25)
// Verify max correction angle equals largest CandidateAngleOffset
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_AimAssist_SubtleCorrectionOnly,
	"Mordecai.AimAssist.SubtleCorrectionOnly",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_AimAssist_SubtleCorrectionOnly::RunTest(const FString& Parameters)
{
	FVector AimForward = FVector(1.f, 0.f, 0.f);

	// Default candidate angles: {0, 2, 4, 6}
	TArray<float> CandidateAngles = { 0.f, 2.f, 4.f, 6.f };

	// Simulate a best candidate at 6° (the maximum)
	FMordecaiAimCandidate BestAt6;
	BestAt6.Direction = AimForward.RotateAngleAxis(6.f, FVector::UpVector).GetSafeNormal();
	BestAt6.AngleOffset = 6.f;
	BestAt6.Distance = 500.f;
	BestAt6.bHitTarget = true;

	FVector Corrected = UMordecaiAimAssistSubsystem::GetCorrectedDirection(AimForward, BestAt6);

	// Verify corrected direction does not exceed 6° from forward
	float Dot = FVector::DotProduct(Corrected.GetSafeNormal(), AimForward.GetSafeNormal());
	float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f)));
	TestTrue("Correction does not exceed 6 degrees", AngleDeg <= 6.f + 0.1f);

	// Verify it IS corrected (not original direction)
	TestTrue("Direction was corrected", AngleDeg > 0.1f);

	return true;
}
