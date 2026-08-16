# Schedule Browser

Open the Schedule Browser from the Day/Night Cycle toolbar button or Project
Settings.

<!-- TODO: Add a real Schedule Browser screenshot showing both tabs. -->

## Combined Schedule

This view combines every asset in `DefaultSchedules`, in the same layer order
used by the game. Use Search, Filter, and Sort to find entries across those
schedules.

Select an entry to edit its source schedule. The right-click menu can add,
rename, duplicate, delete, or open the source entry in the full Schedule Editor.

Use the **Save** button after editing. It saves the dirty schedule assets used by
the Browser.

!!! note
    Combined Schedule shows the authored project defaults. Temporary runtime
    entries added during PIE are not included.

## Schedule Assets

This view lists available schedule assets using Unreal's normal asset picker.
From here you can:

- create a schedule with the **+** button;
- open an existing schedule;
- rename it with **F2**;
- duplicate or delete it with the normal asset actions.

Double-clicking an entry in Combined Schedule opens its owning asset and selects
that exact event or range.
