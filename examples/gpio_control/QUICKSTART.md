# GPIO制御 クイックスタートガイド

## 1. ビルドとインストール

### ステップ1: SDKディレクトリに移動
```bash
cd /home/mn/spresense/sdk
```

### ステップ2: 設定
```bash
tools/config.py examples/gpio_control
```

### ステップ3: ビルド
```bash
make
```

### ステップ4: フラッシュ
```bash
tools/flash.sh -c /dev/ttyUSB0 nuttx.spk
```

## 2. 基本的な使い方

### Spresenseに接続
```bash
screen /dev/ttyUSB0 115200
```

### GPIOピンをON
```bash
nsh> gpio_control D18 on
```

### GPIOピンをOFF
```bash
nsh> gpio_control D18 off
```

### GPIOピンの状態を確認
```bash
nsh> gpio_control D18 read
```

## 3. 実際の配線例

### LED接続例（D18ピン使用）

```
Spresense D18 ----[330Ω]----[LED]----GND
```

**注意**: 
- Spresenseの出力は1.8Vです
- 電流制限抵抗（330Ω程度）を必ず使用してください
- LEDの極性に注意してください（長い方がアノード+）

### 5V系デバイス接続例（レベルシフター使用）

```
Spresense D18 ----[Level Shifter]---- 5V Device
                   (TXS0108E等)
```

## 4. よくある使用例

### 例1: モーター制御
```bash
# モーターON
gpio_control D18 on

# モーターOFF
gpio_control D18 off
```

### 例2: リレー制御
```bash
# リレーON
gpio_control D19 on

# リレーOFF
gpio_control D19 off
```

### 例3: ステッピングモーター制御
```bash
# 4つのGPIOピンを使用
gpio_control D18 on
gpio_control D19 off
gpio_control D25 off
gpio_control D26 off
```

## 5. トラブルシューティング

### ピンが動作しない場合

1. **他の機能で使用されていないか確認**
   - UARTやSPIなどの設定を確認
   - `gpio_control list`で利用可能なピンを確認

2. **配線を確認**
   - GNDが正しく接続されているか
   - 電圧レベルが適切か（1.8V出力）

3. **ピンの状態を確認**
   ```bash
   gpio_control D18 read
   ```

### よく使用できるGPIOピン

以下のピンは、I2Sを使用していない場合にGPIOとして使用可能です：
- **D18** (I2S1_DATA_OUT)
- **D19** (I2S1_DATA_IN)
- **D25** (I2S1_LRCK)
- **D26** (I2S1_BCK)

## 6. 参考リンク

- [Spresense公式ドキュメント](https://developer.sony.com/develop/spresense/)
- [ハードウェア仕様](https://developer.sony.com/develop/spresense/docs/hw_docs_en.html)
- [SDK開発ガイド](https://developer.sony.com/develop/spresense/docs/sdk_developer_guide_en.html)
