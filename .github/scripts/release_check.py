"""Prevent a build-only artifact from being published as a functional release."""
import json
from pathlib import Path

status = json.loads((Path(__file__).resolve().parents[1] / "release.json").read_text(encoding="utf-8"))
if not status["ready"]:
    raise SystemExit("Release blocked: " + status["reason"])
print("No known source-level release blocker; review CI and gameplay validation before publishing.")
