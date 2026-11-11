// router_defs.hpp -- DUT parameters mirrored for the testbench (spec 2.1).
// No Verilator includes: the reference model and packet class stay simulator-free.
#pragma once

constexpr int N_PORTS = 4;
constexpr int MAX_LEN = 31;
constexpr int FIFO_DEPTH = 64;
constexpr int CNT_W = 16;
