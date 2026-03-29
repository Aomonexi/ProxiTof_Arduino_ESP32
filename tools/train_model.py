"""
Train neural network for near-miss detection using TensorFlow
Compatible with ESP-NN acceleration on ESP32-S3
"""
import numpy as np
import tensorflow as tf
from tensorflow import keras
from sklearn.metrics import classification_report, confusion_matrix, roc_auc_score
import matplotlib.pyplot as plt
import os

def create_model(input_size=25, hidden_size=32):
    """
    Create neural network optimized for ESP-NN

    Architecture:
    - Input: 25 features (16 ToF + 9 IMU)
    - Hidden 1: 32 neurons (ReLU) - ESP-NN accelerated
    - Hidden 2: 16 neurons (ReLU) - ESP-NN accelerated
    - Output: 2 classes (safe, near-miss) with softmax

    Selected architecture from architecture search (see docs/ml-architecture-search-results.md)
    Best F1 (0.669), best precision (67.8%), 7.9 KB model size, fits ESP32-S3 comfortably.
    """

    model = keras.Sequential([
        keras.layers.Input(shape=(input_size,), name='input'),
        keras.layers.Dense(hidden_size, activation='relu', name='hidden_1'),
        keras.layers.Dropout(0.2, name='dropout_1'),
        keras.layers.Dense(16, activation='relu', name='hidden_2'),
        keras.layers.Dropout(0.1, name='dropout_2'),
        keras.layers.Dense(2, activation='softmax', name='output')
    ], name='near_miss_detector')
    
    # Compile with Adam optimizer
    model.compile(
        optimizer=keras.optimizers.Adam(learning_rate=0.001),
        loss='sparse_categorical_crossentropy',
        metrics=['accuracy']
    )
    
    return model

def train_model(X_train, y_train, X_test, y_test, 
                epochs=100, batch_size=32, output_dir='../models'):
    """Train the model with early stopping"""
    
    print(f"\n{'='*60}")
    print("MODEL TRAINING")
    print(f"{'='*60}")
    
    # Create model
    model = create_model(input_size=X_train.shape[1])
    
    print("\nModel Architecture:")
    model.summary()
    
    # Calculate class weights (handle imbalanced data)
    n_samples = len(y_train)
    n_class_0 = np.sum(y_train == 0)
    n_class_1 = np.sum(y_train == 1)
    
    class_weight = {
        0: n_samples / (2 * n_class_0),
        1: n_samples / (2 * n_class_1)
    }
    
    print(f"\nClass weights:")
    print(f"  Safe (0):      {class_weight[0]:.3f}")
    print(f"  Near-miss (1): {class_weight[1]:.3f}")
    
    # Callbacks
    os.makedirs(output_dir, exist_ok=True)
    
    callbacks = [
        keras.callbacks.EarlyStopping(
            monitor='val_loss',
            patience=15,
            restore_best_weights=True,
            verbose=1
        ),
        keras.callbacks.ModelCheckpoint(
            os.path.join(output_dir, 'best_model.keras'),
            monitor='val_accuracy',
            save_best_only=True,
            verbose=1
        ),
        keras.callbacks.ReduceLROnPlateau(
            monitor='val_loss',
            factor=0.5,
            patience=5,
            min_lr=0.00001,
            verbose=1
        )
    ]
    
    # Train
    print(f"\nTraining for up to {epochs} epochs...")
    print(f"Batch size: {batch_size}")
    print(f"Training samples: {len(X_train)}")
    print(f"Validation samples: {len(X_test)}")
    
    history = model.fit(
        X_train, y_train,
        validation_data=(X_test, y_test),
        epochs=epochs,
        batch_size=batch_size,
        class_weight=class_weight,
        callbacks=callbacks,
        verbose=1
    )
    
    return model, history

