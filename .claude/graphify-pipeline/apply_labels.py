#!/usr/bin/env python3
"""Apply human-chosen community names to the graph built in the CWD (stripped copy).

Usage (cwd = scan root):  python apply_labels.py <labels.json>
labels.json: [{"anchor": "<node id>", "name": "<community name>"}, ...]
A name is attached to whichever community currently contains the anchor node, so it
survives a rebuild even if community numbers change. Unlabelled ones stay 'Community N'.
"""
import json, sys
from pathlib import Path
from graphify.build import build_from_json
from graphify.cluster import score_all
from graphify.analyze import god_nodes, surprising_connections, suggest_questions
from graphify.report import generate
from graphify.export import to_json

root = Path(".").resolve(); out = root / "graphify-out"
labels_in = json.loads(Path(sys.argv[1]).read_text())
g = json.loads((out / "graph.json").read_text())
node_comm = {n["id"]: n.get("community") for n in g["nodes"]}
comm = {}
for n in g["nodes"]:
    comm.setdefault(n.get("community"), []).append(n["id"])
labels = {c: f"Community {c}" for c in comm}
used, missing = 0, []
for item in labels_in:
    c = node_comm.get(item["anchor"])
    if c is None: missing.append(item["anchor"]); continue
    labels[c] = item["name"]; used += 1

ext = json.loads((out / ".graphify_extract.json").read_text())
det = json.loads((out / ".graphify_detect.json").read_text())
G = build_from_json(ext, root=str(root), directed=False)
coh = score_all(G, comm)
gods = god_nodes(G); sur = surprising_connections(G, comm)
q = suggest_questions(G, comm, labels)
assert to_json(G, comm, str(out / "graph.json"), community_labels=labels, force=True)
(out / "GRAPH_REPORT.md").write_text(generate(G, comm, coh, labels, gods, sur, det, {"input": 0, "output": 0}, str(root), suggested_questions=q))
(out / ".graphify_labels.json").write_text(json.dumps({str(k): v for k, v in labels.items()}, ensure_ascii=False))
print(f"labels applied: {used}/{len(labels_in)}" + (f"; anchors not found: {missing}" if missing else ""))
