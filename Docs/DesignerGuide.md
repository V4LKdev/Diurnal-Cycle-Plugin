# Day Night Cycle - Designer Guide

This plugin runs the game's global time of day.

You do not need to set anything up in a level Blueprint or create any special actor. The clock exists automatically while the game is running.

## 1. Basic Setup

Open:

**Project Settings > Plugins > Day Night Cycle**

The most important settings are:

* **Real Seconds Per Game Hour**
  How long one in-game hour takes at normal speed.

* **Starting Date Time**
  The time when a new game session starts.

* **Default Time Scale**
  `1` is normal speed.
  `2` is twice as fast.
  `0.5` is half speed.
  `0` stops automatic time advancement.

* **Start Paused**
  Starts the clock explicitly paused.

You usually configure these once for the project.

---

# 2. Per-Level Time

Each map can decide whether time should advance.

Use the small **world icon** in the Level Editor toolbar.

Choose:

* **Use Project Default**
  Use the global project setting.

* **Advance**
  Time advances normally in this map.

* **Freeze**
  Automatic time advancement stops in this map.

Typical examples:

* Outdoor gameplay map: **Advance**
* Interior where time should still pass: **Advance**
* Main menu: **Freeze**
* Special dream or cutscene level: **Freeze**

### Do not

Do not use the runtime Blueprint world-policy override to permanently configure a level.

Use the **world icon in the editor** for permanent map configuration.

Runtime overrides are for temporary gameplay situations.

---

# 3. Events

Use an **Event** when something should happen at a specific time.

Examples:

* Shops open at 08:00
* Streetlights turn on at 20:00
* A quest becomes available on Day 3 at 12:00
* A scripted encounter starts at midnight

Events use Gameplay Tags under:

`DiurnalCycle.TimeEvent`

An event can be:

### Daily

Repeats every game day.

Example:

`DiurnalCycle.TimeEvent.ShopOpen` at `08:00:00`

### Dated

Happens on one specific game day.

Example:

`DiurnalCycle.TimeEvent.Festival` on Day 5 at `18:00:00`

### Notify

Normal event.

The clock continues after the event triggers.

### Block Time

Creates a **Time Gate**.

The clock reaches that time and stops there until gameplay explicitly releases the gate.

Use this only when the game must not continue through that point automatically.

Example:

At `18:00`, a story sequence must happen before night begins.

---

# 4. Time Ranges

Use a **Time Range** when you care about a period of time rather than one exact moment.

Examples:

* Daytime
* Nighttime
* Shop opening hours
* Curfew
* Morning ambience
* Evening NPC schedules

Ranges use Gameplay Tags under:

`DiurnalCycle.TimeRange`

Example:

`DiurnalCycle.TimeRange.DayTime`

`06:00:00 -> 18:00:00`

A range may also cross midnight.

Example:

`DiurnalCycle.TimeRange.NightTime`

`18:00:00 -> 06:00:00`

Ranges may overlap.

### Use events for moments

"At 18:00, turn the streetlights on."

### Use ranges for states

"While it is nighttime, these enemies can spawn."

---

# 5. Time Gates

A Time Gate is an Event with **Block Time** behavior.

When the clock reaches it:

1. The event triggers.
2. The clock stops at that exact time.
3. Gameplay performs whatever must happen.
4. Gameplay releases the gate.
5. Time may continue.

Multiple gates can be active at the same time.

All active gates must be released before automatic time can continue.

### Good uses

* Wait for a cutscene to finish
* Wait for a mandatory dialogue sequence
* Prevent time passing beyond an important story event
* Synchronize a major gameplay transition

### Do not

Do not use Time Gates just because you want to temporarily pause time.

Use **Pause** for that.

A gate represents a specific scheduled gameplay barrier.

---

# 6. Listening for Changes in Blueprint

For normal gameplay systems, prefer the notification system.

Use:

**Get Day Night Cycle Notifications**

From this object you can bind to:

### Clock

* On Time Changed
* On Pause State Changed
* On Time Scale Changed

### Events

* On Time Event Triggered
* On Time Event Added
* On Time Event Removed

### Ranges

* On Time Range Added
* On Time Range Removed
* On Time Range Entered
* On Time Range Exited

### Gates

* On Time Gate Activated
* On Time Gate Released

Example:

An ambience manager that needs to react whenever nighttime begins should bind to:

**On Time Range Entered**

and check for:

`DiurnalCycle.TimeRange.NightTime`

This is better than checking the clock every frame.

---

# 7. Async Nodes

There are also one-shot Blueprint nodes:

* Wait for Day Night Cycle Event
* Wait for Day Night Cycle Time Range Entered
* Wait for Day Night Cycle Time Range Exited
* Wait for Day Night Cycle Time Gate Activated
* Wait for Day Night Cycle Time Gate Released

