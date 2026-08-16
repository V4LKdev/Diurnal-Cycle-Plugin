# Day/Night Cycle

A lightweight Unreal Engine 5.8 C++ plugin for a persistent game clock,
reusable schedules, events, time ranges, blocking gates, and save-state support.

> Beta software. APIs and saved-state formats may change before 1.0.

## Install and configure

1. Copy the plugin to `<Project>/Plugins/DiurnalCycle`.
2. Enable **Day/Night Cycle**, restart the editor, and compile when prompted.
3. Open **Project Settings > Plugins > Day Night Cycle**.
4. Choose the starting time and clock speed.
5. Create a **Day/Night Cycle Schedule** in the Content Browser.
6. Add events or ranges, then place the asset in `DefaultSchedules`.

That is enough to run an authored schedule in PIE.

<!-- TODO: Add a real Project Settings screenshot. -->

## Use the plugin

| I want to… | Go to… |
| --- | --- |
| Create or change events and ranges | [Schedule Editor](editor/schedule-editor.md) |
| See everything contributed by the default schedules | [Schedule Browser](editor/schedule-browser.md) |
| Read the time or react to schedules in a graph | [Blueprint guide](blueprints.md) |

## Useful terms

- **Event** — happens once at a specific time.
- **Time Range** — stays active between a start and end time.
- **Gate** — a blocking event that pauses time until the game releases it.
- **Schedule** — a reusable asset containing events and ranges.

Gameplay Tags are optional labels for finding related entries. They are not
unique IDs, so several entries can use the same tag.
