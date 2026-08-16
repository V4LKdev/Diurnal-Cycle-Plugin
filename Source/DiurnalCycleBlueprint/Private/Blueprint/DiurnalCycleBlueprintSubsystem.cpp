#include "Blueprint/DiurnalCycleBlueprintSubsystem.h"

#include "Subsystem/DiurnalCycleSubsystem.h"

// Blueprint notifications bridge native runtime delegates without owning clock state.

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

	Subsystem->OnTimeEventOccurrence().AddUObject(
		this,
		&UDiurnalCycleBlueprintSubsystem::HandleTimeEventOccurrence);

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

	Subsystem->OnTimeRangeEntryEntered().AddUObject(
		this,
		&UDiurnalCycleBlueprintSubsystem::HandleTimeRangeEntryEntered);

	Subsystem->OnTimeRangeEntryExited().AddUObject(
		this,
		&UDiurnalCycleBlueprintSubsystem::HandleTimeRangeEntryExited);

	Subsystem->OnTimeGateActivated().AddUObject(
		this,
		&UDiurnalCycleBlueprintSubsystem::
			HandleTimeGateActivated);

	Subsystem->OnTimeGateOccurrenceActivated().AddUObject(
		this,
		&UDiurnalCycleBlueprintSubsystem::HandleTimeGateOccurrenceActivated);

	Subsystem->OnTimeGateOccurrenceReleased().AddUObject(
		this,
		&UDiurnalCycleBlueprintSubsystem::HandleTimeGateOccurrenceReleased);
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

		Subsystem->OnTimeEventOccurrence().RemoveAll(this);

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

		Subsystem->OnTimeRangeEntryEntered().RemoveAll(this);
		Subsystem->OnTimeRangeEntryExited().RemoveAll(this);

		Subsystem->OnTimeGateActivated().RemoveAll(
			this);

		Subsystem->OnTimeGateOccurrenceActivated().RemoveAll(this);
		Subsystem->OnTimeGateOccurrenceReleased().RemoveAll(this);
	}

	NativeSubsystem.Reset();

	/*
	 * Explicitly clear Blueprint listeners before the subsystem is torn down.
	 */
	OnTimeChanged.Clear();
	OnPauseStateChanged.Clear();
	OnTimeScaleChanged.Clear();

	OnTimeEventTriggered.Clear();
	OnTimeEventOccurrence.Clear();
	OnTimeEventAdded.Clear();
	OnTimeEventRemoved.Clear();

	OnTimeRangeAdded.Clear();
	OnTimeRangeRemoved.Clear();
	OnTimeRangeEntered.Clear();
	OnTimeRangeExited.Clear();
	OnTimeRangeEntryEntered.Clear();
	OnTimeRangeEntryExited.Clear();

	OnTimeGateActivated.Clear();
	OnTimeGateOccurrenceActivated.Clear();
	OnTimeGateOccurrenceReleased.Clear();

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

void UDiurnalCycleBlueprintSubsystem::HandleTimeEventOccurrence(
	const FDiurnalEventOccurrenceHandle& Occurrence,
	const FDiurnalTimeEvent& TimeEvent,
	const FDiurnalDateTime& OccurrenceTime)
{
	OnTimeEventOccurrence.Broadcast(Occurrence, TimeEvent, OccurrenceTime);
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

void UDiurnalCycleBlueprintSubsystem::HandleTimeRangeEntryEntered(
	const FDiurnalScheduleEntryReference& Entry,
	const FDiurnalTimeRange& TimeRange,
	const FDiurnalDateTime& CurrentDateTime)
{
	OnTimeRangeEntryEntered.Broadcast(Entry, TimeRange, CurrentDateTime);
}

void UDiurnalCycleBlueprintSubsystem::HandleTimeRangeEntryExited(
	const FDiurnalScheduleEntryReference& Entry,
	const FDiurnalTimeRange& TimeRange,
	const FDiurnalDateTime& CurrentDateTime)
{
	OnTimeRangeEntryExited.Broadcast(Entry, TimeRange, CurrentDateTime);
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

void UDiurnalCycleBlueprintSubsystem::HandleTimeGateOccurrenceActivated(
	const FDiurnalEventOccurrenceHandle& Occurrence,
	const FDiurnalTimeEvent& TimeEvent)
{
	OnTimeGateOccurrenceActivated.Broadcast(Occurrence, TimeEvent);
}

void UDiurnalCycleBlueprintSubsystem::HandleTimeGateOccurrenceReleased(
	const FDiurnalEventOccurrenceHandle& Occurrence,
	const FDiurnalDateTime& ReleaseTime)
{
	OnTimeGateOccurrenceReleased.Broadcast(Occurrence, ReleaseTime);
}

#pragma endregion