Use these when one Blueprint wants to wait for one specific thing once.

Example:

A quest Blueprint waits for:

`DiurnalCycle.TimeEvent.MeetingStart`

and continues when it triggers.

### Do not

Do not use async waits as permanent global listeners.

For systems that need to listen for something throughout the game, use **Get Day Night Cycle Notifications** instead.

Async waits are also not saved automatically. Recreate them after loading if your gameplay requires them.

---

# 8. Changing Time

You can:

* Pause / resume
* Change Time Scale
* Set an exact Date Time
* Advance forward by a number of game hours

There is an important difference between **Advance** and **Set Date Time**.

### Advance

Moves normally through time.

Events between the old time and new time can trigger.

Time Gates can stop the advancement.

### Set Date Time

Teleports directly to another time.

Normal events between the old and new time are not replayed.

Use this for things like:

* Loading a save
* Debugging
* Sleeping until morning
* Explicit time skips

### Do not

Do not use **Set Date Time** when you expect every event between the two times to trigger.

It is a teleport, not simulated passage of time.

---

# 9. Pause vs Time Scale 0 vs Freeze

These are intentionally different.

### Pause

The clock has explicitly been paused.

Use when gameplay says:

"Time is paused."

This should be instrumented by the player, like pressing the pause button.

### Time Scale = 0

The clock is not paused, but its automatic speed is zero.

Use when controlling clock speed dynamically.

This is the "games" dynamic way of pausing, without letting the player instrument a resume.

### World Freeze

The current map does not allow automatic time advancement.

Use for map-level behavior such as menus or special levels.

### Time Gate

The timeline has reached a scheduled gameplay barrier.

Use when something must happen before time is allowed to continue.

Do not treat these as interchangeable.

---

# 10. Saving

The plugin does not manage save slots.

Your game's SaveGame should store the returned:

**Day Night Cycle State**

Use:

**Capture Day Night Cycle State**

when saving.

Use:

**Restore Day Night Cycle State**

when loading.

The captured state includes the runtime clock, event schedule, ranges, Time Scale, and active Time Gates.

Your gameplay systems must still save their own state.

Example:

If an event at 18:00 caused a bridge to collapse, the bridge being collapsed belongs in the game's safe data.

Do not expect the Day Night Cycle plugin to remember gameplay consequences for you.

---

# 11. Debugging

During PIE, the Level Editor toolbar shows the current game time.

Hover it to see whether automatic time is:

* Running
* Paused
* Stopped by zero Time Scale
* Blocked by a Time Gate
* Frozen by the current world

For more information, use the Unreal **Gameplay Debugger** ['] and enable:

**DiurnalCycle**

Useful controls:

* `P` - Pause / Resume
* `,` - Rewind 1 hour
* `.` - Advance 1 hour
* `-` - Half speed
* `=` - Double speed
* `G` - Release all Time Gates
* `O` - Temporarily override the current world policy

These controls are for testing and debugging.

---

# Quick Rules

## Do

* Use **Events** for exact moments.
* Use **Ranges** for periods of time.
* Use **Time Gates** only when gameplay must stop the timeline.
* Use **Notifications** for long-lived gameplay systems.
* Use **Async nodes** for one-shot waits.
* Configure permanent map behavior with the **world icon**.
* Save `Day Night Cycle State` inside your game's existing SaveGame.
* Use Gameplay Tags to identify events and ranges.

## Do Not

* Do not create your own clock in a level Blueprint.
* Do not manually advance the clock every Tick.
* Do not poll the current time every frame when a notification can be used.
* Do not use Time Gates as a replacement for Pause.
* Do not expect `Set Date Time` to replay skipped events.
* Do not expect skipped ranges to be replayed during a teleport.
* Do not use runtime world overrides as permanent map settings.
* Do not expect the plugin to save gameplay consequences caused by events.
* Do not rely on async waits surviving a save/load.
* Do not place `BP_DiurnalCycleReference` in gameplay maps. It is documentation only.

# Which Tool Do I Need?

**Something happens at exactly 08:00?**
Use an **Event**.

**Something should be active between 20:00 and 06:00?**
Use a **Time Range**.

**The clock must stop at midnight until a cutscene finishes?**
Use a **Block Time Event / Time Gate**.

**I need to know whenever nighttime starts?**
Bind to **On Time Range Entered**.

**This quest only needs to wait for tomorrow at 10:00 once?**
Use an **Async Wait**.

**This whole level should not advance time?**
Set the map to **Freeze** using the world icon.

**I need to skip directly to morning?**
Use **Set Day Night Cycle Date Time**.

**I need to simulate the next three hours and trigger anything crossed?**
Use **Advance Day Night Cycle Hours**.
