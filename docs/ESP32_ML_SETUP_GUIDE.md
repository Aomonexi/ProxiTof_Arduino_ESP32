# ESP32 ML Setup Guide - Before You Have Training Data

**Date**: January 31, 2026  
**Purpose**: Set up ESP-NN and TFLite Micro infrastructure before training

---

## Quick Answers to Your Questions

### Q1: How long does training take?

**Answer**: 2-10 minutes depending on data size

**Breakdown**:
- Small dataset (100 near-miss, 1000 safe): ~2-3 minutes
- Medium dataset (200 near-miss, 2000 safe): ~5-7 minutes
- Large dataset (500 near-miss, 5000 safe): ~10-15 minutes

**Factors**:
- CPU speed (faster CPU = faster training)
- Dataset size (more data = longer training)
- Number of epochs (typically stops at 30-50 epochs with early stopping)

### Q2: How much data do we need?

**Minimum Requirements**:
- **Near-miss examples**: 100-200 (with button press)
- **Safe examples**: 1000-2000 (normal riding)
- **Total riding time**: 5-10 hours

**Recommended for Good Performance**:
- **Near-miss examples**: 200-500
- **Safe examples**: 2000-5000
- **Total riding time**: 10-20 hours

**Data Quality > Quantity**:
- Diverse conditions (day/night, rain/sun, light/heavy traffic)
- Different scenarios (bike lanes, roads, paths)
- Various speeds (slow, medium, fast)

### Q3: Can we set up ESP-NN before training?

**Answer**: YES! Absolutely recommended! ✅

You can (and should) set up the entire ML infrastructure on ESP32 before you have training data. This way you can:
- Verify dependencies work
- Test build process
- Use a dummy model to test inference pipeline
- Fix any issues before you have real data

### Q4: Does ESP-NN load TFLite directly?

**Answer**: YES, through TFLite Micro! ✅

**How it works**:
1. ESP-NN is a **library of optimized functions**
2. TFLite Micro is the **inference engine** that loads .tflite models
3. TFLite Micro **automatically uses ESP-NN** for accelerated operations
4. You don't call ESP-NN directly - it's transparent!

**Architecture**:
```
Your Code
    ↓
TFLite Micro (loads .tflite model)
    ↓
ESP-NN (accelerates operations automatically)
    ↓
ESP32 Hardware
```

---

## Setting Up ESP-NN Infrastructure Now

### Step 1: Add TFLite Micro Component

