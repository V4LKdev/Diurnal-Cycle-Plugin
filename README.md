# Day/Night Cycle 0.3.0 beta

Day/Night Cycle is an Unreal Engine 5.8 C++ plugin for an ordinal game clock, authored schedules, recurring events and ranges, blocking time gates, runtime overlays, persistence, Blueprint access, editor tooling, and Gameplay Debugger integration.

## Schedule model

Create `UDiurnalSchedule` primary assets and assign an ordered set to **Project Settings > Plugins > Day/Night Cycle > Default Schedules**. An empty array means no authored default schedule. Runtime-added entries and disabled-entry overlays remain independent of their source assets.

Every entry has an `EntryId`, a designer-facing name, an `FDiurnalRecurrence` (`Once` or `Repeating`, with anchor and interval), and optional `EventTags` or `RangeTags`. Tags are many-to-many semantic metadata: entries may be tagless and several entries may share a tag. Exact authored operations use `FDiurnalScheduleEntryReference`; exact blocking occurrences use `FDiurnalEventOccurrenceHandle`. Tag queries and explicitly named matching-tag batch nodes operate on semantic sets.

Schedule assets are Primary Assets. If the project has configured the `DiurnalSchedule` Primary Asset type, that configuration is authoritative. Otherwise the plugin registers `/Game/` plus mounted content roots from project-installed plugins and marks discovered schedules for cooking; it does not scan Engine content or Engine plugins. Schedule activation is synchronous and may load an unloaded asset; default layers are prepared during subsystem initialization and runtime-triggered loads are logged with their asset path.

## Editor

The Schedule Browser is a read-only merged view of the configured default layers. Double-click a row to open its owning schedule. Each schedule has a transactional List/Timeline editor with exact-entry selection, structured validation, search, filters, sorting, Sequencer-style Working/View day navigation, and independent 24–120 px/hour vertical zoom. Timeline navigation state is editor-only and never changes schedule or runtime semantics.

Project Settings contains clock/world options, `DefaultSchedules`, validation help, and the Schedule Browser button. The PIE toolbar updates its live clock directly without rebuilding the toolbar.

## Runtime and persistence

The clock supports explicit pause, time scale, world advancement policy, manual advancement, and safe maximum-date handling. An attempted advance beyond the representable date pauses the clock and emits one contextual warning until the clock is resumed or repositioned.

Persistence starts at schema version 1. `FDiurnalClockState` and `FDiurnalScheduleRuntimeState` may be captured independently; `FDiurnalCycleState` captures them atomically. The current schema preserves clock configuration, exact schedule references, dynamic entries, disabled-entry overlays, and active exact gate occurrences.

## Blueprint guidance

Search for **Day Night Cycle** nodes. Use exact-reference nodes when one authored entry matters, and occurrence-handle nodes when releasing one blocker. Semantic tag results are unique aggregate tags, may represent several contributors, and omit tagless entries; use exact active-entry/occurrence queries when identity matters. Event waits report `Failed` for invalid configuration or when all matching occurrences are already in the past, `Invalidated` when a started wait becomes unreachable, and `Triggered` on success. `Cancel` is the explicit runtime cancellation path.

See [Designer Guide](Docs/DesignerGuide.md) and [Blueprint Reference](Docs/BP_DiurnalCycleReference.md).

## Intentionally deferred

The beta does not include drag scheduling, resize handles, additional calendar modes, countdown systems, runtime clock controls beyond the current API, or an asynchronous schedule-activation state machine. The Browser opens the owning editor, but direct cross-toolkit exact-row focus remains an optional enhancement.
