#include "Blueprint/DiurnalCycleBlueprintSubsystem.h"

#include "Subsystem/DiurnalCycleSubsystem.h"

#include "Subsystems/SubsystemCollection.h"

#pragma region USubsystem

void UDiurnalCycleBlueprintSubsystem::Initialize(
	FSubsystemCollectionBase& Collection)
{
	Super::Initialize(
		Collection);

	/*
	 * Both systems are game-instance subsystems. Initializing the dependency
	 * guarantees the native clock exists before this Blueprint bridge binds.
	 */
	UDiurnalCycleSubsystem* Subsystem =
		Collection.InitializeDependency<
			UDiurnalCycleSubsystem>();

	check(Subsystem);

	NativeSubsystem =
		Subsystem;

	Subsystem->OnTimeChanged().AddUObject(
		this,
		&UDiurnalCycleBlueprintSubsystem::
			HandleTimeChanged);

	Subsystem->OnTimeEventTriggered().AddUObject(
		this,
		&UDiurnalCycleBlueprintSubsystem::
			HandleTimeEventTriggered);

	Subsystem->OnTimeEventAdded().AddUObject(
		this,
		&UDiurnalCycleBlueprintSubsystem::
			HandleTimeEventAdded);

	Subsystem->OnTimeEventRemoved().AddUObject(
		this,
		&UDiurnalCycleBlueprintSubsystem::
			HandleTimeEventRemoved);

	Subsystem->OnPauseStateChanged().AddUObject(
		this,
		&UDiurnalCycleBlueprintSubsystem::
			HandlePauseStateChanged);

	Subsystem->OnTimeScaleChanged().AddUObject(
		this,
		&UDiurnalCycleBlueprintSubsystem::
			HandleTimeScaleChanged);

	Subsystem->OnTimeRangeAdded().AddUObject(
		this,
		&UDiurnalCycleBlueprintSubsystem::
			HandleTimeRangeAdded);

	Subsystem->OnTimeRangeRemoved().AddUObject(
		this,
		&UDiurnalCycleBlueprintSubsystem::
			HandleTimeRangeRemoved);

	Subsystem->OnTimeRangeEntered().AddUObject(
		this,
		&UDiurnalCycleBlueprintSubsystem::
			HandleTimeRangeEntered);

	Subsystem->OnTimeRangeExited().AddUObject(
		this,
		&UDiurnalCycleBlueprintSubsystem::
			HandleTimeRangeExited);

	Subsystem->OnTimeGateActivated().AddUObject(
		this,
		&UDiurnalCycleBlueprintSubsystem::
			HandleTimeGateActivated);

	Subsystem->OnTimeGateReleased().AddUObject(
		this,
		&UDiurnalCycleBlueprintSubsystem::
			HandleTimeGateReleased);
}

void UDiurnalCycleBlueprintSubsystem::Deinitialize()
{
	if (UDiurnalCycleSubsystem* Subsystem =
		NativeSubsystem.Get())
	{
		Subsystem->OnTimeChanged().RemoveAll(
			this);

		Subsystem->OnTimeEventTriggered().RemoveAll(
			this);

		Subsystem->OnTimeEventAdded().RemoveAll(
			this);

		Subsystem->OnTimeEventRemoved().RemoveAll(
			this);

		Subsystem->OnPauseStateChanged().RemoveAll(
			this);

		Subsystem->OnTimeScaleChanged().RemoveAll(
			this);

		Subsystem->OnTimeRangeAdded().RemoveAll(
			this);

		Subsystem->OnTimeRangeRemoved().RemoveAll(
			this);

		Subsystem->OnTimeRangeEntered().RemoveAll(
			this);

		Subsystem->OnTimeRangeExited().RemoveAll(
			this);

		Subsystem->OnTimeGateActivated().RemoveAll(
			this);

		Subsystem->OnTimeGateReleased().RemoveAll(
			this);
	}

	NativeSubsystem.Reset();

	/*
	 * Explicitly clear Blueprint listeners before the subsystem is torn down.
	 */
	OnTimeChanged.Clear();
	OnPauseStateChanged.Clear();
	OnTimeScaleChanged.Clear();

	OnTimeEventTriggered.Clear();
	OnTimeEventAdded.Clear();
	OnTimeEventRemoved.Clear();

	OnTimeRangeAdded.Clear();
	OnTimeRangeRemoved.Clear();
	OnTimeRangeEntered.Clear();
	OnTimeRangeExited.Clear();

	OnTimeGateActivated.Clear();
	OnTimeGateReleased.Clear();

	Super::Deinitialize();
}

