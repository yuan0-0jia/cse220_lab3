#include "hhrt.h"
#include <vector>
#include <cmath>
#include <cassert>
#include <iostream>

extern "C" {
#include "bp/bp.param.h"
#include "core.param.h"
#include "globals/assert.h"
#include "statistics.h"
}

// Configuration Constants
#define PATTERN_TABLE_SIZE (1 << GHR_SIZE)    // Pattern table size based on GHR size

// Data Structures
static std::vector<uint8_t> pattern_table(PATTERN_TABLE_SIZE, 1); // Pattern table with 2-bit saturating counters
static std::vector<uint32_t> hash_hrt(HRT_SIZE, 0);              // Hashed History Register Table

// Hashing Function
unsigned int hash_function(Addr address) {
    return address % HRT_SIZE; // Simple modulo hash function
}

// Predictor Interface Functions
void bp_hhrt_init() {
    // Initialize pattern table to weakly taken
    std::fill(pattern_table.begin(), pattern_table.end(), 1);

    // Initialize hashed history register table to 0
    std::fill(hash_hrt.begin(), hash_hrt.end(), 0);
}

void bp_hhrt_timestamp(Op* op) {}

uns8 bp_hhrt_pred(Op* op) {
    // Get branch address and hash it to retrieve the global history
    Addr address = op->oracle_info.pred_addr;
    unsigned int hrt_index = hash_function(address);
    uint32_t ghr = hash_hrt[hrt_index];

    // Use the global history to index the pattern table
    unsigned int pht_index = ghr & ((1 << GHR_SIZE) - 1);
    return (pattern_table[pht_index] >= 2) ? 1 : 0; // Predict TAKEN if counter >= 2
}

void bp_hhrt_update(Op* op) {
    // Get branch address and hash it to retrieve the global history
    Addr address = op->oracle_info.pred_addr;
    unsigned int hrt_index = hash_function(address);
    uint32_t ghr = hash_hrt[hrt_index];

    // Use the global history to index the pattern table
    unsigned int pht_index = ghr & ((1 << GHR_SIZE) - 1);

    // Update the pattern table based on the actual branch outcome
    bool actual_outcome = op->oracle_info.dir;
    if (actual_outcome) {
        if (pattern_table[pht_index] < 3) {
            pattern_table[pht_index]++;
        }
    } else {
        if (pattern_table[pht_index] > 0) {
            pattern_table[pht_index]--;
        }
    }

    // Update the hashed history register
    hash_hrt[hrt_index] = ((ghr << 1) | actual_outcome) & ((1 << GHR_SIZE) - 1);
}

void bp_hhrt_recover(Recovery_Info* info) {}

void bp_hhrt_spec_update(Op* op) {}

void bp_hhrt_retire(Op* op) {}

uns8 bp_hhrt_full(uns proc_id) {
    return 0; // HHRT is not considered full in this implementation
}
