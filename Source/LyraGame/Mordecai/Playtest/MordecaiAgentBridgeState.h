// Project Mordecai — Agent bridge state gathering (Specs/agent_bridge_v1.md §4 state)
//
// Read-only snapshot of what the game shows and holds, as JSON. Every gatherer takes explicit inputs so
// the pieces can be tested headless with NewObject'd components; a section that cannot be gathered is
// {"valid": false, "reason": "..."} — values are never invented. Nothing here mutates game state.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

class AActor;
class AMordecaiItemPickup;
class APawn;
class APlayerController;
class UButton;
class UGameInstance;
class UGameViewportClient;
class UMordecaiAttributeSet;
class UMordecaiInventoryComponent;
class UMordecaiInventoryWidget;
class UMordecaiPauseMenuWidget;
class UMordecaiPickupPromptWidget;
class UMordecaiResourceLedger;
class UTextRenderComponent;
class UWidget;
class UWorld;
struct FGeometry;

/** An input bridge connections hold, for state.input.held (one entry per input across connections). */
struct FMordecaiAgentBridgeHeldInput
{
	FString Key;
	FString Kind; // "key" | "axis" | "mouse"
	double Value = 1.0;
	/** How many connections hold it. */
	int32 Holders = 1;
};

/** Everything a state gather may read. Resolve with FromGameInstance on the game thread. */
struct FMordecaiAgentBridgeStateContext
{
	UGameInstance* GameInstance = nullptr;
	UWorld* World = nullptr;
	APlayerController* PlayerController = nullptr;
	APawn* Pawn = nullptr;
	UGameViewportClient* ViewportClient = nullptr;

	/** Bridge-side facts (filled by the bridge server). */
	TArray<FMordecaiAgentBridgeHeldInput> HeldInputs;
	/** Threshold buttons the bridge currently synthesizes from held axes (candidates for input.pressed). */
	TArray<FString> SynthesizedKeys;
	/** Live: Slate's cursor user is on the bridge's in-memory cursor right now. */
	bool bVirtualCursor = false;

	static FMordecaiAgentBridgeStateContext FromGameInstance(UGameInstance* GameInstance);
};

/** The readiness fields shared by ping and state.game. */
struct FMordecaiAgentBridgeReadiness
{
	bool bHasWorld = false;
	FString Map;
	FString Experience;          // normalized; empty until loaded
	bool bExperienceLoaded = false;
	bool bLoadingScreen = false;
	bool bLoadingScreenKnown = false;
	bool bHasPawn = false;
	bool bPaused = false;

	bool IsReady() const { return bExperienceLoaded && !bLoadingScreen && bHasPawn; }
};

/** Viewport placement in Slate absolute space, for viewport-pixel conversions. */
struct FMordecaiAgentBridgeViewportTransform
{
	bool bValid = false;
	FVector2D AbsolutePosition = FVector2D::ZeroVector;
	FVector2D AbsoluteSize = FVector2D::ZeroVector;
	FIntPoint ViewportSize = FIntPoint::ZeroValue;

	FVector2D ToViewport(const FVector2D& Absolute) const;
	FVector2D ToAbsolute(const FVector2D& Pixel) const;
	static FMordecaiAgentBridgeViewportTransform FromViewportClient(const UGameViewportClient* ViewportClient);
};

namespace MordecaiAgentBridge::State
{
	/** {"valid": false, "reason": Reason} */
	TSharedRef<FJsonObject> MakeInvalidSection(const FString& Reason);

	/** {"section": ...} for each requested section (all of them when Sections is empty). */
	TSharedRef<FJsonObject> GatherState(const FMordecaiAgentBridgeStateContext& Context, const TArray<FString>& Sections, double Radius, bool bIncludeAllActors);

	/** One section by name; unknown names are an invalid section. */
	TSharedRef<FJsonValue> GatherSection(const FMordecaiAgentBridgeStateContext& Context, const FString& Section, double Radius, bool bIncludeAllActors);

