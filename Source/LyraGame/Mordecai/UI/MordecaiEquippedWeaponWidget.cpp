// Project Mordecai — Equipped Weapon Widget (US-077)

#include "Mordecai/UI/MordecaiEquippedWeaponWidget.h"
#include "Mordecai/Combat/MordecaiAttackProfileDataAsset.h"
#include "Mordecai/Weapons/MordecaiEquipmentComponent.h"
#include "Mordecai/Weapons/MordecaiWeaponDataAsset.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

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

void UMordecaiEquippedWeaponWidget::BuildDefaultContent()
{
	UBorder* Root = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LoadoutPlate"));
	Root->SetVisibility(ESlateVisibility::HitTestInvisible);
	Root->SetPadding(FMargin(12.f, 8.f));
	Root->SetBrushColor(FLinearColor(0.025f, 0.035f, 0.045f, 0.88f));
	WidgetTree->RootWidget = Root;

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("LoadoutRow"));
	Root->SetContent(Row);

	UVerticalBox* TextColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TextColumn"));
	if (UHorizontalBoxSlot* ColumnSlot = Row->AddChildToHorizontalBox(TextColumn))
	{
		ColumnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ColumnSlot->SetVerticalAlignment(VAlign_Center);
	}

	WeaponNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("WeaponNameText"));
	FSlateFontInfo NameFont = WeaponNameText->GetFont();
	NameFont.Size = 24;
	WeaponNameText->SetFont(NameFont);
	if (UVerticalBoxSlot* NameSlot = TextColumn->AddChildToVerticalBox(WeaponNameText))
	{
		NameSlot->SetHorizontalAlignment(HAlign_Left);
	}

	DamageSubtypeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DamageSubtypeText"));
	FSlateFontInfo SubtypeFont = DamageSubtypeText->GetFont();
	SubtypeFont.Size = 18;
	DamageSubtypeText->SetFont(SubtypeFont);
	DamageSubtypeText->SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.75f, 0.78f, 1.f)));
	if (UVerticalBoxSlot* SubtypeSlot = TextColumn->AddChildToVerticalBox(DamageSubtypeText))
	{
		SubtypeSlot->SetHorizontalAlignment(HAlign_Left);
	}

	USpacer* Gap = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("KeyGap"));
	Gap->SetSize(FVector2D(12.f, 1.f));
	Row->AddChildToHorizontalBox(Gap);

	SwapKey = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SwapKey"));
	FSlateFontInfo KeyFont = SwapKey->GetFont();
	KeyFont.Size = 20;
	SwapKey->SetFont(KeyFont);
	SwapKey->SetJustification(ETextJustify::Center);
	if (UHorizontalBoxSlot* KeySlot = Row->AddChildToHorizontalBox(SwapKey))
	{
		KeySlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		KeySlot->SetVerticalAlignment(VAlign_Center);
		KeySlot->SetPadding(FMargin(8.f, 0.f));
	}
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
}

#undef LOCTEXT_NAMESPACE
