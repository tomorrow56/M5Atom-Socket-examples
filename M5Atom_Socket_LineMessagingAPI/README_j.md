# M5Atom Socket with LINE Messaging API
M5Atom Socketを使用したスマートコンセントのプロジェクトです。電源のON/OFF制御、消費電力モニタリング、LINE通知、およびAmbientへのデータ送信機能を備えています。

## 主な機能

- **電源制御:**
  - Web UIからの遠隔ON/OFF
  - 本体ボタンでのON/OFF（短押し）
- **電力モニタリング:**
  - Web UIで電圧、電流、電力をリアルタイム表示
- **通知機能:**
  - 起動時にLINEへIPアドレスを通知
  - 消費電力がしきい値を下回った際にLINEへ完了通知（3Dプリンタ等での利用を想定）
- **WiFi設定機能:**
  - 初回起動時に設定用のアクセスポイントを自動起動
  - 本体ボタンの長押し（5秒以上）で、いつでもWiFi設定モードに移行可能
- **データ送信:**
  - Ambientへのデータ送信（オプション）
- **ファームウェア更新:**
  - OTA（Over-The-Air）によるWeb UIからのアップデート

## ハードウェア要件

- M5Atom
- M5Atom Socket
- Wi-Fi接続環境

## ソフトウェア要件

- Arduino IDE
- 以下のライブラリ:
  - M5Atom
  - ElegantOTA
  - ESP32LineMessenger
  - WiFiManager
  - WiFiClientSecure
  - Ambient (オプション)

## インストール手順

1. リポジトリをクローンするか、ソースコードをダウンロードします
2. Arduino IDEで `M5Atom_Socket_LineMessagingAPI.ino` を開きます
3. 必要なライブラリをインストールします
4. 設定をカスタマイズします（後述）
5. M5Atomにスケッチをアップロードします

## 設定

### 必須設定

`M5Atom_Socket_LineMessagingAPI.ino` を開き、以下の設定を行ってください：

```cpp
// デバイス名
#define DEVICE_NAME "Fraxinus"  // 任意のデバイス名に変更可能

// LINE Messaging API アクセストークン
const char* accessToken = "<YOUR_LINE_MESSAGING_API_CHANNEL_ACCESS_TOKEN>";  // 実際のトークンに置き換え
```

### オプション設定

- **Ambientへのデータ送信:**
  - `M5Atom_Socket_LineMessagingAPI.ino` の先頭にある `//#define useAmb` のコメントを外して機能を有効にします。
  - `channelId` と `writeKey` をご自身のものに設定してください。
    ```cpp
    #ifdef useAmb
      #include <Ambient.h>
      Ambient ambient;
      unsigned int channelId = 40780; // ご自身のAmbientチャンネルID
      const char* writeKey = "<YOUR_Ambient_WRITE_KEY>"; // ご自身のAmbientライトキー
    #endif
    ```
- **デバッグモード:**
  - `debug` の値を `true` または `false` に変更して、シリアルコンソールへのデバッグ出力の有効/無効を切り替えます。

## 使用方法

### 1. WiFi設定 (初回または変更時)
- 電源を入れると、M5Atomは保存されたWiFi情報での接続を試みます。
- 接続できない場合、または**本体ボタンを5秒以上長押し**した場合は、WiFi設定モードになります。
- M5AtomのLEDが**青色**に点灯し、`ATOM-SOCKET-XXXX` という名前のアクセスポイントが起動します。
- スマートフォンやPCでこのアクセスポイントに接続し、Webブラウザで `192.168.4.1` にアクセスすると、接続したいWiFiのSSIDとパスワードを設定できます。
- 設定完了後、デバイスは自動的に再起動します。

### 2. 通常利用
- WiFi接続後、LINEに起動通知とIPアドレスが送信されます。
- **本体ボタンを短く押す**と、ソケット電源のON/OFFが切り替わります。
  - ON: 赤色LED
  - OFF: 緑色LED
- WebブラウザでデバイスのIPアドレスにアクセスすると、Web UIが表示されます。

### 3. Web UIでの操作
- 電源のON/OFF
- 消費電力（電圧、電流、電力）のリアルタイム確認
- OTA（無線）でのファームウェアアップデート

## OTAアップデート

1. Webブラウザで `http://[デバイスのIPアドレス]/update` にアクセス
2. 認証情報を入力（デフォルト: ユーザー名 `admin`、パスワード `admin`）
3. ArduinoIDEで生成した新しいファームウェア（.binファイル）をアップロード

### OTAアップデート時の注意

- パーティションスキームは「Default」を選択してください

## トラブルシューティング

### OTAアップデートに失敗する場合
1. パーティションスキームが「Default」に設定されていることを確認
2. 安定したWi-Fi接続を確保

### LINE通知が届かない場合
1. アクセストークンが正しく設定されているか確認
2. インターネット接続を確認
3. LINE Messaging APIの制限に達していないか確認

## ライセンス

このプロジェクトはオープンソースです。詳細はLICENSEファイルを参照してください。

## 参照

- [M5Stack公式サイト](https://m5stack.com/)
- [LINE Messaging API](https://developers.line.biz/ja/services/messaging-api/)
- [Ambient](https://ambidata.io/)

