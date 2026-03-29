"""
Sliding Window 1D-CNN - Offline Comparative Experiment
=======================================================
Trains a 1D-CNN on 10-frame temporal windows.
OFFLINE ONLY - does NOT overwrite single-frame model files.

Outputs (all prefixed sw_ or in models/sw_*):
  models/sw_model.keras
  models/sw_best.keras
  data/processed/sw_X_test.npy
  data/processed/sw_y_test.npy
  data/processed/sw_scaler_mean.npy
  data/processed/sw_scaler_std.npy

Run evaluation separately with: evaluate_sliding_window.py

Usage:
  cd tools
  ..\venv_ml\Scripts\activate
  python train_sliding_window.py
"""

import os
import sys
import warnings
import numpy as np
import pandas as pd
from sklearn.model_selection import train_test_split

warnings.filterwarnings("ignore")
os.environ["TF_CPP_MIN_LOG_LEVEL"] = "2"

# ── paths ──────────────────────────────────────────────────────────────────
ROOT    = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
RAW     = os.path.join(ROOT, "data", "raw")
PROC    = os.path.join(ROOT, "data", "processed")
MODELS  = os.path.join(ROOT, "models")

# ── window config ──────────────────────────────────────────────────────────
WIN     = 10        # frames per window  (~0.167 s at ~60 Hz recorded rate)
STEP    = 1         # stride (1 = max overlap)
RATIO   = 10        # safe:near-miss balance ratio
SPLIT   = 0.2
SEED    = 42
THRESH  = 0.70

# ── feature columns (match actual CSV headers) ─────────────────────────────
TOF_COLS = [f"tof_{r}{c}" for r in range(4) for c in range(4)]   # tof_00..tof_33
IMU_COLS = ["roll","pitch","yaw","accel_x","accel_y","accel_z","gyro_x","gyro_y","gyro_z"]
FEAT     = TOF_COLS + IMU_COLS   # 25 features
LABEL    = "flag_manual"


def load_data():
    print("\n" + "="*60)
    print("STEP 1 — LOAD RAW CSV FILES")
    print("="*60)
    files = sorted(f for f in os.listdir(RAW) if f.lower().endswith(".csv"))
    print(f"Found {len(files)} CSV files in {RAW}")
    frames = []
    for fn in files:
        try:
            df = pd.read_csv(os.path.join(RAW, fn), comment="#", on_bad_lines="skip")
            missing = [c for c in FEAT + [LABEL] if c not in df.columns]
            if missing:
                print(f"  skip {fn} — missing: {missing[:3]}...")
                continue
            df["_src"] = fn
            before = len(df)
            df = df.dropna(subset=FEAT + [LABEL])
            if len(df) < before:
                print(f"           (dropped {before-len(df)} NaN rows)")
            frames.append(df)
            print(f"  ok   {fn}  ({len(df):,} rows, near-miss={df[LABEL].sum()})")
        except Exception as exc:
            print(f"  err  {fn} — {exc}")
    if not frames:
        sys.exit("ERROR: no usable CSV files found")
    data = pd.concat(frames, ignore_index=True)
    print(f"\nLoaded {len(data):,} rows total")
    print(f"Near-miss frames: {data[LABEL].sum():,} ({100*data[LABEL].mean():.2f}%)")
    return data


def make_windows(data):
    print("\n" + "="*60)
    print("STEP 2 — EXTRACT SLIDING WINDOWS")
    print("="*60)
    print(f"Window={WIN} frames (~{WIN/60:.3f}s at ~60Hz recorded rate)  step={STEP}  label=ANY-frame-is-near-miss")
    Xw, yw = [], []
    for src in sorted(data["_src"].unique()):
        sub  = data[data["_src"] == src].reset_index(drop=True)
        feat = sub[FEAT].values.astype(np.float32)
        lbl  = sub[LABEL].values.astype(np.int32)
        n    = len(sub)
        if n < WIN:
            continue
        for s in range(0, n - WIN + 1, STEP):
            Xw.append(feat[s:s+WIN])
            yw.append(int(lbl[s:s+WIN].any()))
    X = np.array(Xw, dtype=np.float32)
    y = np.array(yw, dtype=np.int32)
    print(f"Windows: {len(X):,}  shape={X.shape}")
    print(f"Near-miss windows: {y.sum():,} ({100*y.mean():.2f}%)")
    return X, y


