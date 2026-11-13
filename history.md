# Storia del lavoro: grafo di conoscenza del repo con graphify

Branch: `experiment/graph` (creato da `master`, `75cb119`). Data: 2026-11-13.
Obiettivo: avere un grafo del repo che dia a Claude Code il massimo contesto (codice, test, documenti, struttura di build), e misurare quanto sia affidabile prima di usarlo per audit di bug concettuali e per un planner.

Questo file è un resoconto di sessione, scritto a mano. Non è tracciato da git e non è stato committato.

---

## 1. Cronologia

### 1.1 Installazione
1. Creato il branch `experiment/graph`.
2. `uv tool install graphifyy` -> graphifyy 0.9.108 (eseguibili `graphify` e `graphify-mcp`).
3. `graphify install --project` ha creato:
   - `.claude/skills/graphify/` (skill e 8 file di riferimento),
   - `.claude/CLAUDE.md` (mappa `/graphify` alla skill),
   - `.claude/settings.json` (3 hook: `graphify hook-guard search|read|compact`),
   - una sezione `## graphify` in coda al `CLAUDE.md` del repo.
4. `graphify-out/` escluso con `.git/info/exclude` (locale al clone, non trasferibile).

Nota: la skill `/graphify` non risultava registrata nella sessione in cui era stata installata (le skill si caricano all'avvio). I passi di `SKILL.md` sono stati quindi eseguiti a mano. Dopo il riavvio della sessione le istruzioni si sono ricaricate.

### 1.2 Primo build (solo codice, AST, zero token)
- Rilevamento iniziale: 1602 file (1380 codice, 102 documenti, 120 immagini), ~1,73 M parole. Oltre la soglia di 500 file della skill, quindi ci si è fermati a controllare lo scope.
- Esclusioni scelte (`.graphifyignore`): sottoalberi vendored (`src/leveldb`, `src/secp256k1`, `src/univalue`, `src/crc32c`), `depends/`, `build_msvc/`, `.claude/`, immagini, `src/qt/locale/`, `src/test/data/`, `test/fuzz/corpus/`. `src/minisketch` e `src/ipc/libmultiprocess` non esistono in questo fork.
- Dopo le esclusioni: 1016 file (939 codice, 77 documenti, 0 immagini).
- Primo grafo: 17.136 nodi, 38.683 archi, 608 community, 87% EXTRACTED / 13% INFERRED. Tempo AST: ~14 s.
- 36 file con errori di sintassi del parser; 38 file senza simboli; circa 3.950 archi (~10%) con estremo mancante.

### 1.3 Sanity check del grafo (aspettative scritte prima)
Quattro controlli con aspettative scritte prima di guardare i risultati.

| Controllo | Esito sul primo grafo |
|---|---|
| 1. `explain CTxMemPool` | PASS (6 metodi su 8 tra i primi 19 vicini; al primo tentativo nome ambiguo: 8 nodi) |
| 2. `path AcceptToMemoryPool -> CheckTransaction` | FAIL: percorso spurio di 3 salti via il nodo del file e `CheckBlock`; la catena vera mancava |
| 3. `query GetNextWorkRequired` | PARZIALE: chiamante in `validation.cpp:3493` etichettato `EXCLUSIVE_LOCKS_REQUIRED()` |
| 4. negativo `path CheckProofOfWork -> CWallet` | diretto: nessun percorso (criterio soddisfatto); `--undirected` trova 4 salti via un test case |

Causa principale trovata (verificata sul sorgente): le funzioni `static` dichiarate con la macro in coda, per esempio
`static bool AcceptToMemoryPoolWithTime(...) EXCLUSIVE_LOCKS_REQUIRED(cs_main)` (`validation.cpp:1055`) e
`static bool ContextualCheckBlockHeader(...) EXCLUSIVE_LOCKS_REQUIRED(cs_main)` (`validation.cpp:3485`),
perdono il nome: il parser prende la macro per il nome della funzione. Mancavano i nodi `AcceptToMemoryPoolWithTime` e `ContextualCheckBlockHeader`.

Altre osservazioni, tutte tuttora valide:
- Il nodo del file fa da scorciatoia (due funzioni dello stesso file sono a 2 salti).
- Un nodo segnaposto per file per le classi (`CTxMemPool` x8, `CWallet` x11): serve `path::symbol`.
- Le chiamate tramite temporaneo (`MemPoolAccept(pool).AcceptSingleTransaction(...)`, `validation.cpp:1061`) non sono risolte.
- Gli archi di chiamata tra file sono tutti INFERRED 0,85.
- I test sono circa un terzo dei risultati di una query (19 nodi in `src/test`/`src/bench` su 105-121).
- I `god nodes` iniziali erano dominati dai test (`assert_equal`, `PalladiumTestFramework`).

### 1.4 Copia pulita senza macro
Script che copia il repo fuori da esso e svuota le macro di annotazione sostituendole con spazi (stessi numeri di riga e di colonna; verificato su 937 file C/C++ con 0 differenze).
- Prima versione: `EXCLUSIVE_LOCKS_REQUIRED`, `LOCKS_EXCLUDED`, `GUARDED_BY`, `NODISCARD`, `Q_OBJECT`, `Q_SIGNALS`, `Q_SLOTS`.
- Risultato: nodi con etichetta uguale a una macro da 83 a 0; compaiono i nodi mancanti; il chiamante a `validation.cpp:3493` passa a `ContextualCheckBlockHeader()`; file con errori da 36 a 17.
- Controllo nuovo su `ProcessOrphanTx` (net_processing): presente (L1919), callee `AcceptToMemoryPool`, `EraseOrphanTx`, `RelayTransaction`, `MaybePunishNodeForTx`; caller `ProcessMessage` (L2631) e `.ProcessMessages()` (L3373); righe verificate sul sorgente.
- Controllo 2 dopo la pulizia: 3 passi su 4 diretti. Resta mancante `ATMPWithTime -> AcceptSingleTransaction` (chiamata via temporaneo).
- Bug del mio script scoperto e corretto lungo la strada: saltava solo le righe `#define`, ma anche `#ifndef NODISCARD` veniva svuotata, il che creava un errore nuovo in `attributes.h`; ora salta tutte le righe del preprocessore.

### 1.5 Pipeline stabile
`refresh.sh` (circa 30-70 s, nessun token): pulizia -> build -> etichette -> installazione in `graphify-out/`. Il grafo originale (con il difetto) è conservato in `graphify-out/raw/graph.raw.json`. La regola del `CLAUDE.md` "dopo una modifica esegui `graphify update .`" è stata sostituita con `refresh.sh`, con l'avvertimento che `graphify update .` riporterebbe il difetto.

### 1.6 Etichette delle community
Le community sono cluster di nodi più collegati tra loro che col resto, calcolati sugli archi (non sul significato). I nomi sono miei, scelti leggendo i nodi principali.
- Prime 45 etichette, agganciate a un nodo "àncora" (non all'id della community) perché la numerazione cambia tra build.
- Dopo i cambiamenti successivi la suddivisione si è spostata: ho aggiunto etichette e ne ho rinominate (alcune restano "(mixed)", cioè descrivono solo in parte). Stato finale: 66 etichette su 648 community.

### 1.7 Documenti (estrazione semantica con subagenti, costa token)
- Dopo il tuo ok ("usa claude code per usare token"): 77 documenti in 4 chunk, 4 subagenti in parallelo, circa 418.000 token. Risultato: 207 nodi, 321 archi, 96 archi documento->codice.
- Nel prompt dei subagenti, oltre alla specifica di graphify, ho aggiunto: collegare i concetti ai nodi di codice esistenti cercando l'id con `graphify explain`, mai inventato.
- Limiti dichiarati dai subagenti: il chunk 1 ha solo scorso `CONTRIBUTING.md` (righe 120-362) e troncato le righe lunghe di `README.md`, `contrib/README.md`, `contrib/macdeploy/README.md`; `contrib/seeds/suspicious_hosts.txt` letto 15 righe su 16. Per questi file l'estrazione è parziale.
- Seconda passata mirata (1 subagente, circa 118.000 token) su `doc/palladium-consensus.md`, `doc/TAPROOT.md`, `doc/taproot-guide.md`, `CHANGELOG.md`, `doc/bips.md`, `README.md`, `CLAUDE.md`: +64 nodi, +142 archi (134 verso il codice). I simboli senza nodo proprio (`TaprootHeight`, `nPowTargetSpacing`, limite OP_RETURN, porte, HRP) sono collegati al file che li definisce con arco INFERRED 0,85. Non risolti: `CChainParams`, `XOnlyPubKey`, `CheckSchnorrSignature`, `CheckTapTweak`, `CreateSig`, `GetTaprootInternalKey`, `MemPoolAccept`, `GetStandardScriptVerifyFlags`, `CMainParams`, `CTestNetParams`, `CRegTestParams`, `getnewaddress`.
- Strato dei documenti unito: 271 nodi, 463 archi (230 documento<->codice), 0 scartati. Salvato in `.claude/graphify-pipeline/semantic.json` (non è rigenerato da `refresh.sh`).
- Costo totale dei subagenti: circa 536.000 token.
- Correzione mia: un primo conteggio di 4.208 archi documento<->codice era sbagliato (contava anche i nodi segnaposto `concept`); il dato corretto è 99 (prima passata) e 230 dopo la seconda.

### 1.8 Copertura del codice
Audit: dei file tracciati con estensione di codice (1183) ne sono nel grafo 1179 (99%). I 4 mancanti sono script Python in `build_msvc/`.
Cambiamenti, ognuno misurato separatamente:
- Gruppo `misc`: `QT_BEGIN_NAMESPACE`/`QT_END_NAMESPACE`, `UNUSED`, `__declspec(...)`, uso della macro tinyformat, blocchi `asm`.
- Gruppo `cond`: per `netbase.cpp` e `util/time.cpp` si tiene solo il primo ramo di `#ifdef ... #else`. Primo difetto mio: lasciava `#ifdef` senza `#endif`; corretto.
- `.graphifyrc` con `language.h=cpp`: graphify tratta come C gli header senza parole chiave C++ (`stdin.h` con un costruttore falliva).
- Gruppo `boost`: `BOOST_AUTO_TEST_CASE(nome)` -> `void nome()` (459 casi), suite svuotate (180). Ora i test hanno nomi veri (`get_next_work_resilience_fork()`, `get_next_work_lwma_before_fork()`, ...).
- Sottoalberi vendored reinclusi (+4.038 nodi: leveldb, secp256k1, univalue, crc32c), richiesta "copertura totale".
- File con errori di sintassi: da 36 a 23, di cui 4 in codice di Palladium (`crypto/sha256_sse4.cpp`, `palladium-cli.cpp`, `test/arith_uint256_tests.cpp`, `tinyformat.h`) e 19 vendored (soprattutto secp256k1 e leveldb). Il codice di Palladium con errori: da 17 a 4.
- 75 file rilevati hanno solo il nodo del file: 61 header (soli prototipi, costanti, macro), 8 `.cpp` (tra cui `sha256_sse4.cpp` e `winshutdownmonitor.cpp`, perdite reali), 3 `.py`, 2 `.c`, 1 `.cc`.

### 1.9 Strato di build (Makefile)
`build_system.py` legge `src/Makefile.am` e `src/Makefile.*.include` e produce 104 target (libreria/binario) con archi `contains` verso i file e `links_against` verso altre librerie. Esempi: `libpalladium_consensus` con 26 file, `palladiumd` che collega server, wallet, common, util, zmq, consensus.
Lo strato inizialmente era nel grafo principale. Il controllo negativo è tornato a fallire (percorso EXTRACTED `CheckProofOfWork <- pow.cpp <- libpalladium_server -> wallet.h -> CWallet`): l'hub del target accorciava artificialmente i percorsi. Spostato in un grafo separato (`graphify-out/buildsys.json`); il controllo negativo è tornato a passare. Si interroga con `--graph graphify-out/buildsys.json`.

### 1.10 Stato finale del grafo (ultima ricostruzione completa)
- 22.446 nodi, 53.164 archi, 648 community, 66 con nome.
- Tipi di nodo: 21.066 `code`, 663 `concept`, 640 `rationale`, 77 `document`.
- File: `graph.json` 24 MB (1,5 MB gzip), `buildsys.json` 1,3 MB, `graph.html`, `GRAPH_REPORT.md`.
- Controlli finali: `explain CTxMemPool` passa; catena ATMP: 3 passi su 4 diretti; `GetNextWorkRequired`: chiamanti, chiamati, test e documenti nominati; negativo diretto: nessun percorso.
- `graph.html` mostra solo una vista aggregata (un nodo per community), perché i nodi superano 5000. L'HTML non è stato aperto nel browser durante la sessione: non so come appaia con le etichette.

### 1.11 Portabilità verso un altro host
Preparato per il clone su un altro host (non ancora provato):
- `semantic.json`: 741 `source_file` assoluti resi relativi; `merge_semantic.py` ora li rende relativi anche per le estrazioni future.
- `.graphifyignore` spostato in `.claude/graphify-pipeline/graphifyignore` (la copia nella root, che era in `.git/info/exclude`, è stata rimossa).

### 1.12 Commit
Su `experiment/graph`, senza riga `Co-Authored-By` (preferenza registrata per questo repo):
- `040b240` `build: add a pipeline that builds a code knowledge graph with graphify`: `.claude/graphify-pipeline/` (script, `labels.json`, `semantic.json`, `first_branch_files.json`, `graphifyignore`) e `CLAUDE.md`.
- `ed6ac67` `build: add the graphify skill for Claude Code`: `.claude/skills/graphify/` e `.claude/CLAUDE.md`.
Nessun push (lo fai tu).

---

## 2. Domande e risposte principali
- Che cos'è una community e dove vive il grafo: tutto in locale (`graphify-out/`, `~/.cache/graphify-clean/palladiumcore/`); dal tuo computer non esce nulla. Il modello è coinvolto solo nell'estrazione semantica dei documenti. `graph.html` scarica solo la libreria `vis-network` da unpkg.com.
- Come aprire `graph.html` (Ubuntu/WSL): `xdg-open`, `explorer.exe "$(wslpath -w ...)"`, oppure `python3 -m http.server`.
- Committare il grafo? Consigliato committare gli ingredienti e ricostruire sull'altro host: `graph.json` pesa 24 MB (1,5 MB compresso) e viene riscritto per intero a ogni build.

---

## 3. File e cartelle (stato attuale)

| Cosa | Dove | In git |
|---|---|---|
| Pipeline e dati | `.claude/graphify-pipeline/` (`refresh.sh`, `strip_annotations.py`, `build_graph.py`, `build_system.py`, `apply_labels.py`, `merge_semantic.py`, `labels.json`, `semantic.json`, `first_branch_files.json`, `graphifyignore`) | sì (`040b240`) |
| Skill | `.claude/skills/graphify/`, `.claude/CLAUDE.md` | sì (`ed6ac67`) |
| Regole per Claude | sezione `## graphify` di `CLAUDE.md` | sì (`040b240`) |
| Hook | `.claude/settings.json` | **no**, volutamente |
| Output | `graphify-out/` (`graph.json`, `buildsys.json`, `graph.html`, `GRAPH_REPORT.md`, `raw/`) | no (escluso) |
| Copia pulita e cache | `~/.cache/graphify-clean/palladiumcore/`, `~/.cache/graphify-clean/semantic/chunk_*.json` | fuori dal repo |
| Questo file | `history.md` | no |

Dipendenze: `uv`, Python 3, `git`, e `graphifyy` (versione usata: 0.9.108).

## 4. Come ricreare il grafo (non ancora verificato da un clone pulito)
1. Clonare il repo sul branch con i commit `040b240` e `ed6ac67`.
2. Installare il tool: `uv tool install graphifyy==0.9.108`.
3. Eseguire `.claude/graphify-pipeline/refresh.sh` (circa 1 minuto, nessun token).
4. Gli hook sono opzionali: vanno solo se `graphify` è nel PATH.
Per ricostruire lo strato dei documenti servirebbe di nuovo l'estrazione con subagenti: `semantic.json` è già nel commit.

---

## 5. Limiti noti del grafo
- Scorciatoia del nodo di file nei `path`: verificare le catene un salto alla volta.
- Nodi segnaposto per classe per file: disambiguare con `file::simbolo`.
- Chiamate tramite temporaneo non risolte; archi di chiamata tra file INFERRED 0,85.
- Risultati inquinati da `src/test` e `src/bench` (circa 18-34% a seconda della query) e dal codice vendored.
- Bassa confidenza su `src/netbase.cpp`, `src/util/time.cpp`, `src/tinyformat.h`, `src/palladium-cli.cpp`, `src/qt/` e sui 4 file ancora con errori.
- La ricerca testuale di `query` è fuzzy (esempio: "fork" ha agganciato `GetDestinationForKey`).
- I numeri delle community cambiano tra build con input diversi; le etichette sono approssimative.
- I documenti dichiarano pochi collegamenti verso `chainparams.cpp`/RPC; la lettura di alcuni README/`CONTRIBUTING.md` è parziale.

## 6. Decisioni e attività aperte
1. **Test della pipeline da un clone pulito**: avviato due volte, interrotto entrambe da te; non ho quindi la prova che `refresh.sh` funzioni su un altro host.
2. **Istruzioni per l'altro host**: richieste, non ancora scritte in un file (qui sopra c'è solo la sintesi).
3. **Hook** (`.claude/settings.json`): da decidere se versionarli, dato che falliscono senza `graphify`.
4. **`graph.json` in git** o no (consigliato no; alternativa `.gz`).
5. **Push** del branch: a tuo carico.
6. Cause ancora aperte nei 4 file di Palladium con errori e nei file di dati senza simboli.
7. Collegamenti documento->codice mancanti (`TaprootHeight`, `nResilienceForkHeight` e le funzioni RPC).
8. Verificare che `graph.html` si apra e sia leggibile; riavviare la sessione per avere `/graphify` come comando.
9. Per un eventuale merge su `master`: valutare cosa portare (il `CLAUDE.md` impone di usare il grafo anche a chi non lo ha).

## 7. Note di metodo
- Le aspettative sono state scritte prima di ogni controllo e i risultati sono stati confrontati con il sorgente (righe citate verificate).
- Il test del grafo ha trovato difetti miei oltre a quelli di graphify: bug nello script di pulizia (`#ifndef`, `#ifdef` senza `#endif`), e un hub di build che ha rotto il controllo negativo.
- Le misure intermedie (liste di errori di sintassi per variante) provengono da build di prova con le stesse trasformazioni; i controlli finali sono stati rieseguiti sul grafo installato.
