/*
 * This example shows how to index and query a vector of random integers with the PGM-index.
 * Compile with:
 *   g++ simple.cpp -std=c++17 -I../include -o simple
 * Run with:
 *   ./simple
 */

 #include <vector>
 #include <cstdlib>
 #include <iostream>
 #include <algorithm>
 #include <fstream>
 #include "pgm/pgm_index.hpp"
 
 
// 从二进制文件中读取uint64_t数据
template<typename K>
std::vector<K> readKeysFromFile(const std::string& filename, size_t max_keys = 0) {
    std::vector<K> keys;
    std::ifstream file(filename, std::ios::binary);
    
    if (!file.is_open()) {
        std::cerr << "无法打开文件: " << filename << std::endl;
        return keys;
    }
    
    // 获取文件大小
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // 计算文件中有多少个uint64_t
    size_t num_keys = fileSize / sizeof(K);

    
    // 分配内存
    keys.resize(num_keys);
    
    // 读取数据
    file.read(reinterpret_cast<char*>(keys.data()), num_keys * sizeof(K));
    keys.assign(keys.begin()+1, keys.end());
    
    std::cout << "read " << keys.size() << " keys from file " << filename << std::endl;
    return keys;
}

// 保存PGM-index的分割点到文件
template<typename K, int Epsilon>
void saveSegmentsToFile(const pgm::PGMIndex<K, Epsilon>& index, const std::string& filename) {
    std::ofstream out(filename, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "fail to open file: " << filename << std::endl;
        return;
    }

    uint64_t segments_count = index.levels_offsets[1]-1;
    out.write(reinterpret_cast<const char*>(&segments_count), sizeof(K));

    // 写入当前层级的所有分割点
    for (const auto& segment : index.segments) {
        out.write(reinterpret_cast<const char*>(&segment.key), sizeof(K));
    }

    
    std::cout << "save to: " << filename << std::endl;
    out.close();
}

// 从文件中读取PGM-index的分割点
template<typename K>
std::vector<K> loadSegmentsFromFile(const std::string& filename) {
    std::ifstream in(filename, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "fail to open file: " << filename << std::endl;
    }
    
    std::vector<K> segKeys;

    K size;
    in.read(reinterpret_cast<char*>(&size), sizeof(K));

    for(size_t i = 0; i < size; i++){
        K key;
        in.read(reinterpret_cast<char*>(&key), sizeof(K));
        segKeys.push_back(key);
    }
    
    std::cout << "read " << segKeys.size() << " points from: " << filename << std::endl;
    in.close();
    
    // 从分割点构建PGM-index
    return segKeys;
}
 
int main() {
    // Generate some random data
    std::string data_file = "/sharenvme/usershome/lqa/Ahri/tests/osm_cellids_200M_uint64";
    std::vector<uint64_t> data = readKeysFromFile<uint64_t>(data_file);

    // Construct the PGM-index
    const int epsilon = 128; // space-time trade-off parameter
    pgm::PGMIndex<uint64_t, epsilon> index(data);
    std::cout << "PGM index size: " << index.size_in_bytes() / (1024 * 1024) << " MB" << std::endl;
    std::cout << "PGM index height: " << index.height() << std::endl;
    std::cout << "PGM index segments count: " << index.segments_count() << std::endl;
    for(auto offset: index.levels_offsets){
        std::cout << "level offset: " << offset << std::endl;
    }
    // 将分割点保存到文件
    std::string segments_file = "/sharenvme/usershome/lqa/Ahri/tests/pgm_segments.bin";
    saveSegmentsToFile<uint64_t, epsilon>(index, segments_file);

    // 从文件加载分割点并创建新索引
    auto loaded_index = loadSegmentsFromFile<uint64_t>(segments_file);
    std::cout << "Loaded PGM index segments count: " << loaded_index.size() << std::endl;

    return 0;
}