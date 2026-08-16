# Blueprint guide

Right-click in a Blueprint graph and search for **Day Night Cycle**. The most
useful nodes are grouped by Clock, Events, Time Ranges, Gates, Schedules, and
State.

<!-- TODO: Add a real Blueprint screenshot with a small clock/event example. -->

## Read and control the clock

| What you need | Node |
| --- | --- |
| Current day and time | `Get Day Night Cycle Date Time` |
| Current day only | `Get Day Night Cycle Current Day` |
| Time of day only | `Get Day Night Cycle Time Of Day` |
| A value from 0 to 1 for the current day | `Get Day Night Cycle Day Progress` |
| Pause or resume | `Set Day Night Cycle Paused` |
| Change clock speed | `Set Day Night Cycle Time Scale` |
| Jump to a date and time | `Set Day Night Cycle Date Time` |

`Day Progress` is convenient for material parameters, lighting blends, and UI.

## React to something

There are two simple approaches.

### Listen for the whole lifetime of an object

Use `Get Day Night Cycle Notifications`, then bind the delegate you need. This
fits managers, UI, lighting controllers, and other objects that should keep
listening.

### Wait for one result

Use an async node when one Blueprint task should wait and then continue:

- `Wait for Day Night Cycle Event`
- `Wait for Day Night Cycle Time Range Entered`
- `Wait for Day Night Cycle Time Range Exited`
- `Wait for Day Night Cycle Time Gate Activated`
- `Wait for Day Night Cycle Time Gate Released`

An event wait can fail when its setup is invalid or all matching occurrences are
already in the past. It becomes **Invalidated** if a valid wait can no longer be
reached. **Cancel** stops it deliberately.

## Tags and exact entries

Use a Gameplay Tag when you mean “anything labelled this way.” For example,
several ranges may use `World.Weather.Cold`.

Use an **Entry Reference** when you mean one exact event or range. Use an
**Occurrence Handle** when you mean one exact active gate.

!!! important
    A tag is a shared label, not a unique ID. A tag operation can find or change
    several entries.

## Events, ranges, and gates

- `Get Day Night Cycle Events` and `Get Day Night Cycle Time Ranges` return the
  current compiled entries.
- `Get Next Day Night Cycle Event Occurrence` finds the next exact occurrence.
- Active tag nodes give a simple semantic overview.
- Exact active-range and active-gate nodes preserve individual identity.
- Release one gate with its occurrence handle. Use the matching-tag or Release
  All nodes only when every matching gate should continue.

## Runtime additions

`Add Day Night Cycle Event` and `Add Day Night Cycle Time Range` create temporary
runtime entries. Keep the returned Entry Reference if you may remove that exact
entry later.

These additions do not edit the schedule asset.

## Change schedule layers

- `Set Active Day Night Cycle Schedules` replaces the complete active set.
- `Activate Day Night Cycle Schedule` adds one layer.
- `Deactivate Day Night Cycle Schedule` removes one layer.

Schedule activation may load the asset immediately, so it is best done during a
normal loading or setup moment.

## Save and restore

`Capture Day Night Cycle State` returns the complete clock and schedule runtime
state. Store it in your own SaveGame object. Later, pass it to
`Restore Day Night Cycle State`.

Restoring does not replay every event that happened between the old and restored
times.
