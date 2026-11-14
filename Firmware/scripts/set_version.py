# Inject the release version into APP_VERSION at compile time.
# Local builds default to DEV; release builds set RELEASE_VERSION in the environment.

Import("env")

import os

version = os.environ.get("RELEASE_VERSION", "DEV").strip() or "DEV"
env.Append(CPPDEFINES=[("APP_VERSION", '\\"' + version + '\\"')])

print("Building CombatLR firmware version: " + version)