def evaluate_model(model, X_test, y_test):
    """Evaluate model performance"""
    
    print(f"\n{'='*60}")
    print("MODEL EVALUATION")
    print(f"{'='*60}")
    
    # Predictions
    y_pred_proba = model.predict(X_test, verbose=0)
    y_pred = np.argmax(y_pred_proba, axis=1)
    
    # Classification report
    print("\nClassification Report:")
    print(classification_report(y_test, y_pred, 
                                target_names=['Safe', 'Near-Miss'],
                                digits=4))
    
    # Confusion matrix
    cm = confusion_matrix(y_test, y_pred)
    print("\nConfusion Matrix:")
    print("                Predicted")
    print("                Safe  Near-Miss")
    print(f"Actual Safe     {cm[0,0]:4d}  {cm[0,1]:4d}")
    print(f"       Near-Miss {cm[1,0]:4d}  {cm[1,1]:4d}")
    
    # Calculate metrics
    accuracy = np.mean(y_pred == y_test)
    
    # True Positives, False Positives, etc.
    TP = cm[1, 1]  # Correctly detected near-miss
    FP = cm[0, 1]  # False alarm (safe classified as near-miss)
    TN = cm[0, 0]  # Correctly classified safe
    FN = cm[1, 0]  # Missed near-miss (near-miss classified as safe)
    
    # Detection rate (Recall for near-miss class)
    detection_rate = TP / (TP + FN) if (TP + FN) > 0 else 0
    
    # False alarm rate
    false_alarm_rate = FP / (FP + TN) if (FP + TN) > 0 else 0
    
    # Precision (when model says near-miss, how often is it correct?)
    precision = TP / (TP + FP) if (TP + FP) > 0 else 0
    
    # F1 score (harmonic mean of precision and recall)
    f1_score = 2 * (precision * detection_rate) / (precision + detection_rate) if (precision + detection_rate) > 0 else 0
    
    # ROC AUC
    roc_auc = roc_auc_score(y_test, y_pred_proba[:, 1])
    
    print(f"\n{'='*60}")
    print("PERFORMANCE METRICS")
    print(f"{'='*60}")
    print(f"Accuracy:          {accuracy:.2%}")
    print(f"Detection Rate:    {detection_rate:.2%} (near-miss recall)")
    print(f"False Alarm Rate:  {false_alarm_rate:.2%}")
    print(f"Precision:         {precision:.2%}")
    print(f"F1 Score:          {f1_score:.4f}")
    print(f"ROC AUC:           {roc_auc:.4f}")
    
    print(f"\n{'='*60}")
    print("INTERPRETATION")
    print(f"{'='*60}")
    print(f"✅ Model correctly detects {detection_rate:.0%} of near-miss events")
    print(f"⚠️  Model has {false_alarm_rate:.0%} false alarm rate")
    print(f"✅ When model alerts, it's correct {precision:.0%} of the time")
    
    if detection_rate < 0.80:
        print(f"\n⚠️  WARNING: Detection rate is low (<80%)")
        print("   Consider: More training data, different features, or model tuning")
    
    if false_alarm_rate > 0.10:
        print(f"\n⚠️  WARNING: False alarm rate is high (>10%)")
        print("   Consider: Adjusting threshold or collecting more diverse safe examples")
    
    return {
        'accuracy': accuracy,
        'detection_rate': detection_rate,
        'false_alarm_rate': false_alarm_rate,
        'precision': precision,
        'f1_score': f1_score,
        'roc_auc': roc_auc
    }

def plot_training_history(history, output_dir='../models'):
    """Plot training history"""
    
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 4))
    
    # Accuracy
    ax1.plot(history.history['accuracy'], label='Train')
    ax1.plot(history.history['val_accuracy'], label='Validation')
    ax1.set_xlabel('Epoch')
    ax1.set_ylabel('Accuracy')
    ax1.set_title('Model Accuracy')
    ax1.legend()
    ax1.grid(True)
    
    # Loss
    ax2.plot(history.history['loss'], label='Train')
    ax2.plot(history.history['val_loss'], label='Validation')
    ax2.set_xlabel('Epoch')
    ax2.set_ylabel('Loss')
    ax2.set_title('Model Loss')
    ax2.legend()
    ax2.grid(True)
    
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, 'training_history.png'), dpi=150)
    print(f"\n✅ Training history plot saved to {output_dir}/training_history.png")

if __name__ == '__main__':
    # Load prepared data
    data_dir = '../data/processed'
    
    print("Loading training data...")
    X_train = np.load(os.path.join(data_dir, 'X_train.npy'))
    X_test = np.load(os.path.join(data_dir, 'X_test.npy'))
    y_train = np.load(os.path.join(data_dir, 'y_train.npy'))
    y_test = np.load(os.path.join(data_dir, 'y_test.npy'))
    
    print(f"✅ Loaded training data")
    print(f"   X_train: {X_train.shape}")
    print(f"   X_test:  {X_test.shape}")
    
    # Train model
    model, history = train_model(X_train, y_train, X_test, y_test, 
                                 epochs=100, batch_size=32)
    
    # Evaluate
    metrics = evaluate_model(model, X_test, y_test)
    
    # Plot history
    plot_training_history(history)
    
    # Save final model
    output_dir = '../models'
    model.save(os.path.join(output_dir, 'near_miss_model.keras'))
    print(f"\n✅ Model saved to {output_dir}/near_miss_model.keras")
    
    print(f"\n🎉 Training complete!")
    print(f"Next step: Run convert_to_tflite.py to convert for ESP32")
