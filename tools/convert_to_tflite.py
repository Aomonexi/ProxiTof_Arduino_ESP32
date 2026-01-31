"""
Convert trained Keras model to TensorFlow Lite format for ESP32
Optimized for ESP-NN acceleration
"""
import tensorflow as tf
import numpy as np
import os

def convert_to_tflite(model_path, output_path='../models/near_miss_model.tflite',
                     quantize=False):
    """
    Convert Keras model to TFLite format
    
    Args:
        model_path: Path to trained Keras model (.h5)
        output_path: Path to save TFLite model
        quantize: If True, apply int8 quantization (smaller model, faster inference)
    """
    
    print(f"\n{'='*60}")
    print("CONVERTING TO TENSORFLOW LITE")
    print(f"{'='*60}")
    print(f"Input model: {model_path}")
    print(f"Output model: {output_path}")
    print(f"Quantization: {'Enabled (int8)' if quantize else 'Disabled (float32)'}")
    
    # Load Keras model
    print("\nLoading Keras model...")
    model = tf.keras.models.load_model(model_path)
    print("✅ Model loaded")
    
    # Convert to TFLite
    print("\nConverting to TFLite...")
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    
    # Optimization settings
    if quantize:
        print("Applying int8 quantization...")
        converter.optimizations = [tf.lite.Optimize.DEFAULT]
        converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
        converter.inference_input_type = tf.int8
        converter.inference_output_type = tf.int8
        
        # Representative dataset for quantization
        def representative_dataset():
            data_dir = '../data/processed'
            X_train = np.load(os.path.join(data_dir, 'X_train.npy'))
            for i in range(min(100, len(X_train))):
                yield [X_train[i:i+1].astype(np.float32)]
        
        converter.representative_dataset = representative_dataset
    else:
        # Float32 model (no quantization)
        converter.optimizations = [tf.lite.Optimize.DEFAULT]
    
    # Convert
    tflite_model = converter.convert()
    
    # Save
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, 'wb') as f:
        f.write(tflite_model)
    
    print(f"✅ TFLite model saved")
    
    # Model size
    model_size = len(tflite_model)
    print(f"\nModel size: {model_size:,} bytes ({model_size/1024:.2f} KB)")
    
    if model_size > 100*1024:
        print(f"⚠️  WARNING: Model is large (>{100}KB)")
        print("   Consider enabling quantization to reduce size")
    
    return tflite_model

def test_tflite_model(tflite_model_path, test_data_path='../data/processed'):
    """Test TFLite model accuracy"""
    
    print(f"\n{'='*60}")
    print("TESTING TFLITE MODEL")
    print(f"{'='*60}")
    
    # Load TFLite model
    interpreter = tf.lite.Interpreter(model_path=tflite_model_path)
    interpreter.allocate_tensors()
    
    # Get input/output details
    input_details = interpreter.get_input_details()
    output_details = interpreter.get_output_details()
    
    print(f"\nInput details:")
    print(f"  Shape: {input_details[0]['shape']}")
    print(f"  Type:  {input_details[0]['dtype']}")
    
    print(f"\nOutput details:")
    print(f"  Shape: {output_details[0]['shape']}")
    print(f"  Type:  {output_details[0]['dtype']}")
    
    # Load test data
    X_test = np.load(os.path.join(test_data_path, 'X_test.npy'))
    y_test = np.load(os.path.join(test_data_path, 'y_test.npy'))
    
    print(f"\nTesting on {len(X_test)} samples...")
    
    # Run inference
    predictions = []
    for i in range(len(X_test)):
        # Prepare input
        input_data = X_test[i:i+1].astype(input_details[0]['dtype'])
        
        # Set input
        interpreter.set_tensor(input_details[0]['index'], input_data)
        
        # Run inference
        interpreter.invoke()
        
        # Get output
        output_data = interpreter.get_tensor(output_details[0]['index'])
        predictions.append(output_data[0])
    
    predictions = np.array(predictions)
    y_pred = np.argmax(predictions, axis=1)
    
    # Calculate accuracy
    accuracy = np.mean(y_pred == y_test)
    
    print(f"\n✅ TFLite model accuracy: {accuracy:.2%}")
    
    # Compare with original Keras model
    keras_model = tf.keras.models.load_model('../models/near_miss_model.h5')
    keras_pred = keras_model.predict(X_test, verbose=0)
    keras_pred_class = np.argmax(keras_pred, axis=1)
    keras_accuracy = np.mean(keras_pred_class == y_test)
    
    print(f"Keras model accuracy:  {keras_accuracy:.2%}")
    print(f"Accuracy difference:   {abs(accuracy - keras_accuracy):.2%}")
    
    if abs(accuracy - keras_accuracy) > 0.01:
        print(f"\n⚠️  WARNING: Accuracy difference is significant (>1%)")
        print("   This may indicate quantization issues")
    else:
        print(f"\n✅ TFLite model matches Keras model accuracy")
    
    return accuracy

