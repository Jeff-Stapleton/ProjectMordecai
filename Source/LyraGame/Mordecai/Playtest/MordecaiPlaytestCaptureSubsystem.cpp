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

	Config.bEnabled = FParse::Param(CommandLine, TEXT("PlaytestCapture"));

	FParse::Value(CommandLine, TEXT("PlaytestShots="), Config.NumShots);
	FParse::Value(CommandLine, TEXT("PlaytestShotDelay="), Config.DelaySeconds);
	FParse::Value(CommandLine, TEXT("PlaytestShotInterval="), Config.IntervalSeconds);
	Config.NumShots = FMath::Clamp(Config.NumShots, 0, 20);
	Config.DelaySeconds = FMath::Max(Config.DelaySeconds, 0.f);
	Config.IntervalSeconds = FMath::Max(Config.IntervalSeconds, 0.1f);

	if (!FParse::Value(CommandLine, TEXT("PlaytestShotDir="), Config.ShotDir) || Config.ShotDir.IsEmpty())
	{
		Config.ShotDir = FPaths::ProjectSavedDir() / TEXT("Playtests/Capture");
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

bool UMordecaiPlaytestCaptureSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	return Super::ShouldCreateSubsystem(Outer) && FMordecaiPlaytestCaptureConfig::FromCommandLine(FCommandLine::Get()).bEnabled;
#endif
}

void UMordecaiPlaytestCaptureSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Config = FMordecaiPlaytestCaptureConfig::FromCommandLine(FCommandLine::Get());
	IFileManager::Get().MakeDirectory(*Config.ShotDir, /*Tree*/ true);
	TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &ThisClass::Tick), 0.1f);

	if (ULoadingScreenManager* Manager = Collection.InitializeDependency<ULoadingScreenManager>())
	{
		LoadingScreenManager = Manager;
		Manager->OnLoadingScreenVisibilityChangedDelegate().AddUObject(this, &ThisClass::HandleLoadingScreenVisibilityChanged);
	}

	UE_LOG(LogMordecaiPlaytest, Display, TEXT("PlaytestCapture: %d screenshot(s) to %s, %.1fs after the experience loads, every %.1fs"),
		Config.NumShots, *Config.ShotDir, Config.DelaySeconds, Config.IntervalSeconds);
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
	if (NextShot >= Config.NumShots)
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
