#include "benchmark.h"
#include "game_def.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <windows.h>
#include <psapi.h>
#include <random>

using namespace std;
using HR = chrono::high_resolution_clock;

volatile int global_dummy = 0;

SIZE_T get_current_memory_kb() {
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize / 1024;
    }
    return 0;
}

void warmup_cpu() {
    volatile int x = 0;
    for (int i = 0; i < 1000000; i++) {
        x += i;
    }
    global_dummy += x;
}

struct BenchmarkResult {
    int64_t total_allocations;
    double total_time_ms;
    double avg_alloc_ns;
    SIZE_T start_memory_kb;
    SIZE_T end_memory_kb;
    double memory_used_kb;
};

void raw_allocation_test(BenchmarkResult& result) {
    const int outer_loops = 50;
    const int inner_loops = 200;
    int64_t total_allocs = 0;
    
    mt19937 rng(12345);
    uniform_int_distribution<int> dist(1, 100);
    
    SIZE_T start_mem = get_current_memory_kb();
    auto start_time = HR::now();
    
    for (int outer = 0; outer < outer_loops; outer++) {
        vector<Particle*> particles;
        
        for (int i = 0; i < inner_loops; i++) {
            Particle* p = new Particle();
            p->x = static_cast<float>(dist(rng));
            p->y = static_cast<float>(dist(rng));
            p->vx = (dist(rng) % 20 - 10) / 10.0f;
            p->vy = (dist(rng) % 20 - 10) / 10.0f;
            p->life = dist(rng) % 20 + 5;
            particles.push_back(p);
            total_allocs++;
            
            global_dummy += static_cast<int>(p->x + p->y);
        }
        
        for (auto p : particles) {
            global_dummy += p->life;
            delete p;
        }
    }
    
    auto end_time = HR::now();
    SIZE_T end_mem = get_current_memory_kb();
    
    double duration_ns = chrono::duration<double, nano>(end_time - start_time).count();
    double duration_ms = duration_ns / 1000000.0;
    
    result.total_allocations = total_allocs;
    result.total_time_ms = duration_ms;
    result.avg_alloc_ns = duration_ns / total_allocs;
    result.start_memory_kb = start_mem;
    result.end_memory_kb = end_mem;
    result.memory_used_kb = static_cast<double>(end_mem - start_mem);
}

void memory_pool_test(BenchmarkResult& result) {
    const int outer_loops = 50;
    const int inner_loops = 200;
    int64_t total_allocs = 0;
    
    mt19937 rng(12345);
    uniform_int_distribution<int> dist(1, 100);
    
    ObjectPool<Particle> pool;
    
    SIZE_T start_mem = get_current_memory_kb();
    auto start_time = HR::now();
    
    for (int outer = 0; outer < outer_loops; outer++) {
        vector<Particle*> particles;
        
        for (int i = 0; i < inner_loops; i++) {
            Particle* p = pool.allocate();
            p->x = static_cast<float>(dist(rng));
            p->y = static_cast<float>(dist(rng));
            p->vx = (dist(rng) % 20 - 10) / 10.0f;
            p->vy = (dist(rng) % 20 - 10) / 10.0f;
            p->life = dist(rng) % 20 + 5;
            particles.push_back(p);
            total_allocs++;
            
            global_dummy += static_cast<int>(p->x + p->y);
        }
        
        for (auto p : particles) {
            global_dummy += p->life;
            pool.deallocate(p);
        }
    }
    
    auto end_time = HR::now();
    SIZE_T end_mem = get_current_memory_kb();
    
    double duration_ns = chrono::duration<double, nano>(end_time - start_time).count();
    double duration_ms = duration_ns / 1000000.0;
    
    result.total_allocations = total_allocs;
    result.total_time_ms = duration_ms;
    result.avg_alloc_ns = duration_ns / total_allocs;
    result.start_memory_kb = start_mem;
    result.end_memory_kb = end_mem;
    result.memory_used_kb = static_cast<double>(end_mem - start_mem);
}

double run_multiple_tests(void (*test_func)(BenchmarkResult&), vector<double>& times, const int runs) {
    double total_time = 0.0;
    
    for (int run = 0; run < runs; run++) {
        BenchmarkResult result{};
        test_func(result);
        times.push_back(result.total_time_ms);
        total_time += result.total_time_ms;
        
        if (run == runs - 1) {
            cout << "  Run " << (run+1) << "/" << runs << ": " << fixed << setprecision(3)
                 << result.total_time_ms << " ms" << endl;
        }
    }
    
    return total_time / runs;
}

double calculate_stddev(const vector<double>& times, double mean) {
    double variance = 0.0;
    for (double t : times) {
        variance += (t - mean) * (t - mean);
    }
    variance /= times.size();
    return sqrt(variance);
}

