# ProxiTof Documentation Index

**Last Updated**: January 31, 2026

---

## 🚀 Quick Start Guides

| Guide | Description |
|-------|-------------|
| [QUICK_START_GUIDE.md](QUICK_START_GUIDE.md) | Getting started with ProxiTof |
| [esp-idf-build-guide.md](esp-idf-build-guide.md) | Building and flashing firmware |
| [complete-wiring-diagram.md](complete-wiring-diagram.md) | Hardware connections |

---

## 📚 Complete Guides (Consolidated)

These comprehensive guides consolidate multiple related documents:

| Guide | Description |
|-------|-------------|
| [USB_COMPLETE_GUIDE.md](USB_COMPLETE_GUIDE.md) | USB Transfer Mode - complete reference |
| [ML_COMPLETE_GUIDE.md](ML_COMPLETE_GUIDE.md) | Machine Learning - training and deployment |
| [OLED_COMPLETE_GUIDE.md](OLED_COMPLETE_GUIDE.md) | OLED Display - modes and configuration |
| [BUTTON_COMPLETE_GUIDE.md](BUTTON_COMPLETE_GUIDE.md) | Button behavior - all modes |

---

## 📖 Thesis Notes (DO NOT MODIFY)

These are thesis documentation files - keep intact:

| Note | Topic |
|------|-------|
| [note-1-development-journey.md](note-1-development-journey.md) | Project development history |
| [note-2-sensor-manager-implementation.md](note-2-sensor-manager-implementation.md) | Sensor integration |
| [note-3-esp-idf-migration-success.md](note-3-esp-idf-migration-success.md) | ESP-IDF migration |
| [note-4-data-processor-implementation.md](note-4-data-processor-implementation.md) | Data processing |
| [note-5-near-miss-algorithm-integration.md](note-5-near-miss-algorithm-integration.md) | Algorithm integration |
| [note-5-relu-activation-explained.md](note-5-relu-activation-explained.md) | ReLU activation |
| [note-6-gps-buzzer-components.md](note-6-gps-buzzer-components.md) | GPS and buzzer |
| [note-6-tensorflow-explained.md](note-6-tensorflow-explained.md) | TensorFlow basics |
| [note-7-OLED_RESOLUTION_FINAL.md](note-7-OLED_RESOLUTION_FINAL.md) | OLED resolution |
| [note-8-ds3231-rtc-time-persistence-fix.md](note-8-ds3231-rtc-time-persistence-fix.md) | RTC time persistence |
| [note-9-gps-geotagging-implementation.md](note-9-gps-geotagging-implementation.md) | GPS geotagging |
| [note-10-usb-host-device-mode-fix.md](note-10-usb-host-device-mode-fix.md) | USB mode fix |
| [note-11-ml-training-methodology-and-validation.md](note-11-ml-training-methodology-and-validation.md) | **ML methodology (thesis-quality)** |
| [note-12-detection-mode-cycling.md](note-12-detection-mode-cycling.md) | Detection mode cycling |

---

## 🔧 Hardware Documentation

### Wiring & GPIO
| Document | Description |
|----------|-------------|
| [complete-wiring-diagram.md](complete-wiring-diagram.md) | All component connections |
| [GPIO_PIN_REFERENCE.md](GPIO_PIN_REFERENCE.md) | GPIO assignments |
| [GPIO_VERIFICATION_COMPLETE.md](GPIO_VERIFICATION_COMPLETE.md) | GPIO verification |
| [HARDWARE-CHECKLIST.md](HARDWARE-CHECKLIST.md) | Hardware checklist |
| [WIRING-CLARIFICATIONS.md](WIRING-CLARIFICATIONS.md) | Wiring clarifications |

### Sensors
| Document | Description |
|----------|-------------|
| [vl53l8cx-integration-fixes.md](vl53l8cx-integration-fixes.md) | ToF sensor integration |
| [sensor-manager-proxitof-ag-integration.md](sensor-manager-proxitof-ag-integration.md) | Sensor manager |
| [SENSOR_BLOCKAGE_FILTER_FIX.md](SENSOR_BLOCKAGE_FILTER_FIX.md) | Sensor filtering |
| [IMU_RESCAN_IMPLEMENTATION_COMPLETE.md](IMU_RESCAN_IMPLEMENTATION_COMPLETE.md) | IMU rescan |

