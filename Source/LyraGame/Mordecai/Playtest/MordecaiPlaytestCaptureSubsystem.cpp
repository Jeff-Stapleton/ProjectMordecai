// Project Mordecai

#include "Mordecai/Playtest/MordecaiPlaytestCaptureSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameModes/LyraExperienceManagerComponent.h"
#include "HAL/FileManager.h"
#include "LoadingScreenManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiPlaytestCaptureSubsystem)

DEFINE_LOG_CATEGORY_STATIC(LogMordecaiPlaytest, Log, All);

FMordecaiPlaytestCaptureConfig FMordecaiPlaytestCaptureConfig::FromCommandLine(const TCHAR* CommandLine)
{
	FMordecaiPlaytestCaptureConfig Config;
	if (!CommandLine)
	{
		return Config;
	}

	Config.bPlaytestCapture = FParse::Param(CommandLine, TEXT("PlaytestCapture"));
	Config.bMarketingFrameCapture = FParse::Param(CommandLine, TEXT("MarketingFrameCapture"));
	Config.bConflictingCaptureModes =
		Config.bPlaytestCapture && Config.bMarketingFrameCapture;
	Config.bEnabled =
		(Config.bPlaytestCapture || Config.bMarketingFrameCapture)
		&& !Config.bConflictingCaptureModes;

	FParse::Value(CommandLine, TEXT("PlaytestShots="), Config.NumShots);
	FParse::Value(CommandLine, TEXT("MarketingFrameLimit="), Config.MarketingFrameLimit);
	FParse::Value(CommandLine, TEXT("PlaytestShotDelay="), Config.DelaySeconds);
	FParse::Value(CommandLine, TEXT("PlaytestShotInterval="), Config.IntervalSeconds);
	Config.NumShots = FMath::Clamp(Config.NumShots, 0, 20);
	Config.MarketingFrameLimit = FMath::Clamp(Config.MarketingFrameLimit, 1, 3600);
	Config.DelaySeconds = FMath::Max(Config.DelaySeconds, 0.f);
	Config.IntervalSeconds = FMath::Max(Config.IntervalSeconds, 0.1f);

	if (!FParse::Value(CommandLine, TEXT("PlaytestShotDir="), Config.ShotDir) || Config.ShotDir.IsEmpty())
	{
		Config.ShotDir = FPaths::ProjectSavedDir() / TEXT("Playtests/Capture");
	}
	if (!FParse::Value(CommandLine, TEXT("MarketingFrameDir="), Config.MarketingFrameDir)
		|| Config.MarketingFrameDir.IsEmpty())
	{
		Config.MarketingFrameDir = FPaths::ProjectSavedDir() / TEXT("Playtests/MarketingFrames");
	}
	return Config;
}

FString FMordecaiPlaytestCaptureConfig::GetShotPath(int32 Index) const
{
	return ShotDir / FString::Printf(TEXT("engine_%02d.png"), Index + 1);
}

FString FMordecaiPlaytestCaptureConfig::GetLoadingShotPath() const
{
	return ShotDir / TEXT("engine_loading.png");
}

FString FMordecaiPlaytestCaptureConfig::GetMarketingFramePath(int32 Index) const
{
	return MarketingFrameDir / FString::Printf(TEXT("frame_%06d.png"), Index + 1);
}

bool UMordecaiPlaytestCaptureSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	const FMordecaiPlaytestCaptureConfig CaptureConfig =
		FMordecaiPlaytestCaptureConfig::FromCommandLine(FCommandLine::Get());
	if (CaptureConfig.bConflictingCaptureModes)
	{
		UE_LOG(LogMordecaiPlaytest, Error,
			TEXT("-PlaytestCapture and -MarketingFrameCapture cannot be combined; refusing capture"));
		return false;
	}
	return Super::ShouldCreateSubsystem(Outer) && CaptureConfig.bEnabled;
#endif
}

void UMordecaiPlaytestCaptureSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Config = FMordecaiPlaytestCaptureConfig::FromCommandLine(FCommandLine::Get());
	IFileManager::Get().MakeDirectory(
		*(Config.bMarketingFrameCapture ? Config.MarketingFrameDir : Config.ShotDir),
		/*Tree*/ true);
	TickHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &ThisClass::Tick),
		Config.bMarketingFrameCapture ? 0.0f : 0.1f);

	if (ULoadingScreenManager* Manager =
		Collection.InitializeDependency<ULoadingScreenManager>())
	{
		LoadingScreenManager = Manager;
		if (Config.bPlaytestCapture)
		{
			Manager->OnLoadingScreenVisibilityChangedDelegate().AddUObject(
				this, &ThisClass::HandleLoadingScreenVisibilityChanged);
		}
	}

	if (Config.bMarketingFrameCapture)
	{
		UE_LOG(LogMordecaiPlaytest, Display,
			TEXT("MarketingFrameCapture: up to %d fixed-timestep UI frame(s) to %s"),
			Config.MarketingFrameLimit, *Config.MarketingFrameDir);
	}
	else
	{
		UE_LOG(LogMordecaiPlaytest, Display,
			TEXT("PlaytestCapture: %d screenshot(s) to %s, %.1fs after the experience loads, every %.1fs"),
			Config.NumShots, *Config.ShotDir, Config.DelaySeconds, Config.IntervalSeconds);
	}
}

