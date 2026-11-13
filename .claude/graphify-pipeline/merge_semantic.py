#!/usr/bin/env python3
"""Merge doc-extraction chunks into one persistent semantic.json.

Usage: merge_semantic.py <chunk_dir> <graph.json> <out semantic.json>
- merges chunk_*.json (dedupe nodes by id)
- keeps only edges whose endpoints exist (in the chunks OR in the code graph); drops the rest and reports them
- reports coverage and counts
The result is applied by build_graph.py on every refresh (it merges semantic.json if present).
"""
import glob, json, sys
from pathlib import Path

chunk_dir, graph_path, out_path = sys.argv[1:4]
g = json.loads(Path(graph_path).read_text())
code_ids = {n["id"] for n in g["nodes"]}

nodes, edges, hyper, cov = {}, [], [], {}
for c in sorted(glob.glob(f"{chunk_dir}/chunk_*.json")):
    d = json.loads(Path(c).read_text())
    for n in d.get("nodes", []):
        nodes.setdefault(n["id"], n)
    edges += d.get("edges", [])
    hyper += d.get("hyperedges", [])
    cov.update(d.get("coverage", {}))

ok, dropped, seen_e = [], [], set()
ids = set(nodes) | code_ids
for e in edges:
    k = (e["source"], e["target"], e.get("relation"))
    if k in seen_e:
        continue
    seen_e.add(k)
    (ok if e["source"] in ids and e["target"] in ids else dropped).append(e)
to_code = sum(1 for e in ok if (e["target"] in code_ids and e["target"] not in nodes) or (e["source"] in code_ids and e["source"] not in nodes))
hyper_ok = [h for h in hyper if all(n in ids for n in h.get("nodes", []))]
for n in nodes.values():
    n.setdefault("_origin", "semantic")
for e in ok:
    e.setdefault("_origin", "semantic")

import re
_pre = re.compile(r"^.*?/graphify-clean/[^/]+/")      # portable: repo-relative source_file
for o in list(nodes.values()) + ok + hyper_ok:
    sf = o.get("source_file")
    if isinstance(sf, str) and sf.startswith("/"):
        o["source_file"] = _pre.sub("", sf)
Path(out_path).write_text(json.dumps({"nodes": list(nodes.values()), "edges": ok, "hyperedges": hyper_ok,
                                      "input_tokens": 0, "output_tokens": 0}, ensure_ascii=False))
print(f"chunks merged -> {len(nodes)} nodes, {len(ok)} edges kept ({to_code} link doc->code), "
      f"{len(dropped)} dropped (missing endpoint), {len(hyper_ok)}/{len(hyper)} hyperedges, coverage for {len(cov)} files")
for e in dropped[:8]:
    print("  dropped:", e["source"], "->", e["target"], e.get("relation"))
