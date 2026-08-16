# Blueprint API reference

The Blueprint surface is organized under **Day Night Cycle**.

## Identity and semantics

- `FDiurnalScheduleEntryReference` identifies one authored or runtime-owned entry by `EntryId`; a null Schedule is valid for runtime-owned entries.
- `FDiurnalEventOccurrenceHandle` identifies one exact event occurrence and is required for exact gate release.
- `EventTags` and `RangeTags` are optional many-to-many semantic containers.
- Exact-reference add/remove/reenable/query nodes act on one entry.
- Matching-tag and tag-query nodes explicitly operate on all semantic matches.
- Active range/gate tag queries return unique aggregate tags, may combine several contributors, and omit tagless entries. Pair them with exact active range-entry and gate-occurrence queries when identity matters.

## Factories and recurrence

Use `MakeOnceTimeEvent`, `MakeRepeatingTimeEvent`, `MakeOneOffDayNightCycleTimeRange`, and `MakeRepeatingDayNightCycleTimeRange`. Recurrence stores mode, anchor day, and repeating interval directly.

## Notifications and waits

The notification subsystem exposes clock, event, range, and exact gate-occurrence events. Gate release uses the exact occurrence handle; no tag is treated as a primary identity. Optional tags remain available on the event data.

Event async actions distinguish outcomes as follows:

- `Triggered`: the requested occurrence was reached.
- `Failed`: target/configuration was invalid, or all matching occurrences were already in the past when activation was attempted.
- `Invalidated`: a valid started wait later became unreachable.
- `Cancel`: explicit runtime cancellation through the async action.

Schedule activation APIs are synchronous and may load an unloaded schedule asset. Use them away from latency-sensitive paths; an asynchronous activation workflow is intentionally deferred.