#pragma endregion

#pragma region NativeHandlers

void UDiurnalCycleBlueprintSubsystem::HandleTimeChanged(
	const FDiurnalTimeChange& Change)
{
	OnTimeChanged.Broadcast(
		Change.PreviousDateTime,
		Change.CurrentDateTime,
		Change.Reason);
}

void UDiurnalCycleBlueprintSubsystem::
HandleTimeEventTriggered(
	const FDiurnalTimeEvent& TimeEvent,
	const FDiurnalDateTime& OccurrenceTime)
{
	OnTimeEventTriggered.Broadcast(
		TimeEvent,
		OccurrenceTime);
}

void UDiurnalCycleBlueprintSubsystem::
HandleTimeEventAdded(
	const FDiurnalTimeEvent& TimeEvent)
{
	OnTimeEventAdded.Broadcast(
		TimeEvent);
}

void UDiurnalCycleBlueprintSubsystem::
HandleTimeEventRemoved(
	const FDiurnalTimeEvent& TimeEvent)
{
	OnTimeEventRemoved.Broadcast(
		TimeEvent);
}

void UDiurnalCycleBlueprintSubsystem::
HandlePauseStateChanged(
	const bool bIsPaused)
{
	OnPauseStateChanged.Broadcast(
		bIsPaused);
}

void UDiurnalCycleBlueprintSubsystem::
HandleTimeScaleChanged(
	const double PreviousTimeScale,
	const double NewTimeScale)
{
	OnTimeScaleChanged.Broadcast(
		PreviousTimeScale,
		NewTimeScale);
}

void UDiurnalCycleBlueprintSubsystem::
HandleTimeRangeAdded(
	const FDiurnalTimeRange& TimeRange)
{
	OnTimeRangeAdded.Broadcast(
		TimeRange);
}

void UDiurnalCycleBlueprintSubsystem::
HandleTimeRangeRemoved(
	const FDiurnalTimeRange& TimeRange)
{
	OnTimeRangeRemoved.Broadcast(
		TimeRange);
}

void UDiurnalCycleBlueprintSubsystem::
HandleTimeRangeEntered(
	const FDiurnalTimeRange& TimeRange,
	const FDiurnalDateTime& CurrentDateTime)
{
	OnTimeRangeEntered.Broadcast(
		TimeRange,
		CurrentDateTime);
}

void UDiurnalCycleBlueprintSubsystem::
HandleTimeRangeExited(
	const FDiurnalTimeRange& TimeRange,
	const FDiurnalDateTime& CurrentDateTime)
{
	OnTimeRangeExited.Broadcast(
		TimeRange,
		CurrentDateTime);
}

void UDiurnalCycleBlueprintSubsystem::
HandleTimeGateActivated(
	const FDiurnalTimeEvent& TimeEvent,
	const FDiurnalDateTime& ActivationTime)
{
	OnTimeGateActivated.Broadcast(
		TimeEvent,
		ActivationTime);
}

void UDiurnalCycleBlueprintSubsystem::
HandleTimeGateReleased(
	const FGameplayTag GateTag,
	const FDiurnalDateTime& ReleaseTime)
{
	OnTimeGateReleased.Broadcast(
		GateTag,
		ReleaseTime);
}

#pragma endregion