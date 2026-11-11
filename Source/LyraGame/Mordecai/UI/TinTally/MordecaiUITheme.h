// Project Mordecai — Tin Tally runtime theme contract (US-102)

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "MordecaiUITheme.generated.h"

UENUM(BlueprintType)
enum class EMordecaiUITheme : uint8
{
	Diorama,
	HighContrast
};

/**
 * Every runtime color token from Design/TinTally/ds/project/tokens.json.
 * Values are stored as linear colors, but authored from the design system's
 * sRGB hex values through FLinearColor::FromSRGBColor.
 */
USTRUCT(BlueprintType)
struct LYRAGAME_API FMordecaiUIThemeColors
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "plate"))
	FLinearColor Plate = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "plate-raised"))
	FLinearColor PlateRaised = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "well"))
	FLinearColor Well = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "lip"))
	FLinearColor Lip = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "rim"))
	FLinearColor Rim = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "rim-shade"))
	FLinearColor RimShade = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "ink"))
	FLinearColor Ink = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "enamel"))
	FLinearColor Enamel = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "enamel-shade"))
	FLinearColor EnamelShade = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "veil"))
	FLinearColor Veil = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "veil-death"))
	FLinearColor VeilDeath = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "text"))
	FLinearColor Text = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "text-muted"))
	FLinearColor TextMuted = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "text-disabled"))
	FLinearColor TextDisabled = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "ink-muted"))
	FLinearColor InkMuted = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "focus"))
	FLinearColor Focus = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "vital"))
	FLinearColor Vital = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "vital-trail"))
	FLinearColor VitalTrail = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "heal"))
	FLinearColor Heal = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "stamina-green"))
	FLinearColor StaminaGreen = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "stamina-yellow"))
	FLinearColor StaminaYellow = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "danger"))
	FLinearColor Danger = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "danger-text"))
	FLinearColor DangerText = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "delta-up"))
	FLinearColor DeltaUp = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "delta-down"))
	FLinearColor DeltaDown = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "stamina-red"))
	FLinearColor StaminaRed = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "foe"))
	FLinearColor Foe = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "posture"))
	FLinearColor Posture = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "spell-points"))
	FLinearColor SpellPoints = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "good"))
	FLinearColor Good = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "rarity-common"))
	FLinearColor RarityCommon = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "rarity-green"))
	FLinearColor RarityGreen = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "rarity-blue"))
	FLinearColor RarityBlue = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "rarity-purple"))
	FLinearColor RarityPurple = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "rarity-red"))
	FLinearColor RarityRed = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "rarity-gold"))
	FLinearColor RarityGold = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "feat-common"))
	FLinearColor FeatCommon = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "feat-rare"))
	FLinearColor FeatRare = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "feat-legendary"))
	FLinearColor FeatLegendary = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "buff"))
	FLinearColor Buff = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "debuff"))
	FLinearColor Debuff = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "dmg-physical"))
	FLinearColor DamagePhysical = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "dmg-fire"))
	FLinearColor DamageFire = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "dmg-frost"))
	FLinearColor DamageFrost = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "dmg-lightning"))
	FLinearColor DamageLightning = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "dmg-poison"))
	FLinearColor DamagePoison = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "dmg-corrosive"))
	FLinearColor DamageCorrosive = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "dmg-arcane"))
	FLinearColor DamageArcane = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "dmg-shadow"))
	FLinearColor DamageShadow = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "dmg-shadow-edge"))
	FLinearColor DamageShadowEdge = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "dmg-radiant"))
	FLinearColor DamageRadiant = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "biome-verdant"))
	FLinearColor BiomeVerdant = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "biome-searing"))
	FLinearColor BiomeSearing = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "biome-mire"))
	FLinearColor BiomeMire = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "biome-crystal"))
	FLinearColor BiomeCrystal = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "biome-storm"))
	FLinearColor BiomeStorm = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "biome-withered"))
	FLinearColor BiomeWithered = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "world-blockout"))
	FLinearColor WorldBlockout = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "world-blockout-line"))
	FLinearColor WorldBlockoutLine = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "world-grass"))
	FLinearColor WorldGrass = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "world-grass-dot"))
	FLinearColor WorldGrassDot = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "world-sand"))
	FLinearColor WorldSand = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "world-sand-dot"))
	FLinearColor WorldSandDot = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "world-mire"))
	FLinearColor WorldMire = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "world-mire-dot"))
	FLinearColor WorldMireDot = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "world-night"))
	FLinearColor WorldNight = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "world-night-dot"))
	FLinearColor WorldNightDot = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "world-snow"))
	FLinearColor WorldSnow = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "world-snow-dot"))
	FLinearColor WorldSnowDot = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "world-light"))
	FLinearColor WorldLight = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "world-dark"))
	FLinearColor WorldDark = FLinearColor::Transparent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tin Tally|Colors", meta = (DisplayName = "world-shade"))
	FLinearColor WorldShade = FLinearColor::Transparent;

	static FLinearColor FromSRGBHex(const FString& Hex, float Alpha = 1.f);
	static FMordecaiUIThemeColors MakeDiorama();
	static FMordecaiUIThemeColors MakeHighContrast();
	static const TArray<FName>& GetRuntimeColorTokenNames();

	bool TryGetColorToken(FName TokenName, FLinearColor& OutColor) const;
};

/**
 * Designer-authored Tin Tally theme asset. US-103 creates the Diorama and
 * High Contrast instances; native fallbacks keep this code-only story safe.
 */
UCLASS(BlueprintType, Const)
class LYRAGAME_API UMordecaiUIThemeDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UMordecaiUIThemeDataAsset();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tin Tally")
	EMordecaiUITheme Theme = EMordecaiUITheme::Diorama;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tin Tally")
	FMordecaiUIThemeColors Colors;

	UFUNCTION(BlueprintPure, Category = "Tin Tally")
	bool GetColorToken(FName TokenName, FLinearColor& OutColor) const;
};

LYRAGAME_API FName MordecaiUIThemeToName(EMordecaiUITheme Theme);
