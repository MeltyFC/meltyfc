"""PlatformIO extra script: embed git hash + build info at compile time."""
import subprocess
Import("env")

def get_git_info():
    try:
        git_hash = subprocess.check_output(
            ["git", "describe", "--always", "--dirty"],
            stderr=subprocess.DEVNULL
        ).decode().strip()
    except Exception:
        git_hash = "unknown"
    return git_hash

git_hash = get_git_info()
env.Append(CPPDEFINES=[
    ("MELTYFC_GIT_HASH", f'\\"{git_hash}\\"'),
    ("MELTYFC_BUILD_DATE", f'\\"{__import__("datetime").date.today()}\\"'),
])
