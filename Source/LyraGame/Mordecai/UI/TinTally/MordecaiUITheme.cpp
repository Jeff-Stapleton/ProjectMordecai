// Project Mordecai — Tin Tally runtime theme contract (US-102)

#include "Mordecai/UI/TinTally/MordecaiUITheme.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiUITheme)

namespace
{
	struct FTokenEntry
	{
		const TCHAR* Name;
		FLinearColor FMordecaiUIThemeColors::* Member;
	};

#define MORDECAI_UI_TOKEN(NameText, MemberName) { TEXT(NameText), &FMordecaiUIThemeColors::MemberName }
	static const FTokenEntry TokenEntries[] = {
		MORDECAI_UI_TOKEN("plate", Plate), MORDECAI_UI_TOKEN("plate-raised", PlateRaised),
		MORDECAI_UI_TOKEN("well", Well), MORDECAI_UI_TOKEN("lip", Lip),
		MORDECAI_UI_TOKEN("rim", Rim), MORDECAI_UI_TOKEN("rim-shade", RimShade),
		MORDECAI_UI_TOKEN("ink", Ink), MORDECAI_UI_TOKEN("enamel", Enamel),
		MORDECAI_UI_TOKEN("enamel-shade", EnamelShade), MORDECAI_UI_TOKEN("veil", Veil),
		MORDECAI_UI_TOKEN("veil-death", VeilDeath), MORDECAI_UI_TOKEN("text", Text),
		MORDECAI_UI_TOKEN("text-muted", TextMuted), MORDECAI_UI_TOKEN("text-disabled", TextDisabled),
		MORDECAI_UI_TOKEN("ink-muted", InkMuted), MORDECAI_UI_TOKEN("focus", Focus),
		MORDECAI_UI_TOKEN("vital", Vital), MORDECAI_UI_TOKEN("vital-trail", VitalTrail),
		MORDECAI_UI_TOKEN("heal", Heal), MORDECAI_UI_TOKEN("stamina-green", StaminaGreen),
		MORDECAI_UI_TOKEN("stamina-yellow", StaminaYellow), MORDECAI_UI_TOKEN("danger", Danger),
		MORDECAI_UI_TOKEN("danger-text", DangerText), MORDECAI_UI_TOKEN("delta-up", DeltaUp),
		MORDECAI_UI_TOKEN("delta-down", DeltaDown), MORDECAI_UI_TOKEN("stamina-red", StaminaRed),
		MORDECAI_UI_TOKEN("foe", Foe), MORDECAI_UI_TOKEN("posture", Posture),
		MORDECAI_UI_TOKEN("spell-points", SpellPoints), MORDECAI_UI_TOKEN("good", Good),
		MORDECAI_UI_TOKEN("rarity-common", RarityCommon), MORDECAI_UI_TOKEN("rarity-green", RarityGreen),
		MORDECAI_UI_TOKEN("rarity-blue", RarityBlue), MORDECAI_UI_TOKEN("rarity-purple", RarityPurple),
		MORDECAI_UI_TOKEN("rarity-red", RarityRed), MORDECAI_UI_TOKEN("rarity-gold", RarityGold),
		MORDECAI_UI_TOKEN("feat-common", FeatCommon), MORDECAI_UI_TOKEN("feat-rare", FeatRare),
		MORDECAI_UI_TOKEN("feat-legendary", FeatLegendary), MORDECAI_UI_TOKEN("buff", Buff),
		MORDECAI_UI_TOKEN("debuff", Debuff), MORDECAI_UI_TOKEN("dmg-physical", DamagePhysical),
		MORDECAI_UI_TOKEN("dmg-fire", DamageFire), MORDECAI_UI_TOKEN("dmg-frost", DamageFrost),
		MORDECAI_UI_TOKEN("dmg-lightning", DamageLightning), MORDECAI_UI_TOKEN("dmg-poison", DamagePoison),
		MORDECAI_UI_TOKEN("dmg-corrosive", DamageCorrosive), MORDECAI_UI_TOKEN("dmg-arcane", DamageArcane),
		MORDECAI_UI_TOKEN("dmg-shadow", DamageShadow), MORDECAI_UI_TOKEN("dmg-shadow-edge", DamageShadowEdge),
		MORDECAI_UI_TOKEN("dmg-radiant", DamageRadiant), MORDECAI_UI_TOKEN("biome-verdant", BiomeVerdant),
		MORDECAI_UI_TOKEN("biome-searing", BiomeSearing), MORDECAI_UI_TOKEN("biome-mire", BiomeMire),
		MORDECAI_UI_TOKEN("biome-crystal", BiomeCrystal), MORDECAI_UI_TOKEN("biome-storm", BiomeStorm),
		MORDECAI_UI_TOKEN("biome-withered", BiomeWithered),
		MORDECAI_UI_TOKEN("world-blockout", WorldBlockout),
		MORDECAI_UI_TOKEN("world-blockout-line", WorldBlockoutLine),
		MORDECAI_UI_TOKEN("world-grass", WorldGrass), MORDECAI_UI_TOKEN("world-grass-dot", WorldGrassDot),
		MORDECAI_UI_TOKEN("world-sand", WorldSand), MORDECAI_UI_TOKEN("world-sand-dot", WorldSandDot),
		MORDECAI_UI_TOKEN("world-mire", WorldMire), MORDECAI_UI_TOKEN("world-mire-dot", WorldMireDot),
		MORDECAI_UI_TOKEN("world-night", WorldNight), MORDECAI_UI_TOKEN("world-night-dot", WorldNightDot),
		MORDECAI_UI_TOKEN("world-snow", WorldSnow), MORDECAI_UI_TOKEN("world-snow-dot", WorldSnowDot),
		MORDECAI_UI_TOKEN("world-light", WorldLight), MORDECAI_UI_TOKEN("world-dark", WorldDark),
		MORDECAI_UI_TOKEN("world-shade", WorldShade)
	};
#undef MORDECAI_UI_TOKEN

