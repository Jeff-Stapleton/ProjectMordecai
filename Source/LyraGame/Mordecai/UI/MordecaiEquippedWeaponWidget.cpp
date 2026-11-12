// Project Mordecai — Equipped Weapon Widget (US-077)

#include "Mordecai/UI/MordecaiEquippedWeaponWidget.h"
#include "Mordecai/Combat/MordecaiAttackProfileDataAsset.h"
#include "Mordecai/Weapons/MordecaiEquipmentComponent.h"
#include "Mordecai/Weapons/MordecaiWeaponDataAsset.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Mordecai/UI/TinTally/MordecaiUIPrimitives.h"
#include "Mordecai/UI/TinTally/MordecaiUITheme.h"
#include "Mordecai/UI/TinTally/MordecaiUIThemeBinding.h"
#include "Mordecai/UI/TinTally/MordecaiUITypography.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiEquippedWeaponWidget)

#define LOCTEXT_NAMESPACE "MordecaiEquippedWeapon"

void UMordecaiEquippedWeaponWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultContent();
	}
	SetDisplayFromWeapon(nullptr);
}

void UMordecaiEquippedWeaponWidget::NativeConstruct()
{
	Super::NativeConstruct();
	MordecaiUIThemeBinding::Bind(
		this, BoundThemeSubsystem, ThemeChangedHandle,
		this, &UMordecaiEquippedWeaponWidget::ApplyTheme);
}

void UMordecaiEquippedWeaponWidget::BuildDefaultContent()
{
	const FMordecaiUIThemeColors Colors = FMordecaiUIThemeColors::MakeDiorama();
	UOverlay* Root = FMordecaiUIPrimitiveBuilder::Build(
		WidgetTree,
		FMordecaiUIPrimitiveBuilder::Plate(
			Colors, EMordecaiUIPrintDepth::HUD, EMordecaiUITheme::Diorama),
		TEXT("LoadoutPlate"));
	WidgetTree->RootWidget = Root;
	UCanvasPanel* Content = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("LoadoutContent"));
	if (UOverlaySlot* ContentSlot = Root->AddChildToOverlay(Content))
	{
		ContentSlot->SetPadding(FMargin(0.f));
	}
	auto Place = [Content](UWidget* Widget, FVector2D Position, FVector2D Size, int32 ZOrder = 0)
	{
		if (UCanvasPanelSlot* Slot = Content->AddChildToCanvas(Widget))
		{
			Slot->SetPosition(Position);
			Slot->SetSize(Size);
			Slot->SetAutoSize(false);
			Slot->SetZOrder(ZOrder);
		}
	};

	UOverlay* WeaponWell = FMordecaiUIPrimitiveBuilder::Build(
		WidgetTree, FMordecaiUIPrimitiveBuilder::Well(Colors, false), TEXT("WeaponWell"));
	Place(WeaponWell, FVector2D(8.f, 10.f), FVector2D(64.f), 0);
	WeaponIcon = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(), TEXT("WeaponIcon"));
	WeaponIcon->SetColorAndOpacity(Colors.Text);
	WeaponIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
	Place(WeaponIcon, FVector2D(16.f, 18.f), FVector2D(48.f), 1);

	WeaponNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("WeaponNameText"));
	FSlateFontInfo NameFont = WeaponNameText->GetFont();
	NameFont.Size = 20;
	WeaponNameText->SetFont(NameFont);
	WeaponNameText->SetColorAndOpacity(FSlateColor(Colors.Text));
	WeaponNameText->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
	Place(WeaponNameText, FVector2D(84.f, 10.f), FVector2D(166.f, 32.f), 1);

	DamageSubtypeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DamageSubtypeText"));
	FSlateFontInfo SubtypeFont = DamageSubtypeText->GetFont();
	SubtypeFont.Size = 15;
	DamageSubtypeText->SetFont(SubtypeFont);
	DamageSubtypeText->SetColorAndOpacity(FSlateColor(Colors.TextMuted));
	Place(DamageSubtypeText, FVector2D(84.f, 46.f), FVector2D(132.f, 26.f), 1);

	ElementIcon = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(), TEXT("ElementIcon"));
	ElementIcon->SetColorAndOpacity(Colors.DamageFire);
	ElementIcon->SetVisibility(ESlateVisibility::Collapsed);
	Place(ElementIcon, FVector2D(220.f, 48.f), FVector2D(20.f), 1);

	UOverlay* SwapKeycap = FMordecaiUIPrimitiveBuilder::Build(
		WidgetTree,
		FMordecaiUIPrimitiveBuilder::Keycap(Colors, EMordecaiKeycapShape::Pill),
		TEXT("SwapKeycap"));
	Place(SwapKeycap, FVector2D(268.f, 22.f), FVector2D(62.f, 40.f), 1);
	SwapKey = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SwapKey"));
	FSlateFontInfo KeyFont = SwapKey->GetFont();
	KeyFont.Size = 17;
	SwapKey->SetFont(KeyFont);
	SwapKey->SetJustification(ETextJustify::Center);
	SwapKey->SetColorAndOpacity(FSlateColor(Colors.Ink));
	Place(SwapKey, FVector2D(268.f, 29.f), FVector2D(62.f, 24.f), 2);
}

