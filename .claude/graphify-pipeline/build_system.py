#!/usr/bin/env python3
"""Deterministic build-system layer: which source file belongs to which library/binary.

Parses src/Makefile.am and src/Makefile.*.include (automake): <target>_SOURCES / _a_SOURCES
become `contains` edges to the existing file nodes; <target>_LDADD / _LIBADD entries that name
another target become `links_against` edges. Conditionals (`if ...`) are ignored: every
assignment counts, so a conditional source is listed too. Zero tokens, no LLM.
"""
import re
from pathlib import Path


def _read_vars(root: Path):
    vars_, order = {}, []
    files = [root / "src/Makefile.am"] + sorted((root / "src").glob("Makefile.*.include"))
    for f in files:
        if not f.is_file():
            continue
        text = f.read_text(errors="replace").replace("\\\n", " ")
        for ln in text.split("\n"):
            m = re.match(r"^([A-Za-z_][A-Za-z0-9_.]*)\s*(\+?=|:=)\s*(.*)$", ln.strip())
            if not m:
                continue
            name, op, val = m.group(1), m.group(2), m.group(3)
            val = val.split("#")[0].strip()
            if op == "+=" and name in vars_:
                vars_[name] += " " + val
            else:
                vars_[name] = val
            order.append(name)
    return vars_


def _expand(val, vars_, depth=0):
    if depth > 8:
        return val
    def rep(m):
        v = m.group(1)
        return _expand(vars_[v], vars_, depth + 1) if v in vars_ else m.group(0)
    return re.sub(r"\$\(([A-Za-z0-9_]+)\)", rep, val)


def build_layer(root: Path, nodes: list):
    vars_ = _read_vars(root)
    file_ids = {}
    for n in nodes:
        sf = n.get("source_file")
        if sf and n.get("source_location") == "L1" and (n["label"] == sf or n["label"] == sf.rsplit("/", 1)[-1]):
            file_ids.setdefault(sf, n["id"])
    tid = lambda t: "buildsys_" + re.sub(r"\W", "_", t)
    out_nodes, out_edges, known = {}, [], set()

    def target_of(var):
        m = re.match(r"^(.*?)(?:_a|_la)?_(SOURCES|LDADD|LIBADD)$", var)
        return (m.group(1), m.group(2)) if m else (None, None)

    targets = {}
    for var in vars_:
        t, kind = target_of(var)
        if t and kind in ("SOURCES", "LDADD", "LIBADD"):
            targets.setdefault(t, {})[kind] = var
    for t, parts in sorted(targets.items()):
        if "SOURCES" not in parts:
            continue
        label = t if t.startswith("lib") else t.replace("_", "-") if t.startswith("palladium") else t
        out_nodes[tid(t)] = {"id": tid(t), "label": f"{label} (build target)", "file_type": "concept",
                             "source_file": "src/Makefile.am", "source_location": None, "_origin": "buildsys"}
        known.add(t)
    for t, parts in sorted(targets.items()):
        if tid(t) not in out_nodes:
            continue
        for tok in _expand(vars_[parts["SOURCES"]], vars_).split():
            tok = tok.replace("$(srcdir)/", "")
            path = "src/" + tok
            fid = file_ids.get(path)
            if fid:
                out_edges.append({"source": tid(t), "target": fid, "relation": "contains", "confidence": "EXTRACTED",
                                  "confidence_score": 1.0, "source_file": "src/Makefile.am", "weight": 1.0, "_origin": "buildsys"})
        for kind in ("LDADD", "LIBADD"):
            if kind in parts:
                for tok in _expand(vars_[parts[kind]], vars_).split():
                    m = re.match(r"^(?:\.\./)?(?:\$\(builddir\)/)?(lib[\w]+)\.(?:a|la)$", tok)
                    if m and m.group(1) in known and m.group(1) != t:
                        out_edges.append({"source": tid(t), "target": tid(m.group(1)), "relation": "links_against",
                                          "confidence": "EXTRACTED", "confidence_score": 1.0,
                                          "source_file": "src/Makefile.am", "weight": 1.0, "_origin": "buildsys"})
    return list(out_nodes.values()), out_edges


if __name__ == "__main__":
    import json, sys
    g = json.load(open(sys.argv[2]))
    n, e = build_layer(Path(sys.argv[1]), g["nodes"])
    print(len(n), "targets,", len(e), "edges")
    for x in n[:50]:
        print(" ", x["label"], sum(1 for y in e if y["source"] == x["id"] and y["relation"] == "contains"), "files,",
              [y["target"][9:] for y in e if y["source"] == x["id"] and y["relation"] == "links_against"])