	FLinearColor C(const TCHAR* Hex, float Alpha = 1.f)
	{
		return FMordecaiUIThemeColors::FromSRGBHex(Hex, Alpha);
	}
}

FLinearColor FMordecaiUIThemeColors::FromSRGBHex(const FString& Hex, float Alpha)
{
	FString CleanHex = Hex;
	CleanHex.RemoveFromStart(TEXT("#"));
	FColor SRGB = FColor::FromHex(CleanHex);
	SRGB.A = 255;
	FLinearColor Linear = FLinearColor::FromSRGBColor(SRGB);
	Linear.A = FMath::Clamp(Alpha, 0.f, 1.f);
	return Linear;
}

FMordecaiUIThemeColors FMordecaiUIThemeColors::MakeDiorama()
{
	FMordecaiUIThemeColors T;
	T.Plate = C(TEXT("163634"));
	T.PlateRaised = C(TEXT("1C403E"));
	T.Well = C(TEXT("091A19"));
	T.Lip = C(TEXT("5E8C84"));
	T.Rim = C(TEXT("EFE2C2"));
	T.RimShade = C(TEXT("55665C"));
	T.Ink = C(TEXT("0F1418"));
	T.Enamel = C(TEXT("FBF3E1"));
	T.EnamelShade = C(TEXT("EADFC6"));
	T.Veil = C(TEXT("0F1418"), 0.72f);
	T.VeilDeath = C(TEXT("0F1418"), 0.60f);
	T.Text = C(TEXT("FFF8EA"));
	T.TextMuted = C(TEXT("BFD0C8"));
	T.TextDisabled = C(TEXT("9DB3AC"));
	T.InkMuted = C(TEXT("4D5753"));
	T.Focus = C(TEXT("FFFFFF"));
	T.Vital = C(TEXT("FF6F91"));
	T.VitalTrail = C(TEXT("FFE3A0"));
	T.Heal = C(TEXT("6FE39A"));
	T.StaminaGreen = C(TEXT("4CCB6E"));
	T.StaminaYellow = C(TEXT("E6E04A"));
	T.Danger = C(TEXT("FF5A3C"));
	T.DangerText = C(TEXT("FF9670"));
	T.Posture = C(TEXT("B8C4CF"));
	T.SpellPoints = C(TEXT("A98DFF"));
	T.Good = C(TEXT("FFC23D"));
	T.RarityCommon = C(TEXT("D3D8D6"));
	T.RarityGreen = C(TEXT("5FD98A"));
	T.RarityBlue = C(TEXT("6BB8FF"));
	T.RarityPurple = C(TEXT("C69BFF"));
	T.RarityRed = C(TEXT("FF8A8A"));
	T.FeatCommon = C(TEXT("DCE3DD"));
	T.FeatRare = C(TEXT("45D6BE"));
	T.FeatLegendary = C(TEXT("FF7ADA"));
	T.DamageFire = C(TEXT("FF8A3D"));
	T.DamageFrost = C(TEXT("9BE6FF"));
	T.DamageLightning = C(TEXT("FFE45C"));
	T.DamagePoison = C(TEXT("B5E35A"));
	T.DamageCorrosive = C(TEXT("4FE3C1"));
	T.DamageArcane = C(TEXT("C79BFF"));
	T.DamageShadow = C(TEXT("2B1838"));
	T.DamageShadowEdge = C(TEXT("E7D3FF"));
	T.DamageRadiant = C(TEXT("FFF0A8"));
	T.BiomeVerdant = C(TEXT("6E9E58"));
	T.BiomeSearing = C(TEXT("C9A066"));
	T.BiomeMire = C(TEXT("5C6B55"));
	T.BiomeCrystal = C(TEXT("8FB7C9"));
	T.BiomeStorm = C(TEXT("8C93B8"));
	T.BiomeWithered = C(TEXT("7D7176"));
	T.WorldBlockout = C(TEXT("E6E3DA"));
	T.WorldBlockoutLine = C(TEXT("D5D1C6"));
	T.WorldGrass = C(TEXT("7DB55A"));
	T.WorldGrassDot = C(TEXT("6FA34F"));
	T.WorldSand = C(TEXT("E2C690"));
	T.WorldSandDot = C(TEXT("D4B67C"));
	T.WorldMire = C(TEXT("4E5A45"));
	T.WorldMireDot = C(TEXT("5C6B55"));
	T.WorldNight = C(TEXT("1E2430"));
	T.WorldNightDot = C(TEXT("283042"));
	T.WorldSnow = C(TEXT("F2F4F7"));
	T.WorldSnowDot = C(TEXT("E3E8EE"));
	T.WorldShade = C(TEXT("0F1418"), 0.28f);

	T.DeltaUp = T.Heal;
	T.DeltaDown = T.Danger;
	T.StaminaRed = T.Danger;
	T.Foe = T.Danger;
	T.RarityGold = T.Good;
	T.Buff = T.Heal;
	T.Debuff = T.DangerText;
	T.DamagePhysical = T.Text;
	T.WorldLight = T.WorldBlockout;
	T.WorldDark = T.WorldNight;
	return T;
}