void UMordecaiEquippedWeaponWidget::BindToEquipmentComponent(UMordecaiEquipmentComponent* Comp)
{
	UnbindFromEquipmentComponent();
	if (!Comp)
	{
		return;
	}

	BoundComponent = Comp;
	Comp->OnWeaponChanged.AddDynamic(this, &UMordecaiEquippedWeaponWidget::HandleWeaponChanged);
	RefreshFromComponent();
}

void UMordecaiEquippedWeaponWidget::UnbindFromEquipmentComponent()
{
	if (UMordecaiEquipmentComponent* Comp = BoundComponent.Get())
	{
		Comp->OnWeaponChanged.RemoveDynamic(this, &UMordecaiEquippedWeaponWidget::HandleWeaponChanged);
	}
	BoundComponent.Reset();
}

void UMordecaiEquippedWeaponWidget::NativeDestruct()
{
	UnbindFromEquipmentComponent();
	MordecaiUIThemeBinding::Unbind(BoundThemeSubsystem, ThemeChangedHandle);
	Super::NativeDestruct();
}

void UMordecaiEquippedWeaponWidget::HandleWeaponChanged(EMordecaiEquipSlot InSlot, const UMordecaiWeaponDataAsset* NewWeapon)
{
	// Only react to MainHand / TwoHand changes (the displayed weapon)
	if (InSlot != EMordecaiEquipSlot::MainHand && InSlot != EMordecaiEquipSlot::TwoHand)
	{
		return;
	}

	if (NewWeapon)
	{
		SetDisplayFromWeapon(NewWeapon);
	}
	else
	{
		// Unequipped — refresh from component in case another slot is still set
		RefreshFromComponent();
	}
}

void UMordecaiEquippedWeaponWidget::RefreshFromComponent()
{
	UMordecaiEquipmentComponent* Comp = BoundComponent.Get();
	if (!Comp)
	{
		SetDisplayFromWeapon(nullptr);
		return;
	}

	const FMordecaiWeaponInstance* Main = Comp->GetEquippedWeapon(EMordecaiEquipSlot::MainHand);
	SetDisplayFromWeapon(Main ? Main->WeaponDataAsset.Get() : nullptr);
}

void UMordecaiEquippedWeaponWidget::SetDisplayFromWeapon(const UMordecaiWeaponDataAsset* Weapon)
{
	if (Weapon)
	{
		DisplayedWeaponName = Weapon->DisplayName;
		DisplayedWeaponType = Weapon->WeaponType;
	}
	else
	{
		DisplayedWeaponName = LOCTEXT("UnarmedDisplay", "Unarmed");
		DisplayedWeaponType = EMordecaiWeaponType::Unarmed;
	}
	DisplayedDamageSubtype = ResolveDamageSubtype(Weapon);
	UpdateDisplay();
}

void UMordecaiEquippedWeaponWidget::SetGalleryPreviewWeapon(const UMordecaiWeaponDataAsset* Weapon)
{
	SetDisplayFromWeapon(Weapon);
}

FText UMordecaiEquippedWeaponWidget::ResolveDamageSubtype(const UMordecaiWeaponDataAsset* Weapon)
{
	if (!Weapon)
	{
		return LOCTEXT("BluntSubtype", "Blunt");
	}

	const UMordecaiAttackProfileDataAsset* Profile =
		Weapon->LightAttackProfiles.IsEmpty() ? Weapon->HeavyAttackProfile.Get() : Weapon->LightAttackProfiles[0].Get();
	if (Profile)
	{
		switch (Profile->DamageProfile.DamageType)
		{
		case EMordecaiDamageType::Slash:
			return LOCTEXT("SlashSubtype", "Slash");
		case EMordecaiDamageType::Pierce:
			return LOCTEXT("PierceSubtype", "Pierce");
		case EMordecaiDamageType::Blunt:
			return LOCTEXT("BluntSubtypeFromProfile", "Blunt");
		default:
			break;
		}
	}

	// Legacy profiles may only say Physical. Keep the plate useful from the weapon class until their subtype is set.
	switch (Weapon->WeaponType)
	{
	case EMordecaiWeaponType::Dagger:
	case EMordecaiWeaponType::Spear:
		return LOCTEXT("PierceSubtypeFallback", "Pierce");
	case EMordecaiWeaponType::Mace:
	case EMordecaiWeaponType::Quarterstaff:
	case EMordecaiWeaponType::Unarmed:
		return LOCTEXT("BluntSubtypeFallback", "Blunt");
	default:
		return LOCTEXT("SlashSubtypeFallback", "Slash");
	}
}

