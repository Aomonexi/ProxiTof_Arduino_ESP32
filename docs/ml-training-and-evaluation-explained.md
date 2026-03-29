# ML Training and Evaluation — How It Works

This document explains what `train_model.py` and `evaluate_results.py` do,
what TensorFlow is doing under the hood, and how to interpret the results.
Written for someone who understands the hardware but is new to machine learning.

---

## Before You Start: Two Fundamental Questions

### Does the network look at one data point or a window of data points?

**One data point.** Each row in your CSV is one snapshot in time — 25 numbers
recorded at one moment (1/60th of a second). The network looks at that single
snapshot and makes a decision. It does not look at what happened before or after.

This is called a **feedforward network**. It is the simplest and smallest type,
which is why it fits on the ESP32 in 3.5 KB.

(A network that looks at a sequence of frames over time would be called an LSTM
or a CNN. Those are more powerful but much larger and slower — not suitable for
a microcontroller running at 60 Hz.)

### Does it compare each data point against known near-misses?

Yes, exactly. During training, for every data point it:
1. Makes a prediction
2. Compares that prediction to the real label (0 = safe, 1 = near-miss)
3. Calculates how wrong it was
4. Adjusts its internal numbers to be less wrong next time

The key insight is that it does not store examples and compare against them
at runtime. Instead, the lessons from all those comparisons get permanently
encoded into the 450 weight values. At deployment time, no examples are needed —
just the 450 numbers.

---

## The Maths — One Complete Pass (Step by Step)

This is what actually happens inside TensorFlow for a single data point.
Simplified to 3 inputs for clarity, but your system uses 25.

### Setup

Say the network sees this snapshot:

```
tof_00 = 0.3   (normalised — was about 200mm, something is close)
gyro_z = 1.2   (normalised — sudden rotation detected)
roll   = -0.1  (normalised — slight lean)
```

Real label: **1** (near-miss — you pressed the button at this moment)

The weights start as random numbers. Let's say:

```
Neuron 1 weights: w1=0.5,  w2=-0.2, w3=0.8,  bias=0.1
Neuron 2 weights: w1=0.9,  w2=0.7,  w3=-0.3, bias=0.2
```

---

### Step 1: Forward Pass — Calculate Each Neuron's Output

**Neuron 1:**
```
z = (tof_00 × w1) + (gyro_z × w2) + (roll × w3) + bias
z = (0.3 × 0.5) + (1.2 × -0.2) + (-0.1 × 0.8) + 0.1
z = 0.15 - 0.24 - 0.08 + 0.1
z = -0.07

Apply ReLU:  if z > 0, keep it.  if z ≤ 0, set to 0.
ReLU(-0.07) = 0   ← this neuron stays silent
```

**Neuron 2:**
```
z = (0.3 × 0.9) + (1.2 × 0.7) + (-0.1 × -0.3) + 0.2
z = 0.27 + 0.84 + 0.03 + 0.2
z = 1.34

ReLU(1.34) = 1.34  ← this neuron fires
```

In your real network, all 16 neurons do this simultaneously. Some fire, some
stay silent. The ones that fire are the ones whose particular combination of
inputs matched what they learned to look for.

---

### Step 2: Output Layer — Combine the 16 Neuron Outputs

The output layer takes the 16 neuron outputs and produces two numbers using
a function called **softmax**, which forces them to add up to 1.0:

```
P(safe)      = 0.72
P(near-miss) = 0.28   ← wrong! the real label is 1 (near-miss)
```

The network is 72% confident this is safe, but it is actually a near-miss.
This is a bad prediction.

---

### Step 3: Calculate the Loss (How Wrong Was It?)

The loss function measures how wrong the prediction was. Your network uses
**cross-entropy loss**:

```
Loss = -log( P(correct class) )
     = -log( P(near-miss) )
     = -log( 0.28 )
     = 1.27
```

The log function makes this very punishing for confident wrong answers:

| Predicted P(near-miss) | Loss |
|------------------------|------|
| 0.99 (very confident, correct) | 0.01 |
| 0.70 (fairly confident, correct) | 0.36 |
| 0.50 (uncertain) | 0.69 |
| 0.28 (wrong, like above) | 1.27 |
| 0.01 (very confident, wrong) | 4.61 |

High loss = bad. The goal of training is to minimise this number.

---

### Step 4: Backpropagation — Who Is to Blame?

