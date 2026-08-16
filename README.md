# Day/Night Cycle

A lightweight Unreal Engine 5.8 C++ plugin for a persistent game clock,
reusable schedules, events, time ranges, blocking gates, save-state support,
Blueprint access, editor tooling, and Gameplay Debugger integration.

> Beta software. APIs and saved-state formats may change before 1.0.

## Install

1. Download the Unreal Engine 5.8 Win64 bundle from
   [GitHub Releases](https://github.com/V4LKdev/Diurnal-Cycle-Plugin/releases).
2. Extract `DiurnalCycle` to `<Project>/Plugins/DiurnalCycle`.
3. Enable **Day/Night Cycle** in **Edit > Plugins**.
4. Restart Unreal Editor.
5. Open **Project Settings > Plugins > Day Night Cycle**.

## Configure

Set the starting time and clock speed in Project Settings, then create a
**Day/Night Cycle Schedule** in the Content Browser. Add events or ranges and
place the schedule in `DefaultSchedules`.

Gameplay Tags are optional labels. Keep an Entry Reference or Occurrence Handle
when Blueprint code needs to target one exact entry or gate.

## Use

- Open a schedule asset to edit its List or Timeline.
- Open the Schedule Browser to see the combined default schedules.
- Search for **Day Night Cycle** nodes in Blueprint.
- Use **Capture Day Night Cycle State** and **Restore Day Night Cycle State** in
  your own SaveGame flow.
- Enable the **DiurnalCycle** Gameplay Debugger category while testing.

## Documentation

[Read the documentation](https://v4lkdev.github.io/Diurnal-Cycle-Plugin/).

## License

[MIT](LICENSE) © 2026 Nicolas Martin. Third-party notices are listed in
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
