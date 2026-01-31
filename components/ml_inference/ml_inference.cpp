/**
 * ml_inference.cpp
 * 
 * Machine Learning Inference with ESP-NN Acceleration
 * Optimized for ESP32-S3 real-time near-miss detection
 * 
 * Performance targets:
 * - Inference time: <15ms (allows 60Hz sensor loop)
 * - Memory: <15KB total (tensor arena + overhead)
 * - ESP-NN: Automatic SIMD acceleration on ESP32-S3
 */

#include "ml_inference.h"
#include "model_data.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// TFLite Micro includes
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

static const char *TAG = "ML_INFERENCE";

// State
static bool initialized = false;
static bool model_available = false;
static uint32_t last_inference_time_us = 0;  // Microseconds for precision
static uint32_t inference_count = 0;
static uint32_t total_inference_time_us = 0;

// TFLite components
static const tflite::Model* model = nullptr;
static tflite::MicroInterpreter* interpreter = nullptr;
static TfLiteTensor* input = nullptr;
static TfLiteTensor* output = nullptr;

// Tensor arena - optimized size for our small model (25→16→2)
// Model has ~450 parameters = ~1.8KB weights
// Input: 25 floats = 100 bytes
// Hidden: 16 floats = 64 bytes
// Output: 2 floats = 8 bytes
// Total tensors: ~2KB, with overhead: 8KB should be plenty
constexpr int kTensorArenaSize = 8 * 1024;  // 8KB (reduced from 10KB)

// Align tensor arena to 16 bytes for SIMD operations (ESP-NN optimization)
static uint8_t tensor_arena[kTensorArenaSize] __attribute__((aligned(16)));

extern "C" bool ml_inference_init(void) {
    ESP_LOGI(TAG, "Initializing ML inference...");
    ESP_LOGI(TAG, "ESP-NN: SIMD acceleration enabled for ESP32-S3");
    
    // Check if model is available
    if (model_data_len == 0) {
        ESP_LOGW(TAG, "No trained model available yet");
        ESP_LOGI(TAG, "To train model:");
        ESP_LOGI(TAG, "  1. Collect training data (ride bike, press button during near-misses)");
        ESP_LOGI(TAG, "  2. Copy CSV files from SD card to data/raw/");
        ESP_LOGI(TAG, "  3. Run: python tools/quick_start_ml.py");
        ESP_LOGI(TAG, "  4. Rebuild and flash firmware");
        ESP_LOGI(TAG, "ML inference will be disabled until model is trained");
        initialized = true;
        model_available = false;
        return true;  // Return true so system continues
    }
    
    // Load model
    model = tflite::GetModel(model_data);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        ESP_LOGE(TAG, "Model schema version mismatch!");
        ESP_LOGE(TAG, "  Model: %lu, Expected: %d", 
                 (unsigned long)model->version(), TFLITE_SCHEMA_VERSION);
        return false;
    }
    ESP_LOGI(TAG, "✅ Model loaded (version %lu, size %d bytes)", 
             (unsigned long)model->version(), model_data_len);
    
    // Register operations - ESP-NN automatically accelerates these!
    // Only register ops we actually use (minimizes memory footprint)
    static tflite::MicroMutableOpResolver<3> resolver;
    resolver.AddFullyConnected();  // ESP-NN SIMD accelerated
    resolver.AddRelu();            // ESP-NN SIMD accelerated  
    resolver.AddSoftmax();         // Standard implementation
    
    ESP_LOGI(TAG, "✅ Operations registered (ESP-NN SIMD acceleration active)");
    
    // Create interpreter with optimized settings
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
    
    // Verify tensor dimensions
    if (input->dims->data[1] != 25) {
        ESP_LOGE(TAG, "Input tensor size mismatch! Expected 25, got %d", 
                 input->dims->data[1]);
        return false;
    }
    
    ESP_LOGI(TAG, "✅ Tensors allocated");
    ESP_LOGI(TAG, "   Input:  [%d, %d] (25 features)", 
             input->dims->data[0], input->dims->data[1]);
    ESP_LOGI(TAG, "   Output: [%d, %d] (2 classes: safe, near-miss)", 
             output->dims->data[0], output->dims->data[1]);
    ESP_LOGI(TAG, "   Arena used: %zu / %d bytes (%.1f%%)", 
             interpreter->arena_used_bytes(), kTensorArenaSize,
             100.0f * interpreter->arena_used_bytes() / kTensorArenaSize);
    
    // Run warmup inference to prime caches
    ESP_LOGI(TAG, "Running warmup inference...");
    float dummy_features[25] = {0};
    for (int i = 0; i < 25; i++) {
        input->data.f[i] = dummy_features[i];
    }
    
    int64_t warmup_start = esp_timer_get_time();
    interpreter->Invoke();
    int64_t warmup_time = esp_timer_get_time() - warmup_start;
    
    ESP_LOGI(TAG, "   Warmup inference: %lld µs", warmup_time);
    
    initialized = true;
    model_available = true;
    
    ESP_LOGI(TAG, "🎉 ML inference ready!");
    ESP_LOGI(TAG, "   Expected inference time: 5-15ms with ESP-NN");
    ESP_LOGI(TAG, "   Target: <16.67ms for 60Hz operation");
    
    return true;
}

extern "C" float ml_inference_predict(const float features[25]) {
    if (!initialized) {
        ESP_LOGW(TAG, "ML inference not initialized!");
        return 0.0f;
    }
    
    if (!model_available || interpreter == nullptr || input == nullptr || output == nullptr) {
        // No model available or interpreter not ready - return 0 (no prediction)
        return 0.0f;
    }
    
    // Start timing (microseconds for precision)
    int64_t start_time = esp_timer_get_time();
    
    // Copy features to input tensor (optimized: direct memory copy)
    // Features are already normalized by caller
    float* input_data = input->data.f;
    for (int i = 0; i < 25; i++) {
        input_data[i] = features[i];
    }
    
    // Run inference (ESP-NN accelerates FullyConnected and ReLU automatically!)
    TfLiteStatus invoke_status = interpreter->Invoke();
    if (invoke_status != kTfLiteOk) {
        ESP_LOGE(TAG, "Invoke() failed!");
        return 0.0f;
    }
    
    // Get output probability (index 1 = near-miss probability)
    float prob_near_miss = output->data.f[1];
    
    // Calculate inference time
    int64_t end_time = esp_timer_get_time();
    last_inference_time_us = (uint32_t)(end_time - start_time);
    
    // Update statistics
    inference_count++;
    total_inference_time_us += last_inference_time_us;
    
    return prob_near_miss;
}

extern "C" bool ml_inference_is_ready(void) {
    return initialized && model_available;
}

extern "C" uint32_t ml_inference_get_latency_ms(void) {
    return last_inference_time_us / 1000;  // Convert µs to ms
}

extern "C" uint32_t ml_inference_get_latency_us(void) {
    return last_inference_time_us;
}

extern "C" uint32_t ml_inference_get_avg_latency_us(void) {
    if (inference_count == 0) return 0;
    return total_inference_time_us / inference_count;
}

extern "C" uint32_t ml_inference_get_count(void) {
    return inference_count;
}
