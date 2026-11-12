// Project Mordecai — Combat HUD Root Widget (US-052, US-055, US-061)

#include "Mordecai/UI/MordecaiCombatHUDWidget.h"
#include "AbilitySystemComponent.h"
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
#include "Mordecai/UI/MordecaiEquippedWeaponWidget.h"
#include "Mordecai/Magic/MordecaiSpellDataAsset.h"
#include "Mordecai/Items/MordecaiPickupInteractionComponent.h"
#include "Mordecai/Weapons/MordecaiEquipmentComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Input/UIActionBindingHandle.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/UI/TinTally/MordecaiUIPrimitives.h"
#include "Mordecai/UI/TinTally/MordecaiUITheme.h"
#include "UI/IndicatorSystem/IndicatorLayer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiCombatHUDWidget)

// ---------------------------------------------------------------------------
// Programmatic Fallback (empty Blueprint support)
// ---------------------------------------------------------------------------

TOptional<FUIInputConfig> UMordecaiCombatHUDWidget::GetDesiredInputConfig() const
{
	// CommonUI leaves the active root's capture mode on the viewport. Keep the project default so
	// the first click after refocusing the game is delivered to gameplay instead of only capturing.
	return FUIInputConfig(
		ECommonInputMode::Game,
		EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);
}

void UMordecaiCombatHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultLayout();
	}
}

void UMordecaiCombatHUDWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	// UI.Layer.Game deactivates this root while the death screen is on top. Re-read every source when the root
	// returns so pooled/rebuilt child trees cannot show values from the life that ended.
	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		BindToASC(ASC);
	}
	if (UMordecaiEquipmentComponent* Equipment = BoundEquipment.Get())
	{
		BindToEquipmentComponent(Equipment);
	}
	if (UMordecaiPickupInteractionComponent* Interaction = BoundInteraction.Get())
	{
		BindToInteraction(Interaction);
	}
}

void UMordecaiCombatHUDWidget::NativeOnDeactivated()
{
	Super::NativeOnDeactivated();
	SetVisibility(ESlateVisibility::Collapsed);
}

void UMordecaiCombatHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	(void)InDeltaTime;
	UBorder* Rim = WidgetTree
		? Cast<UBorder>(WidgetTree->FindWidget(TEXT("GaugePlate_Rim")))
		: nullptr;
	if (!Rim)
	{
		return;
	}
	const FMordecaiUIThemeColors Colors = FMordecaiUIThemeColors::MakeDiorama();
	const bool bLowHealth = CachedHUDMaxHealth > 0.f
		&& CachedHUDHealth / CachedHUDMaxHealth <= 0.25f;
	const bool bVitalStep = bLowHealth && GetWorld()
		&& FMath::Fmod(GetWorld()->GetTimeSeconds(), 1.f) < 0.5f;
	Rim->SetBrushColor(bVitalStep ? Colors.Vital : Colors.Rim);
}

