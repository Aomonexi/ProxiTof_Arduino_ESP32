# Python Virtual Environment Setup - ProxiTof Project

## Python Version Recommendation
**Recommended: Python 3.13 for best compatibility**
- ✅ TensorFlow 2.20.0 supports Python 3.9-3.13
- ✅ Python 3.13 has the latest performance improvements
- ✅ All packages in requirements are compatible with Python 3.13
- ✅ Future-proof choice for your thesis project

## Quick Setup Commands (Python 3.13 Recommended)

### Option 1: Use Python 3.13 (Recommended)
```bash
# After installing Python 3.13
# Create virtual environment using Python 3.13
py -3.13 -m venv venv

# Activate virtual environment
# Windows:
venv\Scripts\activate
# Linux/Mac:
source venv/bin/activate

# Verify Python version
python --version  # Should show 3.13.x

# Install all project dependencies (latest versions)
pip install -r requirements_proxitof_thesis.txt

# Verify installation
python -c "import tensorflow as tf; print(f'TensorFlow {tf.__version__} ready!')"
```

### Option 2: Use Your Current Python 3.14 (Alternative)
```bash
# If you prefer to keep using Python 3.14
python -m venv venv
venv\Scripts\activate
pip install -r requirements_proxitof_thesis.txt
```

### Daily Usage
```bash
# Activate environment (run this every time you work on the project)
venv\Scripts\activate

# Deactivate when done
deactivate
```

### VS Code Integration
1. Open project in VS Code
2. Press `Ctrl+Shift+P`
3. Type "Python: Select Interpreter"
4. Choose `.\venv\Scripts\python.exe`

### Jupyter Setup
```bash
# After activating venv and installing requirements
python -m ipykernel install --user --name=proxitof --display-name="ProxiTof ML"

# Now Jupyter will show "ProxiTof ML" as a kernel option
jupyter lab
```

## Important Notes

- **Always activate `venv` before working** on the project
- The `venv/` folder is already in `.gitignore` (won't be committed)
- If you mess up the environment, just delete `venv/` folder and recreate it
- All teammates should use the same `requirements_proxitof_thesis.txt` file

## Troubleshooting

**If activation fails on Windows PowerShell (SecurityError):**
```powershell
# Option 1: Enable script execution (recommended)
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser

# Then try activation again
venv\Scripts\activate
```

**Alternative: Use Command Prompt instead of PowerShell:**
```cmd
# Open Command Prompt (cmd) instead of PowerShell
# Then activate works without issues
venv\Scripts\activate.bat
```

**Alternative: Use the .bat file in PowerShell:**
```powershell
# This works even with restricted execution policy
venv\Scripts\activate.bat
```

**If packages fail to install:**
```bash
# Update pip first
python -m pip install --upgrade pip
pip install -r requirements_proxitof_thesis.txt
```

**Check if environment is active:**
```bash
# Should show path to your venv folder
where python
```