#pragma once
#include <cstdio>
#include <cstdint>
#include <vector>
#include <string>

inline bool write_iq_file(const std::string& filename, const std::vector<int8_t>& iq_samples) {
    FILE* iq_file = fopen(filename.c_str(), "wb");
    if (iq_file) {
        fwrite(iq_samples.data(), sizeof(int8_t), iq_samples.size(), iq_file);
        fclose(iq_file);
        printf("Wrote %zu IQ samples to %s\n", iq_samples.size() / 2, filename.c_str());
        return true;
    } 
   
    printf("Failed to open %s for writing!\n", filename.c_str());
    return false;
}