void UMordecaiCombatHUDWidget::BuildDefaultLayout()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	// IndicatorSystem projects descriptors through one full-screen layer. Keep it below ordinary HUD controls.
	if (!IndicatorLayer)
	{
		IndicatorLayer = WidgetTree->ConstructWidget<UIndicatorLayer>(
			UIndicatorLayer::StaticClass(), TEXT("IndicatorLayer"));
	}
	UCanvasPanelSlot* IndicatorSlot = Root->AddChildToCanvas(IndicatorLayer);
	IndicatorSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	IndicatorSlot->SetOffsets(FMargin(0.f));
	IndicatorSlot->SetZOrder(-1);

	// Helper: every final-map box is fixed-size in 1920x1080 design pixels.
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
	const FAnchors Center(0.5f, 0.5f, 0.5f, 0.5f);
	const FAnchors BottomCenter(0.5f, 1.f, 0.5f, 1.f);
	const FAnchors BottomRight(1.f, 1.f, 1.f, 1.f);
	const FMordecaiUIThemeColors Colors = FMordecaiUIThemeColors::MakeDiorama();

	// --- GaugePlate: one Tin Tally plate containing all four resource rows ---
	if (!GaugePlate)
	{
		GaugePlate = FMordecaiUIPrimitiveBuilder::Build(
			WidgetTree,
			FMordecaiUIPrimitiveBuilder::Plate(
				Colors, EMordecaiUIPrintDepth::HUD, EMordecaiUITheme::Diorama),
			TEXT("GaugePlate"));
	}
	UCanvasPanel* GaugeContent = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("GaugeContent"));
	if (UOverlaySlot* ContentSlot = GaugePlate->AddChildToOverlay(GaugeContent))
	{
		ContentSlot->SetPadding(FMargin(0.f));
	}

	auto AddGaugeRow = [GaugeContent](UWidget* Child, FVector2D Position, FVector2D Size)
	{
		if (UCanvasPanelSlot* Slot = GaugeContent->AddChildToCanvas(Child))
		{
			Slot->SetPosition(Position);
			Slot->SetSize(Size);
			Slot->SetAutoSize(false);
		}
	};

	if (!HealthBar)
	{
		HealthBar = CreateWidget<UMordecaiHealthBarWidget>(
			this, UMordecaiHealthBarWidget::StaticClass(), TEXT("HealthBar"));
	}
	AddGaugeRow(HealthBar, FVector2D(14.f, 14.f), FVector2D(424.f, 34.f));

	if (!StaminaBar)
	{
		StaminaBar = CreateWidget<UMordecaiStaminaBarWidget>(
			this, UMordecaiStaminaBarWidget::StaticClass(), TEXT("StaminaBar"));
	}
	AddGaugeRow(StaminaBar, FVector2D(14.f, 56.f), FVector2D(424.f, 28.f));

	if (!PostureBar)
	{
		PostureBar = CreateWidget<UMordecaiPostureBarWidget>(
			this, UMordecaiPostureBarWidget::StaticClass(), TEXT("PostureBar"));
	}
	AddGaugeRow(PostureBar, FVector2D(52.f, 91.f), FVector2D(268.f, 8.f));

	if (!SpellPointsBar)
	{
		SpellPointsBar = CreateWidget<UMordecaiSpellPointsBarWidget>(
			this, UMordecaiSpellPointsBarWidget::StaticClass(), TEXT("SpellPointsBar"));
	}
	AddGaugeRow(SpellPointsBar, FVector2D(14.f, 106.f), FVector2D(424.f, 28.f));
	AddChild(GaugePlate, TopLeft, FVector2D(32.f), FVector2D(456.f, 152.f));

	// --- Spell Cooldowns ---
	TObjectPtr<UMordecaiSpellCooldownWidget>* CooldownWidgets[] = {
		&SpellCooldownA,
		&SpellCooldownB,
		&SpellCooldownC,
		&SpellCooldownD,
	};
	const TCHAR* SlotKeyLabels[] = { TEXT("Q"), TEXT("E"), TEXT("R"), TEXT("1") };
	for (int32 SlotIndex = 0; SlotIndex < UE_ARRAY_COUNT(CooldownWidgets); ++SlotIndex)
	{
		TObjectPtr<UMordecaiSpellCooldownWidget>& CooldownWidget = *CooldownWidgets[SlotIndex];
		if (!CooldownWidget)
		{
			CooldownWidget = CreateWidget<UMordecaiSpellCooldownWidget>(
				this,
				UMordecaiSpellCooldownWidget::StaticClass(),
				FName(*FString::Printf(TEXT("SpellCooldown%c"), TCHAR('A' + SlotIndex))));
		}
		CooldownWidget->SetSlotKeyLabel(FText::FromString(SlotKeyLabels[SlotIndex]));
		AddChild(
			CooldownWidget,
			TopLeft,
			FVector2D(32.f + (88.f * SlotIndex), 216.f),
			FVector2D(78.f));
	}

	// --- Tally rail head (top-right) ---
	if (!TallyRailHead)
	{
		TallyRailHead = FMordecaiUIPrimitiveBuilder::Build(
			WidgetTree,
			FMordecaiUIPrimitiveBuilder::Plate(
				Colors, EMordecaiUIPrintDepth::HUD, EMordecaiUITheme::Diorama),
			TEXT("TallyRailHead"));
	}
	UCanvasPanel* RailContent = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("TallyRailHeadContent"));
	if (UOverlaySlot* ContentSlot = TallyRailHead->AddChildToOverlay(RailContent))
	{
		ContentSlot->SetPadding(FMargin(0.f));
	}
	if (!KillCounter)
	{
		KillCounter = CreateWidget<UMordecaiKillCounterWidget>(
			this, UMordecaiKillCounterWidget::StaticClass(), TEXT("KillCounter"));
	}
	if (UCanvasPanelSlot* RailSlot = RailContent->AddChildToCanvas(KillCounter))
	{
		RailSlot->SetPosition(FVector2D(14.f, 8.f));
		RailSlot->SetSize(FVector2D(292.f, 40.f));
		RailSlot->SetAutoSize(false);
	}
	AddChild(TallyRailHead, TopRight, FVector2D(-32.f, 32.f),
		FVector2D(320.f, 56.f), FVector2D(1.f, 0.f));

	// --- Centre callout lane ---
	if (!ComboCounter)
	{
		ComboCounter = CreateWidget<UMordecaiComboCounterWidget>(
			this, UMordecaiComboCounterWidget::StaticClass(), TEXT("ComboCounter"));
	}
	AddChild(ComboCounter, Center, FVector2D(0.f, 184.f),
		FVector2D(220.f, 36.f), FVector2D(0.5f, 0.f));

	if (!CombatFeedback)
	{
		CombatFeedback = CreateWidget<UMordecaiCombatFeedbackWidget>(
			this, UMordecaiCombatFeedbackWidget::StaticClass(), TEXT("CombatFeedback"));
	}
	AddChild(CombatFeedback, Center, FVector2D(0.f, 128.f),
		FVector2D(520.f, 52.f), FVector2D(0.5f, 0.f));

	// --- Status effects (left column) ---
	if (!StatusEffectBar)
	{
		StatusEffectBar = CreateWidget<UMordecaiStatusEffectBarWidget>(
			this, UMordecaiStatusEffectBarWidget::StaticClass(), TEXT("StatusEffectBar"));
	}
	AddChild(StatusEffectBar, TopLeft, FVector2D(32.f, 318.f), FVector2D(456.f, 112.f));

	// --- Bottom-centre prompt unit (540 prompt + 12 gap + 151 hint) ---
	if (!PickupPrompt)
	{
		PickupPrompt = CreateWidget<UMordecaiPickupPromptWidget>(
			this, UMordecaiPickupPromptWidget::StaticClass(), TEXT("PickupPrompt"));
	}
	AddChild(PickupPrompt, BottomCenter, FVector2D(90.f, -36.f),
		FVector2D(720.f, 60.f), FVector2D(0.5f, 1.f));

	// --- Loadout Plate (Tin Tally D20, bottom-right map box) ---
	if (!EquippedWeaponWidget)
	{
		EquippedWeaponWidget = CreateWidget<UMordecaiEquippedWeaponWidget>(
			this, UMordecaiEquippedWeaponWidget::StaticClass(), TEXT("EquippedWeaponWidget"));
	}
	AddChild(EquippedWeaponWidget, BottomRight, FVector2D(-32.f), FVector2D(340.f, 84.f), FVector2D(1.f));

	BuildLowHealthFrame(Root);
	SetHUDMargin(32.f);
}

