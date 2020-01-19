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
// 3. Neither the name of Intel Corporation nor the names of its contributors
// may be used to endorse or promote products derived from this software without
// specific prior written permission.
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

#include <stdarg.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <getopt.h>

#include <sapi/tpm20.h>
#include <tcti/tcti_socket.h>
#include "common.h"

int debugLevel = 0;
UINT32 nvIndex = 0;
UINT32 authHandle = TPM_RH_PLATFORM;
UINT32 size = 0;
UINT32 nvAttribute = 0;
char handlePasswd[sizeof(TPMU_HA)];
char indexPasswd[sizeof(TPMU_HA)];
bool hexPasswd = false;

int nvSpaceDefine()
{
    UINT32 rval;
    TPM2B_AUTH nvAuth;
    TPM2B_NV_PUBLIC publicInfo;
    TPMS_AUTH_COMMAND sessionData;
    TPMS_AUTH_RESPONSE sessionDataOut;
    TSS2_SYS_CMD_AUTHS sessionsData;
    TSS2_SYS_RSP_AUTHS sessionsDataOut;

    TPMS_AUTH_COMMAND *sessionDataArray[1] = { &sessionData };
    TPMS_AUTH_RESPONSE *sessionDataOutArray[1] = { &sessionDataOut };

    sessionsDataOut.rspAuths = &sessionDataOutArray[0];
    sessionsData.cmdAuths = &sessionDataArray[0];

    sessionsDataOut.rspAuthsCount = 1;
    sessionsData.cmdAuthsCount = 1;

    sessionData.sessionHandle = TPM_RS_PW;
    sessionData.nonce.t.size = 0;
    sessionData.hmac.t.size = 0;
    *( (UINT8 *)((void *)&sessionData.sessionAttributes ) ) = 0;

    if (strlen(handlePasswd) > 0 && !hexPasswd)
    {
        sessionData.hmac.t.size = strlen(handlePasswd);
        memcpy( &sessionData.hmac.t.buffer[0], handlePasswd, sessionData.hmac.t.size );
    }
    else if (strlen(handlePasswd) > 0 && hexPasswd)
    {
        sessionData.hmac.t.size = sizeof(sessionData.hmac) - 2;
        if (hex2ByteStructure(handlePasswd, &sessionData.hmac.t.size,
                              sessionData.hmac.t.buffer) != 0)
        {
            printf( "Failed to convert Hex format password for handlePasswd.\n");
            return -1;
        }
    }

    publicInfo.t.size = sizeof( TPMI_RH_NV_INDEX ) +
            sizeof( TPMI_ALG_HASH ) + sizeof( TPMA_NV ) + sizeof( UINT16) +
            sizeof( UINT16 );
    publicInfo.t.nvPublic.nvIndex = nvIndex;
    publicInfo.t.nvPublic.nameAlg = TPM_ALG_SHA256;

    // Now set the attributes.
    publicInfo.t.nvPublic.attributes.val = nvAttribute;
    publicInfo.t.nvPublic.authPolicy.t.size = 0;
    publicInfo.t.nvPublic.dataSize = size;

    nvAuth.t.size = 0;
    if (strlen(indexPasswd) > 0 && !hexPasswd)
    {
        nvAuth.t.size = strlen(indexPasswd);
        memcpy( &nvAuth.t.buffer[0], indexPasswd, nvAuth.t.size );
    }
    else if (strlen(indexPasswd) > 0 && hexPasswd)
    {
        nvAuth.t.size = sizeof(nvAuth) - 2;
        if (hex2ByteStructure(indexPasswd,&nvAuth.t.size,nvAuth.t.buffer) != 0)
        {
            printf( "Failed to convert Hex format password for indexPasswd.\n");
            return -1;
        }
    }

    rval = Tss2_Sys_NV_DefineSpace( sysContext, authHandle, &sessionsData, &nvAuth, &publicInfo, &sessionsDataOut );
    if(rval != TPM_RC_SUCCESS)
    {
        printf( "\nFailed to define NV area at index 0x%x (%d).Error:0x%x\n", nvIndex, nvIndex, rval );
        return -1;
    }
    printf( "Success to define NV area at index 0x%x (%d).\n", nvIndex, nvIndex);

    return 0;
}

