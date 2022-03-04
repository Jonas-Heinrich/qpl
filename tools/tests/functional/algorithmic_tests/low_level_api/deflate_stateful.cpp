/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2022 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials,
 * and your use of them is governed by the express license under which they
 * were provided to you ("License"). Unless the License provides otherwise,
 * you may not use, modify, copy, publish, distribute, disclose or transmit
 * this software or the related documents without Intel's prior written
 * permission.
 *
 * This software and the related documents are provided as is, with no
 * express or implied warranties, other than those that are expressly
 * stated in the License.
 *
 */

#include <algorithm>
#include <array>
#include "ta_ll_common.hpp"
#include "source_provider.hpp"

namespace qpl::test {
enum compression_mode {
    fixed_compression,
    static_compression,
    dynamic_compression,
    canned_compression
};

auto get_chunk_sizes() -> std::vector<uint32_t> {
    std::vector<uint32_t> result;
    auto insert_numbers_in_range = [&](uint32_t lower_boundary, uint32_t upper_boundary,
                                       uint32_t count) -> auto {
        auto step = (upper_boundary - lower_boundary) / count;
        for (uint32_t i = lower_boundary; i < upper_boundary; i += step)
            result.push_back(i);
    };

    insert_numbers_in_range(1123, 9999, 5);
    insert_numbers_in_range(10000, 48123, 5);

    return result;
}

// Functions to perform compression with given job
// Accepts compression parameters
template<compression_mode mode>
qpl_status compress_with_chunks(std::vector<uint8_t> &source,
                                std::vector<uint8_t> &destination,
                                uint32_t chunk_size,
                                qpl_job *job_ptr,
                                qpl_compression_huffman_table *table_ptr,
                                qpl_compression_levels level,
                                bool omit_verification) { return QPL_STS_OK; }

template<>
qpl_status compress_with_chunks<compression_mode::dynamic_compression>(std::vector<uint8_t> &source,
                                                                       std::vector<uint8_t> &destination,
                                                                       uint32_t chunk_size,
                                                                       qpl_job *job_ptr,
                                                                       qpl_compression_huffman_table *table_ptr,
                                                                       qpl_compression_levels level,
                                                                       bool omit_verification) {
    qpl_status result = QPL_STS_OK;
    // Configure job
    job_ptr->op = qpl_op_compress;
    job_ptr->flags = QPL_FLAG_FIRST | QPL_FLAG_DYNAMIC_HUFFMAN;
    job_ptr->flags |= (omit_verification) ? QPL_FLAG_OMIT_VERIFY : 0;

    job_ptr->available_in = static_cast<uint32_t>(source.size());
    job_ptr->available_out = static_cast<uint32_t>(destination.size());

    job_ptr->next_in_ptr = source.data();
    job_ptr->next_out_ptr = destination.data();

    job_ptr->level = level;

    // Compress
    auto current_chunk_size = chunk_size;
    uint32_t iteration_count = 0;
    auto source_bytes_left = static_cast<uint32_t>(source.size());

    while (source_bytes_left > 0) {
        if (current_chunk_size >= source_bytes_left) {
            job_ptr->flags |= QPL_FLAG_LAST;
            current_chunk_size = source_bytes_left;
        }

        source_bytes_left -= current_chunk_size;
        job_ptr->next_in_ptr = source.data() + iteration_count * chunk_size;
        job_ptr->available_in = current_chunk_size;
        result = run_job_api(job_ptr);

        if (result) {
            std::cout << "err" << result << ", " << iteration_count << std::endl;
            return result;
        }

        job_ptr->flags &= ~QPL_FLAG_FIRST;
        iteration_count++;
    }

    destination.resize(job_ptr->total_out);
    return result;
}

template<>
qpl_status compress_with_chunks<compression_mode::static_compression>(std::vector<uint8_t> &source,
                                                                      std::vector<uint8_t> &destination,
                                                                      uint32_t chunk_size,
                                                                      qpl_job *job_ptr,
                                                                      qpl_compression_huffman_table *table_ptr,
                                                                      qpl_compression_levels level,
                                                                      bool omit_verification) {
    qpl_status result = QPL_STS_OK;
    // Configure job
    job_ptr->op = qpl_op_compress;

    job_ptr->flags = QPL_FLAG_FIRST;
    job_ptr->flags |= (omit_verification) ? QPL_FLAG_OMIT_VERIFY : 0;

    job_ptr->available_in = static_cast<uint32_t>(source.size());
    job_ptr->available_out = static_cast<uint32_t>(destination.size());

    job_ptr->next_in_ptr = source.data();
    job_ptr->next_out_ptr = destination.data();

    job_ptr->compression_huffman_table = table_ptr;
    job_ptr->level = level;

    // Compress
    auto current_chunk_size = chunk_size;
    uint32_t iteration_count = 0;
    auto source_bytes_left = static_cast<uint32_t>(source.size());

    while (source_bytes_left > 0) {
        if (current_chunk_size >= source_bytes_left) {
            job_ptr->flags |= QPL_FLAG_LAST;
            current_chunk_size = source_bytes_left;
        }

        source_bytes_left -= current_chunk_size;
        job_ptr->next_in_ptr = source.data() + iteration_count * chunk_size;
        job_ptr->available_in = current_chunk_size;
        result = run_job_api(job_ptr);

        if (result) {
            return result;
        }

        job_ptr->flags &= ~QPL_FLAG_FIRST;
        iteration_count++;
    }

    destination.resize(job_ptr->total_out);
    return result;
}

template<>
qpl_status compress_with_chunks<compression_mode::fixed_compression>(std::vector<uint8_t> &source,
                                                                     std::vector<uint8_t> &destination,
                                                                     uint32_t chunk_size,
                                                                     qpl_job *job_ptr,
                                                                     qpl_compression_huffman_table *table_ptr,
                                                                     qpl_compression_levels level,
                                                                     bool omit_verification) {
    qpl_status result = QPL_STS_OK;
    // Configure job
    job_ptr->op = qpl_op_compress;

    job_ptr->flags = QPL_FLAG_FIRST;
    job_ptr->flags |= (omit_verification) ? QPL_FLAG_OMIT_VERIFY : 0;

    job_ptr->available_in = static_cast<uint32_t>(source.size());
    job_ptr->available_out = static_cast<uint32_t>(destination.size());

    job_ptr->next_in_ptr = source.data();
    job_ptr->next_out_ptr = destination.data();

    job_ptr->level = level;

    // Compress
    auto current_chunk_size = chunk_size;
    uint32_t iteration_count = 0;
    auto source_bytes_left = static_cast<uint32_t>(source.size());

    while (source_bytes_left > 0) {
        if (current_chunk_size >= source_bytes_left) {
            job_ptr->flags |= QPL_FLAG_LAST;
            current_chunk_size = source_bytes_left;
        }

        source_bytes_left -= current_chunk_size;
        job_ptr->next_in_ptr = source.data() + iteration_count * chunk_size;
        job_ptr->available_in = current_chunk_size;
        result = run_job_api(job_ptr);

        if (result) {
            return result;
        }

        job_ptr->flags &= ~QPL_FLAG_FIRST;
        iteration_count++;
    }

    destination.resize(job_ptr->total_out);
    return result;
}

qpl_status decompress_with_chunks(std::vector<uint8_t> &compressed_source,
                                  std::vector<uint8_t> &destination,
                                  qpl_job *job_ptr,
                                  uint32_t chunk_size) {
    qpl_status result = QPL_STS_OK;
    job_ptr->op = qpl_op_decompress;
    job_ptr->flags = QPL_FLAG_FIRST;
    job_ptr->available_in = static_cast<uint32_t>(compressed_source.size());
    job_ptr->next_in_ptr = destination.data();
    job_ptr->available_out = static_cast<uint32_t>(destination.size());
    job_ptr->next_out_ptr = destination.data();

    auto current_chunk_size = chunk_size;
    uint32_t iteration_count = 0;
    auto source_bytes_left = static_cast<uint32_t>(compressed_source.size());

    while (source_bytes_left > 0) {
        if (current_chunk_size >= source_bytes_left) {
            job_ptr->flags |= QPL_FLAG_LAST;
            current_chunk_size = source_bytes_left;
        }

        source_bytes_left -= current_chunk_size;
        job_ptr->next_in_ptr = compressed_source.data() + iteration_count * chunk_size;
        job_ptr->available_in = current_chunk_size;
        result = run_job_api(job_ptr);

        if (result) {
            return result;
        }

        job_ptr->flags &= ~QPL_FLAG_FIRST;
        iteration_count++;
    }

    destination.resize(job_ptr->total_out);
    return result;
}

QPL_LOW_LEVEL_API_ALGORITHMIC_TEST(deflat, dynamic_default_stateful_compression) {
    auto execution_path = util::TestEnvironment::GetInstance().GetExecutionPath();
    uint32_t job_size = 0;

    auto status = qpl_get_job_size(execution_path, &job_size);
    ASSERT_EQ(QPL_STS_OK, status) << "Failed to get job size\n";

    // Allocate buffers for decompression job
    auto job_buffer = std::make_unique<uint8_t[]>(job_size);
    auto job_ptr = reinterpret_cast<qpl_job *>(job_buffer.get());

    // Initialize decompression job
    status = qpl_init_job(execution_path, job_ptr);
    ASSERT_EQ(QPL_STS_OK, status) << "Failed to initialize job\n";
    
    for (auto &dataset: util::TestEnvironment::GetInstance().GetAlgorithmicDataset().get_data()) {
        std::vector<uint8_t> source;
        source = dataset.second;
        source = dataset.second;
        for (auto block_size: get_chunk_sizes()) {
            std::string error_message = "File name - " + dataset.first + ", block size = " + std::to_string(block_size);
            // status = qpl_init_job(execution_path, job_ptr);
            // ASSERT_EQ(QPL_STS_OK, status) << "Failed to initialize job\n";
            std::vector<uint8_t> compressed_source(source.size() * 2);
            std::vector<uint8_t> reference(source.size());
            status = compress_with_chunks<compression_mode::dynamic_compression>(source,
                                                                                 compressed_source,
                                                                                 block_size,
                                                                                 job_ptr,
                                                                                 nullptr,
                                                                                 qpl_default_level,
                                                                                 true);

            ASSERT_EQ(status, QPL_STS_OK) << "Failed to compress job. " << error_message;

            status = decompress_with_chunks(compressed_source,
                                            reference,
                                            job_ptr,
                                            source.size());

            ASSERT_EQ(status, QPL_STS_OK) << "Failed to decompress job." << error_message;

            ASSERT_EQ(source, reference) << "Compressed and decompressed vectors missmatch!. " << error_message;
        }
    }
}

QPL_LOW_LEVEL_API_ALGORITHMIC_TEST(deflat, dynamic_high_stateful_compression) {
    auto execution_path = util::TestEnvironment::GetInstance().GetExecutionPath();

    if (execution_path == qpl_path_hardware) {
        GTEST_SKIP_("Hardware path doesn't support high level compression");
    }

    uint32_t job_size = 0;

    auto status = qpl_get_job_size(execution_path, &job_size);
    ASSERT_EQ(QPL_STS_OK, status) << "Failed to get job size\n";

    // Allocate buffers for decompression job
    auto job_buffer = std::make_unique<uint8_t[]>(job_size);
    auto job_ptr = reinterpret_cast<qpl_job *>(job_buffer.get());

    // Initialize decompression job
    status = qpl_init_job(execution_path, job_ptr);
    ASSERT_EQ(QPL_STS_OK, status) << "Failed to initialize job\n";

    for (auto &dataset: util::TestEnvironment::GetInstance().GetAlgorithmicDataset().get_data()) {
        std::vector<uint8_t> source;
        source = dataset.second;
        for (auto block_size: get_chunk_sizes()) {
            std::string error_message = "File name - " + dataset.first + ", block size = " + std::to_string(block_size);
            std::vector<uint8_t> compressed_source(source.size() * 2);
            std::vector<uint8_t> reference(source.size());
            status = compress_with_chunks<compression_mode::dynamic_compression>(source,
                                                                                 compressed_source,
                                                                                 block_size,
                                                                                 job_ptr,
                                                                                 nullptr,
                                                                                 qpl_high_level,
                                                                                 true);

            ASSERT_EQ(status, QPL_STS_OK) << "Failed to compress job. " << error_message;

            status = decompress_with_chunks(compressed_source,
                                            reference,
                                            job_ptr,
                                            source.size());

            ASSERT_EQ(status, QPL_STS_OK) << "Failed to decompress job." << error_message;

            ASSERT_EQ(source, reference) << "Compressed and decompressed vectors missmatch!. " << error_message;
        }
    }
}

QPL_LOW_LEVEL_API_ALGORITHMIC_TEST(deflat, dynamic_default_verify_stateful_compression) {
    auto execution_path = util::TestEnvironment::GetInstance().GetExecutionPath();
    uint32_t job_size = 0;

    auto status = qpl_get_job_size(execution_path, &job_size);
    ASSERT_EQ(QPL_STS_OK, status) << "Failed to get job size\n";

    // Allocate buffers for decompression job
    auto job_buffer = std::make_unique<uint8_t[]>(job_size);
    auto job_ptr = reinterpret_cast<qpl_job *>(job_buffer.get());

    // Initialize decompression job
    status = qpl_init_job(execution_path, job_ptr);
    ASSERT_EQ(QPL_STS_OK, status) << "Failed to initialize job\n";

    for (auto &dataset: util::TestEnvironment::GetInstance().GetAlgorithmicDataset().get_data()) {
        std::vector<uint8_t> source;
        source = dataset.second;
        for (auto block_size: get_chunk_sizes()) {
            std::string error_message = "File name - " + dataset.first + ", block size = " + std::to_string(block_size);
            std::vector<uint8_t> compressed_source(source.size() * 2);
            std::vector<uint8_t> reference(source.size());
            status = compress_with_chunks<compression_mode::dynamic_compression>(source,
                                                                                 compressed_source,
                                                                                 block_size,
                                                                                 job_ptr,
                                                                                 nullptr,
                                                                                 qpl_default_level,
                                                                                 false);

            ASSERT_EQ(status, QPL_STS_OK) << "Failed to compress job. " << error_message;

            status = decompress_with_chunks(compressed_source,
                                            reference,
                                            job_ptr,
                                            source.size());

            ASSERT_EQ(status, QPL_STS_OK) << "Failed to decompress job." << error_message;

            ASSERT_EQ(source, reference) << "Compressed and decompressed vectors missmatch!. " << error_message;
        }
    }
}

QPL_LOW_LEVEL_API_ALGORITHMIC_TEST(deflat, dynamic_high_verify_stateful_compression) {
    auto execution_path = util::TestEnvironment::GetInstance().GetExecutionPath();

    if (execution_path == qpl_path_hardware) {
        GTEST_SKIP_("Hardware path doesn't support high level compression");
    }

    uint32_t job_size = 0;

    auto status = qpl_get_job_size(execution_path, &job_size);
    ASSERT_EQ(QPL_STS_OK, status) << "Failed to get job size\n";

    // Allocate buffers for decompression job
    auto job_buffer = std::make_unique<uint8_t[]>(job_size);
    auto job_ptr = reinterpret_cast<qpl_job *>(job_buffer.get());

    // Initialize decompression job
    status = qpl_init_job(execution_path, job_ptr);
    ASSERT_EQ(QPL_STS_OK, status) << "Failed to initialize job\n";

    for (auto &dataset: util::TestEnvironment::GetInstance().GetAlgorithmicDataset().get_data()) {
        std::vector<uint8_t> source;
        source = dataset.second;
        for (auto block_size: get_chunk_sizes()) {
            std::string error_message = "File name - " + dataset.first + ", block size = " + std::to_string(block_size);
            std::vector<uint8_t> compressed_source(source.size() * 2);
            std::vector<uint8_t> reference(source.size());
            status = compress_with_chunks<compression_mode::dynamic_compression>(source,
                                                                                 compressed_source,
                                                                                 block_size,
                                                                                 job_ptr,
                                                                                 nullptr,
                                                                                 qpl_high_level,
                                                                                 false);

            ASSERT_EQ(status, QPL_STS_OK) << "Failed to compress job. " << error_message;

            status = decompress_with_chunks(compressed_source,
                                            reference,
                                            job_ptr,
                                            source.size());

            ASSERT_EQ(status, QPL_STS_OK) << "Failed to decompress job." << error_message;

            ASSERT_EQ(source, reference) << "Compressed and decompressed vectors missmatch!. " << error_message;
        }
    }
}

QPL_LOW_LEVEL_API_ALGORITHMIC_TEST(deflat, fixed_default_stateful_compression) {
    auto execution_path = util::TestEnvironment::GetInstance().GetExecutionPath();
    uint32_t job_size = 0;

    auto status = qpl_get_job_size(execution_path, &job_size);
    ASSERT_EQ(QPL_STS_OK, status) << "Failed to get job size\n";

    // Allocate buffers for decompression job
    auto job_buffer = std::make_unique<uint8_t[]>(job_size);
    auto job_ptr = reinterpret_cast<qpl_job *>(job_buffer.get());

    // Initialize decompression job
    status = qpl_init_job(execution_path, job_ptr);
    ASSERT_EQ(QPL_STS_OK, status) << "Failed to initialize job\n";

    for (auto &dataset: util::TestEnvironment::GetInstance().GetAlgorithmicDataset().get_data()) {
        std::vector<uint8_t> source;
        source = dataset.second;
        for (auto block_size: get_chunk_sizes()) {
            std::string error_message = "File name - " + dataset.first + ", block size = " + std::to_string(block_size);
            std::vector<uint8_t> compressed_source(source.size() * 2);
            std::vector<uint8_t> reference(source.size());
            status = compress_with_chunks<compression_mode::fixed_compression>(source,
                                                                               compressed_source,
                                                                               block_size,
                                                                               job_ptr,
                                                                               nullptr,
                                                                               qpl_default_level,
                                                                               true);

            ASSERT_EQ(status, QPL_STS_OK) << "Failed to compress job. " << error_message;

            status = decompress_with_chunks(compressed_source,
                                            reference,
                                            job_ptr,
                                            source.size());

            ASSERT_EQ(status, QPL_STS_OK) << "Failed to decompress job." << error_message;

            ASSERT_EQ(source, reference) << "Compressed and decompressed vectors missmatch!. " << error_message;
        }
    }
}

QPL_LOW_LEVEL_API_ALGORITHMIC_TEST(deflat, fixed_high_stateful_compression) {
    auto execution_path = util::TestEnvironment::GetInstance().GetExecutionPath();

    if (execution_path == qpl_path_hardware) {
        GTEST_SKIP_("Hardware path doesn't support high level compression");
    }

    uint32_t job_size = 0;

    auto status = qpl_get_job_size(execution_path, &job_size);
    ASSERT_EQ(QPL_STS_OK, status) << "Failed to get job size\n";

    // Allocate buffers for decompression job
    auto job_buffer = std::make_unique<uint8_t[]>(job_size);
    auto job_ptr = reinterpret_cast<qpl_job *>(job_buffer.get());

    // Initialize decompression job
    status = qpl_init_job(execution_path, job_ptr);
    ASSERT_EQ(QPL_STS_OK, status) << "Failed to initialize job\n";

    for (auto &dataset: util::TestEnvironment::GetInstance().GetAlgorithmicDataset().get_data()) {
        std::vector<uint8_t> source;
        source = dataset.second;
        for (auto block_size: get_chunk_sizes()) {
            std::string error_message = "File name - " + dataset.first + ", block size = " + std::to_string(block_size);
            std::vector<uint8_t> compressed_source(source.size() * 2);
            std::vector<uint8_t> reference(source.size());
            status = compress_with_chunks<compression_mode::fixed_compression>(source,
                                                                               compressed_source,
                                                                               block_size,
                                                                               job_ptr,
                                                                               nullptr,
                                                                               qpl_high_level,
                                                                               true);

            ASSERT_EQ(status, QPL_STS_OK) << "Failed to compress job. " << error_message;

            status = decompress_with_chunks(compressed_source,
                                            reference,
                                            job_ptr,
                                            source.size());

            ASSERT_EQ(status, QPL_STS_OK) << "Failed to decompress job." << error_message;

            ASSERT_EQ(source, reference) << "Compressed and decompressed vectors missmatch!. " << error_message;
        }
    }
}

QPL_LOW_LEVEL_API_ALGORITHMIC_TEST(deflat, fixed_default_verify_stateful_compression) {
    auto execution_path = util::TestEnvironment::GetInstance().GetExecutionPath();
    uint32_t job_size = 0;

    auto status = qpl_get_job_size(execution_path, &job_size);
    ASSERT_EQ(QPL_STS_OK, status) << "Failed to get job size\n";

    // Allocate buffers for decompression job
    auto job_buffer = std::make_unique<uint8_t[]>(job_size);
    auto job_ptr = reinterpret_cast<qpl_job *>(job_buffer.get());

    // Initialize decompression job
    status = qpl_init_job(execution_path, job_ptr);
    ASSERT_EQ(QPL_STS_OK, status) << "Failed to initialize job\n";

    for (auto &dataset: util::TestEnvironment::GetInstance().GetAlgorithmicDataset().get_data()) {
        std::vector<uint8_t> source;
        source = dataset.second;
        for (auto block_size: get_chunk_sizes()) {
            std::string error_message = "File name - " + dataset.first + ", block size = " + std::to_string(block_size);
            std::vector<uint8_t> compressed_source(source.size() * 2);
            std::vector<uint8_t> reference(source.size());
            status = compress_with_chunks<compression_mode::fixed_compression>(source,
                                                                               compressed_source,
                                                                               block_size,
                                                                               job_ptr,
                                                                               nullptr,
                                                                               qpl_default_level,
                                                                               false);

            ASSERT_EQ(status, QPL_STS_OK) << "Failed to compress job. " << error_message;

            status = decompress_with_chunks(compressed_source,
                                            reference,
                                            job_ptr,
                                            source.size());

            ASSERT_EQ(status, QPL_STS_OK) << "Failed to decompress job." << error_message;

            ASSERT_EQ(source, reference) << "Compressed and decompressed vectors missmatch!. " << error_message;
        }
    }
}

QPL_LOW_LEVEL_API_ALGORITHMIC_TEST(deflat, fixed_high_verify_stateful_compression) {
    auto execution_path = util::TestEnvironment::GetInstance().GetExecutionPath();

    if (execution_path == qpl_path_hardware) {
        GTEST_SKIP_("Hardware path doesn't support high level compression");
    }

    uint32_t job_size = 0;

    auto status = qpl_get_job_size(execution_path, &job_size);
    ASSERT_EQ(QPL_STS_OK, status) << "Failed to get job size\n";

    // Allocate buffers for decompression job
    auto job_buffer = std::make_unique<uint8_t[]>(job_size);
    auto job_ptr = reinterpret_cast<qpl_job *>(job_buffer.get());

    // Initialize decompression job
    status = qpl_init_job(execution_path, job_ptr);
    ASSERT_EQ(QPL_STS_OK, status) << "Failed to initialize job\n";

    for (auto &dataset: util::TestEnvironment::GetInstance().GetAlgorithmicDataset().get_data()) {
        std::vector<uint8_t> source;
        source = dataset.second;
        for (auto block_size: get_chunk_sizes()) {
            std::string error_message = "File name - " + dataset.first + ", block size = " + std::to_string(block_size);
            std::vector<uint8_t> compressed_source(source.size() * 2);
            std::vector<uint8_t> reference(source.size());
            status = compress_with_chunks<compression_mode::fixed_compression>(source,
                                                                               compressed_source,
                                                                               block_size,
                                                                               job_ptr,
                                                                               nullptr,
                                                                               qpl_high_level,
                                                                               false);

            ASSERT_EQ(status, QPL_STS_OK) << "Failed to compress job. " << error_message;

            status = decompress_with_chunks(compressed_source,
                                            reference,
                                            job_ptr,
                                            source.size());

            ASSERT_EQ(status, QPL_STS_OK) << "Failed to decompress job." << error_message;

            ASSERT_EQ(source, reference) << "Compressed and decompressed vectors missmatch!. " << error_message;
        }
    }
}

QPL_LOW_LEVEL_API_ALGORITHMIC_TEST(deflat, static_default_stateful_compression) {
    auto execution_path = util::TestEnvironment::GetInstance().GetExecutionPath();
    uint32_t job_size = 0;

    auto status = qpl_get_job_size(execution_path, &job_size);
    ASSERT_EQ(QPL_STS_OK, status) << "Failed to get job size\n";

    // Allocate buffers for decompression job
    auto job_buffer = std::make_unique<uint8_t[]>(job_size);
    auto job_ptr = reinterpret_cast<qpl_job *>(job_buffer.get());

    // Initialize decompression job
    status = qpl_init_job(execution_path, job_ptr);
    ASSERT_EQ(QPL_STS_OK, status) << "Failed to initialize job\n";

    for (auto &dataset: util::TestEnvironment::GetInstance().GetAlgorithmicDataset().get_data()) {
        std::vector<uint8_t> source;
        source = dataset.second;
        qpl_histogram histogram{};

        status = qpl_gather_deflate_statistics(source.data(), source.size(), &histogram, qpl_default_level,
                                               execution_path);
        ASSERT_EQ(status, QPL_STS_OK) << "Failed to gather deflate statistics\n";
        auto compression_table_buffer = std::make_unique<uint8_t[]>(QPL_COMPRESSION_TABLE_SIZE);
        auto *compression_table_ptr = reinterpret_cast<qpl_compression_huffman_table *>(compression_table_buffer.get());

        uint32_t representation_flags =
                QPL_DEFLATE_REPRESENTATION | ((execution_path == qpl_path_software) ? QPL_SW_REPRESENTATION
                                                                                    : QPL_HW_REPRESENTATION);

        status = qpl_build_compression_table(&histogram, compression_table_ptr, representation_flags);
        ASSERT_EQ(status, QPL_STS_OK) << "Failed to build the table\n";


        for (auto block_size: get_chunk_sizes()) {
            std::string error_message = "File name - " + dataset.first + ", block size = " + std::to_string(block_size);
            std::vector<uint8_t> compressed_source(source.size() * 2);
            std::vector<uint8_t> reference(source.size());
            status = compress_with_chunks<compression_mode::static_compression>(source,
                                                                                compressed_source,
                                                                                block_size,
                                                                                job_ptr,
                                                                                compression_table_ptr,
                                                                                qpl_default_level,
                                                                                true);

            ASSERT_EQ(status, QPL_STS_OK) << "Failed to compress job. " << error_message;

            status = decompress_with_chunks(compressed_source,
                                            reference,
                                            job_ptr,
                                            source.size());

            ASSERT_EQ(status, QPL_STS_OK) << "Failed to decompress job." << error_message;

            ASSERT_EQ(source, reference) << "Compressed and decompressed vectors missmatch!. " << error_message;
        }
    }
}

QPL_LOW_LEVEL_API_ALGORITHMIC_TEST(deflat, static_high_stateful_compression) {
    auto execution_path = util::TestEnvironment::GetInstance().GetExecutionPath();

    if (execution_path == qpl_path_hardware) {
        GTEST_SKIP_("Hardware path doesn't support high level compression");
    }

    uint32_t job_size = 0;

    auto status = qpl_get_job_size(execution_path, &job_size);
    ASSERT_EQ(QPL_STS_OK, status) << "Failed to get job size\n";

    // Allocate buffers for decompression job
    auto job_buffer = std::make_unique<uint8_t[]>(job_size);
    auto job_ptr = reinterpret_cast<qpl_job *>(job_buffer.get());

    // Initialize decompression job
    status = qpl_init_job(execution_path, job_ptr);
    ASSERT_EQ(QPL_STS_OK, status) << "Failed to initialize job\n";

    for (auto &dataset: util::TestEnvironment::GetInstance().GetAlgorithmicDataset().get_data()) {
        std::vector<uint8_t> source;
        source = dataset.second;
        qpl_histogram histogram{};

        status = qpl_gather_deflate_statistics(source.data(), source.size(), &histogram, qpl_high_level,
                                               execution_path);
        ASSERT_EQ(status, QPL_STS_OK) << "Failed to gather deflate statistics\n";
        auto compression_table_buffer = std::make_unique<uint8_t[]>(QPL_COMPRESSION_TABLE_SIZE);
        auto *compression_table_ptr = reinterpret_cast<qpl_compression_huffman_table *>(compression_table_buffer.get());

        uint32_t representation_flags =
                QPL_DEFLATE_REPRESENTATION | ((execution_path == qpl_path_software) ? QPL_SW_REPRESENTATION
                                                                                    : QPL_HW_REPRESENTATION);

        status = qpl_build_compression_table(&histogram, compression_table_ptr, representation_flags);
        ASSERT_EQ(status, QPL_STS_OK) << "Failed to build the table\n";


        for (auto block_size: get_chunk_sizes()) {
            std::string error_message = "File name - " + dataset.first + ", block size = " + std::to_string(block_size);
            std::vector<uint8_t> compressed_source(source.size() * 2);
            std::vector<uint8_t> reference(source.size());
            status = compress_with_chunks<compression_mode::static_compression>(source,
                                                                                compressed_source,
                                                                                block_size,
                                                                                job_ptr,
                                                                                compression_table_ptr,
                                                                                qpl_high_level,
                                                                                true);

            ASSERT_EQ(status, QPL_STS_OK) << "Failed to compress job. " << error_message;

            status = decompress_with_chunks(compressed_source,
                                            reference,
                                            job_ptr,
                                            source.size());

            ASSERT_EQ(status, QPL_STS_OK) << "Failed to decompress job." << error_message;

            ASSERT_EQ(source, reference) << "Compressed and decompressed vectors missmatch!. " << error_message;
        }
    }
}

QPL_LOW_LEVEL_API_ALGORITHMIC_TEST(deflat, static_default_verify_stateful_compression) {
    auto execution_path = util::TestEnvironment::GetInstance().GetExecutionPath();
    uint32_t job_size = 0;

    auto status = qpl_get_job_size(execution_path, &job_size);
    ASSERT_EQ(QPL_STS_OK, status) << "Failed to get job size\n";

    // Allocate buffers for decompression job
    auto job_buffer = std::make_unique<uint8_t[]>(job_size);
    auto job_ptr = reinterpret_cast<qpl_job *>(job_buffer.get());

    // Initialize decompression job
    status = qpl_init_job(execution_path, job_ptr);
    ASSERT_EQ(QPL_STS_OK, status) << "Failed to initialize job\n";

    for (auto &dataset: util::TestEnvironment::GetInstance().GetAlgorithmicDataset().get_data()) {
        std::vector<uint8_t> source;
        source = dataset.second;
        qpl_histogram histogram{};

        status = qpl_gather_deflate_statistics(source.data(), source.size(), &histogram, qpl_default_level,
                                               execution_path);
        ASSERT_EQ(status, QPL_STS_OK) << "Failed to gather deflate statistics\n";
        auto compression_table_buffer = std::make_unique<uint8_t[]>(QPL_COMPRESSION_TABLE_SIZE);
        auto *compression_table_ptr = reinterpret_cast<qpl_compression_huffman_table *>(compression_table_buffer.get());

        uint32_t representation_flags =
                QPL_DEFLATE_REPRESENTATION | ((execution_path == qpl_path_software) ? QPL_SW_REPRESENTATION
                                                                                    : QPL_HW_REPRESENTATION);

        status = qpl_build_compression_table(&histogram, compression_table_ptr, representation_flags);
        ASSERT_EQ(status, QPL_STS_OK) << "Failed to build the table\n";


        for (auto block_size: get_chunk_sizes()) {
            std::string error_message = "File name - " + dataset.first + ", block size = " + std::to_string(block_size);
            std::vector<uint8_t> compressed_source(source.size() * 2);
            std::vector<uint8_t> reference(source.size());
            status = compress_with_chunks<compression_mode::static_compression>(source,
                                                                                compressed_source,
                                                                                block_size,
                                                                                job_ptr,
                                                                                compression_table_ptr,
                                                                                qpl_default_level,
                                                                                false);

            ASSERT_EQ(status, QPL_STS_OK) << "Failed to compress job. " << error_message;

            status = decompress_with_chunks(compressed_source,
                                            reference,
                                            job_ptr,
                                            source.size());

            ASSERT_EQ(status, QPL_STS_OK) << "Failed to decompress job." << error_message;

            ASSERT_EQ(source, reference) << "Compressed and decompressed vectors missmatch!. " << error_message;
        }
    }
}

QPL_LOW_LEVEL_API_ALGORITHMIC_TEST(deflat, static_high_verify_stateful_compression) {
    auto execution_path = util::TestEnvironment::GetInstance().GetExecutionPath();

    if (execution_path == qpl_path_hardware) {
        GTEST_SKIP_("Hardware path doesn't support high level compression");
    }

    uint32_t job_size = 0;

    auto status = qpl_get_job_size(execution_path, &job_size);
    ASSERT_EQ(QPL_STS_OK, status) << "Failed to get job size\n";

    // Allocate buffers for decompression job
    auto job_buffer = std::make_unique<uint8_t[]>(job_size);
    auto job_ptr = reinterpret_cast<qpl_job *>(job_buffer.get());

    // Initialize decompression job
    status = qpl_init_job(execution_path, job_ptr);
    ASSERT_EQ(QPL_STS_OK, status) << "Failed to initialize job\n";

    for (auto &dataset: util::TestEnvironment::GetInstance().GetAlgorithmicDataset().get_data()) {
        std::vector<uint8_t> source;
        source = dataset.second;
        qpl_histogram histogram{};

        status = qpl_gather_deflate_statistics(source.data(), source.size(), &histogram, qpl_high_level,
                                               execution_path);
        ASSERT_EQ(status, QPL_STS_OK) << "Failed to gather deflate statistics\n";
        auto compression_table_buffer = std::make_unique<uint8_t[]>(QPL_COMPRESSION_TABLE_SIZE);
        auto *compression_table_ptr = reinterpret_cast<qpl_compression_huffman_table *>(compression_table_buffer.get());

        uint32_t representation_flags =
                QPL_DEFLATE_REPRESENTATION | ((execution_path == qpl_path_software) ? QPL_SW_REPRESENTATION
                                                                                    : QPL_HW_REPRESENTATION);

        status = qpl_build_compression_table(&histogram, compression_table_ptr, representation_flags);
        ASSERT_EQ(status, QPL_STS_OK) << "Failed to build the table\n";


        for (auto block_size: get_chunk_sizes()) {
            std::string error_message = "File name - " + dataset.first + ", block size = " + std::to_string(block_size);
            std::vector<uint8_t> compressed_source(source.size() * 2);
            std::vector<uint8_t> reference(source.size());
            status = compress_with_chunks<compression_mode::static_compression>(source,
                                                                                compressed_source,
                                                                                block_size,
                                                                                job_ptr,
                                                                                compression_table_ptr,
                                                                                qpl_high_level,
                                                                                false);

            ASSERT_EQ(status, QPL_STS_OK) << "Failed to compress job. " << error_message;

            status = decompress_with_chunks(compressed_source,
                                            reference,
                                            job_ptr,
                                            source.size());

            ASSERT_EQ(status, QPL_STS_OK) << "Failed to decompress job." << error_message;

            ASSERT_EQ(source, reference) << "Compressed and decompressed vectors missmatch!. " << error_message;
        }
    }
}

QPL_LOW_LEVEL_API_ALGORITHMIC_TEST(deflat, dynamic_start_new_block) {
    auto execution_path = util::TestEnvironment::GetInstance().GetExecutionPath();
    uint32_t job_size = 0;

    auto status = qpl_get_job_size(execution_path, &job_size);
    ASSERT_EQ(QPL_STS_OK, status) << "Failed to get job size\n";

    // Allocate buffers for decompression job
    auto job_buffer = std::make_unique<uint8_t[]>(job_size);
    auto job_ptr = reinterpret_cast<qpl_job *>(job_buffer.get());

    // Initialize decompression job
    status = qpl_init_job(execution_path, job_ptr);
    ASSERT_EQ(QPL_STS_OK, status) << "Failed to initialize job\n";

    for (auto &dataset: util::TestEnvironment::GetInstance().GetAlgorithmicDataset().get_data()) {
        std::vector<uint8_t> source;
        source = dataset.second;
        for (auto block_size: get_chunk_sizes()) {
            std::string error_message = "File name - " + dataset.first + ", block size = " + std::to_string(block_size);
            // status = qpl_init_job(execution_path, job_ptr);
            // ASSERT_EQ(QPL_STS_OK, status) << "Failed to initialize job\n";
            std::vector<uint8_t> compressed_source(source.size() * 2);
            std::vector<uint8_t> reference(source.size() * 2);

            uint32_t deflate_blocks_count = source.size() / block_size;

            if (source.size() % block_size != 0) {
                deflate_blocks_count++;
            }
            // Perform compression
            {
                // Configure job
                job_ptr->op = qpl_op_compress;
                job_ptr->flags = QPL_FLAG_FIRST | QPL_FLAG_DYNAMIC_HUFFMAN | QPL_FLAG_START_NEW_BLOCK | QPL_FLAG_OMIT_VERIFY;

                job_ptr->available_in = static_cast<uint32_t>(source.size());
                job_ptr->available_out = static_cast<uint32_t>(compressed_source.size());

                job_ptr->next_in_ptr = source.data();
                job_ptr->next_out_ptr = compressed_source.data();

                job_ptr->level = qpl_default_level;

                // Compress
                auto current_chunk_size = block_size;
                auto source_bytes_left = static_cast<uint32_t>(source.size());

                uint32_t iteration = 0;
                while (source_bytes_left > 0) {
                    if (current_chunk_size >= source_bytes_left) {
                        job_ptr->flags |= QPL_FLAG_LAST;
                        current_chunk_size = source_bytes_left;
                    }

                    source_bytes_left -= current_chunk_size;
                    job_ptr->next_in_ptr = source.data() + iteration * block_size;
                    job_ptr->available_in = current_chunk_size;
                    status = run_job_api(job_ptr);
                    ASSERT_EQ(status, QPL_STS_OK) << "Failed to compress, " << error_message << " , " << iteration;

                    job_ptr->flags &= ~QPL_FLAG_FIRST;
                    iteration++;
                }

                compressed_source.resize(job_ptr->total_out);
            }

            {
                // Use software path because it's easier to determinate deflate blocks count in that case
                status = qpl_init_job(qpl_path_software, job_ptr);
                ASSERT_EQ(QPL_STS_OK, status) << "Failed to initialize job\n";

                job_ptr->op = qpl_op_decompress;
                job_ptr->flags = QPL_FLAG_FIRST;
                job_ptr->available_in = static_cast<uint32_t>(compressed_source.size());
                job_ptr->next_in_ptr = compressed_source.data();
                job_ptr->available_out = static_cast<uint32_t>(reference.size());
                job_ptr->next_out_ptr = reference.data();
                job_ptr->decomp_end_processing = qpl_decomp_end_proc::qpl_stop_on_any_eob;

                // Assume that we have exactly N = deflate_blocks_count deflated blocks in compressed stream
                // So if we set qpl_stop_on_any_eob option, then we will be able to run decompress exactly N times without any fails
                // (N + 1)th iteration may not fail (probably decoder will spot a block of type btype == 0),
                // however we expect no output at this iteration
                for (uint32_t iteration_count = 0; iteration_count < deflate_blocks_count; iteration_count++) {

                    if (iteration_count == deflate_blocks_count - 1) {
                        job_ptr->flags |= QPL_FLAG_LAST;
                        job_ptr->decomp_end_processing = qpl_decomp_end_proc::qpl_stop_and_check_for_bfinal_eob;
                    }
                    status = run_job_api(job_ptr);
                    ASSERT_EQ(status, QPL_STS_OK) << "Failed to decompress job. " << error_message;
                    job_ptr->flags &= ~QPL_FLAG_FIRST;
                }
                uint32_t saved_output_bytes = job_ptr->total_out;

                status = run_job_api(job_ptr);
                ASSERT_EQ(job_ptr->total_out, saved_output_bytes) << "More deflate blocks found than expected! " << error_message;
                reference.resize(saved_output_bytes);
            }

            ASSERT_EQ(source, reference) << "Compressed and decompressed vectors missmatch!. " << error_message;
        }
    }
}

QPL_LOW_LEVEL_API_ALGORITHMIC_TEST(deflat, fixed_start_new_block) {
    auto execution_path = util::TestEnvironment::GetInstance().GetExecutionPath();
    uint32_t job_size = 0;

    auto status = qpl_get_job_size(execution_path, &job_size);
    ASSERT_EQ(QPL_STS_OK, status) << "Failed to get job size\n";

    // Allocate buffers for decompression job
    auto job_buffer = std::make_unique<uint8_t[]>(job_size);
    auto job_ptr = reinterpret_cast<qpl_job *>(job_buffer.get());

    // Initialize decompression job
    status = qpl_init_job(execution_path, job_ptr);
    ASSERT_EQ(QPL_STS_OK, status) << "Failed to initialize job\n";

    for (auto &dataset: util::TestEnvironment::GetInstance().GetAlgorithmicDataset().get_data()) {
        std::vector<uint8_t> source;
        source = dataset.second;
        for (auto block_size: get_chunk_sizes()) {
            std::string error_message = "File name - " + dataset.first + ", block size = " + std::to_string(block_size);
            // status = qpl_init_job(execution_path, job_ptr);
            // ASSERT_EQ(QPL_STS_OK, status) << "Failed to initialize job\n";
            std::vector<uint8_t> compressed_source(source.size() * 2);
            std::vector<uint8_t> reference(source.size() * 2);

            uint32_t deflate_blocks_count = source.size() / block_size;

            if (source.size() % block_size != 0) {
                deflate_blocks_count++;
            }
            // Perform compression
            {
                // Configure job
                job_ptr->op = qpl_op_compress;
                job_ptr->flags = QPL_FLAG_FIRST | QPL_FLAG_START_NEW_BLOCK | QPL_FLAG_OMIT_VERIFY;

                job_ptr->available_in = static_cast<uint32_t>(source.size());
                job_ptr->available_out = static_cast<uint32_t>(compressed_source.size());

                job_ptr->next_in_ptr = source.data();
                job_ptr->next_out_ptr = compressed_source.data();

                job_ptr->level = qpl_default_level;

                // Compress
                auto current_chunk_size = block_size;
                auto source_bytes_left = static_cast<uint32_t>(source.size());

                uint32_t iteration = 0;
                while (source_bytes_left > 0) {
                    if (current_chunk_size >= source_bytes_left) {
                        job_ptr->flags |= QPL_FLAG_LAST;
                        current_chunk_size = source_bytes_left;
                    }

                    source_bytes_left -= current_chunk_size;
                    job_ptr->next_in_ptr = source.data() + iteration * block_size;
                    job_ptr->available_in = current_chunk_size;
                    status = run_job_api(job_ptr);
                    ASSERT_EQ(status, QPL_STS_OK) << "Failed to compress, " << error_message;

                    job_ptr->flags &= ~QPL_FLAG_FIRST;
                    iteration++;
                }

                compressed_source.resize(job_ptr->total_out);
            }

            {
                // Use software path because it's easier to determinate deflate blocks count in that case
                status = qpl_init_job(qpl_path_software, job_ptr);
                ASSERT_EQ(QPL_STS_OK, status) << "Failed to initialize job\n";

                job_ptr->op = qpl_op_decompress;
                job_ptr->flags = QPL_FLAG_FIRST;
                job_ptr->available_in = static_cast<uint32_t>(compressed_source.size());
                job_ptr->next_in_ptr = compressed_source.data();
                job_ptr->available_out = static_cast<uint32_t>(reference.size());
                job_ptr->next_out_ptr = reference.data();
                job_ptr->decomp_end_processing = qpl_decomp_end_proc::qpl_stop_on_any_eob;

                // Assume that we have exactly N = deflate_blocks_count deflated blocks in compressed stream
                // So if we set qpl_stop_on_any_eob option, then we will be able to run decompress exactly N times without any fails
                // (N + 1)th iteration may not fail (probably decoder will spot a block of type btype == 0),
                // however we expect no output at this iteration
                for (uint32_t iteration_count = 0; iteration_count < deflate_blocks_count; iteration_count++) {

                    if (iteration_count == deflate_blocks_count - 1) {
                        job_ptr->flags |= QPL_FLAG_LAST;
                        job_ptr->decomp_end_processing = qpl_decomp_end_proc::qpl_stop_and_check_for_bfinal_eob;
                    }
                    status = run_job_api(job_ptr);
                    ASSERT_EQ(status, QPL_STS_OK) << "Failed to decompress job. " << error_message;
                    job_ptr->flags &= ~QPL_FLAG_FIRST;
                }
                uint32_t saved_output_bytes = job_ptr->total_out;

                status = run_job_api(job_ptr);
                ASSERT_EQ(job_ptr->total_out, saved_output_bytes) << "More deflate blocks found than expected! " << error_message;
                reference.resize(saved_output_bytes);
            }

            ASSERT_EQ(source, reference) << "Compressed and decompressed vectors missmatch!. " << error_message;
        }
    }
}

QPL_LOW_LEVEL_API_ALGORITHMIC_TEST(deflat, static_start_new_block) {
    auto execution_path = util::TestEnvironment::GetInstance().GetExecutionPath();
    uint32_t job_size = 0;

    auto status = qpl_get_job_size(execution_path, &job_size);
    ASSERT_EQ(QPL_STS_OK, status) << "Failed to get job size\n";

    // Allocate buffers for decompression job
    auto job_buffer = std::make_unique<uint8_t[]>(job_size);
    auto job_ptr = reinterpret_cast<qpl_job *>(job_buffer.get());

    // Initialize decompression job
    status = qpl_init_job(execution_path, job_ptr);
    ASSERT_EQ(QPL_STS_OK, status) << "Failed to initialize job\n";

    for (auto &dataset: util::TestEnvironment::GetInstance().GetAlgorithmicDataset().get_data()) {
        std::vector<uint8_t> source;
        source = dataset.second;
        qpl_histogram histogram{};

        status = qpl_gather_deflate_statistics(source.data(), source.size(), &histogram, qpl_default_level,
                                               execution_path);
        ASSERT_EQ(status, QPL_STS_OK) << "Failed to gather deflate statistics\n";
        auto compression_table_buffer = std::make_unique<uint8_t[]>(QPL_COMPRESSION_TABLE_SIZE);
        auto *compression_table_ptr = reinterpret_cast<qpl_compression_huffman_table *>(compression_table_buffer.get());

        uint32_t representation_flags =
                QPL_DEFLATE_REPRESENTATION | ((execution_path == qpl_path_software) ? QPL_SW_REPRESENTATION
                                                                                    : QPL_HW_REPRESENTATION);

        status = qpl_build_compression_table(&histogram, compression_table_ptr, representation_flags);
        ASSERT_EQ(status, QPL_STS_OK) << "Failed to build the table\n";

        for (auto block_size: get_chunk_sizes()) {
            std::string error_message = "File name - " + dataset.first + ", block size = " + std::to_string(block_size);
            // status = qpl_init_job(execution_path, job_ptr);
            // ASSERT_EQ(QPL_STS_OK, status) << "Failed to initialize job\n";
            std::vector<uint8_t> compressed_source(source.size() * 2);
            std::vector<uint8_t> reference(source.size() * 2);

            uint32_t deflate_blocks_count = source.size() / block_size;

            if (source.size() % block_size != 0) {
                deflate_blocks_count++;
            }
            // Perform compression
            {
                // Configure job
                job_ptr->op = qpl_op_compress;
                job_ptr->flags = QPL_FLAG_FIRST | QPL_FLAG_START_NEW_BLOCK | QPL_FLAG_OMIT_VERIFY;

                job_ptr->available_in = static_cast<uint32_t>(source.size());
                job_ptr->available_out = static_cast<uint32_t>(compressed_source.size());

                job_ptr->next_in_ptr = source.data();
                job_ptr->next_out_ptr = compressed_source.data();
                job_ptr->compression_huffman_table = compression_table_ptr;

                job_ptr->level = qpl_default_level;

                // Compress
                auto current_chunk_size = block_size;
                auto source_bytes_left = static_cast<uint32_t>(source.size());

                uint32_t iteration = 0;
                while (source_bytes_left > 0) {
                    if (current_chunk_size >= source_bytes_left) {
                        job_ptr->flags |= QPL_FLAG_LAST;
                        current_chunk_size = source_bytes_left;
                    }

                    source_bytes_left -= current_chunk_size;
                    job_ptr->next_in_ptr = source.data() + iteration * block_size;
                    job_ptr->available_in = current_chunk_size;
                    status = run_job_api(job_ptr);
                    ASSERT_EQ(status, QPL_STS_OK) << "Failed to compress, " << error_message;

                    job_ptr->flags &= ~QPL_FLAG_FIRST;
                    iteration++;
                }

                compressed_source.resize(job_ptr->total_out);
            }

            {
                // Use software path because it's easier to determinate deflate blocks count in that case
                status = qpl_init_job(qpl_path_software, job_ptr);
                ASSERT_EQ(QPL_STS_OK, status) << "Failed to initialize job\n";

                job_ptr->op = qpl_op_decompress;
                job_ptr->flags = QPL_FLAG_FIRST;
                job_ptr->available_in = static_cast<uint32_t>(compressed_source.size());
                job_ptr->next_in_ptr = compressed_source.data();
                job_ptr->available_out = static_cast<uint32_t>(reference.size());
                job_ptr->next_out_ptr = reference.data();
                job_ptr->decomp_end_processing = qpl_decomp_end_proc::qpl_stop_on_any_eob;

                // Assume that we have exactly N = deflate_blocks_count deflated blocks in compressed stream
                // So if we set qpl_stop_on_any_eob option, then we will be able to run decompress exactly N times without any fails
                // (N + 1)th iteration may not fail (probably decoder will spot a block of type btype == 0),
                // however we expect no output at this iteration
                for (uint32_t iteration_count = 0; iteration_count < deflate_blocks_count; iteration_count++) {

                    if (iteration_count == deflate_blocks_count - 1) {
                        job_ptr->flags |= QPL_FLAG_LAST;
                        job_ptr->decomp_end_processing = qpl_decomp_end_proc::qpl_stop_and_check_for_bfinal_eob;
                    }
                    status = run_job_api(job_ptr);
                    ASSERT_EQ(status, QPL_STS_OK) << "Failed to decompress job. " << error_message;
                    job_ptr->flags &= ~QPL_FLAG_FIRST;
                }
                uint32_t saved_output_bytes = job_ptr->total_out;

                status = run_job_api(job_ptr);
                ASSERT_EQ(job_ptr->total_out, saved_output_bytes) << "More deflate blocks found than expected! " << error_message;
                reference.resize(saved_output_bytes);
            }

            ASSERT_EQ(source, reference) << "Compressed and decompressed vectors missmatch!. " << error_message;
        }
    }
}

}