// Project Mordecai — Tin Tally runtime contract tests (US-102)

#include "Algo/AllOf.h"
#include "Misc/AutomationTest.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Mordecai/UI/TinTally/MordecaiTallyWidget.h"
#include "Mordecai/UI/TinTally/MordecaiUIPrimitives.h"
#include "Mordecai/UI/TinTally/MordecaiUITheme.h"
#include "Mordecai/UI/TinTally/MordecaiUIThemeSubsystem.h"
#include "UObject/UnrealType.h"

namespace MordecaiTinTallyThemeTest
{
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_Theme_SRGBConversion,
	"Mordecai.UI.Theme.SRGBConversion",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_Theme_SRGBConversion::RunTest(const FString& Parameters)
{
	const FLinearColor ExpectedPlate = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("163634")));
	const FLinearColor ConvertedPlate = FMordecaiUIThemeColors::FromSRGBHex(TEXT("#163634"));
	TestTrue("hex colors pass through FLinearColor::FromSRGBColor", ConvertedPlate.Equals(ExpectedPlate));
	TestFalse("sRGB bytes are not treated as already-linear values",
		ConvertedPlate.Equals(FLinearColor(0x16 / 255.f, 0x36 / 255.f, 0x34 / 255.f, 1.f)));

	static const FName ExpectedProperties[] = {
		TEXT("Plate"), TEXT("PlateRaised"), TEXT("Well"), TEXT("Lip"), TEXT("Rim"), TEXT("RimShade"),
		TEXT("Ink"), TEXT("Enamel"), TEXT("EnamelShade"), TEXT("Veil"), TEXT("VeilDeath"),
		TEXT("Text"), TEXT("TextMuted"), TEXT("TextDisabled"), TEXT("InkMuted"), TEXT("Focus"),
		TEXT("Vital"), TEXT("VitalTrail"), TEXT("Heal"), TEXT("StaminaGreen"), TEXT("StaminaYellow"),
		TEXT("Danger"), TEXT("DangerText"), TEXT("DeltaUp"), TEXT("DeltaDown"), TEXT("StaminaRed"),
		TEXT("Foe"), TEXT("Posture"), TEXT("SpellPoints"), TEXT("Good"), TEXT("RarityCommon"),
		TEXT("RarityGreen"), TEXT("RarityBlue"), TEXT("RarityPurple"), TEXT("RarityRed"),
		TEXT("RarityGold"), TEXT("FeatCommon"), TEXT("FeatRare"), TEXT("FeatLegendary"), TEXT("Buff"),
		TEXT("Debuff"), TEXT("DamagePhysical"), TEXT("DamageFire"), TEXT("DamageFrost"),
		TEXT("DamageLightning"), TEXT("DamagePoison"), TEXT("DamageCorrosive"), TEXT("DamageArcane"),
		TEXT("DamageShadow"), TEXT("DamageShadowEdge"), TEXT("DamageRadiant"), TEXT("BiomeVerdant"),
		TEXT("BiomeSearing"), TEXT("BiomeMire"), TEXT("BiomeCrystal"), TEXT("BiomeStorm"),
		TEXT("BiomeWithered"), TEXT("WorldBlockout"), TEXT("WorldBlockoutLine"), TEXT("WorldGrass"),
		TEXT("WorldGrassDot"), TEXT("WorldSand"), TEXT("WorldSandDot"), TEXT("WorldMire"),
		TEXT("WorldMireDot"), TEXT("WorldNight"), TEXT("WorldNightDot"), TEXT("WorldSnow"),
		TEXT("WorldSnowDot"), TEXT("WorldLight"), TEXT("WorldDark"), TEXT("WorldShade")
	};

	UScriptStruct* ColorsStruct = FMordecaiUIThemeColors::StaticStruct();
	for (const FName PropertyName : ExpectedProperties)
	{
		const FStructProperty* Property = FindFProperty<FStructProperty>(ColorsStruct, PropertyName);
		TestNotNull(*FString::Printf(TEXT("%s is a designer-readable FLinearColor field"), *PropertyName.ToString()), Property);
		if (Property)
		{
			TestTrue(*FString::Printf(TEXT("%s has FLinearColor type"), *PropertyName.ToString()),
				Property->Struct == TBaseStructure<FLinearColor>::Get());
		}
	}
	TestEqual("the runtime token table has every tokens.json color", FMordecaiUIThemeColors::GetRuntimeColorTokenNames().Num(),
		static_cast<int32>(UE_ARRAY_COUNT(ExpectedProperties)));

	const FMordecaiUIThemeColors Diorama = FMordecaiUIThemeColors::MakeDiorama();
	TestTrue("plate uses the converted design token", Diorama.Plate.Equals(ExpectedPlate));
	TestEqual("veil keeps its alpha", Diorama.Veil.A, 0.72f);
	TestEqual("death veil keeps its alpha", Diorama.VeilDeath.A, 0.60f);
	TestEqual("world shade keeps its alpha", Diorama.WorldShade.A, 0.28f);
	TestTrue("delta-up aliases heal", Diorama.DeltaUp.Equals(Diorama.Heal));
	TestTrue("delta-down aliases danger", Diorama.DeltaDown.Equals(Diorama.Danger));
	TestTrue("stamina-red aliases danger", Diorama.StaminaRed.Equals(Diorama.Danger));
	TestTrue("foe aliases danger", Diorama.Foe.Equals(Diorama.Danger));
	TestTrue("rarity-gold aliases good", Diorama.RarityGold.Equals(Diorama.Good));
	TestTrue("buff aliases heal", Diorama.Buff.Equals(Diorama.Heal));
	TestTrue("debuff aliases danger-text", Diorama.Debuff.Equals(Diorama.DangerText));
	TestTrue("physical damage aliases text", Diorama.DamagePhysical.Equals(Diorama.Text));
	TestTrue("world-light aliases blockout", Diorama.WorldLight.Equals(Diorama.WorldBlockout));
	TestTrue("world-dark aliases night", Diorama.WorldDark.Equals(Diorama.WorldNight));

	UMordecaiUIThemeDataAsset* Asset = NewObject<UMordecaiUIThemeDataAsset>();
	TestNotNull("theme data asset owns the complete colors struct",
		FindFProperty<FStructProperty>(Asset->GetClass(), TEXT("Colors")));
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

	UObject* LiveWidget = NewObject<UImage>();
	const UObject* OriginalWidget = LiveWidget;
	int32 NotificationCount = 0;
	EMordecaiUITheme NotifiedTheme = EMordecaiUITheme::Diorama;
	const FDelegateHandle Handle = Service->OnThemeChangedNative().AddLambda(
		[&NotificationCount, &NotifiedTheme, LiveWidget](EMordecaiUITheme Theme, const FMordecaiUIThemeColors& Colors)
		{
			++NotificationCount;
			NotifiedTheme = Theme;
			(void)LiveWidget;
			(void)Colors;
		});

	Service->SetActiveTheme(EMordecaiUITheme::HighContrast);
	TestEqual("runtime selection swaps to High Contrast", Service->GetActiveTheme(), EMordecaiUITheme::HighContrast);
	TestEqual("live widget receives one notification", NotificationCount, 1);
	TestEqual("notification identifies High Contrast", NotifiedTheme, EMordecaiUITheme::HighContrast);
	TestTrue("the live widget is not reconstructed", LiveWidget == OriginalWidget);
	TestTrue("High Contrast plate is black", Service->GetActiveColors().Plate.Equals(FLinearColor::Black));
	TestTrue("High Contrast focus is yellow",
		Service->GetActiveColors().Focus.Equals(FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("FFFF00")))));

	Service->SetActiveTheme(EMordecaiUITheme::HighContrast);
	TestEqual("selecting the current theme does not rebroadcast", NotificationCount, 1);
	Service->OnThemeChangedNative().Remove(Handle);
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

	const FMordecaiUIPrimitiveSpec Pill = FMordecaiUIPrimitiveBuilder::Pill(Colors);
	TestEqual("pill uses the five plate layers", Pill.Layers.Num(), 5);
	TestTrue("pill radii follow half-height mode",
		Algo::AllOf(Pill.Layers, [](const FMordecaiUIPrimitiveLayer& Layer)
		{
			return Layer.Brush.OutlineSettings.RoundingType == ESlateBrushRoundingType::HalfHeightRadius;
		}));

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
	TestTrue("grommet is centered at x 22",
		GrommetInk && GrommetShade && GrommetWell
		&& GrommetInk->Center.X == 22.f && GrommetShade->Center.X == 22.f && GrommetWell->Center.X == 22.f);
	TestTrue("grommet sizes are 22/18/12",
		GrommetInk && GrommetShade && GrommetWell
		&& GrommetInk->DesiredSize == FVector2D(22.f)
		&& GrommetShade->DesiredSize == FVector2D(18.f)
		&& GrommetWell->DesiredSize == FVector2D(12.f));

	const FMordecaiUIPrimitiveSpec Well = FMordecaiUIPrimitiveBuilder::Well(Colors, false);
	const FMordecaiUIPrimitiveLayer* WellLayer = FindLayer(Well, TEXT("Well"));
	TestTrue("well radius is 8", WellLayer && IsRadius(WellLayer->Brush, FVector4(8.f)));
	TestEqual("well lip is 2", Well.InnerStrokeWidth, 2.f);
	TestEqual("well brush draws the 2px lip", WellLayer ? WellLayer->Brush.OutlineSettings.Width : 0.f, 2.f);
	const FMordecaiUIPrimitiveSpec BarWell = FMordecaiUIPrimitiveBuilder::Well(Colors, true);
	const FMordecaiUIPrimitiveLayer* BarLayer = FindLayer(BarWell, TEXT("Well"));
	TestTrue("bar well radius is 6", BarLayer && IsRadius(BarLayer->Brush, FVector4(6.f)));

	const FMordecaiUIPrimitiveSpec Keycap = FMordecaiUIPrimitiveBuilder::Keycap(Colors, EMordecaiKeycapShape::Square);
	const FMordecaiUIPrimitiveLayer* KeycapFace = FindLayer(Keycap, TEXT("Face"));
	const FMordecaiUIPrimitiveLayer* KeycapLip = FindLayer(Keycap, TEXT("Lip"));
	TestTrue("square keycap radius is 8", KeycapFace && IsRadius(KeycapFace->Brush, FVector4(8.f)));
	TestEqual("keycap ink border is 2", KeycapFace ? KeycapFace->Brush.OutlineSettings.Width : 0.f, 2.f);
	TestTrue("keycap lower lip is 4", KeycapLip && KeycapLip->Translation == FVector2D(0.f, 4.f));

	const FMordecaiUIPrimitiveSpec Focus =
		FMordecaiUIPrimitiveBuilder::Focus(Colors, 12.f, EMordecaiUITheme::Diorama);
	TestEqual("focus ring is 3", Focus.FocusStrokeWidth, 3.f);
	TestEqual("focus keyline is 2", Focus.FocusKeylineWidth, 2.f);
	TestEqual("focus sits outside a 3px Diorama piece outline", Focus.FocusOffset, 3.f);
	const FMordecaiUIPrimitiveSpec HighContrastFocus =
		FMordecaiUIPrimitiveBuilder::Focus(FMordecaiUIThemeColors::MakeHighContrast(), 12.f,
			EMordecaiUITheme::HighContrast);
	TestEqual("High Contrast piece outline is 4", HighContrastFocus.FocusOffset, 4.f);

	for (const FMordecaiUIPrimitiveSpec* Spec : { &Plate, &Pill, &Tag, &Well, &Keycap, &Focus })
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_TallyWidget_CountAndClamp,
	"Mordecai.UI.TallyWidget.CountAndClamp",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_TallyWidget_CountAndClamp::RunTest(const FString& Parameters)
{
	UMordecaiTallyWidget* Tally = NewObject<UMordecaiTallyWidget>();
	TestEqual("the atlas contract has frames 0 through 5", Tally->GetBundleFrames().Num(), 6);
	TestEqual("default use is a D1-allowed skill rank", Tally->GetUsage(), EMordecaiTallyUsage::SkillRank);

	Tally->SetMax(20);
	Tally->SetCount(0);
	TestEqual("zero remains zero", Tally->GetCount(), 0);
	TestEqual("zero renders four empty bundles", Tally->GetRenderedBundleFrames(), TArray<int32>({ 0, 0, 0, 0 }));

	Tally->SetCount(12);
	TestEqual("twelve renders two bundles and two marks", Tally->GetRenderedBundleFrames(),
		TArray<int32>({ 5, 5, 2, 0 }));

	Tally->SetCount(20);
	TestEqual("Max renders four full bundles", Tally->GetRenderedBundleFrames(),
		TArray<int32>({ 5, 5, 5, 5 }));

	Tally->SetCount(21);
	TestEqual("Count above Max clamps", Tally->GetCount(), 20);
	Tally->SetCount(-4);
	TestEqual("negative Count clamps", Tally->GetCount(), 0);
	Tally->SetMax(-1);
	TestEqual("negative Max clamps", Tally->GetMax(), 0);
	TestEqual("Count reclamps when Max changes", Tally->GetCount(), 0);
	TestEqual("Max zero still renders the empty frame", Tally->GetRenderedBundleFrames(), TArray<int32>({ 0 }));

	const UEnum* UsageEnum = StaticEnum<EMordecaiTallyUsage>();
	TestEqual("only D1's four allowed jobs are exposed", UsageEnum->NumEnums() - 1, 4);
	TestTrue("skill ranks are allowed", UMordecaiTallyWidget::IsAllowedUsage(EMordecaiTallyUsage::SkillRank));
	TestTrue("rarity badges are allowed", UMordecaiTallyWidget::IsAllowedUsage(EMordecaiTallyUsage::RarityBadge));
	TestTrue("seals are allowed", UMordecaiTallyWidget::IsAllowedUsage(EMordecaiTallyUsage::Seal));
	TestTrue("the loading throbber is allowed", UMordecaiTallyWidget::IsAllowedUsage(EMordecaiTallyUsage::LoadingThrobber));
	return true;
}
