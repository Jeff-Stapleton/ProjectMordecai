// Project Mordecai — Enemy Indicator Widget (US-064)

#include "Mordecai/UI/MordecaiEnemyIndicatorWidget.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/Enemy/MordecaiEnemyCharacter.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "UI/IndicatorSystem/IndicatorDescriptor.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiEnemyIndicatorWidget)

// ---------------------------------------------------------------------------
// Static Color Constants (AC-064.8)
// ---------------------------------------------------------------------------

const FLinearColor UMordecaiEnemyIndicatorWidget::HealthColorGreen = FLinearColor(0.0f, 0.8f, 0.0f, 1.0f);
const FLinearColor UMordecaiEnemyIndicatorWidget::HealthColorYellow = FLinearColor(1.0f, 0.85f, 0.0f, 1.0f);
const FLinearColor UMordecaiEnemyIndicatorWidget::HealthColorRed = FLinearColor(0.9f, 0.1f, 0.1f, 1.0f);

// ---------------------------------------------------------------------------
// Initialization
// ---------------------------------------------------------------------------

void UMordecaiEnemyIndicatorWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!HealthProgressBar && WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultContent();
	}
}

void UMordecaiEnemyIndicatorWidget::BuildDefaultContent()
{
	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	// Name text
	NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NameText"));
	UVerticalBoxSlot* NameSlot = Root->AddChildToVerticalBox(NameText);
	NameSlot->SetHorizontalAlignment(HAlign_Center);
	NameText->SetText(EnemyDisplayName);

	// Health progress bar
	HealthProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HealthProgressBar"));
	UVerticalBoxSlot* BarSlot = Root->AddChildToVerticalBox(HealthProgressBar);
	BarSlot->SetHorizontalAlignment(HAlign_Fill);
	HealthProgressBar->SetPercent(1.0f);
	HealthProgressBar->SetFillColorAndOpacity(HealthColorGreen);

	// Status icon container
	StatusIconContainer = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("StatusIconContainer"));
	UVerticalBoxSlot* IconSlot = Root->AddChildToVerticalBox(StatusIconContainer);
	IconSlot->SetHorizontalAlignment(HAlign_Center);
}

// ---------------------------------------------------------------------------
// IIndicatorWidgetInterface (AC-064.5, AC-064.6)
// ---------------------------------------------------------------------------

void UMordecaiEnemyIndicatorWidget::BindIndicator_Implementation(UIndicatorDescriptor* Indicator)
{
	if (!Indicator)
	{
		return;
	}

	UObject* DataObj = Indicator->GetDataObject();
	if (!DataObj)
	{
		return;
	}

	// Get the enemy's ASC
	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(DataObj))
	{
		if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
		{
			BindToASC(ASC);
		}
	}

	// Set display name from enemy character
	if (AMordecaiEnemyCharacter* Enemy = Cast<AMordecaiEnemyCharacter>(DataObj))
	{
		if (NameText)
		{
			NameText->SetText(EnemyDisplayName.IsEmpty() ? FText::FromString(Enemy->GetName()) : EnemyDisplayName);
		}
	}
}

void UMordecaiEnemyIndicatorWidget::UnbindIndicator_Implementation(const UIndicatorDescriptor* Indicator)
{
	UnbindFromASC();
}

// ---------------------------------------------------------------------------
// ASC Binding (AC-064.6)
// ---------------------------------------------------------------------------

void UMordecaiEnemyIndicatorWidget::BindToASC(UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return;
	}

	BoundASC = ASC;

	// Bind health attribute changes
	ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetHealthAttribute())
		.AddUObject(this, &UMordecaiEnemyIndicatorWidget::OnHealthChanged);

	ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetMaxHealthAttribute())
		.AddUObject(this, &UMordecaiEnemyIndicatorWidget::OnMaxHealthChanged);

	// Initialize from current values
	bool bFound = false;
	CachedHealth = ASC->GetGameplayAttributeValue(UMordecaiAttributeSet::GetHealthAttribute(), bFound);
	if (!bFound) { CachedHealth = 0.f; }

	CachedMaxHealth = ASC->GetGameplayAttributeValue(UMordecaiAttributeSet::GetMaxHealthAttribute(), bFound);
	if (!bFound) { CachedMaxHealth = 0.f; }

	UpdateHealthDisplay();

	// Register for status tag changes (AC-064.7)
	// Listen for all Mordecai.Status.* tags
	static const FGameplayTag StatusParentTag = FGameplayTag::RequestGameplayTag(FName("Mordecai.Status"));
	FGameplayTagContainer StatusTags;
	StatusTags.AddTag(StatusParentTag);

	// Register tag events for known status tags
	const FGameplayTag KnownStatusTags[] = {
		MordecaiGameplayTags::Status_Burning,
		MordecaiGameplayTags::Status_Bleeding,
		MordecaiGameplayTags::Status_Poisoned,
		MordecaiGameplayTags::Status_Frostbitten,
		MordecaiGameplayTags::Status_Shocked,
		MordecaiGameplayTags::Status_Weakened,
		MordecaiGameplayTags::Status_Brittle,
		MordecaiGameplayTags::Status_Silenced,
		MordecaiGameplayTags::Status_Rooted,
		MordecaiGameplayTags::Status_Blinded,
		MordecaiGameplayTags::Status_Fear,
		MordecaiGameplayTags::Status_Cursed,
		MordecaiGameplayTags::Status_Exposed,
		MordecaiGameplayTags::Status_Corroded,
		MordecaiGameplayTags::Status_Drenched,
		MordecaiGameplayTags::Status_Focused,
	};

	for (const FGameplayTag& Tag : KnownStatusTags)
	{
		ASC->RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &UMordecaiEnemyIndicatorWidget::OnStatusTagChanged);

		// Add icon if already active
		if (ASC->HasMatchingGameplayTag(Tag))
		{
			AddStatusIcon(Tag);
		}
	}
}