void UMordecaiCombatHUDWidget::BuildLowHealthFrame(UCanvasPanel* Root)
{
	if (!Root || LowHealthFrame)
	{
		return;
	}
	const FMordecaiUIThemeColors Colors = FMordecaiUIThemeColors::MakeDiorama();
	LowHealthFrame = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("LowHealthFrame"));
	LowHealthFrame->SetVisibility(ESlateVisibility::Collapsed);
	LowHealthFrame->SetRenderOpacity(1.f);

	auto AddBand = [this, LowFrame = LowHealthFrame](
		const TCHAR* Name,
		const FLinearColor& Color,
		const FAnchors& Anchors,
		const FMargin& Offsets)
	{
		UBorder* Band = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName(Name));
		Band->SetBrushColor(Color);
		Band->SetPadding(FMargin(0.f));
		Band->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UCanvasPanelSlot* Slot = LowFrame->AddChildToCanvas(Band))
		{
			Slot->SetAnchors(Anchors);
			Slot->SetOffsets(Offsets);
		}
	};
	AddBand(TEXT("LowHealthTopVital"), Colors.Vital,
		FAnchors(0.f, 0.f, 1.f, 0.f), FMargin(0.f, 0.f, 0.f, 8.f));
	AddBand(TEXT("LowHealthTopInk"), Colors.Ink,
		FAnchors(0.f, 0.f, 1.f, 0.f), FMargin(0.f, 8.f, 0.f, 3.f));
	AddBand(TEXT("LowHealthBottomVital"), Colors.Vital,
		FAnchors(0.f, 1.f, 1.f, 1.f), FMargin(0.f, -8.f, 0.f, 8.f));
	AddBand(TEXT("LowHealthBottomInk"), Colors.Ink,
		FAnchors(0.f, 1.f, 1.f, 1.f), FMargin(0.f, -11.f, 0.f, 3.f));
	AddBand(TEXT("LowHealthLeftVital"), Colors.Vital,
		FAnchors(0.f, 0.f, 0.f, 1.f), FMargin(0.f, 0.f, 8.f, 0.f));
	AddBand(TEXT("LowHealthLeftInk"), Colors.Ink,
		FAnchors(0.f, 0.f, 0.f, 1.f), FMargin(8.f, 0.f, 3.f, 0.f));
	AddBand(TEXT("LowHealthRightVital"), Colors.Vital,
		FAnchors(1.f, 0.f, 1.f, 1.f), FMargin(-8.f, 0.f, 8.f, 0.f));
	AddBand(TEXT("LowHealthRightInk"), Colors.Ink,
		FAnchors(1.f, 0.f, 1.f, 1.f), FMargin(-11.f, 0.f, 3.f, 0.f));

	UCanvasPanelSlot* FrameSlot = Root->AddChildToCanvas(LowHealthFrame);
	FrameSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	FrameSlot->SetOffsets(FMargin(0.f));
	FrameSlot->SetZOrder(100);
}

