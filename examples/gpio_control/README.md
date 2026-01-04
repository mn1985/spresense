# GPIO Control Example for Spresense

このサンプルプログラムは、Spresense SDKを使用して任意のGPIOピンをON/OFFするためのツールです。

## 機能

- GPIOピンをHIGH(ON)に設定
- GPIOピンをLOW(OFF)に設定
- GPIOピンの値を読み取り
- GPIOピンの状態をトグル
- 利用可能なピンのリスト表示

## ビルド方法

1. SDKディレクトリに移動:
```bash
cd spresense/sdk
```

2. 設定ツールでこのサンプルを有効化:
```bash
tools/config.py examples/gpio_control
```

または、既存の設定に追加する場合:
```bash
tools/config.py -m
# メニューから Examples -> GPIO Control command を選択して有効化
```

3. ビルド:
```bash
make
```

4. Spresenseボードにフラッシュ:
```bash
tools/flash.sh -c /dev/ttyUSB0 nuttx.spk
```

## 使用方法

### 基本コマンド

```bash
# GPIOピンをONにする
gpio_control D18 on

# GPIOピンをOFFにする
gpio_control D18 off

# GPIOピンの値を読み取る
gpio_control D18 read

# GPIOピンをトグルする
gpio_control D18 toggle

# 利用可能なピンのリスト表示
gpio_control list

# ヘルプ表示
gpio_control help
```

### 使用例

```bash
# D18ピンをHIGHに設定
nsh> gpio_control D18 on
GPIO D18 set to HIGH

# D19ピンをLOWに設定
nsh> gpio_control D19 off
GPIO D19 set to LOW

# D20ピンの値を読み取る
nsh> gpio_control D20 read
GPIO D20 value: 1 (HIGH)

# D21ピンをトグル
nsh> gpio_control D21 toggle
GPIO D21 toggled: 0 -> 1
```

## 利用可能なGPIOピン

D0からD28までのピンが利用可能です。ただし、一部のピンは他のペリフェラル（UART、SPI、I2Cなど）で使用されている場合があります。

### よく使用されるGPIOピン

- **D18** (I2S1_DATA_OUT)
- **D19** (I2S1_DATA_IN)
- **D25** (I2S1_LRCK)
- **D26** (I2S1_BCK)

これらのピンは、I2Sを使用していない場合にGPIOとして使用できます。

## ピン配置

Spresense拡張ボードのピン配置については、以下のドキュメントを参照してください:
https://developer.sony.com/develop/spresense/docs/hw_docs_en.html

## 注意事項

- GPIOピンを使用する前に、そのピンが他の機能（UART、SPI、I2Cなど）で使用されていないことを確認してください。
- 出力電圧は1.8Vです。5V系のデバイスを接続する場合はレベルシフターが必要です。
- 最大出力電流に注意してください。

## ライセンス

Copyright 2025 Sony Semiconductor Solutions Corporation

このソフトウェアは、BSDライセンスの下で配布されています。