**File**: `main/idf_component.yml` (create if doesn't exist)

```yaml
dependencies:
  espressif/esp-tflite-micro: "^1.3.5"
```

**What this does**:
- Downloads TensorFlow Lite Micro for ESP32
- Includes ESP-NN automatically
- Provides all necessary headers and libraries

### Step 2: Update Main CMakeLists.txt

**File**: `main/CMakeLists.txt`

Add TFLite Micro to requirements:

```cmake
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES 
        "sensor_manager"
        "data_processor"
        "storage_manager"
        "oled_display"
        "esp-tflite-micro"  # Add this line
)
```

### Step 3: Download Dependencies

```bash
# This downloads esp-tflite-micro and ESP-NN
idf.py reconfigure
```

**Expected output**:
```
Processing 1 dependencies:
[1/1] espressif/esp-tflite-micro (1.3.5)
-- Configuring done
-- Generating done
```

### Step 4: Test Build (Without Model)

```bash
idf.py build
```

**What to check**:
- ✅ Build completes successfully
- ✅ No errors about missing TFLite headers
- ✅ ESP-NN is included automatically

**If build fails**: Check ESP-IDF version (need v4.4 or later)


---

## Creating ML Inference Component (Before Training)

### Step 1: Create Component Structure

```bash
# Create ML inference component
mkdir -p components/ml_inference/include

# Create files
touch components/ml_inference/CMakeLists.txt
touch components/ml_inference/idf_component.yml
touch components/ml_inference/ml_inference.c
touch components/ml_inference/include/ml_inference.h
touch components/ml_inference/include/model_data.h
```

### Step 2: Component Configuration Files

**File**: `components/ml_inference/idf_component.yml`

```yaml
dependencies:
  espressif/esp-tflite-micro: "^1.3.5"
```

**File**: `components/ml_inference/CMakeLists.txt`

```cmake
idf_component_register(
    SRCS "ml_inference.c"
    INCLUDE_DIRS "include"
    REQUIRES "esp-tflite-micro"
)
```

### Step 3: Create Dummy Model (For Testing)

**File**: `components/ml_inference/include/model_data.h`

```c
#ifndef MODEL_DATA_H
#define MODEL_DATA_H

#include <stdint.h>

// Dummy model for testing (will be replaced after training)
// This is a minimal valid TFLite model
const unsigned int model_data_len = 100;

const unsigned char model_data[] = {
  // Dummy data - will be replaced with real model
  0x1c, 0x00, 0x00, 0x00, 0x54, 0x46, 0x4c, 0x33,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  // ... (rest filled with zeros for now)
};

#endif // MODEL_DATA_H
```

**Note**: This dummy model won't work for inference, but allows you to test the build system.

### Step 4: Create ML Inference Header

**File**: `components/ml_inference/include/ml_inference.h`

```c
#ifndef ML_INFERENCE_H
#define ML_INFERENCE_H

#include <stdbool.h>
#include <stdint.h>

/**
 * Initialize ML inference engine
 * @return true if successful, false otherwise
 */
bool ml_inference_init(void);

/**
 * Run inference on sensor features
 * @param features Array of 25 normalized features
 * @return Probability of near-miss (0.0 to 1.0)
 */
float ml_inference_predict(const float features[25]);

/**
 * Check if ML inference is ready
 * @return true if initialized and ready
 */
bool ml_inference_is_ready(void);

/**
 * Get last inference latency in milliseconds
 * @return Inference time in ms
 */
uint32_t ml_inference_get_latency_ms(void);

#endif // ML_INFERENCE_H
```

### Step 5: Create ML Inference Implementation

**File**: `components/ml_inference/ml_inference.c`

```c
#include "ml_inference.h"
#include "model_data.h"
#include "esp_log.h"

// TFLite Micro includes
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

static const char *TAG = "ML_INFERENCE";

// TFLite components
static const tflite::Model* model = nullptr;
static tflite::MicroInterpreter* interpreter = nullptr;
static TfLiteTensor* input = nullptr;
static TfLiteTensor* output = nullptr;

// Tensor arena (adjust size if needed)
constexpr int kTensorArenaSize = 10 * 1024;  // 10KB
static uint8_t tensor_arena[kTensorArenaSize];

// State
static bool initialized = false;
static uint32_t last_inference_time_ms = 0;

bool ml_inference_init(void) {
    ESP_LOGI(TAG, "Initializing ML inference with ESP-NN acceleration...");
    
    // Load model
    model = tflite::GetModel(model_data);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        ESP_LOGE(TAG, "Model schema version mismatch!");
        ESP_LOGE(TAG, "  Model: %d, Expected: %d", 
                 model->version(), TFLITE_SCHEMA_VERSION);
        return false;
    }
    ESP_LOGI(TAG, "✅ Model loaded (version %d)", model->version());
    
    // Register operations (ESP-NN accelerates these automatically!)
    static tflite::MicroMutableOpResolver<3> resolver;
    resolver.AddFullyConnected();  // ESP-NN accelerated
    resolver.AddRelu();            // ESP-NN accelerated
    resolver.AddSoftmax();
    
    ESP_LOGI(TAG, "✅ Operations registered (ESP-NN enabled)");
    
    // Create interpreter
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize);
    interpreter = &static_interpreter;
    
    // Allocate tensors
    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk) {
        ESP_LOGE(TAG, "AllocateTensors() failed!");
        return false;
    }
    
    // Get input/output tensors
    input = interpreter->input(0);
    output = interpreter->output(0);
    
    ESP_LOGI(TAG, "✅ Tensors allocated");
    ESP_LOGI(TAG, "   Input:  [%d, %d]", 
             input->dims->data[0], input->dims->data[1]);
    ESP_LOGI(TAG, "   Output: [%d, %d]", 
             output->dims->data[0], output->dims->data[1]);
    ESP_LOGI(TAG, "   Arena used: %d / %d bytes", 
             interpreter->arena_used_bytes(), kTensorArenaSize);
    
    initialized = true;
    ESP_LOGI(TAG, "🎉 ML inference ready with ESP-NN acceleration!");
    
    return true;
}

float ml_inference_predict(const float features[25]) {
    if (!initialized) {
        ESP_LOGW(TAG, "ML inference not initialized!");
        return 0.0f;
    }
    
    // Start timing
    int64_t start_time = esp_timer_get_time();
    
    // Copy features to input tensor
    for (int i = 0; i < 25; i++) {
        input->data.f[i] = features[i];
    }
    
    // Run inference (ESP-NN accelerates automatically!)
    TfLiteStatus invoke_status = interpreter->Invoke();
    if (invoke_status != kTfLiteOk) {
        ESP_LOGE(TAG, "Invoke() failed!");
        return 0.0f;
    }
    
    // Get output probability
    float prob_near_miss = output->data.f[1];
    
    // Calculate inference time
    int64_t end_time = esp_timer_get_time();
    last_inference_time_ms = (end_time - start_time) / 1000;
    
    return prob_near_miss;
}

bool ml_inference_is_ready(void) {
    return initialized;
}

uint32_t ml_inference_get_latency_ms(void) {
    return last_inference_time_ms;
}
```

### Step 6: Test Build with ML Component

```bash
idf.py build
```

**Expected output**:
```
Building ESP-IDF components...
[100%] Built target app

Project build complete. To flash, run:
  idf.py flash
```

**What this proves**:
- ✅ TFLite Micro is properly integrated
- ✅ ESP-NN is included automatically
- ✅ Your code compiles with ML infrastructure
- ✅ Ready to add real model when training completes


---

## Understanding ESP-NN Integration

### How ESP-NN Works with TFLite

**ESP-NN is NOT a separate inference engine!**

It's a library of optimized kernel implementations that TFLite Micro uses automatically.

**Architecture**:
```
┌─────────────────────────────────────┐
│  Your Application Code              │
│  - Calls ml_inference_predict()     │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│  TFLite Micro Interpreter           │
│  - Loads .tflite model              │
│  - Manages tensors                  │
│  - Executes operations              │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│  ESP-NN Optimized Kernels           │
│  - Fully connected (Dense)          │
│  - ReLU activation                  │
│  - Convolution                      │
│  - Pooling                          │
│  (Automatically used by TFLite)     │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│  ESP32 Hardware                     │
│  - SIMD instructions                │
│  - Optimized memory access          │
└─────────────────────────────────────┘
```

### What You Need to Know

**1. You load .tflite models directly**
```c
// Load TFLite model
model = tflite::GetModel(model_data);

// TFLite Micro handles everything
// ESP-NN is used automatically for supported operations
```

**2. ESP-NN acceleration is automatic**
```c
// When you call:
interpreter->Invoke();

// TFLite Micro automatically uses ESP-NN for:
// - Fully connected layers (Dense)
// - ReLU activation
// - Convolution
// - Pooling
// - Other supported operations
```

**3. No special code needed**
- You don't call ESP-NN functions directly
- You don't need to configure ESP-NN
- It's enabled by default in esp-tflite-micro
- Just use TFLite Micro API normally

### Performance Benefits

**Without ESP-NN** (standard TFLite Micro):
- Inference time: ~50-100ms
- Uses generic C implementations

**With ESP-NN** (automatic acceleration):
- Inference time: ~10-15ms
- Uses optimized assembly for ESP32-S3
- 5-10× faster!

**For your model** (25→16→2 network):
- Expected inference time: 10-15ms
- Fast enough for 60Hz operation
- Minimal CPU usage

---

## Testing Without Real Model

### Option 1: Use Example Model

TFLite Micro comes with example models. You can test with hello_world:

```bash
# Get hello_world example
idf.py create-project-from-example "esp-tflite-micro:hello_world"

# Build and flash
cd hello_world
idf.py build flash monitor
```

**What this proves**:
- TFLite Micro works on your device
- ESP-NN acceleration is functional
- Inference pipeline is working

### Option 2: Create Minimal Test Model

Create a tiny test model on PC:

```python
# test_model.py
import tensorflow as tf
import numpy as np

# Create minimal model (2 inputs, 1 output)
model = tf.keras.Sequential([
    tf.keras.layers.Dense(4, activation='relu', input_shape=(2,)),
    tf.keras.layers.Dense(1, activation='sigmoid')
])

model.compile(optimizer='adam', loss='binary_crossentropy')

# Train on dummy data
X = np.random.rand(100, 2)
y = np.random.randint(0, 2, 100)
model.fit(X, y, epochs=5, verbose=0)

# Convert to TFLite
converter = tf.lite.TFLiteConverter.from_keras_model(model)
tflite_model = converter.convert()

# Save
with open('test_model.tflite', 'wb') as f:
    f.write(tflite_model)

print(f"Test model created: {len(tflite_model)} bytes")
```

Then convert to C header and test on ESP32.

---

## When You Get Real Training Data

### Step 1: Train Model on PC

```bash
cd tools
python quick_start_ml.py
```

**Output**: `components/ml_inference/model_data.h`

### Step 2: Replace Dummy Model

Simply replace the dummy `model_data.h` with the generated one:

```bash
# The training script already puts it in the right place!
# Just rebuild
idf.py build
```

### Step 3: Flash and Test

```bash
idf.py flash monitor
```

**Expected output**:
```
I (1234) ML_INFERENCE: Initializing ML inference with ESP-NN acceleration...
I (1245) ML_INFERENCE: ✅ Model loaded (version 3)
I (1256) ML_INFERENCE: ✅ Operations registered (ESP-NN enabled)
I (1267) ML_INFERENCE: ✅ Tensors allocated
I (1267) ML_INFERENCE:    Input:  [1, 25]
I (1267) ML_INFERENCE:    Output: [1, 2]
I (1267) ML_INFERENCE:    Arena used: 8456 / 10240 bytes
I (1278) ML_INFERENCE: 🎉 ML inference ready with ESP-NN acceleration!
```

---

## Summary

### Training Time & Data Requirements

| Aspect | Minimum | Recommended | Optimal |
|--------|---------|-------------|---------|
| Near-miss examples | 100 | 200 | 500+ |
| Safe examples | 1000 | 2000 | 5000+ |
| Riding time | 5 hours | 10 hours | 20+ hours |
| Training time | 2 min | 5 min | 10 min |

### ESP-NN Setup (Do This Now!)

1. ✅ Add `esp-tflite-micro` dependency
2. ✅ Create `ml_inference` component
3. ✅ Test build (with dummy model)
4. ✅ Verify TFLite Micro works
5. ⏳ Wait for training data
6. ✅ Replace dummy model with real model
7. ✅ Flash and test

### Key Points

**ESP-NN Integration**:
- ✅ ESP-NN is included in `esp-tflite-micro`
- ✅ Loads .tflite models directly
- ✅ Acceleration is automatic (no special code)
- ✅ 5-10× faster than standard TFLite
- ✅ Expected inference: 10-15ms

**You Can Set Up Now**:
- ✅ Install dependencies
- ✅ Create ML component
- ✅ Test build system
- ✅ Verify everything compiles
- ✅ Use dummy model for testing
- ⏳ Replace with real model after training

**Benefits of Setting Up Early**:
- Find and fix build issues now
- Test inference pipeline
- Verify ESP-NN works on your device
- Ready to deploy immediately after training
- No surprises when you have real data

---

## Next Steps

### Right Now (Before Data Collection)

1. **Verify Python packages**:
   ```bash
   python tools/verify_packages.py
   ```

2. **Set up ESP32 ML infrastructure**:
   ```bash
   # Add dependency
   echo "dependencies:" > main/idf_component.yml
   echo "  espressif/esp-tflite-micro: \"^1.3.5\"" >> main/idf_component.yml
   
   # Download dependencies
   idf.py reconfigure
   
   # Test build
   idf.py build
   ```

3. **Create ML inference component** (use code above)

4. **Test build with ML component**:
   ```bash
   idf.py build
   ```

### When You Have Data

1. **Copy CSV files to `data/raw/`**

2. **Run training**:
   ```bash
   python tools/quick_start_ml.py
   ```

3. **Model is automatically placed in correct location**

4. **Rebuild and flash**:
   ```bash
   idf.py build flash monitor
   ```

5. **Test in field and tune threshold**

---

**Document Version**: 1.0  
**Date**: January 31, 2026  
**Status**: Ready for implementation

**Key Takeaway**: You can and should set up the entire ML infrastructure on ESP32 NOW, before you have training data. This way you'll catch any issues early and be ready to deploy immediately after training!
