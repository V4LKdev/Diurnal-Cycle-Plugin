# Day Night Cycle

A lightweight Unreal Engine 5 plugin for a persistent game clock with scheduled events, recurring time ranges, blocking time gates, save-state support, Blueprint access, editor tooling, and Gameplay Debugger integration.

## Install

1. Put the plugin at `YourProject/Plugins/DiurnalCycle/`.
2. Build the project. The plugin is a C++ source plugin and compiles with the project.
3. Enable **Day Night Cycle** in **Edit > Plugins** if needed, then restart the editor.
4. Open **Project Settings > Plugins > Day Night Cycle**.

## Configure

### Clock
- **Real Seconds Per Game Hour** - base speed at Time Scale `1`.
- **Starting Date Time** - initial game day and time.
- **Default Time Scale** - runtime speed multiplier. `0` stops automatic advancement without pausing.
- **Start Paused** - starts the clock explicitly paused.

### World
- **Advance Time By Default** - fallback for maps without their own policy.
- In the Level Editor toolbar, use the small **world icon** to set the current map to:
    - **Use Project Default**
    - **Advance**
    - **Freeze**

World policy only controls automatic ticking. Manual advance, setting the date/time, and state restoration still work.

### Events
Events use Gameplay Tags under `DiurnalCycle.TimeEvent`.

Each event has an exact time of day, is either daily or tied to one game day, and uses either:
- **Notify** - emit the event normally.
- **Block Time** - emit the event and stop forward time at that timestamp until its active gate is released.

Multiple gates may be active at the same timestamp.

### Time Ranges
Ranges use Gameplay Tags under `DiurnalCycle.TimeRange`.

Ranges are start-inclusive and end-exclusive, may wrap across midnight, may overlap, and can be added or removed at runtime.

## Blueprint

Search for **Day Night Cycle** nodes.

Typical usage:
- **Clock:** get/set date time, pause, change timescale, manually advance.
- **Events:** add/remove/query events and get next occurrences.
- **Ranges:** query or modify ranges and get currently active ranges.
- **Time Gates:** query active gates and release one or all gates.
- **World Policy:** query or temporarily override the current world's advancement policy.
- **Persistence:** use **Capture Day Night Cycle State** and **Restore Day Night Cycle State** inside your own SaveGame flow.
- **Notifications:** call **Get Day Night Cycle Notifications** and bind to clock, event, range, and gate dispatchers.
- **Async:** wait for an event, range enter/exit, or gate activate/release.

Important behavior:
- Normal events fire when crossed during forward advancement.
- Setting/restoring time does **not** replay missed normal events.
- Ranges represent current state and reconcile at the final time.
- Active time gates persist until explicitly released.

For a visual list of every Blueprint-facing node, see `Docs/BP_DiurnalCycleReference.md`.

## Editor Toolbar

The Level Editor toolbar provides:
- the live `Day N, HH:MM:SS` clock while PIE is running,
- the current-map world-policy button,
- a settings button that opens the plugin's Project Settings.

Hover the live clock to see why automatic advancement is running or stopped.

## Gameplay Debugger

Open the Gameplay Debugger and enable the **DiurnalCycle** category.

It shows the current world, exact time, clock state/rates, world policy, event summary and next event, active ranges, and active time gates.

| Key | Action                       |
|-----|------------------------------|
| `P` | Pause / resume               |
| `,` | Rewind 1 game hour           |
| `.` | Advance 1 game hour          |
| `-` | Half time scale              |
| `=` | Double time scale            |
| `G` | Release all active gates     |
| `O` | Cycle runtime world override |

## Saving

The plugin does not create SaveGame slots.

Store `FDiurnalCycleState` inside your game's own SaveGame object:

1. **Capture Day Night Cycle State** before saving.
2. Save the returned struct with the rest of your game state.
3. **Restore Day Night Cycle State** after loading.

Pause state and the configured base rate are intentionally not part of the captured state.

## Distribution

For programmers, keep the source plugin in `Project/Plugins/DiurnalCycle`.

For teammates who should not compile C++, package the plugin from Unreal's Plugin Browser and distribute the packaged plugin folder instead.
