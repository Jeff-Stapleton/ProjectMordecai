// Project Mordecai — Tin Tally cooked-foundation contract tests (US-103 RED)

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/Overlay.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/DataAsset.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "Fonts/CompositeFont.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Misc/AutomationTest.h"
#include "Modules/ModuleManager.h"
#include "Mordecai/UI/TinTally/MordecaiTinTallyGallerySubsystem.h"
#include "Mordecai/UI/TinTally/MordecaiTallyWidget.h"
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
		TestTrue("Diorama contains reviewed sRGB-converted token values",
			Diorama->Colors.Plate.Equals(FMordecaiUIThemeColors::MakeDiorama().Plate, 0.0001f));
	}
	if (HighContrast)
	{
		TestEqual("High Contrast asset is labelled High Contrast",
			HighContrast->Theme, EMordecaiUITheme::HighContrast);
		TestTrue("High Contrast contains reviewed sRGB-converted token values",
			HighContrast->Colors.Focus.Equals(
				FMordecaiUIThemeColors::MakeHighContrast().Focus, 0.0001f));
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

	TestNotNull("the Development gallery subsystem is compiled",
		FindObject<UClass>(nullptr, TEXT("/Script/LyraGame.MordecaiTinTallyGallerySubsystem")));
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
	}
	FMordecaiUIPrimitiveBuilder::Apply(BuiltTag, UnsetWidthTag);
	USizeBox* GrommetBox = BuiltTag && BuiltTag->GetChildrenCount() > 3
		? Cast<USizeBox>(BuiltTag->GetChildAt(3)) : nullptr;
	TestTrue("Apply clears a stale fixed-width override when the new spec unsets that axis",
		GrommetBox && !GrommetBox->IsWidthOverride());

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
	return true;
}
