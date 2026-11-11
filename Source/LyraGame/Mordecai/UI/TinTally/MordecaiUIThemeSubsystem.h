// Project Mordecai — local Tin Tally theme service (US-102)

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Mordecai/UI/TinTally/MordecaiUITheme.h"

#include "MordecaiUIThemeSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMordecaiUIThemeChanged, EMordecaiUITheme, Theme,
	const FMordecaiUIThemeColors&, Colors);
DECLARE_MULTICAST_DELEGATE_TwoParams(FMordecaiUIThemeChangedNative, EMordecaiUITheme,
	const FMordecaiUIThemeColors&);

/**
 * Per-local-player theme selection. Widgets subscribe once and retint their
 * existing brushes when OnThemeChanged fires; no gameplay UI reconstruction
 * is required.
 */
UCLASS()
class LYRAGAME_API UMordecaiUIThemeSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	UMordecaiUIThemeSubsystem();

	UFUNCTION(BlueprintCallable, Category = "Tin Tally|Theme")
	void SetActiveTheme(EMordecaiUITheme NewTheme);

	UFUNCTION(BlueprintPure, Category = "Tin Tally|Theme")
	EMordecaiUITheme GetActiveTheme() const { return ActiveTheme; }

	UFUNCTION(BlueprintPure, Category = "Tin Tally|Theme")
	FName GetActiveThemeId() const;

	UFUNCTION(BlueprintPure, Category = "Tin Tally|Theme")
	FMordecaiUIThemeColors GetActiveColors() const;

	UFUNCTION(BlueprintCallable, Category = "Tin Tally|Theme")
	void SetThemeAssets(UMordecaiUIThemeDataAsset* DioramaAsset, UMordecaiUIThemeDataAsset* HighContrastAsset);

	UFUNCTION(BlueprintPure, Category = "Tin Tally|Theme", meta = (WorldContext = "WorldContextObject"))
	static UMordecaiUIThemeSubsystem* ResolveForWorld(const UObject* WorldContextObject);

	UPROPERTY(BlueprintAssignable, Category = "Tin Tally|Theme")
	FMordecaiUIThemeChanged OnThemeChanged;

	FMordecaiUIThemeChangedNative& OnThemeChangedNative() { return ThemeChangedNative; }

private:
	const FMordecaiUIThemeColors& ResolveColors() const;
	void BroadcastTheme();

	UPROPERTY(Transient)
	TObjectPtr<UMordecaiUIThemeDataAsset> DioramaThemeAsset;

	UPROPERTY(Transient)
	TObjectPtr<UMordecaiUIThemeDataAsset> HighContrastThemeAsset;

	UPROPERTY(Transient)
	EMordecaiUITheme ActiveTheme = EMordecaiUITheme::Diorama;

	FMordecaiUIThemeColors NativeDiorama;
	FMordecaiUIThemeColors NativeHighContrast;
	FMordecaiUIThemeChangedNative ThemeChangedNative;
};
