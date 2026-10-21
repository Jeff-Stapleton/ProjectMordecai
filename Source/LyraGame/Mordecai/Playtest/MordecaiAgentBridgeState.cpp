// Project Mordecai — Agent bridge state gathering (Specs/agent_bridge_v1.md §4 state)

#include "Mordecai/Playtest/MordecaiAgentBridgeState.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "CommonActivatableWidget.h"
#include "CommonInputSubsystem.h"
#include "CommonInputTypeEnum.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/WidgetComponent.h"
#include "EngineUtils.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/SkinnedAsset.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "GameFramework/PlayerState.h"
#include "GameplayEffect.h"
#include "Input/CommonUIActionRouterBase.h"
#include "InputMappingContext.h"
#include "LoadingScreenManager.h"
#include "Misc/App.h"
#include "UObject/UObjectIterator.h"
#include "UnrealClient.h"
#include "Widgets/SViewport.h"
#include "Widgets/SWindow.h"

#include "Character/LyraHealthComponent.h"
#include "GameModes/LyraExperienceDefinition.h"
#include "GameModes/LyraExperienceManagerComponent.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/Combat/MordecaiProjectile.h"
#include "Mordecai/Enemy/MordecaiEnemyAIController.h"
#include "Mordecai/Enemy/MordecaiEnemyAITypes.h"
#include "Mordecai/Enemy/MordecaiEnemyCharacter.h"
#include "Mordecai/Items/MordecaiInventoryComponent.h"
#include "Mordecai/Items/MordecaiItemDefinition.h"
#include "Mordecai/Items/MordecaiItemLibrary.h"
#include "Mordecai/Items/MordecaiItemPickup.h"
#include "Mordecai/Items/MordecaiPickupInteractionComponent.h"
#include "Mordecai/Items/MordecaiResourceLedger.h"
#include "Mordecai/Magic/MordecaiGA_SpellBase.h"
#include "Mordecai/Magic/MordecaiIllusionActor.h"
#include "Mordecai/Magic/MordecaiSpellDataAsset.h"
#include "Mordecai/MordecaiCharacter.h"
#include "Mordecai/MordecaiGameMode.h"
#include "Mordecai/MordecaiPlayerController.h"
#include "Mordecai/Playtest/MordecaiAgentBridgeProtocol.h"
#include "Mordecai/UI/MordecaiCombatHUDWidget.h"
#include "Mordecai/UI/MordecaiInventoryWidget.h"
#include "Mordecai/UI/MordecaiPauseMenuSubsystem.h"
#include "Mordecai/UI/MordecaiPauseMenuWidget.h"
#include "Mordecai/UI/MordecaiPickupPromptWidget.h"
#include "Mordecai/UI/MordecaiTabButton.h"
#include "Mordecai/Weapons/MordecaiEquipmentComponent.h"
#include "Mordecai/Weapons/MordecaiWeaponDataAsset.h"

// ---------------------------------------------------------------------------
// Context and viewport transform
// ---------------------------------------------------------------------------

FMordecaiAgentBridgeStateContext FMordecaiAgentBridgeStateContext::FromGameInstance(UGameInstance* InGameInstance)
{
	FMordecaiAgentBridgeStateContext Context;
	Context.GameInstance = InGameInstance;
	if (!InGameInstance)
	{
		return Context;
	}
	Context.World = InGameInstance->GetWorld();
	Context.ViewportClient = InGameInstance->GetGameViewportClient();
	Context.PlayerController = Context.World ? InGameInstance->GetFirstLocalPlayerController(Context.World) : nullptr;
	Context.Pawn = Context.PlayerController ? Context.PlayerController->GetPawn() : nullptr;
	return Context;
}

FVector2D FMordecaiAgentBridgeViewportTransform::ToViewport(const FVector2D& Absolute) const
{
	return MordecaiAgentBridge::AbsoluteToViewport(Absolute, AbsolutePosition, AbsoluteSize, ViewportSize);
}

FVector2D FMordecaiAgentBridgeViewportTransform::ToAbsolute(const FVector2D& Pixel) const
{
	return MordecaiAgentBridge::ViewportToAbsolute(Pixel, AbsolutePosition, AbsoluteSize, ViewportSize);
}

FMordecaiAgentBridgeViewportTransform FMordecaiAgentBridgeViewportTransform::FromViewportClient(const UGameViewportClient* ViewportClient)
{
	FMordecaiAgentBridgeViewportTransform Transform;
	if (!ViewportClient || !ViewportClient->Viewport)
	{
		return Transform;
	}
	const TSharedPtr<SViewport> ViewportWidget = ViewportClient->GetGameViewportWidget();
	if (!ViewportWidget.IsValid())
	{
		return Transform;
	}
	const FGeometry& Geometry = ViewportWidget->GetTickSpaceGeometry();
	Transform.AbsolutePosition = FVector2D(Geometry.GetAbsolutePosition());
	Transform.AbsoluteSize = FVector2D(Geometry.GetAbsoluteSize());
	Transform.ViewportSize = ViewportClient->Viewport->GetSizeXY();
	Transform.bValid = Transform.AbsoluteSize.X > 0.0 && Transform.AbsoluteSize.Y > 0.0
		&& Transform.ViewportSize.X > 0 && Transform.ViewportSize.Y > 0;
	return Transform;
}

namespace MordecaiAgentBridge::State
{
	namespace StatePrivate
	{
		using MordecaiAgentBridge::MakeNumber;
		using MordecaiAgentBridge::MakeVector;

		static TSharedRef<FJsonValue> NullValue()
		{
			return MakeShared<FJsonValueNull>();
		}

		static TSharedRef<FJsonValue> StringValue(const FString& Value)
		{
			return MakeShared<FJsonValueString>(Value);
		}

		static TSharedRef<FJsonValue> BoolValue(bool bValue)
		{
			return MakeShared<FJsonValueBoolean>(bValue);
		}

		static TSharedRef<FJsonValue> ObjectValue(const TSharedRef<FJsonObject>& Object)
		{
			return MakeShared<FJsonValueObject>(Object);
		}

		template <typename TEnum>
		static FString EnumName(TEnum Value)
		{
			const UEnum* Enum = StaticEnum<TEnum>();
			return Enum ? Enum->GetNameStringByValue(static_cast<int64>(Value)) : FString::FromInt(static_cast<int32>(Value));
		}

		static TArray<TSharedPtr<FJsonValue>> TagsToJson(const FGameplayTagContainer& Tags)
		{
			TArray<FString> Names;
			for (const FGameplayTag& Tag : Tags)
			{
				Names.Add(Tag.ToString());
			}
			Names.Sort();
			TArray<TSharedPtr<FJsonValue>> Out;
			for (const FString& Name : Names)
			{
				Out.Add(StringValue(Name));
			}
			return Out;
		}

		static UAbilitySystemComponent* FindASC(const AActor* Actor)
		{
			return Actor ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor) : nullptr;
		}

		/** Viewport-pixel center of a widget's last painted geometry, or null before it has painted. */
		static TSharedRef<FJsonValue> WidgetCenter(const UWidget* Widget, const FMordecaiAgentBridgeViewportTransform& Transform)
		{
			return Widget ? GeometryCenter(Widget->GetCachedGeometry(), Transform) : NullValue();
		}

		static TSharedRef<FJsonValue> WidgetRect(const UWidget* Widget, const FMordecaiAgentBridgeViewportTransform& Transform)
		{
			return Widget ? GeometryRect(Widget->GetCachedGeometry(), Transform) : NullValue();
		}

