#include <iostream>
#include <iomanip>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <unordered_map>
#include <string>
#include "goroutine.h"
#include "parser/src/parser/lib/ast.h"
#include "gc.h"  // For GC tracking

// Forward declaration of runtime functions
extern "C" uint64_t runtime_sleep(int64_t milliseconds);

// Extern C functions for TechnoScript runtime
extern "C" {
    void print_int64(int64_t value) {
        std::printf("%lld\n", static_cast<long long>(value));
    }

    void print_float64(double value) {
        std::printf("%g\n", value);
    }

    void print_any(uint64_t type, uint64_t value) {
        if (type == static_cast<uint64_t>(DataType::FLOAT64)) {
            union {
                uint64_t u;
                double d;
            } converter;
            converter.u = value;
            std::printf("%g\n", converter.d);
        } else {
            std::printf("[print_any type=%llu value=0x%llx]\n",
                        static_cast<unsigned long long>(type),
                        static_cast<unsigned long long>(value));
        }
    }

    void print_string(const char* str) {
        std::cout << str << std::endl;
    }

// Async functions implementation
uint64_t technoscript_sleep(int64_t milliseconds) {
    // Delegate to the runtime function
    return runtime_sleep(milliseconds);
}

// HashMap implementation for dynamic properties
struct HashMapEntry {
    std::string key;
    uint64_t type;  // DataType enum value
    uint64_t value; // The actual value (could be int64, float64, or pointer)
};

struct HashMap {
    std::unordered_map<std::string, HashMapEntry> entries;
};

// HashMap API functions
void* hashmap_create() {
    HashMap* map = new HashMap();
    // Track the hashmap in GC
    gc_track_object(reinterpret_cast<void*>(map));
    return map;
}

void hashmap_set(void* mapPtr, const char* key, uint64_t type, uint64_t value) {
    HashMap* map = static_cast<HashMap*>(mapPtr);
    HashMapEntry entry;
    entry.key = key;
    entry.type = type;
    entry.value = value;
    map->entries[key] = entry;

    // Note: GC write barriers are handled at the language level when assigning to object fields
    // The hashmap itself doesn't need special handling since it's heap-allocated
}

uint64_t hashmap_get(void* mapPtr, const char* key, uint64_t* outType) {
    HashMap* map = static_cast<HashMap*>(mapPtr);
    auto it = map->entries.find(key);
    if (it != map->entries.end()) {
        *outType = it->second.type;
        return it->second.value;
    }
    // Key not found - return 0 and set type to ANY (which we'll treat as undefined)
    *outType = static_cast<uint64_t>(DataType::ANY);
    return 0;
}

bool hashmap_has(void* mapPtr, const char* key) {
    HashMap* map = static_cast<HashMap*>(mapPtr);
    return map->entries.find(key) != map->entries.end();
}

void hashmap_delete(void* mapPtr, const char* key) {
    HashMap* map = static_cast<HashMap*>(mapPtr);
    map->entries.erase(key);
}

}