### GPS
| Document | Description |
|----------|-------------|
| [GPS_TESTING_GUIDE.md](GPS_TESTING_GUIDE.md) | GPS testing |
| [GPS_DATA_STRATEGY.md](GPS_DATA_STRATEGY.md) | GPS data strategy |
| [GPS_FINAL_CONFIGURATION.md](GPS_FINAL_CONFIGURATION.md) | GPS configuration |
| [GPS_WIRING_CORRECTION_CRITICAL.md](GPS_WIRING_CORRECTION_CRITICAL.md) | GPS wiring |
| [GPS_UART_TROUBLESHOOTING.md](GPS_UART_TROUBLESHOOTING.md) | GPS troubleshooting |
| [gnss-library-options.md](gnss-library-options.md) | GNSS library options |

### Buzzer
| Document | Description |
|----------|-------------|
| [BUZZER_VOLUME_CONTROL_IMPLEMENTATION.md](BUZZER_VOLUME_CONTROL_IMPLEMENTATION.md) | Volume control |
| [HOW-POTENTIOMETER-CONTROLS-BUZZER.md](HOW-POTENTIOMETER-CONTROLS-BUZZER.md) | Potentiometer control |
| [LOGARITHMIC_POT_FIX.md](LOGARITHMIC_POT_FIX.md) | Logarithmic pot fix |
| [VOLUME_CONTROL_TROUBLESHOOTING.md](VOLUME_CONTROL_TROUBLESHOOTING.md) | Volume troubleshooting |
| [gps-buzzer-integration-guide.md](gps-buzzer-integration-guide.md) | GPS-buzzer integration |

---

## 💾 Storage & Data

| Document | Description |
|----------|-------------|
| [STORAGE_AND_USB_MSC_EXPLAINED.md](STORAGE_AND_USB_MSC_EXPLAINED.md) | Storage system overview |
| [storage-integration-guide.md](storage-integration-guide.md) | Storage integration |
| [data-logging-strategy.md](data-logging-strategy.md) | Data logging |
| [DATA_SAFETY_STRATEGY.md](DATA_SAFETY_STRATEGY.md) | Data safety |
| [what-to-expect-on-sd-card.md](what-to-expect-on-sd-card.md) | SD card contents |
| [sd-card-enable-guide.md](sd-card-enable-guide.md) | SD card setup |
| [sd-card-sdio-setup.md](sd-card-sdio-setup.md) | SDIO setup |
| [ZERO_KB_FILES_SOLUTION.md](ZERO_KB_FILES_SOLUTION.md) | Zero KB files fix |
| [CSV_FORMAT_CORRECTION.md](CSV_FORMAT_CORRECTION.md) | CSV format |

---

## 🤖 Machine Learning

| Document | Description |
|----------|-------------|
| [ML_COMPLETE_GUIDE.md](ML_COMPLETE_GUIDE.md) | **Complete ML guide** |
| [ML_MODEL_DEPLOYMENT_EXPLAINED.md](ML_MODEL_DEPLOYMENT_EXPLAINED.md) | Model deployment |
| [ESPNN_IMPLEMENTATION_GUIDE.md](ESPNN_IMPLEMENTATION_GUIDE.md) | ESP-NN optimization |
| [ESPNN_TRAINING_WORKFLOW.md](ESPNN_TRAINING_WORKFLOW.md) | Training workflow |
| [PERFORMANCE_OPTIMIZATION_GUIDE.md](PERFORMANCE_OPTIMIZATION_GUIDE.md) | Performance tuning |
| [esp32-tflite-implementation-guide.md](esp32-tflite-implementation-guide.md) | TFLite implementation |
| [ml-training-guide.md](ml-training-guide.md) | Training guide |
| [ml-training-workflow-plan.md](ml-training-workflow-plan.md) | Workflow plan |
| [ml-training-complete-guide.md](ml-training-complete-guide.md) | Complete training |
| [TRAINING_DATA_SOURCES.md](TRAINING_DATA_SOURCES.md) | Data sources |
| [FEATURE_COUNT_CLARIFICATION.md](FEATURE_COUNT_CLARIFICATION.md) | Feature count |
| [QUICK_START_ML_EXPLAINED.md](QUICK_START_ML_EXPLAINED.md) | ML quick start |
| [why-machine-learning-justification.md](why-machine-learning-justification.md) | ML justification |

