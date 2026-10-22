# Palladium node in Docker

Runs `palladiumd` from the official release binaries (amd64, arm64, armv7l), as the backend for other containerized
services (mining pool, explorer, ...).

- The image downloads the release from GitHub **when it is built**, checks the signature of `SHA256SUMS.asc` against
  the pinned release key and the SHA256 of the archive, and keeps only `palladiumd` and `palladium-cli`. Nothing is
  downloaded when the container starts. The version is `ARG VERSION` in the [Dockerfile](Dockerfile).
- The data directory is the **`~/.palladium` of the host**, mounted into the container. The node and
  `palladium-cli` use the `palladium.conf` in it.
- RPC and ZMQ are reachable only from the containers on the `palladium` network. Only the P2P port 2333 is published.

## Setup of `~/.palladium`

1. Stop `palladiumd` / `palladium-qt` on the host: two nodes cannot use the same directory.
2. The directory must belong to the user the container runs as, which is uid/gid 1000 (the first user of most
   hosts). For another user: `PLM_UID=$(id -u) PLM_GID=$(id -g) docker compose up -d`.
3. Check `~/.palladium/palladium.conf`. It must contain at least:

   ```ini
   server=1
   txindex=1          # needed by explorers; the index is built on the first start and takes a while
   zmqpubhashblock=tcp://0.0.0.0:28332
   zmqpubrawblock=tcp://0.0.0.0:28334
   zmqpubrawtx=tcp://0.0.0.0:28335
   ```

   - `daemon=1` is overridden by the image (`-daemon=0`): a container must stay in the foreground.
   - `rpcbind` and `rpcallowip` can stay on `127.0.0.1`: the compose file adds `-rpcbind=0.0.0.0` and
     `-rpcallowip=172.16.0.0/12` (the networks of Docker) on the command line, and they are added to the ones in the file.
   - The file written by the node on its first start has `rpcuser=username` and `rpcpassword=password`. They are
     fine to try it, not for the services: generate an `rpcauth` line and replace them.

     ```bash
     python3 ../share/rpcauth/rpcauth.py pool       # prints the rpcauth= line for the file and the password
     ```

     Give the password to the service. Anything on the `palladium` network can try to log in, so use a long one.

## Configuration by service

All of this goes in `~/.palladium/palladium.conf` (restart the node after a change: `docker compose restart`).
Options at the top of the file apply to mainnet.

### What each service needs from the node

| Option | Mining pool | Explorer | Electrum server (Fulcrum, ...) | Why |
| --- | --- | --- | --- | --- |
| `server=1` | yes | yes | yes | enables the RPC server |
| `txindex=1` | no | **yes** | **yes** | look up any transaction by id; cannot be used with `prune` |
| `prune=0` (the default) | | **yes** | **yes** | these services read the whole chain; pruning is incompatible with `txindex` |
| `zmqpubhashblock` | yes | yes | recommended | new-block notification, to refresh the work or the index without polling |
| `zmqpubrawblock` | | optional | | the whole block, saves a `getblock` call |
| `zmqpubrawtx` | | optional | | transactions as they enter the mempool (live mempool) |
| `rest=1` | | optional | | read-only HTTP API on the RPC port (`/rest/block/<hash>.json`, ...); it has **no authentication** |
| wallet | **yes**, for payouts | no | no | see below |
| an `rpcauth` user of its own | yes | yes | yes | see "RPC users" |

#### What each option does

- **`server=1`**: without it the node does not start the RPC server, and nothing can connect (not even
  `palladium-cli`). Every service needs it.
- **`txindex=1`**: by default the node can look up a transaction by id only if it is in the mempool or if you also
  give the block it is in. With the index, `getrawtransaction <txid>` works for any transaction of the chain.
  Explorers need it to open a transaction page, Electrum servers to resolve the inputs of a transaction. A pool
  does not. Without it, those calls fail with an error that mentions `-txindex`. Costs: extra disk (it grows with the
  chain), and the index is built on the first start with the option, while the node is also running (hours on a full
  chain). If you add it to a node that already has the chain, the node rescans the blocks; it does not download them
  again. Do not turn it on and off: dropping it means a rebuild.
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
- **wallet**: the pool pays its miners with `sendtoaddress`/`sendmany`, so the node must have a wallet loaded (see
  "Mining pool" below). Explorers and Electrum servers only read the chain and do not need one: with no service using it,
  `disablewallet=1` removes the risk of keeping coins on the server.
- **an `rpcauth` user of its own**: the node writes a cookie in `~/.palladium/.cookie` that changes at every start;
  it is for `palladium-cli` on the same host. A service in another container cannot use it, so it needs a user
  and password. One user per service lets you change one password, or restrict one service, without touching the others.
- **`blockfilterindex=1`**: builds the compact block filters (BIP158) for light clients. None of the three
  services needs it. Leave it off unless the software you run asks for it.

#### Which options for which setup

