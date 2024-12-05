#include "ahrt.h"
#include <vector>
#include <cmath>
#include <limits>
#include <cassert>
#include <iostream>

extern "C" {
#include "bp/bp.param.h"
#include "core.param.h"
#include "globals/assert.h"
#include "statistics.h"
}

#define AHRT_ASSOC 4  // 4-way set associative cache

// Cache entry structure for AHRT
struct CacheEntry {
    Addr tag;             // Branch address tag
    long long history;    // Local history for the branch
    bool valid;           // Valid bit
    int64_t timestamp;    // Timestamp for LRU replacement
    CacheEntry() : tag(0), history(0), valid(false), timestamp(0) {}
};

// Cache set containing associative entries
struct CacheSet {
    std::vector<CacheEntry> entries;
    CacheSet() : entries(AHRT_ASSOC) {}
};

std::vector<CacheSet> cache(0);     // AHRT structure
uns8* pattern_history_table = nullptr; // Pattern table with 2-bit counters
int64_t currentTime = 0;           // Global time for LRU

// Helper: Increment and return the current global time for LRU
int64_t get_current_time() {
    return ++currentTime;
}

// Helper: Compute the set index in AHRT
unsigned int compute_set_index(Addr address) {
    int AHRT_SETS = HRT_SIZE / AHRT_ASSOC;
    unsigned int index_length = static_cast<unsigned>(std::log2(AHRT_SETS));
    return address & ((1 << index_length) - 1);
}

// Helper: Compute the tag for AHRT
Addr compute_tag(Addr address) {
    int AHRT_SETS = HRT_SIZE / AHRT_ASSOC;
    unsigned int index_length = static_cast<unsigned>(std::log2(AHRT_SETS));
    return address >> index_length;
}

// Helper: Check for a matching tag in a set
int check_hit(CacheSet& set, Addr tag) {
    for (size_t i = 0; i < set.entries.size(); ++i) {
        if (set.entries[i].valid && set.entries[i].tag == tag) {
            return i;  // Hit, return index
        }
    }
    return -1; // Miss
}

// Helper: Find an eviction candidate using LRU
int find_evict_index(CacheSet& set) {
    int lru_index = -1;
    int64_t oldest_time = std::numeric_limits<int64_t>::max();
    for (size_t i = 0; i < set.entries.size(); ++i) {
        if (!set.entries[i].valid) {
            return i; // Use an invalid entry if available
        }
        if (set.entries[i].timestamp < oldest_time) {
            oldest_time = set.entries[i].timestamp;
            lru_index = i;
        }
    }
    return lru_index; // Return the LRU index
}

// Helper: Update the AHRT cache with new history
void update_cache(Addr address, long long history) {
    unsigned int set_index = compute_set_index(address);
    Addr tag = compute_tag(address);
    CacheSet& set = cache[set_index];

    int hit_index = check_hit(set, tag);

    // If there's a hit, update the entry
    if (hit_index != -1) {
        set.entries[hit_index].history = history;
        set.entries[hit_index].timestamp = get_current_time();
        return;
    }

    // If there's a miss, replace the least recently used entry
    int evict_index = find_evict_index(set);
    set.entries[evict_index].tag = tag;
    set.entries[evict_index].history = history;
    set.entries[evict_index].valid = true;
    set.entries[evict_index].timestamp = get_current_time();
}

// Helper: Retrieve a history value from AHRT
long long get_cache_entry(Addr address) {
    unsigned int set_index = compute_set_index(address);
    Addr tag = compute_tag(address);
    CacheSet& set = cache[set_index];

    int hit_index = check_hit(set, tag);

    if (hit_index != -1) {
        set.entries[hit_index].timestamp = get_current_time();
        return set.entries[hit_index].history;
    }

    // If miss, initialize history to 0
    int evict_index = find_evict_index(set);
    set.entries[evict_index].tag = tag;
    set.entries[evict_index].history = 0;
    set.entries[evict_index].valid = true;
    set.entries[evict_index].timestamp = get_current_time();
    return 0;
}

// Initialize AHRT and Pattern Table
void bp_ahrt_init() {
    currentTime = 0;

    // Initialize Pattern History Table
    int rows = pow(2, GHR_SIZE);
    pattern_history_table = (uns8*)malloc(rows * sizeof(uns8));
    if (!pattern_history_table) {
        std::cerr << "Failed to allocate memory for pattern history table" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    for (int i = 0; i < rows; i++) {
        pattern_history_table[i] = 1; // Weakly not taken
    }

    // Initialize AHRT Cache
    int AHRT_SETS = HRT_SIZE / AHRT_ASSOC;
    cache.resize(AHRT_SETS);
}

void bp_ahrt_timestamp(Op* op) {}

// Predict branch direction
uns8 bp_ahrt_pred(Op* op) {
    const Addr address = op->oracle_info.pred_addr;
    long long ghr = get_cache_entry(address);
    unsigned int index = static_cast<unsigned int>(ghr);

    const uns8 pred = (pattern_history_table[index] >= 2);
    return pred;
}

void bp_ahrt_spec_update(Op* op) {}

// Update branch predictor state
void bp_ahrt_update(Op* op) {
    const Addr address = op->oracle_info.pred_addr;
    long long ghr = get_cache_entry(address);
    unsigned int index = static_cast<unsigned int>(ghr);

    bool actual_outcome = (op->oracle_info.dir == TAKEN);

    // Update the pattern table
    if (actual_outcome) {
        if (pattern_history_table[index] < 3) {
            pattern_history_table[index]++;
        }
    } else {
        if (pattern_history_table[index] > 0) {
            pattern_history_table[index]--;
        }
    }

    // Update local history in AHRT
    ghr = ((ghr << 1) | actual_outcome) & ((1 << GHR_SIZE) - 1);
    update_cache(address, ghr);
}

void bp_ahrt_retire(Op* op) {}
void bp_ahrt_recover(Recovery_Info* info) {}
uns8 bp_ahrt_full(uns proc_id) { return 0; }