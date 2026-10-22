Release Process
===============

Palladium Core releases are built with the Docker system in [docker-build](../docker-build/README.md), which
gives the same binaries for the same commit, byte for byte. The release is the tag, the binaries built from it
and a signed `SHA256SUMS`.

## Before the release

* Make sure that CI is green on the commit to release (lint, unit and functional tests, fuzz targets).
* Review every change to consensus code since the last release (`src/consensus/`, `src/pow.cpp`,
  `src/validation.cpp`, `src/chainparams.cpp`): `git diff v<previous>..HEAD -- <path>`.
* Run the checks that CI does not: sync mainnet and testnet from scratch with the release binaries, a
  `-reindex-chainstate` with `-assumevalid=0`, and a basic wallet test (create, receive, send, restart).
* Update [bips.md](bips.md) to account for the changes since the last release.
* Write the `CHANGELOG.md` section of the new version (`git shortlog --no-merges v<previous>..HEAD` helps).

### Once per major and minor release, before the tag

Update the values in [`src/chainparams.cpp`](../src/chainparams.cpp) with a **fully synced** node:

* `m_assumed_blockchain_size` and `m_assumed_chain_state_size`: the space a user needs, see
  [below](#how-to-calculate-m_assumed_blockchain_size-and-m_assumed_chain_state_size).
* `chainTxData`: the output of the RPC `getchaintxstats`. Reviewers can check it by running
  `getchaintxstats <window_block_count> <window_last_block_hash>` with the values of your output.
* `nMinimumChainWork`: from `getblockchaininfo`.
* `defaultAssumeValid`: a block hash from `getblockhash`. It must not be orphaned, so take one two blocks back
  from the tip (testnet: some tens of thousands back, because of reorganizations). Review this value with a
  `-reindex-chainstate` with `-assumevalid=0`, to catch any defect that rejects blocks of the past.

Update the hardcoded seeds when needed, see [contrib/seeds/README.md](../contrib/seeds/README.md).

## Version

```bash
./update_version.sh 2.1.1               # a final release
./update_version.sh --rc 1 2.2.0        # release candidate 2.2.0rc1
./update_version.sh --dev 2.3.0         # a development version (CLIENT_VERSION_IS_RELEASE false)
```

Without the version it asks for it. It updates `configure.ac` (including `CLIENT_VERSION_RC` and
`CLIENT_VERSION_IS_RELEASE`), `build_msvc/palladium_config.h` and the version and the date of the five man pages,
then checks the result. Look at `git diff` and commit it (`build: bump the version to X.Y.Z`).

## Build

From a clean checkout of the commit to release:

```bash
cd docker-build
./check-reproducible.sh linux-x86_64     # builds twice and compares: must say [OK]
./check-reproducible.sh linux-aarch64
./check-reproducible.sh linux-armv7l
./check-reproducible.sh windows
./build.sh windows --installer           # the installer is not reproducible and is not checked
```

The binaries are in `build/<target>/`. See [docker-build/README.md](../docker-build/README.md) for the options.
Only committed files are built, and the commit must be the one that will be tagged.

Optionally, ask someone else to build the same commit on another machine and send their `SHA256SUMS`: they must
be identical to yours.

## Tag

```bash
git tag -s v<version> -m "Palladium Core <version>"
git push origin v<version>
```

## Publish

1. Make one archive per target from the binaries of `build/<target>/`, for example
   `palladium-<version>-x86_64-linux-gnu.tar.gz`, `palladium-<version>-win64.zip` and the installer
   `palladium-<version>-win64-setup.exe`.
2. Create `SHA256SUMS` with the checksums of the files to publish and sign it. Check that `SHA256SUMS` itself is
   not in the list.

```bash
sha256sum palladium-<version>-* > SHA256SUMS
gpg --digest-algo sha256 --clearsign SHA256SUMS     # outputs SHA256SUMS.asc
```

   The digest algorithm is forced to sha256 to avoid confusion between the `Hash:` header that GPG adds and the
   SHA256 of the files. Publish `SHA256SUMS` together with `SHA256SUMS.asc`; the unsigned binaries inside the
   archives are the ones whose hashes are in `docker-build`'s `SHA256SUMS`, so anyone can compare them.
3. Create the [GitHub release](https://github.com/palladium-coin/palladiumcore/releases/new) for the tag, with
   the archives, `SHA256SUMS`, `SHA256SUMS.asc` and a link to the `CHANGELOG.md` section.
4. Announce the release on the official channels and update the websites.

## Additional information

### How to calculate `m_assumed_blockchain_size` and `m_assumed_chain_state_size`

Both variables are used as a guideline for how much space the user needs on their drive in total, not just
strictly for the blockchain. All values must be taken from a **fully synced** node, with an overhead of 5-10%
added to the base value.

To calculate `m_assumed_blockchain_size`:
- for `mainnet`: the size of the data directory, excluding the `regtest` and `testnet` directories;
- for `testnet`: the size of the `testnet` directory.

To calculate `m_assumed_chain_state_size`:
- for `mainnet`: the size of the `chainstate` directory;
- for `testnet`: the size of the `testnet/chainstate` directory.

Notes:
- For `m_assumed_blockchain_size` there is no need to exclude the `chainstate` directory: it is a guideline and
  an overhead is added anyway.
- The overhead for growth may change over time: pay attention to it when changing the variables.
