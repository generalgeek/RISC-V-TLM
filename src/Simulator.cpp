/*!
 \file Simulator.cpp
 \brief Top level simulation entity
 \author Màrius Montón
 \date September 2018
 */

#include "Simulator.h"

Simulator::Simulator(sc_core::sc_module_name name, int argc, char* argv[]) : sc_module(name) {
    // prase params from cmd
    // Parse and process program arguments. -f is mandatory
    process_arguments(argc, argv);

    MainMemory = new riscv_tlm::Memory("Main_Memory", filename);
    std::uint32_t start_PC = MainMemory->getPCfromHEX();

    if (cpu_type == riscv_tlm::RV32) {
        cpu = new riscv_tlm::CPURV32("cpu", start_PC, debug_session);
    } else {
        cpu = new riscv_tlm::CPURV64("cpu", start_PC, debug_session);
    }

    Bus = new riscv_tlm::BusCtrl("BusCtrl");
    trace = new riscv_tlm::peripherals::Trace("Trace");
    timer = new riscv_tlm::peripherals::Timer("Timer");

    cpu->instr_bus.bind(Bus->cpu_instr_socket);
    cpu->mem_intf->data_bus.bind(Bus->cpu_data_socket);

    Bus->memory_socket.bind(MainMemory->socket);
    Bus->trace_socket.bind(trace->socket);
    Bus->timer_socket.bind(timer->socket);

    timer->irq_line.bind(cpu->irq_line_socket);

    if (debug_session) {
        if (cpu_type == riscv_tlm::RV32) {
            riscv_tlm::Debug Debug(dynamic_cast<riscv_tlm::CPURV32*>(cpu), MainMemory);
        } else {
            riscv_tlm::Debug Debug(dynamic_cast<riscv_tlm::CPURV64*>(cpu), MainMemory);
        }
    }
    // init logger
    if (logger == nullptr) {
        spdlog::filename_t log_filename = SPDLOG_FILENAME_T("/dev/null");
        logger = spdlog::create<spdlog::sinks::basic_file_sink_mt>("my_logger", log_filename, true);
        logger->set_pattern("%v");
        logger->set_level(spdlog::level::info);
    }
}

Simulator::~Simulator() {
    if (mem_dump) {
        MemoryDump();
    }
    delete MainMemory;
    delete cpu;
    delete Bus;
    delete trace;
    delete timer;
}

void Simulator::MemoryDump() {
    std::cout << "********** MEMORY DUMP ***********\n";

    if (dump_addr_st == 0) {
        dump_addr_st = cpu->getStartDumpAddress();
    }

    if (dump_addr_end == 0) {
        dump_addr_end = cpu->getEndDumpAddress();
    }

    std::cout << "from 0x" << std::hex << dump_addr_st << " to 0x" << dump_addr_end << "\n";
    tlm::tlm_generic_payload trans;
    sc_core::sc_time delay;
    std::uint32_t data[4];

    trans.set_command(tlm::TLM_READ_COMMAND);
    trans.set_data_ptr(reinterpret_cast<unsigned char*>(data));
    trans.set_data_length(4);
    trans.set_streaming_width(4);       // = data_length to indicate no streaming
    trans.set_byte_enable_ptr(nullptr); // 0 indicates unused
    trans.set_dmi_allowed(false);       // Mandatory initial value
    trans.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

    /* Filename in format name.elf.hex should be name.signature_output */
    std::string base_filename = filename.substr(filename.find_last_of("/\\") + 1);
    std::string base_name = base_filename.substr(0, base_filename.find('.'));
    std::string local_name = base_name + ".signature.output";
    std::cout << "filename is " << local_name << '\n';

    std::ofstream signature_file;
    signature_file.open(local_name);

    for (unsigned int i = dump_addr_st; i < dump_addr_end; i = i + 4) {
        // trans.set_address(dump_addr + (i*4));
        trans.set_address(i);
        MainMemory->b_transport(trans, delay);
        signature_file << std::hex << std::setfill('0') << std::setw(8) << data[0] << "\n";
    }

    signature_file.close();
}

void Simulator::process_arguments(int argc, char* argv[]) {

    int c;
    long int debug_level;

    debug_session = false;
    cpu_type = riscv_tlm::RV32;

    while ((c = getopt(argc, argv, "DTE:B:L:f:R:?")) != -1) {
        switch (c) {
            case 'D':
                debug_session = true;
                break;
            case 'T':
                mem_dump = true;
                break;
            case 'B':
                dump_addr_st = std::strtoul(optarg, nullptr, 16);
                break;
            case 'E':
                dump_addr_end = std::strtoul(optarg, nullptr, 16);
                break;
            case 'L':
                debug_level = std::strtol(optarg, nullptr, 10);

                if (debug_level >= 0) {
                    spdlog::filename_t log_filename = SPDLOG_FILENAME_T("Log.txt");
                    logger = spdlog::create<spdlog::sinks::basic_file_sink_mt>("my_logger", log_filename, true);
                    logger->set_pattern("%v");
                    logger->set_level(spdlog::level::info);
                } else {
                    logger = nullptr;
                }

                switch (debug_level) {
                    case 3:
                        logger->set_level(spdlog::level::info);
                        break;
                    case 2:
                        logger->set_level(spdlog::level::warn);
                        break;
                    case 1:
                        logger->set_level(spdlog::level::debug);
                        break;
                    case 0:
                        logger->set_level(spdlog::level::err);
                        break;
                    default:
                        logger->set_level(spdlog::level::info);
                        break;
                }
                break;
            case 'f':
                filename = std::string(optarg);
                break;
            case 'R':
                if (strcmp(optarg, "32") == 0) {
                    cpu_type = riscv_tlm::RV32;
                } else {
                    cpu_type = riscv_tlm::RV64;
                }
                break;
            case '?':
                std::cout << "Call ./RISCV_TLM -D -L <debuglevel> (0..3) filename.hex" << std::endl;
                break;
            default:
                std::cout << "unknown" << std::endl;
        }
    }

    if (filename.empty()) {
        filename = std::string(argv[optind]);
    }

    std::cout << "file: " << filename << '\n';
}

void intHandler(int dummy) {
    // delete top;
    (void)dummy;
    // sc_stop();
    exit(-1);
}
