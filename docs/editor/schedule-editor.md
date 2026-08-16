# Schedule Editor

The Schedule Editor is where you create events and time ranges. Double-click a
Day/Night Cycle Schedule asset to open it.

<!-- TODO: Add a real Schedule Editor screenshot showing the List and Timeline. -->

## Add an entry

Use **Add Event** or **Add Time Range**.

| Entry | Use it for |
| --- | --- |
| Notify Event | Sunrise, shop opening, an NPC routine, a daily signal |
| Blocking Event / Gate | Wait for dialogue, loading, voting, or another game action |
| Time Range | Daylight, business hours, a curfew, an overnight state |

Select an entry to edit it in the inspector.

## Main properties

- **Name** is only for people reading the schedule.
- **Tags** are optional labels used by Blueprint searches and listeners.
- **Time** chooses when an event happens.
- **Start / End** choose when a range is active.
- **Recurrence** controls which day or days use the entry.
- **Behavior** makes an event Notify or Block Time.
- **Color** only changes editor presentation.

## Recurrence

| Desired result | Setup |
| --- | --- |
| Every day | Repeating · Anchor Day 1 · Interval 1 |
| Every 7 days starting on Day 4 | Repeating · Anchor Day 4 · Interval 7 |
| Only on Day 19 | Once · Anchor Day 19 |

An overnight range is allowed. A range from 22:00 to 06:00 starts on the
scheduled day and ends the following morning.

## List and Timeline

**List** is best for quick searching, filtering, renaming, duplicating, deleting,
and manual ordering.

**Timeline** is best for seeing when entries overlap. Use the bottom range
control to pan or zoom between full days. The 1, 7, and 14 Day buttons are quick
presets; wider views are also supported.

- **Current** focuses the configured day, or the live day during PIE.
- **Reset** restores the normal 7-day view, hour height, and scroll position.
- Shift+wheel and middle-drag pan horizontally.
- Hour Height changes vertical zoom only.

Timing is edited in the inspector; Timeline entries are not draggable.

## Finish cleanly

Use **Validate** before saving. Selecting a validation issue focuses the exact
entry that needs attention.

Useful shortcuts include **F2** to rename, **Ctrl+D** to duplicate, Delete, and
**Alt+Up/Down** for manual ordering.
