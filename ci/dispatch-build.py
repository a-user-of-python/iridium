"""Dispatch a build of HEAD and verify GitHub selected that exact commit."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import time
import uuid

ROOT = Path(__file__).resolve().parents[1]
WORKFLOW = "build-unsigned-ipa.yml"


def repo_slug():
    """Derive owner/repo from this checkout's origin remote (fork-aware)."""
    url = subprocess.check_output(
        ["git", "remote", "get-url", "origin"], cwd=ROOT, text=True
    ).strip()
    # Handles https://github.com/owner/repo(.git) and git@github.com:owner/repo(.git)
    m = re.search(r"github\.com[:/]([^/]+/[^/]+?)(?:\.git)?$", url)
    if not m:
        raise RuntimeError(f"Cannot derive repo slug from origin URL: {url}")
    return m.group(1)


def check_commit(expected, actual):
    if not re.fullmatch(r"[0-9a-f]{40}", expected) or expected != actual:
        raise ValueError(f"Build commit mismatch: expected {expected}, got {actual}")


def gh(*args):
    return subprocess.check_output(["gh", *args], cwd=ROOT, text=True)


def dispatch(ref, expected, verify_lgpl_relink=False):
    check_commit(expected, expected)
    repo = repo_slug()
    token = uuid.uuid4().hex
    gh("workflow", "run", WORKFLOW, "--repo", repo, "--ref", ref,
       "-f", f"expected_sha={expected}", "-f", f"dispatch_id={token}",
       "-f", "verify_lgpl_relink=" + str(verify_lgpl_relink).lower())
    # A unique title identifies this request, even with concurrent dispatches.
    for _ in range(30):
        runs = json.loads(gh("api", f"repos/{repo}/actions/workflows/{WORKFLOW}/runs?event=workflow_dispatch&per_page=100"))["workflow_runs"]
        matches = [run for run in runs if run["display_title"].endswith(f" · {token}")]
        if matches:
            if len(matches) != 1:
                raise RuntimeError("Multiple runs matched this dispatch; inspect GitHub Actions.")
            run = matches[0]
            try:
                check_commit(expected, run["head_sha"])
            except ValueError:
                gh("api", "--method", "POST", f"repos/{repo}/actions/runs/{run['id']}/cancel")
                raise
            return run["html_url"]
        time.sleep(2)
    raise RuntimeError(f"Could not verify dispatch {token}. Do not start another build; inspect GitHub Actions.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="Check workflow inputs before any compilation")
    parser.add_argument("--verify-lgpl-relink", action="store_true", help="Rebuild modified GMP and relink an audit app")
    args = parser.parse_args()
    if args.check:
        check_commit(os.environ.get("EXPECTED_SHA", ""), os.environ.get("GITHUB_SHA", ""))
        return
    expected = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    ref = subprocess.check_output(["git", "symbolic-ref", "--short", "HEAD"], cwd=ROOT, text=True).strip()
    print(dispatch(ref, expected, args.verify_lgpl_relink))


if __name__ == "__main__":
    main()
