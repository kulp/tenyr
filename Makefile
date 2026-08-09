# delete all build products built by a rule that exits nonzero
.DELETE_ON_ERROR:

ifneq ($V,1)
.SILENT:
endif

clean:
	cmake --build build --target clean
	rm -rf build-wasm

.DEFAULT_GOAL = all

################################################################################
all:
	cmake -S . -B build -DJIT=${JIT} -DSDL=${SDL} -DICARUS=${ICARUS}
	cmake --build build

# Build all three tools (tas, tld, tsim) for the web browser via Emscripten.
# Produces build-wasm/src/{tas,tld,tsim}.{js,wasm} in auto-run mode (no
# MODULARIZE).  tas/tld can be invoked from the command line; tsim uses
# noInitialRun in the browser and is controlled via callMain from app.js.
wasm-wasm:
	emcmake cmake -S . -B build-wasm -DSDL=0 -DJIT=0 -DICARUS=0 -DBUILD_EXAMPLES=OFF
	cmake --build build-wasm -j

# Assemble and link demo programs using the wasm tas/tld tools (Node.js).
wasm-tools: wasm-wasm
	node build-wasm/src/tas.js -f obj -o ui/web/hello.obj ex/hello.tas
	node build-wasm/src/tas.js -f obj -o ui/web/puts.obj lib/puts.tas
	node build-wasm/src/tld.js -o ui/web/hello.bin ui/web/hello.obj ui/web/puts.obj
	node build-wasm/src/tas.js -f obj -o ui/web/fib.obj ex/fib.tas
	node build-wasm/src/tld.js -o ui/web/fib.bin ui/web/fib.obj
	cp build-wasm/src/tsim.js ui/web/
	cp build-wasm/src/tsim.wasm ui/web/
	rm -f ui/web/hello.obj ui/web/puts.obj ui/web/fib.obj

check: all
	cmake -S . -B build -DJIT=${JIT} -DSDL=${SDL} -DICARUS=${ICARUS} -DTESTING=1
	cmake --build build
	export PATH=$(abspath .):$$PATH && cd build && ctest --output-on-failure

# Use CMAKE_BUILD_TYPE=Debug for coverage to avoid glitchy optimizations.
coverage: CMAKE_BUILD_TYPE = Debug
coverage:
	cmake -S . -B build -DJIT=${JIT} -DSDL=${SDL} -DICARUS=${ICARUS} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCOVERAGE=1
	cmake --build build --target all
	cmake -S . -B build -DJIT=${JIT} -DSDL=${SDL} -DICARUS=${ICARUS} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DTESTING=1 -DCOVERAGE=1
	cmake --build build --target $@