| You run | Minimum in `palladium.conf` |
| --- | --- |
| Only the node (to sync, or to use `palladium-cli`) | nothing: the defaults |
| A pool | `server=1`, `zmqpubhashblock`, `rpcauth`, `wallet`, `fallbackfee`, a synchronized node |
| A pool and an explorer | the above + `txindex=1`, `zmqpubrawtx` |
| An Electrum server | `server=1`, `txindex=1`, `zmqpubhashblock`, `rpcauth` |
| All three | the union: `server=1`, `txindex=1`, the three `zmqpub*`, one `rpcauth` per service, `wallet` |

Not available in this node (it is based on an older Bitcoin Core): `coinstatsindex`, `peerblockfilters`,
`mempoolfullrbf`, the `getindexinfo` RPC. `blockfilterindex=1` exists, for services that ask for it.

### Common settings

```ini
server=1
listen=1                  # accept inbound peers; port 2333 is published by the compose file
maxconnections=40         # peers; the default is 125, the file written by the node uses 20
dbcache=2048              # MiB of cache. Large for the first sync or a reindex, then 512-1024 is enough
rpcthreads=16             # default 4: pools and indexers send many calls at once
rpcworkqueue=64           # default 16: calls waiting for a thread; above it the node answers with an error
rpcservertimeout=60

zmqpubhashblock=tcp://0.0.0.0:28332
zmqpubrawblock=tcp://0.0.0.0:28334
zmqpubrawtx=tcp://0.0.0.0:28335
```

- ZMQ is a one-way stream with no authentication. The container binds it on `0.0.0.0`, but it is not published on the
  host, so only the `palladium` network sees it. Do not add these ports to `ports:`.
- If the server is behind NAT or has several addresses, add `externalip=<public ip>` so that other nodes learn it.
- The node must be **synchronized and have peers** before a pool can work: `getblocktemplate` answers
  "Palladium is not connected!" or that the node is in initial block download otherwise. Check with `getblockchaininfo`.

### RPC users

Make one `rpcauth` line per service, so that a password can be changed or revoked on its own:

```bash
python3 ../share/rpcauth/rpcauth.py pool        # run it again for explorer, fulcrum, ...
```

Paste the `rpcauth=...` lines in the file and give each service its own password (it is printed only once).
Remove `rpcuser`/`rpcpassword` from the file: they are the defaults written by the node.

To restrict what a service can call, add `rpcwhitelist` lines:

```ini
rpcwhitelist=pool:getblocktemplate,submitblock,getblockchaininfo,getblockcount,getmininginfo,getnetworkinfo,getrawtransaction,validateaddress,sendtoaddress,getbalance
rpcwhitelist=explorer:getblockchaininfo,getblock,getblockhash,getblockcount,getrawtransaction,getrawmempool,getmempoolinfo,getmempoolentry,getnetworkinfo,getpeerinfo,gettxout,getchaintips,getmininginfo,estimatesmartfee
rpcwhitelistdefault=0
```

With `rpcwhitelistdefault=0` the users without a line are not restricted. **Without it, as soon as one `rpcwhitelist`
exists, every other user (and the cookie used by `palladium-cli` in the container) can call nothing.** The lists above
are a starting point, not tested against a real pool or explorer: if a service is refused a method, add it.

### Mining pool

- Needs `server=1`, the ZMQ `hashblock` (or `blocknotify=`), an `rpcauth` user, and a synchronized node.
- The pool asks for work with `getblocktemplate` and the rule `segwit` (`{"rules": ["segwit"]}`: the node refuses the
  call without it) and submits the solved block with `submitblock`.
- Payouts need a wallet in the node (the binaries include it; `disablewallet=1` turns it off). Create it once and
  keep a backup: it is in `~/.palladium/wallets/`.

  ```bash
  docker compose exec palladiumd palladium-cli createwallet pool
  ```

  Then add `wallet=pool` and `fallbackfee=0.0001` to the file, so that the wallet is loaded at every start.
- The address that receives the block reward is set in the pool's own configuration (it ends up in the coinbase of
  the template), not in the node.
- `txindex` is not needed. Peers matter: a pool that is slow to learn of a new block mines on a stale one, so keep
  `maxconnections` high enough and the ZMQ `hashblock` connected.

### Explorer

- Needs `txindex=1`. The first start builds the index: expect hours on a full chain; the node answers RPC while it
  does, and the log shows the progress.
- Use `zmqpubhashblock` (and `rawtx` for a mempool page) so that it does not poll. `rest=1` is enough for explorers
  that read blocks over REST: the REST port is the RPC one (2332), open to the `palladium` network only.
- The RPC user can be restricted with the whitelist above: an explorer never needs `sendtoaddress` or a wallet.
  `disablewallet=1` is fine if no service of yours uses the wallet.

### Electrum server (Fulcrum / ElectrumX)

- Needs `server=1`, `txindex=1`, no pruning, and an RPC user. The ZMQ `hashblock` is recommended: it picks up new
  blocks without polling.
- In the server's configuration the node is `palladiumd:2332` with that user and password. The server keeps its own
  database (give it its own volume) and builds it from the node on the first start.