// ---------------------------------------------------------------------------
// Smoke Test Support
// ---------------------------------------------------------------------------

int32 UMordecaiCombatHUDWidget::GetBoundChildCount() const
{
	int32 Count = 0;
	if (IndicatorLayer) Count++;
	if (GaugePlate) Count++;
	if (HealthBar) Count++;
	if (StaminaBar) Count++;
	if (PostureBar) Count++;
	if (SpellPointsBar) Count++;
	if (StatusEffectBar) Count++;
	if (SpellCooldownA) Count++;
	if (SpellCooldownB) Count++;
	if (SpellCooldownC) Count++;
	if (SpellCooldownD) Count++;
	if (ComboCounter) Count++;
	if (CombatFeedback) Count++;
	if (KillCounter) Count++;
	if (TallyRailHead) Count++;
	if (PickupPrompt) Count++;
	if (EquippedWeaponWidget) Count++;
	if (LowHealthFrame) Count++;
	return Count;
}

UWidget* UMordecaiCombatHUDWidget::GetNamedWidget(UUserWidget* Owner, FName WidgetName) const
{
	UUserWidget* SearchOwner = Owner ? Owner : const_cast<UMordecaiCombatHUDWidget*>(this);
	return SearchOwner && SearchOwner->WidgetTree
		? SearchOwner->WidgetTree->FindWidget(WidgetName)
		: nullptr;
}

void UMordecaiCombatHUDWidget::SetHUDMargin(float InMargin)
{
	HUDMargin = InMargin >= 64.f ? 96.f : 32.f;
	const float Shift = HUDMargin - 32.f;
	auto Move = [](UWidget* Widget, const FVector2D& Position)
	{
		if (UCanvasPanelSlot* Slot = Widget ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr)
		{
			Slot->SetPosition(Position);
		}
	};

	Move(GaugePlate, FVector2D(HUDMargin));
	UMordecaiSpellCooldownWidget* Cooldowns[] = {
		SpellCooldownA, SpellCooldownB, SpellCooldownC, SpellCooldownD
	};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Cooldowns); ++Index)
	{
		Move(Cooldowns[Index], FVector2D(HUDMargin + Index * 88.f, 216.f + Shift));
	}
	Move(StatusEffectBar, FVector2D(HUDMargin, 318.f + Shift));
	Move(TallyRailHead, FVector2D(-HUDMargin, HUDMargin));
	Move(PickupPrompt, FVector2D(90.f, -36.f - Shift));
	Move(EquippedWeaponWidget, FVector2D(-HUDMargin));
}

