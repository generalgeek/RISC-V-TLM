#include <systemc>
#include "Simulator.h"

int sc_main(int argc, char* argv[]) {
    /* Capture Ctrl+C and finish the simulation */
    signal(SIGINT, intHandler);

    /* SystemC time resolution set to 1 ns*/
    sc_core::sc_set_time_resolution(1, sc_core::SC_NS);

    Performance* perf = Performance::getInstance();
    Simulator* top = new Simulator("top", argc, argv);

    auto start = std::chrono::steady_clock::now();
    sc_core::sc_start();
    auto end = std::chrono::steady_clock::now();

    std::chrono::duration<double> elapsed_seconds = end - start;
    double instructions = static_cast<double>(perf->getInstructions()) / elapsed_seconds.count();

    std::cout << "Total elapsed time: " << elapsed_seconds.count() << "s" << std::endl;
    std::cout << "Simulated " << int(std::round(instructions)) << " instr/sec" << std::endl;

    if (!top->mem_dump) {
        std::cout << "Press Enter to finish" << std::endl;
        std::cin.ignore();
    }

    // call all destructors, clean exit.
    delete top;

    return 0;
}
