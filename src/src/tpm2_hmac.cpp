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

TPMS_AUTH_COMMAND sessionData;
bool hexPasswd = false;
int debugLevel = 0;

int hmac(TPMI_DH_OBJECT keyHandle, TPM2B_MAX_BUFFER *data, TPMI_ALG_HASH halg, const char *outHmacFilePath)
{
    UINT32 rval;
    TPMS_AUTH_RESPONSE sessionDataOut;
    TSS2_SYS_CMD_AUTHS sessionsData;
    TSS2_SYS_RSP_AUTHS sessionsDataOut;
    TPMS_AUTH_COMMAND *sessionDataArray[1];
    TPMS_AUTH_RESPONSE *sessionDataOutArray[1];

    TPM2B_DIGEST outHMAC = { { sizeof(TPM2B_DIGEST)-2, } };

    sessionDataArray[0] = &sessionData;
    sessionDataOutArray[0] = &sessionDataOut;

    sessionsDataOut.rspAuths = &sessionDataOutArray[0];
    sessionsData.cmdAuths = &sessionDataArray[0];

    sessionsDataOut.rspAuthsCount = 1;
    sessionsData.cmdAuthsCount = 1;

    sessionData.sessionHandle = TPM_RS_PW;
    sessionData.nonce.t.size = 0;
    *((UINT8 *)((void *)&sessionData.sessionAttributes)) = 0;
    if (sessionData.hmac.t.size > 0 && hexPasswd)
    {
        sessionData.hmac.t.size = sizeof(sessionData.hmac) - 2;
        if (hex2ByteStructure((char *)sessionData.hmac.t.buffer,
                              &sessionData.hmac.t.size,
                              sessionData.hmac.t.buffer) != 0)
        {
            printf( "Failed to convert Hex format password for key Passwd.\n");
            return -1;
        }
    }

    rval = Tss2_Sys_HMAC(sysContext, keyHandle, &sessionsData, data, halg, &outHMAC, &sessionsDataOut);
    if(rval != TPM_RC_SUCCESS)
    {
        printf("\n......TPM2_HMAC Error. TPM Error:0x%x......\n", rval);
        return -1;
    }
    printf("\ntpm2_hmac succ.\n\n");

    printf("\nhmac value(hex type): ");
    for(UINT16 i = 0; i < outHMAC.t.size; i++)
        printf("%02x ", outHMAC.t.buffer[i]);
    printf("\n");

    if( saveDataToFile(outHmacFilePath, (UINT8 *)&outHMAC, sizeof(outHMAC)) )
        return -2;

    return 0;
}

void showHelp(const char *name)
{
    printf("\n%s  [options]\n"
        "\n"
        "-h, --help               Display command tool usage info;\n"
        "-v, --version            Display command tool version info\n"
        "-k, --keyHandle <hexHandle> handle for the symmetric signing key providing the HMAC key\n"
        "-c, --keyContext <filename>  filename of the key context used for the operation\n"
        "-P, --pwdk      <string>    the keyHandle's password, optional\n"
        "-g, --halg      <hexAlg>    algorithm for the hash being computed\n"
            "\t0x0004  TPM_ALG_SHA1\n"
            "\t0x000B  TPM_ALG_SHA256\n"
            "\t0x000C  TPM_ALG_SHA384\n"
            "\t0x000D  TPM_ALG_SHA512\n"
            "\t0x0012  TPM_ALG_SM3_256\n"
        "-I, --infile    <inputFilename>  file containning the data to be HMACed\n"
        "-o, --outfile   <hmacFilename>   file record the HMAC result\n"
        "-X, --passwdInHex                passwords given by any options are hex format.\n"
        "-p, --port  <port number>  The Port number, default is %d, optional\n"
        "-d, --debugLevel <0|1|2|3> The level of debug message, default is 0, optional\n"
            "\t0 (high level test results)\n"
            "\t1 (test app send/receive byte streams)\n"
            "\t2 (resource manager send/receive byte streams)\n"
            "\t3 (resource manager tables)\n"
        "\n"
        "Example:\n"
        "%s -k 0x80000001 -P abc123 -g 0x004 -I <inputFilename> -o <hmacFilename> \n"
        "%s -k 0x80000001 -g 0x004 -I <inputFilename> -o <hmacFilename>\n\n"// -i <simulator IP>\n\n",DEFAULT_TPM_PORT);
        "%s -k 0x80000001 -P 123abc -X -g 0x004 -I <inputFilename> -o <hmacFilename> \n"
        , name, DEFAULT_RESMGR_TPM_PORT, name, name, name);
}

