// Project Mordecai — Tin Tally bundle widget (US-102)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"

#include "MordecaiTallyWidget.generated.h"

class UHorizontalBox;
class UImage;

/** D1's complete allow-list for the tally motif. */
UENUM(BlueprintType)
enum class EMordecaiTallyUsage : uint8
{
	SkillRank,
	RarityBadge,
	Seal,
	LoadingThrobber
};

/**
 * Draws Count using atlas frames 0..5, one image per five-mark bundle.
 * US-103 supplies the cooked frame brushes; this native fallback preserves
 * the runtime behavior and BindWidgetOptional contract without assets.
 */
UCLASS(BlueprintType, Blueprintable)
class LYRAGAME_API UMordecaiTallyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UMordecaiTallyWidget(const FObjectInitializer& ObjectInitializer);

	virtual bool Initialize() override;
	virtual void SynchronizeProperties() override;

	UFUNCTION(BlueprintCallable, Category = "Tin Tally|Tally")
	void SetCount(int32 NewCount);

	UFUNCTION(BlueprintCallable, Category = "Tin Tally|Tally")
	void SetMax(int32 NewMax);

	UFUNCTION(BlueprintCallable, Category = "Tin Tally|Tally")
	void SetCountAndMax(int32 NewCount, int32 NewMax);

	UFUNCTION(BlueprintPure, Category = "Tin Tally|Tally")
	int32 GetCount() const { return Count; }

	UFUNCTION(BlueprintPure, Category = "Tin Tally|Tally")
	int32 GetMax() const { return Max; }

	UFUNCTION(BlueprintPure, Category = "Tin Tally|Tally")
	EMordecaiTallyUsage GetUsage() const { return Usage; }

	UFUNCTION(BlueprintPure, Category = "Tin Tally|Tally")
	TArray<int32> GetRenderedBundleFrames() const;

	const TArray<FSlateBrush>& GetBundleFrames() const { return BundleFrames; }

	static bool IsAllowedUsage(EMordecaiTallyUsage Candidate);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tin Tally|Tally", meta = (ClampMin = "0"))
	int32 Count = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tin Tally|Tally", meta = (ClampMin = "0"))
	int32 Max = 20;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tin Tally|Tally")
	EMordecaiTallyUsage Usage = EMordecaiTallyUsage::SkillRank;

	/** Exactly six brushes: empty, one, two, three, four, full bundle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tin Tally|Tally", meta = (EditFixedSize))
	TArray<FSlateBrush> BundleFrames;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> BundleBox;

private:
	void BuildDefaultLayout();
	void Refresh();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> RenderedBundleImages;
};
