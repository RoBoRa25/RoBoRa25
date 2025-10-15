# RoBoRa — ESP32 Robot Firmware & Web UI

Firmware per **ESP32 (testato su ESP32‑C3)** che controlla un piccolo robot a due motori (ponte H tipo **DRV8833**), espone una **Web UI** in locale (HTTP + **WebSocket**) per guida e configurazione, supporta **OTA** (firmware e filesystem), **telemetria IMU** su I²C e un **display OLED** 128×64 (SH1106G).

> Progetto di Orazio Franco — licenza MIT.

---

## ✨ Funzionalità principali

- 🚗 **Controllo motori** (arcade/tank) con dead‑zone, expo, gain e inversioni configurabili.  
- 🌐 **Wi‑Fi STA/AP** con mDNS (“`robora.local`”), health‑check e fallback automatico in AP.  
- 🕹️ **Web UI** single‑page (servita dallo stesso ESP32) con joystick/tastiera, pagina **Config**, **Info**, **OTA**, **Display**, **3D**.  
- 🔌 **WebSocket** per comandi in tempo reale: `move`, `config_*`, `info_req`, `displaymsg`, `function`, `reboot`…  
- ⬆️ **OTA** via `multipart/form-data` (`/update`) o `octet-stream` (`/ota`) con progress in tempo reale e riavvio a fine update.  
- 📡 **Telemetria IMU** su I²C (pitch/roll/yaw, magnetometro, temperatura) con broadcast periodico su WS.  
- 🖥️ **Display OLED 128×64** (SH1106G): testo, pagine a scorrimento, upload di immagini convertite lato browser.  
- 🔵 **LED RGB (NeoPixel)** con helper rapidi (on/off, R/G/B, rainbow).  
- 🧩 **Config persistente (NVS)** con schema dinamico esposto alla UI (etichette, range, tipi).  
- 🛡️ **Safety**: se non ci sono client WebSocket **i motori vanno in stop**.

---

## 🧱 Architettura (alto livello)

- `main.cpp` — bootstrap; inizializzazione di config, rete, WS, OTA, motori, telemetria, display; ciclo di servizio.
- `net.*`, `connection.*` — server HTTP, endpoint, health‑check, mDNS, logica STA/AP.
- `websocket.*` — protocollo e handler comandi in JSON.
- `ota.*` — implementazione OTA (`/update`, `/ota`).
- `config.*` — NVS, schema parametri, I/O e applicazione a runtime.
- `motors.*` — driver DRV8833, mixing arcade/tank e ticker periodico.
- `telemetry.*` — IMU via I²C, ADC, pacchetti sensore su WS.
- `display.*` — SH1106G 128×64, testo, scrolling, buffer immagine.
- `ledsrgb.*` — helper NeoPixel.
- `Index.html`, `Script.js` — Web UI SPA (joystick, pannelli, OTA, display, 3D).

> Pinout e dettagli hardware sono definiti nei sorgenti (`all_define.h`); personalizzali secondo la tua scheda/driver.

---

## 🚀 Avvio rapido

1. **Compila & flash**
   - **Arduino IDE** (ESP32 core) o **PlatformIO**.  
   - Aggiungi le librerie Arduino comunemente usate:  
     `ESP Async WebServer`, `AsyncTCP` (ESP32), `ArduinoJson`, `Adafruit_GFX` + `Adafruit_SH110X` (per OLED), `Adafruit_NeoPixel` (per LED).  
   - Compila e carica lo sketch **e** (se usato) il **filesystem** con i file della Web UI.  

2. **Connessione**
   - Al primo avvio prova STA con le credenziali salvate; se fallisce parte **AP** con SSID/PSK di default (configurabili).  
   - mDNS: `http://robora.local` (oppure l’IP).

3. **Web UI**
   - Apri il browser su `http://192.168.4.1/`.  
   - In alto a destra vedi lo stato WS. Entra in **Robot** per comandare (joystick o frecce).  
   - In **Config** salva Wi‑Fi/Motori/Telemetria; i valori persistono su NVS.  
   - In **OTA** aggiorni **Firmware** o **Filesystem** (progress in tempo reale).

---

## 🌐 Endpoint HTTP

- `GET /` → Web UI (SPA).
- `GET /health` → health‑check.
- `GET /Robot3d.glb` → modello 3D della pagina Robot.
- `POST /upload_image` → carica un’immagine (già convertita 128×64 monocromatica dalla UI) sul display.
- `POST /update` → OTA `multipart/form-data` (firmware o FS).  
- `POST /ota` → OTA `application/octet-stream` (firmware o FS).

