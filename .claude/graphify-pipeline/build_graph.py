#!/usr/bin/env python3
"""Code-only (AST) graphify build for a tree, run from inside that tree.

Usage (cwd = scan root):  python build_graph.py [--list-syntax-errors FILE]
Mirrors the manual SKILL.md steps 1-4 used for the first build, plus HTML.
Prints the FULL list of files with syntax errors (stock graphify shows 5).
"""
import json
import sys
import types
from pathlib import Path
import graphify

root = Path(".").resolve()
out = root / "graphify-out"
out.mkdir(exist_ok=True)
(out / ".graphify_python").write_text(sys.executable)
(out / ".graphify_root").write_text(str(root))

from graphify.detect import detect  # noqa: E402
from graphify.paths import out_path  # noqa: E402
from graphify.watch import _read_build_excludes, _read_build_gitignore  # noqa: E402
det = detect(root, extra_excludes=_read_build_excludes(out_path()), gitignore=_read_build_gitignore(out_path()))
(out / ".graphify_detect.json").write_text(json.dumps(det, ensure_ascii=False))
print("detect:", det["total_files"], {k: len(v) for k, v in det["files"].items()})

# extract with the 5-item cap on the syntax-error warning lifted
F = Path(graphify.__file__).parent / "extract.py"
src = F.read_text().replace("_syntax_error_files[:5]", "_syntax_error_files[:9999]").replace(
    "if len(_syntax_error_files) > 5 else", "if False else")
m = types.ModuleType("graphify.extract_full")
m.__package__ = "graphify"
m.__file__ = str(F)
sys.modules[m.__name__] = m
exec(compile(src, str(F), "exec"), m.__dict__)
code = [Path(f) for f in det["files"]["code"]]
res = m.extract(code, cache_root=root, root=root)
print("AST:", len(res["nodes"]), "nodes", len(res["edges"]), "edges")
ext = {"nodes": res["nodes"], "edges": res["edges"], "hyperedges": [], "input_tokens": 0, "output_tokens": 0}
# optional persistent semantic layer (docs), produced once by merge_semantic.py
sem_path = Path(__file__).parent / "semantic.json"
if sem_path.exists():
    sem = json.loads(sem_path.read_text())
    seen = {n["id"] for n in ext["nodes"]}
    ext["nodes"] += [n for n in sem["nodes"] if n["id"] not in seen]
    ids = {n["id"] for n in ext["nodes"]}
    kept = [e for e in sem["edges"] if e["source"] in ids and e["target"] in ids]
    ext["edges"] += kept
    ext["hyperedges"] = sem.get("hyperedges", [])
    print(f"semantic layer: +{len(sem['nodes'])} nodes, +{len(kept)}/{len(sem['edges'])} edges from {sem_path.name}")
# deterministic build-system layer (library membership / linking): kept in a SEPARATE graph file,
# because a hub node per library would create spurious short paths between unrelated files.
sys.path.insert(0, str(Path(__file__).parent))
from build_system import build_layer  # noqa: E402
bn, be = build_layer(root, ext["nodes"])
_fid = {n["id"]: n for n in ext["nodes"]}
_keep = {n["id"] for n in bn} | {e["target"] for e in be if e["target"] in _fid}
bs = {"directed": False, "multigraph": False, "graph": {}, "hyperedges": [],
      "nodes": bn + [_fid[i] for i in sorted(_keep) if i in _fid],
      "links": be}
(out / "buildsys.json").write_text(json.dumps(bs, ensure_ascii=False))
print(f"build-system graph (separate file buildsys.json): {len(bn)} targets, {len(be)} edges")
# drop exact duplicate edges
_seen, _dedup = set(), []
for e in ext["edges"]:
    k = (e["source"], e["target"], e.get("relation"), e.get("source_location"))
    if k not in _seen:
        _seen.add(k)
        _dedup.append(e)
print(f"edges deduplicated: {len(ext['edges']) - len(_dedup)} removed")
ext["edges"] = _dedup
(out / ".graphify_extract.json").write_text(json.dumps(ext, ensure_ascii=False))

from graphify.build import build_from_json  # noqa: E402
from graphify.cluster import cluster, score_all  # noqa: E402
from graphify.analyze import god_nodes, surprising_connections, suggest_questions  # noqa: E402
from graphify.report import generate  # noqa: E402
from graphify.export import to_json  # noqa: E402
G = build_from_json(ext, root=str(root), directed=False)
comm = cluster(G)
coh = score_all(G, comm)
gods = god_nodes(G)
sur = surprising_connections(G, comm)
labels = {c: "Community " + str(c) for c in comm}
q = suggest_questions(G, comm, labels)
assert to_json(G, comm, str(out / "graph.json"), force=True)
(out / "GRAPH_REPORT.md").write_text(generate(G, comm, coh, labels, gods, sur, det, {"input": 0, "output": 0}, str(root), suggested_questions=q))
print("Graph:", G.number_of_nodes(), "nodes", G.number_of_edges(), "edges", len(comm), "communities")
