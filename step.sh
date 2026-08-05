#!/usr/bin/env bash
set -euo pipefail

PREFIX="/usr"
BUILD_DIR="build"

usage() {
    cat <<EOF
What would you like to do?

  1) Install raqim
  2) Uninstall raqim
  3) Help
  4) Exit
EOF
}

uninstall() {
    if [[ ! -f "$BUILD_DIR/install_manifest.txt" ]]; then
        echo "No install manifest found at $BUILD_DIR/install_manifest.txt" >&2
        echo "Run install first, or remove files manually." >&2
        return 1
    fi

    sudo cmake -P "$BUILD_DIR/cmake_uninstall.cmake"
    sudo update-desktop-database /usr/share/applications
    sudo gtk-update-icon-cache /usr/share/icons/hicolor
}

install() {
    cmake -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PREFIX"
    cmake --build "$BUILD_DIR"
    sudo cmake --install "$BUILD_DIR"
    sudo update-desktop-database /usr/share/applications
    sudo gtk-update-icon-cache /usr/share/icons/hicolor
}

while true; do
    usage
    read -rp "Enter your choice: " choice
    case "$choice" in
        1) install ;;
        2) uninstall ;;
        3) usage ;;
        4) echo "Bye"; exit 0 ;;
        *) echo "Invalid choice, please try again." >&2 ;;
    esac
    echo
done