void run_all_benchmarks() {
    cout << "========================================" << endl;
    cout << "  Rigorous Memory Pool Benchmark" << endl;
    cout << "========================================" << endl << endl;
    
    cout << "Step 1: CPU Warmup" << endl;
    warmup_cpu();
    cout << "CPU warmed up" << endl << endl;
    
    const int test_runs = 10;
    const int warmup_runs = 3;
    
    cout << "Step 2: Running RAW allocation test" << endl;
    cout << "Warmup runs (will be discarded):" << endl;
    for (int i = 0; i < warmup_runs; i++) {
        BenchmarkResult warmup{};
        raw_allocation_test(warmup);
        cout << "  Warmup " << (i+1) << "/" << warmup_runs << ": " << warmup.total_time_ms << " ms" << endl;
    }
    
    cout << "Actual test runs:" << endl;
    vector<double> raw_times;
    double avg_raw = run_multiple_tests(raw_allocation_test, raw_times, test_runs);
    double stddev_raw = calculate_stddev(raw_times, avg_raw);
    double min_raw = *min_element(raw_times.begin(), raw_times.end());
    double max_raw = *max_element(raw_times.begin(), raw_times.end());
    
    cout << "RAW Allocation Statistics:" << endl;
    cout << "  Average: " << avg_raw << " ms" << endl;
    cout << "  StdDev:  " << stddev_raw << " ms" << endl;
    cout << "  Min:     " << min_raw << " ms" << endl;
    cout << "  Max:     " << max_raw << " ms" << endl << endl;
    
    cout << "Step 3: Running MEMORY POOL test" << endl;
    cout << "Warmup runs (will be discarded):" << endl;
    for (int i = 0; i < warmup_runs; i++) {
        BenchmarkResult warmup{};
        memory_pool_test(warmup);
        cout << "  Warmup " << (i+1) << "/" << warmup_runs << ": " << warmup.total_time_ms << " ms" << endl;
    }
    
    cout << "Actual test runs:" << endl;
    vector<double> pool_times;
    double avg_pool = run_multiple_tests(memory_pool_test, pool_times, test_runs);
    double stddev_pool = calculate_stddev(pool_times, avg_pool);
    double min_pool = *min_element(pool_times.begin(), pool_times.end());
    double max_pool = *max_element(pool_times.begin(), pool_times.end());
    
    cout << "Memory Pool Statistics:" << endl;
    cout << "  Average: " << avg_pool << " ms" << endl;
    cout << "  StdDev:  " << stddev_pool << " ms" << endl;
    cout << "  Min:     " << min_pool << " ms" << endl;
    cout << "  Max:     " << max_pool << " ms" << endl << endl;
    
    double speedup = avg_raw / avg_pool;
    
    BenchmarkResult final_raw{};
    raw_allocation_test(final_raw);
    
    BenchmarkResult final_pool{};
    memory_pool_test(final_pool);
    
    cout << "========================================" << endl;
    cout << "FINAL RESULTS:" << endl;
    cout << "========================================" << endl;
    cout << "Total allocations: " << final_raw.total_allocations << endl;
    cout << "Raw allocation:" << endl;
    cout << "  Avg time: " << avg_raw << " ms" << endl;
    cout << "  Avg per alloc: " << (avg_raw * 1000000.0 / final_raw.total_allocations) << " ns" << endl;
    cout << "Memory pool:" << endl;
    cout << "  Avg time: " << avg_pool << " ms" << endl;
    cout << "  Avg per alloc: " << (avg_pool * 1000000.0 / final_pool.total_allocations) << " ns" << endl;
    cout << "Performance improvement:" << endl;
    cout << "  Speedup: " << speedup << "x" << endl;
    cout << "  Raw StdDev: " << (stddev_raw / avg_raw * 100.0) << "%" << endl;
    cout << "  Pool StdDev: " << (stddev_pool / avg_pool * 100.0) << "%" << endl;
    cout << "========================================" << endl;
    
    stringstream json;
    json << "{\n" << "  \"test_info\": {\n";
    json << "    \"test_runs\": " << test_runs << ",\n";
    json << "    \"warmup_runs\": " << warmup_runs << ",\n";
    json << "    \"total_allocations\": " << final_raw.total_allocations << "\n";
    json << "  },\n";
    json << "  \"without_pool\": {\n";
    json << "    \"total_time_ms\": " << fixed << setprecision(3) << avg_raw << ",\n";
    json << "    \"avg_alloc_ns\": " << fixed << setprecision(2) << (avg_raw * 1000000.0 / final_raw.total_allocations) << ",\n";
    json << "    \"stddev_ms\": " << fixed << setprecision(3) << stddev_raw << ",\n";
    json << "    \"min_ms\": " << fixed << setprecision(3) << min_raw << ",\n";
    json << "    \"max_ms\": " << fixed << setprecision(3) << max_raw << ",\n";
    json << "    \"peak_memory_kb\": " << final_raw.memory_used_kb << "\n";
    json << "  },\n";
    json << "  \"with_pool\": {\n";
    json << "    \"total_time_ms\": " << fixed << setprecision(3) << avg_pool << ",\n";
    json << "    \"avg_alloc_ns\": " << fixed << setprecision(2) << (avg_pool * 1000000.0 / final_pool.total_allocations) << ",\n";
    json << "    \"stddev_ms\": " << fixed << setprecision(3) << stddev_pool << ",\n";
    json << "    \"min_ms\": " << fixed << setprecision(3) << min_pool << ",\n";
    json << "    \"max_ms\": " << fixed << setprecision(3) << max_pool << ",\n";
    json << "    \"peak_memory_kb\": " << final_pool.memory_used_kb << "\n";
    json << "  },\n";
    json << "  \"speedup\": " << fixed << setprecision(2) << speedup << "\n";
    json << "}";

    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    string exePath = path;
    size_t lastSlash = exePath.find_last_of("\\/");
    string jsonPath = exePath.substr(0, lastSlash + 1) + "memorypool.json";

    ofstream outFile(jsonPath);
    if (outFile.is_open()) {
        outFile << json.str();
        outFile.close();
        cout << "Results saved to: " << jsonPath << endl;
    } else {
        cerr << "Failed to write results file!" << endl;
    }
    
    cout << "\nDummy value to prevent optimization: " << global_dummy << endl;
}
