# ⏱️ Multi-Client IoT Conference Timer

A robust, enterprise-ready conference timer developed for managing speaker sessions with precision. Powered by an **WEMOS D1 Mini** MCU and a physical **MAX7219 16x8 LED Matrix**, the system syncs instantly with an unlimited number of modern web browser control interfaces using full-duplex **WebSockets**.

It features automated Wi-Fi provisioning via a Captive Portal, local hostname mapping via mDNS, automatic hardware power-saving routines, and adaptive layout presentation tailored for small modular screen arrays.

---

## ✨ Key Features

* **Real-Time Synchronization**: Multi-client ecosystem driving dynamic state metrics upstream/downstream over low-latency WebSockets (Port 81).
* **Zero Hardcoded Credentials**: Integrated `WiFiManager` automatically boots into a secure configuration Captive Portal if local Wi-Fi configuration drops.
* **Friendly Local URL (mDNS)**: Access the control dashboard via `http://timer.local` directly instead of remembering dynamic IP strings.
* **Smart UI Safety Lockouts**: The web dashboard automatically locks quick-set selections and manual input boxes while the countdown is actively operational to prevent accidental mid-session data corruption.
* **Eco Energy Management**: Shifts matrix hardware into a deep power-down shutdown register array mode if left completely idle for over 5 minutes.
* **Overrun Visual Alarming**: Flashes the entire structural framework (both the web dashboard and physical matrix modules) high-visibility red/on-off loops when speaker runtime crosses into negative overtime overrun territory.
* **Adaptive Matrix Presentation**: Optimizes 16x8 pixel dual-segment restrictions by shifting dynamically from Minutes tracking (MM) to high-precision raw Seconds (SS) representation in the final operational minute.

---

## 🔌 Hardware Connections (Wiring)

Connect the **WEMOS D1 Mini** to the **MAX7219 (2-Module Cascaded)** segment block array using the following high-speed hardware SPI / Bit-Banging layout map:

| MAX7219 Pin | WEMOS D1 Mini Pin | Description |
| :--- | :--- | :--- |
| **VCC** | **5V** | Main 5V Direct Hardware System Power Supply |
| **GND** | **GND** | Shared System Common Ground Path |
| **DIN** | **D7** | Master Out Slave In Data Communication Line (GPIO13) |
| **CS** | **D8** | Dedicated Chip Select Line Register Shifting Latch (GPIO15) |
| **CLK** | **D5** | System Common Bus Synchronous Clock Line (GPIO14) |

---

## 📚 Required Software Libraries

Ensure the following community core dependencies are loaded into your active **Arduino IDE Environment** workspace before compilation phases:

1. `WiFiManager` (by tzapu) – Enforces automatic network registration portals.
2. `WebSockets` (by Markus Sattler) – High-efficiency background data transport pipes.

*Note: The MAX7219 hardware operations layout handling routine uses custom light drivers embedded natively inside the project (`max7219.h` / `font.h`) ensuring no heavy external display frameworks are needed.*

---

## 🚀 Step-by-Step On-Site Deployments

1. **Power Up**: Connect the MCU to a standard stable USB or 5V rail infrastructure point.
2. **Wi-Fi Initial Setup**:
   * Search for a hot wireless access network point named `Timer_Configuration` using any mobile phone or computer.
   * Access it using the default passkey `timer123`.
   * Pick your venue's venue wireless network profile from the generated graphic pop-up manager dashboard, insert the venue's active security password keys, and tap **Save**.
3. **Pristup**: The display will automatically scroll the assigned dynamic local IP address. Open any web browser on the same network and surf directly into: **`http://timer.local`**.
4. **Control**: Utilize the quick-set buttons (30 min, 45 min, etc.) or dial-in parameters manually via the dynamic responsive dashboard interface.

---

## 🛠️ Project File Architecture

* `Timer.ino`: Core firmware state processing pipeline engines, mDNS network stack parameters management routing, and time evaluation loop logic blocks.
* `html.h`: Dark UI concept control dashboard panel structures written directly in highly optimized optimized standard raw compressed HTML5, CSS3 transitions animations, and asynchronous execution JavaScript files loaded dynamically inside `PROGMEM` flash memory spaces.
* `max7219.h`: Custom lightweight localized low-overhead bit register driver maps addressing the display cascaded cascade interfaces directly without bloatware libraries.
* `font.h`: Raw byte tables for the 6x8 and 5x7 fonts.

---

## 🤝 Acknowledgments
Crafted cleanly using an collaborative framework setup pairing human domain engineering expertise alongside the assistive programming processing generation metrics of **✨ Gemini**.
