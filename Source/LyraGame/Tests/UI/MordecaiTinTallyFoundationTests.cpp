// Project Mordecai — Tin Tally cooked-foundation contract tests (US-103)

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/DataAsset.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "Fonts/CompositeFont.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Misc/AutomationTest.h"
#include "Modules/ModuleManager.h"
#include "Mordecai/UI/MordecaiCombatFeedbackWidget.h"
#include "Mordecai/UI/MordecaiCombatHUDWidget.h"
#include "Mordecai/UI/MordecaiComboCounterWidget.h"
#include "Mordecai/UI/MordecaiEquippedWeaponWidget.h"
#include "Mordecai/UI/MordecaiHealthBarWidget.h"
#include "Mordecai/UI/MordecaiPickupPromptWidget.h"
#include "Mordecai/UI/MordecaiSpellCooldownWidget.h"
#include "Mordecai/UI/MordecaiStatusEffectBarWidget.h"
#include "Mordecai/UI/MordecaiStatusEffectIndicatorWidget.h"
#include "Mordecai/UI/TinTally/MordecaiTinTallyGallerySubsystem.h"
#include "Mordecai/UI/TinTally/MordecaiTallyWidget.h"
#include "Mordecai/UI/TinTally/MordecaiUIFoundation.h"
#include "Mordecai/UI/TinTally/MordecaiUIPrimitives.h"
#include "Mordecai/UI/TinTally/MordecaiUITheme.h"
#include "UObject/UnrealType.h"

namespace MordecaiTinTallyFoundationTest
{
	static const TCHAR* Root = TEXT("/MordecaiCore/UI/TinTally");

	template <typename T>
	T* Load(const TCHAR* Path)
	{
		return LoadObject<T>(nullptr, Path);
	}

	struct FTypeStyleExpectation
	{
		const TCHAR* AssetName;
		int32 PointSize;
		float LineHeight;
		const TCHAR* CompositeFont;
		const TCHAR* Typeface;
		const TCHAR* FaceAsset;
	};

	static const FTypeStyleExpectation TypeStyles[] = {
		{ TEXT("tally_display"), 60, 84.f / 80.f, TEXT("F_Tally_Display"), TEXT("Bold"), TEXT("FF_Rowdies_Bold") },
		{ TEXT("tally_title"), 36, 54.f / 48.f, TEXT("F_Tally_Display"), TEXT("Bold"), TEXT("FF_Rowdies_Bold") },
		{ TEXT("tally_heading"), 24, 38.f / 32.f, TEXT("F_Tally_Display"), TEXT("Bold"), TEXT("FF_Rowdies_Bold") },
		{ TEXT("callout"), 30, 44.f / 40.f, TEXT("F_Tally_Display"), TEXT("Bold"), TEXT("FF_Rowdies_Bold") },
		{ TEXT("kicker"), 15, 24.f / 20.f, TEXT("F_Tally_Display"), TEXT("Regular"), TEXT("FF_Rowdies_Regular") },
		{ TEXT("pop"), 23, 1.f, TEXT("F_Tally_Display"), TEXT("Bold"), TEXT("FF_Rowdies_Bold") },
		{ TEXT("world_title"), 54, 80.f / 72.f, TEXT("F_Tally_UI"), TEXT("ExtraBold"), TEXT("FF_AtkinsonNext_ExtraBold") },
		{ TEXT("world_heading"), 30, 48.f / 40.f, TEXT("F_Tally_UI"), TEXT("ExtraBold"), TEXT("FF_AtkinsonNext_ExtraBold") },
		{ TEXT("ui_title"), 20, 32.f / 26.f, TEXT("F_Tally_UI"), TEXT("Bold"), TEXT("FF_AtkinsonNext_Bold") },
		{ TEXT("ui_body"), 17, 30.f / 22.f, TEXT("F_Tally_UI"), TEXT("Medium"), TEXT("FF_AtkinsonNext_Medium") },
		{ TEXT("ui_label"), 15, 26.f / 20.f, TEXT("F_Tally_UI"), TEXT("SemiBold"), TEXT("FF_AtkinsonNext_SemiBold") },
		{ TEXT("num_xl"), 42, 60.f / 56.f, TEXT("F_Tally_Mono"), TEXT("ExtraBold"), TEXT("FF_AtkinsonMono_ExtraBold") },
		{ TEXT("num_l"), 23, 34.f / 30.f, TEXT("F_Tally_Mono"), TEXT("Bold"), TEXT("FF_AtkinsonMono_Bold") },
		{ TEXT("num_m"), 18, 28.f / 24.f, TEXT("F_Tally_Mono"), TEXT("Bold"), TEXT("FF_AtkinsonMono_Bold") },
		{ TEXT("num_s"), 15, 24.f / 20.f, TEXT("F_Tally_Mono"), TEXT("SemiBold"), TEXT("FF_AtkinsonMono_SemiBold") },
	};

	UTextBlock* FindTextBlock(UWidgetTree* Tree, const FString& Expected)
	{
		if (!Tree)
		{
			return nullptr;
		}
		TArray<UWidget*> Widgets;
		Tree->GetAllWidgets(Widgets);
		for (UWidget* Widget : Widgets)
		{
			if (UTextBlock* Text = Cast<UTextBlock>(Widget);
				Text && Text->GetText().ToString().Equals(Expected, ESearchCase::IgnoreCase))
			{
				return Text;
			}
		}
		return nullptr;
	}

	const FMordecaiUIPrimitiveLayer* FindLayer(const FMordecaiUIPrimitiveSpec& Spec, FName Name)
	{
		return Spec.Layers.FindByPredicate([Name](const FMordecaiUIPrimitiveLayer& Layer)
		{
			return Layer.Name == Name;
		});
	}

