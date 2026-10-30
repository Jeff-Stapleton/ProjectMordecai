// Project Mordecai

#pragma once

#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "MordecaiPlaytestCaptureSubsystem.generated.h"

class ULoadingScreenManager;

/**
 * Capture settings from the command line:
 *   -PlaytestCapture [-PlaytestShotDir=<dir>] [-PlaytestShots=3] [-PlaytestShotDelay=6] [-PlaytestShotInterval=2]
 */
struct FMordecaiPlaytestCaptureConfig
{
	bool bEnabled = false;
	int32 NumShots = 3;
	float DelaySeconds = 6.f;
	float IntervalSeconds = 2.f;
	FString ShotDir;

	static FMordecaiPlaytestCaptureConfig FromCommandLine(const TCHAR* CommandLine);

	/** <ShotDir>/engine_01.png for Index 0, and so on. */
	FString GetShotPath(int32 Index) const;

	/** <ShotDir>/engine_loading.png: the frame taken when the loading screen first shows. */
	FString GetLoadingShotPath() const;

	/** Seconds after the experience finishes loading that shot Index is taken. */
	float GetShotTime(int32 Index) const { return DelaySeconds + Index * IntervalSeconds; }
};

/**
 * UMordecaiPlaytestCaptureSubsystem
 *
 *	Saves screenshots from the game's own viewport once the experience has loaded, for
 *	packaged-build playtests (Scripts/playtest_boot.py). Desktop capture comes back black when
 *	the machine is locked or the window is covered; this reads the rendered frame instead.
 *	Also saves engine_loading.png when the loading screen first shows: it is gone ~2 s after the
 *	experience loads, before any scheduled shot (issue #39).
 *	Only created in non-Shipping builds launched with -PlaytestCapture.
 */
UCLASS()
class UMordecaiPlaytestCaptureSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	bool Tick(float DeltaTime);
	void HandleLoadingScreenVisibilityChanged(bool bIsVisible);

	FMordecaiPlaytestCaptureConfig Config;
	FTSTicker::FDelegateHandle TickHandle;
	TWeakObjectPtr<ULoadingScreenManager> LoadingScreenManager;
	double ExperienceLoadedTime = -1.0;
	int32 NextShot = 0;
	bool bLoadingShotTaken = false;
};
