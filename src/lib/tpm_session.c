//**********************************************************************;
// Copyright (c) 2015, Intel Corporation
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//**********************************************************************;

#include <stdbool.h>
#include <stdlib.h>

#include <sapi/tpm20.h>

#include "log.h"
#include "tpm_kdfa.h"
#include "tpm_session.h"
#include "tpm2_alg_util.h"
#include "tpm2_util.h"

//
// This is a wrapper function around the TPM2_StartAuthSession command.
// It performs the command, calculates the session key, and updates a
// SESSION structure.
//
static TPM_RC StartAuthSession(TSS2_SYS_CONTEXT *sapi_context, SESSION *session )
{
    TPM_RC rval;
    TPM2B_ENCRYPTED_SECRET key;
    char label[] = "ATH";
    UINT16 bytes;
    int i;

    key.t.size = 0;

    if( session->nonceOlder.t.size == 0 )
    {
        session->nonceOlder.t.size = tpm2_alg_util_get_hash_size( TPM_ALG_SHA1 );
        for( i = 0; i < session->nonceOlder.t.size; i++ )
            session->nonceOlder.t.buffer[i] = 0;
    }

    session->nonceNewer.t.size = session->nonceOlder.t.size;
    rval = Tss2_Sys_StartAuthSession( sapi_context, session->tpmKey, session->bind, 0,
            &( session->nonceOlder ), &( session->encryptedSalt ), session->sessionType,
            &( session->symmetric ), session->authHash, &( session->sessionHandle ),
            &( session->nonceNewer ), 0 );

    if( rval == TPM_RC_SUCCESS )
    {
        if( session->tpmKey == TPM_RH_NULL )
            session->salt.t.size = 0;
        if( session->bind == TPM_RH_NULL )
            session->authValueBind.t.size = 0;

        if( session->tpmKey == TPM_RH_NULL && session->bind == TPM_RH_NULL )
        {
            session->sessionKey.b.size = 0;
        }
        else
        {
            // Generate the key used as input to the KDF.
            // Generate the key used as input to the KDF.
            bool result = tpm2_util_concat_buffer( (TPM2B_MAX_BUFFER *)&key, &( session->authValueBind.b ) );
            if (!result)
            {
               return TSS2_SYS_RC_BAD_VALUE;
            }

            result = tpm2_util_concat_buffer( (TPM2B_MAX_BUFFER *)&key, &( session->salt.b ) );
            if (!result)
            {
                return TSS2_SYS_RC_BAD_VALUE;
            }

            bytes = tpm2_alg_util_get_hash_size( session->authHash );

            if( key.t.size == 0 )
            {
                session->sessionKey.t.size = 0;
            }
            else
            {
                rval = tpm_kdfa(session->authHash, &(key.b), label, &( session->nonceNewer.b ),
                        &( session->nonceOlder.b ), bytes * 8, (TPM2B_MAX_BUFFER *)&( session->sessionKey ) );
            }

            if( rval != TPM_RC_SUCCESS )
            {
                return( TSS2_APP_RC_CREATE_SESSION_KEY_FAILED );
            }
        }

        session->nonceTpmDecrypt.b.size = 0;
        session->nonceTpmEncrypt.b.size = 0;
        session->nvNameChanged = 0;
    }

    return rval;
}

void tpm_session_auth_end(SESSION *session ) {

    free(session);
}

//
// This version of StartAuthSession initializes the fields
// of the session structure using the passed in
// parameters, then calls StartAuthSession
// with just a pointer to the session structure.
// This allows all params to be set in one line of code when
// the function is called; cleaner this way, for
// some uses.
//
TPM_RC tpm_session_start_auth_with_params(TSS2_SYS_CONTEXT *sapi_context, SESSION **session,
    TPMI_DH_OBJECT tpmKey, TPM2B_MAX_BUFFER *salt,
    TPMI_DH_ENTITY bind, TPM2B_AUTH *bindAuth, TPM2B_NONCE *nonceCaller,
    TPM2B_ENCRYPTED_SECRET *encryptedSalt,
    TPM_SE sessionType, TPMT_SYM_DEF *symmetric, TPMI_ALG_HASH algId )
{
    TPM_RC rval;

    *session = calloc(1, sizeof(**session));
    if (!*session) {
        LOG_ERR("oom");
        return TPM_RC_MEMORY;
    }

    // Copy handles to session struct.
    (*session)->bind = bind;
    (*session)->tpmKey = tpmKey;

    // Copy nonceCaller to nonceOlder in session struct.
    // This will be used as nonceCaller when StartAuthSession
    // is called.
    memcpy( &(*session)->nonceOlder.b, &nonceCaller->b, sizeof(nonceCaller->b));

    // Copy encryptedSalt
    memcpy( &(*session)->encryptedSalt.b, &encryptedSalt->b, sizeof(encryptedSalt->b));

    // Copy sessionType.
    (*session)->sessionType = sessionType;

    // Init symmetric.
    (*session)->symmetric.algorithm = symmetric->algorithm;
    (*session)->symmetric.keyBits.sym = symmetric->keyBits.sym;
    (*session)->symmetric.mode.sym = symmetric->mode.sym;
    (*session)->authHash = algId;

    // Copy bind' authValue.
    if( bindAuth == 0 )
    {
        (*session)->authValueBind.b.size = 0;
    }
    else
    {
        memcpy( &( (*session)->authValueBind.b ), &( bindAuth->b ), sizeof(bindAuth->b));
    }

    // Calculate sessionKey
    if( (*session)->tpmKey == TPM_RH_NULL )
    {
        (*session)->salt.t.size = 0;
    }
    else
    {
        memcpy( &(*session)->salt.b, &salt->b, sizeof(salt->b));
    }

    if( (*session)->bind == TPM_RH_NULL )
        (*session)->authValueBind.t.size = 0;

    rval = StartAuthSession(sapi_context, *session );

    return rval;
}
