# ProxiTof Documentation Index

**Last Updated**: March 24, 2026

---

## Quick Start

| Guide | Description |
|-------|-------------|
| [esp-idf-build-and-flash-guide.md](esp-idf-build-and-flash-guide.md) | Build, flash, and monitor firmware (ESP-IDF v5.5.1) |
| [esp-idf-installation-guide.md](esp-idf-installation-guide.md) | Installing ESP-IDF on Windows/macOS/Linux |
| [complete-wiring-diagram.md](complete-wiring-diagram.md) | Full hardware wiring reference |
| [quick-reference-card.md](quick-reference-card.md) | Quick reference for modes, buttons, pins |

---

## Complete Guides

| Guide | Description |
|-------|-------------|
| [oled-display-guide.md](oled-display-guide.md) | OLED display — modes, wiring, depth map |
| [usb-transfer-guide.md](usb-transfer-guide.md) | USB transfer mode — file access from PC |
| [ml-complete-guide.md](ml-complete-guide.md) | Machine learning — training and deployment |
| [button-guide.md](button-guide.md) | Button behavior across all modes |

---

## System & Architecture

| Document | Description |
|----------|-------------|
| [system-overview.md](system-overview.md) | System architecture and component overview |
| [system-operation-guide.md](system-operation-guide.md) | How to operate the device |
| [how-the-code-works.md](how-the-code-works.md) | Code walkthrough |
| [display-modes-and-layouts.md](display-modes-and-layouts.md) | Display modes and layouts |
| [shutdown-mode.md](shutdown-mode.md) | Safe shutdown mode |

---

## Hardware & Wiring

| Document | Description |
|----------|-------------|
| [complete-wiring-diagram.md](complete-wiring-diagram.md) | All component connections |
| [gpio-pin-reference.md](gpio-pin-reference.md) | GPIO pin assignments (verified from code) |

---

## GPS

| Document | Description |
|----------|-------------|
| [gps-final-configuration.md](gps-final-configuration.md) | GPS wiring (TX=38, RX=47), UART config |
| [gps-dynamic-model-and-antenna-guide.md](gps-dynamic-model-and-antenna-guide.md) | Dynamic models, antenna placement, GNSS |
| [gps-hardware-checklist.md](gps-hardware-checklist.md) | GPS hardware troubleshooting checklist |
| [gps-explained-simple.md](gps-explained-simple.md) | GPS explained simply (non-technical) |
| [gps-data-strategy.md](gps-data-strategy.md) | GPS data logging strategy |
| [gps-buzzer-integration-guide.md](gps-buzzer-integration-guide.md) | GPS + buzzer integration |

---

## Buzzer & Audio

| Document | Description |
|----------|-------------|
| [buzzer-volume-control.md](buzzer-volume-control.md) | Buzzer volume control (GPIO 45) |
| [potentiometer-buzzer-control.md](potentiometer-buzzer-control.md) | Potentiometer → buzzer mapping (GPIO 4) |

---

## Near-Miss Detection

| Document | Description |
|----------|-------------|
| [near-miss-detection-algorithm.md](near-miss-detection-algorithm.md) | Detection algorithm details |
| [near-miss-definition-and-ml-pipeline.md](near-miss-definition-and-ml-pipeline.md) | Definition and ML pipeline |
| [hybrid-detection-strategy.md](hybrid-detection-strategy.md) | Hybrid (algorithm + ML) strategy |
| [priority-based-detection.md](priority-based-detection.md) | Priority-based detection |

---

## Machine Learning

| Document | Description |
|----------|-------------|
| [ml-complete-guide.md](ml-complete-guide.md) | Complete ML guide (training → deployment) |
| [ml-quick-start.md](ml-quick-start.md) | ML training quick start |
| [espnn-implementation-guide.md](espnn-implementation-guide.md) | ESP-NN optimization for ESP32-S3 |
| [espnn-training-workflow.md](espnn-training-workflow.md) | Training workflow |
| [ml-training-workflow-plan.md](ml-training-workflow-plan.md) | Training workflow plan |
| [why-machine-learning-justification.md](why-machine-learning-justification.md) | ML justification for thesis |
| [python-setup.md](python-setup.md) | Python environment setup |
| [performance-optimization-guide.md](performance-optimization-guide.md) | Performance tuning |

---

## Storage & Data

| Document | Description |
|----------|-------------|
| [data-logging-strategy.md](data-logging-strategy.md) | Data logging strategy |
| [data-safety-strategy.md](data-safety-strategy.md) | Data safety and integrity |
| [data-collection-field-guide.md](data-collection-field-guide.md) | Field data collection guide |
| [sd-card-enable-guide.md](sd-card-enable-guide.md) | SD card setup |
| [what-to-expect-on-sd-card.md](what-to-expect-on-sd-card.md) | SD card file structure |

---

## USB System

| Document | Description |
|----------|-------------|
| [usb-transfer-guide.md](usb-transfer-guide.md) | Complete USB transfer guide |
| [usb-interface-guide.md](usb-interface-guide.md) | USB serial interface |
| [usb-commands-reference.md](usb-commands-reference.md) | USB commands reference |

---

## Time Management

| Document | Description |
|----------|-------------|
| [smart-time-system-guide.md](smart-time-system-guide.md) | Smart time system (RTC + WiFi + GPS) |
| [ds3231-rtc-setup-guide.md](ds3231-rtc-setup-guide.md) | DS3231 RTC setup |

---

## Configuration

| Document | Description |
|----------|-------------|
| [config-file-guide.md](config-file-guide.md) | Config file reference (`/sdcard/config.txt`) |
| [field-configuration-guide.md](field-configuration-guide.md) | Field configuration |
| [wifi-settings-mode.md](wifi-settings-mode.md) | WiFi settings mode |

---

## Tools & Development

| Document | Description |
|----------|-------------|
| [tools-reference.md](tools-reference.md) | Python tools reference |
| [esp-idf-build-and-flash-guide.md](esp-idf-build-and-flash-guide.md) | Build and flash guide |

---

## Thesis

| Document | Description |
|----------|-------------|
| [thesis-layout-and-recommendation.md](thesis-layout-and-recommendation.md) | Thesis structure |
| [thesis-new-features-report.md](thesis-new-features-report.md) | New features report |

---

## Thesis Notes (DO NOT MODIFY)

| Note | Topic |
|------|-------|
| [note-1](note-1-development-journey.md) | Development journey |
| [note-2](note-2-sensor-manager-implementation.md) | Sensor manager |
| [note-3](note-3-esp-idf-migration-success.md) | ESP-IDF migration |
| [note-4](note-4-data-processor-implementation.md) | Data processor |
| [note-5](note-5-near-miss-algorithm-integration.md) | Near-miss algorithm |
| [note-5b](note-5-relu-activation-explained.md) | ReLU activation |
| [note-6](note-6-gps-buzzer-components.md) | GPS and buzzer |
| [note-6b](note-6-tensorflow-explained.md) | TensorFlow basics |
| [note-7](note-7-OLED_RESOLUTION_FINAL.md) | OLED resolution |
| [note-8](note-8-ds3231-rtc-time-persistence-fix.md) | RTC time persistence |
| [note-9](note-9-gps-geotagging-implementation.md) | GPS geotagging |
| [note-10](note-10-usb-host-device-mode-fix.md) | USB mode fix |
| [note-11](note-11-ml-training-methodology-and-validation.md) | ML methodology (thesis-quality) |
| [note-12](note-12-detection-mode-cycling.md) | Detection mode cycling |
