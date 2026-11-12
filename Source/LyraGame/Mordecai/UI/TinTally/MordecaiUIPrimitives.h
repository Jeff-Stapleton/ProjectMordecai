// Project Mordecai — shared Tin Tally RoundedBox primitives (US-102)

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "Mordecai/UI/TinTally/MordecaiUITheme.h"

#include "MordecaiUIPrimitives.generated.h"

class UOverlay;
class UWidgetTree;

UENUM(BlueprintType)
enum class EMordecaiUIPrintDepth : uint8
{
	HUD,
	Menu
};

UENUM(BlueprintType)
enum class EMordecaiKeycapShape : uint8
{
	Square,
	Round,
	Pill
};

UENUM(BlueprintType)
enum class EMordecaiUIFocusVariant : uint8
{
	Default,
	Border,
	FillGap
};

USTRUCT(BlueprintType)
struct LYRAGAME_API FMordecaiUIPrimitiveLayer
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Tin Tally")
	FName Name;

	UPROPERTY(BlueprintReadOnly, Category = "Tin Tally")
	FSlateBrush Brush;

	UPROPERTY(BlueprintReadOnly, Category = "Tin Tally")
	FMargin Padding;

	UPROPERTY(BlueprintReadOnly, Category = "Tin Tally")
	FVector2D Translation = FVector2D::ZeroVector;

	/** Non-zero only for fixed-size layers such as the tag grommet. */
	UPROPERTY(BlueprintReadOnly, Category = "Tin Tally")
	FVector2D DesiredSize = FVector2D::ZeroVector;

	/** Position of a fixed-size layer's center in design pixels. */
	UPROPERTY(BlueprintReadOnly, Category = "Tin Tally")
	FVector2D Center = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Tin Tally")
	TEnumAsByte<EHorizontalAlignment> HorizontalAlignment = HAlign_Fill;

	UPROPERTY(BlueprintReadOnly, Category = "Tin Tally")
	TEnumAsByte<EVerticalAlignment> VerticalAlignment = VAlign_Fill;
};

USTRUCT(BlueprintType)
struct LYRAGAME_API FMordecaiUIPrimitiveSpec
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Tin Tally")
	TArray<FMordecaiUIPrimitiveLayer> Layers;

	UPROPERTY(BlueprintReadOnly, Category = "Tin Tally")
	float OuterOutlineWidth = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Tin Tally")
	float InnerStrokeWidth = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Tin Tally")
	float FocusStrokeWidth = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Tin Tally")
	float FocusKeylineWidth = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Tin Tally")
	float FocusOffset = 0.f;
};

/**
 * Constructs all shared Tin Tally pieces from texture-free RoundedBox brushes.
 * Build() materializes a spec into a reusable Overlay in a widget tree.
 * Apply() retints and regeometries its named layers in place without rebuilding.
 */
struct LYRAGAME_API FMordecaiUIPrimitiveBuilder
{
	static FMordecaiUIPrimitiveSpec Plate(
		const FMordecaiUIThemeColors& Colors,
		EMordecaiUIPrintDepth PrintDepth = EMordecaiUIPrintDepth::HUD);
	static FMordecaiUIPrimitiveSpec Plate(
		const FMordecaiUIThemeColors& Colors,
		EMordecaiUIPrintDepth PrintDepth,
		EMordecaiUITheme Theme);
	static FMordecaiUIPrimitiveSpec Pill(
		const FMordecaiUIThemeColors& Colors,
		EMordecaiUIPrintDepth PrintDepth = EMordecaiUIPrintDepth::HUD);
	static FMordecaiUIPrimitiveSpec Pill(
		const FMordecaiUIThemeColors& Colors,
		EMordecaiUIPrintDepth PrintDepth,
		EMordecaiUITheme Theme);
	static FMordecaiUIPrimitiveSpec Tag(
		const FMordecaiUIThemeColors& Colors,
		EMordecaiUIPrintDepth PrintDepth = EMordecaiUIPrintDepth::HUD);
	static FMordecaiUIPrimitiveSpec Tag(
		const FMordecaiUIThemeColors& Colors,
		EMordecaiUIPrintDepth PrintDepth,
		EMordecaiUITheme Theme);
	static FMordecaiUIPrimitiveSpec Well(const FMordecaiUIThemeColors& Colors, bool bBar);
	static FMordecaiUIPrimitiveSpec InsetCardWell(const FMordecaiUIThemeColors& Colors);
	static FMordecaiUIPrimitiveSpec Keycap(
		const FMordecaiUIThemeColors& Colors,
		EMordecaiKeycapShape Shape);
	static FMordecaiUIPrimitiveSpec Focus(
		const FMordecaiUIThemeColors& Colors,
		float PieceRadius,
		EMordecaiUITheme Theme);
	static FMordecaiUIPrimitiveSpec Focus(
		const FMordecaiUIThemeColors& Colors,
		float PieceRadius,
		EMordecaiUITheme Theme,
		EMordecaiUIFocusVariant Variant);

	static UOverlay* Build(UWidgetTree* WidgetTree, const FMordecaiUIPrimitiveSpec& Spec, FName RootName);
	/** Returns false when the built overlay and incoming named-layer set do not match. */
	static bool Apply(UOverlay* Overlay, const FMordecaiUIPrimitiveSpec& Spec);
};
