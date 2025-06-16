# 🌤️ ESP32-C3 Weather Station – AWS IoT Core

This project reads **temperature** and **humidity** from an SHTC3 sensor over I²C using an ESP32-C3 microcontroller, and publishes the data securely to **AWS IoT Core** using **MQTT over TLS**.

---

## 📌 Features

- ⏱ Periodically reads temperature and humidity
- 🔒 Connects securely to AWS IoT using client certificates (TLS)
- 📡 Publishes MQTT messages to AWS IoT Core
- ⚡ Powered by [ESP-IDF](https://github.com/espressif/esp-idf)

---

## 🧰 Hardware Requirements

- ESP32-C3 dev board (tested on ESP32-C3-DevKitM-1)
- SHTC3 Temperature & Humidity sensor
- I²C wiring (SDA: GPIO10, SCL: GPIO8)

---

## 🛠 Installation & Setup

### 1. 📥 Install ESP-IDF

Follow Espressif's official instructions or use this:

```bash
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh
. ./export.sh
