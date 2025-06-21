# M5Atom Socket with LINE Messaging API
M5Atom Socketを使用したスマートコンセントのプロジェクトです。電源のON/OFF制御、消費電力モニタリング、LINE通知、およびAmbientへのデータ送信機能を備えています。

## 主な機能

- Webインターフェースからの遠隔操作
- リアルタイム電力消費モニタリング（電圧、電流、電力）
- 消費電力が設定値を下回るとLINE Messaging APIにより完了を通知
- Ambientへのデータ送信（オプション）
- OTA（Over-The-Air）アップデート対応

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
2. Arduino IDEで `M5Atom_Socket_LineAPI_Ambient.ino` を開きます
3. 必要なライブラリをインストールします
4. 設定をカスタマイズします（後述）
5. M5Atomにスケッチをアップロードします

## 設定

### 必須設定

`M5Atom_Socket_LineAPI_Ambient.ino` を開き、以下の設定を行ってください：

```cpp
// デバイス名
#define DEVICE_NAME "M5Atom Socket"  // 任意のデバイス名に変更可能

// LINE Notify アクセストークン
const char* accessToken = "<YOUR_LINE_NOTIFY_TOKEN>";  // 実際のトークンに置き換え
```

### オプション設定

- Ambientを使用する場合は、`useAmb` のコメントを外し、チャンネルIDとライトキーを設定します
- デバッグモードの有効/無効を切り替えるには `debug` を変更します

## 使用方法

1. 電源を入れると、M5AtomがWi-Fiに接続します
2. WiFiに接続できない場合はWiFiManagerが立ち上がりますので、"ATOM-SOCKET-"で始まるAPに接続し、ブラウザで"192.168.4.1"にアクセスして接続するAPのSSIDとパスワードを入力します
3. シリアルモニタに表示されるIPアドレスにWebブラウザでアクセスします
4. Webインターフェースから以下の操作が可能です：
   - 電源のON/OFF
   - 消費電力の確認
   - デバイス情報の表示

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