void UMordecaiEnemyIndicatorWidget::UnbindFromASC()
{
	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetHealthAttribute())
			.RemoveAll(this);
		ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetMaxHealthAttribute())
			.RemoveAll(this);

		// Unregister tag events
		const FGameplayTag KnownStatusTags[] = {
			MordecaiGameplayTags::Status_Burning,
			MordecaiGameplayTags::Status_Bleeding,
			MordecaiGameplayTags::Status_Poisoned,
			MordecaiGameplayTags::Status_Frostbitten,
			MordecaiGameplayTags::Status_Shocked,
			MordecaiGameplayTags::Status_Weakened,
			MordecaiGameplayTags::Status_Brittle,
			MordecaiGameplayTags::Status_Silenced,
			MordecaiGameplayTags::Status_Rooted,
			MordecaiGameplayTags::Status_Blinded,
			MordecaiGameplayTags::Status_Fear,
			MordecaiGameplayTags::Status_Cursed,
			MordecaiGameplayTags::Status_Exposed,
			MordecaiGameplayTags::Status_Corroded,
			MordecaiGameplayTags::Status_Drenched,
			MordecaiGameplayTags::Status_Focused,
		};

		for (const FGameplayTag& Tag : KnownStatusTags)
		{
			ASC->RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved)
				.RemoveAll(this);
		}
	}

	BoundASC.Reset();
	ActiveStatusIcons.Empty();
}

// ---------------------------------------------------------------------------
// Health Display (AC-064.6, AC-064.8)
// ---------------------------------------------------------------------------

void UMordecaiEnemyIndicatorWidget::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	CachedHealth = Data.NewValue;
	UpdateHealthDisplay();
}

void UMordecaiEnemyIndicatorWidget::OnMaxHealthChanged(const FOnAttributeChangeData& Data)
{
	CachedMaxHealth = Data.NewValue;
	UpdateHealthDisplay();
}

void UMordecaiEnemyIndicatorWidget::UpdateHealthDisplay()
{
	const float Percent = (CachedMaxHealth > 0.f) ? FMath::Clamp(CachedHealth / CachedMaxHealth, 0.f, 1.f) : 0.f;

	if (HealthProgressBar)
	{
		HealthProgressBar->SetPercent(Percent);
		HealthProgressBar->SetFillColorAndOpacity(CalcHealthBarColor(Percent));
	}
}

FLinearColor UMordecaiEnemyIndicatorWidget::CalcHealthBarColor(float HealthPercent)
{
	if (HealthPercent > 0.5f)
	{
		return HealthColorGreen;
	}
	else if (HealthPercent >= 0.25f)
	{
		return HealthColorYellow;
	}
	else
	{
		return HealthColorRed;
	}
}

// ---------------------------------------------------------------------------
// Status Icons (AC-064.7)
// ---------------------------------------------------------------------------

void UMordecaiEnemyIndicatorWidget::OnStatusTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		AddStatusIcon(Tag);
	}
	else
	{
		RemoveStatusIcon(Tag);
	}
}

void UMordecaiEnemyIndicatorWidget::AddStatusIcon(const FGameplayTag& StatusTag)
{
	if (ActiveStatusIcons.Contains(StatusTag))
	{
		return;
	}

	if (!StatusIconContainer || !WidgetTree)
	{
		return;
	}

	UImage* IconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),
		*FString::Printf(TEXT("StatusIcon_%s"), *StatusTag.GetTagName().ToString()));

	// Try to load the texture from the map
	if (const TSoftObjectPtr<UTexture2D>* TexturePtr = StatusIconMap.Find(StatusTag))
	{
		if (UTexture2D* Texture = TexturePtr->LoadSynchronous())
		{
			IconImage->SetBrushFromTexture(Texture);
		}
	}

	UHorizontalBoxSlot* IconSlot = StatusIconContainer->AddChildToHorizontalBox(IconImage);
	IconSlot->SetHorizontalAlignment(HAlign_Center);
	IconSlot->SetVerticalAlignment(VAlign_Center);

	// Set a small fixed size for icons
	IconImage->SetDesiredSizeOverride(FVector2D(16.0, 16.0));

	ActiveStatusIcons.Add(StatusTag, IconImage);
}

void UMordecaiEnemyIndicatorWidget::RemoveStatusIcon(const FGameplayTag& StatusTag)
{
	TObjectPtr<UImage>* Found = ActiveStatusIcons.Find(StatusTag);
	if (!Found)
	{
		return;
	}

	if (StatusIconContainer && *Found)
	{
		StatusIconContainer->RemoveChild(*Found);
	}

	ActiveStatusIcons.Remove(StatusTag);
}