def generate_c_header(tflite_model_path, output_path='../components/ml_inference/include/model_data.h',
                     scaler_path='../data/processed/scaler_params.npz'):
    """Generate C header file with model data and scaler parameters"""
    
    print(f"\n{'='*60}")
    print("GENERATING C HEADER FILE")
    print(f"{'='*60}")
    
    # Read TFLite model
    with open(tflite_model_path, 'rb') as f:
        tflite_model = f.read()
    
    # Load scaler parameters if available
    scaler_mean = None
    scaler_std = None
    if os.path.exists(scaler_path):
        scaler_data = np.load(scaler_path)
        scaler_mean = scaler_data['mean']
        scaler_std = scaler_data['std']
        print(f"✅ Loaded scaler parameters from {scaler_path}")
    else:
        print(f"⚠️  Scaler parameters not found at {scaler_path}")
        print("   Using placeholder values (update after training)")
        # Placeholder values - will be updated by training
        scaler_mean = np.zeros(25)
        scaler_std = np.ones(25)
    
    # Generate C array
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    
    with open(output_path, 'w') as f:
        f.write("/**\n")
        f.write(" * Auto-generated TFLite model data and scaler parameters\n")
        f.write(" * DO NOT EDIT MANUALLY - Generated by convert_to_tflite.py\n")
        f.write(f" * Generated: {np.datetime64('now')}\n")
        f.write(" */\n\n")
        f.write("#ifndef MODEL_DATA_H\n")
        f.write("#define MODEL_DATA_H\n\n")
        f.write("#include <stdint.h>\n\n")
        
        # Model data
        f.write(f"// Model size: {len(tflite_model)} bytes ({len(tflite_model)/1024:.2f} KB)\n")
        f.write(f"const unsigned int model_data_len = {len(tflite_model)};\n\n")
        
        f.write("const unsigned char model_data[] = {\n")
        for i in range(0, len(tflite_model), 12):
            chunk = tflite_model[i:i+12]
            hex_values = ', '.join([f'0x{b:02x}' for b in chunk])
            f.write(f"  {hex_values},\n")
        f.write("};\n\n")
        
        # Scaler parameters for feature normalization
        f.write("// ============================================================\n")
        f.write("// Scaler parameters for feature normalization\n")
        f.write("// Features: 16 ToF (tof_0_0 to tof_3_3) + 9 IMU\n")
        f.write("// Order: tof[16], roll, pitch, yaw, accel_x/y/z, gyro_x/y/z\n")
        f.write("// ============================================================\n\n")
        
        # Mean values
        f.write("// Feature means (from training data)\n")
        f.write("static const float scaler_mean[25] = {\n")
        f.write("    // ToF features (16)\n")
        f.write("    " + ", ".join([f"{v:.6f}f" for v in scaler_mean[:16]]) + ",\n")
        f.write("    // IMU features (9): roll, pitch, yaw, accel_x/y/z, gyro_x/y/z\n")
        f.write("    " + ", ".join([f"{v:.6f}f" for v in scaler_mean[16:]]) + "\n")
        f.write("};\n\n")
        
        # Std values
        f.write("// Feature standard deviations (from training data)\n")
        f.write("static const float scaler_std[25] = {\n")
        f.write("    // ToF features (16)\n")
        f.write("    " + ", ".join([f"{v:.6f}f" for v in scaler_std[:16]]) + ",\n")
        f.write("    // IMU features (9): roll, pitch, yaw, accel_x/y/z, gyro_x/y/z\n")
        f.write("    " + ", ".join([f"{v:.6f}f" for v in scaler_std[16:]]) + "\n")
        f.write("};\n\n")
        
        f.write("#endif // MODEL_DATA_H\n")
    
    print(f"✅ C header file saved to {output_path}")
    print(f"   - Model data: {len(tflite_model)} bytes")
    print(f"   - Scaler mean: 25 values")
    print(f"   - Scaler std: 25 values")
    
    return output_path

if __name__ == '__main__':
    model_path = '../models/near_miss_model.h5'
    tflite_path = '../models/near_miss_model.tflite'
    
    # Convert to TFLite (float32)
    print("\n" + "="*60)
    print("OPTION 1: Float32 Model (No Quantization)")
    print("="*60)
    tflite_model = convert_to_tflite(model_path, tflite_path, quantize=False)
    
    # Test TFLite model
    test_tflite_model(tflite_path)
    
    # Generate C header
    generate_c_header(tflite_path)
    
    # Optional: Convert with quantization
    print("\n" + "="*60)
    print("OPTION 2: Int8 Quantized Model (Smaller, Faster)")
    print("="*60)
    tflite_quant_path = '../models/near_miss_model_quant.tflite'
    tflite_model_quant = convert_to_tflite(model_path, tflite_quant_path, quantize=True)
    
    # Test quantized model
    test_tflite_model(tflite_quant_path)
    
    # Generate C header for quantized model
    generate_c_header(tflite_quant_path, '../components/ml_inference/model_data_quant.h')
    
    print(f"\n🎉 Conversion complete!")
    print(f"\nNext steps:")
    print(f"1. Copy model_data.h to your ESP32 project")
    print(f"2. Implement ML inference component")
    print(f"3. Flash firmware and test!")