		/** First text block among a panel/content widget's direct children (button labels, row names). */
		static const UTextBlock* FirstTextChild(const UWidget* Widget)
		{
			const UPanelWidget* Panel = Cast<UPanelWidget>(Widget);
			if (!Panel)
			{
				return nullptr;
			}
			for (int32 Index = 0; Index < Panel->GetChildrenCount(); ++Index)
			{
				if (const UTextBlock* Text = Cast<UTextBlock>(Panel->GetChildAt(Index)))
				{
					return Text;
				}
			}
			return nullptr;
		}

		static const TCHAR* SpellKeyForSlot(int32 Slot)
		{
			// UMordecaiHeroComponent::EnsureMordecaiInputBound maps spell slots 0-3 to Q / E / R / One
			static const TCHAR* Keys[] = { TEXT("Q"), TEXT("E"), TEXT("R"), TEXT("One") };
			return Slot >= 0 && Slot < UE_ARRAY_COUNT(Keys) ? Keys[Slot] : nullptr;
		}

		/** The keys the runtime Mordecai IMC binds, for input.pressed. */
		static const TArray<FKey>& LiveMordecaiKeys()
		{
			static const TArray<FKey> Keys = {
				EKeys::W, EKeys::A, EKeys::S, EKeys::D, EKeys::F, EKeys::I, EKeys::Escape, EKeys::Q, EKeys::E, EKeys::R,
				EKeys::One, EKeys::Tab, EKeys::Backslash, EKeys::SpaceBar, EKeys::LeftShift, EKeys::LeftMouseButton,
				EKeys::RightMouseButton, EKeys::Gamepad_FaceButton_Left, EKeys::Gamepad_FaceButton_Top, EKeys::Gamepad_Special_Right,
				EKeys::Gamepad_LeftShoulder, EKeys::Gamepad_RightShoulder, EKeys::Gamepad_DPad_Up,
			};
			return Keys;
		}

		static ULocalPlayer* FindLocalPlayer(const FMordecaiAgentBridgeStateContext& Context)
		{
			if (Context.PlayerController && Context.PlayerController->GetLocalPlayer())
			{
				return Context.PlayerController->GetLocalPlayer();
			}
			return Context.GameInstance ? Context.GameInstance->GetFirstGamePlayer() : nullptr;
		}

		// --- Sections ---

		static TSharedRef<FJsonObject> GatherGame(const FMordecaiAgentBridgeStateContext& Context)
		{
			if (!Context.World)
			{
				return MakeInvalidSection(TEXT("no world yet"));
			}
			const FMordecaiAgentBridgeReadiness Ready = GetReadiness(Context);
			const TSharedRef<FJsonObject> Game = MakeShared<FJsonObject>();
			Game->SetBoolField(TEXT("valid"), true);
			Game->SetStringField(TEXT("map"), Ready.Map);
			Game->SetField(TEXT("experience"), MakeStringOrNull(Ready.Experience, Ready.Experience.IsEmpty()));
			Game->SetBoolField(TEXT("experience_loaded"), Ready.bExperienceLoaded);
			Game->SetField(TEXT("loading_screen"), Ready.bLoadingScreenKnown ? BoolValue(Ready.bLoadingScreen) : NullValue());
			Game->SetBoolField(TEXT("ready"), Ready.IsReady());
			Game->SetBoolField(TEXT("paused"), Ready.bPaused);
			Game->SetField(TEXT("real_time_s"), MakeNumber(FPlatformTime::Seconds() - GStartTime));
			Game->SetField(TEXT("world_time_s"), MakeNumber(Context.World->GetTimeSeconds()));
			const double DeltaTime = FApp::GetDeltaTime();
			Game->SetField(TEXT("fps"), DeltaTime > 0.0 ? MakeNumber(1.0 / DeltaTime, 1) : NullValue());
			if (Context.ViewportClient && Context.ViewportClient->Viewport)
			{
				const FIntPoint Size = Context.ViewportClient->Viewport->GetSizeXY();
				Game->SetField(TEXT("viewport_size"), MakeShared<FJsonValueArray>(TArray<TSharedPtr<FJsonValue>>{ MakeNumber(Size.X), MakeNumber(Size.Y) }));
			}
			else
			{
				Game->SetField(TEXT("viewport_size"), NullValue());
			}
			return Game;
		}

		static TSharedRef<FJsonObject> GatherFocus(const FMordecaiAgentBridgeStateContext& Context)
		{
			if (!Context.GameInstance)
			{
				return MakeInvalidSection(TEXT("no game instance"));
			}
			if (!FSlateApplication::IsInitialized())
			{
				return MakeInvalidSection(TEXT("Slate is not initialized"));
			}
			FSlateApplication& Slate = FSlateApplication::Get();
			const TSharedRef<FJsonObject> Focus = MakeShared<FJsonObject>();
			Focus->SetBoolField(TEXT("valid"), true);
			Focus->SetBoolField(TEXT("app_active"), Slate.IsActive());
			Focus->SetBoolField(TEXT("app_has_focus"), FApp::HasFocus());
			Focus->SetField(TEXT("viewport_has_focus"), Context.ViewportClient && Context.ViewportClient->Viewport
				? BoolValue(Context.ViewportClient->Viewport->HasFocus()) : NullValue());
			const TSharedPtr<SWidget> Focused = Slate.GetUserFocusedWidget(0);
			Focus->SetField(TEXT("focused_widget"), Focused.IsValid() ? StringValue(Focused->GetTypeAsString()) : NullValue());

			ULocalPlayer* LocalPlayer = FindLocalPlayer(Context);
			const UCommonUIActionRouterBase* Router = LocalPlayer ? LocalPlayer->GetSubsystem<UCommonUIActionRouterBase>() : nullptr;
			Focus->SetField(TEXT("commonui_input_mode"), Router ? StringValue(LexToString(Router->GetActiveInputMode())) : NullValue());
			Focus->SetField(TEXT("game_input_allowed"), Router ? BoolValue(Router->CanProcessNormalGameInput()) : NullValue());
			const UCommonInputSubsystem* CommonInput = LocalPlayer ? UCommonInputSubsystem::Get(LocalPlayer) : nullptr;
			Focus->SetField(TEXT("input_type"), CommonInput ? StringValue(EnumName(CommonInput->GetCurrentInputType())) : NullValue());

			Focus->SetBoolField(TEXT("virtual_cursor"), Context.bVirtualCursor);
			const FMordecaiAgentBridgeViewportTransform Transform = FMordecaiAgentBridgeViewportTransform::FromViewportClient(Context.ViewportClient);
			const FVector2D CursorAbs = Slate.GetCursorPos();
			const bool bCursorKnown = Transform.bValid && FMath::Abs(CursorAbs.X) < 1.0e8 && FMath::Abs(CursorAbs.Y) < 1.0e8;
			Focus->SetField(TEXT("cursor"), bCursorKnown ? MakeVector2D(Transform.ToViewport(CursorAbs)) : NullValue());
			Focus->SetField(TEXT("mouse_cursor_shown"), Context.PlayerController
				? BoolValue(Context.PlayerController->ShouldShowMouseCursor()) : NullValue());
			return Focus;
		}

