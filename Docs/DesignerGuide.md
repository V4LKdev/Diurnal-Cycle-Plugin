# Day/Night Cycle designer guide

This guide covers the 0.3.0 beta for Unreal Engine 5.8.

Create a **Diurnal Schedule** asset, author events and ranges in its List or Timeline editor, then add the asset to **Default Schedules** in Project Settings. Layers compose in their configured order. Clearing `DefaultSchedules` produces an empty authored schedule.

Events and ranges use `FDiurnalRecurrence`: **Once** occurs on its anchor day; **Repeating** occurs from its anchor every `IntervalDays`. Tags are optional semantic labels and are not unique identity. Use several tags for classification and reuse a tag across contributors when useful.

Notify events dispatch when forward time crosses their occurrence. Block Time events also create exact gate occurrence handles and hold advancement until those handles are released. Tagless blockers are supported. Ranges are start-inclusive, end-exclusive, may cross midnight, and may overlap.

The Browser is read-only and refreshes for default-setting changes and relevant schedule asset edits, replacement, rename, or deletion. Search, filters, sorting, and surviving exact selection are preserved across refreshes. Open the source schedule to edit it.

The Timeline provides 1, 7, and 14-day quick presets and supports any complete-day View span up to six weeks. Its editor-only Working Range bounds navigation, while its View Range selects the rendered days through native Sequencer-style range interaction and compact integer-day fields. Shift-wheel and middle-drag pan that same View Range. Working/View ranges do not affect recurrence or runtime behavior, and independent 24–120 pixels-per-hour vertical zoom preserves them. Validation messages carry exact entry IDs, so selecting an issue focuses the corresponding event or range without parsing message text.

For Blueprint work, use exact-reference operations for a particular authored entry, occurrence handles for a particular gate, and matching-tag batch/query nodes for semantic groups. Aggregate tag queries return unique tags, can combine multiple contributors, and omit tagless entries.

Deferred beta work: drag/resize authoring, broader calendar modes, countdown systems, asynchronous schedule activation, and direct Browser-to-exact-row focus across editor toolkits.
