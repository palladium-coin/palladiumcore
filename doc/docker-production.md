# Running a Palladium node in production with Docker

How to put the node of [docker/](../docker/README.md) on a server, for each way you may use it: a node alone, the
backend of a mining pool, of a block explorer, of an Electrum server, or of all of them. For what the image is and
how to build it, see [docker/README.md](../docker/README.md).

The commands are typed in a terminal **on the server**, in the `docker/` directory of the repository (get it with
`git clone https://github.com/palladium-coin/palladiumcore.git`), unless they say otherwise. In a command or a file,
`<something like this>` is a value that you replace with your own.

### Words used in this guide

| Word | What it is |
| --- | --- |
| node | the program `palladiumd`: it keeps a copy of the blockchain and talks to the other nodes |
| image, container | the image is the packaged program; a container is a running copy of it. Docker Compose starts containers from the file `docker-compose.yml` |
| data directory | the folder where the node keeps the blockchain, the wallet and its settings: here the `~/.palladium` of the server |
| `palladium.conf` | the settings file of the node, inside the data directory |
| synchronized | the node has downloaded and checked the whole chain, up to the latest block |
| P2P | the connections between nodes (port 2333) |
| RPC | how other programs ask the node for data and give it commands (port 2332). It needs a user and a password |
| ZMQ | a stream of notifications from the node ("a new block arrived"), so that a service does not have to keep asking |
| `txindex` | an optional index that lets the node look up any transaction by its id. Explorers and Electrum servers need it |
| wallet | the file with the keys and the coins. Only a mining pool needs one, to pay its miners |
| pool, explorer, Electrum server | programs of yours that use the node: a mining pool, a block explorer website, a server for Electrum wallets |

## 1. Pick your setup

