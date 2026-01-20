/**
 * data_processor_c_wrapper.c
 * 
 * C wrapper for C++ DataProcessor class
 * Allows C code (main.c) to use the DataProcessor
 */

#include "data_processor_c_wrapper.h"
#include "data_processor.h"
#include <stdlib.h>

// Opaque handle to C++ DataProcessor object
struct data_processor_handle {
    DataProcessor *processor;
};

data_processor_handle_t* data_processor_create(void) {
    data_processor_handle_t *handle = (data_processor_handle_t*)malloc(sizeof(data_processor_handle_t));
    if (handle == NULL) {
        return NULL;
    }
    
    handle->processor = new DataProcessor();
    if (handle->processor == NULL) {
        free(handle);
        return NULL;
    }
    
    return handle;
}

void data_processor_destroy(data_processor_handle_t *handle) {
    if (handle != NULL) {
        if (handle->processor != NULL) {
            delete handle->processor;
        }
        free(handle);
    }
}

void data_processor_set_sync_window(data_processor_handle_t *handle, uint32_t window_ms) {
    if (handle != NULL && handle->processor != NULL) {
        handle->processor->setSyncWindow(window_ms);
    }
}

uint32_t data_processor_get_sync_window(data_processor_handle_t *handle) {
    if (handle != NULL && handle->processor != NULL) {
        return handle->processor->getSyncWindow();
    }
    return 0;
}

bool data_processor_synchronize_data(data_processor_handle_t *handle,
                                     const tof_data_t *tof,
                                     const imu_data_t *imu,
                                     synced_data_t *output) {
    if (handle != NULL && handle->processor != NULL) {
        return handle->processor->synchronizeData(*tof, *imu, *output);
    }
    return false;
}

void data_processor_extract_features(data_processor_handle_t *handle,
                                     const synced_data_t *data,
                                     feature_vector_t *features) {
    if (handle != NULL && handle->processor != NULL) {
        handle->processor->extractFeatures(*data, *features);
    }
}

bool data_processor_detect_near_miss(data_processor_handle_t *handle,
                                     const feature_vector_t *features) {
    if (handle != NULL && handle->processor != NULL) {
        return handle->processor->detectNearMiss(*features);
    }
    return false;
}

