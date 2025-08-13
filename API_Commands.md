# Mugi API Commands Documentation

This document describes the API commands sent via `sendSocket()` function in the Mugi Rocket League plugin.

## Command Structure

All commands are sent as JSON objects with the following basic structure:
```json
{
  "cmd": "command_name",
  "data": { /* command-specific data */ }
}
```

## Available Commands

### 1. `matchId`
**Purpose:** Send match ID when a game starts
**When sent:** Game start
```json
{
  "cmd": "matchId",
  "data": {
    "matchId": "string"
  }
}
```

### 2. `start`
**Purpose:** Indicate game start
**When sent:** Game start (only when total score is 0)
```json
{
  "cmd": "start",
  "data": 0
}
```

### 3. `scored`
**Purpose:** Indicate when a goal is scored
**When sent:** When `scored` event is triggered
```json
{
  "cmd": "scored"
}
```

### 4. `end`
**Purpose:** Indicate game end
**When sent:** Game end
```json
{
  "cmd": "end"
}
```

### 5. `stats`
**Purpose:** Send final match statistics for all players
**When sent:** After game end
```json
{
  "cmd": "stats",
  "data": [
    {
      "id": "string",
      "teams": 0,
      "scores": 100,
      "goals": 5,
      "assists": 3,
      "saves": 2,
      "shots": 8,
      "demos": 1,
      "ballTouches": 15
    }
  ]
}
```
**Notes:** 
- Array is sorted by teams (0=orange, 1=blue) then by scores (highest to lowest)
- `id` is either player unique ID or bot index in debug mode

### 6. `time`
**Purpose:** Update game time information
**When sent:** Time updates (currently commented out)
```json
{
  "cmd": "time",
  "data": {
    "time": 300,
    "isOvertime": false
  }
}
```

### 7. `boost`
**Purpose:** Send player boost level updates
**When sent:** When a player's boost amount changes
```json
{
  "cmd": "boost",
  "data": {
    "boost": 75,
    "index": 0
  }
}
```
**Notes:** 
- `boost` is percentage (0-100)
- `index` is player index in the match

### 8. `subScore`
**Purpose:** Send detailed score breakdown
**When sent:** When player's sub-scores change
```json
{
  "cmd": "subScore",
  "data": {
    "goals": 2,
    "shots": 5,
    "assists": 1,
    "saves": 3
  }
}
```

### 9. `score`
**Purpose:** Send focused player's total score
**When sent:** When focused player's score changes
```json
{
  "cmd": "score",
  "data": {
    "score": 250
  }
}
```

### 10. `player`
**Purpose:** Send information about currently focused player
**When sent:** When camera focus changes to a different player
```json
{
  "cmd": "player",
  "data": {
    "playerIndex": "0",
    "team": 0,
    "playerName": "PlayerName"
  }
}
```
**Notes:** 
- `team`: 0=orange, 1=blue
- `playerName` shows actual display name (not in debug mode) or actor name (in debug mode)

### 11. `epicSave`
**Purpose:** Indicate when an epic save occurs
**When sent:** When EpicSave stat event is triggered
```json
{
  "cmd": "epicSave"
}
```

### 12. `goals`
**Purpose:** Send detailed goal information
**When sent:** When a goal is scored
```json
{
  "cmd": "goals",
  "data": {
    "team": "blue",
    "scoreId": "player_id",
    "assistId": "assister_id"
  }
}
```
**Notes:** 
- `team`: "blue" or "orange"
- `assistId` may be empty string if no assist

## Socket Implementation

The `sendSocket()` function sends data to two sockets:
- Primary socket (`sock`)
- Secondary socket (`sock2`)

Both sockets receive the same JSON string data using TCP send operations.

## Debug Mode

When `isDebug` is true:
- Player IDs use bot indices instead of unique player IDs
- Player names show actor names directly

## Firebase Integration

Some commands also trigger Firebase Realtime Database updates:
- `scored` → updates status to "scored"
- `end` → updates status to "end"
- `time` → updates blue_setPoint with time value