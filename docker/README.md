# Palladium node in Docker

Runs `palladiumd` from the official release binaries (amd64, arm64), as the backend for other containerized
services (mining pool, explorer, ...).

- The image downloads the release from GitHub **when it is built**, checks the signature of `SHA256SUMS.asc` against
  the pinned release key and the SHA256 of the archive, and keeps only `palladiumd` and `palladium-cli`. Nothing is
  downloaded when the container starts. The version is `ARG VERSION` in the [Dockerfile](Dockerfile).
- The data directory is the **`~/.palladium` of the host**, mounted into the container. The node and
  `palladium-cli` use the `palladium.conf` in it.
- RPC and ZMQ are reachable only from the containers on the `palladium` network. Only the P2P port 2333 is published.
- The image is published as `ghcr.io/palladium-coin/palladiumcore:<version>` (and `:latest`).

| File | What it is |
| --- | --- |
| [Dockerfile](Dockerfile) | the image |
| [docker-compose.yml](docker-compose.yml) | the node, from the published image |
| [docker-compose.build.yml](docker-compose.build.yml) | added on top of the one above to build the image locally |

## Where to start

| You want to | Read |
| --- | --- |
| try the node on your computer | [Quick start](#quick-start) |
| run a node on a server, for a mining pool, an explorer or an Electrum server (ports, firewall, backups, updates) | **[doc/docker-production.md](../doc/docker-production.md)** |
| build the image yourself, for your machine or for several architectures | [Build the image](#build-the-image) |
| publish a new version of the image (maintainers of the project) | [Publish the image](#publish-the-image-maintainers) |

## Quick start

You need Docker with the Compose plugin ([installation](https://docs.docker.com/engine/install/)): `docker compose version`
must print a version. In a terminal, in this directory:

```bash
mkdir -p ~/.palladium && sudo chown 1000:1000 ~/.palladium     # the container runs as uid/gid 1000
cd docker
docker compose up -d               # pulls ghcr.io/palladium-coin/palladiumcore
docker compose logs -f palladiumd  # Ctrl+C leaves the node running
docker compose stop                # clean shutdown (up to 2 minutes to flush the chainstate)
```

- The first line creates the data directory, where the node keeps the blockchain and its settings, and gives it to the
  user number 1000, the one the container runs as. On most single-user servers that is you (`id -u` prints your number;
  if it is not 1000, use the next point).
- `docker compose up -d` downloads the image and starts the node in the background. The first start downloads the
  whole blockchain, which takes hours. `docker compose logs -f palladiumd` shows what it is doing.
- Stop `palladiumd` / `palladium-qt` on the host first: two nodes cannot use the same directory.
- Another owner of the directory: `PLM_UID=$(id -u) PLM_GID=$(id -g) docker compose up -d`. Another directory:
  `PALLADIUM_DATADIR=/path docker compose up -d`.
- With an empty directory the node writes `palladium.conf` (its settings file) on its first start, and `palladium-cli`
  fails with `Incorrect rpcuser or rpcpassword` until `docker compose restart`. For anything but a try, create the file
  first ([doc/docker-production.md](../doc/docker-production.md#3-the-data-directory-and-palladiumconf)).

Without compose:

```bash
docker run -d --name docker-palladiumcore --restart unless-stopped -v ~/.palladium:/home/palladium/.palladium -p 2333:2333 ghcr.io/palladium-coin/palladiumcore:2.1.0
docker exec docker-palladiumcore palladium-cli getblockchaininfo
```

Is it connected?

```bash
docker compose exec palladiumd palladium-cli getconnectioncount     # > 0: it has peers
docker compose exec palladiumd palladium-cli getblockchaininfo      # synced: blocks == headers, verificationprogress ~ 1
```

## Build the image

You only need this to change the image or to try it before it is published: to run the node, the image is pulled
automatically (Quick start). The commands run in this directory. `docker buildx version` must print a version
(`buildx` is the Docker builder that can also build for other kinds of CPU).

**For your machine**, with compose. The image is tagged `palladiumd:dev`, so it never replaces the pulled one:

```bash
docker compose -f docker-compose.yml -f docker-compose.build.yml build            # only build
docker compose -f docker-compose.yml -f docker-compose.build.yml up -d --build     # build and start
```

Or without compose (from the root of the repository: `docker build -t palladiumd:dev docker`):

```bash
docker build -t palladiumd:dev .
```

The build downloads the release and checks the signature and the checksums: if it finishes, they passed. Try it:

```bash
docker run --rm palladiumd:dev --version                                   # palladiumd
docker run --rm --entrypoint palladium-cli palladiumd:dev --version
```

**Options** (`--build-arg NAME=value`, valid in every command here):

| Argument | Default | What |
| --- | --- | --- |
| `VERSION` | the one in the Dockerfile | the release to download, e.g. `2.1.1` |
| `RELEASE_FPR` | the pinned fingerprint | fingerprint of the key that signs `SHA256SUMS.asc` |

`--no-cache` downloads and verifies the release again.

### Multi-architecture (amd64 and arm64)

The architecture is the kind of CPU the image is for: **amd64** is the usual PC or server (Intel, AMD); **arm64** is
64-bit ARM (Raspberry Pi 4 and 5 with a 64-bit system, Apple Silicon, some cloud servers). A multi-architecture image
holds both under one name, and Docker pulls the one that fits the machine. 32-bit systems are not supported.

Once, per machine:

```bash
docker run --privileged --rm tonistiigi/binfmt --install all        # emulation, to build arm64 on amd64 (and back)
docker buildx create --name multiarch --driver docker-container --use
docker buildx inspect --bootstrap                                   # must list linux/amd64 and linux/arm64
```

(`binfmt` does not survive a reboot of WSL: run it again if an arm64 step fails with `exec format error`.)

Check that both build, without keeping the result:

```bash
docker buildx build --platform linux/amd64,linux/arm64 .
```

With this command the result stays in the build cache of Docker: it is not an image that you can start. The local
Docker cannot hold a multi-architecture image (`--load` works for one architecture only), so to start one, build it on
its own (arm64 runs under emulation on an amd64 machine: slow, but enough to see it start):

```bash
docker buildx build --platform linux/arm64 -t palladiumd:arm64test --load .
docker run --rm --platform linux/arm64 palladiumd:arm64test --version
```

The multi-architecture image itself exists only in a registry (a server that stores images: here `ghcr.io`, the one of
GitHub), so it is created when you publish it, see the next section. To try it without publishing, you can run a local
registry (`docker run -d -p 5000:5000 --name registry registry:2`, then build with `--push` and the name
`localhost:5000/palladiumd:dev`; the builder needs `--driver-opt network=host` at `docker buildx create` to reach it).

## Publish the image (maintainers)

Only for who publishes the official image of a release, and needs write access to the `palladium-coin` organization on
GitHub. The user of the node does not do this.

1. Create a classic Personal Access Token with `write:packages` (GitHub then adds the `repo` scopes too; give it a short
   expiry, authorize it for SSO if the organization asks) and log in:

   ```bash
   echo "<token>" | docker login ghcr.io -u <github user> --password-stdin
   ```

2. Build for both architectures and push, with the version and `latest`:

   ```bash
   docker buildx build --platform linux/amd64,linux/arm64 \
     -t ghcr.io/palladium-coin/palladiumcore:2.1.0 -t ghcr.io/palladium-coin/palladiumcore:latest --push .
   ```

3. Check that the image has both architectures (the two `unknown/unknown` entries are not architectures, they are
   records that Docker attaches about how it was built):

   ```bash
   docker buildx imagetools inspect ghcr.io/palladium-coin/palladiumcore:2.1.0 | grep Platform
   ```

4. The first time, the package is private. On GitHub: the organization → Packages → `palladiumcore` → Package settings →
   Change visibility → Public, and "Connect repository" to `palladiumcore`.
5. Log out (`docker logout ghcr.io`: the token is stored unencrypted in `~/.docker/config.json`), delete the token on
   GitHub, and test the pull without login: `docker run --rm ghcr.io/palladium-coin/palladiumcore:2.1.0 --version`.

A tag can be pushed again to overwrite it; a wrong version of the package can be deleted from its page on GitHub.

**A new release `x.y.z`:** publish the release and its `SHA256SUMS.asc` on GitHub first (the build downloads them), then
build with `--build-arg VERSION=x.y.z`, push as above with the tags `x.y.z` and `latest`, change `VERSION` in the
Dockerfile and the tag in `docker-compose.yml` and in the READMEs. The signing key is pinned by `RELEASE_FPR`.

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

`image: ...` is the image of your service. `networks: [palladium]` lets it reach the node by the name `palladiumd`.
Publish (`ports:`) only the ports of the service that must be reachable from outside (e.g. the stratum port of a pool),
never the RPC. How to create the RPC user and password, and which options each service needs, is in
[doc/docker-production.md](../doc/docker-production.md#4-rpc-users).

## Troubleshooting

- `docker compose ps` shows the container exited: `docker compose logs palladiumd`. The usual cause is the lock of
  the data directory (a node is still running on the host) or a wrong owner of `~/.palladium`.
- `Authorization failed: Incorrect rpcuser or rpcpassword` from `palladium-cli`: it reads the `palladium.conf` of
  `~/.palladium`, so the credentials in the file are the ones to use; if the file was just written by the node, run
  `docker compose restart`.
- `denied` when pushing: the token lacks `write:packages`, or is not authorized for the organization's SSO.
- More in [doc/docker-production.md](../doc/docker-production.md#troubleshooting).
