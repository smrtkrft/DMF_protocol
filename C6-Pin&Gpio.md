# XIAO ESP32C6 Pin Mapping

## Doğrulanmış Pin → GPIO Eşleştirmesi

| Fiziksel Pin | GPIO Numarası | Test Durumu |
|--------------|---------------|-------------|
| D0           | GPIO 0        | ✅ Doğrulandı |
| D1           | GPIO 1        | ✅ Doğrulandı |
| D2           | GPIO 2        | ✅ Doğrulandı |
| D3           | GPIO 21       | ✅ Doğrulandı |
| D4           | GPIO 22       | ✅ Doğrulandı |
| D5           | GPIO 23       | ✅ Doğrulandı |
| D6           | GPIO 16       | ✅ Doğrulandı |
| D7           | GPIO 17       | ✅ Doğrulandı |
| D8           | GPIO 19       | ✅ Doğrulandı |
| D9           | GPIO 20       | ✅ Doğrulandı |
| D10          | GPIO 18       | ✅ Doğrulandı |

## Test Yöntemi

GND ile fiziksel pin arası jumper kablo ile temas yapılarak `digitalRead()` ile GPIO durumu okundu.

## Kod Kullanımı

```cpp
// SmartKraft_DMF.ino için pin tanımları
constexpr uint8_t BUTTON_PIN = 21;  // D3 -> GPIO21
constexpr uint8_t RELAY_PIN = 18;   // D10 -> GPIO18
```

## Notlar

- Tüm pinler GND temas testi ile doğrulandı
- INPUT_PULLUP modunda test edildi
- Test tarihi: 12 Ekim 2025
