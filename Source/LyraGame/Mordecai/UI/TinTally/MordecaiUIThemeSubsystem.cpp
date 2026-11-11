// Project Mordecai — local Tin Tally theme service (US-102)

#include "Mordecai/UI/TinTally/MordecaiUIThemeSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiUIThemeSubsystem)

UMordecaiUIThemeSubsystem::UMordecaiUIThemeSubsystem()
	: NativeDiorama(FMordecaiUIThemeColors::MakeDiorama())
	, NativeHighContrast(FMordecaiUIThemeColors::MakeHighContrast())
{
	// TODO(US-103): load DA_UITheme_Diorama and DA_UITheme_HighContrast once
	// those cooked assets exist; native colors remain the code-only fallback.
}

void UMordecaiUIThemeSubsystem::SetActiveTheme(EMordecaiUITheme NewTheme)
{
	if (ActiveTheme == NewTheme)
	{
		return;
	}

	ActiveTheme = NewTheme;
	BroadcastTheme();
}

FName UMordecaiUIThemeSubsystem::GetActiveThemeId() const
{
	return MordecaiUIThemeToName(ActiveTheme);
}

FMordecaiUIThemeColors UMordecaiUIThemeSubsystem::GetActiveColors() const
{
	return ResolveColors();
}

void UMordecaiUIThemeSubsystem::SetThemeAssets(
	UMordecaiUIThemeDataAsset* DioramaAsset,
	UMordecaiUIThemeDataAsset* HighContrastAsset)
{
	DioramaThemeAsset =
		DioramaAsset && DioramaAsset->Theme == EMordecaiUITheme::Diorama ? DioramaAsset : nullptr;
	HighContrastThemeAsset =
		HighContrastAsset && HighContrastAsset->Theme == EMordecaiUITheme::HighContrast
		? HighContrastAsset
		: nullptr;
	BroadcastTheme();
}

UMordecaiUIThemeSubsystem* UMordecaiUIThemeSubsystem::ResolveForWorld(const UObject* WorldContextObject)
{
	if (!GEngine || !WorldContextObject)
	{
		return nullptr;
	}

	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	return LocalPlayer ? LocalPlayer->GetSubsystem<UMordecaiUIThemeSubsystem>() : nullptr;
}

UMordecaiUIThemeSubsystem* UMordecaiUIThemeSubsystem::ResolveForWidget(const UUserWidget* Widget)
{
	const ULocalPlayer* LocalPlayer = Widget ? Widget->GetOwningLocalPlayer() : nullptr;
	return LocalPlayer
		? const_cast<ULocalPlayer*>(LocalPlayer)->GetSubsystem<UMordecaiUIThemeSubsystem>()
		: nullptr;
}

const FMordecaiUIThemeColors& UMordecaiUIThemeSubsystem::ResolveColors() const
{
	if (ActiveTheme == EMordecaiUITheme::HighContrast)
	{
		return HighContrastThemeAsset ? HighContrastThemeAsset->Colors : NativeHighContrast;
	}
	return DioramaThemeAsset ? DioramaThemeAsset->Colors : NativeDiorama;
}

void UMordecaiUIThemeSubsystem::BroadcastTheme()
{
	const FMordecaiUIThemeColors& Colors = ResolveColors();
	ThemeChangedNative.Broadcast(ActiveTheme, Colors);
	OnThemeChanged.Broadcast(ActiveTheme, Colors);
}
