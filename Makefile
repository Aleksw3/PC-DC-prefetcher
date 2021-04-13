include ./scripts/FRAMEWORK

CXX=g++-4.8
CC=gcc

export

all: compile test

compile:
	./scripts/compile.sh

test:
	./scripts/test_prefetcher.py


clean:
	rm -Rf build
	rm -Rf output
	rm -Rf stats.txt

debug:
	export M5_CPU2000=$(pwd)/data/cpu2000
	#gdb --args ./build/ALPHA_SE/m5.debug --trace-flags=HWPrefetch --remote-gdb-port=0 -re --outdir=output/ammp-user /opt/prefetcher/m5/configs/example/se.py --checkpoint-dir=/opt/prefetcher/lib/cp --checkpoint-restore=1000000000 --at-instruction --caches --l2cache --standard-switch --warmup-insts=10000000 --max-inst=10000000 --l2size=1MB --bench=ammp --prefetcher=on_access=true:policy=proxy
	gdb --args ./build/ALPHA_SE/m5.opt --remote-gdb-port=0 -re --outdir=output/ammp-user /opt/prefetcher/m5/configs/example/se.py --checkpoint-dir=/opt/prefetcher/lib/cp --checkpoint-restore=1000000000 --at-instruction --caches --l2cache --standard-switch --warmup-insts=10000000 --max-inst=10000000 --l2size=1MB --bench=twolf-user--prefetcher=on_access=true:policy=proxy