# Fuzzing Palladium Core using libFuzzer

## Quickstart guide

To quickly get started fuzzing Palladium Core using [libFuzzer](https://llvm.org/docs/LibFuzzer.html):

```sh
$ git clone https://github.com/palladium/palladium
$ cd palladium/
$ ./autogen.sh
$ CC=clang CXX=clang++ ./configure --enable-fuzz --with-sanitizers=address,fuzzer,undefined
# macOS users: If you have problem with this step then make sure to read "macOS hints for
# libFuzzer" on https://github.com/palladium/palladium/blob/master/doc/fuzzing.md#macos-hints-for-libfuzzer
$ make
$ mkdir -p corpus/pow
$ src/test/fuzz/pow corpus/pow
# abort fuzzing using ctrl-c
```

`--enable-fuzz` disables every other target (`palladiumd`, the wallet, the GUI, the unit tests),
so use a separate build directory or checkout for fuzzing.

## Fuzzing harnesses

Each harness is one binary in [`src/test/fuzz/`](../src/test/fuzz). It defines
`test_one_input()` (and optionally `initialize()`), and [`fuzz.cpp`](../src/test/fuzz/fuzz.cpp)
connects it to libFuzzer. A harness must never crash or fail an `assert` for any input:
an abort is a finding.

Palladium-specific harnesses:

| Target | What it checks |
|--------|----------------|
| `pow` | `GetNextWorkRequired` on fuzzed chains on mainnet, testnet and regtest, around the difficulty reset (28930-28999), the LWMA start (29000) and the Resilience fork (`nResilienceForkHeight`). The result is deterministic, valid and never above the pow limit; regtest difficulty stays fixed. |
| `taproot_verify` | `VerifyScript` on witness v1 outputs: arbitrary programs and witnesses (control block, annex), and valid outputs for fuzzed tapscripts. Deterministic for any flags, and without `SCRIPT_VERIFY_TAPROOT` a v1 program is spendable by anyone. |
| `taproot_tweak` | BIP341 output key tweaking: a key path signature made with `SignSchnorrTaproot` verifies against the output key from `CreatePayToTaprootPubKey`, and not for another message. |
| `key_io` | Address and key decoding on all networks (base58, bech32, bech32m; `plm`, `tplm`, `rplm`) and the encode/decode round trip. |

`process_message` sends one network message of a known type, with fuzzed contents, to a connected peer of a regtest node (`ProcessMessage`).

Other harnesses: `bech32`, `base_encode_decode`, `hex`, `decode_tx`, `transaction`, `block`,
`eval_script` (legacy, segwit v0 and tapscript), `parse_script`, `descriptor_parse`, `psbt`.
Also `netaddress`, `net_permissions`, `p2p_transport_deserializer` (the wire format deserializer), `parse_numbers`,
`parse_iso8601`, `parse_hd_keypath`, `spanparsing`, `fee_rate` and `bloom_filter`.

`*_deserialize` harnesses deserialize one type each and check that serializing and deserializing
again gives the same bytes (block, blockheader, blocklocator, blockundo, txundo, block_file_info, blocktransactions, blocktransactionsrequest, prefilled_transaction, block_header_and_short_txids, bloomfilter, coins, flat_file_pos, inv, merkle_block, partial_merkle_tree, partially_signed_transaction, psbt_input, psbt_output, pub_key, script, tx_in, out_point, txoutcompressor, uint160, uint256, netaddr, service, address, sub_net, banentry). Coins and undo data
(`coins`, `blockundo`, `txundo`) are only checked for not crashing, because several compressed
encodings give the same script.

To add one: create `src/test/fuzz/<name>.cpp`, add `test/fuzz/<name>` to `FUZZ_TARGETS` and its
`test_fuzz_<name>_*` block in [`src/Makefile.test.include`](../src/Makefile.test.include).
`test/fuzz/test_runner.py` finds the targets from that list.

[`FuzzedDataProvider.h`](../src/test/fuzz/FuzzedDataProvider.h) comes from the LLVM project
(Apache-2.0 with LLVM exception).

## Fuzzing output and corpora

The fuzzer prints `NEW` every time it creates an input that covers new code. For more information
on how to read the output, see the [libFuzzer documentation](https://llvm.org/docs/LibFuzzer.html).

If you give a corpus directory, inputs that increase coverage are saved there, and a crashing
input is written to the current directory (use `-artifact_prefix=` to change it). To replay one:

```sh
$ src/test/fuzz/pow crash-1234abcd
```

## Running all harnesses once

```sh
$ test/fuzz/test_runner.py test/fuzz/corpus            # all targets, each over corpus/<target>/
$ test/fuzz/test_runner.py test/fuzz/corpus pow key_io # selected targets
```

[`test/fuzz/corpus/`](../test/fuzz/corpus) has a small seed corpus (about 70 KB) for `pow`, `taproot_verify`,
`taproot_tweak` and `key_io`. Targets without one are listed and run on empty input. To refresh a corpus
after fuzzing, merge the new inputs into it: `src/test/fuzz/pow -merge=1 test/fuzz/corpus/pow <new dir>`.

## macOS hints for libFuzzer

The default Clang/LLVM version supplied by Apple on macOS does not include fuzzing libraries,
so install a full LLVM (for example with Homebrew) and point `CC`, `CXX` and `LDFLAGS` to it
before running `./configure`.
