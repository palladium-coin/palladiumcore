#!/usr/bin/env python3
"""Copy a source tree, rewriting constructs that tree-sitter cannot parse.

Usage: strip_annotations.py <src_root> <dest_root> [--groups a,b,c] [--include DIR ...]

Every replaced character becomes a space (newlines kept), so line AND column numbers
of the copy equal the original. Preprocessor lines ('#...') are never touched.
Groups (each is one measurable transformation):
  annot  EXCLUSIVE_LOCKS_REQUIRED/LOCKS_EXCLUDED/GUARDED_BY(...), NODISCARD, Q_OBJECT,
         Q_SIGNALS/Q_SLOTS                                        (default on)
  misc   QT_BEGIN/END_NAMESPACE, UNUSED, __declspec(...), tinyformat macro uses,
         asm/__asm__ blocks                                        (default on)
  cond   keep only the first branch of #if/#else/#elif..#endif in the files listed in
         first_branch_files.json (conditional code split mid-statement)  (default on)
  boost  BOOST_AUTO/FIXTURE_TEST_CASE(name..) -> `void name()`, suites blanked  (opt-in)
"""
import bisect, json, re, shutil, subprocess, sys
from collections import defaultdict
from pathlib import Path

HERE = Path(__file__).resolve().parent
ARGS_MACROS = {
    "annot": ("EXCLUSIVE_LOCKS_REQUIRED", "LOCKS_EXCLUDED", "GUARDED_BY"),
    "misc": ("__declspec", "TINYFORMAT_DEFINE_FORMAT_TRUNCATED_CSTR"),
}
BARE = {"annot": ("NODISCARD", "Q_OBJECT"), "misc": ("QT_BEGIN_NAMESPACE", "QT_END_NAMESPACE")}
EXTS = {".h", ".hpp", ".c", ".cc", ".cpp", ".cxx"}
RE_ACC = re.compile(r"\b(Q_SIGNALS|Q_SLOTS)\b")
RE_AFTER_ACCESS = re.compile(r"\b(public|private|protected)\s+$")
RE_UNUSED = re.compile(r"\bUNUSED\b(?!\s*\()")
RE_ASM = re.compile(r"\b(__asm__|asm)\b(\s*(__volatile__|volatile))?\s*\(")
RE_BOOST_CASE = re.compile(r"\bBOOST_(?:AUTO|FIXTURE)_TEST_CASE\s*\(\s*(\w+)\s*(?:,[^)]*)?\)")
RE_BOOST_SUITE = re.compile(r"\bBOOST_(?:AUTO|FIXTURE)_TEST_SUITE(?:_END)?\s*\([^)]*\)")


def blank(s):
    return "".join(c if c == "\n" else " " for c in s)


def match_paren(text, j):
    """text[j-1] == '(' ; return index after the matching ')' or -1. Skips string/char literals."""
    depth, n = 1, len(text)
    while j < n and depth:
        c = text[j]
        if c in "\"'":
            q = c; j += 1
            while j < n and text[j] != q:
                j += 2 if text[j] == "\\" else 1
        elif c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
        j += 1
    return j if depth == 0 else -1


def first_branch_only(text):
    """For every #if group that has an #else/#elif: keep the first branch, blank the
    other branches AND all of that group's directives (so no #if is left without #endif)."""
    lines = text.split("\n")
    stack, groups = [], []       # groups: dict(directive line idxs, has_else)
    for i, ln in enumerate(lines):
        m = re.match(r"\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b", ln)
        if not m:
            continue
        d = m.group(1)
        if d in ("if", "ifdef", "ifndef"):
            g = {"dirs": [i], "else_from": None, "end": None}
            groups.append(g); stack.append(g)
        elif stack:
            g = stack[-1]; g["dirs"].append(i)
            if d in ("elif", "else") and g["else_from"] is None:
                g["else_from"] = i
            if d == "endif":
                g["end"] = i; stack.pop()
    kill = set()
    for g in groups:
        if g["else_from"] is not None and g["end"] is not None:
            kill.update(g["dirs"])
            kill.update(range(g["else_from"], g["end"] + 1))
    return "\n".join(blank(ln) if i in kill else ln for i, ln in enumerate(lines))