---

## 🎯 Near-Miss Detection

| Document | Description |
|----------|-------------|
| [near-miss-detection-algorithm.md](near-miss-detection-algorithm.md) | Detection algorithm |
| [near-miss-definition-and-ml-pipeline.md](near-miss-definition-and-ml-pipeline.md) | Definition and pipeline |
| [NEAR_MISS_DETECTION_ANALYSIS.md](NEAR_MISS_DETECTION_ANALYSIS.md) | Detection analysis |
| [NEAR_MISS_THRESHOLD_UPDATE.md](NEAR_MISS_THRESHOLD_UPDATE.md) | Threshold update |
| [HYBRID_DETECTION_STRATEGY.md](HYBRID_DETECTION_STRATEGY.md) | Hybrid strategy |
| [PRIORITY_BASED_NEAR_MISS_DETECTION.md](PRIORITY_BASED_NEAR_MISS_DETECTION.md) | Priority-based detection |
| [TOF_FILTERING_AND_NEAR_MISS_DETECTION_JAN29.md](TOF_FILTERING_AND_NEAR_MISS_DETECTION_JAN29.md) | ToF filtering |
| [QUICK-REFERENCE-NEAR-MISS.md](QUICK-REFERENCE-NEAR-MISS.md) | Quick reference |
| [DETECTION_MODE_INTEGRATION_COMPLETE.md](DETECTION_MODE_INTEGRATION_COMPLETE.md) | Mode integration |

---

## ⏰ Time Management

| Document | Description |
|----------|-------------|
| [smart-time-system-guide.md](smart-time-system-guide.md) | Smart time system |
| [ds3231-rtc-setup-guide.md](ds3231-rtc-setup-guide.md) | DS3231 RTC setup |
| [ds3231-rtc-verification-guide.md](ds3231-rtc-verification-guide.md) | RTC verification |
| [wifi-time-sync-guide.md](wifi-time-sync-guide.md) | WiFi time sync |
| [TIME_SYNC_IMPLEMENTATION.md](TIME_SYNC_IMPLEMENTATION.md) | Time sync implementation |
| [TIME_SYNC_SOLUTION.md](TIME_SYNC_SOLUTION.md) | Time sync solution |
| [WIFI_SYNC_AND_DATE_DISPLAY_JAN27.md](WIFI_SYNC_AND_DATE_DISPLAY_JAN27.md) | WiFi sync and display |
| [RTC_AND_LOGGING_FIX_JAN27.md](RTC_AND_LOGGING_FIX_JAN27.md) | RTC and logging fix |

---

## 🖥️ Display (OLED)

| Document | Description |
|----------|-------------|
| [OLED_COMPLETE_GUIDE.md](OLED_COMPLETE_GUIDE.md) | **Complete OLED guide** |
| [Current Modes and Layouts.md](Current%20Modes%20and%20Layouts.md) | Display modes |
| [OLED_MODE_BUTTON.md](OLED_MODE_BUTTON.md) | Mode button |
| [OLED_CRASH_FIX.md](OLED_CRASH_FIX.md) | Crash fix |
| [OLED_SPI_FIXED.md](OLED_SPI_FIXED.md) | SPI fix |
| [OLED_DUAL_COLOR_GUIDE.md](OLED_DUAL_COLOR_GUIDE.md) | Dual color |
| [OLED_IMPLEMENTATION_OPTIONS.md](OLED_IMPLEMENTATION_OPTIONS.md) | Implementation options |
| [oled-display-spi-guide.md](oled-display-spi-guide.md) | SPI guide |
| [oled-4wire-spi-wiring-guide.md](oled-4wire-spi-wiring-guide.md) | 4-wire SPI wiring |

