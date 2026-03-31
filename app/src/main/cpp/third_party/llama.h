/**
 * llama.h - Simplified llama.cpp API header
 * 
 * This is a simplified header for llama.cpp integration.
 * For full functionality, replace with the actual llama.h from:
 * https://github.com/ggerganov/llama.cpp
 * 
 * Usage:
 * 1. Download llama.cpp source: git clone https://github.com/ggerganov/llama.cpp
 * 2. Copy llama.h, ggml.h, ggml-alloc.h, ggml-backend.h to this directory
 * 3. Copy llama.cpp, ggml.c, ggml-alloc.cpp, ggml-backend.cpp to cpp/src/
 * 4. Update CMakeLists.txt to compile these files
 */

#ifndef LLAMA_H
#define LLAMA_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Model context parameters
struct llama_context_params {
    uint32_t seed;              // RNG seed
    uint32_t n_ctx;             // text context, 0 = from model
    uint32_t n_batch;           // logical maximum batch size
    uint32_t n_ubatch;          // physical maximum batch size
    uint32_t n_seq_max;         // max number of sequences
    uint32_t n_threads;         // number of threads
    uint32_t n_threads_batch;   // number of threads for batch processing
    
    // for quantize-only
    bool lora_base;
    bool embedding;
    bool offload_kqv;
    bool flash_attn;
    
    float rope_freq_base;       // RoPE base frequency
    float rope_freq_scale;      // RoPE frequency scaling factor
    
    float defrag_thold;         // defragment the KV cache if holes/size > thold
    
    // Keep the booleans together to avoid misalignment during copy.
    bool logits_all;            // the llama_eval() call computes all logits
    bool warmup;                // warmup run
};

// Model parameters
struct llama_model_params {
    int32_t n_gpu_layers;       // number of layers to store in VRAM
    int32_t main_gpu;           // the GPU that is used for the entire model
    float   tensor_split[128];  // how split tensors should be distributed across GPUs
    int32_t n_gpu_layers_draft; // number of layers to store in VRAM for the draft model
    
    // Keep the booleans together to avoid misalignment during copy.
    bool progress_callback;
    bool vocab_only;            // only load the vocabulary
    bool use_mmap;              // use mmap if possible
    bool use_mlock;             // force system to keep model in RAM
    bool check_tensors;         // validate model tensor data
};

// Tokenization
struct llama_model;
struct llama_context;
struct llama_sampler;

typedef int32_t llama_token;
typedef struct llama_model llama_model;
typedef struct llama_context llama_context;

// Token attributes
enum llama_token_type {
    LLAMA_TOKEN_TYPE_UNDEFINED    = 0,
    LLAMA_TOKEN_TYPE_NORMAL       = 1,
    LLAMA_TOKEN_TYPE_UNKNOWN      = 2,
    LLAMA_TOKEN_TYPE_CONTROL      = 3,
    LLAMA_TOKEN_TYPE_USER_DEFINED = 4,
    LLAMA_TOKEN_TYPE_UNUSED       = 5,
    LLAMA_TOKEN_TYPE_BYTE         = 6,
};

// Initialize the library
void llama_backend_init(void);
void llama_backend_free(void);

// Model loading
struct llama_model_params llama_model_default_params(void);
struct llama_context_params llama_context_default_params(void);

struct llama_model * llama_load_model_from_file(
        const char * path_model,
        struct llama_model_params params);

void llama_free_model(struct llama_model * model);

// Context creation
struct llama_context * llama_new_context_with_model(
        struct llama_model * model,
        struct llama_context_params params);

void llama_free(struct llama_context * ctx);

// Tokenization
llama_token llama_vocab_bos(const struct llama_model * model); // beginning-of-sentence
llama_token llama_vocab_eos(const struct llama_model * model); // end-of-sentence
llama_token llama_vocab_sep(const struct llama_model * model); // sentence separator

int32_t llama_tokenize(
        const struct llama_model * model,
        const char * text,
        int32_t   text_len,
        llama_token * tokens,
        int32_t   n_tokens_max,
        bool   add_bos,
        bool   special);

// Token to string
int32_t llama_token_to_piece(
        const struct llama_model * model,
        llama_token   token,
        char * piece,
        int32_t   piece_size,
        int32_t   lstrip,
        bool   special);

// Batch evaluation
struct llama_batch {
    llama_token * token;
    float      * embd;
    int32_t    * pos;
    int32_t    * n_seq_id;
    llama_token ** seq_id;
    int8_t     * logits;

    int32_t n_tokens;
};

struct llama_batch llama_batch_init(int32_t n_tokens, int32_t embd, int32_t n_seq_max);
void llama_batch_free(struct llama_batch batch);

// Decode batch
int32_t llama_decode(
        struct llama_context * ctx,
        struct llama_batch   batch);

// Get logits
float * llama_get_logits(struct llama_context * ctx);
float * llama_get_logits_ith(struct llama_context * ctx, int32_t i);

// KV cache management
void llama_kv_cache_clear(struct llama_context * ctx);

// Model info
uint32_t llama_n_ctx(const struct llama_context * ctx);
int32_t  llama_n_vocab(const struct llama_model * model);

// Sampling (simplified)
struct llama_sampler * llama_sampler_init_greedy(void);
struct llama_sampler * llama_sampler_init_temp(float temp);
llama_token llama_sampler_sample(struct llama_sampler * smpl, struct llama_context * ctx, int32_t idx);
void llama_sampler_free(struct llama_sampler * smpl);

// Performance timing
void llama_perf_context_print(const struct llama_context * ctx);

#ifdef __cplusplus
}
#endif

#endif // LLAMA_H