> La UI **può** inviare header di integrità (es. SHA‑256) e di selezione partizione/target.

---

## 🔄 Protocollo WebSocket (estratto)

Messaggi JSON; i comandi principali lato **client → ESP32**:

| CMD            | Payload (esempio)                                             | Risposta/Note                            |
|----------------|---------------------------------------------------------------|------------------------------------------|
| `hello_robora` | `{ "client":"webui", "ver":"1.0" }`                           | ESP32 risponde con `hello_webui`.        |
| `info_req`     | —                                                             | Info runtime: IP, RSSI, uptime, heap, …  |
| `config_req`   | —                                                             | Schema + valori correnti.                |
| `config_rd`    | `{ "key":"wifi.ssid" }`                                       | Valore.                                  |
| `config_wr`    | `{ "key":"moto.maxVel", "val":100 }`                          | Applica, salva su NVS.                   |
| `move`         | `{ "x":-127..127, "y":-127..127 }`                            | Aggiorna motori; `y=throttle`, `x=steer` |
| `function`     | `{ "slot":0..7 }`                                             | Esegue callback registrato.              |
| `displaymsg`   | `{ "text":"Hello", "mode":"scroll|page|hold" }`               | Mostra su OLED.                          |
| `reboot`       | —                                                             | Riavvio.                                 |

Telemetria **ESP32 → client**: pacchetti `sensor` con IMU (angoli, mag, temp) a intervalli configurabili.

---

## ⚙️ Configurazione (NVS)

Parametri raggruppati (esempi di chiavi):

- **Wi‑Fi:** `wifi.mode` (STA/AP), `wifi.ssid`, `wifi.psk`, `ap.ssid`, `ap.psk`, `mdns.host`.  
- **Motori:** `moto.maxVel`, `moto.deadzone`, `moto.expoPct`, `moto.steerGain`, `moto.arcadeK`, `moto.arcadeEnabled`, `moto.invertA`, `moto.invertB`, `moto.tank`.  
- **Telemetria:** `tele.enabled`, `tele.period`, `tele.refresh` (broadcast), `i2c.freq` (es. 400kHz).  
- **Display/LED:** `disp.enabled`, `disp.scroll`, `led.mode`…  

La **pagina Config** legge lo **schema** dal firmware (`config_req`) e costruisce i form dinamicamente (etichette, min/max, tipo dato).

---

## 🔧 Build & Flash

### Arduino IDE
1. Installa il core **ESP32**.
2. Aggiungi le librerie elencate in “Avvio rapido”.
3. Se usi l’OLED o il LED, verifica i pin in `all_define.h`/`motors.h` e modifica se necessario.
4. Carica lo sketch. Per la **Web UI**, carica anche lo **SPIFFS/LittleFS** (dipende dalla tua partizione e dalle define nel progetto).

### PlatformIO (consigliato)
- Crea un progetto ESP32‑C3, copia i sorgenti in `src/` e la UI in `data/` (se usi FS).  
- Configura l’upload del filesystem (`pio run -t uploadfs`).
> OTA: una volta online, puoi aggiornare da browser nella pagina **OTA** (consigliato) oppure con `curl` verso `/update` o `/ota`.

---

## 🧪 Suggerimenti & Safety

- I motori vengono **forzati a 0** quando non ci sono client WebSocket connessi (fail‑safe).  
- La **modalità AP** permette di recuperare il dispositivo se le credenziali STA non sono valide.    
- La pagina **Display** converte le immagini lato browser in 128×64 1‑bit e le invia con `multipart/form-data` a `/upload_image`.

---

## 📁 Struttura (cartelle/fiIe principali)


---

## ❓ Troubleshooting

- **Non vedo la UI** → prova `http://robora.local` o l’IP mostrato su seriale; verifica che lo SPIFFS/LittleFS sia stato caricato.  
- **WS disconnesso** → controlla che l’orologio del browser non blocchi WS (reti aziendali), e che la porta `80` sia aperta sulla tua rete.  
- **OTA fallisce** → usa il metodo alternativo (`/update` ↔ `/ota`) e verifica la partizione/target corretti; riduci la dimensione del FS se necessario.  
- **Display/IMU non funzionano** → verifica collegamenti I²C e il tipo di display (SH1106G) e la frequenza del bus.

---

## 📜 Licenza

Distribuito con licenza **MIT**. Vedi header dei file sorgente.

---

## 👤 Credits

© 2025 **Orazio Franco** — <robora2025@gmail.com>  
Contributi e PR benvenuti!