float UMordecaiCombatHUDWidget::GetTopCenterSafeWidth() const
{
	return HUDMargin >= 64.f ? 784.f : 880.f;
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
	if (UAbilitySystemComponent* Previous = BoundASC.Get())
	{
		Previous->GetGameplayAttributeValueChangeDelegate(
			UMordecaiAttributeSet::GetHealthAttribute()).RemoveAll(this);
		Previous->GetGameplayAttributeValueChangeDelegate(
			UMordecaiAttributeSet::GetMaxHealthAttribute()).RemoveAll(this);
	}
	BoundASC = ASC;
	ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetHealthAttribute())
		.AddUObject(this, &ThisClass::OnHUDHealthChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetMaxHealthAttribute())
		.AddUObject(this, &ThisClass::OnHUDMaxHealthChanged);
	bool bFound = false;
	CachedHUDHealth = ASC->GetGameplayAttributeValue(
		UMordecaiAttributeSet::GetHealthAttribute(), bFound);
	if (!bFound)
	{
		CachedHUDHealth = 0.f;
	}
	CachedHUDMaxHealth = ASC->GetGameplayAttributeValue(
		UMordecaiAttributeSet::GetMaxHealthAttribute(), bFound);
	if (!bFound)
	{
		CachedHUDMaxHealth = 0.f;
	}
	UpdateLowHealthFrame();

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
	// Note: SpellCooldownA-D are bound individually via BindToCooldownTag()
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

void UMordecaiCombatHUDWidget::BindSpellCooldownSlots(
	UAbilitySystemComponent* ASC,
	const TArray<TObjectPtr<UMordecaiSpellDataAsset>>& SpellDataAssets)
{
	auto BindSlot = [ASC, &SpellDataAssets](
		UMordecaiSpellCooldownWidget* Widget,
		const int32 SlotIndex)
	{
		if (!Widget)
		{
			return;
		}
		const UMordecaiSpellDataAsset* Data =
			SpellDataAssets.IsValidIndex(SlotIndex) ? SpellDataAssets[SlotIndex].Get() : nullptr;
		Widget->BindToCooldownTag(
			ASC,
			Data ? Data->CooldownTag : FGameplayTag(),
			Data ? Data->CooldownDuration : 0.f);
	};

	UMordecaiSpellCooldownWidget* CooldownWidgets[] = {
		SpellCooldownA,
		SpellCooldownB,
		SpellCooldownC,
		SpellCooldownD,
	};
	for (int32 SlotIndex = 0; SlotIndex < UE_ARRAY_COUNT(CooldownWidgets); ++SlotIndex)
	{
		BindSlot(CooldownWidgets[SlotIndex], SlotIndex);
	}
}

void UMordecaiCombatHUDWidget::BindToEquipmentComponent(UMordecaiEquipmentComponent* Equipment)
{
	BoundEquipment = Equipment;
	if (EquippedWeaponWidget)
	{
		EquippedWeaponWidget->BindToEquipmentComponent(Equipment);
	}
}

void UMordecaiCombatHUDWidget::BindToInteraction(UMordecaiPickupInteractionComponent* Interaction)
{
	BoundInteraction = Interaction;
	if (PickupPrompt)
	{
		PickupPrompt->BindToInteraction(Interaction);
	}
}

void UMordecaiCombatHUDWidget::OnHUDHealthChanged(const FOnAttributeChangeData& Data)
{
	CachedHUDHealth = Data.NewValue;
	UpdateLowHealthFrame();
}

void UMordecaiCombatHUDWidget::OnHUDMaxHealthChanged(const FOnAttributeChangeData& Data)
{
	CachedHUDMaxHealth = Data.NewValue;
	UpdateLowHealthFrame();
}

void UMordecaiCombatHUDWidget::UpdateLowHealthFrame()
{
	if (!LowHealthFrame)
	{
		return;
	}
	const float HealthRatio = CachedHUDMaxHealth > 0.f
		? CachedHUDHealth / CachedHUDMaxHealth
		: 1.f;
	LowHealthFrame->SetRenderOpacity(1.f);
	LowHealthFrame->SetVisibility(
		HealthRatio <= 0.25f
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
}