Now the network works backwards through every weight asking:
"if I had changed this weight slightly, how much would the loss have changed?"

This is a derivative from calculus, written as:

```
gradient of w = ∂Loss / ∂w
```

You do not need to calculate this by hand — TensorFlow does it automatically
using the chain rule of calculus. This is the main thing TensorFlow provides:
automatic differentiation at high speed.

The gradient tells you the direction. If the gradient is positive, increasing
that weight makes the loss worse — so you should decrease it. If negative,
increase it.

---

### Step 5: Weight Update — Gradient Descent

Each weight is nudged slightly in the direction that reduces the loss:

```
new_weight = old_weight - (learning_rate × gradient)
```

With learning rate = 0.001 and a gradient of 3.4 for weight w2:

```
w2 = -0.2 - (0.001 × 3.4)
w2 = -0.2034
```

A tiny change. But this happens for all 450 weights, for every batch of 32
samples, for every epoch. Over time, the weights drift toward values that
produce low loss on most examples.

---

### Step 6: Repeat Until Convergence

This entire process — forward pass, loss calculation, backpropagation,
weight update — repeats:

```
32 samples per batch
58,784 samples per epoch  →  1,837 batches per epoch
44 epochs (early stopping)
─────────────────────────────────────────────────────
Total weight updates: 1,837 × 44 = 80,828 updates
Total samples seen:   58,784 × 44 = 2,586,496
```

After 2.6 million individual corrections, the 450 weights have settled into
values that correctly classify most of the training examples. The pattern is
now encoded in those numbers — not as stored examples, but as a mathematical
function that approximates the boundary between "safe" and "near-miss."

---

### Why Does This Work? The Intuition

Each weight controls how much one input influences one neuron. After training:

- A weight near zero means "this input is irrelevant to this neuron"
- A large positive weight means "when this input is high, this neuron fires more"
- A large negative weight means "when this input is high, this neuron fires less"

The 16 neurons in the hidden layer each end up specialising in a different
combination of inputs. One might learn to respond to "multiple close ToF zones
simultaneously." Another might respond to "sudden gyroscope spike with close
distance." The output layer then learns to combine these 16 specialised
detectors into a final probability.

No human programmed these specialisations. They emerged automatically from
the process of minimising prediction error across thousands of examples.

**TensorFlow's role** is to make all this maths — especially the backpropagation
derivatives — run fast enough to be practical. Without it, training your 450-weight
network on 58,784 samples would take hours instead of minutes.

---

## Gradient Descent — The Mathematics of Learning

### The Hill Analogy

Imagine you are blindfolded on a hilly landscape. Your goal is to find the
lowest valley. You cannot see anything, but you can feel the slope under
your feet.

- If the ground slopes down to your left, take a step left
- If it slopes down to your right, take a step right
- Take small steps so you do not overshoot the valley
- Keep going until the ground feels flat — you are at the bottom

That is gradient descent. The "landscape" is the loss function — a surface
where every point represents a different combination of 450 weight values,
and the height at that point is how wrong the network is with those weights.
The goal is to find the lowest point (minimum loss).

The "slope under your feet" is the **gradient** — the derivative of the loss
with respect to each weight. It tells you which direction is downhill for
every weight simultaneously.

```
gradient of w = ∂Loss / ∂w
```

If this value is positive, the loss increases when w increases — so decrease w.
If negative, the loss decreases when w increases — so increase w.

The weight update rule is:

```
w_new = w_old - α × (∂Loss / ∂w)
```

where α (alpha) is the **learning rate** — the size of each step.
Too large and you overshoot the valley. Too small and training takes forever.
Your network uses α = 0.001, which halves automatically when progress stalls.

---

### Why the Formula Treats Everything as Matrices

Doing this calculation one sample at a time, one neuron at a time, would be
impossibly slow. Matrices allow the entire batch of 32 samples and all 16
neurons to be computed in a single operation.

**One sample, written out longhand (scalar form):**

For neuron j receiving input from 25 features:
```
z_j = w_j1×x_1 + w_j2×x_2 + w_j3×x_3 + ... + w_j25×x_25 + b_j
```

**One sample, vector form:**

Stack the 25 weights into a row vector W_j and the 25 inputs into a column
vector x:
```
z_j = W_j · x + b_j
```

The dot product (·) multiplies each weight by its corresponding input and
sums them all — exactly the same calculation, just written compactly.

**All 16 neurons at once, matrix form:**

Stack all 16 weight vectors into a matrix W of shape 16×25:

```
     ┌ w_11  w_12  ...  w_1,25 ┐         ┌ x_1  ┐     ┌ b_1  ┐
     │ w_21  w_22  ...  w_2,25 │         │ x_2  │     │ b_2  │
W =  │  ...                    │    x =  │ ...  │  b= │ ...  │
     └ w_16,1 ...   w_16,25   ┘         └ x_25 ┘     └ b_16 ┘

z = W · x + b        (shape: 16×1)
```

One matrix-vector multiply produces all 16 neuron outputs simultaneously.

**All 32 samples in a batch at once:**

Stack 32 input vectors into a matrix X of shape 32×25:

```
     ┌ x_1,1   x_1,2   ...  x_1,25  ┐
     │ x_2,1   x_2,2   ...  x_2,25  │
X =  │  ...                          │     (shape: 32 × 25)
     └ x_32,1  x_32,2  ...  x_32,25 ┘

Z = X · Wᵀ + b       (shape: 32 × 16)
```

One matrix multiplication replaces:
```
32 samples × 16 neurons × 25 inputs = 12,800 individual multiplications
```

All done in a single GPU or SIMD operation. This is why TensorFlow is fast —
it maps every operation onto highly optimised matrix multiplication routines.

**The full forward pass in matrix form:**

```
Layer 1 (hidden):   Z1 = X · W1ᵀ + b1        (32×25 · 25×16 = 32×16)
                    A1 = ReLU(Z1)              (32×16, element-wise)

Layer 2 (output):   Z2 = A1 · W2ᵀ + b2       (32×16 · 16×2  = 32×2)
                    A2 = softmax(Z2)           (32×2, row-wise)
```

The final output A2 is a 32×2 matrix — 32 rows (one per sample), 2 columns
(probability of safe, probability of near-miss).

**Backpropagation in matrix form:**

The gradients flow backwards through the same matrices using the chain rule:

```
∂Loss/∂W2 = A1ᵀ · δ2                         (16×32 · 32×2 = 16×2)
∂Loss/∂W1 = Xᵀ  · δ1                         (25×32 · 32×16 = 25×16)
```

where δ (delta) is the error signal at each layer, computed by multiplying
the upstream gradient by the derivative of the activation function.

TensorFlow computes all of this automatically using **automatic differentiation**
— you never write the gradient formulas yourself. You just define the forward
pass, and TensorFlow builds the backward pass for you using the chain rule.

---

### Why This Is Efficient on the ESP32

The ESP32-S3 has a hardware unit called **ESP-NN** that accelerates exactly
these matrix operations using SIMD (Single Instruction, Multiple Data)
instructions — one CPU instruction that operates on 8 or 16 numbers
simultaneously instead of one at a time.

Your network's two matrix multiplications (12×25 and 16×2) take roughly
5–15 ms on the ESP32-S3 with ESP-NN, well within the 16.7 ms frame budget
at 60 Hz.

---



### The neuron as a person with an opinion

Think of each neuron as one person with one opinion. They start with a
completely random, uninformed opinion — because the weights start random,
the network literally knows nothing at the beginning.

Every time they are wrong, they adjust their opinion slightly. After seeing
thousands of examples, their opinion has been shaped by experience into
something useful.

What makes it powerful is that **each person specialises without being told to.**

Imagine 16 people in a room, all watching the same cyclist footage. Nobody
tells them what to look for. But because they each started with different
random opinions, they drift toward different specialisations:

- Person 3 ends up becoming the expert on "things that are very close on the left side"
- Person 7 becomes the expert on "sudden swerving movements"
- Person 11 becomes the expert on "close distance AND fast approach combined"
- Person 15 ends up not caring about anything useful and mostly stays quiet

Nobody assigned those roles. The correction process pushed each person toward
whatever they happened to be slightly better at noticing from their random
starting point.

Then there is a **17th person** (the output layer) whose only job is to listen
to all 16 experts and make the final call. They learn which experts to trust
and how much.

---

### The hot and cold game

You know the game where someone hides an object and says "warmer" when you
walk toward it and "colder" when you walk away?

- The weights start at random positions (you are standing somewhere random in the room)
- Each training step, the network gets told "warmer" or "colder" (the gradient)
- It takes a tiny step in the warmer direction
- After millions of tiny steps, it ends up at the hidden object (the correct weights)