| You run | Needs from the node | Go to |
| --- | --- | --- |
| Only the node (to sync, to relay, to use `palladium-cli`) | nothing: the defaults | [3](#3-the-data-directory-and-palladiumconf), then [5](#5-start-the-node) |
| A mining pool | RPC user, ZMQ `hashblock`, wallet, synchronized node | [6.1](#61-mining-pool) |
| A block explorer | `txindex=1`, RPC user, ZMQ | [6.2](#62-block-explorer) |
| An Electrum server (Fulcrum / ElectrumX) | `txindex=1`, RPC user, ZMQ `hashblock` | [6.3](#63-electrum-server-fulcrum--electrumx) |
| Several of them | the union of the above, one RPC user per service | [6.4](#64-more-than-one-service) |

What each option does is in [Appendix A](#appendix-a-what-each-option-does).

## 2. Prepare the host

1. **Docker** with the Compose plugin. Install Docker Engine as described at <https://docs.docker.com/engine/install/>
   and check that `docker compose version` prints a version. Then run `sudo systemctl enable docker`, so that Docker
   starts when the server boots. The compose file has `restart: unless-stopped`, so the node comes back after a
   reboot or a crash, and stays down only if you stopped it yourself.
2. **Disk.** The chain is stored in full (a few hundred gigabytes according to [doc/README.md](README.md); on a node
   that is already running, `du -sh ~/.palladium` tells you the real size) and `txindex=1` adds to it. Use an SSD: the
   first synchronization and the index build read and write a lot. Keep free space (`df -h ~` shows it): a node whose
   disk fills up stops, and that can corrupt its database.
3. **Time.** Keep the clock synchronized (`timedatectl`: `System clock synchronized: yes`). A node whose clock is off
   rejects blocks and peers.
4. **Network.** The node needs outbound access to other nodes on port 2333 (and DNS for the seeds), and inbound on 2333
   to help the network (section 7).
5. **Log size.** Docker keeps the logs of a container without limit by default, and over months they fill the disk.
   Cap them by adding the `logging:` lines to the service in `docker-compose.yml`. The indentation matters (spaces,
   not tabs): `logging:` lines up with `image:` and `restart:`.

   ```yaml
   services:
     palladiumd:
       image: ghcr.io/palladium-coin/palladiumcore:2.1.0
       logging:
         driver: json-file
         options: { max-size: "50m", max-file: "5" }     # at most 5 files of 50 MB
       # ... the rest of the service stays as it is
   ```

6. **One node per data directory.** Stop `palladiumd` / `palladium-qt` on the host if they use the same directory.

## 3. The data directory and `palladium.conf`

The data directory is the **`~/.palladium` of the host** (another one: `PALLADIUM_DATADIR=/path docker compose ...`, or
`export` it in the shell or in a `.env` file next to the compose file). The node and `palladium-cli` use the
`palladium.conf` in it.

1. The folder must belong to the user that the container runs as, which is the one with number 1000 (the first user
   created on most servers; `id -u` shows yours):

   ```bash
   mkdir -p ~/.palladium && sudo chown 1000:1000 ~/.palladium
   ```

   If `id -u` prints another number, start the node with that user instead:
   `PLM_UID=$(id -u) PLM_GID=$(id -g) docker compose up -d`. Without the right owner the node cannot write to the
   folder and stops at once (see Troubleshooting).
2. **Create `palladium.conf` before the first start**, with a text editor (`nano ~/.palladium/palladium.conf`: paste,
   Ctrl+O and Enter to save, Ctrl+X to leave). Do not create it with `sudo`: the node must be able to read it (if you
   did, `sudo chown 1000:1000 ~/.palladium/palladium.conf`). If the file does not exist, the node writes one on its
   first start, but that first run does not use its passwords and `palladium-cli` fails with
   `Incorrect rpcuser or rpcpassword` until a `docker compose restart`. That file also contains
   `rpcuser=username` and `rpcpassword=password`, which everybody knows: never keep them on a server.
3. A starting point for any setup:

   ```ini
   server=1
   listen=1                  # accept inbound peers; port 2333 is published by the compose file
   maxconnections=40         # the default is 125, the file written by the node uses 20
   dbcache=2048              # MiB. Large for the first sync or a reindex, then 512-1024 is enough
   rpcthreads=16             # default 4: pools and indexers send many calls at once
   rpcworkqueue=64           # default 16: calls waiting for a thread; above it the node answers with an error
   rpcservertimeout=60

   zmqpubhashblock=tcp://0.0.0.0:28332
   zmqpubrawblock=tcp://0.0.0.0:28334
   zmqpubrawtx=tcp://0.0.0.0:28335
   ```

   What the lines do, so that you can change them knowing why:

   - `server=1`: turns on the RPC, which every service needs.
   - `listen=1`, `maxconnections`: the node accepts connections from other nodes, up to that number of peers in all.
   - `dbcache`: memory (in MiB, so 2048 is 2 GB) that the node uses as cache. Lower it on a small server (512 is
     enough once the chain is synchronized); a larger one makes the first synchronization faster.
   - `rpcthreads`, `rpcworkqueue`, `rpcservertimeout`: how many calls from your services the node handles at once.
     Leave them as they are unless a service gets errors about a busy node.
   - `zmqpub...`: the notifications. They do nothing if no service listens, so they can stay in the file.

   And, good to know:

   - `daemon=1` is overridden by the image (`-daemon=0`): a container must stay in the foreground.
   - `rpcbind` and `rpcallowip` can stay on `127.0.0.1`: the compose file adds `-rpcbind=0.0.0.0` and
     `-rpcallowip=172.16.0.0/12` (the networks of Docker) on the command line, and they are added to the ones in the file.
   - ZMQ is a one-way stream with no authentication. The container binds it on `0.0.0.0`, but it is not published on the
     host, so only the `palladium` network sees it. Do not add these ports to `ports:`.
   - Behind NAT or with several addresses, add `externalip=<public ip>` so that other nodes learn it.
4. Add the lines that your setup needs (section 6).

## 4. RPC users

The cookie in `~/.palladium/.cookie` changes at every start and is for `palladium-cli` on the same host. A service in
another container needs a user and a password, and one user per service lets you change or restrict one without
touching the others. The node does not store the password, only a hash of it. You make the pair with a script of the
repository (it needs `python3`), run from the root of the repository:

```bash
python3 share/rpcauth/rpcauth.py pool        # "pool" is the user name; run it again for explorer, fulcrum, ...
```

It prints two things: a line that starts with `rpcauth=` and a generated password. Paste the `rpcauth=` line in
`palladium.conf` and give the password to the service (in its own configuration, with the user name you chose). The
password is printed only once: save it where the service's configuration is. Then `docker compose restart`. Anything
on the `palladium` network can try to log in, so keep the passwords long and one per service.

**Optional, for the careful: limit what each service can call.** Add `rpcwhitelist` lines. Skip this on a first
setup, and add it later if you want it:

```ini
rpcwhitelist=pool:getblocktemplate,submitblock,getblockchaininfo,getblockcount,getmininginfo,getnetworkinfo,getrawtransaction,validateaddress,sendtoaddress,getbalance
rpcwhitelist=explorer:getblockchaininfo,getblock,getblockhash,getblockcount,getrawtransaction,getrawmempool,getmempoolinfo,getmempoolentry,getnetworkinfo,getpeerinfo,gettxout,getchaintips,getmininginfo,estimatesmartfee
rpcwhitelistdefault=0
```

With `rpcwhitelistdefault=0` the users without a line are not restricted. **Without it, as soon as one `rpcwhitelist`
exists, every other user (and the cookie used by `palladium-cli` in the container) can call nothing.** The lists above
are a starting point, not tested against a real pool or explorer: if a service is refused a method, add it.

## 5. Start the node

```bash
docker compose up -d
docker compose logs -f palladiumd      # Ctrl+C leaves the node running
```

`up -d` starts the node in the background; `logs -f` shows what it is doing. The first start downloads and checks the
whole chain, which takes hours, and possibly more than a day on a slow server or disk. The node answers commands in the
meantime, and the log keeps scrolling with lines such as `Synchronizing blockheaders` and then `UpdateTip`. The node is
**synchronized** when `blocks` equals `headers` and `verificationprogress` is about 1 (0.99999...):

```bash
docker compose exec palladiumd palladium-cli getblockchaininfo
docker compose exec palladiumd palladium-cli getconnectioncount     # > 0: it has peers
```

**Do not connect a service before the node is synchronized**: a pool's `getblocktemplate` answers "Palladium is not
connected!" or that the node is in initial block download, and an indexer would build on an incomplete chain. With
`txindex=1` the index is built on the first start while the node also syncs: expect hours more on a full chain, and the
log shows the progress.

## 6. Setups

Read only the section of what you run. In each one: add the lines to `palladium.conf` (section 3), and
`docker compose restart` so that the node reads them. `<the line printed for "pool">` means the `rpcauth=...` line that
`rpcauth.py` printed for that service (section 4).

### 6.1 Mining pool

`palladium.conf`:

```ini
server=1
zmqpubhashblock=tcp://0.0.0.0:28332
rpcauth=<the line printed for "pool">
fallbackfee=0.0001
```

Steps:

1. Start the node and wait for the sync (section 5).
2. Create the wallet once:

   ```bash
   docker compose exec palladiumd palladium-cli createwallet pool
   ```

   Then add `wallet=pool` to the file, so that it is loaded at every start (a `wallet=` line for a wallet that does not
   exist yet stops the node from starting), and `docker compose restart`.
3. **Back the wallet up now** and after every change of address use (section 8). The pool pays its miners from it.
4. Point the pool at `http://palladiumd:2332` with its `rpcauth` user, and at `tcp://palladiumd:28332` for the new
   blocks (section 9 shows the compose side).
5. The pool asks for work with `getblocktemplate` and the rule `segwit` (`{"rules": ["segwit"]}`: the node refuses
   the call without it) and submits the solved block with `submitblock`. The address that receives the block reward is
   set in the pool's own configuration (it ends up in the coinbase of the template), not in the node.
6. Open the stratum port of the pool (section 7), not 2332.

`txindex` is not needed. Peers matter: a pool that is slow to learn of a new block mines on a stale one, so keep
`maxconnections` high enough, 2333 open for inbound peers, and the ZMQ `hashblock` connected.

### 6.2 Block explorer

`palladium.conf`:

```ini
server=1
txindex=1
zmqpubhashblock=tcp://0.0.0.0:28332
zmqpubrawtx=tcp://0.0.0.0:28335     # only for a live mempool page
rest=1                              # only if the explorer reads blocks over REST
rpcauth=<the line printed for "explorer">
disablewallet=1                     # if no other service of yours uses the wallet
```

- Needs `txindex=1` and no pruning. The index build is the long part of the first start.
- `rest=1` has **no authentication**; it is on the RPC port (2332), which is only on the `palladium` network.
- The RPC user can be restricted with the whitelist in section 4: an explorer never needs `sendtoaddress` or a wallet.
- The explorer has its own database: give it its own volume. Open its HTTP(S) port (section 7), not 2332.

### 6.3 Electrum server (Fulcrum / ElectrumX)

`palladium.conf`:

```ini
server=1
txindex=1
zmqpubhashblock=tcp://0.0.0.0:28332
rpcauth=<the line printed for "fulcrum">
disablewallet=1
```

- Needs `server=1`, `txindex=1`, no pruning, and an RPC user. The ZMQ `hashblock` is recommended: it picks up new
  blocks without polling.
- In the server's configuration the node is `palladiumd:2332` with that user and password. The server keeps its own
  database (give it its own volume) and builds it from the node on the first start.
- Public ports: 50001 (TCP) and 50002 (SSL), the usual ones.
- **Not verified.** These servers recognize a chain by its genesis hash and by the address and message formats of the
  coin, and Electrum wallets need their own build with the Palladium parameters (address prefixes, bech32 `plm`,
  ports). Check that the version you choose supports Palladium, or can be given its parameters, before relying on it.

### 6.4 More than one service

Take the union of the lines above and keep **one `rpcauth` user per service**:

| You run | Minimum in `palladium.conf` |
| --- | --- |
| A pool and an explorer | `server=1`, `txindex=1`, `zmqpubhashblock`, `zmqpubrawtx`, one `rpcauth` each, `wallet`, `fallbackfee` |
| An Electrum server and an explorer | `server=1`, `txindex=1`, `zmqpubhashblock`, `zmqpubrawtx`, one `rpcauth` each |
| All three | the union: `server=1`, `txindex=1`, the three `zmqpub*`, one `rpcauth` per service, `wallet` |

The wallet stays enabled (no `disablewallet=1`) only if the pool is there. The services share the one node and its RPC
queue, so keep `rpcthreads` and `rpcworkqueue` as in section 3.

## 7. Ports and firewall

A port is a numbered door of the server. Opening a door to the internet means that anybody can knock on it, so open
only the ones that have to be open. The node uses three kinds:

| Port | What | Open to | Published by `docker-compose.yml` |
| --- | --- | --- | --- |
| **2333/tcp** | P2P (mainnet) | the internet, **inbound** | yes |
| 2332/tcp | RPC (and REST) | only the `palladium` network | no |
| 28332, 28334, 28335/tcp | ZMQ (hashblock, rawblock, rawtx) | only the `palladium` network | no |

- **Only 2333 has to be opened on the server's firewall and router.** It is for *inbound* connections, the ones that
  other nodes start toward yours. Without it the node still works (it connects to others itself), but it has fewer
  peers and does not help the network. A pool should have it open: more peers mean learning of new blocks sooner.
  Where you open it depends on where the server is: on a **VPS or a server with a public address**, in the firewall of
  the provider and in the firewall of the server (`sudo ufw allow 2333/tcp` on Ubuntu); on a **server at home or in an
  office**, behind a router, by forwarding the port (see below).
- **Do not open or publish 2332 and the ZMQ ports.** The RPC can send coins from the wallet, and ZMQ has no
  authentication. The other containers reach them by name (`palladiumd:2332`) on the `palladium` network, with no
  `ports:` needed.
- The services you add have their own ports, and only these go in their `ports:` and in the firewall: for example the
  stratum port of a pool (the one your pool software uses), the HTTP(S) port of an explorer, 50001/50002 for an
  Electrum server.
- Docker publishes `ports:` by writing its own firewall rules, which **bypass `ufw` and `firewalld`**: a port
  published in the compose file is open to the internet even if `ufw` denies it. To keep a published port private, bind
  it to an address: `"127.0.0.1:8080:8080"`.

### Port forwarding on the router

Only for a server behind a router (a server with a public IP, e.g. a VPS, opens the port in its firewall instead).
The router connects your local network to the internet and, by default, drops what comes in from outside. In its
administration page (often called "Port forwarding" or "Virtual server") you tell it to send a port to the server. First
give the server a fixed address on the local network (a "DHCP reservation" in the router, or a static address), or the
forward breaks when the address changes. Then forward to it:

| External port | Protocol | To | Needed for |
| --- | --- | --- | --- |
| **2333** | TCP | `<server LAN IP>:2333` | the node: always |
| the stratum port | TCP | `<server LAN IP>:<port>` | a pool, if miners connect from the internet |
| 80 / 443 | TCP | `<server LAN IP>:<port>` | an explorer for the public |
| 50001 / 50002 | TCP | `<server LAN IP>:<port>` | an Electrum server for the public |

Never forward 2332 or 28332/28334/28335. A service that only you (or your other servers) use needs no forward.

### Check the ports

On the server (the compose file must show only 2333 for the node):

```bash
docker compose ps                    # PORTS column: 0.0.0.0:2333->2333/tcp and nothing else
docker port docker-palladiumcore     # same, one line per published port
sudo ss -ltnp | grep -E ':(2332|2333|2833[245])\b'     # 2333 yes; 2332 and 2833x must NOT be there
```

From **another machine outside your network** (a test from inside the LAN says nothing about the forward; a phone
on mobile data with a terminal app works), with the public address of the server (`curl -4 ifconfig.me` on the server):

```bash
nc -zv <public IP> 2333              # "succeeded" / "open": the forward and the firewall work
nc -zv <public IP> 2332              # must say "refused" or time out
```

That the node is actually reachable shows up in the node itself: after a few hours it must have inbound peers.

```bash
docker compose exec palladiumd palladium-cli getnetworkinfo | grep -A6 localaddresses   # the address other nodes learn
docker compose exec palladiumd palladium-cli getpeerinfo | grep -c '"inbound": true'    # > 0: others connect to you
```

If `nc` from outside fails but `ss` shows 2333: the router forward (or the firewall of the provider) is the problem.
If `getpeerinfo` shows no inbound peers after a day while `nc` succeeds, check `listen=1` in `palladium.conf` and,
behind NAT, `externalip=<public IP>`. For services that are not run by Docker, open the port in `ufw` too:
`sudo ufw allow 2333/tcp`.

## 8. Operations

**Logs and state**

```bash
docker compose logs -f palladiumd
docker compose exec palladiumd palladium-cli getblockchaininfo
docker compose exec palladiumd palladium-cli getpeerinfo
```

**Stop and restart.** `docker compose stop` is a clean shutdown: the node flushes the chainstate, which can take up to
2 minutes (`stop_grace_period`). Do not `kill` the container or power off the server while it runs: an unclean
shutdown can corrupt the database and force a reindex. After a change to `palladium.conf`: `docker compose restart`.

**Back up the wallet** (pool). If the server breaks and the wallet is lost, the coins in it are lost: the backup is
the only way back. The wallet is in `~/.palladium/wallets/`. Copying the folder while the node runs may give a broken
copy, so ask the node to write a consistent one:

```bash
docker compose exec palladiumd palladium-cli -rpcwallet=pool backupwallet /home/palladium/.palladium/pool-backup.dat
```

The file appears in `~/.palladium` on the host: move it off the server, and keep it private (it holds the keys). Repeat
it after you generate new addresses.

**Update to a new release.**

1. Read the release notes ([CHANGELOG.md](../CHANGELOG.md)): some releases need a reindex or have an incompatible change.
2. Set the new version in the `image:` line of `docker-compose.yml` (for example `...palladiumcore:2.1.1`). The part
   after the colon is the tag: the version of the image.
3. `docker compose pull && docker compose up -d`. `pull` downloads the new image, `up -d` stops the old container
   cleanly and starts a new one from it. The data directory is not touched, so the chain and the wallet stay.
4. Check the log and `getblockchaininfo`. If the new image does not start, put the old tag back.

The image is published as `ghcr.io/palladium-coin/palladiumcore:<version>` and `:latest`. In production use the
version, not `latest`: an update then happens when you decide it.

**Add `txindex=1` to a node that already has the chain.** Add the line and restart: the node builds the index without
downloading the chain again, and answers RPC meanwhile. Do not turn it on and off: dropping it means a rebuild.

**Disk full or a corrupted database.** Free space, then start the node. If the log says that the database needs a
reindex (the node rebuilds its databases from the blocks it has on disk), add `"-reindex"` to the `command:` list of the
service in `docker-compose.yml`, run `docker compose up -d`, and remove it again when it is done: it takes hours, and
with the option left in it would start over at every restart. Use it only when the log asks for it.

## 9. Connecting a service

A service in the same compose file (or in another one, with the network declared `external: true`, `name: palladium`):

```yaml
services:
  pool:
    image: ...
    networks: [palladium]
    environment:
      RPC_URL: http://palladiumd:2332
      ZMQ_HASHBLOCK: tcp://palladiumd:28332     # rawblock :28334, rawtx :28335
networks:
  palladium:
    name: palladium
```

`image: ...` stands for the image of your pool, explorer or Electrum server. `networks: [palladium]` is what lets it
reach the node by the name `palladiumd`, and the `networks:` block at the end says that this network is the one the
node already uses. Publish (`ports:`) only what must be reachable from outside. A quick test of the RPC from a
container on the network (the port is not published on the host), with the user and password of section 4:

```bash
docker run --rm --network palladium --entrypoint palladium-cli ghcr.io/palladium-coin/palladiumcore:2.1.0 \
  -rpcconnect=palladiumd -rpcport=2332 -rpcuser=<user> -rpcpassword=<password> getconnectioncount
```

## 10. Checklist before going live

- [ ] `palladium.conf` exists before the first start, with no `rpcuser=username` / `rpcpassword=password`.
- [ ] One `rpcauth` user per service, long passwords, `rpcwhitelist` if you use it (with `rpcwhitelistdefault=0`).
- [ ] `ss -ltnp` and `docker compose ps` show only 2333 (and the ports of your services) on the host; 2332 and the ZMQ ports
      are not reachable from outside.
- [ ] `nc -zv <public IP> 2333` works from outside, and `getpeerinfo` shows inbound peers after a few hours.
- [ ] The node is synchronized before the services are started.
- [ ] The wallet is backed up off the server (pool).
- [ ] Logs are capped, the clock is synchronized, Docker starts at boot, there is free disk.
- [ ] The image tag is a version, not `latest`.

## Appendix A: what each option does

| Option | Mining pool | Explorer | Electrum server | Why |
| --- | --- | --- | --- | --- |
| `server=1` | yes | yes | yes | enables the RPC server |
| `txindex=1` | no | **yes** | **yes** | look up any transaction by id; cannot be used with `prune` |
| `prune=0` (the default) | | **yes** | **yes** | these services read the whole chain; pruning is incompatible with `txindex` |
| `zmqpubhashblock` | yes | yes | recommended | new-block notification, to refresh the work or the index without polling |
| `zmqpubrawblock` | | optional | | the whole block, saves a `getblock` call |
| `zmqpubrawtx` | | optional | | transactions as they enter the mempool (live mempool) |
| `rest=1` | | optional | | read-only HTTP API on the RPC port (`/rest/block/<hash>.json`, ...); it has **no authentication** |
| wallet | **yes**, for payouts | no | no | the pool pays with `sendtoaddress`/`sendmany` |
| an `rpcauth` user of its own | yes | yes | yes | section 4 |

- **`server=1`**: without it the node does not start the RPC server, and nothing can connect (not even
  `palladium-cli`). Every service needs it.
- **`txindex=1`**: by default the node can look up a transaction by id only if it is in the mempool or if you also
  give the block it is in. With the index, `getrawtransaction <txid>` works for any transaction of the chain.
  Explorers need it to open a transaction page, Electrum servers to resolve the inputs of a transaction. A pool
  does not. Without it, those calls fail with an error that mentions `-txindex`. Costs: extra disk (it grows with the
  chain), and the index is built on the first start with the option, while the node is also running (hours on a full
  chain). If you add it to a node that already has the chain, the node rescans the blocks; it does not download them
  again.
- **`prune`**: deletes old blocks to save disk. Leave it unset (`prune=0`). The node refuses to start with `prune`
  and `txindex` together, and an explorer or an Electrum server needs the old blocks anyway.
- **`zmqpubhashblock`**: the node publishes the hash of every new block as soon as it is connected. A service
  that gets it can ask for the new block or refresh its work immediately, instead of calling `getblockcount` every few
  seconds. For a pool it is what keeps miners from working on an old block. If a service does not use ZMQ, it
  polls the RPC, and the option can stay in the file without harm.
- **`zmqpubrawblock`**: publishes the whole block, not just the hash. Useful for a service that indexes every block:
  it saves a `getblock` call. Nobody needs it to work.
- **`zmqpubrawtx`**: publishes every transaction that enters the mempool (and every one in a new block). It is what
  an explorer needs to show unconfirmed transactions live. It is a continuous stream: leave it off if no service
  uses it.
- **`rest=1`**: turns on a read-only HTTP interface on the RPC port (`/rest/block/<hash>.json`,
  `/rest/tx/<txid>.json`, ...). It needs no password, which is why it must stay on the `palladium` network. Only
  explorers that use REST need it.
- **wallet**: the pool pays its miners with `sendtoaddress`/`sendmany`, so the node must have a wallet loaded (section
  6.1). Explorers and Electrum servers only read the chain and do not need one: with no service using it,
  `disablewallet=1` removes the risk of keeping coins on the server.
- **an `rpcauth` user of its own**: see section 4.
- **`blockfilterindex=1`**: builds the compact block filters (BIP158) for light clients. None of the three
  services needs it. Leave it off unless the software you run asks for it.

Not available in this node (it is based on an older Bitcoin Core): `coinstatsindex`, `peerblockfilters`,
`mempoolfullrbf`, the `getindexinfo` RPC.

## Troubleshooting

- `docker compose ps` shows the container exited: `docker compose logs palladiumd`. The usual cause is the lock of
  the data directory (a node is still running on the host) or a wrong owner of `~/.palladium`.
- `Authorization failed: Incorrect rpcuser or rpcpassword` from `palladium-cli`: it reads the `palladium.conf` of
  `~/.palladium`, so the credentials in the file are the ones to use. If the file was written by the node on this very
  start, run `docker compose restart` (section 3).
- A service gets "RPC method is not whitelisted": the `rpcwhitelist` of its user does not list the method (section 4).
- `getblocktemplate` fails with "not connected" or "initial block download": the node has no peers yet or is not
  synchronized (section 5).
- An error that mentions `-txindex`: the service needs `txindex=1` (section 6).
