// Project Mordecai — packaged Tin Tally visual acceptance gallery (US-103)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TimerManager.h"

#include "MordecaiTinTallyGallerySubsystem.generated.h"

/** Deterministic, command-line selected foundation sheet used only by non-Shipping builds. */
UCLASS()
class LYRAGAME_API UMordecaiTinTallyGalleryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

	/** Selects a deterministic fixture without mutating the process command line. */
	void SetFixtureSelectionForTest(const FString& InComponent, const FString& InTheme);

private:
	FString FixtureComponentOverride;
	FString FixtureThemeOverride;
};

/**
 * Launch with -TinTallyGallery and the Component/Theme/State/Output switches.
 * The subsystem adds the selected sheet to MovementTestMap and writes a
 * 1920x1080 viewport capture. Shipping builds never create it.
 */
UCLASS()
class LYRAGAME_API UMordecaiTinTallyGallerySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	static bool IsValidSelection(
		const FString& Component,
		const FString& Theme,
		const FString& State);

private:
	void OnWorldInitialized(UWorld* World, const UWorld::InitializationValues InitializationValues);
	void ShowGallery();
	void CaptureGallery();

	FDelegateHandle WorldInitializedHandle;
	FTimerHandle ShowTimer;
	FTimerHandle CaptureTimer;
	TWeakObjectPtr<UWorld> GalleryWorld;

	UPROPERTY(Transient)
	TObjectPtr<UMordecaiTinTallyGalleryWidget> GalleryWidget;
	FString OutputPath;
};