int main(int argc, char* argv[])
{
    char hostName[200] = DEFAULT_HOSTNAME;
    int port = DEFAULT_RESMGR_TPM_PORT;

    TPMI_DH_OBJECT keyHandle;
    TPM2B_MAX_BUFFER data;
    TPMI_ALG_HASH  halg;
    char outHmacFilePath[PATH_MAX] = {0};
    char *contextKeyFile = NULL;
    long fileSize = 0;

    setbuf(stdout, NULL);
    setvbuf (stdout, NULL, _IONBF, BUFSIZ);

    int opt = -1;
    const char *optstring = "hvk:P:g:I:o:p:d:c:X";
    static struct option long_options[] = {
      {"help",0,NULL,'h'},
      {"version",0,NULL,'v'},
      {"keyHandle",1,NULL,'k'},
      {"pwdk",1,NULL,'P'},
      {"halg",1,NULL,'g'},
      {"infile",1,NULL,'I'},
      {"outfile",1,NULL,'o'},
      {"port",1,NULL,'p'},
      {"debugLevel",1,NULL,'d'},
      {"keyContext",1,NULL,'c'},
      {"passwdInHex",0,NULL,'X'},
      {0,0,0,0}
    };

    int returnVal = 0;
    int flagCnt = 0;
    int h_flag = 0,
        v_flag = 0,
        k_flag = 0,
        P_flag = 0,
        g_flag = 0,
        I_flag = 0,
        c_flag = 0,
        o_flag = 0;

    if(argc == 1)
    {
        showHelp(argv[0]);
        return 0;
    }

    while((opt = getopt_long(argc,argv,optstring,long_options,NULL)) != -1)
    {
        switch(opt)
        {
        case 'h':
            h_flag = 1;
            break;
        case 'v':
            v_flag = 1;
            break;
        case 'k':
            if(getSizeUint32Hex(optarg,&keyHandle) != 0)
            {
                returnVal = -1;
                break;
            }
            k_flag = 1;
            break;
        case 'P':
            sessionData.hmac.t.size = sizeof(sessionData.hmac.t) - 2;
            if(str2ByteStructure(optarg,&sessionData.hmac.t.size,sessionData.hmac.t.buffer) != 0)
            {
                returnVal = -2;
                break;
            }
            P_flag = 1;
            break;
        case 'g':
            if(getSizeUint16Hex(optarg,&halg) != 0)
            {
                showArgError(optarg, argv[0]);
                returnVal = -3;
                break;
            }
            printf("halg = 0x%4.4x\n", halg);
            g_flag = 1;
            break;
        case 'I':
//              long fileSize = 0;
            if( getFileSize(optarg, &fileSize) != 0)
            {
                returnVal = -4;
                break;
            }
            if(fileSize > MAX_DIGEST_BUFFER)
            {
                printf("Input data too long: %ld, should be less than %d bytes\n", fileSize, MAX_DIGEST_BUFFER);
                returnVal = -5;
                break;
            }
            data.t.size = sizeof(data) - 2;
            if(loadDataFromFile(optarg, (UINT8 *)data.t.buffer, &data.t.size) != 0)
            {
                returnVal = -6;
                break;
            }
            I_flag = 1;
            break;
        case 'o':
            safeStrNCpy(outHmacFilePath, optarg, sizeof(outHmacFilePath));
            if(checkOutFile(outHmacFilePath) != 0)
            {
                returnVal = -7;
                break;
            }
            o_flag = 1;
            break;
        case 'p':
            if( getPort(optarg, &port) )
            {
                printf("Incorrect port number.\n");
                returnVal = -8;
            }
            break;
        case 'd':
            if( getDebugLevel(optarg, &debugLevel) )
            {
                printf("Incorrect debug level.\n");
                returnVal = -9;
            }
            break;
         case 'c':
            contextKeyFile = optarg;
            if(contextKeyFile == NULL || contextKeyFile[0] == '\0')
            {
                returnVal = -10;
                break;
            }
            printf("contextKeyFile = %s\n", contextKeyFile);
            c_flag = 1;
            break;
        case 'X':
            hexPasswd = true;
            break;
        case ':':
//              printf("Argument %c needs a value!\n",optopt);
            returnVal = -11;
            break;
        case '?':
//              printf("Unknown Argument: %c\n",optopt);
            returnVal = -12;
            break;
        //default:
        //  break;
        }
        if(returnVal)
            break;
    };

    if(returnVal != 0)
        return returnVal;

    if(P_flag != 1)
        sessionData.hmac.t.size = 0;
    flagCnt = h_flag + v_flag + k_flag + g_flag + I_flag + o_flag + c_flag;
    if(flagCnt == 1)
    {
        if(h_flag == 1)
            showHelp(argv[0]);
        else if(v_flag == 1)
            showVersion(argv[0]);
        else
        {
            showArgMismatch(argv[0]);
            return -13;
        }
    }
    else if((flagCnt == 4) && (k_flag == 1 || c_flag == 1) && (I_flag == 1) && (o_flag == 1) && g_flag == 1)
    {
        prepareTest(hostName, port, debugLevel);

        if(c_flag)
            returnVal = loadTpmContextFromFile(sysContext, &keyHandle, contextKeyFile);
        if(returnVal == 0)
            returnVal = hmac(keyHandle, &data, halg, outHmacFilePath);

        finishTest();

        if(returnVal)
            return -14;
    }
    else
    {
        showArgMismatch(argv[0]);
        return -15;
    }

    return 0;
}