void UMordecaiPlaytestCaptureSubsystem::Deinitialize()
{
	FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
	if (ULoadingScreenManager* Manager = LoadingScreenManager.Get())
	{
		Manager->OnLoadingScreenVisibilityChangedDelegate().RemoveAll(this);
	}
	Super::Deinitialize();
}

void UMordecaiPlaytestCaptureSubsystem::HandleLoadingScreenVisibilityChanged(bool bIsVisible)
{
	// The first show only: the boot's loading screen, which the scheduled shots never see
	if (!bIsVisible || bLoadingShotTaken)
	{
		return;
	}
	bLoadingShotTaken = true;

	if (FScreenshotRequest::IsScreenshotRequested())
	{
		UE_LOG(LogMordecaiPlaytest, Warning, TEXT("PlaytestCapture: no loading screen shot: another screenshot is pending"));
		return;
	}
	// Broadcast just before the manager adds its widget to the viewport. The request is served at the next viewport
	// draw, by a Slate capture of the whole window (bShowUI), with the loading screen on top.
	const FString Path = Config.GetLoadingShotPath();
	FScreenshotRequest::RequestScreenshot(Path, /*bInShowUI*/ true, /*bAddFilenameSuffix*/ false);
	UE_LOG(LogMordecaiPlaytest, Display, TEXT("PlaytestCapture: requested %s (loading screen)"), *Path);
}

bool UMordecaiPlaytestCaptureSubsystem::Tick(float DeltaTime)
{
	if ((!Config.bMarketingFrameCapture && NextShot >= Config.NumShots)
		|| (Config.bMarketingFrameCapture && MarketingTickCount >= Config.MarketingFrameLimit))
	{
		return false;
	}

	const double Now = FPlatformTime::Seconds();
	if (ExperienceLoadedTime < 0.0)
	{
		const UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
		const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
		const ULyraExperienceManagerComponent* ExperienceComponent = GameState ? GameState->FindComponentByClass<ULyraExperienceManagerComponent>() : nullptr;
		if (!ExperienceComponent || !ExperienceComponent->IsExperienceLoaded())
		{
			return true;
		}
		ExperienceLoadedTime = Now;
		UE_LOG(LogMordecaiPlaytest, Display, TEXT("PlaytestCapture: experience loaded in %s"), *World->GetMapName());
	}

	if (Config.bMarketingFrameCapture)
	{
		// The loading screen remains after the experience loads. Numbered marketing frames must
		// begin in gameplay so the encoded clip never needs a lossy post-capture trim.
		if (const ULoadingScreenManager* Manager = LoadingScreenManager.Get();
			Manager && Manager->GetLoadingScreenDisplayStatus())
		{
			return true;
		}

		// Let the startup r.SetRes command and viewport settle before frame 1. Requests made
		// during that resize can be accepted by FScreenshotRequest but never reach disk.
		// Warm-up ticks are intentionally unnumbered; every numbered tick must produce a file.
		if (MarketingWarmupTickCount++ < 30)
		{
			return true;
		}

		const int32 MarketingFrameIndex = MarketingTickCount++;
		if (!FScreenshotRequest::IsScreenshotRequested())
		{
			// The filename follows the fixed-timestep tick, not the number of successful requests.
			// A pending screenshot therefore leaves a detectable gap instead of silently compacting time.
			const FString Path = Config.GetMarketingFramePath(MarketingFrameIndex);
			FScreenshotRequest::RequestScreenshot(
				Path, /*bInShowUI*/ true, /*bAddFilenameSuffix*/ false);
		}
		else
		{
			UE_LOG(LogMordecaiPlaytest, Error,
				TEXT("PlaytestCapture: missed marketing frame %d because another screenshot was pending"),
				MarketingFrameIndex + 1);
		}
		return MarketingTickCount < Config.MarketingFrameLimit;
	}

	// One request at a time: the viewport handles a single pending screenshot per frame
	if (!FScreenshotRequest::IsScreenshotRequested() && Now - ExperienceLoadedTime >= Config.GetShotTime(NextShot))
	{
		const FString Path = Config.GetShotPath(NextShot);
		FScreenshotRequest::RequestScreenshot(Path, /*bInShowUI*/ true, /*bAddFilenameSuffix*/ false);
		UE_LOG(LogMordecaiPlaytest, Display, TEXT("PlaytestCapture: requested %s"), *Path);
		++NextShot;
	}
	return NextShot < Config.NumShots;
}