FMordecaiUIThemeColors FMordecaiUIThemeColors::MakeHighContrast()
{
	FMordecaiUIThemeColors T;
	T.Plate = C(TEXT("000000"));
	T.PlateRaised = C(TEXT("161616"));
	T.Well = C(TEXT("000000"));
	T.Lip = C(TEXT("FFFFFF"));
	T.Rim = C(TEXT("FFFFFF"));
	T.RimShade = C(TEXT("FFFFFF"));
	T.Ink = C(TEXT("000000"));
	T.Enamel = C(TEXT("FFFFFF"));
	T.EnamelShade = C(TEXT("FFFFFF"));
	T.Veil = C(TEXT("000000"), 0.85f);
	T.VeilDeath = C(TEXT("000000"), 0.75f);
	T.Text = C(TEXT("FFFFFF"));
	T.TextMuted = C(TEXT("E6E6E6"));
	T.TextDisabled = C(TEXT("C4C4C4"));
	T.InkMuted = C(TEXT("333333"));
	T.Focus = C(TEXT("FFFF00"));
	T.Vital = C(TEXT("FF8FAB"));
	T.VitalTrail = C(TEXT("FFE6A6"));
	T.Heal = C(TEXT("7CFFAA"));
	T.StaminaGreen = C(TEXT("5CFF8F"));
	T.StaminaYellow = C(TEXT("FFF05C"));
	T.Danger = C(TEXT("FF8A70"));
	T.DangerText = C(TEXT("FFA88F"));
	T.Posture = C(TEXT("E0E8F0"));
	T.SpellPoints = C(TEXT("C4B2FF"));
	T.Good = C(TEXT("FFD45C"));
	T.RarityCommon = C(TEXT("EDEDED"));
	T.RarityGreen = C(TEXT("7CF2A0"));
	T.RarityBlue = C(TEXT("9CCFFF"));
	T.RarityPurple = C(TEXT("DCBEFF"));
	T.RarityRed = C(TEXT("FFB0B0"));
	T.FeatCommon = C(TEXT("FFFFFF"));
	T.FeatRare = C(TEXT("66FFE6"));
	T.FeatLegendary = C(TEXT("FF9AE6"));
	T.DamageFire = C(TEXT("FFA460"));
	T.DamageFrost = C(TEXT("B8F0FF"));
	T.DamageLightning = C(TEXT("FFF07A"));
	T.DamagePoison = C(TEXT("C8F27A"));
	T.DamageCorrosive = C(TEXT("70FFDB"));
	T.DamageArcane = C(TEXT("DCBEFF"));
	T.DamageShadow = C(TEXT("000000"));
	T.DamageShadowEdge = C(TEXT("F0E0FF"));
	T.DamageRadiant = C(TEXT("FFF7CC"));

	// Level-art and biome stand-ins are intentionally theme invariant.
	const FMordecaiUIThemeColors Diorama = MakeDiorama();
	T.BiomeVerdant = Diorama.BiomeVerdant;
	T.BiomeSearing = Diorama.BiomeSearing;
	T.BiomeMire = Diorama.BiomeMire;
	T.BiomeCrystal = Diorama.BiomeCrystal;
	T.BiomeStorm = Diorama.BiomeStorm;
	T.BiomeWithered = Diorama.BiomeWithered;
	T.WorldBlockout = Diorama.WorldBlockout;
	T.WorldBlockoutLine = Diorama.WorldBlockoutLine;
	T.WorldGrass = Diorama.WorldGrass;
	T.WorldGrassDot = Diorama.WorldGrassDot;
	T.WorldSand = Diorama.WorldSand;
	T.WorldSandDot = Diorama.WorldSandDot;
	T.WorldMire = Diorama.WorldMire;
	T.WorldMireDot = Diorama.WorldMireDot;
	T.WorldNight = Diorama.WorldNight;
	T.WorldNightDot = Diorama.WorldNightDot;
	T.WorldSnow = Diorama.WorldSnow;
	T.WorldSnowDot = Diorama.WorldSnowDot;
	T.WorldShade = Diorama.WorldShade;

	T.DeltaUp = T.Heal;
	T.DeltaDown = T.Danger;
	T.StaminaRed = T.Danger;
	T.Foe = T.Danger;
	T.RarityGold = T.Good;
	T.Buff = T.Heal;
	T.Debuff = T.DangerText;
	T.DamagePhysical = T.Text;
	T.WorldLight = T.WorldBlockout;
	T.WorldDark = T.WorldNight;
	return T;
}

