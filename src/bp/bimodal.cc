#include "bimodal.h"
#include <vector>
#include <memory>
#include <cstdint>
#include <cassert>

// Constants for the bimodal predictor
constexpr size_t TABLE_SIZE = 1024;     // Number of entries in the prediction table
constexpr int8_t INITIAL_COUNTER = 0;  // Initial value of each counter
constexpr int8_t MAX_COUNTER = 3;      // Max counter value (for 2-bit saturation)
constexpr int8_t MIN_COUNTER = -4;     // Min counter value (for 2-bit saturation)

// A vector of bimodal predictor tables (one table per core)
static std::vector<std::vector<int8_t>> bimodal_tables;

static void initialize_bimodal_tables() {
    if (bimodal_tables.empty()) {
        bimodal_tables.resize(NUM_CORES, std::vector<int8_t>(TABLE_SIZE, INITIAL_COUNTER));
    }
}

static size_t get_table_index(uint64_t addr) {
    return addr % TABLE_SIZE;  // Simple modulo-based indexing
}

/********************* Interface Implementation ************************/

void bp_bimodal_init() {
    initialize_bimodal_tables();
}

void bp_bimodal_timestamp(Op* op) {}

uns8 bp_bimodal_pred(Op* op) {
    uns proc_id = op->proc_id;
    assert(proc_id < bimodal_tables.size());
    size_t index = get_table_index(op->inst_info->addr);
    return bimodal_tables[proc_id][index] >= 0;  // Predict "taken" if counter is non-negative
}

void bp_bimodal_spec_update(Op* op) {}

void bp_bimodal_update(Op* op) {
    uns proc_id = op->proc_id;
    assert(proc_id < bimodal_tables.size());
    size_t index = get_table_index(op->inst_info->addr);
    int8_t& counter = bimodal_tables[proc_id][index];

    if (op->oracle_info.dir) {  // Branch was actually "taken"
        if (counter < MAX_COUNTER) ++counter;  // Increment counter for taken branches
    } else {  // Branch was "not taken"
        if (counter > MIN_COUNTER) --counter;  // Decrement counter for not-taken branches
    }
}

void bp_bimodal_retire(Op* op) {}

void bp_bimodal_recover(Recovery_Info* recovery_info) {}

uns8 bp_bimodal_full(uns proc_id) {
    return 0;  // Always return "not full"
}