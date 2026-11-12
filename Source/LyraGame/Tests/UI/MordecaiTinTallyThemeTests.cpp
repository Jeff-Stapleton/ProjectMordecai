// Project Mordecai — Tin Tally runtime contract tests (US-102)

#include "Algo/AllOf.h"
#include "Misc/AutomationTest.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Mordecai/UI/TinTally/MordecaiTallyWidget.h"
#include "Mordecai/UI/TinTally/MordecaiUIPrimitives.h"
#include "Mordecai/UI/TinTally/MordecaiUITheme.h"
#include "Mordecai/UI/TinTally/MordecaiUIThemeSubsystem.h"
#include "UObject/UnrealType.h"

namespace MordecaiTinTallyThemeTest
{
	struct FExpectedToken
	{
		const TCHAR* Name;
		const TCHAR* DioramaHex;
		float DioramaAlpha;
		const TCHAR* HighContrastHex;
		float HighContrastAlpha;
	};

	// Resolved literals from Design/TinTally/ds/project/tokens.json. Aliases are
	// repeated deliberately: this table verifies all 72 tokens in both themes.
	static const FExpectedToken ExpectedTokens[] = {
		{ TEXT("plate"), TEXT("163634"), 1.00f, TEXT("000000"), 1.00f },
		{ TEXT("plate-raised"), TEXT("1C403E"), 1.00f, TEXT("161616"), 1.00f },
		{ TEXT("well"), TEXT("091A19"), 1.00f, TEXT("000000"), 1.00f },
		{ TEXT("lip"), TEXT("5E8C84"), 1.00f, TEXT("FFFFFF"), 1.00f },
		{ TEXT("rim"), TEXT("EFE2C2"), 1.00f, TEXT("FFFFFF"), 1.00f },
		{ TEXT("rim-shade"), TEXT("55665C"), 1.00f, TEXT("FFFFFF"), 1.00f },
		{ TEXT("ink"), TEXT("0F1418"), 1.00f, TEXT("000000"), 1.00f },
		{ TEXT("enamel"), TEXT("FBF3E1"), 1.00f, TEXT("FFFFFF"), 1.00f },
		{ TEXT("enamel-shade"), TEXT("EADFC6"), 1.00f, TEXT("FFFFFF"), 1.00f },
		{ TEXT("veil"), TEXT("0F1418"), 0.72f, TEXT("000000"), 0.85f },
		{ TEXT("veil-death"), TEXT("0F1418"), 0.60f, TEXT("000000"), 0.75f },
		{ TEXT("text"), TEXT("FFF8EA"), 1.00f, TEXT("FFFFFF"), 1.00f },
		{ TEXT("text-muted"), TEXT("BFD0C8"), 1.00f, TEXT("E6E6E6"), 1.00f },
		{ TEXT("text-disabled"), TEXT("9DB3AC"), 1.00f, TEXT("C4C4C4"), 1.00f },
		{ TEXT("ink-muted"), TEXT("4D5753"), 1.00f, TEXT("333333"), 1.00f },
		{ TEXT("focus"), TEXT("FFFFFF"), 1.00f, TEXT("FFFF00"), 1.00f },
		{ TEXT("vital"), TEXT("FF6F91"), 1.00f, TEXT("FF8FAB"), 1.00f },
		{ TEXT("vital-trail"), TEXT("FFE3A0"), 1.00f, TEXT("FFE6A6"), 1.00f },
		{ TEXT("heal"), TEXT("6FE39A"), 1.00f, TEXT("7CFFAA"), 1.00f },
		{ TEXT("stamina-green"), TEXT("4CCB6E"), 1.00f, TEXT("5CFF8F"), 1.00f },
		{ TEXT("stamina-yellow"), TEXT("E6E04A"), 1.00f, TEXT("FFF05C"), 1.00f },
		{ TEXT("danger"), TEXT("FF5A3C"), 1.00f, TEXT("FF8A70"), 1.00f },
		{ TEXT("danger-text"), TEXT("FF9670"), 1.00f, TEXT("FFA88F"), 1.00f },
		{ TEXT("delta-up"), TEXT("6FE39A"), 1.00f, TEXT("7CFFAA"), 1.00f },
		{ TEXT("delta-down"), TEXT("FF5A3C"), 1.00f, TEXT("FF8A70"), 1.00f },
		{ TEXT("stamina-red"), TEXT("FF5A3C"), 1.00f, TEXT("FF8A70"), 1.00f },
		{ TEXT("foe"), TEXT("FF5A3C"), 1.00f, TEXT("FF8A70"), 1.00f },
		{ TEXT("posture"), TEXT("B8C4CF"), 1.00f, TEXT("E0E8F0"), 1.00f },
		{ TEXT("spell-points"), TEXT("A98DFF"), 1.00f, TEXT("C4B2FF"), 1.00f },
		{ TEXT("good"), TEXT("FFC23D"), 1.00f, TEXT("FFD45C"), 1.00f },
		{ TEXT("rarity-common"), TEXT("D3D8D6"), 1.00f, TEXT("EDEDED"), 1.00f },
		{ TEXT("rarity-green"), TEXT("5FD98A"), 1.00f, TEXT("7CF2A0"), 1.00f },
		{ TEXT("rarity-blue"), TEXT("6BB8FF"), 1.00f, TEXT("9CCFFF"), 1.00f },
		{ TEXT("rarity-purple"), TEXT("C69BFF"), 1.00f, TEXT("DCBEFF"), 1.00f },
		{ TEXT("rarity-red"), TEXT("FF8A8A"), 1.00f, TEXT("FFB0B0"), 1.00f },
		{ TEXT("rarity-gold"), TEXT("FFC23D"), 1.00f, TEXT("FFD45C"), 1.00f },
		{ TEXT("feat-common"), TEXT("DCE3DD"), 1.00f, TEXT("FFFFFF"), 1.00f },
		{ TEXT("feat-rare"), TEXT("45D6BE"), 1.00f, TEXT("66FFE6"), 1.00f },
		{ TEXT("feat-legendary"), TEXT("FF7ADA"), 1.00f, TEXT("FF9AE6"), 1.00f },
		{ TEXT("buff"), TEXT("6FE39A"), 1.00f, TEXT("7CFFAA"), 1.00f },
		{ TEXT("debuff"), TEXT("FF9670"), 1.00f, TEXT("FFA88F"), 1.00f },
		{ TEXT("dmg-physical"), TEXT("FFF8EA"), 1.00f, TEXT("FFFFFF"), 1.00f },
		{ TEXT("dmg-fire"), TEXT("FF8A3D"), 1.00f, TEXT("FFA460"), 1.00f },
		{ TEXT("dmg-frost"), TEXT("9BE6FF"), 1.00f, TEXT("B8F0FF"), 1.00f },
		{ TEXT("dmg-lightning"), TEXT("FFE45C"), 1.00f, TEXT("FFF07A"), 1.00f },
		{ TEXT("dmg-poison"), TEXT("B5E35A"), 1.00f, TEXT("C8F27A"), 1.00f },
		{ TEXT("dmg-corrosive"), TEXT("4FE3C1"), 1.00f, TEXT("70FFDB"), 1.00f },
		{ TEXT("dmg-arcane"), TEXT("C79BFF"), 1.00f, TEXT("DCBEFF"), 1.00f },
		{ TEXT("dmg-shadow"), TEXT("2B1838"), 1.00f, TEXT("000000"), 1.00f },
		{ TEXT("dmg-shadow-edge"), TEXT("E7D3FF"), 1.00f, TEXT("F0E0FF"), 1.00f },
		{ TEXT("dmg-radiant"), TEXT("FFF0A8"), 1.00f, TEXT("FFF7CC"), 1.00f },
		{ TEXT("biome-verdant"), TEXT("6E9E58"), 1.00f, TEXT("6E9E58"), 1.00f },
		{ TEXT("biome-searing"), TEXT("C9A066"), 1.00f, TEXT("C9A066"), 1.00f },
		{ TEXT("biome-mire"), TEXT("5C6B55"), 1.00f, TEXT("5C6B55"), 1.00f },
		{ TEXT("biome-crystal"), TEXT("8FB7C9"), 1.00f, TEXT("8FB7C9"), 1.00f },
		{ TEXT("biome-storm"), TEXT("8C93B8"), 1.00f, TEXT("8C93B8"), 1.00f },
		{ TEXT("biome-withered"), TEXT("7D7176"), 1.00f, TEXT("7D7176"), 1.00f },
		{ TEXT("world-blockout"), TEXT("E6E3DA"), 1.00f, TEXT("E6E3DA"), 1.00f },
		{ TEXT("world-blockout-line"), TEXT("D5D1C6"), 1.00f, TEXT("D5D1C6"), 1.00f },
		{ TEXT("world-grass"), TEXT("7DB55A"), 1.00f, TEXT("7DB55A"), 1.00f },
		{ TEXT("world-grass-dot"), TEXT("6FA34F"), 1.00f, TEXT("6FA34F"), 1.00f },
		{ TEXT("world-sand"), TEXT("E2C690"), 1.00f, TEXT("E2C690"), 1.00f },
		{ TEXT("world-sand-dot"), TEXT("D4B67C"), 1.00f, TEXT("D4B67C"), 1.00f },
		{ TEXT("world-mire"), TEXT("4E5A45"), 1.00f, TEXT("4E5A45"), 1.00f },
		{ TEXT("world-mire-dot"), TEXT("5C6B55"), 1.00f, TEXT("5C6B55"), 1.00f },
		{ TEXT("world-night"), TEXT("1E2430"), 1.00f, TEXT("1E2430"), 1.00f },
		{ TEXT("world-night-dot"), TEXT("283042"), 1.00f, TEXT("283042"), 1.00f },
		{ TEXT("world-snow"), TEXT("F2F4F7"), 1.00f, TEXT("F2F4F7"), 1.00f },
		{ TEXT("world-snow-dot"), TEXT("E3E8EE"), 1.00f, TEXT("E3E8EE"), 1.00f },
		{ TEXT("world-light"), TEXT("E6E3DA"), 1.00f, TEXT("E6E3DA"), 1.00f },
		{ TEXT("world-dark"), TEXT("1E2430"), 1.00f, TEXT("1E2430"), 1.00f },
		{ TEXT("world-shade"), TEXT("0F1418"), 0.28f, TEXT("0F1418"), 0.28f },
	};

