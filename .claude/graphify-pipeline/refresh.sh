#!/usr/bin/env bash
# Rebuild the code graph from a macro-stripped copy of the repo and install it
# into <repo>/graphify-out/. Use this INSTEAD of `graphify update .`, which would
# re-parse the original sources and bring back the EXCLUSIVE_LOCKS_REQUIRED / NODISCARD / Q_* defects.
#   .claude/graphify-pipeline/refresh.sh
export LC_ALL=C
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/../.." && pwd)"
WORK="${GRAPHIFY_CLEAN_DIR:-$HOME/.cache/graphify-clean/$(basename "$REPO")}"
# graphify is pinned: a different version can parse differently and give a different graph.
GRAPHIFY_VERSION="${GRAPHIFY_VERSION:-0.9.108}"
PY="$(uv tool run --from "graphifyy==$GRAPHIFY_VERSION" python -c 'import sys; print(sys.executable)')"
echo "graphify $("$PY" -c 'import importlib.metadata as m; print(m.version("graphifyy"))') ($PY)"

rm -rf "$WORK"; mkdir -p "$WORK"
python3 -I "$HERE/strip_annotations.py" "$REPO" "$WORK" --groups "${GRAPHIFY_GROUPS:-annot,misc,cond,boost}" \
  --include src/leveldb --include src/secp256k1 --include src/univalue --include src/crc32c
( cd "$WORK" && "$PY" "$HERE/build_graph.py" ) | grep -vE 'AST extraction:|AST cache'

( cd "$WORK" && "$PY" "$HERE/apply_labels.py" "$HERE/labels.json" )
OUT="$REPO/graphify-out"; mkdir -p "$OUT/raw"
# keep the first (unstripped) graph once, for comparison
[ -f "$OUT/raw/graph.raw.json" ] || { [ -f "$OUT/graph.json" ] && cp "$OUT/graph.json" "$OUT/raw/graph.raw.json" || true; }
cp "$WORK/graphify-out/graph.json" "$WORK/graphify-out/GRAPH_REPORT.md" "$WORK/graphify-out/buildsys.json" "$OUT/"
( cd "$WORK" && "$PY" -m graphify export html >/dev/null 2>&1 ) && cp "$WORK/graphify-out/graph.html" "$OUT/" || true
echo "installed clean graph into $OUT (source of truth: stripped copy at $WORK)"
