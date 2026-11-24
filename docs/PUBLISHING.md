# Repository setup and publishing

## What the repository owner needs to do

1. Create an **empty** repository named `Addon-Sscanf` under `Legacy-Multiplayer`. Choose Public if it is ready for public source review. Do not initialize a README, license or `.gitignore`; the local project already contains them.
2. Use this repository description:

   > Typed command-parameter parsing for Legacy Multiplayer servers, with native Lua functions and player/NPC lookup.

3. Ensure the account used to push has write access and the organization permits GitHub Actions and the pinned checkout/upload/download actions used by this workflow. No bot token or personal access token is needed by the build workflow.
4. Send the repository URL to the person preparing the local checkout. Review the commit message and source before authorizing the first commit/push. The intended default branch is `main`.
5. After the authorized push, open **Actions → Build plugins**. Wait for both `windows-x86` and `linux-x86` jobs. Download their artifacts to inspect binaries, documentation, checksums and `build-manifest.json`.
6. Complete the outstanding checks in [VALIDATION.md](VALIDATION.md). Never treat a compiled transport stub as functional networking.
7. Open **Actions → Build plugins → Run workflow**, select `main` and enter a new version tag such as `v1.0.0`. Leave the tag empty for a build without a release. The workflow rejects an existing release for that tag.
8. After both build jobs and the readiness gate pass, inspect the **draft release** under Releases. Review its notes, platform requirements and attachments; then explicitly publish it when satisfied. No server installation occurs automatically.

## Commands for the authorized first push

The local checkout already uses `main` and has an SDK submodule. After confirming the real empty repository URL and reviewing the files:

```sh
git remote add origin https://github.com/Legacy-Multiplayer/Addon-Sscanf.git
git add .
git diff --cached --check
git diff --cached --stat
git commit -m "Add native Sscanf parser and cross-platform releases"
git push -u origin main
```

Do not run these commands without the owner's commit/push authorization. If `origin` already exists, inspect `git remote -v` rather than blindly replacing it. Check that `SDK` is a mode `160000` gitlink and `.gitmodules` points to the public Addon-SDK repository. The CI checkout uses `submodules: recursive`.

## Release contents

- `Addon-Sscanf-windows-x86.zip` and its `.sha256` file.
- `Addon-Sscanf-linux-x86.tar.gz` and its `.sha256` file.
- Each archive contains the plugin library, license, docs, examples when present and a manifest identifying source/SDK revisions and binary SHA-256.

GitHub-generated source archives omit submodule contents. Build from a recursive Git clone instead.