- **Not verified.** These servers recognize a chain by its genesis hash and by the address and message formats of the
  coin, and Electrum wallets need their own build with the Palladium parameters (address prefixes, bech32 `plm`,
  ports). Check that the version you choose supports Palladium, or can be given its parameters, before relying on it.

## Ports

| Port | What | Open to | Published by `docker-compose.yml` |
| --- | --- | --- | --- |
| **2333/tcp** | P2P (mainnet) | the internet, **inbound** | yes |
| 2332/tcp | RPC (and REST) | only the `palladium` network | no |
| 28332, 28334, 28335/tcp | ZMQ (hashblock, rawblock, rawtx) | only the `palladium` network | no |

- **Only 2333 has to be opened on the server's firewall and router** (forward it to the host if it is behind NAT).
  Without it the node still works, with outbound connections only, but it has fewer peers and does not help the
  network. A pool should have it open: more peers mean learning of new blocks sooner.
- The node also needs **outbound** access to the other nodes on port 2333 (and DNS, for the seeds in
  `dnsseed.palladium-coin.*`).
- **Do not open or publish 2332 and the ZMQ ports.** The RPC can send coins from the wallet, and ZMQ has no
  authentication. The other containers reach them by name (`palladiumd:2332`) on the `palladium` network, with no
  `ports:` needed.
- The services you add have their own ports, and only these go in their `ports:` and in the firewall: for example the
  stratum port of a pool (the one your pool software uses), the HTTP(S) port of an explorer, 50001/50002 for an
  Electrum server (the usual ones for TCP and SSL).
- Docker publishes `ports:` by writing its own firewall rules, which **bypass `ufw` and `firewalld`**: a port
  published in the compose file is open to the internet even if `ufw` denies it. To keep a published port private, bind
  it to an address: `"127.0.0.1:8080:8080"`.

### Port forwarding on the router

Only for a server behind a router (a server with a public IP, e.g. a VPS, opens the port in its firewall instead).
Give the server a fixed LAN address (a DHCP reservation), then forward to it:

| External port | Protocol | To | Needed for |
| --- | --- | --- | --- |
| **2333** | TCP | `<server LAN IP>:2333` | the node: always |
| the stratum port | TCP | `<server LAN IP>:<port>` | a pool, if miners connect from the internet |
| 80 / 443 | TCP | `<server LAN IP>:<port>` | an explorer for the public |
| 50001 / 50002 | TCP | `<server LAN IP>:<port>` | an Electrum server for the public |

Never forward 2332 or 28332/28334/28335. A service that only you (or your other servers) use needs no forward.

### Check the ports

On the server, what the host is listening on (the compose file must show only 2333 for the node):

```bash
docker compose ps                    # PORTS column: 0.0.0.0:2333->2333/tcp and nothing else
docker port docker-palladiumd-1      # same, one line per published port
sudo ss -ltnp | grep -E ':(2332|2333|2833[245])\b'     # 2333 yes; 2332 and 2833x must NOT be there
```

From **another machine outside your network** (a test from inside the LAN says nothing about the forward; a phone
on mobile data with a terminal app works), replace `<public IP>` with the public address of the server (`curl -4 ifconfig.me` on the server):

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
If `getpeerinfo` shows no inbound peers after a day while `nc` succeeds, check `listen=1` in `palladium.conf`
and, behind NAT, `externalip=<public IP>`.

Opening the port in `ufw` on the host (it matters for the services that are not run by Docker, see the note above
on published ports):

```bash
sudo ufw allow 2333/tcp
```

## Run

```bash
cd docker
docker compose up -d --build       # --build only the first time or after changing the Dockerfile
docker compose logs -f palladiumd  # Ctrl+C leaves the node running
docker compose stop                # clean shutdown (up to 2 minutes to flush the chainstate)
```

Without compose:

```bash
docker build -t palladiumd .
docker run -d --name palladium --restart unless-stopped -v ~/.palladium:/home/palladium/.palladium -p 2333:2333 palladiumd
```

## Is it connected?

```bash
docker compose exec palladiumd palladium-cli getconnectioncount     # > 0: it has peers
docker compose exec palladiumd palladium-cli getblockchaininfo      # synced: blocks == headers, verificationprogress ~ 1
docker compose exec palladiumd palladium-cli getpeerinfo
```

From another container on the `palladium` network (the port is not published on the host):

```bash
curl --user <user>:<password> --data-binary '{"method":"getconnectioncount","id":1}' http://palladiumd:2332/
```

## Connecting a service

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

Publish only the ports of the service that must be reachable from outside (e.g. the stratum port of a pool).

## Troubleshooting

- `docker compose ps` shows the container exited: `docker compose logs palladiumd`. The usual cause is the lock of
  the data directory (a node is still running on the host) or a wrong owner of `~/.palladium`.
- `Authorization failed: Incorrect rpcuser or rpcpassword` from `palladium-cli`: it reads the `palladium.conf` of
  `~/.palladium`, so the credentials in the file are the ones to use.
- A new release: `docker compose build --build-arg VERSION=<x.y.z>`; the signing key is pinned by `RELEASE_FPR`.
