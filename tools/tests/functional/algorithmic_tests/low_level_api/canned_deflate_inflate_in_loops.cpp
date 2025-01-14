/*******************************************************************************
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#include <array>
#include <memory>
#include "operation_test.hpp"
#include "source_provider.hpp"
#include "ta_ll_common.hpp"

namespace qpl::test {
template <class Iterator>
auto init_huffman_table(qpl_huffman_table_t huffman_table,
                                    Iterator begin,
                                    Iterator end,
                                    qpl_compression_levels level,
                                    qpl_path_t path) -> void {
    auto           *source_ptr = &*begin;
    const uint32_t source_size = std::distance(begin, end);

    qpl_histogram deflate_histogram{};

    auto status = qpl_gather_deflate_statistics(source_ptr,
                                                source_size,
                                                &deflate_histogram,
                                                level,
                                                path);

    ASSERT_EQ(status, QPL_STS_OK) << "Failed to gather statistics";

    status = qpl_huffman_table_init_with_histogram(huffman_table, &deflate_histogram);

    ASSERT_EQ(status, QPL_STS_OK) << "Failed to build compression table";
}


QPL_LOW_LEVEL_API_ALGORITHMIC_TEST_F(deflate_inflate_canned_in_loops, default_level, JobFixture) {
    auto path = GetExecutionPath();

    // TODO investigate and fix software path failure (compressed sizes differ across iterations)
    if (path == qpl_path_software) {
        GTEST_SKIP() << "Skip deflate_inflate_canned_in_loops test on software path";
    }

    for (auto &dataset: util::TestEnvironment::GetInstance().GetAlgorithmicDataset().get_data()) {
        source = dataset.second;

        destination.resize(source.size() * 2);
        std::vector<uint8_t> reference_buffer(destination.size(), 0u);

        const uint32_t file_size = (uint32_t) source.size();
        ASSERT_NE(0u, file_size) << "Couldn't open file: "
                                 << dataset.first;

        uint32_t size = 0;
        qpl_status status = qpl_get_job_size(path, &size);
        ASSERT_EQ(QPL_STS_OK, status) << "Failed to get job size";

        auto job_buffer = std::make_unique<uint8_t[]>(size);
        job_ptr = reinterpret_cast<qpl_job *>(job_buffer.get());

        // Init job for a file
        status = qpl_init_job(path, job_ptr);
        ASSERT_EQ(QPL_STS_OK, status) << "Failed to init job";

        uint32_t compressed_size = -1;
        uint32_t decompressed_size = -1;

        // Submit deflate and inflate jobs in loops using the same job object
        for (int loop = 0; loop < 10; loop++) {
            qpl_huffman_table_t huffman_table;
            auto ht_destroy_status = QPL_STS_OK;

            auto status = qpl_deflate_huffman_table_create(combined_table_type,
                                                           path,
                                                           DEFAULT_ALLOCATOR_C,
                                                           &huffman_table);
            ASSERT_EQ(status, QPL_STS_OK) << "Table creation failed";

            init_huffman_table(huffman_table,
                               source.data(),
                               source.data() + file_size,
                               qpl_default_level,
                               path);
            ASSERT_EQ(QPL_STS_OK, status) << "Failed to build huffman table";

            // Configure compression job fields
            job_ptr->op            = qpl_op_compress;
            job_ptr->level         = qpl_default_level;
            job_ptr->next_in_ptr   = source.data();
            job_ptr->available_in  = file_size;
            job_ptr->next_out_ptr  = destination.data();
            job_ptr->available_out = static_cast<uint32_t>(destination.size());
            job_ptr->huffman_table = huffman_table;
            job_ptr->flags         = QPL_FLAG_FIRST |
                                     QPL_FLAG_LAST |
                                     QPL_FLAG_OMIT_VERIFY |
                                     QPL_FLAG_CANNED_MODE;

            status = run_job_api(job_ptr);
            if (QPL_STS_OK != status) {
                ht_destroy_status = qpl_huffman_table_destroy(huffman_table);
                ASSERT_EQ(QPL_STS_OK, ht_destroy_status) << "Huffman table destruction failed when exiting the test upon compression failure";
            }
            ASSERT_EQ(QPL_STS_OK, status) << "Compression failed";

            destination.resize(job_ptr->total_out);

            // Check if the compressed size is the same as in the previous loop
            if (loop != 0) {
                EXPECT_EQ(compressed_size, job_ptr->total_out) << "File: " + dataset.first;
            }
            compressed_size = job_ptr->total_out;

            // Configure decompression job fields
            job_ptr->op            = qpl_op_decompress;
            job_ptr->next_in_ptr   = destination.data();
            job_ptr->available_in  = job_ptr->total_out;
            job_ptr->next_out_ptr  = reference_buffer.data();
            job_ptr->available_out = static_cast<uint32_t>(reference_buffer.size());
            job_ptr->flags         = QPL_FLAG_FIRST | QPL_FLAG_LAST | QPL_FLAG_CANNED_MODE;
            job_ptr->huffman_table = huffman_table;

            status = run_job_api(job_ptr);
            if (QPL_STS_OK != status) {
                ht_destroy_status = qpl_huffman_table_destroy(huffman_table);
                ASSERT_EQ(QPL_STS_OK, ht_destroy_status) << "Huffman table destruction failed when exiting the test upon decompression failure";
            }
            ASSERT_EQ(QPL_STS_OK, status) << "Decompression failed";

            reference_buffer.resize(job_ptr->total_out);

            // Check if the decompressed size is the same as in the previous loop
            if (decompressed_size != -1) {
                EXPECT_EQ(decompressed_size, job_ptr->total_out) << "File: " + dataset.first;
            }
            decompressed_size = job_ptr->total_out;

            EXPECT_EQ(source.size(), reference_buffer.size());
            EXPECT_TRUE(CompareVectors(reference_buffer,
                                       source,
                                       file_size,
                                       "File: " + dataset.first));

            status = qpl_huffman_table_destroy(huffman_table);
            ASSERT_EQ(QPL_STS_OK, status) << "Huffman table destruction failed";

            destination.resize(source.size() * 2);
            reference_buffer.resize(source.size());
       }

       status = qpl_fini_job(job_ptr);
       ASSERT_EQ(QPL_STS_OK, status) << "Failed to fini job";
    }
}
}
