// main.cpp -- parses +args, builds Sim, runs the test, writes results.
//   ./Vrouter_tb +TEST=<name> +SEED=<n> [+OUT=<dir>] [+VERBOSITY=INFO] [+TRACE] [knobs...]
// Exit codes: 0 pass, 1 fail, 2 unknown test (R-TB-07, R-TB-09).
#include <chrono>
#include <cstdio>
#include <filesystem>

#include "base/logger.hpp"
#include "tests/base_test.hpp"

int main(int argc, char** argv) {
    PlusArgs args(argc, argv);
    const std::string test_name = args.str("TEST");
    const uint64_t seed = static_cast<uint64_t>(args.num("SEED", 1));
    const std::string out = args.str("OUT", ".");

    auto& reg = RouterTestRegistry::inst();
    if (!reg.has(test_name)) {
        std::printf("Unknown test '%s'. Registered tests:\n", test_name.c_str());
        for (const auto& n : reg.names()) std::printf("  %s\n", n.c_str());
        return 2;
    }
    std::filesystem::create_directories(out);

    Logger& log = Logger::get();
    log.set_verbosity(Logger::parse_level(args.str("VERBOSITY", "INFO")));

    const auto t0 = std::chrono::steady_clock::now();
    Rng rng(seed);  // R-TB-02: the only source of testbench randomness
    Sim sim(argc, argv, args.has("TRACE"), out);
    log.set_cycle_source(sim.cycle_ptr());

    TestContext ctx{&sim, &rng, &args, out, seed};
    std::unique_ptr<Test> test = reg.create(test_name, ctx);
    LOG_INFO("main", "running ", test_name, " seed=", seed);

    try {
        test->run_test();
    } catch (const FatalError& e) {
        std::printf("FATAL: %s\n", e.what());
    }
    sim.finish(out + "/coverage.dat");

    const double wall = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    const bool passed = log.errors() == 0;
    test->write_outputs(out, passed, wall);

    std::printf("errors=%d warnings=%d\n", log.errors(), log.warnings());
    std::printf("%s\n", passed ? "TEST PASSED" : "TEST FAILED");
    std::fflush(stdout);
    return passed ? 0 : 1;
}
