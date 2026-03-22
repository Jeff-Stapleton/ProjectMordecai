// Project Mordecai — Posture & Stagger System (US-007)

#include "Mordecai/Combat/MordecaiPostureSystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiPostureSystem)

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMordecaiPostureSystem::UMordecaiPostureSystem()
{
}

// ---------------------------------------------------------------------------
// Computation Methods
// ---------------------------------------------------------------------------

float UMordecaiPostureSystem::ComputePostureDamage(float BasePower, float PostureDamageScalar, bool bIsBlocking) const
{
	// AC-007.4: PostureDamage = BasePower x PostureDamageScalar
	// AC-007.5: x BlockPostureDamageMultiplier if blocking
	// AC-007.15: x WeakenedPostureMultiplier (attacker debuff, default 1.0)
	// AC-007.16: x BrittlePostureMultiplier (target debuff, default 1.0)
	float BlockMult = bIsBlocking ? BlockPostureDamageMultiplier : 1.0f;
	return BasePower * PostureDamageScalar * BlockMult * WeakenedPostureMultiplier * BrittlePostureMultiplier;
}

float UMordecaiPostureSystem::ComputeStaggeredDamage(float IncomingDamage) const
{
	// AC-007.10: Damage x StaggeredDamageMultiplier during PostureBroken
	return IncomingDamage * StaggeredDamageMultiplier;
}

float UMordecaiPostureSystem::ComputeKnockDownDamage(float IncomingDamage) const
{
	// AC-007.10a: Damage x KnockDownDamageMultiplier during KnockedDown
	return IncomingDamage * KnockDownDamageMultiplier;
}

float UMordecaiPostureSystem::GetVulnerabilityDamageMultiplier() const
{
	if (bIsKnockedDown)
	{
		return KnockDownDamageMultiplier;
	}
	if (bIsPostureBroken)
	{
		return StaggeredDamageMultiplier;
	}
	return 1.0f;
}

// ---------------------------------------------------------------------------
// State Queries
// ---------------------------------------------------------------------------

bool UMordecaiPostureSystem::ShouldBreakPosture(float PostureAfterDamage) const
{
	// AC-007.7: Break when posture reaches 0
	return PostureAfterDamage <= 0.f;
}

bool UMordecaiPostureSystem::ShouldKnockDown() const
{
	// AC-007.10a: Knock down when cumulative damage during stagger exceeds threshold
	return CumulativeDamageDuringStagger >= KnockDownDamageThreshold;
}

bool UMordecaiPostureSystem::IsRegenAllowed(double CurrentTime) const
{
	// AC-007.14: No regen during PostureBroken or KnockedDown
	if (bIsPostureBroken || bIsKnockedDown)
	{
		return false;
	}

	// AC-007.13: Regen paused for PostureRegenDelayMs after taking posture damage
	double DelaySeconds = PostureRegenDelayMs / 1000.0;
	double TimeSinceLastDamage = CurrentTime - LastPostureDamageTime;
	return TimeSinceLastDamage > DelaySeconds;
}

float UMordecaiPostureSystem::ComputeRegenAmount(float DeltaTime) const
{
	// AC-007.12: Regen = PostureRegenRate x DeltaTime
	return PostureRegenRate * DeltaTime;
}

// ---------------------------------------------------------------------------
// State Management
// ---------------------------------------------------------------------------

void UMordecaiPostureSystem::NotifyPostureDamageApplied(double CurrentTime)
{
	// AC-007.13: Track when posture damage was last taken for regen delay
	LastPostureDamageTime = CurrentTime;
}

void UMordecaiPostureSystem::EnterPostureBroken()
{
	// AC-007.7: Enter posture broken (stagger) state
	bIsPostureBroken = true;
	bIsKnockedDown = false;
	CumulativeDamageDuringStagger = 0.f;
}

float UMordecaiPostureSystem::ExitPostureBroken(float MaxPosture)
{
	// AC-007.11: After PostureBroken expires, Posture resets to MaxPosture
	bIsPostureBroken = false;
	CumulativeDamageDuringStagger = 0.f;
	return MaxPosture;
}

void UMordecaiPostureSystem::AccumulateStaggerDamage(float DamageAmount)
{
	// AC-007.10a: Track cumulative damage during stagger for knock down check
	CumulativeDamageDuringStagger += DamageAmount;
}

void UMordecaiPostureSystem::EnterKnockDown()
{
	// AC-007.10a: Knock down replaces stagger
	bIsPostureBroken = false;
	bIsKnockedDown = true;
}

float UMordecaiPostureSystem::ExitKnockDown(float MaxPosture)
{
	// AC-007.10a/AC-007.11: After knock down expires, Posture resets to MaxPosture
	bIsKnockedDown = false;
	CumulativeDamageDuringStagger = 0.f;
	return MaxPosture;
}

void UMordecaiPostureSystem::Reset()
{
	bIsPostureBroken = false;
	bIsKnockedDown = false;
	LastPostureDamageTime = -999.0;
	CumulativeDamageDuringStagger = 0.f;
}
