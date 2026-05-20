# SwitchBot SmartLock Opener (M5 ATOM Lite)

M5 ATOM Lite のボタン操作で SwitchBot Smart Lock を開閉するプロジェクトです。  
本プロジェクトは **SwitchBot SmartLock Pro** でのみ動作確認しています。

Code authored with assistance from Claude (Anthropic).

## 対応機種

- **M5 ATOM Lite のみ対応**
- SwitchBot SmartLock Pro でのみ動作確認済み

## 必要なもの

- M5 ATOM Lite
- SwitchBot SmartLock Pro
- SwitchBot Hub（例: Hub Mini 30）
- Wi-Fi（2.4GHz）
- PlatformIO 環境

## セットアップ

1. `src/const.hpp.example` を `src/const.hpp` にコピー
2. `src/const.hpp` を開き、以下の値を設定
   - `WIFI_SSID`
   - `WIFI_PASS`
   - `SWITCHBOT_TOKEN`
   - `SWITCHBOT_SECRET`
   - `SWITCHBOT_DEVICE_ID`

> `const.hpp` は機密情報を含むため `.gitignore` されています。

## ビルド方法

```bash
pio run
```

## 転送（書き込み）方法

```bash
pio run -t upload
```

必要ならシリアルモニターを使って確認できます。

```bash
pio device monitor
```

## 使い方

- **ボタン短押し（2 秒未満）**: 解錠（LED 赤）
- **ボタン長押し（2 秒以上）**: 施錠（LED 緑）
- 連続操作防止のため **5 秒のクールダウン**があります

## LED の色

| 色 | 状態 |
|---|---|
| 消灯 | 通常時（待機中） |
| 青 | WiFi 接続中 |
| 紫 | API コマンド送信中 |
| 緑 3回点滅 | 施錠成功 |
| 赤 3回点滅 | 解錠成功 |
| 黄 | API エラー（3 秒間点灯） |

## M5StickC Plus 版との違い

- LCD 表示なし（RGB LED のみでステータス表示）
- ボタンが 1 つのため、短押し/長押しで解錠/施錠を切り替え
- バッテリー非搭載のため自動電源オフ機能なし
- M5StickC Plus 版は [switchbot-opener](https://github.com/m-matsubara/switchbot-opener) を参照

## 注意点

- 本プロジェクトは **SwitchBot SmartLock Pro でのみ動作確認済み**です
- **M5 ATOM Lite のみ対応**しています（ATOM Matrix などは未検証）
- `src/const.hpp.example` から `src/const.hpp` を作成する必要があります