void UMordecaiEquippedWeaponWidget::UpdateDisplay()
{
	if (WeaponNameText)
	{
		WeaponNameText->SetText(DisplayedWeaponName);
	}
	if (DamageSubtypeText)
	{
		DamageSubtypeText->SetText(DisplayedDamageSubtype);
	}
	if (SwapKey)
	{
		SwapKey->SetText(LOCTEXT("SwapKey", "Tab"));
	}
	if (WeaponIcon)
	{
		const TCHAR* Glyph = TEXT("unarmed");
		switch (DisplayedWeaponType)
		{
		case EMordecaiWeaponType::Longsword: Glyph = TEXT("longsword"); break;
		case EMordecaiWeaponType::Dagger: Glyph = TEXT("dagger"); break;
		case EMordecaiWeaponType::Greatsword: Glyph = TEXT("greatsword"); break;
		case EMordecaiWeaponType::Spear: Glyph = TEXT("spear"); break;
		case EMordecaiWeaponType::Mace: Glyph = TEXT("mace"); break;
		case EMordecaiWeaponType::Shortsword: Glyph = TEXT("shortsword"); break;
		case EMordecaiWeaponType::Axe: Glyph = TEXT("axe"); break;
		case EMordecaiWeaponType::Quarterstaff: Glyph = TEXT("quarterstaff"); break;
		case EMordecaiWeaponType::Unarmed:
		default: break;
		}
		const FString Path = FString::Printf(
			TEXT("/MordecaiCore/UI/TinTally/Textures/Glyphs/T_UI_Glyph_%s.T_UI_Glyph_%s"),
			Glyph, Glyph);
		if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *Path))
		{
			WeaponIcon->SetBrushFromTexture(Texture, true);
		}
	}
}

void UMordecaiEquippedWeaponWidget::ApplyTheme(
	EMordecaiUITheme Theme,
	const FMordecaiUIThemeColors& Colors)
{
	CurrentTheme = Theme;
	CurrentThemeColors = Colors;
	if (UOverlay* Plate = WidgetTree
		? Cast<UOverlay>(WidgetTree->FindWidget(TEXT("LoadoutPlate")))
		: nullptr)
	{
		FMordecaiUIPrimitiveBuilder::Apply(
			Plate,
			FMordecaiUIPrimitiveBuilder::Plate(
				Colors, EMordecaiUIPrintDepth::HUD, Theme));
	}
	if (UOverlay* Well = WidgetTree
		? Cast<UOverlay>(WidgetTree->FindWidget(TEXT("WeaponWell")))
		: nullptr)
	{
		FMordecaiUIPrimitiveBuilder::Apply(
			Well, FMordecaiUIPrimitiveBuilder::Well(Colors, false));
	}
	if (UOverlay* Keycap = WidgetTree
		? Cast<UOverlay>(WidgetTree->FindWidget(TEXT("SwapKeycap")))
		: nullptr)
	{
		FMordecaiUIPrimitiveBuilder::Apply(
			Keycap,
			FMordecaiUIPrimitiveBuilder::Keycap(
				Colors, EMordecaiKeycapShape::Pill));
	}
	if (WeaponIcon)
	{
		WeaponIcon->SetColorAndOpacity(Colors.Text);
	}
	MordecaiUITypography::Apply(WeaponNameText, TEXT("ui_title"));
	MordecaiUITypography::Apply(DamageSubtypeText, TEXT("ui_label"));
	MordecaiUITypography::Apply(SwapKey, TEXT("num_s"));
	if (WeaponNameText) WeaponNameText->SetColorAndOpacity(FSlateColor(Colors.Text));
	if (DamageSubtypeText) DamageSubtypeText->SetColorAndOpacity(FSlateColor(Colors.TextMuted));
	if (SwapKey)
	{
		FSlateFontInfo Font = SwapKey->GetFont();
		Font.TypefaceFontName = TEXT("ExtraBold");
		// UMG uses points: the card's 22px Mono label is 16.5pt.
		Font.Size = 16.5f;
		SwapKey->SetFont(Font);
		SwapKey->SetColorAndOpacity(FSlateColor(Colors.Ink));
	}
}

#undef LOCTEXT_NAMESPACE