const TArray<FName>& FMordecaiUIThemeColors::GetRuntimeColorTokenNames()
{
	static const TArray<FName> Names = []
	{
		TArray<FName> Result;
		Result.Reserve(UE_ARRAY_COUNT(TokenEntries));
		for (const FTokenEntry& Entry : TokenEntries)
		{
			Result.Emplace(Entry.Name);
		}
		return Result;
	}();
	return Names;
}

bool FMordecaiUIThemeColors::TryGetColorToken(FName TokenName, FLinearColor& OutColor) const
{
	for (const FTokenEntry& Entry : TokenEntries)
	{
		if (TokenName == FName(Entry.Name))
		{
			OutColor = this->*(Entry.Member);
			return true;
		}
	}
	return false;
}

UMordecaiUIThemeDataAsset::UMordecaiUIThemeDataAsset()
	: Colors(FMordecaiUIThemeColors::MakeDiorama())
{
}

bool UMordecaiUIThemeDataAsset::GetColorToken(FName TokenName, FLinearColor& OutColor) const
{
	return Colors.TryGetColorToken(TokenName, OutColor);
}

FName MordecaiUIThemeToName(EMordecaiUITheme Theme)
{
	return Theme == EMordecaiUITheme::HighContrast ? FName(TEXT("HighContrast")) : FName(TEXT("Diorama"));
}