	bool SpecsEqual(const FMordecaiUIPrimitiveSpec& Actual, const FMordecaiUIPrimitiveSpec& Expected)
	{
		if (Actual.Layers.Num() != Expected.Layers.Num()
			|| !FMath::IsNearlyEqual(Actual.OuterOutlineWidth, Expected.OuterOutlineWidth)
			|| !FMath::IsNearlyEqual(Actual.InnerStrokeWidth, Expected.InnerStrokeWidth)
			|| !FMath::IsNearlyEqual(Actual.FocusStrokeWidth, Expected.FocusStrokeWidth)
			|| !FMath::IsNearlyEqual(Actual.FocusKeylineWidth, Expected.FocusKeylineWidth)
			|| !FMath::IsNearlyEqual(Actual.FocusOffset, Expected.FocusOffset))
		{
			return false;
		}
		for (int32 Index = 0; Index < Actual.Layers.Num(); ++Index)
		{
			const FMordecaiUIPrimitiveLayer& A = Actual.Layers[Index];
			const FMordecaiUIPrimitiveLayer& E = Expected.Layers[Index];
			if (A.Name != E.Name
				|| A.Padding != E.Padding
				|| A.Translation != E.Translation
				|| A.DesiredSize != E.DesiredSize
				|| A.Center != E.Center
				|| A.HorizontalAlignment != E.HorizontalAlignment
				|| A.VerticalAlignment != E.VerticalAlignment
				|| A.Brush.DrawAs != E.Brush.DrawAs
				|| !A.Brush.TintColor.GetSpecifiedColor().Equals(
					E.Brush.TintColor.GetSpecifiedColor(), 0.0001f)
				|| !A.Brush.OutlineSettings.CornerRadii.Equals(
					E.Brush.OutlineSettings.CornerRadii, 0.0001f)
				|| !A.Brush.OutlineSettings.Color.GetSpecifiedColor().Equals(
					E.Brush.OutlineSettings.Color.GetSpecifiedColor(), 0.0001f)
				|| !FMath::IsNearlyEqual(
					A.Brush.OutlineSettings.Width, E.Brush.OutlineSettings.Width)
				|| A.Brush.OutlineSettings.RoundingType
					!= E.Brush.OutlineSettings.RoundingType)
			{
				return false;
			}
		}
		return true;
	}