The object it is trying to find is "the set of 450 numbers that correctly
separates near-miss sensor patterns from safe ones." It starts with no idea
where that is. The feedback from millions of labelled examples guides it there.

---

### The 450 dials analogy — why it does not try random formulas

A common misconception is that the network tries different formulas until
one works. It does not. **The formula never changes. Only the numbers inside
it change.**

The formula is always:
```
output = ReLU( inputs × weights + bias )
```

What changes are the 450 weight values inside that formula. Training is just
the process of finding the right values for those 450 numbers.

Think of a mixing board with 450 volume dials, each going from -10 to +10.
The formula is always "multiply the signal by the dial number and add them up."
You do not change the formula — you just turn the dials until the output
sounds right.

This is not random searching. Gradient descent follows the mathematical slope
downhill toward the answer, like water finding the lowest point in a landscape.
It is systematic and guided — which is why it converges in 44 epochs instead
of taking forever.

---

### Why random starting weights and not zero?

If all weights started at zero, every neuron would compute the exact same
thing and they would all learn the same pattern — which defeats the purpose
of having 16 neurons. Random starting points mean each neuron starts in a
different place and ends up specialising in something different.

---

### The Universal Approximation Theorem — why this works at all

The reason neural networks can learn any pattern is captured in a mathematical
theorem called the **Universal Approximation Theorem**:

> A neural network with even just one hidden layer can approximate any
> mathematical function, given enough neurons.

Any pattern that exists in data is ultimately just a mathematical function
mapping inputs to outputs. A neural network, through random start and repeated
correction, can find that function.

One neuron doing `inputs × weights` can only draw a straight line between
safe and dangerous. But 16 neurons, each with different weights, combined
together, can draw curved, complex, irregular boundaries — like drawing any
shape using enough short straight lines arranged together.

The "opinion" analogy is mathematically real. The weights literally are
opinions — numerical beliefs about how much each input matters. And gradient
descent is literally the mathematical version of "I was wrong, let me reconsider."

---



You have a sensor that produces 25 numbers every 1/60th of a second:
- 16 ToF distances (the 4×4 grid)
- 9 IMU values (roll, pitch, yaw, 3× acceleration, 3× gyroscope)

The goal is to teach a computer program to look at those 25 numbers and answer
one question: **"Is this a near-miss situation right now?"**

That is exactly what the neural network does. It takes 25 numbers in, and
outputs a probability between 0 and 1. If the probability is above 0.70,
the device triggers an alert.

---

## Part 1: Training the Model (`train_model.py`)

### What is TensorFlow?

TensorFlow is a Python library made by Google for building and training neural
networks. It handles all the mathematics automatically — you just describe the
shape of the network and it figures out how to adjust the weights to make
better predictions.

### What is a Neural Network?

A neural network is a chain of mathematical operations that transforms inputs
into outputs. Think of it like a series of filters, each one looking for
different patterns in the data.

Your network has this structure:

```
Input layer:   25 numbers  (16 ToF + 9 IMU)
       ↓
Hidden layer:  16 neurons  (each one looks for a different pattern)
       ↓
Dropout:       randomly turns off 20% of neurons during training
               (prevents the model from memorising instead of learning)
       ↓
Output layer:  2 numbers   (probability of "safe", probability of "near-miss")
```

Total parameters (adjustable numbers): **450**
This is tiny — a typical phone app model has millions. Tiny is good here
because it has to run on the ESP32 in under 17ms.

### What Does "Training" Actually Mean?

Training is the process of adjusting those 450 numbers (called weights) so
that the network gives correct answers.

The process repeats for up to 100 rounds (called epochs):

1. **Forward pass** — feed a batch of 32 samples through the network,
   get 32 predictions
2. **Calculate loss** — compare predictions to the real labels using a
   formula called cross-entropy loss. Higher loss = worse predictions.
3. **Backward pass (backpropagation)** — calculate how much each of the
   450 weights contributed to the error
4. **Update weights** — nudge each weight slightly in the direction that
   reduces the error (this is called gradient descent, using the Adam
   optimiser)
5. **Repeat** with the next batch

After each epoch, the model is tested on the validation set (data it has
never seen during training). If validation accuracy stops improving for
15 epochs in a row, training stops early (early stopping).

### Handling Imbalanced Data

Your dataset has 77 safe samples for every 1 near-miss sample. If you trained
on this directly, the model would learn to always say "safe" and be 99% accurate
while being completely useless.

Two things are done to fix this:

