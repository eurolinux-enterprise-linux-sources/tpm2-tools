/***********************************************************************
 * Copyright (c) 2015 - 2017, Intel Corporation
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 ***********************************************************************/

#include <inttypes.h>
#include <string.h>

#include "tss2_mu.h"
#include "sapi/tpm20.h"
#include "tss2_endian.h"
#include "log.h"

#define TPMA_MARSHAL(type) \
TSS2_RC Tss2_MU_##type##_Marshal(type src, uint8_t buffer[], \
                                 size_t buffer_size, size_t *offset) \
{ \
    size_t  local_offset = 0; \
\
    if (offset != NULL) { \
        LOG_INFO( "offset non-NULL, initial value: %zu", *offset); \
        local_offset = *offset; \
    } \
\
    if (buffer == NULL && offset == NULL) { \
        LOG_WARN( "buffer and offset parameter are NULL"); \
        return TSS2_TYPES_RC_BAD_REFERENCE; \
    } else if (buffer == NULL && offset != NULL) { \
        *offset += sizeof (src); \
        LOG_INFO( "buffer NULL and offset non-NULL, updating offset to %zu", \
             *offset); \
        return TSS2_RC_SUCCESS; \
    } else if (buffer_size < local_offset || \
               buffer_size - local_offset < sizeof (src)) \
    { \
        LOG_WARN( \
             "buffer_size: %zu with offset: %zu are insufficient for object " \
             "of size %zu", \
             buffer_size, \
             local_offset, \
             sizeof (src)); \
        return TSS2_TYPES_RC_INSUFFICIENT_BUFFER; \
    } \
\
    LOG_INFO( \
         "Marshalling " #type " from 0x%" PRIxPTR " to buffer 0x%" PRIxPTR \
         " at index 0x%zx", \
         (uintptr_t)&src, \
         (uintptr_t)buffer, \
         local_offset); \
\
    switch (sizeof(src.val)) { \
        case 1: \
            break; \
        case 2: \
            src.val = HOST_TO_BE_16(src.val); \
            break; \
        case 4: \
            src.val = HOST_TO_BE_32(src.val); \
            break; \
        case 8: \
            src.val = HOST_TO_BE_64(src.val); \
            break; \
\
    } \
    memcpy (&buffer [local_offset], &src, sizeof(src.val)); \
    if (offset != NULL) { \
        *offset = local_offset + sizeof (src.val); \
        LOG_INFO( "offset parameter non-NULL, updated to %zu", *offset); \
    } \
\
    return TSS2_RC_SUCCESS; \
}

#define TPMA_UNMARSHAL(type) \
TSS2_RC Tss2_MU_##type##_Unmarshal(uint8_t const buffer[], size_t buffer_size, \
                                   size_t *offset, type *dest) \
{ \
    size_t  local_offset = 0; \
    type tmp; \
    memset(&tmp, 0, sizeof(tmp)); \
\
    if (offset != NULL) { \
        LOG_INFO( "offset non-NULL, initial value: %zu", *offset); \
        local_offset = *offset; \
    } \
\
    if (buffer == NULL || (dest == NULL && offset == NULL)) { \
        LOG_WARN( "buffer or dest and offset parameter are NULL"); \
        return TSS2_TYPES_RC_BAD_REFERENCE; \
    } else if (dest == NULL && offset != NULL) { \
        *offset += sizeof (type); \
        LOG_INFO( \
             "buffer NULL and offset non-NULL, updating offset to %zu", \
             *offset); \
        return TSS2_RC_SUCCESS; \
    } else if (buffer_size < local_offset || \
               sizeof (*dest) > buffer_size - local_offset) \
    { \
        LOG_WARN( \
             "buffer_size: %zu with offset: %zu are insufficient for object " \
             "of size %zu", \
             buffer_size, \
             local_offset, \
             sizeof (*dest)); \
        return TSS2_TYPES_RC_INSUFFICIENT_BUFFER; \
    } \
\
    LOG_INFO( \
         "Unmarshalling " #type " from 0x%" PRIxPTR " to buffer 0x%" PRIxPTR \
         " at index 0x%zx", \
         (uintptr_t)buffer, \
         (uintptr_t)dest, \
         local_offset); \
\
    memcpy (&tmp, &buffer [local_offset], sizeof (tmp)); \
\
    switch (sizeof(tmp.val)) { \
        case 1: \
            dest->val = tmp.val; \
            break; \
        case 2: \
            dest->val = BE_TO_HOST_16(tmp.val); \
            break; \
        case 4: \
            dest->val = BE_TO_HOST_32(tmp.val); \
            break; \
        case 8: \
            dest->val = BE_TO_HOST_64(tmp.val); \
            break; \
\
    } \
\
    if (offset != NULL) { \
        *offset = local_offset + sizeof (dest->val); \
        LOG_INFO( "offset parameter non-NULL, updated to %zu", *offset); \
    } \
    return TSS2_RC_SUCCESS; \
}

/*
 * These macros expand to (un)marshal functions for each of the TPMA types
 * the specification part 2.
 */
TPMA_MARSHAL  (TPMA_ALGORITHM);
TPMA_UNMARSHAL(TPMA_ALGORITHM);
TPMA_MARSHAL  (TPMA_CC);
TPMA_UNMARSHAL(TPMA_CC);
TPMA_MARSHAL  (TPMA_LOCALITY);
TPMA_UNMARSHAL(TPMA_LOCALITY);
TPMA_MARSHAL  (TPMA_NV);
TPMA_UNMARSHAL(TPMA_NV);
TPMA_MARSHAL  (TPMA_OBJECT);
TPMA_UNMARSHAL(TPMA_OBJECT);
TPMA_MARSHAL  (TPMA_PERMANENT);
TPMA_UNMARSHAL(TPMA_PERMANENT);
TPMA_MARSHAL  (TPMA_SESSION);
TPMA_UNMARSHAL(TPMA_SESSION);
TPMA_MARSHAL  (TPMA_STARTUP_CLEAR);
TPMA_UNMARSHAL(TPMA_STARTUP_CLEAR);