def strip_text(text, groups, stats):
    pre = {i for i, ln in enumerate(text.split("\n")) if re.match(r"\s*#", ln)}
    starts = [0] + [m.end() for m in re.finditer("\n", text)]
    lineno = lambda off: bisect.bisect_right(starts, off) - 1
    edits = []

    def add(s, e, r, key):
        if lineno(s) in pre:
            return
        edits.append((s, e, r)); stats[key] += 1

    for g in groups & {"annot", "misc"}:
        names = ARGS_MACROS.get(g, ())
        if names:
            for m in re.finditer(r"\b(" + "|".join(names) + r")\b\s*\(", text):
                e = match_paren(text, m.end())
                if e < 0: stats["unbalanced"] += 1; continue
                add(m.start(), e, blank(text[m.start():e]), m.group(1))
        if BARE.get(g):
            for m in re.finditer(r"\b(" + "|".join(BARE[g]) + r")\b", text):
                add(m.start(), m.end(), blank(m.group(0)), m.group(1))
    if "annot" in groups:
        for m in RE_ACC.finditer(text):
            rep = blank(m.group(0)) if RE_AFTER_ACCESS.search(text[max(0, m.start() - 20):m.start()]) \
                else "public".ljust(len(m.group(0)))
            add(m.start(), m.end(), rep, m.group(1))
    if "misc" in groups:
        for m in RE_UNUSED.finditer(text):
            add(m.start(), m.end(), blank(m.group(0)), "UNUSED")
        for m in RE_ASM.finditer(text):
            e = match_paren(text, m.end())
            if e < 0: stats["unbalanced"] += 1; continue
            add(m.start(), e, blank(text[m.start():e]), "asm")
    if "boost" in groups:
        for m in RE_BOOST_CASE.finditer(text):
            rep = f"void {m.group(1)}()".ljust(m.end() - m.start())
            add(m.start(), m.end(), rep, "BOOST_CASE")
        for m in RE_BOOST_SUITE.finditer(text):
            add(m.start(), m.end(), blank(m.group(0)), "BOOST_SUITE")

    edits.sort()
    res, pos = [], 0
    for s, e, r in edits:
        if s < pos:
            continue
        res.append(text[pos:s]); res.append(r); pos = e
    res.append(text[pos:])
    new = "".join(res)
    assert len(new) == len(text) and new.count("\n") == text.count("\n")
    return new


def main(argv):
    src, dst = Path(argv[0]).resolve(), Path(argv[1]).resolve()
    groups = {"annot", "misc", "cond"}
    includes = []
    i = 2
    while i < len(argv):
        if argv[i] == "--groups":
            groups = set(argv[i + 1].split(",")) - {""}; i += 2
        elif argv[i] == "--include":
            includes.append(argv[i + 1].rstrip("/") + "/"); i += 2
        else:
            raise SystemExit("bad arg " + argv[i])
    fb = set(json.loads((HERE / "first_branch_files.json").read_text())) if "cond" in groups else set()
    files = subprocess.check_output(["git", "-C", str(src), "ls-files", "-z"]).decode().split("\0")
    stats = defaultdict(int)
    for rel in filter(None, files):
        s = src / rel
        if not s.is_file():
            continue
        d = dst / rel
        d.parent.mkdir(parents=True, exist_ok=True)
        if s.suffix in EXTS:
            raw = s.read_bytes()
            try:
                text, enc = raw.decode("utf-8"), "utf-8"
            except UnicodeDecodeError:
                text, enc = raw.decode("latin-1"), "latin-1"
            stats["files_total"] += 1
            new = strip_text(text, groups, stats)
            if rel in fb:
                new = first_branch_only(new); stats["first_branch_files"] += 1
            if new != text:
                stats["files_changed"] += 1
            d.write_bytes(new.encode(enc))
        else:
            shutil.copy2(s, d)
    (dst / ".graphifyrc").write_text("# .h headers without an obvious C++ keyword were parsed as C and failed\nlanguage.h=cpp\n")
    ig = HERE / "graphifyignore"
    if not ig.exists():
        ig = src / ".graphifyignore"
    if ig.exists():
        lines = ig.read_text().splitlines()
        # --include DIR: drop that directory's exclusion line so it enters the graph
        lines = [l for l in lines if l.strip() not in includes]
        (dst / ".graphifyignore").write_text("\n".join(lines) + "\n")
    print(dict(stats))


if __name__ == "__main__":
    main(sys.argv[1:])
