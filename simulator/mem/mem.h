/**
 * mem.h - Simulation of memory stage
 * Copyright 2015-2018 MIPT-MIPS
 */


#ifndef MEM_H
#define MEM_H


#include <infra/ports/ports.h>
#include <core/perf_instr.h>
#include <bpu/bpu.h>


template <typename ISA>
class Mem : public Log
{
    using FuncInstr = typename ISA::FuncInstr;
    using Instr = PerfInstr<FuncInstr>;
    using Memory = typename ISA::Memory;
    using RegisterUInt = typename ISA::RegisterUInt;
    using RegDstUInt = typename ISA::RegDstUInt;

    private:
        Memory* memory = nullptr;

        std::unique_ptr<WritePort<Instr>> wp_datapath = nullptr;
        std::unique_ptr<ReadPort<Instr>> rp_datapath = nullptr;

        std::unique_ptr<WritePort<bool>> wp_flush_all = nullptr;
        std::unique_ptr<ReadPort<bool>> rp_flush = nullptr;

        std::unique_ptr<WritePort<Addr>> wp_flush_target = nullptr;
        std::unique_ptr<WritePort<BPInterface>> wp_bp_update = nullptr;

        static constexpr const uint8 SRC_REGISTERS_NUM = 2;

        std::unique_ptr<WritePort<RegDstUInt>> wp_bypass = nullptr;

        std::unique_ptr<WritePort<Instr>> wp_bypassing_unit_flush_notify = nullptr;
    
    public:
        explicit Mem( bool log);
        void clock( Cycle cycle);
        void set_memory( Memory* mem) { memory = mem; }
};


#endif // MEM_H
