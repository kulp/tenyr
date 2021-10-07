# delete all build products built by a rule that exits nonzero
.DELETE_ON_ERROR:

ifneq ($V,1)
.SILENT:
endif

clean:
	cmake --build build --target clean

.DEFAULT_GOAL = all

################################################################################
all:
	cmake -S . -B build -DJIT=${JIT} -DSDL=${SDL} -DICARUS=${ICARUS}
	cmake --build build

check: all
	cmake -S . -B build -DJIT=${JIT} -DSDL=${SDL} -DICARUS=${ICARUS} -DTESTING=1
	cmake --build build
	export PATH=$(abspath .):$$PATH && cd build && ctest --rerun-failed --output-on-failure
