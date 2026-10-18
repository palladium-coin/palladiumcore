Palladium Core
=============

Setup
---------------------
Palladium Core is the original Palladium client and it builds the backbone of the network. It downloads and, by default, stores the entire history of Palladium transactions, which requires a few hundred gigabytes of disk space. Depending on the speed of your computer and network connection, the synchronization process can take anywhere from a few hours to a day or more.

To download Palladium Core, visit [palladiumcore.org]([https://github.com/NotRin7/Palladium/releases/).

Running
---------------------
The following are some helpful notes on how to run Palladium Core on your native platform.

### Unix

Unpack the files into a directory and run:

- `bin/palladium-qt` (GUI) or
- `bin/palladiumd` (headless)

### Windows

Unpack the files into a directory, and then run palladium-qt.exe.

### macOS

Drag Palladium Core to your applications folder, and then run Palladium Core.

### Need Help?

* Ask for help on [Discord]([https://discord.com/invite/z2T8qvzeGc]).

Building
---------------------
The following are developer notes on how to build Palladium Core on your native platform. They are not complete guides, but include notes on the necessary libraries, compile flags, etc.

- [Dependencies](dependencies.md)
- [macOS Build Notes](build-osx.md)
- [Unix Build Notes](build-unix.md)
- [Windows Build Notes](build-windows.md)
- [FreeBSD Build Notes](build-freebsd.md)
- [OpenBSD Build Notes](build-openbsd.md)
- [NetBSD Build Notes](build-netbsd.md)
- [Gitian Building Guide (External Link)](https://github.com/palladium-core/docs/blob/master/gitian-building.md)

Development
---------------------
The Palladium repo's [root README](/README.md) contains relevant information on the development process and automated testing.

- [Developer Notes](developer-notes.md)
- [Productivity Notes](productivity.md)
- [Release Notes](release-notes/)
- [Release Process](release-process.md)
- [Source Code Documentation (External Link)](https://doxygen.palladiumcore.org/)
- [Translation Process](translation_process.md)
- [Translation Strings Policy](translation_strings_policy.md)
- [JSON-RPC Interface](JSON-RPC-interface.md)
- [Unauthenticated REST Interface](REST-interface.md)
- [Shared Libraries](shared-libraries.md)
- [BIPS](bips.md)
- [Dnsseed Policy](dnsseed-policy.md)
- [Benchmarking](benchmarking.md)

### Resources
* Discuss on the [PalladiumTalk](https://palladiumtalk.org/) forums, in the [Development & Technical Discussion board](https://palladiumtalk.org/index.php?board=6.0).
* Discuss project-specific development on #palladium-core-dev on Freenode. If you don't have an IRC client, use [webchat here](https://webchat.freenode.net/#palladium-core-dev).
* Discuss general Palladium development on #palladium-dev on Freenode. If you don't have an IRC client, use [webchat here](https://webchat.freenode.net/#palladium-dev).

### Miscellaneous
- [Assets Attribution](assets-attribution.md)
- [palladium.conf Configuration File](palladium-conf.md)
- [Files](files.md)
- [Fuzz-testing](fuzzing.md)
- [Reduce Memory](reduce-memory.md)
- [Reduce Traffic](reduce-traffic.md)
- [Tor Support](tor.md)
- [Init Scripts (systemd/upstart/openrc)](init.md)
- [ZMQ](zmq.md)
- [PSBT support](psbt.md)

License
---------------------
Distributed under the [MIT software license](/COPYING).
