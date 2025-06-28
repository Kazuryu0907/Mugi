# Mugi UDP API ドキュメント

## 概要

MugiはRocket League用のBakkesModプラグインで、UDPソケット通信を通じてリアルタイムでゲームイベントを送信します。プラグインはUDPポート12345でJSON形式のメッセージを使用して通信します。

## 接続詳細

- **プロトコル**: UDP
- **アドレス**: 127.0.0.1
- **ポート**: 12345
- **フォーマット**: JSON

## メッセージ構造

すべてのメッセージは以下の基本構造に従います：

```json
{
  "cmd": "コマンド名",
  "data": { ... }
}
```

## 送信メッセージ（プラグイン → 外部アプリケーション）

### プラグイン初期化

#### `init`
プラグイン読み込み時に送信されます。

```json
{
  "cmd": "init",
  "data": {
    "version": "プラグインバージョン",
    "receiverEnabled": true
  }
}
```

### 試合管理

#### `start`
試合開始時に送信されます。

```json
{
  "cmd": "start",
  "data": 0
}
```

#### `end`
試合終了時に送信されます。

```json
{
  "cmd": "end"
}
```

#### `matchId`
試合GUIDと共に送信されます。

```json
{
  "cmd": "matchId",
  "data": {
    "matchId": "試合GUID文字列"
  }
}
```

### チーム情報

#### `teamNames`
チーム名が利用可能な時に送信されます。

```json
{
  "cmd": "teamNames",
  "data": {
    "blue": "青チーム名",
    "orange": "オレンジチーム名",
    "matchId": "試合GUID文字列"
  }
}
```

#### `displayNames`
プレイヤー表示名のリストと共に送信されます。

```json
{
  "cmd": "displayNames",
  "data": ["プレイヤー1", "プレイヤー2", "プレイヤー3", ...]
}
```

#### `playerTable`
プレイヤーIDマッピングと共に送信されます。

```json
{
  "cmd": "playerTable",
  "data": ["プレイヤーID_1", "プレイヤーID_2", "プレイヤーID_3", ...]
}
```

### ゲームイベント

#### `scored`
ゴールが決まった時に送信されます。

```json
{
  "cmd": "scored"
}
```

#### `goals`
詳細なゴール情報と共に送信されます。

```json
{
  "cmd": "goals",
  "data": {
    "team": "blue|orange",
    "scoreId": "得点者プレイヤーID",
    "assistId": "アシスト者プレイヤーID"
  }
}
```

#### `demolished`
プレイヤーがデモリッションされた時に送信されます。

```json
{
  "cmd": "demolished",
  "data": {
    "receiverIndex": 0,
    "victimIndex": 1
  }
}
```

#### `epicSave`
エピックセーブが発生した時に送信されます。

```json
{
  "cmd": "epicSave"
}
```

### リアルタイムゲームデータ

#### `time`
試合中、ゲーム時間と共に継続的に送信されます。

```json
{
  "cmd": "time",
  "data": {
    "time": 120,
    "isOvertime": false
  }
}
```

#### `boost`
プレイヤーのブースト量が変化した時に送信されます。

```json
{
  "cmd": "boost",
  "data": {
    "boost": 75,
    "index": 0
  }
}
```

#### `player`
カメラフォーカスが別のプレイヤーに変更された時に送信されます。

```json
{
  "cmd": "player",
  "data": {
    "playerIndex": "0",
    "team": "blue|orange",
    "playerName": "プレイヤー表示名"
  }
}
```

#### `score`
フォーカス中のプレイヤーのスコアが変化した時に送信されます。

```json
{
  "cmd": "score",
  "data": {
    "score": 150
  }
}
```

#### `subScore`
フォーカス中のプレイヤーの詳細統計が変化した時に送信されます。

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

### 試合統計

#### `stats`
試合終了時に完全なプレイヤー統計と共に送信されます。

```json
{
  "cmd": "stats",
  "data": [
    {
      "id": "プレイヤーID",
      "teams": 0,
      "scores": 250,
      "goals": 2,
      "assists": 1,
      "saves": 3,
      "shots": 5,
      "demos": 1,
      "ballTouches": 15
    },
    ...
  ]
}
```

### リプレイイベント

#### `endReplay`
リプレイ再生終了時に送信されます。

```json
{
  "cmd": "endReplay"
}
```

#### `endStats`
ハイライト再生終了時に送信されます。

```json
{
  "cmd": "endStats"
}
```

### デバッグメッセージ

#### `dbg`
プレイヤーIDを含むデバッグメッセージ。

```json
{
  "cmd": "dbg",
  "data": "プレイヤーID"
}
```

## 受信メッセージ（外部アプリケーション → プラグイン）

### ヘルスチェックコマンド

#### `ping`
プラグインからのping応答を要求します。

**リクエスト:**
```json
{
  "cmd": "ping"
}
```

**レスポンス:**
```json
{
  "cmd": "pong",
  "data": {
    "status": "ok",
    "timestamp": 1703123456789
  }
}
```

#### `getVersion`
プラグインのバージョン情報を要求します。

**リクエスト:**
```json
{
  "cmd": "getVersion"
}
```

**レスポンス:**
```json
{
  "cmd": "version",
  "data": {
    "version": "プラグインバージョン",
    "status": "ok",
    "buildTime": "Dec 21 2023 10:30:45"
  }
}
```

### エラーレスポンス

#### `error`
無効なコマンドを受信した場合やJSON解析に失敗した場合に送信されます。

```json
{
  "cmd": "error",
  "data": {
    "message": "エラーの説明",
    "status": "error"
  }
}
```

## データタイプと定数

### チーム番号
- `0`: 青チーム
- `1`: オレンジチーム  
- `255`: 観戦者

### プレイヤーインデックス
- プレイヤーは0から始まるインデックス
- チーム別にソート（青チームが先、次にオレンジチーム）
- 各チーム内でスコア順

### 時間フォーマット
- 通常時間: 残り秒数（カウントダウン）
- オーバータイム: オーバータイム開始からの経過秒数（カウントアップ）

### ブースト値
- 範囲: 0-100（整数パーセンテージ）

## 使用例

### 基本的なクライアント接続

```python
import socket
import json

# UDPソケット作成
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(('127.0.0.1', 12345))

while True:
    data, addr = sock.recvfrom(1024)
    message = json.loads(data.decode())
    
    cmd = message.get('cmd')
    if cmd == 'scored':
        print("ゴール!")
    elif cmd == 'time':
        time_data = message['data']
        print(f"時間: {time_data['time']}秒, オーバータイム: {time_data['isOvertime']}")
```

### プラグインへのコマンド送信

```python
import socket
import json

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# pingコマンド送信
ping_cmd = {"cmd": "ping"}
sock.sendto(json.dumps(ping_cmd).encode(), ('127.0.0.1', 12345))

# pongレスポンス受信
data, addr = sock.recvfrom(1024)
response = json.loads(data.decode())
print(response)  # {"cmd": "pong", "data": {"status": "ok", "timestamp": ...}}
```

## 注意事項

- すべての通信は非同期です
- プラグインはイベント発生時や値変更時のみデータを送信します
- クライアントアプリケーションは潜在的なメッセージ損失（UDPの性質）を処理する必要があります
- プレイヤーIDは試合セッション中一貫しています
- Botプレイヤーはデバッグモードで「Player_Bot_」プレフィックスによる特別な処理があります