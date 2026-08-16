#include "ScheduleEditor/Projection/DiurnalScheduleProjection.h"

#include "DiurnalSchedule.h"
#include "ScheduleEditor/DiurnalScheduleEditorPresentation.h"

namespace
{
	int32 CompareGuid(const FGuid& Left, const FGuid& Right)
	{
		if (Left.A != Right.A) return Left.A < Right.A ? -1 : 1;
		if (Left.B != Right.B) return Left.B < Right.B ? -1 : 1;
		if (Left.C != Right.C) return Left.C < Right.C ? -1 : 1;
		if (Left.D != Right.D) return Left.D < Right.D ? -1 : 1;
		return 0;
	}

	template <typename TProjected>
	int32 CompareProvenance(const TProjected& Left, const TProjected& Right)
	{
		if (Left.SourceLayer != Right.SourceLayer) return Left.SourceLayer < Right.SourceLayer ? -1 : 1;
		const int32 PathCompare = Left.EntryReference.Schedule.ToSoftObjectPath().ToString().Compare(
			Right.EntryReference.Schedule.ToSoftObjectPath().ToString(), ESearchCase::CaseSensitive);
		if (PathCompare != 0) return PathCompare;
		return CompareGuid(Left.EntryReference.EntryId, Right.EntryReference.EntryId);
	}

	FDiurnalScheduleEntryReference MakeReference(const FDiurnalScheduleProjectionLayer& Layer, const FGuid& EntryId)
	{
		FDiurnalScheduleEntryReference Result;
		Result.Schedule = Layer.SourceSchedule;
		Result.EntryId = EntryId;
		return Result;
	}

	FProjectedDiurnalEvent MakeEvent(
		const FDiurnalScheduleProjectionLayer& Layer,
		const FDiurnalTimeEvent& Event,
		const int32 Day)
	{
		FProjectedDiurnalEvent Result;
		Result.EntryReference = MakeReference(Layer, Event.EntryId);
		Result.SourceSchedule = Layer.SourceSchedule;
		Result.SourceDisplayName = Layer.SourceDisplayName;
		Result.SourceLayer = Layer.LayerOrder;
		Result.Provenance = Layer.Provenance;
		Result.bReadOnly = Layer.bReadOnly;
		Result.DisplayName = Event.GetDisplayName();
		Result.Tags = Event.EventTags;
		Result.VisibleDay = Day;
		Result.TimeOfDay = Event.TimeOfDay;
		Result.Behavior = Event.Behavior;
		Result.bIsRepeatingOccurrence = Event.Recurrence.Mode == EDiurnalRecurrenceMode::Repeating;
		Result.EditorColor = DiurnalScheduleEditor::GetEditorColor(Event);
		return Result;
	}

	FProjectedDiurnalRangeSegment MakeRange(
		const FDiurnalScheduleProjectionLayer& Layer,
		const FDiurnalTimeRange& Range,
		const int32 Day,
		const int32 StartSecond,
		const int32 EndSecond,
		const bool bFromPrevious,
		const bool bIntoNext)
	{
		FProjectedDiurnalRangeSegment Result;
		Result.EntryReference = MakeReference(Layer, Range.EntryId);
		Result.SourceSchedule = Layer.SourceSchedule;
		Result.SourceDisplayName = Layer.SourceDisplayName;
		Result.SourceLayer = Layer.LayerOrder;
		Result.Provenance = Layer.Provenance;
		Result.bReadOnly = Layer.bReadOnly;
		Result.DisplayName = Range.GetDisplayName();
		Result.Tags = Range.RangeTags;
		Result.VisibleDay = Day;
		Result.StartSecond = StartSecond;
		Result.EndSecond = EndSecond;
		Result.bContinuesFromPreviousDay = bFromPrevious;
		Result.bContinuesIntoNextDay = bIntoNext;
		Result.bIsRepeatingOccurrence = Range.Recurrence.Mode == EDiurnalRecurrenceMode::Repeating;
		Result.EditorColor = DiurnalScheduleEditor::GetEditorColor(Range);
		return Result;
	}

	void AssignEventLanes(TArray<FProjectedDiurnalEvent>& Events)
	{
		Events.Sort([](const FProjectedDiurnalEvent& Left, const FProjectedDiurnalEvent& Right)
		{
			const int32 LeftSecond = Left.TimeOfDay.ToSecondsIntoDay();
			const int32 RightSecond = Right.TimeOfDay.ToSecondsIntoDay();
			if (LeftSecond != RightSecond) return LeftSecond < RightSecond;
			return CompareProvenance(Left, Right) < 0;
		});
		for (int32 Begin = 0; Begin < Events.Num();)
		{
			int32 End = Begin + 1;
			const int32 Second = Events[Begin].TimeOfDay.ToSecondsIntoDay();
			while (End < Events.Num() && Events[End].TimeOfDay.ToSecondsIntoDay() == Second) ++End;
			const int32 Count = End - Begin;
			for (int32 Index = Begin; Index < End; ++Index)
			{
				Events[Index].CollisionLane = Index - Begin;
				Events[Index].CollisionLaneCount = Count;
			}
			Begin = End;
		}
	}

