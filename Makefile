# Packet router verification -- run inside WSL2 Ubuntu (or any Linux with Verilator 5).
#   make build | make quick | make full | make dash | make clean
#   make one TEST=hotspot_test SEED=1234     # one test, one seed (waves: add TRACE=1)
#   make test                                # pytest for the coverage-merge math
#   make bugs                                # prove each injected RTL bug is caught
#   make lint                                # verilator --lint-only -Wall

PY    ?= python3
JOBS  ?= $(shell nproc)
TEST  ?= smoke_test
SEED  ?= 1
RTL    = rtl/router_pkg.sv rtl/sync_fifo.sv rtl/in_port.sv rtl/rr_arbiter.sv \
         rtl/out_port.sv rtl/router_top.sv rtl/sva/router_sva.sv rtl/sva/bind_router.sv

.PHONY: build lint quick full one dash test bugs clean

build:
	$(PY) scripts/regress.py --build-only

lint:
	verilator --lint-only -Wall --top-module router_top $(RTL)

quick:
	$(PY) scripts/regress.py --suite quick

full:
	$(PY) scripts/regress.py --suite full --jobs $(JOBS) --timeout 300

one: build
	mkdir -p out/one/$(TEST)_$(SEED)
	./obj_dir/Vrouter_tb +TEST=$(TEST) +SEED=$(SEED) +verilator+seed+$(SEED) +verilator+rand+reset+2 \
	    +OUT=out/one/$(TEST)_$(SEED) $(if $(TRACE),+TRACE,)

dash:
	$(PY) scripts/dashboard.py out/latest

test:
	$(PY) -m pytest -q scripts/tests

bugs:
	$(PY) scripts/inject_bugs.py

clean:
	rm -rf obj_dir obj_dir_* out