---

## 🔌 USB System

| Document | Description |
|----------|-------------|
| [USB_COMPLETE_GUIDE.md](USB_COMPLETE_GUIDE.md) | **Complete USB guide** |
| [usb-interface-guide.md](usb-interface-guide.md) | USB interface |
| [usb-mass-storage-guide.md](usb-mass-storage-guide.md) | Mass storage |
| [usb-quick-start-guide.md](usb-quick-start-guide.md) | Quick start |
| [usb-system-explained.md](usb-system-explained.md) | System explained |
| [usb-commands-reference.md](usb-commands-reference.md) | Commands reference |
| [usb-flash-partition-guide.md](usb-flash-partition-guide.md) | Flash partition |
| [usb-flash-partition-implementation-summary.md](usb-flash-partition-implementation-summary.md) | Implementation |
| [usb-msc-final-solution.md](usb-msc-final-solution.md) | MSC solution |
| [usb-msc-implementation-notes.md](usb-msc-implementation-notes.md) | Implementation notes |
| [usb-direct-sd-card-guide.md](usb-direct-sd-card-guide.md) | Direct SD card |
| [usb-flash-drive-alternatives.md](usb-flash-drive-alternatives.md) | Alternatives |
| [automatic-usb-detection-guide.md](automatic-usb-detection-guide.md) | Auto detection |

---

## 🛠️ Build & Development

| Document | Description |
|----------|-------------|
| [esp-idf-build-guide.md](esp-idf-build-guide.md) | Build guide |
| [esp-idf-installation-guide.md](esp-idf-installation-guide.md) | Installation |
| [esp-idf-migration.md](esp-idf-migration.md) | Migration |
| [esp-idf-component-structure.md](esp-idf-component-structure.md) | Component structure |
| [BUILD_ESP_IDF.md](BUILD_ESP_IDF.md) | Build ESP-IDF |
| [BUILD_FIX_JAN31.md](BUILD_FIX_JAN31.md) | Build fix |
| [ML_INFERENCE_BUILD_FIX_JAN31.md](ML_INFERENCE_BUILD_FIX_JAN31.md) | ML inference build fix |
| [build-and-test-guide.md](build-and-test-guide.md) | Build and test |
| [testing-guide.md](testing-guide.md) | Testing guide |
| [testing-working-implementation.md](testing-working-implementation.md) | Working implementation |
| [PYTHON_SETUP.md](PYTHON_SETUP.md) | Python setup |
| [ML_PACKAGE_COMPATIBILITY.md](ML_PACKAGE_COMPATIBILITY.md) | Package compatibility |

---

## ⚙️ Configuration

| Document | Description |
|----------|-------------|
| [CONFIG_FILE_GUIDE.md](CONFIG_FILE_GUIDE.md) | Config file guide |
| [field-configuration-guide.md](field-configuration-guide.md) | Field configuration |
| [WIFI_SETTINGS_MODE.md](WIFI_SETTINGS_MODE.md) | WiFi settings |
| [GPS_LOGGING_CONFIG_CHANGES.md](GPS_LOGGING_CONFIG_CHANGES.md) | GPS logging config |
| [PIN_REASSIGNMENT_SUMMARY.md](PIN_REASSIGNMENT_SUMMARY.md) | Pin reassignment |
| [SHUTDOWN_MODE_IMPLEMENTATION.md](SHUTDOWN_MODE_IMPLEMENTATION.md) | Shutdown mode |
| [SHUTDOWN_MODE_CHECKLIST.md](SHUTDOWN_MODE_CHECKLIST.md) | Shutdown checklist |

---

## 📋 System Documentation

