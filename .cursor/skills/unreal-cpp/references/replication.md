# Replication & Networking — Full Reference

## Enabling Replication

```cpp
// In Actor constructor
AMyActor::AMyActor()
{
    bReplicates = true;           // actor replicates
    bAlwaysRelevant = false;      // only relevant when in range
    NetUpdateFrequency = 10.f;    // Hz — how often to replicate
    MinNetUpdateFrequency = 2.f;  // adaptive minimum
    NetPriority = 1.0f;           // replication priority

    // Component replication
    MeshComp->SetIsReplicated(true);
}
```

## Property Replication

### GetLifetimeReplicatedProps

```cpp
#include "Net/UnrealNetwork.h"

void AMyActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // Always replicate to everyone
    DOREPLIFETIME(AMyActor, Health);

    // Conditional replication
    DOREPLIFETIME_CONDITION(AMyActor, Ammo, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(AMyActor, SecretData, COND_OwnerOnly);

    // With RepNotify control
    DOREPLIFETIME_CONDITION_NOTIFY(AMyActor, Score, COND_None, REPNOTIFY_Always);
    // REPNOTIFY_Always  — fire OnRep even if value didn't change
    // REPNOTIFY_OnChanged — only fire if value actually changed (default)
}
```

### Replication Conditions

| Condition | Description |
|-----------|-------------|
| `COND_None` | Replicate to all |
| `COND_OwnerOnly` | Only to owning connection |
| `COND_SkipOwner` | Everyone except owner |
| `COND_SimulatedOnly` | Simulated proxies only |
| `COND_AutonomousOnly` | Autonomous proxy only |
| `COND_SimulatedOrPhysics` | Simulated or physics-simulating |
| `COND_InitialOnly` | Only on initial replication |
| `COND_Custom` | Custom condition via `PreReplication` |
| `COND_ReplayOrOwner` | Replay or owning connection |
| `COND_ReplayOnly` | Replay only |
| `COND_SkipReplay` | Never during replay |
| `COND_Dynamic` | Runtime-configurable condition |
| `COND_Never` | Never replicate (property still declared replicated) |

### RepNotify

```cpp
// Header
UPROPERTY(ReplicatedUsing=OnRep_Health)
float Health;

UFUNCTION()
void OnRep_Health();

// Or with old value parameter:
UFUNCTION()
void OnRep_Health(float OldHealth);

// Implementation
void AMyActor::OnRep_Health(float OldHealth)
{
    // React to health change on client
    if (Health <= 0.f && OldHealth > 0.f)
    {
        PlayDeathAnimation();
    }
}
```

## RPCs (Remote Procedure Calls)

### Server RPC — Client calls, Server executes

```cpp
UFUNCTION(Server, Reliable, WithValidation)
void ServerFire(FVector_NetQuantize Direction);

// Must implement _Implementation and _Validate
void AMyActor::ServerFire_Implementation(FVector_NetQuantize Direction)
{
    // Runs on server
    SpawnProjectile(Direction);
}

bool AMyActor::ServerFire_Validate(FVector_NetQuantize Direction)
{
    // Return false to disconnect cheating client
    return Direction.SizeSquared() <= 1.1f;
}
```

### Client RPC — Server calls, Client executes

```cpp
UFUNCTION(Client, Reliable)
void ClientShowDamageNumber(float Amount, FVector Location);

void AMyActor::ClientShowDamageNumber_Implementation(float Amount, FVector Location)
{
    // Runs on owning client only
    SpawnDamageWidget(Amount, Location);
}
```

### Multicast RPC — Server calls, All clients execute

```cpp
UFUNCTION(NetMulticast, Unreliable)
void MulticastPlayHitEffect(FVector Location, FRotator Rotation);

void AMyActor::MulticastPlayHitEffect_Implementation(FVector Location, FRotator Rotation)
{
    // Runs on server AND all clients
    UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HitFX, Location, Rotation);
}
```

### Reliable vs Unreliable

| | Reliable | Unreliable |
|-|----------|-----------|
| **Guarantees delivery** | Yes (ordered) | No |
| **Use for** | Important state changes | Frequent cosmetic updates |
| **Examples** | Equip weapon, use ability | Play hit effect, footstep |
| **Bandwidth** | Higher (retry overhead) | Lower |

**Rule:** Never use Reliable for frequent calls (e.g., every tick). Will saturate the reliable buffer and disconnect.

## Net Roles

```cpp
ENetRole Role = GetLocalRole();
ENetRole RemoteRole = GetRemoteRole();

// ROLE_Authority     — server-authoritative instance
// ROLE_AutonomousProxy — locally controlled (owning client)
// ROLE_SimulatedProxy — replicated to non-owning clients
// ROLE_None          — not replicated

bool bIsServer = HasAuthority();
bool bIsLocallyControlled = IsLocallyControlled(); // Pawn
```

## Network-Optimized Types

```cpp
FVector_NetQuantize      // quantized FVector (1 decimal precision)
FVector_NetQuantize10    // 10-bit per component
FVector_NetQuantize100   // 100x precision
FVector_NetQuantizeNormal // unit vector, 16-bit per component
FRotator                 // auto-compressed for replication

// Quantize custom
UPROPERTY(Replicated, meta=(NetQuantize))
float MyValue;
```

## Replication Graph

For large-scale games (100+ actors), use Replication Graph to control which actors replicate to which connections. Override `UReplicationDriver`.

## Common Patterns

### Authority Check

```cpp
void AMyActor::TakeDamage(float Amount)
{
    if (!HasAuthority()) return; // only process on server

    Health -= Amount;
    if (Health <= 0.f)
    {
        MulticastPlayDeathEffect();
    }
}
```

### Ownership for RPCs

Server RPCs require the calling client to own the actor (or its Pawn must own it via Controller).

```cpp
// Set owner
SetOwner(PlayerController);

// Pawn ownership chain: Controller → Pawn → WeaponActor
// Server RPCs on WeaponActor work because ownership traces to Controller
```
