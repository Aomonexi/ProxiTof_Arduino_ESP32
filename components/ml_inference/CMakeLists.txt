idf_component_register(
    SRCS "ml_inference.cpp"
    INCLUDE_DIRS "include"
    REQUIRES "esp-tflite-micro" "esp_timer" "freertos" "log"
)

# Enable ESP-NN optimizations (automatic with esp-tflite-micro)
# ESP-NN provides SIMD acceleration for:
# - Fully connected layers (matrix multiply)
# - Convolution operations
# - Activation functions (ReLU, etc.)
# 
# On ESP32-S3, this uses the vector extensions for 5-10x speedup
