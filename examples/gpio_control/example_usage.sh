# GPIO制御サンプルスクリプト
# Spresense上で実行する例

# 例1: LEDの点滅（D18ピンを使用）
echo "Example 1: LED Blink on D18"
gpio_control D18 on
sleep 1
gpio_control D18 off
sleep 1
gpio_control D18 on
sleep 1
gpio_control D18 off

# 例2: 複数のGPIOピンを順番にON
echo "Example 2: Sequential GPIO activation"
gpio_control D18 on
gpio_control D19 on
gpio_control D25 on
gpio_control D26 on

# 例3: すべてのピンをOFF
echo "Example 3: Turn off all pins"
gpio_control D18 off
gpio_control D19 off
gpio_control D25 off
gpio_control D26 off

# 例4: ピンの状態を読み取る
echo "Example 4: Read pin states"
gpio_control D18 read
gpio_control D19 read
gpio_control D25 read
gpio_control D26 read

# 例5: トグル機能の使用
echo "Example 5: Toggle pins"
gpio_control D18 toggle
gpio_control D18 toggle
gpio_control D18 toggle