void showHelp(const char *name)
{
    printf("Usage: %s [-h/--help]\n"
           "   or: %s [-v/--version]\n"
           "   or: %s [-x/--index <nvIdx>] [-a/--authHandle <hexHandle>] [-s/--size <size>] [-t/--attribute <attributeDWord>]\n"
           "                     [-P/--handlePasswd <string>] [-I/--indexPasswd <string>]\n"
           "   or: %s [-x/--index <nvIdx>] [-a/--authHandle <hexHandle>] [-s/--size <size>] [-t/--attribute <attributeDWord>]\n"
           "                     [-P/--handlePasswd <string>] [-I/--indexPasswd <string>]\n"
           "                     [-X/--passwdInHex]\n"
           "                     [-p/--port <port>] [-d/--dbg <dbgLevel>]\n"
           "\nwhere:\n\n"
           "   -h/--help                       display this help and exit.\n"
           "   -v/--version                    display version information and exit.\n"
           "   -x/--index <nvIdx>              specifies the index of the NV area.\n"
           "   -a/--authHandle <hexHandle>     specifies the handle used to authorize:\n"
           "                                     0x40000001 (TPM_RH_OWNER)\n"
           "                                     0x4000000C (TPM_RH_PLATFORM)\n"
           "   -s/--size <size>                specifies the size of data area.\n"
           "   -t/--attribute <attributeDWord> specifies the value of attribute in publicInfo struct (need calculate outside).\n"
           "   -P/--handlePasswd <string>      specifies the password of authHandle.\n"
           "   -I/--indexPasswd <string>       specifies the password of NV Index when created.\n"
           "   -X/--passwdInHex                passwords given by any options are hex format.\n"
           "   -p/--port <port>                specifies the port number, default:%d, optional\n"
           "   -d/--dbg <dbgLevel>             specifies level of debug messages, optional:\n"
           "                                     0 (high level test results)\n"
           "                                     1 (test app send/receive byte streams)\n"
           "                                     2 (resource manager send/receive byte streams)\n"
           "                                     3 (resource manager tables)\n"
           "\nexample:\n"
           "   %s -x 0x1500016 -a 0x40000001 -s 32 -t 0x2000A\n"
           "   %s -x 0x1500016 -a 0x40000001 -s 32 -t 0x2000A -I 1a1b1c -X\n"
           , name, name, name, name, DEFAULT_RESMGR_TPM_PORT, name, name);
}

int main(int argc, char* argv[])
{
    int opt;
    char hostName[200] = DEFAULT_HOSTNAME;
    int port = DEFAULT_RESMGR_TPM_PORT;
    int returnVal = 0;

    struct option sOpts[] = {
        { "index"       , required_argument, NULL, 'x' },
        { "authHandle"  , required_argument, NULL, 'a' },
        { "size"        , required_argument, NULL, 's' },
        { "attribute"   , required_argument, NULL, 't' },
        { "handlePasswd", required_argument, NULL, 'P' },
        { "indexPasswd" , required_argument, NULL, 'I' },
        { "passwdInHex" , no_argument,       NULL, 'X' },
        { "port"        , required_argument, NULL, 'p' },
        { "dbg"         , required_argument, NULL, 'd' },
        { "help"        , no_argument,       NULL, 'h' },
        { "version"     , no_argument,       NULL, 'v' },
        { NULL          , no_argument,       NULL,  0  },
    };

    if( argc == 1)
    {
        showHelp(argv[0]);
        return 0;
    }

    if( argc > (int)(2*sizeof(sOpts)/sizeof(struct option)) )
    {
        showArgMismatch(argv[0]);
        return -1;
    }

    while ( ( opt = getopt_long( argc, argv, "x:a:s:t:P:I:Xp:d:hv", sOpts, NULL ) ) != -1 )
    {
        switch ( opt ) {
        case 'h':
        case '?':
            showHelp(argv[0]);
            return -2;
        case 'v':
            showVersion(argv[0]);
            return -3;

        case 'x':
            if( getSizeUint32Hex(optarg, &nvIndex) != 0 )
                return -4;
            break;

        case 'a':
            if( getSizeUint32Hex(optarg, &authHandle) != 0 )
                return -5;
            break;

        case 's':
            if( getSizeUint32(optarg, &size) != 0 )
                return -6;
            break;

        case 't':
            if( getSizeUint32Hex(optarg, &nvAttribute) != 0 )
                return -7;
            break;

        case 'P':
            if( optarg == NULL || (strlen(optarg) >= sizeof(TPMU_HA)) )
            {
                printf("\nPlease input the handle password(optional,no more than %d characters).\n", (int)sizeof(TPMU_HA)-1);
                return -8;
            }
            safeStrNCpy(handlePasswd, optarg, sizeof(handlePasswd));
            break;

        case 'I':
            if( optarg == NULL || (strlen(optarg) >= sizeof(TPMU_HA)) )
            {
                printf("\nPlease input the index password(optional,no more than %d characters).\n", (int)sizeof(TPMU_HA)-1);
                return -9;
            }
            safeStrNCpy(indexPasswd, optarg, sizeof(indexPasswd));
            break;
        case 'X':
            hexPasswd = true;
            break;
        case 'p':
            if( getPort(optarg, &port) )
            {
                printf("Incorrect port number.\n");
                return -10;
            }
            break;
        case 'd':
            if( getDebugLevel(optarg, &debugLevel) )
            {
                printf("Incorrect debug level.\n");
                return -11;
            }
            break;
        }
    }

    if( nvIndex == 0 )
    {
        printf("\nYou must provide an index (!= 0) for the NV area.\n");
        return -12;
    }

    if( authHandle == 0 )
    {
        printf("\nYou must provide an right auth handle for this operation.\n");
        showHelp(argv[0]);
        return -13;
    }

    prepareTest(hostName, port, debugLevel);

    returnVal = nvSpaceDefine();

    finishTest();

    if(returnVal)
        return -14;

    return 0;
}
