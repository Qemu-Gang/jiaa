# jiaa
QEMU KVM cheat for the egg game. Powered by [memflow](https://github.com/memflow/memflow)

## Building from source

This is an in-tree build, don't be put off by all the folders, it's a plain C project.

#### build memflow
* `cargo build --release --workspace`
#### build jiaa
* `mkdir jiaa-build && cd jiaa-build`
* `cmake ../jiaa-src`
* `make`

## Basic usage

First you will want to ensure that [evdev-mirror](https://github.com/LWSS/evdev-mirror) and [peeper](https://github.com/LWSS/peeper) are running.

If this is your first time using memflow, make sure to install the [qemu_procfs](https://github.com/memflow/memflow-qemu-procfs) connector. ( Use install.sh )

After all that is done, you can finally run jiaa as root

`sudo ./jiaa`

## Features 
- ESP tracers
- Flight [CTRL - toggle altitude lock, SPACE - ascend]

## Media
https://streamable.com/7oa3ou

## Acknowledgements
- [UC Thread](https://www.unknowncheats.me/forum/other-fps-games/415582-diabotical-release-reversal-discussion.html)
- zZzeta/S - entity iteration and some struct data
- Finz Rus - some sigs and info on entities
- Frankie11 - w2s
#### memflow
- [CasualX](https://github.com/casualx/) for his wonderful pelite crate
- [ufrisk](https://github.com/ufrisk/) for his prior work on the subject and many inspirations