| Document | Description |
|----------|-------------|
| [system-overview.md](system-overview.md) | System overview |
| [system-operation-guide.md](system-operation-guide.md) | Operation guide |
| [HOW_THE_CODE_WORKS.md](HOW_THE_CODE_WORKS.md) | Code explanation |
| [quick-reference-card.md](quick-reference-card.md) | Quick reference |
| [implementation-status-and-next-steps.md](implementation-status-and-next-steps.md) | Status and next steps |
| [IMPLEMENTATION-STATUS.md](IMPLEMENTATION-STATUS.md) | Implementation status |
| [MIGRATION_SUMMARY.md](MIGRATION_SUMMARY.md) | Migration summary |
| [performance-optimization-plan.md](performance-optimization-plan.md) | Optimization plan |

---

## 📝 Fix Documentation

| Document | Description |
|----------|-------------|
| [AUTO_FLAG_AND_FLUSH_FIX_JAN29.md](AUTO_FLAG_AND_FLUSH_FIX_JAN29.md) | Auto flag fix |
| [FALSE_MANUAL_FLAG_FIX.md](FALSE_MANUAL_FLAG_FIX.md) | Manual flag fix |
| [FLAG_INDICATOR_OPTIONS.md](FLAG_INDICATOR_OPTIONS.md) | Flag indicators |
| [VOLUME_AND_TOF_FIXES.md](VOLUME_AND_TOF_FIXES.md) | Volume and ToF fixes |
| [led-responsiveness-fix.md](led-responsiveness-fix.md) | LED fix |
| [gyroscope-and-button-improvements.md](gyroscope-and-button-improvements.md) | Gyro and button |
| [main-application-improvements.md](main-application-improvements.md) | Main app improvements |
| [storage-buffer-optimization.md](storage-buffer-optimization.md) | Buffer optimization |
| [GPS_AND_FLUSH_IMPROVEMENTS_SUMMARY.md](GPS_AND_FLUSH_IMPROVEMENTS_SUMMARY.md) | GPS improvements |
| [BUTTON_AND_DISPLAY_FIXES_JAN26.md](BUTTON_AND_DISPLAY_FIXES_JAN26.md) | Button and display |
| [MATHEMATICAL_FORMULATIONS_VERIFICATION.md](MATHEMATICAL_FORMULATIONS_VERIFICATION.md) | Math verification |

---

## 📄 Session Summaries

| Document | Description |
|----------|-------------|
| [FINAL_SUMMARY_JAN31.md](FINAL_SUMMARY_JAN31.md) | Final summary Jan 31 |
| [CONTEXT_TRANSFER_COMPLETE_JAN31.md](CONTEXT_TRANSFER_COMPLETE_JAN31.md) | Context transfer |
| [README_UPDATE_JAN31.md](README_UPDATE_JAN31.md) | README update |
| [thesis-new-features-report.md](thesis-new-features-report.md) | Thesis features |
| [Questions_answered.md](Questions_answered.md) | Q&A |
| [user-questions-answered.md](user-questions-answered.md) | User Q&A |
| [ESP32_ML_SETUP_GUIDE.md](ESP32_ML_SETUP_GUIDE.md) | ML setup guide |

---

## 📊 Document Statistics

- **Total Documents**: ~120 (after cleanup)
- **Consolidated Guides**: 4 (USB, ML, OLED, Button)
- **Thesis Notes**: 14 (preserved)
- **Deleted Duplicates**: ~65

---

## 🔍 Finding Documentation

### By Topic
- **USB issues**: Start with `USB_COMPLETE_GUIDE.md`
- **ML training**: Start with `ML_COMPLETE_GUIDE.md`
- **Display issues**: Start with `OLED_COMPLETE_GUIDE.md`
- **Button behavior**: Start with `BUTTON_COMPLETE_GUIDE.md`
- **Thesis writing**: Use `note-11-ml-training-methodology-and-validation.md`

### By Task
- **Building firmware**: `esp-idf-build-guide.md`
- **Wiring hardware**: `complete-wiring-diagram.md`
- **Training ML model**: `ML_COMPLETE_GUIDE.md`
- **Accessing files via USB**: `USB_COMPLETE_GUIDE.md`
- **Understanding modes**: `Current Modes and Layouts.md`

---

**Documentation maintained by Kiro AI Assistant**
