# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## プロジェクトの概要

このプロジェクトは、Rocket League用のBakkesModプラグイン「Mugi」です。ゲーム内のイベント（ゴール、アシスト、デモリッション、エピックセーブなど）をWebSocketを通じてリアルタイムで外部アプリケーションに送信します。

## ビルドコマンド

### Visual Studioでのビルド
```bash
# Visual Studio 2022を使用してビルド
MSBuild.exe Mugi.sln /p:Configuration=Release /p:Platform=x64
```

または、Visual Studio IDEを使用：
1. `Mugi.sln`をVisual Studio 2022で開く
2. Configuration: Release, Platform: x64を選択
3. ビルド -> ソリューションのビルド

### プリビルドイベント
ビルド前に`update_version.ps1`スクリプトが自動実行され、バージョン情報が更新されます。

## アーキテクチャ

### コアクラス
- **Mugi**: メインプラグインクラス（BakkesModPlugin継承）
  - ゲームイベントのフック管理
  - WebSocket通信（UDP）
  - プレイヤー/チーム情報の管理

### 主要コンポーネント

#### イベントフック
- `onLoad()`: プラグイン初期化時に各種ゲームイベントをフック
- `onUnload()`: プラグイン終了時にフックを解除

#### WebSocket通信
- UDPソケット（127.0.0.1:12345）でJSONメッセージを送信
- 主要コマンド: `init`, `start`, `end`, `scored`, `goals`, `demolished`, `epicSave`, `time`, `boost`, `stats`

#### プレイヤー管理
- `createNameTable()`: 試合開始時にプレイヤー情報テーブルを作成
- BotとHumanプレイヤーの区別
- 観戦時の表示名とゲーム内IDのマッピング

#### データ構造
- `playerData`: プレイヤー名、ID、チーム情報
- `OwnerIndexMap`: プレイヤー名/IDからインデックスへのマッピング
- `PlayerMap`: アクター名からPriWrapperへのマッピング

### 依存関係
- **BakkesMod SDK**: Rocket Leagueプラグイン開発フレームワーク
- **ImGui**: GUI（現在は未使用だが設定画面用に準備）
- **nlohmann/json**: JSON操作ライブラリ
- **Winsock2**: ネットワーク通信

### ファイル構成
- `Mugi.cpp/.h`: メインプラグインロジック
- `GuiBase.cpp/.h`: GUI基底クラス（未実装）
- `IMGUI/`: ImGuiライブラリ
- `version.h`: バージョン情報（自動生成）
- `update_version.ps1`: バージョン更新スクリプト

## 開発時の注意事項

### デバッグ設定
- `isDebug`フラグでBot名の処理方法を切り替え
- デバッグ時はBot名を「Player_Bot_」プレフィックス付きで処理

### ネットワーク通信
- UDPソケットを使用（ポート12345）
- JSONフォーマットでゲーム情報を送信
- **強化されたエラーハンドリング**: 接続・送信エラーの詳細ログ出力
- **ソケット状態管理**: `isSocketInitialized`フラグで安全性確保

### メモリ管理
- BakkesModのWrapper系クラスはスマートポインタで管理
- `IsNull()`チェックを必須で実行
- **初期化済み配列**: `Boosts[MAX_BOOST_SLOTS] = {0}`で安全な初期化
- **未使用変数削除**: デッドコードを排除しメモリ効率化

### イベント処理
- 試合開始/終了の検知
- リアルタイムでのブーストやスコア情報更新  
- デモリッション、エピックセーブなどの特殊イベント処理
- **パフォーマンス最適化**: 変更時のみJSONを生成・送信

## 定数定義

### ネットワーク定数
- `DEFAULT_PORT = 12345`: デフォルトUDPポート
- `DEFAULT_ADDRESS = "127.0.0.1"`: デフォルトサーバーアドレス

### ゲーム定数
- `OVERTIME_START_OFFSET = 4`: オーバータイム開始時のオフセット秒
- `MAX_BOOST_SLOTS = 10`: ブースト情報の最大スロット数
- `MAX_BOT_COUNT = 6`: Bot数の最大値
- `SPECTATOR_TEAM_NUM = 255`: 観戦者のチーム番号

## 最近の改善履歴

### v2024.12 - 安定性・パフォーマンス大幅改善
- **致命的バグ修正**: ソケット管理とエラーハンドリングの修正
- **メモリ安全性向上**: 全メンバ変数の適切な初期化
- **デッドコード削除**: 未使用変数・関数の完全除去
- **パフォーマンス最適化**: tick関数とJSON生成の効率化
- **定数化**: マジックナンバーの排除とメンテナンス性向上