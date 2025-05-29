#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <chrono>
#include <csignal>
#include <cstdint>
#include <memory>
#include <systemc>
#include <unistd.h>
#include "BusCtrl.h"
#include "CPU.h"
#include "Debug.h"
#include "Timer.h"
#include "Trace.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

/**
 * @class Simulator
 * This class instantiates all necessary modules, connects its ports and starts
 * the simulation.
 *
 * @brief Top simulation entity
 */
class Simulator : sc_core::sc_module {
  public:
    Simulator(sc_core::sc_module_name name, int argc, char* argv[]);
    ~Simulator();

  private:
    void MemoryDump();
    void process_arguments(int argc, char* argv[]);

  private:
    riscv_tlm::CPU* cpu;
    riscv_tlm::Memory* MainMemory;
    riscv_tlm::BusCtrl* Bus;
    riscv_tlm::peripherals::Trace* trace;
    riscv_tlm::peripherals::Timer* timer;
    std::shared_ptr<spdlog::logger> logger;

  public:
    // params from cmd
    riscv_tlm::cpu_types_t cpu_type;
    std::string filename;
    bool debug_session;
    bool mem_dump;
    uint32_t dump_addr_st;
    uint32_t dump_addr_end;
};

void intHandler(int dummy);

#endif
