// Project Mordecai — Tin Tally runtime theme contract (US-102)

#include "Mordecai/UI/TinTally/MordecaiUITheme.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiUITheme)

namespace MordecaiUITheme
{
	struct FTokenEntry
	{
		const TCHAR* Name;
		FLinearColor FMordecaiUIThemeColors::* Member;
	};

#define TOKEN(NameText, MemberName) { TEXT(NameText), &FMordecaiUIThemeColors::MemberName }
	static const FTokenEntry TokenEntries[] = {
		TOKEN("plate", Plate), TOKEN("plate-raised", PlateRaised), TOKEN("well", Well), TOKEN("lip", Lip),
		TOKEN("rim", Rim), TOKEN("rim-shade", RimShade), TOKEN("ink", Ink), TOKEN("enamel", Enamel),
		TOKEN("enamel-shade", EnamelShade), TOKEN("veil", Veil), TOKEN("veil-death", VeilDeath),
		TOKEN("text", Text), TOKEN("text-muted", TextMuted), TOKEN("text-disabled", TextDisabled),
		TOKEN("ink-muted", InkMuted), TOKEN("focus", Focus), TOKEN("vital", Vital),
		TOKEN("vital-trail", VitalTrail), TOKEN("heal", Heal), TOKEN("stamina-green", StaminaGreen),
		TOKEN("stamina-yellow", StaminaYellow), TOKEN("danger", Danger), TOKEN("danger-text", DangerText),
		TOKEN("delta-up", DeltaUp), TOKEN("delta-down", DeltaDown), TOKEN("stamina-red", StaminaRed),
		TOKEN("foe", Foe), TOKEN("posture", Posture), TOKEN("spell-points", SpellPoints), TOKEN("good", Good),
		TOKEN("rarity-common", RarityCommon), TOKEN("rarity-green", RarityGreen),
		TOKEN("rarity-blue", RarityBlue), TOKEN("rarity-purple", RarityPurple),
		TOKEN("rarity-red", RarityRed), TOKEN("rarity-gold", RarityGold),
		TOKEN("feat-common", FeatCommon), TOKEN("feat-rare", FeatRare),
		TOKEN("feat-legendary", FeatLegendary), TOKEN("buff", Buff), TOKEN("debuff", Debuff),
		TOKEN("dmg-physical", DamagePhysical), TOKEN("dmg-fire", DamageFire),
		TOKEN("dmg-frost", DamageFrost), TOKEN("dmg-lightning", DamageLightning),
		TOKEN("dmg-poison", DamagePoison), TOKEN("dmg-corrosive", DamageCorrosive),
		TOKEN("dmg-arcane", DamageArcane), TOKEN("dmg-shadow", DamageShadow),
		TOKEN("dmg-shadow-edge", DamageShadowEdge), TOKEN("dmg-radiant", DamageRadiant),
		TOKEN("biome-verdant", BiomeVerdant), TOKEN("biome-searing", BiomeSearing),
		TOKEN("biome-mire", BiomeMire), TOKEN("biome-crystal", BiomeCrystal),
		TOKEN("biome-storm", BiomeStorm), TOKEN("biome-withered", BiomeWithered),
		TOKEN("world-blockout", WorldBlockout), TOKEN("world-blockout-line", WorldBlockoutLine),
		TOKEN("world-grass", WorldGrass), TOKEN("world-grass-dot", WorldGrassDot),
		TOKEN("world-sand", WorldSand), TOKEN("world-sand-dot", WorldSandDot),
		TOKEN("world-mire", WorldMire), TOKEN("world-mire-dot", WorldMireDot),
		TOKEN("world-night", WorldNight), TOKEN("world-night-dot", WorldNightDot),
		TOKEN("world-snow", WorldSnow), TOKEN("world-snow-dot", WorldSnowDot),
		TOKEN("world-light", WorldLight), TOKEN("world-dark", WorldDark), TOKEN("world-shade", WorldShade)
	};
#undef TOKEN

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
	using namespace MordecaiUITheme;
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
	using namespace MordecaiUITheme;
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
		Result.Reserve(UE_ARRAY_COUNT(MordecaiUITheme::TokenEntries));
		for (const MordecaiUITheme::FTokenEntry& Entry : MordecaiUITheme::TokenEntries)
		{
			Result.Emplace(Entry.Name);
		}
		return Result;
	}();
	return Names;
}

bool FMordecaiUIThemeColors::TryGetColorToken(FName TokenName, FLinearColor& OutColor) const
{
	for (const MordecaiUITheme::FTokenEntry& Entry : MordecaiUITheme::TokenEntries)
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