def prepare(X, y):
    print("\n" + "="*60)
    print("STEP 3 — NORMALISE / BALANCE / SPLIT")
    print("="*60)
    N, W, F = X.shape
    flat    = X.reshape(-1, F)
    mean    = flat.mean(axis=0)
    std     = flat.std(axis=0);  std[std == 0] = 1.0
    Xn      = (X - mean) / std
    # Safety check — replace any remaining NaN/Inf with 0
    Xn = np.nan_to_num(Xn, nan=0.0, posinf=0.0, neginf=0.0)

    rng     = np.random.RandomState(SEED)
    pos     = np.where(y == 1)[0]
    neg     = np.where(y == 0)[0]
    n_neg   = min(len(neg), len(pos) * RATIO)
    neg_s   = rng.choice(neg, size=n_neg, replace=False)
    idx     = np.concatenate([pos, neg_s]);  rng.shuffle(idx)
    Xb, yb  = Xn[idx], y[idx]
    print(f"Balanced: {len(Xb):,}  (near-miss={yb.sum():,}  safe={(yb==0).sum():,})")

    Xtr, Xte, ytr, yte = train_test_split(Xb, yb, test_size=SPLIT,
                                           random_state=SEED, stratify=yb)
    print(f"Train: {len(Xtr):,}   Test: {len(Xte):,}")

    os.makedirs(PROC, exist_ok=True)
    np.save(os.path.join(PROC, "sw_X_test.npy"),       Xte)
    np.save(os.path.join(PROC, "sw_y_test.npy"),       yte)
    np.save(os.path.join(PROC, "sw_scaler_mean.npy"),  mean)
    np.save(os.path.join(PROC, "sw_scaler_std.npy"),   std)
    print("Saved sw_X_test / sw_y_test / sw_scaler_* to data/processed/")
    return Xtr, Xte, ytr, yte


def train(Xtr, ytr, Xte, yte):
    import tensorflow as tf
    from tensorflow import keras

    print("\n" + "="*60)
    print("STEP 4 — TRAIN 1D-CNN")
    print("="*60)

    _, W, F = Xtr.shape
    model = keras.Sequential([
        keras.layers.Input(shape=(W, F), name="input"),
        keras.layers.Conv1D(32, kernel_size=3, activation="relu",
                            padding="same", name="conv1"),
        keras.layers.Dropout(0.2, name="drop1"),
        keras.layers.Conv1D(16, kernel_size=3, activation="relu",
                            padding="same", name="conv2"),
        keras.layers.GlobalAveragePooling1D(name="gap"),
        keras.layers.Dense(16, activation="relu", name="dense"),
        keras.layers.Dense(2,  activation="softmax", name="output"),
    ], name="sw_cnn")

    model.compile(
        optimizer=keras.optimizers.Adam(learning_rate=0.0005, clipnorm=1.0),
        loss="sparse_categorical_crossentropy",
        metrics=["accuracy"],
    )
    model.summary()

    n  = len(ytr)
    n0 = (ytr == 0).sum();  n1 = (ytr == 1).sum()
    print(f"\nClass distribution — safe: {n0}  near-miss: {n1}")
    # No class weights — dataset is already balanced via undersampling (10:1)
    # Class weights cause NaN loss with Conv1D; balanced sampling is sufficient

    os.makedirs(MODELS, exist_ok=True)
    best_path = os.path.join(MODELS, "sw_best.keras")

    cbs = [
        keras.callbacks.EarlyStopping(
            monitor="val_loss", patience=15,
            restore_best_weights=True, verbose=1),
        keras.callbacks.ModelCheckpoint(
            best_path, monitor="val_accuracy",
            save_best_only=True, verbose=1),
        keras.callbacks.ReduceLROnPlateau(
            monitor="val_loss", factor=0.5,
            patience=5, min_lr=1e-5, verbose=1),
    ]

    print(f"\nTraining on {len(Xtr):,} windows …")
    model.fit(
        Xtr, ytr,
        validation_data=(Xte, yte),
        epochs=100, batch_size=32,
        callbacks=cbs,
        verbose=1,
    )

    save_path = os.path.join(MODELS, "sw_model.keras")
    model.save(save_path)
    print(f"\nModel saved → {save_path}")
    return model


def main():
    print("\n" + "="*60)
    print("SLIDING WINDOW TRAINING  (offline experiment)")
    print("="*60)
    data            = load_data()
    X, y            = make_windows(data)
    Xtr, Xte, ytr, yte = prepare(X, y)
    model           = train(Xtr, ytr, Xte, yte)

    print("\n" + "="*60)
    print("TRAINING COMPLETE")
    print("="*60)
    print("Next step: run evaluate_sliding_window.py")


if __name__ == "__main__":
    main()
