#pragma once

#if WITH_GAMEPLAY_DEBUGGER

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "GameplayDebuggerCategory.h"

class UDiurnalCycleSubsystem;
class UDiurnalCycleWorldSubsystem;

/**
 * Gameplay Debugger category for inspecting and controlling the active
 * day-night-cycle runtime.
 *
 * The category is global to the owning game instance and does not require a
 * selected debug actor. Collected state is replicated through the Gameplay
 * Debugger so clients inspect the authoritative debugger-side clock state.
 */
class FGameplayDebuggerCategory_DiurnalCycle final
	: public FGameplayDebuggerCategory
{
public:
	FGameplayDebuggerCategory_DiurnalCycle();

#pragma region FGameplayDebuggerCategory

	virtual void CollectData(
		APlayerController* OwnerPC,
		AActor* DebugActor) override;

	virtual void DrawData(
		APlayerController* OwnerPC,
		FGameplayDebuggerCanvasContext& CanvasContext) override;

#pragma endregion

#pragma region Factory

	static TSharedRef<FGameplayDebuggerCategory> MakeInstance();

#pragma endregion

private:
#pragma region InputActions

	void TogglePaused();
	void RewindHour();
	void AdvanceHour();

	void DecreaseTimeScale();
	void IncreaseTimeScale();

	void ReleaseTimeGates();
	void CycleWorldTimePolicyOverride();

#pragma endregion

#pragma region RuntimeResolution

	UDiurnalCycleSubsystem* GetSubsystem() const;
	UDiurnalCycleWorldSubsystem* GetWorldSubsystem() const;

#pragma endregion

#pragma region ReplicatedData

	struct FRepData
	{
		bool bAvailable = false;

		bool bPaused = false;
		bool bBlockedByTimeGate = false;

		bool bWorldPolicyAvailable = false;
		bool bWorldAllowsAdvancement = false;
		bool bHasRuntimeWorldPolicyOverride = false;

		uint8 NetMode = NM_Standalone;
		uint8 ConfiguredWorldPolicy = 0;
		uint8 EffectiveWorldPolicy = 0;

		int32 Day = 1;
		int32 Hour = 0;
		int32 Minute = 0;
		int32 Second = 0;

		double TimeScale = 0.0;
		double TotalGameHours = 0.0;
		double DayProgress = 0.0;

		double BaseRealSecondsPerGameHour = 0.0;
		double EffectiveRealSecondsPerGameHour = 0.0;

		FString WorldName;

		int32 EventCount = 0;
		int32 DailyEventCount = 0;
		int32 DatedEventCount = 0;
		int32 BlockingEventCount = 0;

		int32 TimeRangeCount = 0;
		TArray<FName> ActiveTimeRanges;
		TArray<FName> ActiveTimeGates;

		bool bHasNextEvent = false;
		bool bNextEventDated = false;
		bool bNextEventBlocking = false;

		FName NextEventName = NAME_None;

		int32 NextEventDay = 1;
		int32 NextEventHour = 0;
		int32 NextEventMinute = 0;
		int32 NextEventSecond = 0;

		void Serialize(
			FArchive& Ar);
	};

	FRepData Data;

#pragma endregion

#pragma region RuntimeCache

	/**
	 * Server-side runtime references used by replicated debugger input handlers.
	 *
	 * They are refreshed during CollectData and are never replicated.
	 */
	TWeakObjectPtr<UDiurnalCycleSubsystem> CachedSubsystem;
	TWeakObjectPtr<UDiurnalCycleWorldSubsystem> CachedWorldSubsystem;

#pragma endregion
};

#endif // WITH_GAMEPLAY_DEBUGGER