	USizeBox* FindLayerSizeBox(UOverlay* Overlay, FName LayerName)
	{
		if (!Overlay)
		{
			return nullptr;
		}
		const FString Suffix = FString::Printf(TEXT("_%s"), *LayerName.ToString());
		for (int32 Index = 0; Index < Overlay->GetChildrenCount(); ++Index)
		{
			USizeBox* SizeBox = Cast<USizeBox>(Overlay->GetChildAt(Index));
			const UWidget* Child = SizeBox && SizeBox->GetChildrenCount() > 0
				? SizeBox->GetChildAt(0)
				: nullptr;
			if (Child && Child->GetName().EndsWith(Suffix))
			{
				return SizeBox;
			}
		}
		return nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_Foundation_AssetPathsAndTypes,
	"Mordecai.UI.Foundation.AssetPathsAndTypes",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_Foundation_AssetPathsAndTypes::RunTest(const FString& Parameters)
{
	using namespace MordecaiTinTallyFoundationTest;

	UMordecaiUIThemeDataAsset* Diorama = Load<UMordecaiUIThemeDataAsset>(
		TEXT("/MordecaiCore/UI/TinTally/Themes/DA_UITheme_Diorama.DA_UITheme_Diorama"));
	UMordecaiUIThemeDataAsset* HighContrast = Load<UMordecaiUIThemeDataAsset>(
		TEXT("/MordecaiCore/UI/TinTally/Themes/DA_UITheme_HighContrast.DA_UITheme_HighContrast"));
	TestNotNull("the cooked Diorama theme asset loads", Diorama);
	TestNotNull("the cooked High Contrast theme asset loads", HighContrast);
	if (Diorama)
	{
		TestEqual("Diorama asset is labelled Diorama", Diorama->Theme, EMordecaiUITheme::Diorama);
	}
	if (HighContrast)
	{
		TestEqual("High Contrast asset is labelled High Contrast",
			HighContrast->Theme, EMordecaiUITheme::HighContrast);
	}
	if (Diorama && HighContrast)
	{
		const FMordecaiUIThemeColors NativeDiorama = FMordecaiUIThemeColors::MakeDiorama();
		const FMordecaiUIThemeColors NativeHighContrast =
			FMordecaiUIThemeColors::MakeHighContrast();
		for (FName Token : FMordecaiUIThemeColors::GetRuntimeColorTokenNames())
		{
			FLinearColor SavedDiorama;
			FLinearColor SavedHighContrast;
			FLinearColor ExpectedDiorama;
			FLinearColor ExpectedHighContrast;
			const bool bResolved = Diorama->Colors.TryGetColorToken(Token, SavedDiorama)
				&& HighContrast->Colors.TryGetColorToken(Token, SavedHighContrast)
				&& NativeDiorama.TryGetColorToken(Token, ExpectedDiorama)
				&& NativeHighContrast.TryGetColorToken(Token, ExpectedHighContrast);
			TestTrue(*FString::Printf(TEXT("%s resolves in both saved and native themes"),
				*Token.ToString()), bResolved);
			TestTrue(*FString::Printf(TEXT("%s saved Diorama equals native"), *Token.ToString()),
				bResolved && SavedDiorama.Equals(ExpectedDiorama, 0.0001f));
			TestTrue(*FString::Printf(TEXT("%s saved High Contrast equals native"), *Token.ToString()),
				bResolved && SavedHighContrast.Equals(ExpectedHighContrast, 0.0001f));
		}
	}

	for (const TCHAR* FontName : { TEXT("F_Tally_Display"), TEXT("F_Tally_UI"), TEXT("F_Tally_Mono") })
	{
		const FString Path = FString::Printf(
			TEXT("/MordecaiCore/UI/TinTally/Fonts/%s.%s"), FontName, FontName);
		UFont* Font = Load<UFont>(*Path);
		TestNotNull(*FString::Printf(TEXT("%s composite font loads"), FontName), Font);
		if (Font)
		{
			TestEqual(*FString::Printf(TEXT("%s uses the runtime font cache"), FontName),
				Font->FontCacheType, EFontCacheType::Runtime);
		}
	}

	for (const TCHAR* FaceName : {
		TEXT("FF_Rowdies_Regular"), TEXT("FF_Rowdies_Bold"),
		TEXT("FF_AtkinsonNext_Medium"), TEXT("FF_AtkinsonNext_SemiBold"),
		TEXT("FF_AtkinsonNext_Bold"), TEXT("FF_AtkinsonNext_ExtraBold"),
		TEXT("FF_AtkinsonMono_SemiBold"), TEXT("FF_AtkinsonMono_Bold"),
		TEXT("FF_AtkinsonMono_ExtraBold") })
	{
		const FString Path = FString::Printf(
			TEXT("/MordecaiCore/UI/TinTally/Fonts/Faces/%s.%s"), FaceName, FaceName);
		TestNotNull(*FString::Printf(TEXT("%s static TTF face loads"), FaceName),
			Load<UFontFace>(*Path));
	}

	for (const TCHAR* StyleName : {
		TEXT("ST_Tally_Plate"), TEXT("ST_Tally_Pill"), TEXT("ST_Tally_Tag"),
		TEXT("ST_Tally_Well"), TEXT("ST_Tally_Keycap"), TEXT("ST_Tally_Focus") })
	{
		const FString Path = FString::Printf(
			TEXT("/MordecaiCore/UI/TinTally/Styles/%s.%s"), StyleName, StyleName);
		TestNotNull(*FString::Printf(TEXT("%s rounded style asset loads"), StyleName),
			Load<UDataAsset>(*Path));
	}

	UMaterial* Pattern = Load<UMaterial>(
		TEXT("/MordecaiCore/UI/TinTally/Materials/M_UI_PatternFill.M_UI_PatternFill"));
	UTexture2D* Hatch = Load<UTexture2D>(
		TEXT("/MordecaiCore/UI/TinTally/Textures/T_UI_Hatch16.T_UI_Hatch16"));
	TestNotNull("the shared pattern material loads", Pattern);
	TestNotNull("the 16px hatch texture loads", Hatch);
	if (Pattern)
	{
		TestEqual("pattern material is a UI material", Pattern->MaterialDomain, MD_UI);
		TArray<FMaterialParameterInfo> Scalars;
		TArray<FMaterialParameterInfo> Vectors;
		TArray<FGuid> ScalarIds;
		TArray<FGuid> VectorIds;
		Pattern->GetAllScalarParameterInfo(Scalars, ScalarIds);
		Pattern->GetAllVectorParameterInfo(Vectors, VectorIds);
		const TSet<FName> ScalarNames = {
			TEXT("Period"), TEXT("Angle"), TEXT("LineWidth")
		};
		const TSet<FName> VectorNames = { TEXT("Base"), TEXT("Line") };
		for (FName Name : ScalarNames)
		{
			TestTrue(*FString::Printf(TEXT("pattern exposes scalar %s"), *Name.ToString()),
				Scalars.ContainsByPredicate([Name](const FMaterialParameterInfo& Info)
				{
					return Info.Name == Name;
				}));
		}
		for (FName Name : VectorNames)
		{
			TestTrue(*FString::Printf(TEXT("pattern exposes vector %s"), *Name.ToString()),
				Vectors.ContainsByPredicate([Name](const FMaterialParameterInfo& Info)
				{
					return Info.Name == Name;
				}));
		}
		bool bSamplesHatch = false;
		for (const TObjectPtr<UObject>& ReferencedTexture : Pattern->GetReferencedTextures())
		{
			bSamplesHatch |= ReferencedTexture
				&& ReferencedTexture->GetName() == TEXT("T_UI_Hatch16");
		}
		TestTrue("pattern graph samples T_UI_Hatch16", bSamplesHatch);
	}

	TestNotNull("the tally atlas loads", Load<UTexture2D>(
		TEXT("/MordecaiCore/UI/TinTally/Textures/T_UI_TallyAtlas.T_UI_TallyAtlas")));
	for (const TCHAR* BadgeName : {
		TEXT("T_UI_Badge_FeatCommon"), TEXT("T_UI_Badge_FeatRare"),
		TEXT("T_UI_Badge_FeatLegendary"), TEXT("T_UI_Badge_Technique"),
		TEXT("T_UI_Badge_KeyTag") })
	{
		const FString Path = FString::Printf(
			TEXT("/MordecaiCore/UI/TinTally/Textures/Badges/%s.%s"), BadgeName, BadgeName);
		TestNotNull(*FString::Printf(TEXT("%s silhouette loads"), BadgeName), Load<UTexture2D>(*Path));
	}

	UDataAsset* Manifest = Load<UDataAsset>(
		TEXT("/MordecaiCore/UI/TinTally/DA_TinTallyFoundation.DA_TinTallyFoundation"));
	TestNotNull("the cooked foundation manifest loads", Manifest);
	if (Manifest)
	{
		const FArrayProperty* Notices = FindFProperty<FArrayProperty>(
			Manifest->GetClass(), TEXT("FontLicenseNotices"));
		TestNotNull("the foundation manifest retains font license notices", Notices);
		if (Notices)
		{
			FScriptArrayHelper Helper(Notices, Notices->ContainerPtrToValuePtr<void>(Manifest));
			TestEqual("the three OFL notices are retained for Credits", Helper.Num(), 3);
		}
	}

	FAssetRegistryModule& RegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> FoundationAssets;
	RegistryModule.Get().GetAssetsByPath(FName(Root), FoundationAssets, true);
	for (const FAssetData& AssetData : FoundationAssets)
	{
		if (const UTexture2D* Texture = Cast<UTexture2D>(AssetData.GetAsset()))
		{
			if (Texture->GetName() == TEXT("T_UI_Hatch16"))
			{
				TestEqual("the native 16px hatch uses nearest filtering",
					Texture->Filter, TEnumAsByte<TextureFilter>(TF_Nearest));
#if WITH_EDITORONLY_DATA
				TestEqual("the native 16px hatch has no generated mip chain",
					Texture->MipGenSettings,
					TEnumAsByte<TextureMipGenSettings>(TMGS_NoMipmaps));
#endif
				TestEqual("the native 16px hatch wraps horizontally",
					Texture->AddressX, TEnumAsByte<TextureAddress>(TA_Wrap));
				TestEqual("the native 16px hatch wraps vertically",
					Texture->AddressY, TEnumAsByte<TextureAddress>(TA_Wrap));
			}
			else
			{
				TestTrue(*FString::Printf(TEXT("%s keeps bilinear filtering"),
					*Texture->GetName()), Texture->Filter != TF_Nearest);
			}
		}
	}

	TestNotNull("the non-Shipping gallery subsystem is compiled",
		FindObject<UClass>(nullptr, TEXT("/Script/LyraGame.MordecaiTinTallyGallerySubsystem")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_Foundation_PrimitiveStyleAssetsMatchBuilders,
	"Mordecai.UI.Foundation.PrimitiveStyleAssetsMatchBuilders",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_Foundation_PrimitiveStyleAssetsMatchBuilders::RunTest(const FString& Parameters)
{
	using namespace MordecaiTinTallyFoundationTest;
	const FMordecaiUIThemeColors Diorama = FMordecaiUIThemeColors::MakeDiorama();
	const FMordecaiUIThemeColors HighContrast = FMordecaiUIThemeColors::MakeHighContrast();
	struct FStyleExpectation
	{
		const TCHAR* Name;
		FMordecaiUIPrimitiveSpec Diorama;
		FMordecaiUIPrimitiveSpec HighContrast;
	};
	const FStyleExpectation Expectations[] = {
		{ TEXT("Plate"),
			FMordecaiUIPrimitiveBuilder::Plate(Diorama, EMordecaiUIPrintDepth::HUD, EMordecaiUITheme::Diorama),
			FMordecaiUIPrimitiveBuilder::Plate(HighContrast, EMordecaiUIPrintDepth::HUD, EMordecaiUITheme::HighContrast) },
		{ TEXT("Pill"),
			FMordecaiUIPrimitiveBuilder::Pill(Diorama, EMordecaiUIPrintDepth::HUD, EMordecaiUITheme::Diorama),
			FMordecaiUIPrimitiveBuilder::Pill(HighContrast, EMordecaiUIPrintDepth::HUD, EMordecaiUITheme::HighContrast) },
		{ TEXT("Tag"),
			FMordecaiUIPrimitiveBuilder::Tag(Diorama, EMordecaiUIPrintDepth::HUD, EMordecaiUITheme::Diorama),
			FMordecaiUIPrimitiveBuilder::Tag(HighContrast, EMordecaiUIPrintDepth::HUD, EMordecaiUITheme::HighContrast) },
		{ TEXT("Well"),
			FMordecaiUIPrimitiveBuilder::Well(Diorama, false),
			FMordecaiUIPrimitiveBuilder::Well(HighContrast, false) },
		{ TEXT("Keycap"),
			FMordecaiUIPrimitiveBuilder::Keycap(Diorama, EMordecaiKeycapShape::Square),
			FMordecaiUIPrimitiveBuilder::Keycap(HighContrast, EMordecaiKeycapShape::Square) },
		{ TEXT("Focus"),
			FMordecaiUIPrimitiveBuilder::Focus(Diorama, 12.f, EMordecaiUITheme::Diorama),
			FMordecaiUIPrimitiveBuilder::Focus(HighContrast, 12.f, EMordecaiUITheme::HighContrast) },
	};
	for (const FStyleExpectation& Expected : Expectations)
	{
		const FString Path = FString::Printf(
			TEXT("/MordecaiCore/UI/TinTally/Styles/ST_Tally_%s.ST_Tally_%s"),
			Expected.Name, Expected.Name);
		const UMordecaiUIPrimitiveStyleDataAsset* Style =
			Load<UMordecaiUIPrimitiveStyleDataAsset>(*Path);
		TestNotNull(*FString::Printf(TEXT("ST_Tally_%s loads"), Expected.Name), Style);
		TestTrue(*FString::Printf(TEXT("ST_Tally_%s Diorama equals builder output"), Expected.Name),
			Style && SpecsEqual(Style->Diorama, Expected.Diorama));
		TestTrue(*FString::Printf(TEXT("ST_Tally_%s High Contrast equals builder output"), Expected.Name),
			Style && SpecsEqual(Style->HighContrast, Expected.HighContrast));
	}
	const UMordecaiUIPrimitiveStyleDataAsset* FocusStyle =
		Load<UMordecaiUIPrimitiveStyleDataAsset>(
			TEXT("/MordecaiCore/UI/TinTally/Styles/ST_Tally_Focus.ST_Tally_Focus"));
	const FMordecaiUIPrimitiveLayer* OuterInk =
		FocusStyle ? FindLayer(FocusStyle->Diorama, TEXT("Keyline")) : nullptr;
	const FMordecaiUIPrimitiveLayer* Focus =
		FocusStyle ? FindLayer(FocusStyle->Diorama, TEXT("Focus")) : nullptr;
	TestTrue("saved focus has an outer ink keyline",
		OuterInk && OuterInk->Brush.OutlineSettings.Color.GetSpecifiedColor().Equals(Diorama.Ink));
	TestTrue("saved focus has the focus-token middle ring",
		Focus && Focus->Brush.OutlineSettings.Color.GetSpecifiedColor().Equals(Diorama.Focus));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_Foundation_FontStylesMatchTokens,
	"Mordecai.UI.Foundation.FontStylesMatchTokens",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_Foundation_FontStylesMatchTokens::RunTest(const FString& Parameters)
{
	using namespace MordecaiTinTallyFoundationTest;

	TestEqual("tokens.json defines fifteen text styles", static_cast<int32>(UE_ARRAY_COUNT(TypeStyles)), 15);
	for (const FTypeStyleExpectation& Expected : TypeStyles)
	{
		const FString ClassPath = FString::Printf(
			TEXT("/MordecaiCore/UI/TinTally/Fonts/Styles/TS_Tally_%s.TS_Tally_%s_C"),
			Expected.AssetName, Expected.AssetName);
		UClass* StyleClass = LoadClass<UCommonTextStyle>(nullptr, *ClassPath);
		TestNotNull(*FString::Printf(TEXT("TS_Tally_%s loads in runtime"), Expected.AssetName), StyleClass);
		const UCommonTextStyle* Style = StyleClass ? GetDefault<UCommonTextStyle>(StyleClass) : nullptr;
		if (!Style)
		{
			continue;
		}

		FSlateFontInfo Font;
		Style->GetFont(Font);
		TestEqual(*FString::Printf(TEXT("%s point size is px x 0.75"), Expected.AssetName),
			static_cast<int32>(Font.Size), Expected.PointSize);
		TestEqual(*FString::Printf(TEXT("%s line height matches tokens"), Expected.AssetName),
			Style->GetLineHeightPercentage(), Expected.LineHeight, 0.001f);
		const UObject* FontObject = Font.FontObject;
		TestTrue(*FString::Printf(TEXT("%s uses %s"), Expected.AssetName, Expected.CompositeFont),
			FontObject && FontObject->GetName() == Expected.CompositeFont);
		TestEqual(*FString::Printf(TEXT("%s requests the token weight face"), Expected.AssetName),
			Font.TypefaceFontName, FName(Expected.Typeface));

		const UFont* CompositeFont = Cast<UFont>(Font.FontObject);
		const FTypefaceEntry* TypefaceEntry = CompositeFont
			? CompositeFont->CompositeFont.DefaultTypeface.Fonts.FindByPredicate(
				[&Expected](const FTypefaceEntry& Entry)
				{
					return Entry.Name == Expected.Typeface;
				})
			: nullptr;
		TestNotNull(*FString::Printf(TEXT("%s composite contains %s"), Expected.AssetName, Expected.Typeface),
			TypefaceEntry);
		const UObject* FaceAsset = TypefaceEntry ? TypefaceEntry->Font.GetFontFaceAsset() : nullptr;
		TestTrue(*FString::Printf(TEXT("%s resolves to %s"), Expected.AssetName, Expected.FaceAsset),
			FaceAsset && FaceAsset->GetName() == Expected.FaceAsset);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_Foundation_GalleryTextUsesTokenStylesAndCase,
	"Mordecai.UI.Foundation.GalleryTextUsesTokenStylesAndCase",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_Foundation_GalleryTextUsesTokenStylesAndCase::RunTest(const FString& Parameters)
{
	using namespace MordecaiTinTallyFoundationTest;

	struct FGalleryTextExpectation
	{
		const TCHAR* Component;
		const TCHAR* Text;
		const TCHAR* FontObject;
		const TCHAR* Typeface;
		int32 PointSize;
	};
	const FGalleryTextExpectation Expectations[] = {
		{ TEXT("Plate"), TEXT("ANATOMY AT 4X · ONE PLATE ACROSS A LIGHT AND A DARK FLOOR"), TEXT("F_Tally_Display"), TEXT("Regular"), 15 },
		{ TEXT("Plate"), TEXT("Stored Resources"), TEXT("F_Tally_UI"), TEXT("Bold"), 20 },
		{ TEXT("Plate"), TEXT("Iron Ore"), TEXT("F_Tally_UI"), TEXT("Medium"), 17 },
		{ TEXT("Plate"), TEXT("Town Stone"), TEXT("F_Tally_UI"), TEXT("Medium"), 17 },
		{ TEXT("Plate"), TEXT("Fireball"), TEXT("F_Tally_UI"), TEXT("SemiBold"), 15 },
		{ TEXT("Plate"), TEXT("Blink"), TEXT("F_Tally_UI"), TEXT("SemiBold"), 15 },
		{ TEXT("Plate"), TEXT("Stone Skin"), TEXT("F_Tally_UI"), TEXT("SemiBold"), 15 },
		{ TEXT("Tag"), TEXT("Upgrade Key"), TEXT("F_Tally_UI"), TEXT("Bold"), 20 },
		{ TEXT("Tag"), TEXT("Health Potion"), TEXT("F_Tally_UI"), TEXT("Medium"), 17 },
		{ TEXT("Tag"), TEXT("Exhausted: every action at ×0.5"), TEXT("F_Tally_UI"), TEXT("SemiBold"), 15 },
		{ TEXT("Keycap"), TEXT("Keyboard and mouse · square caps"), TEXT("F_Tally_UI"), TEXT("Bold"), 20 },
		{ TEXT("Keycap"), TEXT("Face buttons"), TEXT("F_Tally_UI"), TEXT("SemiBold"), 15 },
		{ TEXT("Keycap"), TEXT("Shoulders"), TEXT("F_Tally_UI"), TEXT("SemiBold"), 15 },
		{ TEXT("Keycap"), TEXT("Triggers"), TEXT("F_Tally_UI"), TEXT("SemiBold"), 15 },
		{ TEXT("Keycap"), TEXT("Stick clicks"), TEXT("F_Tally_UI"), TEXT("SemiBold"), 15 },
		{ TEXT("Tally"), TEXT("Skill ranks"), TEXT("F_Tally_UI"), TEXT("Bold"), 20 },
		{ TEXT("Tally"), TEXT("Seal, throbber"), TEXT("F_Tally_UI"), TEXT("Bold"), 20 },
		{ TEXT("Tally"), TEXT("0 / 20"), TEXT("F_Tally_Mono"), TEXT("Bold"), 18 },
		{ TEXT("Tally"), TEXT("12 / 20"), TEXT("F_Tally_Mono"), TEXT("Bold"), 18 },
		{ TEXT("Tally"), TEXT("Longswords"), TEXT("F_Tally_UI"), TEXT("Bold"), 20 },
	};

	TMap<FString, UMordecaiTinTallyGalleryWidget*> Galleries;
	for (const FGalleryTextExpectation& Expected : Expectations)
	{
		UMordecaiTinTallyGalleryWidget*& Gallery = Galleries.FindOrAdd(Expected.Component);
		if (!Gallery)
		{
			Gallery = NewObject<UMordecaiTinTallyGalleryWidget>();
			Gallery->SetFixtureSelectionForTest(Expected.Component, TEXT("diorama"));
			Gallery->Initialize();
		}

		UTextBlock* Text = FindTextBlock(Gallery->WidgetTree, Expected.Text);
		TestNotNull(*FString::Printf(TEXT("%s fixture contains '%s'"), Expected.Component, Expected.Text), Text);
		if (!Text)
		{
			continue;
		}
		TestEqual(*FString::Printf(TEXT("%s preserves preview case"), Expected.Text),
			Text->GetText().ToString(), FString(Expected.Text));
		TestFalse(*FString::Printf(TEXT("%s is a single non-wrapping line"), Expected.Text),
			Text->GetAutoWrapText());
		const FSlateFontInfo Font = Text->GetFont();
		TestTrue(*FString::Printf(TEXT("%s uses %s"), Expected.Text, Expected.FontObject),
			Font.FontObject && Font.FontObject->GetName() == Expected.FontObject);
		TestEqual(*FString::Printf(TEXT("%s uses the token typeface"), Expected.Text),
			Font.TypefaceFontName, FName(Expected.Typeface));
		TestEqual(*FString::Printf(TEXT("%s uses the token point size"), Expected.Text),
			static_cast<int32>(Font.Size), Expected.PointSize);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_Foundation_GalleryRejectsUnknownSelection,
	"Mordecai.UI.Foundation.GalleryRejectsUnknownSelection",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_Foundation_GalleryRejectsUnknownSelection::RunTest(const FString& Parameters)
{
	TestTrue("known component/theme/sheet selection is valid",
		UMordecaiTinTallyGallerySubsystem::IsValidSelection(
			TEXT("Plate"), TEXT("diorama"), TEXT("sheet")));
	for (const TCHAR* Component : {
		TEXT("HUDLayout"), TEXT("GaugePlate"), TEXT("Bar"),
		TEXT("SpellSlot"), TEXT("InteractPrompt"), TEXT("LoadoutPlate"),
		TEXT("StatusChip"), TEXT("CombatCallout") })
	{
		TestTrue(*FString::Printf(TEXT("%s HUD gallery selection is valid"), Component),
			UMordecaiTinTallyGallerySubsystem::IsValidSelection(
				Component, TEXT("high-contrast"), TEXT("sheet")));
	}
	TestFalse("unknown component is rejected",
		UMordecaiTinTallyGallerySubsystem::IsValidSelection(
			TEXT("Unknown"), TEXT("diorama"), TEXT("sheet")));
	TestFalse("unknown theme is rejected",
		UMordecaiTinTallyGallerySubsystem::IsValidSelection(
			TEXT("Plate"), TEXT("sepia"), TEXT("sheet")));
	TestFalse("only the sheet state is currently accepted",
		UMordecaiTinTallyGallerySubsystem::IsValidSelection(
			TEXT("Plate"), TEXT("diorama"), TEXT("hover")));

	const FObjectProperty* GalleryWidgetProperty = FindFProperty<FObjectProperty>(
		UMordecaiTinTallyGallerySubsystem::StaticClass(), TEXT("GalleryWidget"));
	TestNotNull("gallery subsystem retains its widget through reflection", GalleryWidgetProperty);
	TestTrue("gallery widget reference is transient",
		GalleryWidgetProperty
		&& GalleryWidgetProperty->HasAnyPropertyFlags(CPF_Transient));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_Foundation_HUDGalleryUsesShippedWidgets,
	"Mordecai.UI.Foundation.HUDGalleryUsesShippedWidgets",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_Foundation_HUDGalleryUsesShippedWidgets::RunTest(const FString& Parameters)
{
	struct FExpectation
	{
		const TCHAR* Component;
		const TCHAR* WidgetName;
		const TCHAR* StageName;
		UClass* WidgetClass;
	};
	const FExpectation Expectations[] = {
		{ TEXT("HUDLayout"), TEXT("GalleryRealCombatHUD"), nullptr, UMordecaiCombatHUDWidget::StaticClass() },
		{ TEXT("GaugePlate"), TEXT("GalleryRealGaugeHUD_0"), nullptr, UMordecaiCombatHUDWidget::StaticClass() },
		{ TEXT("Bar"), TEXT("GalleryRealHealthBar_0"), TEXT("GalleryBarStage0"), UMordecaiHealthBarWidget::StaticClass() },
		{ TEXT("SpellSlot"), TEXT("GalleryRealSpellSlot_0"), TEXT("GallerySpellStage0"), UMordecaiSpellCooldownWidget::StaticClass() },
		{ TEXT("InteractPrompt"), TEXT("GalleryRealInteractPrompt_0"), nullptr, UMordecaiPickupPromptWidget::StaticClass() },
		{ TEXT("LoadoutPlate"), TEXT("GalleryRealLoadout_0"), TEXT("GalleryLoadoutStage"), UMordecaiEquippedWeaponWidget::StaticClass() },
		{ TEXT("StatusChip"), TEXT("GalleryRealStatusChip_0"), nullptr, UMordecaiStatusEffectIndicatorWidget::StaticClass() },
		{ TEXT("StatusChip"), TEXT("GalleryRealStatusBar"), nullptr, UMordecaiStatusEffectBarWidget::StaticClass() },
		{ TEXT("CombatCallout"), TEXT("GalleryRealCombatCallout_0"), nullptr, UMordecaiCombatFeedbackWidget::StaticClass() },
	};
	for (const TCHAR* Theme : { TEXT("diorama"), TEXT("high-contrast") })
	{
		for (const FExpectation& Expected : Expectations)
		{
			UMordecaiTinTallyGalleryWidget* Gallery = NewObject<UMordecaiTinTallyGalleryWidget>();
			Gallery->SetFixtureSelectionForTest(Expected.Component, Theme);
			Gallery->Initialize();
			UWidget* Widget = Gallery->WidgetTree
				? Gallery->WidgetTree->FindWidget(Expected.WidgetName)
				: nullptr;
			TestTrue(*FString::Printf(TEXT("%s/%s embeds real %s"),
				Expected.Component, Theme, *Expected.WidgetClass->GetName()),
				Widget && Widget->IsA(Expected.WidgetClass));
			if (FCString::Stricmp(Expected.Component, TEXT("InteractPrompt")) == 0)
			{
				const UCanvasPanelSlot* PromptSlot =
					Widget ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr;
				TestEqual(*FString::Printf(
					TEXT("%s/%s embeds the prompt at its live 720px width"),
					Expected.Component, Theme),
					PromptSlot ? static_cast<float>(PromptSlot->GetSize().X) : 0.f,
					720.f);
			}
			if (Expected.StageName)
			{
				TestNotNull(*FString::Printf(
					TEXT("%s/%s presents the shipped widget on its reference stage"),
					Expected.Component, Theme),
					Gallery->WidgetTree->FindWidget(Expected.StageName));
			}
		}

		UMordecaiTinTallyGalleryWidget* HUDGallery =
			NewObject<UMordecaiTinTallyGalleryWidget>();
		HUDGallery->SetFixtureSelectionForTest(TEXT("HUDLayout"), Theme);
		HUDGallery->Initialize();
		UMordecaiCombatHUDWidget* HUD = HUDGallery->WidgetTree
			? Cast<UMordecaiCombatHUDWidget>(
				HUDGallery->WidgetTree->FindWidget(TEXT("GalleryRealCombatHUD")))
			: nullptr;
		if (!TestNotNull(*FString::Printf(
			TEXT("HUDLayout/%s exposes its shipped HUD fixture"), Theme), HUD))
		{
			continue;
		}
		TSharedRef<SWidget> HUDSlate = HUD->TakeWidget();
		UMordecaiStatusEffectBarWidget* StatusBar =
			Cast<UMordecaiStatusEffectBarWidget>(
				HUD->GetNamedWidget(HUD, TEXT("StatusEffectBar")));
		UPanelWidget* StatusContainer = StatusBar
			? Cast<UPanelWidget>(
				StatusBar->WidgetTree->FindWidget(TEXT("IndicatorContainer")))
			: nullptr;
		TSet<FString> StatusNames;
		for (int32 Index = 0; StatusContainer && Index < StatusContainer->GetChildrenCount(); ++Index)
		{
			if (UMordecaiStatusEffectIndicatorWidget* Indicator =
				Cast<UMordecaiStatusEffectIndicatorWidget>(
					StatusContainer->GetChildAt(Index)))
			{
				TArray<UWidget*> Widgets;
				Indicator->WidgetTree->GetAllWidgets(Widgets);
				for (const UWidget* Widget : Widgets)
				{
					if (const UTextBlock* Text = Cast<UTextBlock>(Widget);
						Text && !Text->GetText().IsEmpty())
					{
						StatusNames.Add(Text->GetText().ToString());
					}
				}
			}
		}
		for (const TCHAR* ExpectedStatus : {
			TEXT("Frostbitten"), TEXT("Focused"), TEXT("Burning"), TEXT("Stone Skin") })
		{
			TestTrue(*FString::Printf(
				TEXT("HUDLayout/%s seeds card frame status '%s'"),
				Theme, ExpectedStatus),
				StatusNames.Contains(ExpectedStatus));
		}
		UMordecaiCombatFeedbackWidget* Callout =
			Cast<UMordecaiCombatFeedbackWidget>(
				HUD->GetNamedWidget(HUD, TEXT("CombatFeedback")));
		UMordecaiComboCounterWidget* Combo =
			Cast<UMordecaiComboCounterWidget>(
				HUD->GetNamedWidget(HUD, TEXT("ComboCounter")));
		TestNotNull(*FString::Printf(TEXT("HUDLayout/%s callout"), Theme), Callout);
		TestNotNull(*FString::Printf(TEXT("HUDLayout/%s combo"), Theme), Combo);
		TestNotNull(*FString::Printf(TEXT("HUDLayout/%s seeds frame 3 callout"), Theme),
			MordecaiTinTallyFoundationTest::FindTextBlock(
				Callout ? Callout->WidgetTree : nullptr, TEXT("Parried!")));
		TestNotNull(*FString::Printf(TEXT("HUDLayout/%s seeds max authored combo"), Theme),
			MordecaiTinTallyFoundationTest::FindTextBlock(
				Combo ? Combo->WidgetTree : nullptr, TEXT("Hit 5")));
		const UCanvasPanelSlot* StatusSlot = StatusBar
			? Cast<UCanvasPanelSlot>(StatusBar->Slot) : nullptr;
		const UCanvasPanelSlot* CalloutSlot = Callout
			? Cast<UCanvasPanelSlot>(Callout->Slot) : nullptr;
		const UCanvasPanelSlot* ComboSlot = Combo
			? Cast<UCanvasPanelSlot>(Combo->Slot) : nullptr;
		TestEqual(*FString::Printf(TEXT("HUDLayout/%s status uses real offset"), Theme),
			StatusSlot ? StatusSlot->GetPosition() : FVector2D::ZeroVector,
			FVector2D(32.f, 318.f));
		TestEqual(*FString::Printf(TEXT("HUDLayout/%s callout uses real bottom"), Theme),
			CalloutSlot ? CalloutSlot->GetPosition() : FVector2D::ZeroVector,
			FVector2D(0.f, 180.f));
		TestEqual(*FString::Printf(TEXT("HUDLayout/%s combo uses real top"), Theme),
			ComboSlot ? ComboSlot->GetPosition() : FVector2D::ZeroVector,
			FVector2D(0.f, 184.f));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_Foundation_NoRetiredAssetNames,
	"Mordecai.UI.Foundation.NoRetiredAssetNames",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_Foundation_NoRetiredAssetNames::RunTest(const FString& Parameters)
{
	using namespace MordecaiTinTallyFoundationTest;
	FAssetRegistryModule& RegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> Assets;
	RegistryModule.Get().GetAssetsByPath(FName(Root), Assets, /*bRecursive*/ true);
	TestTrue("the committed foundation contains its complete cooked asset set", Assets.Num() >= 150);

	int32 GlyphCount = 0;
	const TSet<FName> RetiredNames = {
		TEXT("T_UI_PlateMask"), TEXT("M_UI_Plate"), TEXT("T_UI_PlateFrame"),
		TEXT("T_UI_Plate"), TEXT("T_UI_PillPlate"), TEXT("T_UI_TagMask"),
		TEXT("T_UI_KeySquare"), TEXT("T_UI_KeyRound"), TEXT("T_UI_KeyPill"),
		TEXT("M_UI_BarFill"), TEXT("T_UI_StripeMask")
	};
	for (const FAssetData& Asset : Assets)
	{
		const FName Name = Asset.AssetName;
		TestFalse(*FString::Printf(TEXT("retired asset name %s is absent"), *Name.ToString()),
			RetiredNames.Contains(Name));
		if (Name.ToString().StartsWith(TEXT("T_UI_Glyph_")))
		{
			++GlyphCount;
		}
	}
	TestEqual("all 117 single-ink glyph masks are committed", GlyphCount, 117);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_Foundation_RuntimeFollowups,
	"Mordecai.UI.Foundation.RuntimeFollowups",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_Foundation_RuntimeFollowups::RunTest(const FString& Parameters)
{
	using namespace MordecaiTinTallyFoundationTest;

	UMordecaiTallyWidget* Tally = NewObject<UMordecaiTallyWidget>();
	const FArrayProperty* GhostFrames =
		FindFProperty<FArrayProperty>(Tally->GetClass(), TEXT("GhostBundleFrames"));
	TestNotNull("tally exposes the dotted-ghost atlas layer separately from its filled frames",
		GhostFrames);
	if (GhostFrames)
	{
		FScriptArrayHelper Helper(GhostFrames, GhostFrames->ContainerPtrToValuePtr<void>(Tally));
		TestEqual("dotted ghost atlas has frames 0 through 5", Helper.Num(), 6);
	}
	Tally->Initialize();
	TestNotNull("the tally materializes a separately tinted ghost image",
		Tally->WidgetTree ? Tally->WidgetTree->FindWidget(TEXT("TallyGhost_0")) : nullptr);
	TestNotNull("the tally materializes a separately tinted fill image",
		Tally->WidgetTree ? Tally->WidgetTree->FindWidget(TEXT("TallyFill_0")) : nullptr);

	const FMordecaiUIThemeColors Colors = FMordecaiUIThemeColors::MakeDiorama();
	UWidgetTree* Tree = NewObject<UWidgetTree>();
	const FMordecaiUIPrimitiveSpec Tag = FMordecaiUIPrimitiveBuilder::Tag(Colors);
	UOverlay* BuiltTag = FMordecaiUIPrimitiveBuilder::Build(Tree, Tag, TEXT("FollowupTag"));
	FMordecaiUIPrimitiveSpec UnsetWidthTag = Tag;
	if (FMordecaiUIPrimitiveLayer* Grommet =
		UnsetWidthTag.Layers.FindByPredicate([](const FMordecaiUIPrimitiveLayer& Layer)
		{
			return Layer.Name == TEXT("GrommetInk");
		}))
	{
		Grommet->DesiredSize.X = 0.f;
		Grommet->DesiredSize.Y = 0.f;
	}
	FMordecaiUIPrimitiveBuilder::Apply(BuiltTag, UnsetWidthTag);
	USizeBox* GrommetBox = FindLayerSizeBox(BuiltTag, TEXT("GrommetInk"));
	TestTrue("Apply clears a stale fixed-width override when the new spec unsets that axis",
		GrommetBox && !GrommetBox->IsWidthOverride());
	TestTrue("Apply clears a stale fixed-height override when the new spec unsets that axis",
		GrommetBox && !GrommetBox->IsHeightOverride());

	UWidgetTree* PlateTree = NewObject<UWidgetTree>();
	const FMordecaiUIPrimitiveSpec Plate = FMordecaiUIPrimitiveBuilder::Plate(Colors);
	UOverlay* BuiltPlate = FMordecaiUIPrimitiveBuilder::Build(PlateTree, Plate, TEXT("FollowupPlate"));
	FMordecaiUIPrimitiveSpec MismatchedPlate = Plate;
	MismatchedPlate.Layers.RemoveAt(MismatchedPlate.Layers.Num() - 1);
	AddExpectedMessagePlain(
		TEXT("Tin Tally primitive Apply layer mismatch"),
		ELogVerbosity::Warning,
		EAutomationExpectedMessageFlags::Contains,
		1);
	TestFalse("Apply rejects a primitive spec whose named layer set no longer matches",
		FMordecaiUIPrimitiveBuilder::Apply(BuiltPlate, MismatchedPlate));

	FMordecaiUIPrimitiveSpec SameCountNameMismatch = Plate;
	SameCountNameMismatch.Layers.Last().Name = TEXT("NotTheFill");
	AddExpectedMessagePlain(
		TEXT("Tin Tally primitive Apply layer mismatch"),
		ELogVerbosity::Warning,
		EAutomationExpectedMessageFlags::Contains,
		1);
	TestFalse("Apply rejects a same-count spec whose named layer set differs",
		FMordecaiUIPrimitiveBuilder::Apply(BuiltPlate, SameCountNameMismatch));
	return true;
}