	const FMordecaiUIPrimitiveLayer* FindLayer(const FMordecaiUIPrimitiveSpec& Spec, FName Name)
	{
		return Spec.Layers.FindByPredicate([Name](const FMordecaiUIPrimitiveLayer& Layer)
		{
			return Layer.Name == Name;
		});
	}

	bool IsRadius(const FSlateBrush& Brush, const FVector4& Expected)
	{
		return Brush.OutlineSettings.CornerRadii.Equals(Expected, UE_KINDA_SMALL_NUMBER);
	}

	FLinearColor BrushTint(const FSlateBrush& Brush)
	{
		return Brush.TintColor.GetSpecifiedColor();
	}

	UBorder* FindBuiltBorder(UOverlay* Overlay, FName LayerName)
	{
		if (!Overlay)
		{
			return nullptr;
		}
		const FString NameSuffix = FString::Printf(TEXT("_%s"), *LayerName.ToString());
		for (int32 Index = 0; Index < Overlay->GetChildrenCount(); ++Index)
		{
			UWidget* Child = Overlay->GetChildAt(Index);
			UBorder* Border = Cast<UBorder>(Child);
			if (USizeBox* SizeBox = Cast<USizeBox>(Child))
			{
				Border = SizeBox->GetChildrenCount() > 0 ? Cast<UBorder>(SizeBox->GetChildAt(0)) : nullptr;
			}
			if (Border && Border->GetName().EndsWith(NameSuffix))
			{
				return Border;
			}
		}
		return nullptr;
	}

	struct FThemeWidgetTestEnv
	{
		UWorld* World = nullptr;
		APlayerController* PlayerController = nullptr;
		ULocalPlayer* LocalPlayer = nullptr;
		TSharedPtr<SWidget> SlateWidget;
		bool bWorldContextCreated = false;
		bool bPlayerAdded = false;

		~FThemeWidgetTestEnv()
		{
			Teardown();
		}

		bool Setup()
		{
			Teardown();
			if (!GEngine)
			{
				return false;
			}
			World = UWorld::CreateWorld(EWorldType::Game, false);
			if (!World)
			{
				return false;
			}
			FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
			bWorldContextCreated = true;
			WorldContext.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
			PlayerController = World->SpawnActor<APlayerController>();
			LocalPlayer = NewObject<ULocalPlayer>(GEngine);
			if (!PlayerController || !LocalPlayer)
			{
				return false;
			}
			PlayerController->Player = LocalPlayer;
			LocalPlayer->PlayerController = PlayerController;
			LocalPlayer->PlayerAdded(nullptr, FPlatformMisc::GetPlatformUserForUserIndex(0));
			bPlayerAdded = true;
			return FLocalPlayerContext(LocalPlayer, World).IsValid();
		}

		UMordecaiTallyWidget* MakeTally()
		{
			UMordecaiTallyWidget* Widget = NewObject<UMordecaiTallyWidget>(World);
			Widget->SetPlayerContext(FLocalPlayerContext(LocalPlayer, World));
			Widget->Initialize();
			SlateWidget = Widget->TakeWidget();
			return Widget;
		}

		void Teardown()
		{
			SlateWidget.Reset();
			if (PlayerController)
			{
				PlayerController->Player = nullptr;
			}
			if (LocalPlayer)
			{
				if (bPlayerAdded)
				{
					LocalPlayer->PlayerRemoved();
					bPlayerAdded = false;
				}
				LocalPlayer->PlayerController = nullptr;
			}
			if (World)
			{
				if (bWorldContextCreated && GEngine)
				{
					GEngine->DestroyWorldContext(World);
					bWorldContextCreated = false;
				}
				World->DestroyWorld(false);
			}
			World = nullptr;
			PlayerController = nullptr;
			LocalPlayer = nullptr;
		}
	};

	UImage* GetBundleImage(const UMordecaiTallyWidget* Tally, int32 Index)
	{
		const UHorizontalBox* Box = Tally && Tally->WidgetTree
			? Cast<UHorizontalBox>(Tally->WidgetTree->RootWidget)
			: nullptr;
		if (!Box)
		{
			return nullptr;
		}
		if (UImage* LegacyImage = Cast<UImage>(Box->GetChildAt(Index)))
		{
			return LegacyImage;
		}
		return Tally->WidgetTree
			? Cast<UImage>(Tally->WidgetTree->FindWidget(
				FName(*FString::Printf(TEXT("TallyFill_%d"), Index))))
			: nullptr;
	}