**Undersampling** — the safe class is reduced to 10× the near-miss count
(from 515,144 down to 66,800). This gives the model a more balanced view.

**Class weights** — during training, near-miss errors are penalised 5.5×
more than safe errors. This forces the model to pay more attention to the
rare class.

### What Gets Saved After Training?

| File | What it is |
|------|-----------|
| `models/best_model.h5` | The Keras model at its best validation accuracy |
| `models/near_miss_model.h5` | Final model after all epochs |
| `models/near_miss_model.tflite` | Compressed version for ESP32 (3.5 KB) |
| `models/training_history.png` | Graph of accuracy and loss over epochs |
| `data/processed/scaler.pkl` | The normalisation parameters |
| `components/ml_inference/include/model_data.h` | C array for firmware |

### What is the Scaler?

Raw sensor values have very different ranges:
- ToF distances: 0 to 3800 mm
- Gyroscope: -5 to +5 rad/s
- Roll: -180 to +180 degrees

If you feed these directly into the network, the large ToF numbers would
dominate and the small IMU values would be ignored.

The scaler (StandardScaler from scikit-learn) transforms every feature to
have mean = 0 and standard deviation = 1. This is done by:

```
normalised_value = (raw_value - mean) / std_deviation
```

The mean and std for each of the 25 features are calculated from the training
data and saved. The same values are used at inference time on the ESP32
(they are embedded in `model_data.h`).

### What is TFLite Conversion?

The trained Keras model is in a format designed for training on a PC.
TensorFlow Lite (TFLite) is a stripped-down version designed to run on
microcontrollers. The conversion:
- Removes training-only operations
- Optimises the computation graph
- Reduces the file from ~100 KB to 3.5 KB

On the ESP32-S3, the ESP-NN library accelerates the Dense and ReLU operations
using SIMD instructions, making inference take roughly 5–15 ms.

---

## Part 2: Evaluating the Model (`evaluate_results.py`)

### What Does Evaluation Do?

After training, you need to know how well the model actually works.
The evaluation script tests three detection approaches on the same held-out
test set (20% of the data, never seen during training):

1. **Algorithm** — the rule-based detector already in `data_processor.cpp`
   (checks distance thresholds and approach rate)
2. **ML** — the neural network at threshold 0.70
3. **Hybrid** — fires if either Algorithm OR ML detects a near-miss

### The Confusion Matrix

For each approach, every test sample gets classified as one of four outcomes:

```
                    Predicted: Safe    Predicted: Near-Miss
Actual: Safe            TN                   FP
Actual: Near-Miss       FN                   TP
```

- **TP (True Positive)** — real near-miss, correctly detected. Good.
- **TN (True Negative)** — safe situation, correctly ignored. Good.
- **FP (False Positive)** — safe situation, wrongly triggered alert. Bad (annoying).
- **FN (False Negative)** — real near-miss, missed entirely. Bad (dangerous).

From your results:

| | Algorithm | ML | Hybrid |
|--|-----------|-----|--------|
| TP | 106 | 987 | 991 |
| TN | 13,254 | 12,709 | 12,505 |
| FP | 106 | 651 | 855 |
| FN | 1,230 | 349 | 345 |

### The Four Key Metrics

**Recall (also called Sensitivity or Detection Rate)**
```
Recall = TP / (TP + FN)
```
Out of all real near-misses, what fraction did we catch?
- Algorithm: 106 / (106 + 1230) = **8%** — misses almost everything
- ML: 987 / (987 + 349) = **74%** — catches 3 out of 4 near-misses
- Hybrid: 991 / (991 + 345) = **74%** — similar to ML

For a safety device, recall is the most important metric. A missed near-miss
is more dangerous than a false alarm.

**Precision**
```
Precision = TP / (TP + FP)
```
When the device fires an alert, how often is it a real near-miss?
- Algorithm: 106 / (106 + 106) = **32%** — 2 out of 3 alerts are false
- ML: 987 / (987 + 651) = **62%** — about 1 in 3 alerts is false
- Hybrid: 991 / (991 + 855) = **55%** — slightly worse than ML alone

**F1-Score**
```
F1 = 2 × (Precision × Recall) / (Precision + Recall)
```
The harmonic mean of precision and recall. A single number that balances both.
- Algorithm: 0.13 — very poor
- ML: 0.67 — good
- Hybrid: 0.63 — slightly worse than ML alone

