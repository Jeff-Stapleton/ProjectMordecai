// Project Mordecai — cooked Tin Tally foundation manifest (US-103)

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Mordecai/UI/TinTally/MordecaiUIPrimitives.h"

#include "MordecaiUIFoundation.generated.h"

class UFont;
class UMaterialInterface;
class UTexture2D;

/** Saved Diorama and High Contrast geometry for one shared primitive. */
UCLASS(BlueprintType)
class LYRAGAME_API UMordecaiUIPrimitiveStyleDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tin Tally")
	FMordecaiUIPrimitiveSpec Diorama;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tin Tally")
	FMordecaiUIPrimitiveSpec HighContrast;
};

/** Cook anchor and credits data for the complete Tin Tally foundation. */
UCLASS(BlueprintType)
class LYRAGAME_API UMordecaiUIFoundationDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tin Tally|Credits")
	TArray<FString> FontLicenseNotices;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tin Tally|Fonts")
	TArray<TSoftObjectPtr<UFont>> CompositeFonts;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tin Tally|Art")
	TArray<TSoftObjectPtr<UTexture2D>> Textures;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tin Tally|Art")
	TSoftObjectPtr<UMaterialInterface> PatternFillMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tin Tally|Styles")
	TArray<TSoftObjectPtr<UMordecaiUIPrimitiveStyleDataAsset>> PrimitiveStyles;
};
