#!/usr/bin/env bash
set -euo pipefail

# release.sh — build a Release binary and publish it as a GitHub release.
#
# Usage:
#   ./release.sh            # version is read from CMakeLists.txt
#   ./release.sh 1.2.3      # explicit version (overrides CMakeLists.txt)
#   ./release.sh --yes      # skip the confirmation prompt
#
# What it does:
#   1. Builds raqim in Release mode into build-release/.
#   2. Stages the binary and the changelog for this version under dist/.
#   3. Creates a GitHub release tagged v<version>, attaching the binary and
#      the changelog file, with the version's changelog section as the body.

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DIST_DIR="$REPO_ROOT/dist"
BUILD_DIR="$REPO_ROOT/build-release"
CONFIRM=1

for arg in "$@"; do
    case "$arg" in
        --yes) CONFIRM=0 ;;
        --help|-h)
            sed -n '2,14p' "$0"
            exit 0
            ;;
        -*) echo "Unknown option: $arg" >&2; exit 1 ;;
        *) VERSION="$arg" ;;
    esac
done

if [[ -z "${VERSION:-}" ]]; then
    VERSION="$(sed -n 's/^project(raqim VERSION \([0-9][0-9.]*\).*/\1/p' "$REPO_ROOT/CMakeLists.txt")"
fi
if [[ -z "$VERSION" ]]; then
    echo "Could not determine a version. Pass one as an argument or set it in CMakeLists.txt." >&2
    exit 1
fi
if [[ ! "$VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "Invalid version '$VERSION' (expected X.Y.Z)." >&2
    exit 1
fi

TAG="v$VERSION"
BINARY="raqim-$VERSION-linux-x86_64"
NOTES_FILE="$DIST_DIR/CHANGELOG-$VERSION.md"

command -v gh >/dev/null 2>&1 || { echo "gh CLI not found — install it from https://cli.github.com" >&2; exit 1; }
if ! gh auth status >/dev/null 2>&1; then
    echo "Not authenticated with GitHub. Run 'gh auth login' first." >&2
    exit 1
fi
if gh release view "$TAG" >/dev/null 2>&1; then
    echo "Release $TAG already exists. Pick a new version." >&2
    exit 1
fi

echo "==> Building raqim $VERSION (Release)"
if [[ -f "$BUILD_DIR/CMakeCache.txt" ]]; then
    cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
else
    cmake -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
fi
cmake --build "$BUILD_DIR"

mkdir -p "$DIST_DIR"
cp "$BUILD_DIR/raqim" "$DIST_DIR/$BINARY"

# Extract this version's section from CHANGELOG.md to use as the release body.
awk -v ver="$VERSION" '
    $0 ~ "^## \\[" ver "\\]" { inside = 1; print; next }
    inside && /^## / { exit }
    inside { print }
' "$REPO_ROOT/CHANGELOG.md" > "$NOTES_FILE"
if [[ ! -s "$NOTES_FILE" ]]; then
    echo "No '## [$VERSION]' section found in CHANGELOG.md." >&2
    exit 1
fi

echo "==> Staged artifacts in $DIST_DIR:"
echo "    $BINARY"
echo "    CHANGELOG-$VERSION.md"

if [[ "$CONFIRM" == 1 ]]; then
    read -rp "Create GitHub release $TAG now? [y/N] " answer
    [[ "$answer" =~ ^[Yy]$ ]] || { echo "Aborted." >&2; exit 1; }
fi

gh release create "$TAG" \
    "$DIST_DIR/$BINARY" \
    "$NOTES_FILE" \
    --title "Raqim $VERSION" \
    --notes-file "$NOTES_FILE"

echo "==> Done: https://github.com/Abdogouhmad/raqim/releases/tag/$TAG"
