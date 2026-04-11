// Project Mordecai — Spell Cooldown Widget (US-055)

#include "Mordecai/UI/MordecaiSpellCooldownWidget.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiSpellCooldownWidget)

// ---------------------------------------------------------------------------
// Programmatic Fallback (empty Blueprint support)
// ---------------------------------------------------------------------------

void UMordecaiSpellCooldownWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!CooldownProgressBar && WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultContent();
	}
}

void UMordecaiSpellCooldownWidget::BuildDefaultContent()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	CooldownProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("CooldownProgressBar"));
	UCanvasPanelSlot* BarSlot = Root->AddChildToCanvas(CooldownProgressBar);
	BarSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 0.7f));
	BarSlot->SetOffsets(FMargin(0.f));

	CooldownProgressBar->SetPercent(1.f);
	CooldownProgressBar->SetFillColorAndOpacity(FLinearColor(0.3f, 0.6f, 1.f)); // Light blue

	CooldownText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CooldownText"));
	UCanvasPanelSlot* TextSlot = Root->AddChildToCanvas(CooldownText);
	TextSlot->SetAnchors(FAnchors(0.f, 0.7f, 1.f, 1.f));
	TextSlot->SetOffsets(FMargin(0.f));

	CooldownText->SetText(FText::GetEmpty());
	CooldownText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	CooldownText->SetJustification(ETextJustify::Center);

	FSlateFontInfo FontInfo = CooldownText->GetFont();
	FontInfo.Size = 9;
	CooldownText->SetFont(FontInfo);
}

// ---------------------------------------------------------------------------
// Static Helpers (AC-055.8)
// ---------------------------------------------------------------------------

float UMordecaiSpellCooldownWidget::CalcCooldownPercent(float ElapsedTime, float TotalDuration)
{
	if (TotalDuration <= 0.f)
	{
		return 1.f; // No cooldown = ready
	}
	return FMath::Clamp(ElapsedTime / TotalDuration, 0.f, 1.f);
}

FString UMordecaiSpellCooldownWidget::FormatRemainingTime(float RemainingSeconds)
{
	if (RemainingSeconds <= 0.f)
	{
		return FString(); // Ready state = empty string
	}
	return FString::Printf(TEXT("%.1f"), RemainingSeconds);
}

// ---------------------------------------------------------------------------
// Binding (AC-055.9)
// ---------------------------------------------------------------------------

void UMordecaiSpellCooldownWidget::BindToCooldownTag(UAbilitySystemComponent* ASC, FGameplayTag CooldownTag, float TotalCooldown)
{
	if (!ASC || !CooldownTag.IsValid())
	{
		return;
	}

	BoundASC = ASC;
	BoundCooldownTag = CooldownTag;
	TotalCooldownDuration = TotalCooldown;

	ASC->RegisterGameplayTagEvent(BoundCooldownTag, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &UMordecaiSpellCooldownWidget::OnCooldownTagChanged);

	// Check if already on cooldown
	if (ASC->HasMatchingGameplayTag(BoundCooldownTag))
	{
		bIsOnCooldown = true;
		CooldownStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	}
	else
	{
		bIsOnCooldown = false;
	}

	UpdateDisplay();
}

// ---------------------------------------------------------------------------
// Tag Change Callback (AC-055.9)
// ---------------------------------------------------------------------------

void UMordecaiSpellCooldownWidget::OnCooldownTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		// Cooldown started
		bIsOnCooldown = true;
		CooldownStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

		// Try to get actual remaining time from the active GE with this cooldown tag
		if (UAbilitySystemComponent* ASC = BoundASC.Get())
		{
			FGameplayEffectQuery Query;
			Query.OwningTagQuery = FGameplayTagQuery::MakeQuery_MatchAnyTags(FGameplayTagContainer(BoundCooldownTag));
			TArray<FActiveGameplayEffectHandle> ActiveEffects = ASC->GetActiveEffects(Query);
			for (const FActiveGameplayEffectHandle& Handle : ActiveEffects)
			{
				const float Remaining = ASC->GetActiveGameplayEffect(Handle)->GetTimeRemaining(GetWorld()->GetTimeSeconds());
				const float Duration = ASC->GetActiveGameplayEffect(Handle)->GetDuration();
				if (Duration > 0.f)
				{
					TotalCooldownDuration = Duration;
					CooldownStartTime = GetWorld()->GetTimeSeconds() - (Duration - Remaining);
					break;
				}
			}
		}
	}
	else
	{
		// Cooldown ended
		bIsOnCooldown = false;
	}

	UpdateDisplay();
}

// ---------------------------------------------------------------------------
// Tick (AC-055.8)
// ---------------------------------------------------------------------------

void UMordecaiSpellCooldownWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bIsOnCooldown)
	{
		UpdateDisplay();
	}
}

void UMordecaiSpellCooldownWidget::UpdateDisplay()
{
	if (bIsOnCooldown && GetWorld())
	{
		const float CurrentTime = GetWorld()->GetTimeSeconds();
		const float Elapsed = CurrentTime - CooldownStartTime;
		const float Remaining = FMath::Max(0.f, TotalCooldownDuration - Elapsed);
		const float Percent = CalcCooldownPercent(Elapsed, TotalCooldownDuration);

		if (CooldownProgressBar)
		{
			CooldownProgressBar->SetPercent(Percent);
		}
		if (CooldownText)
		{
			CooldownText->SetText(FText::FromString(FormatRemainingTime(Remaining)));
		}

		// Auto-clear if cooldown should have expired
		if (Remaining <= 0.f)
		{
			bIsOnCooldown = false;
		}
	}
	else
	{
		// Ready state
		if (CooldownProgressBar)
		{
			CooldownProgressBar->SetPercent(1.f);
		}
		if (CooldownText)
		{
			CooldownText->SetText(FText::GetEmpty());
		}
	}
}
