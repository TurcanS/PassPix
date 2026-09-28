#!/usr/bin/env bash
set -uo pipefail

BINARY="${1:-./passpix}"
PASS="smoke-test-master"
SECRET="smoke-secret-123"

if [ ! -f "$BINARY" ]; then
    echo "FAIL: binary not found: $BINARY"
    exit 1
fi

# Keep the extension so the copy still runs under MSYS2 on Windows
BIN_NAME="passpix"
case "$BINARY" in
    *.exe) BIN_NAME="passpix.exe" ;;
esac

TESTDIR=$(mktemp -d)
trap 'rm -rf "$TESTDIR"' EXIT
cp "$BINARY" "$TESTDIR/$BIN_NAME"
cd "$TESTDIR"
BIN="./$BIN_NAME"

fail() {
    echo "FAIL: $1"
    exit 1
}

# Windows console output uses CRLF line endings
run() {
    "$@" | tr -d '\r'
}

# --version and --help
VERSION_OUT=$(run $BIN --version)
[[ "$VERSION_OUT" =~ ^passpix\ [0-9]+\.[0-9]+\.[0-9]+$ ]] || fail "unexpected --version output: $VERSION_OUT"
echo "PASS: --version ($VERSION_OUT)"

run $BIN --help | grep -q "Usage:" || fail "--help has no usage"
echo "PASS: --help"

# Unknown command is a usage error
$BIN bogus >/dev/null 2>&1
[ $? -eq 2 ] || fail "unknown command should exit with 2"
echo "PASS: unknown command exits with 2"

# End of input at the menu exits instead of looping
printf '' | timeout 20 $BIN >/dev/null 2>&1
[ $? -eq 0 ] || fail "menu did not exit cleanly on end of input"
echo "PASS: menu exits on end of input"

# Mismatched passphrases create nothing
MISMATCH_OUT=$(printf '1\n%s\nsomething-else\n%s\n3\n' "$PASS" "$SECRET" | run $BIN 2>&1)
echo "$MISMATCH_OUT" | grep -q "do not match" || fail "no mismatch message"
ls img_*.png >/dev/null 2>&1 && fail "image created despite mismatched passphrases"
echo "PASS: mismatched passphrases rejected"

# Menu encrypt
ENCRYPT_OUT=$(printf '1\n%s\n%s\n%s\n3\n' "$PASS" "$PASS" "$SECRET" | run $BIN 2>&1)
echo "$ENCRYPT_OUT" | grep -q "Password encrypted" || fail "menu encryption"
echo "PASS: encryption"

ENC_FILE=$(ls img_*.png 2>/dev/null | head -1)
[ -n "$ENC_FILE" ] || fail "no encrypted file generated"
echo "PASS: file generated: $ENC_FILE"

# Menu decrypt
RESULT=$(printf '2\n1\n%s\n3\n' "$PASS" | run $BIN 2>&1)
echo "$RESULT" | grep -q "$SECRET" || fail "menu decryption did not return the password"
echo "PASS: decryption returned correct password"

# list subcommand
LIST_OUT=$(run $BIN list)
[ "$LIST_OUT" = "$ENC_FILE" ] || fail "list printed '$LIST_OUT', expected '$ENC_FILE'"
echo "PASS: list"

# decrypt subcommand prints only the password on stdout
DECRYPT_OUT=$(printf '%s\n' "$PASS" | run $BIN decrypt "$ENC_FILE" 2>/dev/null)
[ "$DECRYPT_OUT" = "$SECRET" ] || fail "decrypt printed '$DECRYPT_OUT'"
echo "PASS: decrypt subcommand"

printf 'wrong-passphrase\n' | $BIN decrypt "$ENC_FILE" >/dev/null 2>&1
[ $? -eq 1 ] || fail "wrong passphrase should exit with 1"
echo "PASS: wrong passphrase exits with 1"

$BIN decrypt missing.png </dev/null >/dev/null 2>&1
[ $? -eq 1 ] || fail "missing file should exit with 1"
echo "PASS: missing file exits with 1"

# encrypt subcommand prints only the new file name on stdout
NEW_FILE=$(printf '%s\n%s\n%s\n' "$PASS" "$PASS" "second-secret" | run $BIN encrypt 2>/dev/null)
[ -f "$NEW_FILE" ] || fail "encrypt printed '$NEW_FILE', which is not a file"
SECOND=$(printf '%s\n' "$PASS" | run $BIN decrypt "$NEW_FILE" 2>/dev/null)
[ "$SECOND" = "second-secret" ] || fail "roundtrip through subcommands returned '$SECOND'"
echo "PASS: encrypt subcommand roundtrip"

echo "All smoke tests passed."