**Accuracy**
```
Accuracy = (TP + TN) / Total
```
Overall fraction of correct predictions. This metric is misleading with
imbalanced data — the algorithm gets 90% accuracy while barely detecting
anything, because it correctly classifies all the safe samples.

### ROC AUC

The ROC AUC (Area Under the Receiver Operating Characteristic Curve) measures
how well the model separates the two classes across all possible thresholds.

- 0.5 = random guessing
- 1.0 = perfect separation
- Your ML model: **0.95** — excellent

This means the model's probability scores are very well calibrated. The 0.70
threshold is just one operating point on this curve.

### McNemar's Test

This is a statistical test that checks whether two classifiers make
significantly different errors on the same test set.

The null hypothesis is: "both classifiers make the same mistakes."
If p < 0.05/3 (Bonferroni corrected for 3 comparisons), the difference
is statistically significant.

Your results: all three comparisons have p < 0.000001, meaning the
differences between Algorithm, ML, and Hybrid are not due to chance.

### Bootstrap Confidence Intervals

To know how reliable the F1-score estimate is, the test set is resampled
1000 times with replacement (bootstrap). The 2.5th and 97.5th percentiles
give the 95% confidence interval.

- ML F1: 0.674 [0.654, 0.693]

This means you can be 95% confident the true F1-score is between 0.654
and 0.693. The interval is narrow, meaning the estimate is stable.

---

## Part 3: How to Improve Precision and Recall

### What Causes Low Precision?

Low precision means too many false alarms. The model fires when it should not.
This happens when:
- The model has not seen enough examples of "safe but looks dangerous"
  situations (riding close to a wall, parked cars, narrow paths)
- The detection threshold is too low (0.70 is already a good starting point)

### What Causes Low Recall?

Low recall means missed near-misses. The model fails to fire when it should.
This happens when:
- The near-miss examples in training are not diverse enough
- The model has only seen fast, obvious near-misses and misses slow ones

### Data Collection Recommendations

**To improve recall — collect more varied near-miss situations:**

| Scenario | Why it helps |
|----------|-------------|
| Slow close pass (50–80 cm, low speed) | Most common real near-miss, hardest to detect |
| Side approach (vehicle from the left or right) | Different ToF zone pattern |
| Fast pass at 1–2 m distance | High approach rate but not very close |
| Near-miss while turning or leaning | IMU pattern is different |
| Near-miss on narrow road | Sustained close distance, not just a brief pass |

**To improve precision — collect more "safe but close" situations:**

| Scenario | Why it helps |
|----------|-------------|
| Riding close to a parked car | Looks like a near-miss to the sensor |
| Narrow path between walls | Sustained close distance on both sides |
| Overtaking another cyclist | Close but not dangerous |
| Riding through a gate or doorway | Brief very close reading |
| Stopping next to a vehicle at traffic lights | Close but stationary |

**General rules:**
- Press the button (flag_manual) only during genuine near-miss moments
- Aim for at least 200–300 near-miss events across diverse scenarios
- Collect data in different environments (road, cycle path, urban, rural)
- Vary your speed — slow cycling produces different sensor patterns

### After Collecting More Data

Re-run the full pipeline:
```
.\venv_ml\Scripts\python.exe tools\quick_start_ml.py
.\venv_ml\Scripts\python.exe tools\evaluate_results.py
```

The model will retrain from scratch on the new combined dataset.
You should see both precision and recall improve as the training data
becomes more representative of real-world conditions.

---

## Summary

| Step | Tool | What it does |
|------|------|-------------|
| Collect data | Device (SD card) | Records 25 sensor values at 60 Hz, button marks near-miss |
| Load & merge | `load_data.py` | Combines all CSV files, checks quality |
| Prepare | `prepare_training_data.py` | Balances classes, normalises, splits 80/20 |
| Train | `train_model.py` | Adjusts 450 weights using TensorFlow/Keras |
| Convert | `convert_to_tflite.py` | Shrinks model to 3.5 KB for ESP32 |
| Deploy | `model_data.h` | C array embedded in firmware |
| Evaluate | `evaluate_results.py` | Tests Algorithm vs ML vs Hybrid, generates graphs |

The key insight is that the ML model (F1 = 0.67, AUC = 0.95) significantly
outperforms the rule-based algorithm (F1 = 0.13) and the hybrid combination
does not add meaningful benefit over ML alone. More diverse training data,
particularly slow close passes and safe-but-close scenarios, will push
precision and recall higher.