	FLinearColor RarityColor(
		const FMordecaiUIThemeColors& Colors,
		EMordecaiItemRarity Rarity)
	{
		switch (Rarity)
		{
		case EMordecaiItemRarity::Green: return Colors.RarityGreen;
		case EMordecaiItemRarity::Blue: return Colors.RarityBlue;
		case EMordecaiItemRarity::Purple: return Colors.RarityPurple;
		case EMordecaiItemRarity::Red: return Colors.RarityRed;
		case EMordecaiItemRarity::Gold: return Colors.RarityGold;
		case EMordecaiItemRarity::Common:
		default:
			return Colors.RarityCommon;
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_Theme_SRGBConversion,
	"Mordecai.UI.Theme.SRGBConversion",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_Theme_SRGBConversion::RunTest(const FString& Parameters)
{
	using namespace MordecaiTinTallyThemeTest;

	const FLinearColor ExpectedPlate = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("163634")));
	const FLinearColor ConvertedPlate = FMordecaiUIThemeColors::FromSRGBHex(TEXT("#163634"));
	TestTrue("hex colors pass through FLinearColor::FromSRGBColor", ConvertedPlate.Equals(ExpectedPlate));
	TestEqual("0x16 has the hard-coded sRGB-to-linear result", ConvertedPlate.R, 0.00802f, 0.0001f);
	TestFalse("sRGB bytes are not treated as already-linear values",
		ConvertedPlate.Equals(FLinearColor(0x16 / 255.f, 0x36 / 255.f, 0x34 / 255.f, 1.f)));

	UScriptStruct* ColorsStruct = FMordecaiUIThemeColors::StaticStruct();
	TArray<const FStructProperty*> ColorProperties;
	for (TFieldIterator<FStructProperty> PropertyIt(ColorsStruct); PropertyIt; ++PropertyIt)
	{
		if (PropertyIt->Struct == TBaseStructure<FLinearColor>::Get())
		{
			ColorProperties.Add(*PropertyIt);
		}
	}
	const FMordecaiUIThemeColors Diorama = FMordecaiUIThemeColors::MakeDiorama();
	const FMordecaiUIThemeColors HighContrast = FMordecaiUIThemeColors::MakeHighContrast();
	UMordecaiUIThemeDataAsset* Asset = NewObject<UMordecaiUIThemeDataAsset>();
	TestNotNull("theme data asset owns the complete colors struct",
		FindFProperty<FStructProperty>(Asset->GetClass(), TEXT("Colors")));
	TestEqual("tokens.json has 72 runtime colors", static_cast<int32>(UE_ARRAY_COUNT(ExpectedTokens)), 72);
	TestEqual("the runtime token name table is complete",
		FMordecaiUIThemeColors::GetRuntimeColorTokenNames().Num(), 72);
	TestEqual("the reflected color property table is complete", ColorProperties.Num(), 72);

	for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(ExpectedTokens)); ++Index)
	{
		const FExpectedToken& Expected = ExpectedTokens[Index];
		const FName TokenName(Expected.Name);
		TestEqual(*FString::Printf(TEXT("token name %d"), Index),
			FMordecaiUIThemeColors::GetRuntimeColorTokenNames()[Index], TokenName);
		if (ColorProperties.IsValidIndex(Index))
		{
#if WITH_EDITOR
			TestEqual(*FString::Printf(TEXT("%s DisplayName"), Expected.Name),
				ColorProperties[Index]->GetDisplayNameText().ToString(), FString(Expected.Name));
#endif
		}

		const FLinearColor ExpectedDiorama =
			FMordecaiUIThemeColors::FromSRGBHex(Expected.DioramaHex, Expected.DioramaAlpha);
		const FLinearColor ExpectedHighContrast =
			FMordecaiUIThemeColors::FromSRGBHex(Expected.HighContrastHex, Expected.HighContrastAlpha);
		FLinearColor ActualDiorama;
		FLinearColor ActualHighContrast;
		TestTrue(*FString::Printf(TEXT("%s resolves in Diorama"), Expected.Name),
			Diorama.TryGetColorToken(TokenName, ActualDiorama));
		TestTrue(*FString::Printf(TEXT("%s resolves in High Contrast"), Expected.Name),
			HighContrast.TryGetColorToken(TokenName, ActualHighContrast));
		TestTrue(*FString::Printf(TEXT("%s Diorama literal"), Expected.Name),
			ActualDiorama.Equals(ExpectedDiorama, 0.0001f));
		TestTrue(*FString::Printf(TEXT("%s High Contrast literal"), Expected.Name),
			ActualHighContrast.Equals(ExpectedHighContrast, 0.0001f));

		Asset->Colors = Diorama;
		FLinearColor AssetColor;
		TestTrue(*FString::Printf(TEXT("%s resolves through the data asset"), Expected.Name),
			Asset->GetColorToken(TokenName, AssetColor));
		TestTrue(*FString::Printf(TEXT("%s data-asset value"), Expected.Name),
			AssetColor.Equals(ExpectedDiorama, 0.0001f));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_Theme_RuntimeSwapNotifiesWidgets,
	"Mordecai.UI.Theme.RuntimeSwapNotifiesWidgets",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_Theme_RuntimeSwapNotifiesWidgets::RunTest(const FString& Parameters)
{
	ULocalPlayer* LocalPlayer = NewObject<ULocalPlayer>(GEngine);
	UMordecaiUIThemeSubsystem* Service = NewObject<UMordecaiUIThemeSubsystem>(LocalPlayer);
	TestEqual("local theme defaults to Diorama", Service->GetActiveTheme(), EMordecaiUITheme::Diorama);
	TestEqual("Diorama id is stable", Service->GetActiveThemeId(), FName(TEXT("Diorama")));

	int32 NotificationCount = 0;
	EMordecaiUITheme NotifiedTheme = EMordecaiUITheme::Diorama;
	const FDelegateHandle Handle = Service->OnThemeChangedNative().AddLambda(
		[&NotificationCount, &NotifiedTheme](EMordecaiUITheme Theme, const FMordecaiUIThemeColors& Colors)
		{
			++NotificationCount;
			NotifiedTheme = Theme;
			(void)Colors;
		});

	Service->SetActiveTheme(EMordecaiUITheme::HighContrast);
	TestEqual("runtime selection swaps to High Contrast", Service->GetActiveTheme(), EMordecaiUITheme::HighContrast);
	TestEqual("live widget receives one notification", NotificationCount, 1);
	TestEqual("notification identifies High Contrast", NotifiedTheme, EMordecaiUITheme::HighContrast);
	TestTrue("High Contrast plate is black", Service->GetActiveColors().Plate.Equals(FLinearColor::Black));
	TestTrue("High Contrast focus is yellow",
		Service->GetActiveColors().Focus.Equals(FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("FFFF00")))));

	Service->SetActiveTheme(EMordecaiUITheme::HighContrast);
	TestEqual("selecting the current theme does not rebroadcast", NotificationCount, 1);
	Service->OnThemeChangedNative().Remove(Handle);

	UMordecaiUIThemeDataAsset* DioramaAsset = NewObject<UMordecaiUIThemeDataAsset>();
	DioramaAsset->Theme = EMordecaiUITheme::Diorama;
	DioramaAsset->Colors = FMordecaiUIThemeColors::MakeDiorama();
	DioramaAsset->Colors.Plate = FLinearColor(0.21f, 0.32f, 0.43f, 1.f);
	UMordecaiUIThemeDataAsset* HighContrastAsset = NewObject<UMordecaiUIThemeDataAsset>();
	HighContrastAsset->Theme = EMordecaiUITheme::HighContrast;
	HighContrastAsset->Colors = FMordecaiUIThemeColors::MakeHighContrast();
	HighContrastAsset->Colors.Plate = FLinearColor(0.71f, 0.62f, 0.53f, 1.f);
	Service->SetThemeAssets(DioramaAsset, HighContrastAsset);
	Service->SetActiveTheme(EMordecaiUITheme::Diorama);
	TestTrue("a correctly labelled Diorama asset supplies its colors",
		Service->GetActiveColors().Plate.Equals(DioramaAsset->Colors.Plate));
	Service->SetActiveTheme(EMordecaiUITheme::HighContrast);
	TestTrue("a correctly labelled High Contrast asset supplies its colors",
		Service->GetActiveColors().Plate.Equals(HighContrastAsset->Colors.Plate));

	UMordecaiUIThemeDataAsset* MislabelledDioramaSlot = NewObject<UMordecaiUIThemeDataAsset>();
	MislabelledDioramaSlot->Theme = EMordecaiUITheme::HighContrast;
	MislabelledDioramaSlot->Colors = FMordecaiUIThemeColors::MakeHighContrast();
	MislabelledDioramaSlot->Colors.Plate = FLinearColor::Red;
	Service->SetThemeAssets(MislabelledDioramaSlot, nullptr);
	Service->SetActiveTheme(EMordecaiUITheme::Diorama);
	TestTrue("a High Contrast-labelled asset in the Diorama slot resolves to native Diorama",
		Service->GetActiveColors().Plate.Equals(FMordecaiUIThemeColors::MakeDiorama().Plate));
	TestFalse("the rejected Diorama-slot asset's distinctive red is not observable",
		Service->GetActiveColors().Plate.Equals(FLinearColor::Red));

	UMordecaiUIThemeDataAsset* MislabelledHighContrastSlot = NewObject<UMordecaiUIThemeDataAsset>();
	MislabelledHighContrastSlot->Theme = EMordecaiUITheme::Diorama;
	MislabelledHighContrastSlot->Colors = FMordecaiUIThemeColors::MakeDiorama();
	MislabelledHighContrastSlot->Colors.Plate = FLinearColor::Red;
	Service->SetThemeAssets(nullptr, MislabelledHighContrastSlot);
	Service->SetActiveTheme(EMordecaiUITheme::HighContrast);
	TestTrue("a Diorama-labelled asset in the High Contrast slot resolves to native High Contrast",
		Service->GetActiveColors().Plate.Equals(FMordecaiUIThemeColors::MakeHighContrast().Plate));
	TestFalse("the rejected High Contrast-slot asset's distinctive red is not observable",
		Service->GetActiveColors().Plate.Equals(FLinearColor::Red));
	TestNotNull("ResolveForWidget is exposed to owning-player widgets",
		Service->GetClass()->FindFunctionByName(TEXT("ResolveForWidget")));

	using namespace MordecaiTinTallyThemeTest;
	FThemeWidgetTestEnv Env;
	if (!TestTrue("theme widget test world", Env.Setup()))
	{
		return true;
	}
	UMordecaiUIThemeSubsystem* PlayerService =
		Env.LocalPlayer->GetSubsystem<UMordecaiUIThemeSubsystem>();
	UMordecaiUIThemeDataAsset* FreshDiorama = PlayerService
		? PlayerService->GetThemeAsset(EMordecaiUITheme::Diorama)
		: nullptr;
	UMordecaiUIThemeDataAsset* FreshHighContrast = PlayerService
		? PlayerService->GetThemeAsset(EMordecaiUITheme::HighContrast)
		: nullptr;
	TestNotNull("fresh local-player subsystem resolves the saved Diorama theme asset",
		FreshDiorama);
	TestNotNull("fresh local-player subsystem resolves the saved High Contrast theme asset",
		FreshHighContrast);
	TestTrue("fresh Diorama asset uses the canonical saved path",
		FreshDiorama
		&& FreshDiorama->GetPathName()
			== TEXT("/MordecaiCore/UI/TinTally/Themes/DA_UITheme_Diorama.DA_UITheme_Diorama"));
	TestTrue("fresh High Contrast asset uses the canonical saved path",
		FreshHighContrast
		&& FreshHighContrast->GetPathName()
			== TEXT("/MordecaiCore/UI/TinTally/Themes/DA_UITheme_HighContrast.DA_UITheme_HighContrast"));
	UMordecaiTallyWidget* Tally = Env.MakeTally();
	UHorizontalBox* OriginalBox = Tally && Tally->WidgetTree
		? Cast<UHorizontalBox>(Tally->WidgetTree->RootWidget)
		: nullptr;
	UImage* OriginalImage = GetBundleImage(Tally, 0);
	TestNotNull("the local player owns one theme service", PlayerService);
	TestNotNull("the live tally builds an image", OriginalImage);
	const int32 OriginalChildCount = OriginalBox ? OriginalBox->GetChildrenCount() : 0;
	const FLinearColor DioramaTint = OriginalImage
		? OriginalImage->GetColorAndOpacity()
		: FLinearColor::Transparent;
	TestTrue("the live tally starts in the Diorama text tint",
		DioramaTint.Equals(FMordecaiUIThemeColors::MakeDiorama().Text, 0.0001f));
	if (PlayerService)
	{
		PlayerService->SetActiveTheme(EMordecaiUITheme::HighContrast);
	}
	TestTrue("theme swap preserves the tally widget pointer", Tally != nullptr);
	TestTrue("theme swap preserves the root pointer", Tally && Tally->WidgetTree->RootWidget == OriginalBox);
	TestTrue("theme swap preserves the rendered image pointer", GetBundleImage(Tally, 0) == OriginalImage);
	TestEqual("theme swap preserves the child count",
		OriginalBox ? OriginalBox->GetChildrenCount() : 0, OriginalChildCount);
	const FLinearColor HighContrastTint = OriginalImage
		? OriginalImage->GetColorAndOpacity()
		: FLinearColor::Transparent;
	TestTrue("the same live image retints to High Contrast",
		HighContrastTint.Equals(FMordecaiUIThemeColors::MakeHighContrast().Text, 0.0001f));
	TestFalse("the tint actually changed", DioramaTint.Equals(HighContrastTint, 0.0001f));
	TestTrue("the constructed tally is bound to the local player's theme delegate",
		PlayerService && PlayerService->OnThemeChangedNative().IsBoundToObject(Tally));
	Env.SlateWidget.Reset();
	if (Tally)
	{
		Tally->ReleaseSlateResources(true);
	}
	TestFalse("the theme delegate is unbound after the live widget is destroyed",
		PlayerService && PlayerService->OnThemeChangedNative().IsBoundToObject(Tally));
	Env.Teardown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_TallyWidget_UsageTints,
	"Mordecai.UI.TallyWidget.UsageTints",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_TallyWidget_UsageTints::RunTest(const FString& Parameters)
{
	using namespace MordecaiTinTallyThemeTest;
	FThemeWidgetTestEnv Env;
	if (!TestTrue("tally usage tint test world", Env.Setup()))
	{
		return true;
	}
	UMordecaiUIThemeSubsystem* Service =
		Env.LocalPlayer->GetSubsystem<UMordecaiUIThemeSubsystem>();
	UMordecaiTallyWidget* Tally = Env.MakeTally();
	Tally->SetCountAndMax(3, 5);
	UImage* Fill = GetBundleImage(Tally, 0);
	if (!TestNotNull("usage tint test has a rendered fill", Fill) || !Service)
	{
		return true;
	}

	for (EMordecaiUITheme Theme :
		{ EMordecaiUITheme::Diorama, EMordecaiUITheme::HighContrast })
	{
		Service->SetActiveTheme(
			Theme == EMordecaiUITheme::Diorama
				? EMordecaiUITheme::HighContrast
				: EMordecaiUITheme::Diorama);
		Service->SetActiveTheme(Theme);
		const FMordecaiUIThemeColors Colors = Service->GetActiveColors();
		const FString ThemeName =
			StaticEnum<EMordecaiUITheme>()->GetNameStringByValue(static_cast<int64>(Theme));

		Tally->SetUsage(EMordecaiTallyUsage::SkillRank);
		TestTrue(*FString::Printf(TEXT("%s skill rank uses text"), *ThemeName),
			Fill->GetColorAndOpacity().Equals(Colors.Text, 0.0001f));
		Tally->SetUsage(EMordecaiTallyUsage::LoadingThrobber);
		TestTrue(*FString::Printf(TEXT("%s loading throbber uses text"), *ThemeName),
			Fill->GetColorAndOpacity().Equals(Colors.Text, 0.0001f));
		Tally->SetSealOnEnamel(false);
		Tally->SetUsage(EMordecaiTallyUsage::Seal);
		TestTrue(*FString::Printf(TEXT("%s plate seal uses rim"), *ThemeName),
			Fill->GetColorAndOpacity().Equals(Colors.Rim, 0.0001f));
		Tally->SetSealOnEnamel(true);
		TestTrue(*FString::Printf(TEXT("%s enamel seal stamp uses ink"), *ThemeName),
			Fill->GetColorAndOpacity().Equals(Colors.Ink, 0.0001f));

		for (EMordecaiItemRarity Rarity : {
			EMordecaiItemRarity::Common, EMordecaiItemRarity::Green,
			EMordecaiItemRarity::Blue, EMordecaiItemRarity::Purple,
			EMordecaiItemRarity::Red, EMordecaiItemRarity::Gold })
		{
			Tally->SetRarityTier(Rarity);
			Tally->SetUsage(EMordecaiTallyUsage::RarityBadge);
			const FString RarityName =
				StaticEnum<EMordecaiItemRarity>()->GetNameStringByValue(
					static_cast<int64>(Rarity));
			TestTrue(*FString::Printf(TEXT("%s rarity %s uses its explicit token, never Good"),
				*ThemeName, *RarityName),
				Fill->GetColorAndOpacity().Equals(RarityColor(Colors, Rarity), 0.0001f));
		}
	}
	Env.SlateWidget.Reset();
	Tally->ReleaseSlateResources(true);
	Env.Teardown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_Primitives_RoundedBoxManifest,
	"Mordecai.UI.Primitives.RoundedBoxManifest",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_Primitives_RoundedBoxManifest::RunTest(const FString& Parameters)
{
	using namespace MordecaiTinTallyThemeTest;

	const FMordecaiUIThemeColors Colors = FMordecaiUIThemeColors::MakeDiorama();
	const FMordecaiUIPrimitiveSpec Plate = FMordecaiUIPrimitiveBuilder::Plate(Colors, EMordecaiUIPrintDepth::HUD);
	TestEqual("plate has print, outline, rim shade, rim, and fill", Plate.Layers.Num(), 5);
	const FMordecaiUIPrimitiveLayer* PlatePrint = FindLayer(Plate, TEXT("Print"));
	const FMordecaiUIPrimitiveLayer* PlateOutline = FindLayer(Plate, TEXT("Outline"));
	const FMordecaiUIPrimitiveLayer* PlateRimShade = FindLayer(Plate, TEXT("RimShade"));
	const FMordecaiUIPrimitiveLayer* PlateRim = FindLayer(Plate, TEXT("Rim"));
	const FMordecaiUIPrimitiveLayer* PlateFill = FindLayer(Plate, TEXT("Fill"));
	TestNotNull("plate print layer", PlatePrint);
	TestNotNull("plate outline layer", PlateOutline);
	TestNotNull("plate rim-shade layer", PlateRimShade);
	TestNotNull("plate rim layer", PlateRim);
	TestNotNull("plate fill layer", PlateFill);
	if (PlatePrint && PlateOutline && PlateRimShade && PlateRim && PlateFill)
	{
		TestTrue("print radius 15", IsRadius(PlatePrint->Brush, FVector4(15.f)));
		TestTrue("outline radius 15", IsRadius(PlateOutline->Brush, FVector4(15.f)));
		TestTrue("rim-shade radius 12", IsRadius(PlateRimShade->Brush, FVector4(12.f)));
		TestTrue("rim radius 12", IsRadius(PlateRim->Brush, FVector4(12.f)));
		TestTrue("fill radius 10", IsRadius(PlateFill->Brush, FVector4(10.f)));
		TestTrue("HUD print offset is 4", PlatePrint->Translation == FVector2D(0.f, 4.f));
		TestTrue("rim shade inset is 3", PlateRimShade->Padding == FMargin(3.f));
		TestTrue("rim inset is 3/3/5/5", PlateRim->Padding == FMargin(3.f, 3.f, 5.f, 5.f));
		TestTrue("fill inset is 5", PlateFill->Padding == FMargin(5.f));
	}
	TestEqual("plate outer stroke is 3", Plate.OuterOutlineWidth, 3.f);
	TestEqual("plate rim stroke is 2", Plate.InnerStrokeWidth, 2.f);

	const FMordecaiUIPrimitiveSpec MenuPlate =
		FMordecaiUIPrimitiveBuilder::Plate(Colors, EMordecaiUIPrintDepth::Menu);
	const FMordecaiUIPrimitiveLayer* MenuPrint = FindLayer(MenuPlate, TEXT("Print"));
	TestTrue("menu print offset is 6", MenuPrint && MenuPrint->Translation == FVector2D(0.f, 6.f));
	TestEqual("menu print keeps the canonical 0.55 alpha",
		MenuPrint ? BrushTint(MenuPrint->Brush).A : 0.f, 0.55f, 0.0001f);

	const FMordecaiUIThemeColors HighContrastColors = FMordecaiUIThemeColors::MakeHighContrast();
	const FMordecaiUIPrimitiveSpec HighContrastPlate = FMordecaiUIPrimitiveBuilder::Plate(
		HighContrastColors, EMordecaiUIPrintDepth::HUD, EMordecaiUITheme::HighContrast);
	const FMordecaiUIPrimitiveLayer* HighContrastPrint = FindLayer(HighContrastPlate, TEXT("Print"));
	const FMordecaiUIPrimitiveLayer* HighContrastOutline = FindLayer(HighContrastPlate, TEXT("Outline"));
	const FMordecaiUIPrimitiveLayer* HighContrastRim = FindLayer(HighContrastPlate, TEXT("Rim"));
	const FMordecaiUIPrimitiveLayer* HighContrastFill = FindLayer(HighContrastPlate, TEXT("Fill"));
	TestEqual("High Contrast plate outline is 4", HighContrastPlate.OuterOutlineWidth, 4.f);
	TestTrue("High Contrast print is opaque ink",
		HighContrastPrint && BrushTint(HighContrastPrint->Brush).Equals(HighContrastColors.Ink));
	TestTrue("High Contrast outer radius grows to 16",
		HighContrastOutline && IsRadius(HighContrastOutline->Brush, FVector4(16.f)));
	TestTrue("High Contrast rim inset shifts to 4/4/6/6",
		HighContrastRim && HighContrastRim->Padding == FMargin(4.f, 4.f, 6.f, 6.f));
	TestTrue("High Contrast fill inset shifts to 6",
		HighContrastFill && HighContrastFill->Padding == FMargin(6.f));
	TestTrue("High Contrast concentric rim radius remains 12",
		HighContrastRim && IsRadius(HighContrastRim->Brush, FVector4(12.f)));
	TestTrue("High Contrast concentric fill radius remains 10",
		HighContrastFill && IsRadius(HighContrastFill->Brush, FVector4(10.f)));

	const FMordecaiUIPrimitiveSpec Pill = FMordecaiUIPrimitiveBuilder::Pill(Colors);
	TestEqual("pill uses the five plate layers", Pill.Layers.Num(), 5);
	TestTrue("pill radii follow half-height mode",
		Algo::AllOf(Pill.Layers, [](const FMordecaiUIPrimitiveLayer& Layer)
		{
			return Layer.Brush.OutlineSettings.RoundingType == ESlateBrushRoundingType::HalfHeightRadius;
		}));
	const FMordecaiUIPrimitiveSpec HighContrastPill = FMordecaiUIPrimitiveBuilder::Pill(
		HighContrastColors, EMordecaiUIPrintDepth::HUD, EMordecaiUITheme::HighContrast);
	TestEqual("High Contrast pill outline is 4", HighContrastPill.OuterOutlineWidth, 4.f);
	const FMordecaiUIPrimitiveLayer* HighContrastPillFill = FindLayer(HighContrastPill, TEXT("Fill"));
	TestTrue("High Contrast pill fill inset shifts to 6",
		HighContrastPillFill && HighContrastPillFill->Padding == FMargin(6.f));

	const FMordecaiUIPrimitiveSpec Tag = FMordecaiUIPrimitiveBuilder::Tag(Colors);
	TestEqual("tag has three face and three grommet layers", Tag.Layers.Num(), 6);
	const FMordecaiUIPrimitiveLayer* TagOutline = FindLayer(Tag, TEXT("Outline"));
	const FMordecaiUIPrimitiveLayer* TagFace = FindLayer(Tag, TEXT("Face"));
	const FMordecaiUIPrimitiveLayer* GrommetInk = FindLayer(Tag, TEXT("GrommetInk"));
	const FMordecaiUIPrimitiveLayer* GrommetShade = FindLayer(Tag, TEXT("GrommetShade"));
	const FMordecaiUIPrimitiveLayer* GrommetWell = FindLayer(Tag, TEXT("GrommetWell"));
	TestTrue("tag outline radii are 7/13/13/7",
		TagOutline && IsRadius(TagOutline->Brush, FVector4(7.f, 13.f, 13.f, 7.f)));
	TestTrue("tag face radii are 4/10/10/4",
		TagFace && IsRadius(TagFace->Brush, FVector4(4.f, 10.f, 10.f, 4.f)));
	TestTrue("tag face inset is 3", TagFace && TagFace->Padding == FMargin(3.f));
	TestTrue("Diorama grommet center is 22px from the face and 25px from the overlay edge",
		GrommetInk && GrommetShade && GrommetWell
		&& GrommetInk->Center.X == 25.f && GrommetShade->Center.X == 25.f && GrommetWell->Center.X == 25.f);
	TestTrue("grommet vertical position is represented by center alignment",
		GrommetInk && GrommetShade && GrommetWell
		&& GrommetInk->Center.Y == 0.f && GrommetShade->Center.Y == 0.f && GrommetWell->Center.Y == 0.f
		&& GrommetInk->VerticalAlignment == VAlign_Center
		&& GrommetShade->VerticalAlignment == VAlign_Center
		&& GrommetWell->VerticalAlignment == VAlign_Center);
	TestTrue("grommet sizes are 22/18/12",
		GrommetInk && GrommetShade && GrommetWell
		&& GrommetInk->DesiredSize == FVector2D(22.f)
		&& GrommetShade->DesiredSize == FVector2D(18.f)
		&& GrommetWell->DesiredSize == FVector2D(12.f));
	const FMordecaiUIPrimitiveSpec HighContrastTag = FMordecaiUIPrimitiveBuilder::Tag(
		HighContrastColors, EMordecaiUIPrintDepth::HUD, EMordecaiUITheme::HighContrast);
	const FMordecaiUIPrimitiveLayer* HighContrastTagPrint = FindLayer(HighContrastTag, TEXT("Print"));
	const FMordecaiUIPrimitiveLayer* HighContrastTagOutline = FindLayer(HighContrastTag, TEXT("Outline"));
	const FMordecaiUIPrimitiveLayer* HighContrastTagFace = FindLayer(HighContrastTag, TEXT("Face"));
	const FMordecaiUIPrimitiveLayer* HighContrastGrommetInk = FindLayer(HighContrastTag, TEXT("GrommetInk"));
	const FMordecaiUIPrimitiveLayer* HighContrastGrommetShade = FindLayer(HighContrastTag, TEXT("GrommetShade"));
	const FMordecaiUIPrimitiveLayer* HighContrastGrommetWell = FindLayer(HighContrastTag, TEXT("GrommetWell"));
	TestEqual("High Contrast tag outline is 4", HighContrastTag.OuterOutlineWidth, 4.f);
	TestTrue("High Contrast tag print is opaque",
		HighContrastTagPrint && BrushTint(HighContrastTagPrint->Brush).A == 1.f);
	TestTrue("High Contrast tag outer radii grow by 1",
		HighContrastTagOutline && IsRadius(HighContrastTagOutline->Brush, FVector4(8.f, 14.f, 14.f, 8.f)));
	TestTrue("High Contrast tag face inset shifts to 4",
		HighContrastTagFace && HighContrastTagFace->Padding == FMargin(4.f));
	TestTrue("High Contrast tag face radii remain concentric",
		HighContrastTagFace && IsRadius(HighContrastTagFace->Brush, FVector4(4.f, 10.f, 10.f, 4.f)));
	TestTrue("High Contrast grommet center is 22px from the face and 26px from the overlay edge",
		HighContrastGrommetInk && HighContrastGrommetShade && HighContrastGrommetWell
		&& HighContrastGrommetInk->Center.X == 26.f
		&& HighContrastGrommetShade->Center.X == 26.f
		&& HighContrastGrommetWell->Center.X == 26.f);

	const FMordecaiUIPrimitiveSpec Well = FMordecaiUIPrimitiveBuilder::Well(Colors, false);
	const FMordecaiUIPrimitiveLayer* WellLayer = FindLayer(Well, TEXT("Well"));
	TestTrue("well radius is 8", WellLayer && IsRadius(WellLayer->Brush, FVector4(8.f)));
	TestEqual("well lip is 2", Well.InnerStrokeWidth, 2.f);
	TestEqual("well brush draws the 2px lip", WellLayer ? WellLayer->Brush.OutlineSettings.Width : 0.f, 2.f);
	const FMordecaiUIPrimitiveSpec BarWell = FMordecaiUIPrimitiveBuilder::Well(Colors, true);
	const FMordecaiUIPrimitiveLayer* BarLayer = FindLayer(BarWell, TEXT("Well"));
	TestTrue("bar well radius is 6", BarLayer && IsRadius(BarLayer->Brush, FVector4(6.f)));
	const FMordecaiUIPrimitiveSpec InsetCardWell = FMordecaiUIPrimitiveBuilder::InsetCardWell(Colors);
	const FMordecaiUIPrimitiveLayer* InsetCardLayer = FindLayer(InsetCardWell, TEXT("InsetCard"));
	TestEqual("inset-card well lip is 2", InsetCardWell.InnerStrokeWidth, 2.f);
	TestTrue("inset-card well radius is 12",
		InsetCardLayer && IsRadius(InsetCardLayer->Brush, FVector4(12.f)));
	TestTrue("inset-card well fill is plate-raised",
		InsetCardLayer && BrushTint(InsetCardLayer->Brush).Equals(Colors.PlateRaised));
	TestEqual("inset-card well lip brush is 2",
		InsetCardLayer ? InsetCardLayer->Brush.OutlineSettings.Width : 0.f, 2.f);

	TArray<FMordecaiUIPrimitiveSpec> Keycaps;
	for (const EMordecaiKeycapShape Shape :
		{ EMordecaiKeycapShape::Square, EMordecaiKeycapShape::Round, EMordecaiKeycapShape::Pill })
	{
		const FMordecaiUIPrimitiveSpec Keycap = FMordecaiUIPrimitiveBuilder::Keycap(Colors, Shape);
		Keycaps.Add(Keycap);
		const FString ShapeName = StaticEnum<EMordecaiKeycapShape>()->GetNameStringByValue(
			static_cast<int64>(Shape));
		const FMordecaiUIPrimitiveLayer* KeycapLip = FindLayer(Keycap, TEXT("Lip"));
		const FMordecaiUIPrimitiveLayer* KeycapFace = FindLayer(Keycap, TEXT("Face"));
		TestEqual(*FString::Printf(TEXT("%s keycap has base and inset face"), *ShapeName),
			Keycap.Layers.Num(), 2);
		TestTrue(*FString::Printf(TEXT("%s keycap draws Lip before Face"), *ShapeName),
			Keycap.Layers.Num() == 2
			&& Keycap.Layers[0].Name == TEXT("Lip")
			&& Keycap.Layers[1].Name == TEXT("Face"));
		TestTrue(*FString::Printf(TEXT("%s keycap base uses enamel-shade"), *ShapeName),
			KeycapLip && BrushTint(KeycapLip->Brush).Equals(Colors.EnamelShade));
		TestEqual(*FString::Printf(TEXT("%s keycap base has one 2px ink border"), *ShapeName),
			KeycapLip ? KeycapLip->Brush.OutlineSettings.Width : 0.f, 2.f);
		TestTrue(*FString::Printf(TEXT("%s keycap base outline uses ink"), *ShapeName),
			KeycapLip
			&& KeycapLip->Brush.OutlineSettings.Color.GetSpecifiedColor().Equals(Colors.Ink));
		TestTrue(*FString::Printf(TEXT("%s keycap base has zero padding"), *ShapeName),
			KeycapLip && KeycapLip->Padding == FMargin(0.f));
		TestTrue(*FString::Printf(TEXT("%s keycap face uses enamel"), *ShapeName),
			KeycapFace && BrushTint(KeycapFace->Brush).Equals(Colors.Enamel));
		TestEqual(*FString::Printf(TEXT("%s keycap face has no second outline"), *ShapeName),
			KeycapFace ? KeycapFace->Brush.OutlineSettings.Width : -1.f, 0.f);
		TestTrue(*FString::Printf(TEXT("%s keycap face is inset 2/2/2/6"), *ShapeName),
			KeycapFace && KeycapFace->Padding == FMargin(2.f, 2.f, 2.f, 6.f));
		TestTrue(*FString::Printf(TEXT("%s keycap layers stay inside the shape"), *ShapeName),
			KeycapLip && KeycapFace
			&& KeycapLip->Translation.IsNearlyZero() && KeycapFace->Translation.IsNearlyZero()
			&& KeycapLip->DesiredSize.IsNearlyZero() && KeycapFace->DesiredSize.IsNearlyZero());
		if (Shape == EMordecaiKeycapShape::Square)
		{
			TestTrue("square keycap outer radius is 8",
				KeycapLip && IsRadius(KeycapLip->Brush, FVector4(8.f)));
			TestTrue("square keycap inner radius is 6",
				KeycapFace && IsRadius(KeycapFace->Brush, FVector4(6.f)));
		}
		else
		{
			TestTrue(*FString::Printf(TEXT("%s keycap base is half-height"), *ShapeName),
				KeycapLip && KeycapLip->Brush.OutlineSettings.RoundingType
					== ESlateBrushRoundingType::HalfHeightRadius);
			TestTrue(*FString::Printf(TEXT("%s keycap inset face is half-height"), *ShapeName),
				KeycapFace && KeycapFace->Brush.OutlineSettings.RoundingType
					== ESlateBrushRoundingType::HalfHeightRadius);
		}
	}
	const FMordecaiUIPrimitiveSpec& Keycap = Keycaps[0];

	const FMordecaiUIPrimitiveSpec Focus =
		FMordecaiUIPrimitiveBuilder::Focus(Colors, 12.f, EMordecaiUITheme::Diorama);
	TestEqual("focus ring is 3", Focus.FocusStrokeWidth, 3.f);
	TestEqual("focus keyline is 2", Focus.FocusKeylineWidth, 2.f);
	TestEqual("focus sits outside a 3px Diorama piece outline", Focus.FocusOffset, 3.f);
	const FMordecaiUIPrimitiveLayer* FocusRing = FindLayer(Focus, TEXT("Focus"));
	const FMordecaiUIPrimitiveLayer* FocusKeyline = FindLayer(Focus, TEXT("Keyline"));
	TestTrue("focus ring radius includes offset and focus stroke",
		FocusRing && IsRadius(FocusRing->Brush, FVector4(18.f)));
	TestTrue("focus keyline radius adds the keyline width",
		FocusKeyline && IsRadius(FocusKeyline->Brush, FVector4(20.f)));
	const FMordecaiUIPrimitiveSpec HighContrastFocus =
		FMordecaiUIPrimitiveBuilder::Focus(FMordecaiUIThemeColors::MakeHighContrast(), 12.f,
			EMordecaiUITheme::HighContrast);
	TestEqual("High Contrast piece outline is 4", HighContrastFocus.FocusOffset, 4.f);
	const FMordecaiUIPrimitiveSpec BorderFocus = FMordecaiUIPrimitiveBuilder::Focus(
		Colors, 8.f, EMordecaiUITheme::Diorama, EMordecaiUIFocusVariant::Border);
	TestEqual("border focus has zero offset", BorderFocus.FocusOffset, 0.f);
	TestNotNull("border focus keeps the outer keyline", FindLayer(BorderFocus, TEXT("Keyline")));
	const FMordecaiUIPrimitiveSpec FillGapFocus = FMordecaiUIPrimitiveBuilder::Focus(
		Colors, 12.f, EMordecaiUITheme::Diorama, EMordecaiUIFocusVariant::FillGap);
	const FMordecaiUIPrimitiveLayer* GapLayer = FindLayer(FillGapFocus, TEXT("Gap"));
	TestNotNull("fill-gap focus has an ink gap layer", GapLayer);
	TestNull("fill-gap focus has no keyline", FindLayer(FillGapFocus, TEXT("Keyline")));
	TestTrue("fill-gap layer uses ink",
		GapLayer && GapLayer->Brush.OutlineSettings.Color.GetSpecifiedColor().Equals(Colors.Ink));

	for (const EMordecaiUITheme FocusTheme :
		{ EMordecaiUITheme::Diorama, EMordecaiUITheme::HighContrast })
	{
		const FMordecaiUIThemeColors ThemeColors = FocusTheme == EMordecaiUITheme::HighContrast
			? FMordecaiUIThemeColors::MakeHighContrast()
			: FMordecaiUIThemeColors::MakeDiorama();
		const float PieceOutline = FocusTheme == EMordecaiUITheme::HighContrast ? 4.f : 3.f;
		for (const EMordecaiUIFocusVariant Variant :
			{ EMordecaiUIFocusVariant::Default, EMordecaiUIFocusVariant::Border,
				EMordecaiUIFocusVariant::FillGap })
		{
			const FString CaseName = FString::Printf(TEXT("%s.%s"),
				*StaticEnum<EMordecaiUITheme>()->GetNameStringByValue(static_cast<int64>(FocusTheme)),
				*StaticEnum<EMordecaiUIFocusVariant>()->GetNameStringByValue(static_cast<int64>(Variant)));
			const float ExpectedOffset = Variant == EMordecaiUIFocusVariant::Border ? 0.f : PieceOutline;
			const float ExpectedRingRadius = 12.f + ExpectedOffset + 3.f;
			const FMordecaiUIPrimitiveSpec FocusCase =
				FMordecaiUIPrimitiveBuilder::Focus(ThemeColors, 12.f, FocusTheme, Variant);
			UWidgetTree* FocusTree = NewObject<UWidgetTree>();
			UOverlay* BuiltFocus = FMordecaiUIPrimitiveBuilder::Build(
				FocusTree, FocusCase, FName(*FString::Printf(
					TEXT("FocusCase_%d_%d"), static_cast<int32>(FocusTheme), static_cast<int32>(Variant))));
			UBorder* BuiltRing = FindBuiltBorder(BuiltFocus, TEXT("Focus"));
			UOverlaySlot* RingSlot = BuiltRing ? Cast<UOverlaySlot>(BuiltRing->Slot) : nullptr;
			TestTrue(*FString::Printf(TEXT("%s Build materializes the focus ring"), *CaseName),
				BuiltRing && RingSlot);
			TestTrue(*FString::Printf(TEXT("%s focus radius is concentric"), *CaseName),
				BuiltRing && IsRadius(BuiltRing->Background, FVector4(ExpectedRingRadius)));
			if (Variant == EMordecaiUIFocusVariant::FillGap)
			{
				UBorder* BuiltGap = FindBuiltBorder(BuiltFocus, TEXT("Gap"));
				UOverlaySlot* GapSlot = BuiltGap ? Cast<UOverlaySlot>(BuiltGap->Slot) : nullptr;
				TestTrue(*FString::Printf(TEXT("%s gap is inset behind the focus ring"), *CaseName),
					GapSlot && GapSlot->GetPadding() == FMargin(3.f));
				TestEqual(*FString::Printf(TEXT("%s gap stroke follows the theme outline"), *CaseName),
					BuiltGap ? BuiltGap->Background.OutlineSettings.Width : 0.f, PieceOutline);
				TestTrue(*FString::Printf(TEXT("%s inset gap radius stays concentric"), *CaseName),
					BuiltGap && IsRadius(BuiltGap->Background, FVector4(12.f + ExpectedOffset)));
				TestTrue(*FString::Printf(TEXT("%s focus ring keeps the outer bounds"), *CaseName),
					RingSlot && RingSlot->GetPadding() == FMargin(0.f));
			}
			else
			{
				UBorder* BuiltKeyline = FindBuiltBorder(BuiltFocus, TEXT("Keyline"));
				UOverlaySlot* KeylineSlot =
					BuiltKeyline ? Cast<UOverlaySlot>(BuiltKeyline->Slot) : nullptr;
				TestTrue(*FString::Printf(TEXT("%s focus ring is inset inside the keyline"), *CaseName),
					RingSlot && RingSlot->GetPadding() == FMargin(2.f));
				TestTrue(*FString::Printf(TEXT("%s keyline keeps the outer bounds"), *CaseName),
					KeylineSlot && KeylineSlot->GetPadding() == FMargin(0.f));
				TestEqual(*FString::Printf(TEXT("%s keyline stroke is 2"), *CaseName),
					BuiltKeyline ? BuiltKeyline->Background.OutlineSettings.Width : 0.f, 2.f);
				TestTrue(*FString::Printf(TEXT("%s keyline radius stays concentric"), *CaseName),
					BuiltKeyline && IsRadius(
						BuiltKeyline->Background, FVector4(ExpectedRingRadius + 2.f)));
			}
		}
	}

	for (const FMordecaiUIPrimitiveSpec* Spec : { &Plate, &Pill, &Tag, &Well, &InsetCardWell, &Keycap, &Focus })
	{
		for (const FMordecaiUIPrimitiveLayer& Layer : Spec->Layers)
		{
			TestEqual(*FString::Printf(TEXT("%s is RoundedBox"), *Layer.Name.ToString()),
				Layer.Brush.DrawAs, ESlateBrushDrawType::RoundedBox);
			TestNull(*FString::Printf(TEXT("%s uses no texture"), *Layer.Name.ToString()),
				Layer.Brush.GetResourceObject());
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_Primitives_BuildMaterializesFillLayers,
	"Mordecai.UI.Primitives.BuildMaterializesFillLayers",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_Primitives_BuildMaterializesFillLayers::RunTest(const FString& Parameters)
{
	using namespace MordecaiTinTallyThemeTest;
	const FMordecaiUIThemeColors Colors = FMordecaiUIThemeColors::MakeDiorama();
	for (const TPair<FName, FMordecaiUIPrimitiveSpec>& Case : {
		TPair<FName, FMordecaiUIPrimitiveSpec>(TEXT("Plate"), FMordecaiUIPrimitiveBuilder::Plate(Colors)),
		TPair<FName, FMordecaiUIPrimitiveSpec>(TEXT("Well"), FMordecaiUIPrimitiveBuilder::Well(Colors, false))
	})
	{
		UWidgetTree* Tree = NewObject<UWidgetTree>();
		UOverlay* Built = FMordecaiUIPrimitiveBuilder::Build(Tree, Case.Value, Case.Key);
		if (!TestNotNull(*FString::Printf(TEXT("%s builds an overlay"), *Case.Key.ToString()), Built))
		{
			continue;
		}
		TestEqual(*FString::Printf(TEXT("%s materializes every layer"), *Case.Key.ToString()),
			Built->GetChildrenCount(), Case.Value.Layers.Num());
		for (int32 Index = 0; Index < Built->GetChildrenCount(); ++Index)
		{
			UBorder* Border = Cast<UBorder>(Built->GetChildAt(Index));
			UOverlaySlot* OverlaySlot = Border ? Cast<UOverlaySlot>(Border->Slot) : nullptr;
			TestNotNull(*FString::Printf(TEXT("%s layer %d is a border"), *Case.Key.ToString(), Index), Border);
			TestNotNull(*FString::Printf(TEXT("%s layer %d has an overlay slot"), *Case.Key.ToString(), Index), OverlaySlot);
			if (Border && OverlaySlot)
			{
				TestEqual("non-sized layer fills horizontally", OverlaySlot->GetHorizontalAlignment(), HAlign_Fill);
				TestEqual("non-sized layer fills vertically", OverlaySlot->GetVerticalAlignment(), VAlign_Fill);
				TestTrue("slot uses the layer padding",
					OverlaySlot->GetPadding() == Case.Value.Layers[Index].Padding);
				TestTrue("border content padding is zero", Border->GetPadding() == FMargin(0.f));
				TestEqual("border is hit-test invisible", Border->GetVisibility(), ESlateVisibility::HitTestInvisible);
				TestEqual("built brush draw type", Border->Background.DrawAs, Case.Value.Layers[Index].Brush.DrawAs);
				TestTrue("built brush tint",
					BrushTint(Border->Background).Equals(BrushTint(Case.Value.Layers[Index].Brush), 0.0001f));
			}
		}
	}

	const FMordecaiUIPrimitiveSpec Tag = FMordecaiUIPrimitiveBuilder::Tag(Colors);
	UWidgetTree* TagTree = NewObject<UWidgetTree>();
	UOverlay* BuiltTag = FMordecaiUIPrimitiveBuilder::Build(TagTree, Tag, TEXT("Tag"));
	USizeBox* GrommetBox = BuiltTag && BuiltTag->GetChildrenCount() > 3
		? Cast<USizeBox>(BuiltTag->GetChildAt(3))
		: nullptr;
	UOverlaySlot* GrommetSlot = GrommetBox ? Cast<UOverlaySlot>(GrommetBox->Slot) : nullptr;
	TestNotNull("tag builds its first grommet size box", GrommetBox);
	TestTrue("tag grommet is vertically centered",
		GrommetSlot && GrommetSlot->GetVerticalAlignment() == VAlign_Center);
	TestTrue("Diorama tag grommet Build center is 25 from the overlay edge",
		GrommetSlot && GrommetSlot->GetPadding().Left == 14.f && GrommetSlot->GetPadding().Top == 0.f);

	const FMordecaiUIPrimitiveSpec HighContrastTag = FMordecaiUIPrimitiveBuilder::Tag(
		FMordecaiUIThemeColors::MakeHighContrast(), EMordecaiUIPrintDepth::HUD,
		EMordecaiUITheme::HighContrast);
	UWidgetTree* HighContrastTagTree = NewObject<UWidgetTree>();
	UOverlay* BuiltHighContrastTag = FMordecaiUIPrimitiveBuilder::Build(
		HighContrastTagTree, HighContrastTag, TEXT("HighContrastTag"));
	USizeBox* HighContrastGrommetBox =
		BuiltHighContrastTag && BuiltHighContrastTag->GetChildrenCount() > 3
		? Cast<USizeBox>(BuiltHighContrastTag->GetChildAt(3))
		: nullptr;
	UOverlaySlot* HighContrastGrommetSlot =
		HighContrastGrommetBox ? Cast<UOverlaySlot>(HighContrastGrommetBox->Slot) : nullptr;
	TestTrue("High Contrast tag grommet Build center is 26 from the overlay edge",
		HighContrastGrommetSlot
		&& HighContrastGrommetSlot->GetPadding().Left == 15.f
		&& HighContrastGrommetSlot->GetPadding().Top == 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_Primitives_ApplyRetintsInPlace,
	"Mordecai.UI.Primitives.ApplyRetintsInPlace",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_Primitives_ApplyRetintsInPlace::RunTest(const FString& Parameters)
{
	using namespace MordecaiTinTallyThemeTest;
	UWidgetTree* Tree = NewObject<UWidgetTree>();
	const FMordecaiUIPrimitiveSpec DioramaPlate = FMordecaiUIPrimitiveBuilder::Plate(
		FMordecaiUIThemeColors::MakeDiorama(), EMordecaiUIPrintDepth::HUD, EMordecaiUITheme::Diorama);
	UOverlay* Built = FMordecaiUIPrimitiveBuilder::Build(Tree, DioramaPlate, TEXT("ApplyPlate"));
	if (!TestNotNull("Diorama plate builds before Apply", Built))
	{
		return true;
	}

	TMap<FName, UBorder*> OriginalBorders;
	for (const FMordecaiUIPrimitiveLayer& Layer : DioramaPlate.Layers)
	{
		OriginalBorders.Add(Layer.Name, FindBuiltBorder(Built, Layer.Name));
		TestNotNull(*FString::Printf(TEXT("%s exists before Apply"), *Layer.Name.ToString()),
			OriginalBorders[Layer.Name]);
	}
	const int32 OriginalChildCount = Built->GetChildrenCount();

	FMordecaiUIPrimitiveSpec HighContrastPlate = FMordecaiUIPrimitiveBuilder::Plate(
		FMordecaiUIThemeColors::MakeHighContrast(), EMordecaiUIPrintDepth::HUD,
		EMordecaiUITheme::HighContrast);
	HighContrastPlate.Layers.Swap(0, 1);
	FMordecaiUIPrimitiveBuilder::Apply(Built, HighContrastPlate);

	TestEqual("Apply does not reconstruct or remove layers", Built->GetChildrenCount(), OriginalChildCount);
	for (const TPair<FName, UBorder*>& Original : OriginalBorders)
	{
		TestTrue(*FString::Printf(TEXT("%s keeps the same UBorder pointer"), *Original.Key.ToString()),
			FindBuiltBorder(Built, Original.Key) == Original.Value);
	}

	UBorder* AppliedOutline = FindBuiltBorder(Built, TEXT("Outline"));
	TestTrue("the same outline carries the High Contrast ink tint",
		AppliedOutline
		&& BrushTint(AppliedOutline->Background).Equals(FMordecaiUIThemeColors::MakeHighContrast().Ink));
	TestTrue("the same outline carries the High Contrast radius 16",
		AppliedOutline && IsRadius(AppliedOutline->Background, FVector4(16.f)));

	UBorder* AppliedPrint = FindBuiltBorder(Built, TEXT("Print"));
	TestTrue("Apply matches Print by name and preserves its HUD render translation",
		AppliedPrint && AppliedPrint->GetRenderTransform().Translation == FVector2D(0.f, 4.f));

	UBorder* AppliedRimShade = FindBuiltBorder(Built, TEXT("RimShade"));
	UOverlaySlot* AppliedRimShadeSlot =
		AppliedRimShade ? Cast<UOverlaySlot>(AppliedRimShade->Slot) : nullptr;
	TestTrue("the same rim-shade carries the High Contrast tint",
		AppliedRimShade
		&& BrushTint(AppliedRimShade->Background).Equals(
			FMordecaiUIThemeColors::MakeHighContrast().RimShade));
	TestTrue("the same rim-shade slot carries the High Contrast 4px padding",
		AppliedRimShadeSlot && AppliedRimShadeSlot->GetPadding() == FMargin(4.f));

	UWidgetTree* TagTree = NewObject<UWidgetTree>();
	const FMordecaiUIPrimitiveSpec DioramaTag = FMordecaiUIPrimitiveBuilder::Tag(
		FMordecaiUIThemeColors::MakeDiorama(), EMordecaiUIPrintDepth::HUD, EMordecaiUITheme::Diorama);
	UOverlay* BuiltTag = FMordecaiUIPrimitiveBuilder::Build(TagTree, DioramaTag, TEXT("ApplyTag"));
	UBorder* OriginalGrommet = FindBuiltBorder(BuiltTag, TEXT("GrommetInk"));
	USizeBox* OriginalGrommetBox =
		OriginalGrommet ? Cast<USizeBox>(OriginalGrommet->GetParent()) : nullptr;
	UOverlaySlot* OriginalGrommetSlot =
		OriginalGrommetBox ? Cast<UOverlaySlot>(OriginalGrommetBox->Slot) : nullptr;
	TestTrue("Diorama grommet SizeBox starts at Left 14",
		OriginalGrommetSlot && OriginalGrommetSlot->GetPadding().Left == 14.f);

	const FMordecaiUIPrimitiveSpec HighContrastTag = FMordecaiUIPrimitiveBuilder::Tag(
		FMordecaiUIThemeColors::MakeHighContrast(), EMordecaiUIPrintDepth::HUD,
		EMordecaiUITheme::HighContrast);
	FMordecaiUIPrimitiveBuilder::Apply(BuiltTag, HighContrastTag);
	UBorder* AppliedGrommet = FindBuiltBorder(BuiltTag, TEXT("GrommetInk"));
	USizeBox* AppliedGrommetBox =
		AppliedGrommet ? Cast<USizeBox>(AppliedGrommet->GetParent()) : nullptr;
	UOverlaySlot* AppliedGrommetSlot =
		AppliedGrommetBox ? Cast<UOverlaySlot>(AppliedGrommetBox->Slot) : nullptr;
	TestTrue("Apply preserves the grommet SizeBox and matches it by layer name",
		AppliedGrommet == OriginalGrommet && AppliedGrommetBox == OriginalGrommetBox);
	TestTrue("High Contrast Apply moves the grommet SizeBox slot Left from 14 to 15",
		AppliedGrommetSlot && AppliedGrommetSlot->GetPadding().Left == 15.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_TallyWidget_CountAndClamp,
	"Mordecai.UI.TallyWidget.CountAndClamp",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_TallyWidget_CountAndClamp::RunTest(const FString& Parameters)
{
	UMordecaiTallyWidget* Tally = NewObject<UMordecaiTallyWidget>();
	TestEqual("the atlas contract has frames 0 through 5", Tally->GetBundleFrames().Num(), 6);
	for (int32 Index = 0; Index < Tally->GetBundleFrames().Num(); ++Index)
	{
		TestEqual(*FString::Printf(TEXT("default frame %d draws nothing until US-103"), Index),
			Tally->GetBundleFrames()[Index].DrawAs, ESlateBrushDrawType::NoDrawType);
	}
	TestEqual("default use is a D1-allowed skill rank", Tally->GetUsage(), EMordecaiTallyUsage::SkillRank);
	TestTrue("the default usage is on D1's allow-list",
		UMordecaiTallyWidget::IsAllowedUsage(Tally->GetUsage()));
	TestFalse("an invalid usage is rejected",
		UMordecaiTallyWidget::IsAllowedUsage(static_cast<EMordecaiTallyUsage>(255)));

	const FIntProperty* MaxProperty = FindFProperty<FIntProperty>(Tally->GetClass(), TEXT("Max"));
	TestNotNull("Max is reflected", MaxProperty);
#if WITH_EDITOR
	TestEqual("Max documents the supported rank ceiling", MaxProperty ? MaxProperty->GetMetaData(TEXT("ClampMax")) : FString(),
		FString(TEXT("20")));
#endif
	const FStructProperty* CapDotProperty = FindFProperty<FStructProperty>(Tally->GetClass(), TEXT("CapDotBrush"));
	TestNotNull("the optional D1 rank-one cap-dot brush is exposed", CapDotProperty);
	if (CapDotProperty)
	{
		const FSlateBrush* CapDot = CapDotProperty->ContainerPtrToValuePtr<FSlateBrush>(Tally);
		TestEqual("cap dot draws nothing until US-103", CapDot->DrawAs, ESlateBrushDrawType::NoDrawType);
	}

	TArray<FSlateBrush>& Frames = const_cast<TArray<FSlateBrush>&>(Tally->GetBundleFrames());
	for (int32 Index = 0; Index < Frames.Num(); ++Index)
	{
		Frames[Index].DrawAs = ESlateBrushDrawType::Box;
		Frames[Index].TintColor = FSlateColor(FLinearColor(Index / 10.f, 0.f, 0.f, 1.f));
	}
	TestTrue("the tally initializes its real widget tree", Tally->Initialize());
	UHorizontalBox* BundleBox = Tally->WidgetTree ? Cast<UHorizontalBox>(Tally->WidgetTree->RootWidget) : nullptr;
	TestNotNull("the rendered tally root is a horizontal box", BundleBox);

	Tally->SetMax(20);
	Tally->SetCount(0);
	TestEqual("zero remains zero", Tally->GetCount(), 0);
	TestEqual("zero renders four empty bundles", Tally->GetRenderedBundleFrames(), TArray<int32>({ 0, 0, 0, 0 }));
	TestEqual("zero materializes four images", BundleBox ? BundleBox->GetChildrenCount() : 0, 4);
	for (int32 Index = 0; Index < 4; ++Index)
	{
		const UImage* Image = MordecaiTinTallyThemeTest::GetBundleImage(Tally, Index);
		TestTrue("zero uses the distinguishable empty brush",
			Image && MordecaiTinTallyThemeTest::BrushTint(Image->GetBrush()).Equals(
				MordecaiTinTallyThemeTest::BrushTint(Frames[0])));
	}

	Tally->SetCount(12);
	TestEqual("twelve renders two bundles and two marks", Tally->GetRenderedBundleFrames(),
		TArray<int32>({ 5, 5, 2, 0 }));
	for (int32 Index = 0; Index < 4; ++Index)
	{
		const int32 ExpectedFrame[] = { 5, 5, 2, 0 };
		const UImage* Image = MordecaiTinTallyThemeTest::GetBundleImage(Tally, Index);
		TestTrue(*FString::Printf(TEXT("twelve image %d uses frame %d"), Index, ExpectedFrame[Index]),
			Image && MordecaiTinTallyThemeTest::BrushTint(Image->GetBrush()).Equals(
				MordecaiTinTallyThemeTest::BrushTint(Frames[ExpectedFrame[Index]])));
	}

	Tally->SetCount(20);
	TestEqual("Max renders four full bundles", Tally->GetRenderedBundleFrames(),
		TArray<int32>({ 5, 5, 5, 5 }));
	for (int32 Index = 0; Index < 4; ++Index)
	{
		const UImage* Image = MordecaiTinTallyThemeTest::GetBundleImage(Tally, Index);
		TestTrue("twenty uses the full-bundle brush",
			Image && MordecaiTinTallyThemeTest::BrushTint(Image->GetBrush()).Equals(
				MordecaiTinTallyThemeTest::BrushTint(Frames[5])));
	}

	Tally->SetCount(21);
	TestEqual("Count above Max clamps", Tally->GetCount(), 20);
	Tally->SetCount(-4);
	TestEqual("negative Count clamps", Tally->GetCount(), 0);
	Tally->SetMax(-1);
	TestEqual("negative Max clamps", Tally->GetMax(), 0);
	TestEqual("Count reclamps when Max changes", Tally->GetCount(), 0);
	TestEqual("Max zero still renders the empty frame", Tally->GetRenderedBundleFrames(), TArray<int32>({ 0 }));
	TestEqual("Max zero materializes one empty image", BundleBox ? BundleBox->GetChildrenCount() : 0, 1);

	Tally->SetCountAndMax(20, 20);
	Tally->SetMax(10);
	TestEqual("shrinking Max reclamps Count", Tally->GetCount(), 10);
	TestEqual("shrinking Max removes surplus images", BundleBox ? BundleBox->GetChildrenCount() : 0, 2);
	for (int32 Index = 0; Index < 2; ++Index)
	{
		const UImage* Image = MordecaiTinTallyThemeTest::GetBundleImage(Tally, Index);
		TestTrue("shrunk Max keeps the correct full brush",
			Image && MordecaiTinTallyThemeTest::BrushTint(Image->GetBrush()).Equals(
				MordecaiTinTallyThemeTest::BrushTint(Frames[5])));
	}

	FIntProperty* MutableMaxProperty = FindFProperty<FIntProperty>(Tally->GetClass(), TEXT("Max"));
	FIntProperty* CountProperty = FindFProperty<FIntProperty>(Tally->GetClass(), TEXT("Count"));
	MutableMaxProperty->SetPropertyValue_InContainer(Tally, 99);
	CountProperty->SetPropertyValue_InContainer(Tally, 88);
	Tally->SynchronizeProperties();
	TestEqual("SynchronizeProperties clamps Max to the documented ceiling", Tally->GetMax(), 20);
	TestEqual("SynchronizeProperties clamps Count to Max", Tally->GetCount(), 20);
	return true;
}
