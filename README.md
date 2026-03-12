# Philco AC Remote for Flipper Zero

A Flipper Zero application to control Philco air conditioners via infrared.

![Flipper Zero](https://img.shields.io/badge/Flipper%20Zero-FAP-orange)
![License](https://img.shields.io/badge/license-MIT-blue)

## Features

- **4 modes**: Cool, Heat, Dry, Fan
- **Temperature**: 18°C – 30°C
- **Fan speed**: Auto, Low, Medium, High
- **Swing toggle**
- **Display toggle** (AC unit's LED display on/off)
- **Power on/off**
- **Settings persistence** across sessions
- **LED blink** feedback on IR transmission

## Installation

### Option 1: Download the .fap (easiest)

1. Download `philco_ac.fap` from the [Releases](../../releases/latest) page
2. Connect your Flipper Zero via USB or use the Flipper Mobile App
3. Copy the file to `SD Card/apps/Infrared/` on your Flipper
4. The app will appear under **Apps > Infrared > Philco AC**

### Option 2: Build from source

Requires [ufbt](https://github.com/flipperdevices/flipperzero-ufbt) (micro Flipper Build Tool):

```bash
pip install ufbt
git clone https://github.com/tarikbc/Philco-Flipper.git
cd Philco-Flipper
ufbt launch    # builds and deploys to connected Flipper
```

Or build without deploying:
```bash
ufbt            # output: dist/philco_ac.fap
```

## Usage

Navigate buttons with the **D-pad**, press **OK** to activate.

| Button | Action |
|--------|--------|
| Power | Toggle AC power on/off |
| Mode | Cycle: Cool > Heat > Dry > Fan |
| Temp Up/Down | Adjust temperature (18-30°C) |
| Fan | Cycle: Auto > Low > Med > High |
| Swing | Toggle swing on/off |
| LED | Toggle AC display on/off |

Settings are transmitted via IR automatically when changed (while power is on). Your last settings are saved and restored on next launch.

## Compatibility

Protocol was decoded from the **Philco PH9000QFM4**. Should work with other Philco models that use the same 15-byte IR protocol (0x56 signature byte, 38kHz carrier).

**Known compatible models:**
- Philco PH9000QFM4

If you test this with another Philco model, please open an issue to let us know!

## License

MIT — see [LICENSE](LICENSE).

## Credits

- UI based on [flipperzero-midea-ac-remote](https://github.com/xakep666/flipperzero-midea-ac-remote) by xakep666
- Button panel design from [flipperzero-mitsubishi-ac-remote](https://github.com/achistyakov/flipperzero-mitsubishi-ac-remote) by achistyakov
- IR protocol decoded from [esphome-philco-ac](https://github.com/brunoamui/esphome-philco-ac) by brunoamui