		static TSharedRef<FJsonValue> GatherWeapon(const AMordecaiCharacter* Hero)
		{
			const UMordecaiEquipmentComponent* Equipment = Hero ? Hero->GetEquipmentComponent() : nullptr;
			const FMordecaiWeaponInstance* MainHand = Equipment ? Equipment->GetEquippedWeapon(EMordecaiEquipSlot::MainHand) : nullptr;
			const UMordecaiWeaponDataAsset* Weapon = MainHand ? MainHand->WeaponDataAsset.Get() : nullptr;
			if (!Weapon)
			{
				return NullValue();
			}
			const TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
			Entry->SetStringField(TEXT("id"), Weapon->WeaponId.ToString());
			Entry->SetStringField(TEXT("display_name"), Weapon->DisplayName.ToString());
			Entry->SetStringField(TEXT("type"), EnumName(Weapon->WeaponType));
			Entry->SetField(TEXT("index"), MakeNumber(Equipment->GetCurrentWeaponIndex()));
			Entry->SetField(TEXT("count"), MakeNumber(Equipment->GetAvailableWeaponCount()));
			return MakeShared<FJsonValueObject>(Entry);
		}

		static TArray<TSharedPtr<FJsonValue>> GatherSpells(const AMordecaiPlayerController* Controller, UAbilitySystemComponent* ASC, const UMordecaiAttributeSet* Attributes)
		{
			TArray<TSharedPtr<FJsonValue>> Spells;
			if (!Controller)
			{
				return Spells;
			}
			for (int32 Slot = 0; Slot < Controller->DefaultSpellAbilities.Num(); ++Slot)
			{
				const TSubclassOf<UGameplayAbility> AbilityClass = Controller->GetSpellAbilityClass(Slot);
				FGameplayAbilitySpec* Spec = (ASC && AbilityClass) ? ASC->FindAbilitySpecFromClass(AbilityClass) : nullptr;
				const UMordecaiGA_SpellBase* Instance = Spec ? Cast<UMordecaiGA_SpellBase>(Spec->GetPrimaryInstance()) : nullptr;
				// The spell data lives on the granted instance (or the spec's source object), never on the CDO
				const UMordecaiSpellDataAsset* Data = (Instance && Instance->SpellData) ? Instance->SpellData.Get()
					: (Spec ? Cast<UMordecaiSpellDataAsset>(Spec->SourceObject.Get()) : nullptr);

				const TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
				Entry->SetField(TEXT("slot"), MakeNumber(Slot));
				const TCHAR* Key = SpellKeyForSlot(Slot);
				Entry->SetField(TEXT("key"), Key ? StringValue(Key) : NullValue());
				Entry->SetField(TEXT("ability"), AbilityClass ? StringValue(AbilityClass->GetName()) : NullValue());
				Entry->SetBoolField(TEXT("granted"), Spec != nullptr);
				Entry->SetField(TEXT("spell_id"), Data ? StringValue(Data->SpellId.ToString()) : NullValue());
				Entry->SetField(TEXT("display_name"), Data ? StringValue(Data->DisplayName.ToString()) : NullValue());
				Entry->SetField(TEXT("sp_cost"), Data ? MakeNumber(Data->SpellPointCost) : NullValue());
				Entry->SetField(TEXT("cooldown_tag"), Data ? StringValue(Data->CooldownTag.IsValid() ? Data->CooldownTag.ToString() : FString()) : NullValue());

				if (Data)
				{
					float Remaining = 0.f;
					if (ASC && Data->CooldownTag.IsValid())
					{
						const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(Data->CooldownTag));
						for (const TPair<float, float>& TimeAndDuration : ASC->GetActiveEffectsTimeRemainingAndDuration(Query))
						{
							Remaining = FMath::Max(Remaining, TimeAndDuration.Key);
						}
					}
					Entry->SetField(TEXT("cooldown_remaining_s"), MakeNumber(Remaining));
				}
				else
				{
					Entry->SetField(TEXT("cooldown_remaining_s"), NullValue());
				}
				Entry->SetBoolField(TEXT("active"), Spec ? Spec->IsActive() : false);
				Entry->SetField(TEXT("can_afford"), (Data && Attributes)
					? BoolValue(Attributes->GetSpellPoints() >= Data->SpellPointCost) : NullValue());
				Spells.Add(MakeShared<FJsonValueObject>(Entry));
			}
			return Spells;
		}

		static TSharedRef<FJsonObject> GatherPlayer(const FMordecaiAgentBridgeStateContext& Context)
		{
			if (!Context.PlayerController)
			{
				return MakeInvalidSection(TEXT("no local player controller yet"));
			}
			if (!Context.Pawn)
			{
				return MakeInvalidSection(TEXT("no pawn yet"));
			}
			APawn* Pawn = Context.Pawn;
			const AMordecaiCharacter* Hero = Cast<AMordecaiCharacter>(Pawn);
			const ACharacter* Character = Cast<ACharacter>(Pawn);
			APlayerState* PlayerState = Context.PlayerController->PlayerState;
			UAbilitySystemComponent* ASC = FindASC(PlayerState);
			const UMordecaiAttributeSet* Attributes = ASC ? ASC->GetSet<UMordecaiAttributeSet>() : nullptr;

			const TSharedRef<FJsonObject> Player = MakeShared<FJsonObject>();
			Player->SetBoolField(TEXT("valid"), true);
			Player->SetStringField(TEXT("name"), Pawn->GetName());
			Player->SetStringField(TEXT("class"), Pawn->GetClass()->GetName());
			Player->SetField(TEXT("location"), MakeVector(Pawn->GetActorLocation()));
			Player->SetField(TEXT("rotation"), MakeRotator(Pawn->GetActorRotation()));
			Player->SetField(TEXT("control_rotation"), MakeRotator(Context.PlayerController->GetControlRotation()));
			const FVector Velocity = Pawn->GetVelocity();
			Player->SetField(TEXT("velocity"), MakeVector(Velocity));
			Player->SetField(TEXT("speed"), MakeNumber(Velocity.Size2D()));
			const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
			Player->SetField(TEXT("movement_mode"), Movement ? StringValue(Movement->GetMovementName()) : NullValue());
			Player->SetField(TEXT("is_dead"), Hero ? BoolValue(Hero->IsDead()) : NullValue());
			Player->SetObjectField(TEXT("attributes"), MakeAttributes(Attributes));
			Player->SetArrayField(TEXT("tags"), ASC ? TagsToJson(ASC->GetOwnedGameplayTags()) : TArray<TSharedPtr<FJsonValue>>());
			Player->SetField(TEXT("weapon"), GatherWeapon(Hero));
			Player->SetArrayField(TEXT("spells"), GatherSpells(Cast<AMordecaiPlayerController>(Context.PlayerController), ASC, Attributes));
			const AMordecaiGameMode* GameMode = Context.World ? Cast<AMordecaiGameMode>(Context.World->GetAuthGameMode()) : nullptr;
			Player->SetField(TEXT("kills"), GameMode ? MakeNumber(GameMode->GetEnemyKillCount()) : NullValue());
			return Player;
		}

		static TSharedRef<FJsonObject> GatherInput(const FMordecaiAgentBridgeStateContext& Context)
		{
			if (!Context.PlayerController)
			{
				return MakeInvalidSection(TEXT("no local player controller yet"));
			}
			const UPlayerInput* PlayerInput = Context.PlayerController->PlayerInput;
			if (!PlayerInput)
			{
				return MakeInvalidSection(TEXT("the player controller has no PlayerInput"));
			}

			const TSharedRef<FJsonObject> Input = MakeShared<FJsonObject>();
			Input->SetBoolField(TEXT("valid"), true);

			TArray<TSharedPtr<FJsonValue>> Held;
			TArray<FKey> Candidates = LiveMordecaiKeys();
			for (const FMordecaiAgentBridgeHeldInput& HeldInput : Context.HeldInputs)
			{
				Held.Add(MakeShared<FJsonValueObject>(MakeHeldInputEntry(HeldInput)));
				Candidates.AddUnique(FKey(FName(*HeldInput.Key)));
			}
			// Threshold buttons the bridge synthesizes from axes are not held by a connection, but show in pressed
			for (const FString& Synthesized : Context.SynthesizedKeys)
			{
				Candidates.AddUnique(FKey(FName(*Synthesized)));
			}
			Input->SetArrayField(TEXT("held"), Held);

			TArray<TSharedPtr<FJsonValue>> Pressed;
			for (const FKey& Key : Candidates)
			{
				if (Key.IsValid() && PlayerInput->IsPressed(Key))
				{
					Pressed.Add(StringValue(Key.GetFName().ToString()));
				}
			}
			Input->SetArrayField(TEXT("pressed"), Pressed);

			// The applied-context map on UEnhancedPlayerInput is protected; ask the subsystem about each loaded context
			TArray<TPair<int32, FString>> Applied;
			ULocalPlayer* LocalPlayer = Context.PlayerController->GetLocalPlayer();
			const UEnhancedInputLocalPlayerSubsystem* EnhancedInput = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
			if (EnhancedInput)
			{
				for (TObjectIterator<UInputMappingContext> It; It; ++It)
				{
					const UInputMappingContext* MappingContext = *It;
					int32 Priority = 0;
					if (IsValid(MappingContext) && !MappingContext->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject)
						&& EnhancedInput->HasMappingContext(MappingContext, Priority))
					{
						Applied.Add({ Priority, MappingContext->GetName() });
					}
				}
			}
			Applied.Sort([](const TPair<int32, FString>& A, const TPair<int32, FString>& B) { return A.Key != B.Key ? A.Key > B.Key : A.Value < B.Value; });
			TArray<TSharedPtr<FJsonValue>> Contexts;
			for (const TPair<int32, FString>& Pair : Applied)
			{
				Contexts.Add(StringValue(Pair.Value));
			}
			Input->SetArrayField(TEXT("mapping_contexts"), Contexts);
			return Input;
		}

		static TSharedRef<FJsonObject> GatherPrompt(const FMordecaiAgentBridgeStateContext& Context)
		{
			const AMordecaiPlayerController* Controller = Cast<AMordecaiPlayerController>(Context.PlayerController);
			const UMordecaiCombatHUDWidget* Hud = Controller ? Controller->GetCombatHUDWidget() : nullptr;
			const UMordecaiPickupPromptWidget* Prompt = Hud ? Hud->GetPickupPrompt() : nullptr;
			if (!Prompt)
			{
				return MakeInvalidSection(Hud ? TEXT("the combat HUD has no pickup prompt") : TEXT("no combat HUD"));
			}
			return GatherPromptWidget(Prompt);
		}

		static TSharedRef<FJsonObject> GatherPickups(const FMordecaiAgentBridgeStateContext& Context)
		{
			if (!Context.World)
			{
				return MakeInvalidSection(TEXT("no world yet"));
			}
			if (!Context.Pawn)
			{
				return MakeInvalidSection(TEXT("no pawn yet"));
			}
			const FVector Origin = Context.Pawn->GetActorLocation();
			const UMordecaiPickupInteractionComponent* Interaction = Context.Pawn->FindComponentByClass<UMordecaiPickupInteractionComponent>();

			const TSharedRef<FJsonObject> Pickups = MakeShared<FJsonObject>();
			Pickups->SetBoolField(TEXT("valid"), true);
			const AMordecaiItemPickup* Focused = Interaction ? Interaction->GetFocusedPickup() : nullptr;
			Pickups->SetField(TEXT("focused"), Focused ? ObjectValue(MakePickupEntry(Focused, &Origin)) : NullValue());
			Pickups->SetField(TEXT("in_range_count"), Interaction ? MakeNumber(Interaction->GetInRangePickupCount()) : NullValue());
			Pickups->SetObjectField(TEXT("prompt"), GatherPrompt(Context));

			TArray<TPair<double, TSharedPtr<FJsonValue>>> Sorted;
			for (TActorIterator<AMordecaiItemPickup> It(Context.World); It; ++It)
			{
				const AMordecaiItemPickup* Pickup = *It;
				if (!IsValid(Pickup) || Pickup->IsActorBeingDestroyed())
				{
					continue;
				}
				Sorted.Add({ FVector::Dist(Origin, Pickup->GetActorLocation()), MakeShared<FJsonValueObject>(MakePickupEntry(Pickup, &Origin)) });
			}
			Sorted.Sort([](const TPair<double, TSharedPtr<FJsonValue>>& A, const TPair<double, TSharedPtr<FJsonValue>>& B) { return A.Key < B.Key; });
			TArray<TSharedPtr<FJsonValue>> World;
			for (const TPair<double, TSharedPtr<FJsonValue>>& Pair : Sorted)
			{
				World.Add(Pair.Value);
			}
			Pickups->SetArrayField(TEXT("world"), World);
			return Pickups;
		}

		static TSharedRef<FJsonObject> GatherInventoryForPawn(const FMordecaiAgentBridgeStateContext& Context)
		{
			if (!Context.Pawn)
			{
				return MakeInvalidSection(TEXT("no pawn yet"));
			}
			return GatherInventory(Context.Pawn->FindComponentByClass<UMordecaiInventoryComponent>(),
				Context.Pawn->FindComponentByClass<UMordecaiResourceLedger>());
		}

		static TSharedRef<FJsonObject> GatherPauseMenu(const FMordecaiAgentBridgeStateContext& Context)
		{
			UMordecaiPauseMenuSubsystem* Subsystem = Context.GameInstance ? Context.GameInstance->GetSubsystem<UMordecaiPauseMenuSubsystem>() : nullptr;
			if (!Subsystem)
			{
				return MakeInvalidSection(Context.GameInstance ? TEXT("no pause menu subsystem") : TEXT("no game instance"));
			}
			return GatherPauseMenuWidget(Subsystem->GetActiveMenuWidget(), Subsystem->IsMenuOpen(),
				FMordecaiAgentBridgeViewportTransform::FromViewportClient(Context.ViewportClient));
		}

		static TSharedRef<FJsonValue> GatherWidgets(const FMordecaiAgentBridgeStateContext& Context)
		{
			if (!Context.World)
			{
				return MakeShared<FJsonValueObject>(MakeInvalidSection(TEXT("no world yet")));
			}
			const FMordecaiAgentBridgeViewportTransform Transform = FMordecaiAgentBridgeViewportTransform::FromViewportClient(Context.ViewportClient);

			TArray<TSharedPtr<FJsonValue>> Widgets;
			for (TObjectIterator<UUserWidget> It; It; ++It)
			{
				const UUserWidget* Widget = *It;
				if (!IsValid(Widget) || Widget->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject) || Widget->GetWorld() != Context.World)
				{
					continue;
				}
				if (!GetNativeClassName(Widget->GetClass()).StartsWith(TEXT("Mordecai")) || !IsWidgetOnScreen(Widget))
				{
					continue;
				}
				if (const UCommonActivatableWidget* Activatable = Cast<UCommonActivatableWidget>(Widget))
				{
					if (!Activatable->IsActivated())
					{
						continue;
					}
				}

				const TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
				Entry->SetStringField(TEXT("class"), Widget->GetClass()->GetName());
				Entry->SetStringField(TEXT("name"), Widget->GetName());
				Entry->SetField(TEXT("rect"), WidgetRect(Widget, Transform));
				TArray<TSharedPtr<FJsonValue>> Texts;
				TArray<TSharedPtr<FJsonValue>> Buttons;
				if (Widget->WidgetTree)
				{
					Widget->WidgetTree->ForEachWidget([&](UWidget* Child)
					{
						if (const UTextBlock* Text = Cast<UTextBlock>(Child))
						{
							const FString String = Text->GetText().ToString();
							if (!String.IsEmpty() && IsWidgetOnScreen(Text))
							{
								Texts.Add(StringValue(String));
							}
						}
						else if (const UButton* Button = Cast<UButton>(Child))
						{
							if (IsWidgetOnScreen(Button))
							{
								Buttons.Add(MakeShared<FJsonValueObject>(MakeButtonEntry(Button, Transform)));
							}
						}
					});
				}
				Entry->SetArrayField(TEXT("texts"), Texts);
				Entry->SetArrayField(TEXT("buttons"), Buttons);
				Widgets.Add(MakeShared<FJsonValueObject>(Entry));
			}
			return MakeShared<FJsonValueArray>(Widgets);
		}

		static TSharedRef<FJsonValue> GatherActors(const FMordecaiAgentBridgeStateContext& Context, double Radius, bool bIncludeAllActors)
		{
			if (!Context.World)
			{
				return MakeShared<FJsonValueObject>(MakeInvalidSection(TEXT("no world yet")));
			}
			const bool bHasOrigin = Context.Pawn != nullptr;
			const FVector Origin = bHasOrigin ? Context.Pawn->GetActorLocation() : FVector::ZeroVector;

			TArray<TPair<double, TSharedPtr<FJsonValue>>> Sorted;
			for (TActorIterator<AActor> It(Context.World); It; ++It)
			{
				const AActor* Actor = *It;
				if (!IsValid(Actor) || Actor->IsActorBeingDestroyed())
				{
					continue;
				}
				if (!bIncludeAllActors && !IsInterestingActor(Actor))
				{
					continue;
				}
				const double Distance = FVector::Dist(Origin, Actor->GetActorLocation());
				if (bHasOrigin && Distance > Radius)
				{
					continue;
				}
				Sorted.Add({ Distance, MakeShared<FJsonValueObject>(MakeActorEntry(Actor, bHasOrigin ? &Origin : nullptr)) });
			}
			Sorted.Sort([](const TPair<double, TSharedPtr<FJsonValue>>& A, const TPair<double, TSharedPtr<FJsonValue>>& B) { return A.Key < B.Key; });
			TArray<TSharedPtr<FJsonValue>> Actors;
			for (const TPair<double, TSharedPtr<FJsonValue>>& Pair : Sorted)
			{
				Actors.Add(Pair.Value);
			}
			return MakeShared<FJsonValueArray>(Actors);
		}
	}

	TSharedRef<FJsonObject> MakeInvalidSection(const FString& Reason)
	{
		const TSharedRef<FJsonObject> Section = MakeShared<FJsonObject>();
		Section->SetBoolField(TEXT("valid"), false);
		Section->SetStringField(TEXT("reason"), Reason);
		return Section;
	}

	TSharedRef<FJsonObject> GatherState(const FMordecaiAgentBridgeStateContext& Context, const TArray<FString>& Sections, double Radius, bool bIncludeAllActors)
	{
		const TSharedRef<FJsonObject> State = MakeShared<FJsonObject>();
		for (const FString& Section : (Sections.Num() > 0 ? Sections : GetStateSectionNames()))
		{
			State->SetField(Section, GatherSection(Context, Section, Radius, bIncludeAllActors));
		}
		return State;
	}

	TSharedRef<FJsonValue> GatherSection(const FMordecaiAgentBridgeStateContext& Context, const FString& Section, double Radius, bool bIncludeAllActors)
	{
		using namespace StatePrivate;
		if (Section == TEXT("game")) { return MakeShared<FJsonValueObject>(GatherGame(Context)); }
		if (Section == TEXT("focus")) { return MakeShared<FJsonValueObject>(GatherFocus(Context)); }
		if (Section == TEXT("player")) { return MakeShared<FJsonValueObject>(GatherPlayer(Context)); }
		if (Section == TEXT("input")) { return MakeShared<FJsonValueObject>(GatherInput(Context)); }
		if (Section == TEXT("pickups")) { return MakeShared<FJsonValueObject>(GatherPickups(Context)); }
		if (Section == TEXT("inventory")) { return MakeShared<FJsonValueObject>(GatherInventoryForPawn(Context)); }
		if (Section == TEXT("pause_menu")) { return MakeShared<FJsonValueObject>(GatherPauseMenu(Context)); }
		if (Section == TEXT("widgets")) { return GatherWidgets(Context); }
		if (Section == TEXT("actors")) { return GatherActors(Context, Radius, bIncludeAllActors); }
		return MakeShared<FJsonValueObject>(MakeInvalidSection(FString::Printf(TEXT("unknown section '%s'"), *Section)));
	}

	FMordecaiAgentBridgeReadiness GetReadiness(const FMordecaiAgentBridgeStateContext& Context)
	{
		FMordecaiAgentBridgeReadiness Ready;
		Ready.bHasWorld = Context.World != nullptr;
		Ready.bHasPawn = Context.Pawn != nullptr;
		if (Context.World)
		{
			Ready.Map = UWorld::RemovePIEPrefix(Context.World->GetOutermost()->GetName());
			Ready.bPaused = Context.World->IsPaused();
			const AGameStateBase* GameState = Context.World->GetGameState();
			const ULyraExperienceManagerComponent* Experience = GameState ? GameState->FindComponentByClass<ULyraExperienceManagerComponent>() : nullptr;
			if (Experience && Experience->IsExperienceLoaded())
			{
				Ready.bExperienceLoaded = true;
				Ready.Experience = NormalizeExperienceName(Experience->GetCurrentExperienceChecked()->GetName());
			}
		}
		if (Context.GameInstance)
		{
			if (const ULoadingScreenManager* LoadingScreen = Context.GameInstance->GetSubsystem<ULoadingScreenManager>())
			{
				Ready.bLoadingScreenKnown = true;
				Ready.bLoadingScreen = LoadingScreen->GetLoadingScreenDisplayStatus();
			}
		}
		return Ready;
	}

	TSharedRef<FJsonObject> MakeAttributes(const UMordecaiAttributeSet* Attributes)
	{
		if (!Attributes)
		{
			return MakeInvalidSection(TEXT("no UMordecaiAttributeSet on the PlayerState ability system"));
		}
		const TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		Json->SetField(TEXT("health"), MakeNumber(Attributes->GetHealth()));
		Json->SetField(TEXT("max_health"), MakeNumber(Attributes->GetMaxHealth()));
		Json->SetField(TEXT("stamina"), MakeNumber(Attributes->GetStamina()));
		Json->SetField(TEXT("max_stamina"), MakeNumber(Attributes->GetMaxStamina()));
		Json->SetField(TEXT("posture"), MakeNumber(Attributes->GetPosture()));
		Json->SetField(TEXT("max_posture"), MakeNumber(Attributes->GetMaxPosture()));
		Json->SetField(TEXT("spell_points"), MakeNumber(Attributes->GetSpellPoints()));
		Json->SetField(TEXT("max_spell_points"), MakeNumber(Attributes->GetMaxSpellPoints()));
		return Json;
	}

	TSharedRef<FJsonObject> GatherInventory(const UMordecaiInventoryComponent* Inventory, const UMordecaiResourceLedger* Ledger)
	{
		using namespace StatePrivate;
		if (!Inventory)
		{
			return MakeInvalidSection(TEXT("the pawn has no UMordecaiInventoryComponent"));
		}

		const TSharedRef<FJsonObject> Section = MakeShared<FJsonObject>();
		Section->SetBoolField(TEXT("valid"), true);

		// GetSortedItems is the order the inventory UI lists them in
		TArray<TSharedPtr<FJsonValue>> Items;
		for (const FMordecaiItemInstance& Instance : Inventory->GetSortedItems())
		{
			const UMordecaiItemDefinition* Def = Instance.ItemDefinition;
			const TSharedRef<FJsonObject> Item = MakeShared<FJsonObject>();
			Item->SetStringField(TEXT("instance_id"), Instance.InstanceId.ToString());
			Item->SetField(TEXT("item_id"), Def ? StringValue(Def->ItemId.ToString()) : NullValue());
			Item->SetStringField(TEXT("display_name"), UMordecaiItemLibrary::GetDisplayName(Instance).ToString());
			Item->SetField(TEXT("quantity"), MakeNumber(Instance.Quantity));
			Item->SetField(TEXT("type"), Def ? StringValue(EnumName(Def->ItemType)) : NullValue());
			Item->SetField(TEXT("rarity"), Def ? StringValue(EnumName(Def->Rarity)) : NullValue());
			Item->SetBoolField(TEXT("identified"), Instance.IsIdentified());
			Item->SetBoolField(TEXT("equipped"), Instance.IsEquipped);
			Items.Add(MakeShared<FJsonValueObject>(Item));
		}
		Section->SetField(TEXT("count"), MakeNumber(Items.Num()));
		Section->SetArrayField(TEXT("items"), Items);

		if (Ledger)
		{
			TArray<FMordecaiResourceEntry> Entries = Ledger->GetAllResources();
			Entries.Sort([](const FMordecaiResourceEntry& A, const FMordecaiResourceEntry& B) { return A.ItemId.LexicalLess(B.ItemId); });
			TArray<TSharedPtr<FJsonValue>> LedgerJson;
			for (const FMordecaiResourceEntry& Entry : Entries)
			{
				const TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
				Row->SetStringField(TEXT("item_id"), Entry.ItemId.ToString());
				Row->SetStringField(TEXT("display_name"), Entry.Def ? Entry.Def->DisplayName.ToString() : Entry.ItemId.ToString());
				Row->SetField(TEXT("count"), MakeNumber(Entry.Count));
				LedgerJson.Add(MakeShared<FJsonValueObject>(Row));
			}
			Section->SetField(TEXT("ledger_count"), MakeNumber(LedgerJson.Num()));
			Section->SetArrayField(TEXT("ledger"), LedgerJson);
		}
		else
		{
			Section->SetField(TEXT("ledger_count"), NullValue());
			Section->SetField(TEXT("ledger"), NullValue());
		}
		return Section;
	}

	TSharedRef<FJsonObject> MakePickupEntry(const AMordecaiItemPickup* Pickup, const FVector* Origin)
	{
		using namespace StatePrivate;
		const TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
		if (!Pickup)
		{
			return Entry;
		}
		Entry->SetStringField(TEXT("label"), Pickup->GetActorNameOrLabel());
		Entry->SetField(TEXT("item_id"), Pickup->ItemDefinition ? StringValue(Pickup->ItemDefinition->ItemId.ToString()) : NullValue());
		Entry->SetStringField(TEXT("display_name"), Pickup->GetItemDisplayName().ToString());
		Entry->SetField(TEXT("location"), MakeVector(Pickup->GetActorLocation()));
		Entry->SetField(TEXT("distance"), Origin ? MakeNumber(FVector::Dist(*Origin, Pickup->GetActorLocation())) : NullValue());
		return Entry;
	}

	TSharedRef<FJsonObject> MakeActorEntry(const AActor* Actor, const FVector* Origin)
	{
		using namespace StatePrivate;
		const TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
		if (!Actor)
		{
			return Entry;
		}
		Entry->SetStringField(TEXT("label"), Actor->GetActorNameOrLabel());
		Entry->SetStringField(TEXT("name"), Actor->GetName());
		Entry->SetStringField(TEXT("class"), Actor->GetClass()->GetName());
		Entry->SetStringField(TEXT("native_class"), GetNativeClassName(Actor->GetClass()));
		Entry->SetStringField(TEXT("kind"), ClassifyActorKind(Actor));
		Entry->SetField(TEXT("location"), MakeVector(Actor->GetActorLocation()));
		Entry->SetField(TEXT("distance"), Origin ? MakeNumber(FVector::Dist(*Origin, Actor->GetActorLocation())) : NullValue());
		Entry->SetBoolField(TEXT("hidden"), Actor->IsHidden());

		bool bVisibleMesh = false;
		TInlineComponentArray<UPrimitiveComponent*> Primitives(Actor);
		for (const UPrimitiveComponent* Primitive : Primitives)
		{
			if (!Primitive || !Primitive->IsVisible() || Primitive->bHiddenInGame)
			{
				continue;
			}
			if (const UStaticMeshComponent* StaticMesh = Cast<UStaticMeshComponent>(Primitive))
			{
				bVisibleMesh |= StaticMesh->GetStaticMesh() != nullptr;
			}
			else if (const USkinnedMeshComponent* Skinned = Cast<USkinnedMeshComponent>(Primitive))
			{
				bVisibleMesh |= Skinned->GetSkinnedAsset() != nullptr;
			}
		}
		Entry->SetBoolField(TEXT("has_visible_mesh"), bVisibleMesh);

		float Current = 0.f;
		float Max = 0.f;
		if (GetActorHealth(Actor, Current, Max))
		{
			const TSharedRef<FJsonObject> Health = MakeShared<FJsonObject>();
			Health->SetField(TEXT("current"), MakeNumber(Current));
			Health->SetField(TEXT("max"), MakeNumber(Max));
			Entry->SetObjectField(TEXT("health"), Health);
		}
		else
		{
			Entry->SetField(TEXT("health"), NullValue());
		}

		const AMordecaiEnemyCharacter* Enemy = Cast<AMordecaiEnemyCharacter>(Actor);
		const AMordecaiCharacter* Hero = Cast<AMordecaiCharacter>(Actor);
		if (Enemy)
		{
			Entry->SetBoolField(TEXT("is_dead"), Enemy->IsDead());
		}
		else if (Hero)
		{
			Entry->SetBoolField(TEXT("is_dead"), Hero->IsDead());
		}
		else
		{
			Entry->SetField(TEXT("is_dead"), NullValue());
		}

		const AMordecaiEnemyAIController* AI = Enemy ? Cast<AMordecaiEnemyAIController>(Enemy->GetController()) : nullptr;
		Entry->SetField(TEXT("ai_state"), AI ? StringValue(EnumName(AI->GetCurrentState())) : NullValue());

		const UAbilitySystemComponent* ASC = FindASC(Actor);
		Entry->SetArrayField(TEXT("tags"), ASC ? TagsToJson(ASC->GetOwnedGameplayTags()) : TArray<TSharedPtr<FJsonValue>>());

		TArray<TSharedPtr<FJsonValue>> Widgets;
		TInlineComponentArray<UWidgetComponent*> WidgetComponents(Actor);
		for (const UWidgetComponent* WidgetComponent : WidgetComponents)
		{
			if (!WidgetComponent)
			{
				continue;
			}
			const TSharedRef<FJsonObject> Widget = MakeShared<FJsonObject>();
			Widget->SetStringField(TEXT("component"), WidgetComponent->GetName());
			const TSubclassOf<UUserWidget> WidgetClass = WidgetComponent->GetWidgetClass();
			Widget->SetField(TEXT("class"), WidgetClass ? StringValue(WidgetClass->GetName()) : NullValue());
			Widget->SetStringField(TEXT("space"), WidgetComponent->GetWidgetSpace() == EWidgetSpace::Screen ? TEXT("Screen") : TEXT("World"));
			Widget->SetBoolField(TEXT("visible"), WidgetComponent->IsWidgetVisible());
			Widgets.Add(MakeShared<FJsonValueObject>(Widget));
		}
		Entry->SetArrayField(TEXT("widgets"), Widgets);
		return Entry;
	}

	bool GetActorHealth(const AActor* Actor, float& OutCurrent, float& OutMax)
	{
		if (!Actor)
		{
			return false;
		}
		// Mordecai damage writes UMordecaiAttributeSet; for the player this resolves to the PlayerState ASC
		if (const UAbilitySystemComponent* ASC = StatePrivate::FindASC(Actor))
		{
			if (const UMordecaiAttributeSet* Set = ASC->GetSet<UMordecaiAttributeSet>())
			{
				OutCurrent = Set->GetHealth();
				OutMax = Set->GetMaxHealth();
				return true;
			}
		}
		if (const AMordecaiEnemyCharacter* Enemy = Cast<AMordecaiEnemyCharacter>(Actor))
		{
			if (const UMordecaiAttributeSet* Set = Enemy->GetMordecaiAttributeSet())
			{
				OutCurrent = Set->GetHealth();
				OutMax = Set->GetMaxHealth();
				return true;
			}
		}
		if (const AMordecaiIllusionActor* Illusion = Cast<AMordecaiIllusionActor>(Actor))
		{
			OutCurrent = Illusion->GetCurrentHP();
			OutMax = Illusion->GetMaxHP();
			return true;
		}
		// Lyra's health component reads an unrelated set; use it only when it was ever initialized
		if (const ULyraHealthComponent* Health = ULyraHealthComponent::FindHealthComponent(Actor))
		{
			if (Health->GetMaxHealth() > 0.f)
			{
				OutCurrent = Health->GetHealth();
				OutMax = Health->GetMaxHealth();
				return true;
			}
		}
		return false;
	}

	FString ClassifyActorKind(const AActor* Actor)
	{
		if (Cast<AMordecaiEnemyCharacter>(Actor)) { return TEXT("enemy"); }
		if (Cast<AMordecaiItemPickup>(Actor)) { return TEXT("pickup"); }
		if (Cast<AMordecaiProjectile>(Actor)) { return TEXT("projectile"); }
		if (Cast<AMordecaiIllusionActor>(Actor)) { return TEXT("illusion"); }
		if (Cast<APawn>(Actor)) { return TEXT("pawn"); }
		return TEXT("other");
	}

	bool IsInterestingActor(const AActor* Actor)
	{
		if (!Actor)
		{
			return false;
		}
		return Actor->IsA<APawn>() || Actor->IsA<AMordecaiItemPickup>() || Actor->IsA<AMordecaiIllusionActor>()
			|| Actor->IsA<AMordecaiProjectile>() || Actor->FindComponentByClass<UWidgetComponent>() != nullptr;
	}

	FString GetNativeClassName(const UClass* Class)
	{
		while (Class && !Class->HasAnyClassFlags(CLASS_Native))
		{
			Class = Class->GetSuperClass();
		}
		return Class ? Class->GetName() : FString();
	}

	bool IsWidgetOnScreen(const UWidget* Widget)
	{
		if (!Widget)
		{
			return false;
		}
		TSharedPtr<SWidget> Slate = Widget->GetCachedWidget();
		while (Slate.IsValid())
		{
			if (!Slate->GetVisibility().IsVisible() || Slate->GetRenderOpacity() <= 0.f)
			{
				return false;
			}
			if (Slate->GetType() == FName(TEXT("SWindow")))
			{
				return StaticCastSharedPtr<SWindow>(Slate)->IsVisible();
			}
			Slate = Slate->GetParentWidget();
		}
		return false;
	}

	TSharedRef<FJsonObject> MakeHeldInputEntry(const FMordecaiAgentBridgeHeldInput& Held)
	{
		const TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
		Entry->SetStringField(TEXT("key"), Held.Key);
		Entry->SetStringField(TEXT("kind"), Held.Kind);
		Entry->SetField(TEXT("value"), MakeNumber(Held.Value));
		Entry->SetField(TEXT("holders"), MakeNumber(Held.Holders));
		return Entry;
	}

	TSharedRef<FJsonObject> GatherPromptWidget(const UMordecaiPickupPromptWidget* Prompt)
	{
		using namespace StatePrivate;
		if (!Prompt || !Prompt->WidgetTree)
		{
			return MakeInvalidSection(Prompt ? TEXT("the pickup prompt has no widget tree") : TEXT("no pickup prompt"));
		}
		const UTextBlock* PromptText = Prompt->WidgetTree->FindWidget<UTextBlock>(TEXT("PromptText"));
		const UTextBlock* HintText = Prompt->WidgetTree->FindWidget<UTextBlock>(TEXT("InventoryHintText"));
		if (!PromptText)
		{
			return MakeInvalidSection(TEXT("the pickup prompt has no PromptText text block"));
		}
		const TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
		Entry->SetBoolField(TEXT("valid"), true);
		Entry->SetBoolField(TEXT("visible"), IsWidgetOnScreen(PromptText));
		Entry->SetStringField(TEXT("text"), PromptText->GetText().ToString());
		Entry->SetField(TEXT("hint_text"), HintText ? StringValue(HintText->GetText().ToString()) : NullValue());
		Entry->SetField(TEXT("hint_visible"), HintText ? BoolValue(IsWidgetOnScreen(HintText)) : NullValue());
		return Entry;
	}

	TSharedRef<FJsonObject> GatherPauseMenuWidget(const UMordecaiPauseMenuWidget* Menu, bool bOpen, const FMordecaiAgentBridgeViewportTransform& Transform)
	{
		using namespace StatePrivate;
		const TSharedRef<FJsonObject> PauseMenu = MakeShared<FJsonObject>();
		PauseMenu->SetBoolField(TEXT("valid"), true);
		PauseMenu->SetBoolField(TEXT("open"), bOpen);
		PauseMenu->SetBoolField(TEXT("visible"), IsWidgetOnScreen(Menu));
		// Tab ids are FNames: lowercase them, a cooked build prints the first-registered spelling ("Inventory")
		PauseMenu->SetField(TEXT("active_tab"), Menu ? StringValue(CanonicalNameId(Menu->GetActiveTabId())) : NullValue());
		TArray<TSharedPtr<FJsonValue>> Tabs;
		if (Menu)
		{
			for (const FMordecaiPauseMenuTabEntry& Tab : Menu->GetTabModel().Tabs)
			{
				const TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
				Entry->SetStringField(TEXT("id"), CanonicalNameId(Tab.TabId));
				Entry->SetStringField(TEXT("name"), Tab.DisplayName.ToString());
				Tabs.Add(MakeShared<FJsonValueObject>(Entry));
			}
		}
		PauseMenu->SetArrayField(TEXT("tabs"), Tabs);
		PauseMenu->SetField(TEXT("inventory_tab"), GatherInventoryTab(
			Menu ? Cast<UMordecaiInventoryWidget>(Menu->GetCurrentContentWidget()) : nullptr, Transform));
		return PauseMenu;
	}

	TSharedRef<FJsonValue> GatherInventoryTab(const UMordecaiInventoryWidget* Inventory, const FMordecaiAgentBridgeViewportTransform& Transform)
	{
		using namespace StatePrivate;
		if (!Inventory || !Inventory->WidgetTree)
		{
			return NullValue();
		}
		const TSharedRef<FJsonObject> Tab = MakeShared<FJsonObject>();
		Tab->SetBoolField(TEXT("visible"), IsWidgetOnScreen(Inventory));

		const TArray<FMordecaiInventoryRowModel> Model = Inventory->GetVisibleRows();
		TArray<TSharedPtr<FJsonValue>> Rows;
		if (const UScrollBox* List = Inventory->WidgetTree->FindWidget<UScrollBox>(TEXT("ItemListBox")))
		{
			const TArray<UWidget*> Children = List->GetAllChildren();
			const bool bModelMatches = Model.Num() == Children.Num();
			for (int32 Index = 0; Index < Children.Num(); ++Index)
			{
				const UPanelWidget* RowWidget = Cast<UPanelWidget>(Children[Index]);
				const UTextBlock* Name = FirstTextChild(RowWidget);
				const UMordecaiTabButton* Identify = nullptr;
				if (RowWidget)
				{
					for (int32 ChildIndex = 0; ChildIndex < RowWidget->GetChildrenCount() && !Identify; ++ChildIndex)
					{
						Identify = Cast<UMordecaiTabButton>(RowWidget->GetChildAt(ChildIndex));
					}
				}

				const TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
				Row->SetField(TEXT("text"), Name ? StringValue(Name->GetText().ToString()) : NullValue());
				// The Identify button id is FName(guid): re-format it exactly as inventory.items[].instance_id
				FString InstanceId;
				if (Identify && TryCanonicalGuid(Identify->GetButtonId().ToString(), InstanceId))
				{
					Row->SetStringField(TEXT("instance_id"), InstanceId);
				}
				else
				{
					Row->SetField(TEXT("instance_id"), bModelMatches ? StringValue(Model[Index].InstanceId.ToString()) : NullValue());
				}
				Row->SetBoolField(TEXT("unidentified"), bModelMatches ? Model[Index].bIsUnidentified : Identify != nullptr);
				if (Identify)
				{
					const TSharedRef<FJsonObject> Button = MakeShared<FJsonObject>();
					const UTextBlock* Label = FirstTextChild(Identify);
					Button->SetField(TEXT("label"), Label ? StringValue(Label->GetText().ToString()) : NullValue());
					Button->SetField(TEXT("center"), WidgetCenter(Identify, Transform));
					Button->SetBoolField(TEXT("enabled"), Identify->GetIsEnabled());
					Row->SetObjectField(TEXT("identify_button"), Button);
				}
				else
				{
					Row->SetField(TEXT("identify_button"), NullValue());
				}
				Rows.Add(MakeShared<FJsonValueObject>(Row));
			}
		}
		Tab->SetArrayField(TEXT("rows"), Rows);

		TArray<TSharedPtr<FJsonValue>> LedgerRows;
		if (const UVerticalBox* LedgerBox = Inventory->WidgetTree->FindWidget<UVerticalBox>(TEXT("LedgerBox")))
		{
			for (const UWidget* Child : LedgerBox->GetAllChildren())
			{
				if (const UTextBlock* Text = Cast<UTextBlock>(Child))
				{
					LedgerRows.Add(StringValue(Text->GetText().ToString()));
				}
			}
		}
		Tab->SetArrayField(TEXT("ledger_rows"), LedgerRows);

		// Placeholders are reported only while they are actually shown
		const UTextBlock* ListPlaceholder = Inventory->WidgetTree->FindWidget<UTextBlock>(TEXT("ListPlaceholder"));
		const UTextBlock* LedgerPlaceholder = Inventory->WidgetTree->FindWidget<UTextBlock>(TEXT("LedgerPlaceholder"));
		Tab->SetField(TEXT("list_placeholder"), ListPlaceholder && IsWidgetOnScreen(ListPlaceholder)
			? StringValue(ListPlaceholder->GetText().ToString()) : NullValue());
		Tab->SetField(TEXT("ledger_placeholder"), LedgerPlaceholder && IsWidgetOnScreen(LedgerPlaceholder)
			? StringValue(LedgerPlaceholder->GetText().ToString()) : NullValue());
		return MakeShared<FJsonValueObject>(Tab);
	}

	TSharedRef<FJsonObject> MakeButtonEntry(const UButton* Button, const FMordecaiAgentBridgeViewportTransform& Transform)
	{
		using namespace StatePrivate;
		const TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
		if (!Button)
		{
			return Entry;
		}
		// Tab and filter ids lowercase, identify-button ids in instance_id form (see CanonicalButtonId)
		const UMordecaiTabButton* TabButton = Cast<UMordecaiTabButton>(Button);
		Entry->SetStringField(TEXT("id"), TabButton ? CanonicalButtonId(TabButton->GetButtonId().ToString()) : Button->GetName());
		const UTextBlock* Label = FirstTextChild(Button);
		Entry->SetField(TEXT("label"), Label ? StringValue(Label->GetText().ToString()) : NullValue());
		Entry->SetField(TEXT("center"), WidgetCenter(Button, Transform));
		Entry->SetBoolField(TEXT("enabled"), Button->GetIsEnabled());
		return Entry;
	}

	TSharedRef<FJsonValue> GeometryCenter(const FGeometry& Geometry, const FMordecaiAgentBridgeViewportTransform& Transform)
	{
		if (!Transform.bValid)
		{
			return StatePrivate::NullValue();
		}
		const FVector2D Size(Geometry.GetAbsoluteSize());
		if (Size.X <= 0.0 || Size.Y <= 0.0)
		{
			return StatePrivate::NullValue();
		}
		const FVector2D Center = FVector2D(Geometry.GetAbsolutePosition()) + Size * 0.5;
		return MakeVector2D(Transform.ToViewport(Center));
	}

	TSharedRef<FJsonValue> GeometryRect(const FGeometry& Geometry, const FMordecaiAgentBridgeViewportTransform& Transform)
	{
		if (!Transform.bValid)
		{
			return StatePrivate::NullValue();
		}
		const FVector2D AbsPos(Geometry.GetAbsolutePosition());
		const FVector2D AbsSize(Geometry.GetAbsoluteSize());
		const FVector2D TopLeft = Transform.ToViewport(AbsPos);
		const FVector2D BottomRight = Transform.ToViewport(AbsPos + AbsSize);
		return MakeShared<FJsonValueArray>(TArray<TSharedPtr<FJsonValue>>{
			MakeNumber(TopLeft.X, 1), MakeNumber(TopLeft.Y, 1),
			MakeNumber(BottomRight.X - TopLeft.X, 1), MakeNumber(BottomRight.Y - TopLeft.Y, 1) });
	}
}