	FMordecaiAgentBridgeReadiness GetReadiness(const FMordecaiAgentBridgeStateContext& Context);

	// --- Pieces (headless-testable) ---

	/** {health,max_health,stamina,max_stamina,posture,max_posture,spell_points,max_spell_points}; null set -> invalid. */
	TSharedRef<FJsonObject> MakeAttributes(const UMordecaiAttributeSet* Attributes);

	/** The inventory section: {valid,count,items,ledger_count,ledger}. Null inventory -> invalid. */
	TSharedRef<FJsonObject> GatherInventory(const UMordecaiInventoryComponent* Inventory, const UMordecaiResourceLedger* Ledger);

	/** {label,item_id,display_name,location,distance}; distance is null without an origin. */
	TSharedRef<FJsonObject> MakePickupEntry(const AMordecaiItemPickup* Pickup, const FVector* Origin);

	/** One actors[] entry. Distance is null without an origin; text_labels[].facing/upright are null without a view rotation. */
	TSharedRef<FJsonObject> MakeActorEntry(const AActor* Actor, const FVector* Origin, const FRotator* ViewRotation = nullptr);

	/** One actors[].text_labels[] entry: {component, text, visible, facing, upright}; facing/upright are null without a view rotation. */
	TSharedRef<FJsonObject> MakeTextLabelEntry(const UTextRenderComponent* Label, const AActor* Owner, const FRotator* ViewRotation);

	/** Mordecai health for any actor: ASC attribute set, enemy set, illusion HP, then Lyra health. */
	bool GetActorHealth(const AActor* Actor, float& OutCurrent, float& OutMax);

	/** "enemy" | "pickup" | "pawn" | "projectile" | "illusion" | "other" */
	FString ClassifyActorKind(const AActor* Actor);

	/** The default actors filter: pawns, pickups, illusions, projectiles and actors with widget components. */
	bool IsInterestingActor(const AActor* Actor);

	/** First native class name in the hierarchy (e.g. MordecaiEnemyCharacter for a BP subclass). */
	FString GetNativeClassName(const UClass* Class);

	/** True when the widget's Slate widget and every parent up to an SWindow are visible. */
	bool IsWidgetOnScreen(const UWidget* Widget);

	/** One input.held entry: {key, kind, value, holders}. */
	TSharedRef<FJsonObject> MakeHeldInputEntry(const FMordecaiAgentBridgeHeldInput& Held);

	/** pickups.prompt from the HUD prompt widget's rendered PromptText / InventoryHintText blocks. */
	TSharedRef<FJsonObject> GatherPromptWidget(const UMordecaiPickupPromptWidget* Prompt);

	/** pause_menu from the pushed menu widget (null while closed): {valid, open, visible, active_tab, tabs, inventory_tab}. */
	TSharedRef<FJsonObject> GatherPauseMenuWidget(const UMordecaiPauseMenuWidget* Menu, bool bOpen, const FMordecaiAgentBridgeViewportTransform& Transform);

	/** pause_menu.inventory_tab from the rendered inventory widget, or a JSON null. */
	TSharedRef<FJsonValue> GatherInventoryTab(const UMordecaiInventoryWidget* Inventory, const FMordecaiAgentBridgeViewportTransform& Transform);

	/** One widgets[].buttons[] entry: {id, label, center, enabled}. */
	TSharedRef<FJsonObject> MakeButtonEntry(const UButton* Button, const FMordecaiAgentBridgeViewportTransform& Transform);

	/** Viewport-pixel centre [x,y] of a painted geometry; null before the first paint or without a viewport transform. */
	TSharedRef<FJsonValue> GeometryCenter(const FGeometry& Geometry, const FMordecaiAgentBridgeViewportTransform& Transform);

	/** Viewport-pixel rect [x,y,w,h] of a painted geometry; null without a viewport transform. */
	TSharedRef<FJsonValue> GeometryRect(const FGeometry& Geometry, const FMordecaiAgentBridgeViewportTransform& Transform);
}
