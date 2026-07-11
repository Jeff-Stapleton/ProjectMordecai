// Project Mordecai — Combat HUD Root Widget (US-052, US-055, US-061)

#include "Mordecai/UI/MordecaiCombatHUDWidget.h"
#include "Mordecai/UI/MordecaiHealthBarWidget.h"
#include "Mordecai/UI/MordecaiStaminaBarWidget.h"
#include "Mordecai/UI/MordecaiPostureBarWidget.h"
#include "Mordecai/UI/MordecaiSpellPointsBarWidget.h"
#include "Mordecai/UI/MordecaiStatusEffectBarWidget.h"
#include "Mordecai/UI/MordecaiSpellCooldownWidget.h"
#include "Mordecai/UI/MordecaiComboCounterWidget.h"
#include "Mordecai/UI/MordecaiCombatFeedbackWidget.h"
#include "Mordecai/UI/MordecaiKillCounterWidget.h"
#include "Mordecai/UI/MordecaiPickupPromptWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiCombatHUDWidget)

// ---------------------------------------------------------------------------
// Programmatic Fallback (empty Blueprint support)
// ---------------------------------------------------------------------------

void UMordecaiCombatHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultLayout();
	}
}

void UMordecaiCombatHUDWidget::BuildDefaultLayout()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	// Helper: add a child UUserWidget to the canvas at a specific position/size
	auto AddChild = [Root](UWidget* Child, FAnchors Anchors, FVector2D Position, FVector2D Size, FVector2D Alignment = FVector2D::ZeroVector)
	{
		if (!Child) return;
		UCanvasPanelSlot* Slot = Root->AddChildToCanvas(Child);
		Slot->SetAnchors(Anchors);
		Slot->SetPosition(Position);
		Slot->SetSize(Size);
		Slot->SetAutoSize(false);
		Slot->SetAlignment(Alignment);
	};

	const FAnchors TopLeft(0.f, 0.f, 0.f, 0.f);
	const FAnchors TopRight(1.f, 0.f, 1.f, 0.f);
	const FAnchors BottomCenter(0.5f, 1.f, 0.5f, 1.f);
	const FAnchors BottomLeft(0.f, 1.f, 0.f, 1.f);

	// --- Core Resource Bars (top-left cluster) ---
	if (!HealthBar)
	{
		HealthBar = CreateWidget<UMordecaiHealthBarWidget>(this);
	}
	AddChild(HealthBar, TopLeft, FVector2D(20, 20), FVector2D(250, 20));

	if (!StaminaBar)
	{
		StaminaBar = CreateWidget<UMordecaiStaminaBarWidget>(this);
	}
	AddChild(StaminaBar, TopLeft, FVector2D(20, 48), FVector2D(250, 20));

	if (!PostureBar)
	{
		PostureBar = CreateWidget<UMordecaiPostureBarWidget>(this);
	}
	AddChild(PostureBar, TopLeft, FVector2D(20, 76), FVector2D(250, 16));

	if (!SpellPointsBar)
	{
		SpellPointsBar = CreateWidget<UMordecaiSpellPointsBarWidget>(this);
	}
	AddChild(SpellPointsBar, TopLeft, FVector2D(20, 100), FVector2D(250, 16));

	// --- Spell Cooldowns ---
	if (!SpellCooldownA)
	{
		SpellCooldownA = CreateWidget<UMordecaiSpellCooldownWidget>(this);
	}
	AddChild(SpellCooldownA, TopLeft, FVector2D(20, 124), FVector2D(120, 16));

	if (!SpellCooldownB)
	{
		SpellCooldownB = CreateWidget<UMordecaiSpellCooldownWidget>(this);
	}
	AddChild(SpellCooldownB, TopLeft, FVector2D(150, 124), FVector2D(120, 16));

	// --- Kill Counter (top-right) ---
	if (!KillCounter)
	{
		KillCounter = CreateWidget<UMordecaiKillCounterWidget>(this);
	}
	AddChild(KillCounter, TopRight, FVector2D(-170, 20), FVector2D(150, 24));

	// --- Combat Feedback (center-bottom) ---
	if (!ComboCounter)
	{
		ComboCounter = CreateWidget<UMordecaiComboCounterWidget>(this);
	}
	AddChild(ComboCounter, BottomCenter, FVector2D(0, -120), FVector2D(200, 40), FVector2D(0.5f, 1.f));

	if (!CombatFeedback)
	{
		CombatFeedback = CreateWidget<UMordecaiCombatFeedbackWidget>(this);
	}
	AddChild(CombatFeedback, BottomCenter, FVector2D(0, -80), FVector2D(300, 30), FVector2D(0.5f, 1.f));

	// --- Status Effects (bottom-left) ---
	if (!StatusEffectBar)
	{
		StatusEffectBar = CreateWidget<UMordecaiStatusEffectBarWidget>(this);
	}
	AddChild(StatusEffectBar, BottomLeft, FVector2D(20, -60), FVector2D(400, 40));

	// --- Pickup / Inventory Assist Prompts (US-079, bottom-center above feedback) ---
	if (!PickupPrompt)
	{
		PickupPrompt = CreateWidget<UMordecaiPickupPromptWidget>(this);
	}
	AddChild(PickupPrompt, BottomCenter, FVector2D(0, -30), FVector2D(420, 56), FVector2D(0.5f, 1.f));
}

// ---------------------------------------------------------------------------
// Smoke Test Support
// ---------------------------------------------------------------------------

int32 UMordecaiCombatHUDWidget::GetBoundChildCount() const
{
	int32 Count = 0;
	if (HealthBar) Count++;
	if (StaminaBar) Count++;
	if (PostureBar) Count++;
	if (SpellPointsBar) Count++;
	if (StatusEffectBar) Count++;
	if (SpellCooldownA) Count++;
	if (SpellCooldownB) Count++;
	if (ComboCounter) Count++;
	if (CombatFeedback) Count++;
	if (KillCounter) Count++;
	return Count;
}

// ---------------------------------------------------------------------------
// ASC Binding
// ---------------------------------------------------------------------------

void UMordecaiCombatHUDWidget::BindToASC(UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return;
	}

	// Core resource bars (US-052)
	if (HealthBar)
	{
		HealthBar->BindToASC(ASC);
	}
	if (StaminaBar)
	{
		StaminaBar->BindToASC(ASC);
	}
	if (PostureBar)
	{
		PostureBar->BindToASC(ASC);
	}

	// Spell HUD (US-055)
	if (SpellPointsBar)
	{
		SpellPointsBar->BindToASC(ASC);
	}
	if (StatusEffectBar)
	{
		StatusEffectBar->BindToASC(ASC);
	}
	// Note: SpellCooldownA/B are bound individually via BindToCooldownTag()
	// from the player controller or experience setup, since each slot needs
	// its specific cooldown tag and duration.

	// Combat feedback (US-061)
	if (ComboCounter)
	{
		ComboCounter->BindToASC(ASC);
	}
	if (CombatFeedback)
	{
		CombatFeedback->BindToASC(ASC);
	}

	// Kill counter (US-063)
	if (KillCounter)
	{
		KillCounter->BindToASC(ASC);
	}
}
