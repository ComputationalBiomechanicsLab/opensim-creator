import sys
from pathlib import Path

# Add project root to PYTHONPATH - makes tests runnable via IDE integrations
# (e.g. CLion's pytest detector) without having to screw around with configuration
# PYTHONPATH environment variables etc.
sys.path.insert(0, str(Path(__file__).resolve().parent))