	void AssignRangeLanes(TArray<FProjectedDiurnalRangeSegment>& Segments)
	{
		Segments.Sort([](const FProjectedDiurnalRangeSegment& Left, const FProjectedDiurnalRangeSegment& Right)
		{
			if (Left.StartSecond != Right.StartSecond) return Left.StartSecond < Right.StartSecond;
			if (Left.EndSecond != Right.EndSecond) return Left.EndSecond < Right.EndSecond;
			return CompareProvenance(Left, Right) < 0;
		});
		for (int32 GroupBegin = 0; GroupBegin < Segments.Num();)
		{
			int32 GroupEnd = GroupBegin + 1;
			int32 ConnectedEnd = Segments[GroupBegin].EndSecond;
			while (GroupEnd < Segments.Num() && Segments[GroupEnd].StartSecond < ConnectedEnd)
			{
				ConnectedEnd = FMath::Max(ConnectedEnd, Segments[GroupEnd].EndSecond);
				++GroupEnd;
			}

			TArray<int32> LaneEndSeconds;
			for (int32 Index = GroupBegin; Index < GroupEnd; ++Index)
			{
				int32 Lane = 0;
				while (Lane < LaneEndSeconds.Num() && LaneEndSeconds[Lane] > Segments[Index].StartSecond) ++Lane;
				if (Lane == LaneEndSeconds.Num()) LaneEndSeconds.Add(Segments[Index].EndSecond);
				else LaneEndSeconds[Lane] = Segments[Index].EndSecond;
				Segments[Index].OverlapLane = Lane;
			}
			for (int32 Index = GroupBegin; Index < GroupEnd; ++Index) Segments[Index].OverlapLaneCount = LaneEndSeconds.Num();
			GroupBegin = GroupEnd;
		}
	}
}

FDiurnalScheduleProjectionLayer FDiurnalScheduleProjectionLayer::FromSchedule(
	UDiurnalSchedule& Schedule,
	const int32 LayerOrder,
	const bool bReadOnly)
{
	FDiurnalScheduleProjectionLayer Result;
	Result.SourceSchedule = &Schedule;
	Result.SourceDisplayName = Schedule.GetName();
	Result.LayerOrder = LayerOrder;
	Result.Provenance = EDiurnalScheduleProjectionProvenance::AuthoredAsset;
	Result.bReadOnly = bReadOnly;
	Result.Events = Schedule.TimeEvents;
	Result.Ranges = Schedule.TimeRanges;
	return Result;
}

const FDiurnalProjectedDay* FDiurnalScheduleProjectionResult::FindDay(const int32 Day) const
{
	return Days.FindByPredicate([Day](const FDiurnalProjectedDay& Candidate) { return Candidate.Day == Day; });
}

FDiurnalScheduleProjectionResult FDiurnalScheduleProjection::Build(const FDiurnalScheduleProjectionRequest& Request)
{
	FDiurnalScheduleProjectionResult Result;
	Result.FirstVisibleDay = FMath::Max(1, Request.FirstVisibleDay);
	Result.VisibleDayCount = FMath::Max(0, Request.VisibleDayCount);
	Result.RuntimeCursor = Request.RuntimeCursor;
	Result.Days.Reserve(Result.VisibleDayCount);
	for (int32 Offset = 0; Offset < Result.VisibleDayCount; ++Offset)
	{
		FDiurnalProjectedDay& Day = Result.Days.AddDefaulted_GetRef();
		Day.Day = Result.FirstVisibleDay + Offset;
		Day.bIsRuntimeDay = Request.RuntimeCursor.IsSet() && Request.RuntimeCursor->Day == Day.Day;
	}

	for (const FDiurnalScheduleProjectionLayer& Layer : Request.Layers)
	{
		for (const FDiurnalTimeEvent& Event : Layer.Events)
		{
			if (!Event.EntryId.IsValid() || !Event.IsValid() || !Request.Filter.MatchesEvent(Event, Layer.SourceDisplayName)) continue;
			for (FDiurnalProjectedDay& Day : Result.Days)
			{
				if (Event.OccursOnDay(Day.Day)) Day.Events.Add(MakeEvent(Layer, Event, Day.Day));
			}
		}

		for (const FDiurnalTimeRange& Range : Layer.Ranges)
		{
			if (!Range.EntryId.IsValid() || !Range.IsValid() || !Request.Filter.MatchesRange(Range, Layer.SourceDisplayName)) continue;
			const int32 StartSecond = Range.StartTime.ToSecondsIntoDay();
			const int32 EndSecond = Range.EndTime.ToSecondsIntoDay();
			for (FDiurnalProjectedDay& Day : Result.Days)
			{
				if (StartSecond < EndSecond)
				{
					if (Range.OccursOnDay(Day.Day))
					{
						Day.RangeSegments.Add(MakeRange(Layer, Range, Day.Day, StartSecond, EndSecond, false, false));
					}
				}
				else
				{
					// Morning belongs to a range that began on the previous authored occurrence day.
					if (EndSecond > 0 && Day.Day > 1 && Range.OccursOnDay(Day.Day - 1))
					{
						Day.RangeSegments.Add(MakeRange(Layer, Range, Day.Day, 0, EndSecond, true, false));
					}
					if (Range.OccursOnDay(Day.Day))
					{
						Day.RangeSegments.Add(MakeRange(Layer, Range, Day.Day, StartSecond, static_cast<int32>(DiurnalCycle::GSecondsPerDay), false, true));
					}
				}
			}
		}
	}

	for (FDiurnalProjectedDay& Day : Result.Days)
	{
		AssignEventLanes(Day.Events);
		AssignRangeLanes(Day.RangeSegments);
	}
	return Result;
}
