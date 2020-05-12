/*************************************************************************************************/
/*!
 *  \file   mesh_security_toolbox.h
 *
 *  \brief  Security Toolbox module interface.
 *
 *  Copyright (c) 2010-2018 Arm Ltd. All Rights Reserved.
 *
 *  Copyright (c) 2019 Packetcraft, Inc.
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *      http://www.apache.org/licenses/LICENSE-2.0
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */
/*************************************************************************************************/

/*! ***********************************************************************************************
 * @file
 *
 * @addtogroup MESH_SECURITY
 * @{
 *************************************************************************************************/

#ifndef MESH_SECURITY_TOOLBOX_H
#define MESH_SECURITY_TOOLBOX_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! AES-128 block size in bytes */
#define MESH_SEC_TOOL_AES_BLOCK_SIZE       16

/*! Nonce size for Mesh CCM operations is fixed: 13 bytes */
#define MESH_SEC_TOOL_CCM_NONCE_SIZE       13

/*! Maximum CCM MAC size for Mesh Security operations */
#define MESH_SEC_TOOL_CCM_MAX_MAC_SIZE     8

/*! ECC key length in P-256 space */
#define MESH_SEC_TOOL_ECC_KEY_SIZE         32

/*! Request queue size for AES-128 requests */
#ifndef MESH_SEC_TOOL_AES_REQ_QUEUE_SIZE
#define MESH_SEC_TOOL_AES_REQ_QUEUE_SIZE   6
#endif

/*! Request queue size for AES-CMAC requests */
#ifndef MESH_SEC_TOOL_CMAC_REQ_QUEUE_SIZE
#define MESH_SEC_TOOL_CMAC_REQ_QUEUE_SIZE  6
#endif

/*! Request queue size for AES-CCM requests */
#ifndef MESH_SEC_TOOL_CCM_REQ_QUEUE_SIZE
#define MESH_SEC_TOOL_CCM_REQ_QUEUE_SIZE   6
#endif

/*! Request queue size for Kx derivation requests */
#ifndef MESH_SEC_TOOL_KX_REQ_QUEUE_SIZE
#define MESH_SEC_TOOL_KX_REQ_QUEUE_SIZE    6
#endif

/*! K1 derivation function result size in bytes */
#define MESH_SEC_TOOL_K1_RESULT_SIZE       16

/*! K2 derivation function result size in bytes */
#define MESH_SEC_TOOL_K2_RESULT_SIZE       33

/*! K3 derivation function result size in bytes */
#define MESH_SEC_TOOL_K3_RESULT_SIZE       8

/*! K4 derivation function result size in bytes */
#define MESH_SEC_TOOL_K4_RESULT_SIZE       1

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Mesh Security Toolbox return value type. See ::meshReturnValues */
typedef uint16_t meshSecToolRetVal_t;

/*! Mesh Security Toolbox used algorithms bitmask */
enum meshSecToolAlgoBitMask
{
  MESH_SEC_TOOL_ALGO_AES_128  = (1 << 0),  /*!< AES 128 */
  MESH_SEC_TOOL_ALGO_AES_CMAC = (1 << 1),  /*!< AES-CMAC */
  MESH_SEC_TOOL_ALGO_CCM      = (1 << 2),  /*!< AES-CCM */
  MESH_SEC_TOOL_ALGO_ECC      = (1 << 3),  /*!< ECC KeyGen and ECDH */
};

/*! Mesh Security Toolbox supported algorithms bitfield */
typedef uint8_t meshSecToolAlgoBitfield_t;

/*! Mesh Security CCM operation types */
enum meshSecToolCcmOperationType
{
  MESH_SEC_TOOL_CCM_ENCRYPT = 0x00, /*!< Encrypt operation */
  MESH_SEC_TOOL_CCM_DECRYPT = 0x01, /*!< Decrypt operation */
};

/*! Mesh Security CCM operation type. See ::meshSecToolCcmOperationType */
typedef uint8_t meshSecToolCcmOperation_t;

/*! Mesh Security Toolbox CCM request parameter structure */
typedef struct meshSecToolCcmParams_tag
{
  uint8_t  *pIn;           /*!< Pointer to input buffer */
  uint8_t  *pOut;          /*!< Pointer output buffer */
  uint8_t  *pAuthData;     /*!< Pointer to authentication data */
  uint8_t  *pCbcMac;       /*!< Pointer to CBC-MAC in/out buffer */
  uint8_t  *pCcmKey;       /*!< Pointer to 128-bit AES CCM Key */
  uint8_t  *pNonce;        /*!< 13-byte Nonce for counter */
  uint16_t inputLen;       /*!< Input/Output buffer length */
  uint16_t authDataLen;    /*!< Authentication data length */
  uint8_t  cbcMacSize;     /*!< Size of the CBC-MAC */
} meshSecToolCcmParams_t;

/*! Mesh Security CCM Encrypt operation result */
typedef struct meshSecToolCcmEncryptResult_tag
{
  uint8_t  *pCipherText;   /*!< Pointer to buffer storing the ciphertext
                            *   (passed as pOut in the request)
                            */
  uint16_t cipherTextSize; /*!< Size of the ciphertext */
  uint8_t  *pCbcMac;       /*!< Pointer to buffer storing the CBC-MAC calculation
                            *   (passed as parameter in the request)
                            */
  uint8_t  cbcMacSize;     /*!< Size in bytes of the CBC-MAC */
} meshSecToolCcmEncryptResult_t;

/*! Mesh CCM Decrypt operation result */
typedef struct meshSecToolCcmDecryptResult_tag
{
  uint8_t  *pPlainText;    /*!< Pointer to buffer storing the plaintext
                            *   (passed as pOut in the request)
                            */
  uint16_t plainTextSize;  /*!< Size of the plaintext */
  bool_t   isAuthSuccess;  /*!< TRUE if PDU is authenticated */
} meshSecToolCcmDecryptResult_t;

typedef union meshSecToolCcmResults_tag
{
  meshSecToolCcmEncryptResult_t encryptResult;  /*!< Encryption result */
  meshSecToolCcmDecryptResult_t decryptResult;  /*!< Decryption result */
} meshSecToolCcmResults_t;

/*! Mesh CCM operation result */
typedef struct meshSecToolCcmResult_tag
{
  meshSecToolCcmOperation_t op;      /*!< Operation identifier */
  meshSecToolCcmResults_t   results; /*!< Results union */
} meshSecToolCcmResult_t;

/*************************************************************************************************/
/*!
 *  \brief     Callback for AES-128 block encryption (e - function).
 *
 *  \param[in] pCipherTextBlock  Pointer to 16 byte ciphertext block.
 *  \param[in] pParam            Pointer to generic parameter provided in request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*meshSecToolAesCback_t)(const uint8_t *pCipherTextBlock, void *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Callback for AES-CMAC operation.
 *
 *  \param[in] pCmacResult  Pointer to 16 byte CMAC result.
 *  \param[in] pParam       Pointer to generic parameter provided in request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void (*meshSecToolCmacCback_t)(const uint8_t *pCmacResult, void *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Callback for AES-CCM operation.
 *
 *  \param[in] pCmacResult  Pointer to CCM response structure.
 *  \param[in] pParam       Pointer to generic parameter provided in request.
 *
 *  \return    None.
 *
 *  \see       meshSecToolCcmResult_t
 */
/*************************************************************************************************/
typedef void (*meshSecToolCcmCback_t)(const meshSecToolCcmResult_t *pCcmResult, void *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Callback for ECC key generation.
 *
 *  \param[in] pPubX  Pointer to the X component of ECC public key.
 *  \param[in] pPubY  Pointer to the Y component of ECC public key.
 *  \param[in] pPriv  Pointer to the ECC private key.
 *
 *  \return    None.
 *
 *  \remarks   The caller must copy the value of the ECC keys until the callback ends execution.
 */
/*************************************************************************************************/
typedef void (*meshSecToolEccKeyGenCback_t)(const uint8_t *pPubX,
                                            const uint8_t *pPubY,
                                            const uint8_t *pPriv);

/*************************************************************************************************/
/*!
 *  \brief     Callback for ECDH shared secret calculation.
 *
 *  \param[in] isValid        TRUE if the peer ECC key is valid, FALSE otherwise.
 *  \param[in] pSharedSecret  Pointer to ::MESH_SEC_TOOL_ECC_KEY_SIZE bytes of shared secret.
 *
 *  \return    None.
 *
 *  \remarks   The caller must copy the value of the ECDH secret until the callback ends execution.
 */
/*************************************************************************************************/
typedef void (*meshSecToolEcdhCback_t)(bool_t isValid, const uint8_t *pSharedSecret);

/*************************************************************************************************/
/*!
 *  \brief     Callback for Mesh Security Derivation functions.
 *
 *  \param[in] pResult     Pointer to result buffer.
 *  \param[in] resultSize  Size of the result.
 *  \param[in] pParam      Pointer to generic parameter provided in request.
 *
 *  \return    None.
 */
/*************************************************************************************************/
typedef void(*meshSecToolKeyDerivationCback_t)(const uint8_t *pResult, uint8_t resultSize,
                                               void *pParam);

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Mesh Security Toolbox Init.
 *
 *  \return None.
 */
/*************************************************************************************************/
void MeshSecToolInit(void);

/*************************************************************************************************/
/*!
 *  \brief     Mesh Security Toolbox AES-128 primitive.
 *
 *  \param[in] pAesKey          Pointer to 128-bit AES Key.
 *  \param[in] pPlainTextBlock  Pointer to 128-bit plaintext block.
 *  \param[in] aesCback         Callback invoked after encryption is complete.
 *  \param[in] pParam           Pointer to generic callback parameter.
 *
 *  \see       meshSecToolAesCback_t
 *
 *  \retval    MESH_SUCCESS                  Request processed successfully.
 *  \retval    MESH_SEC_TOOL_INVALID_PARAMS  Invalid parameters in request.
 *  \retval    MESH_SEC_TOOL_OUT_OF_MEMORY   No resources to process request.
 *  \retval    MESH_SEC_TOOL_UNINITIALIZED   The algorithm used by this request is not initialized.
 *  \retval    MESH_SEC_TOOL_UNKNOWN_ERROR   An error occurred in the PAL layer.
 *
 *  \remarks   The caller should not overwrite the memory referenced by input pointers until the
 *             callback is triggered.
 */
/*************************************************************************************************/
meshSecToolRetVal_t MeshSecToolAesEncrypt(uint8_t *pAesKey, uint8_t *pPlainTextBlock,
                                          meshSecToolAesCback_t aesCback, void *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Mesh Security Toolbox AES-CMAC primitive.
 *
 *  \param[in] pKey          Pointer to 128-bit AES Key.
 *  \param[in] pPlainText    Pointer to plain text array.
 *  \param[in] plainTextLen  Length of the plain text.
 *  \param[in] cmacCback     Callback invoked after CMAC calculation is complete.
 *  \param[in] pParam        Pointer to generic parameter for the callback.
 *
 *  \see       meshSecToolCmacCback_t
 *
 *  \retval    MESH_SUCCESS                  Request processed successfully.
 *  \retval    MESH_SEC_TOOL_INVALID_PARAMS  Invalid parameters in request.
 *  \retval    MESH_SEC_TOOL_OUT_OF_MEMORY   No resources to process request.
 *  \retval    MESH_SEC_TOOL_UNINITIALIZED   The algorithm used by this request is not initialized.
 *  \retval    MESH_SEC_TOOL_UNKNOWN_ERROR   An error occurred in the PAL layer.
 *
 *  \remarks   The caller should not overwrite the memory referenced by input pointers until the
 *             callback is triggered.
 */
/*************************************************************************************************/
meshSecToolRetVal_t MeshSecToolCmacCalculate(uint8_t *pKey, uint8_t *pPlainText, uint16_t plainTextLen,
                                             meshSecToolCmacCback_t cmacCback, void *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Mesh Security Toolbox AES-CCM primitive.
 *
 *  \param[in] opType     Type of operation (encrypt or decrypt).
 *  \param[in] pOpParams  Pointer to structure holding the parameters to configure CCM.
 *  \param[in] ccmCback   Callback invoked after CCM operation is complete.
 *  \param[in] pParam     Pointer to generic parameter for the callback.
 *
 *  \see       meshSecToolCcmCback_t
 *  \see       meshSecToolCcmOperation_t
 *
 *  \retval    MESH_SUCCESS                  Request processed successfully.
 *  \retval    MESH_SEC_TOOL_INVALID_PARAMS  Invalid parameters in request.
 *  \retval    MESH_SEC_TOOL_OUT_OF_MEMORY   No resources to process request.
 *  \retval    MESH_SEC_TOOL_UNINITIALIZED   The algorithm used by this request is not initialized.
 *  \retval    MESH_SEC_TOOL_UNKNOWN_ERROR   An error occurred in the PAL layer.
 *
 *  Usage:
 *  \code
 *  //global variables
 *  uint8_t ccmKey[16] = { 0x00, 0x01, 0x02 };
 *  uint8_t plain[5] = {'P','L','A','I','N'}, cipher[5] = { 0 };
 *  uint8_t authData[3] = { 0x00, 0x02, 0x03 };
 *  uint8_t cbcMac[8] = { 0 };
 *  uint8_t nonce[13] = { 0x01, 0x02, 0x02, 0x01, 0x04, 0x08,
 *                        0xFE, 0xAB, 0xAA, 0x90, 0x11, 0x22, 0x05 };
 *
 *  void ccmCback(const meshSecToolCcmResult_t *pCcmResult, void *pParam)
 *  {
 *    if(pCcmResult->op == (meshSecToolCcmOperation_t)MESH_SEC_TOOL_CCM_ENCRYPT)
 *    {
 *      //pCipherText points to cipher array
 *      PRINT_ARRAY(pCcmResult->results.encryptResult.pCipherText,
 *                  pCcmResult->results.encryptResult.cipherTextLen);
 *
 *      //pCbcMac points to cbcMac array
 *      PRINT_ARRAY(pCcmResult->results.encryptResult.pCbcMac,
 *                  pCcmResult->results.encryptResult.cbcMacSize);
 *    }
 *    else
 *    {
 *      //pPlainText points to plain array
 *      PRINT_ARRAY(pCcmResult->results.decryptResult.pPlainText,
 *                  pCcmResult->results.decryptResult.plainTextSize);
 *
 *      PRINT("AUTH has %s",
 *            (pCcmResult->results.decryptResult.isAuthSuccess) ? "SUCCEEDED":"FAILED");
 *    }
 *  }
 *  void myFunc(void)
 *  {
 *      meshSecToolCcmParams_t params;
 *      meshSecToolCcmOperation_t opType;
 *
 *      params.pNonce = nonce;
 *      params.pCcmKey = ccmKey;
 *      params.pAuthData = authData;
 *      params.authDataLen = sizeof(authData);
 *      params.inputLen = sizeof(plain); //sizeof(cipher);
 *      params.pCbcMac = cbcMac;
 *      params.cbcMacSize = sizeof(cbcMac);
 *
 *      if(encrypt)
 *      {
 *        params.pIn = plain;
 *        params.pOut = cipher;
 *        opType = (meshSecToolCcmOperation_t)MESH_SEC_TOOL_CCM_ENCRYPT;
 *      }
 *      else //ciphertext located in cipher, cbcMac already contains 8 byte MAC
 *      {
 *        params.pIn = cipher;
 *        params.pOut = plain;
 *        opType = (meshSecToolCcmOperation_t)MESH_SEC_TOOL_CCM_DECRYPT;
 *      }
 *      retVal = MeshSecToolCcmEncryptDecrypt(opType, &params, ccmCback, NULL);
 *      //check retval
 *  }
 *  \endcode
 *
 *  \remarks The caller should not overwrite the memory referenced by input pointers until the
 *           callback is triggered.
 */
/*************************************************************************************************/
meshSecToolRetVal_t MeshSecToolCcmEncryptDecrypt(meshSecToolCcmOperation_t opType,
                                                 meshSecToolCcmParams_t *pOpParams,
                                                 meshSecToolCcmCback_t ccmCback, void *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Mesh Security Toolbox Generate P-256 ECC Key.
 *
 *  \param[in] eccKeyGenCback  Callback invoked after a fresh key has been generated.
 *
 *  \see       meshSecToolEccKeyGenCback_t
 *
 *  \retval    MESH_SUCCESS                  Request processed successfully.
 *  \retval    MESH_SEC_TOOL_INVALID_PARAMS  Invalid parameters in request.
 *  \retval    MESH_SEC_TOOL_OUT_OF_MEMORY   No resources to process request.
 *  \retval    MESH_SEC_TOOL_UNINITIALIZED   The algorithm used by this request is not initialized.
 *  \retval    MESH_SEC_TOOL_UNKNOWN_ERROR   An error occurred in the PAL layer.
 */
/*************************************************************************************************/
meshSecToolRetVal_t MeshSecToolEccGenerateKey(meshSecToolEccKeyGenCback_t eccKeyGenCback);

/*************************************************************************************************/
/*!
 *  \brief     Mesh Security Toolbox compute ECDH shared secret.
 *
 *  \param[in] pPeerPubX          Pointer to the peer's public key X component.
 *  \param[in] pPeerPubY          Pointer to the peer's public key Y component.
 *  \param[in] pLocalPriv         Pointer to the local private key.
 *  \param[in] sharedSecretCback  Callback invoked at the end of the shared secret calculation.
 *
 *  \see       meshSecToolEcdhCback_t
 *
 *  \retval    MESH_SUCCESS                  Request processed succesfully.
 *  \retval    MESH_SEC_TOOL_INVALID_PARAMS  Invalid parameters in request.
 *  \retval    MESH_SEC_TOOL_OUT_OF_MEMORY   No resources to process request.
 *  \retval    MESH_SEC_TOOL_UNINITIALIZED   The algorithm used by this request is not initialized.
 *  \retval    MESH_SEC_TOOL_UNKNOWN_ERROR   An error occurred in the PAL layer.
 *
 *  \remarks   The caller should not overwrite the memory referenced by the input pointers until the
 *             callback is triggered.
 */
/*************************************************************************************************/
meshSecToolRetVal_t MeshSecToolEccCompSharedSecret(const uint8_t *pPeerPubX,
                                                   const uint8_t *pPeerPubY,
                                                   const uint8_t *pLocalPriv,
                                                   meshSecToolEcdhCback_t sharedSecretCback);

/*************************************************************************************************/
/*!
 *  \brief     Mesh Security Toolbox Salt s1 primitive.
 *
 *  \param[in] pPlainText    Pointer to non-zero length plaintext.
 *  \param[in] plainTextLen  Length of the plaintext.
 *  \param[in] saltCback     Callback invoked after salt is generated.
 *  \param[in] pParam        Pointer to generic callback parameter.
 *
 *  \retval    MESH_SUCCESS                  Request processed succesfully.
 *  \retval    MESH_SEC_TOOL_INVALID_PARAMS  Invalid parameters in request.
 *  \retval    MESH_SEC_TOOL_OUT_OF_MEMORY   No resources to process request.
 *  \retval    MESH_SEC_TOOL_UNINITIALIZED   The algorithm used by this request is not initialized.
 *  \retval    MESH_SEC_TOOL_UNKNOWN_ERROR   An error occurred in the PAL layer.
 *
 *  \see       meshSecToolCmacCback_t
 *
 *  \remarks   The caller should not overwrite the memory referenced by input pointers until the
 *             callback is triggered.
 */
/*************************************************************************************************/
meshSecToolRetVal_t MeshSecToolGenerateSalt(uint8_t *pPlainText, uint16_t plainTextLen,
                                            meshSecToolCmacCback_t saltCback, void *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Mesh Security Toolbox k1 derivation primitive.
 *
 *  Used to derive Device, Identity and Beacon Keys.
 *  Description:
 *  k1(N, SALT, P) = CMAC(T, P), T = CMAC(SALT, N), size(SALT) = 16B, size(N) >= 0B, size(P) >= 0B;
 *
 *  \param[in] pPlainText           Pointer to plaintext data (P).
 *  \param[in] plainTextSize        Size of the plaintext.
 *  \param[in] pSalt                Pointer to 128-bit Salt.
 *  \param[in] pTempKeyMaterial     Pointer to plaintext used to compute the T key (N).
 *  \param[in] tempKeyMaterialSize  Size of the plaintext used to compute the T key.
 *  \param[in] derivCompleteCback   Callback invoked after derivation is complete.
 *  \param[in] pParam               Pointer to generic callback parameter.
 *
 *  \see       meshSecToolKeyDerivationCback_t
 *
 *  \retval    MESH_SUCCESS                  Request processed succesfully.
 *  \retval    MESH_SEC_TOOL_INVALID_PARAMS  Invalid parameters in request.
 *  \retval    MESH_SEC_TOOL_OUT_OF_MEMORY   No resources to process request.
 *  \retval    MESH_SEC_TOOL_UNINITIALIZED   The algorithm used by this request is not initialized.
 *  \retval    MESH_SEC_TOOL_UNKNOWN_ERROR   An error occurred in the PAL layer.
 *
 *  \remarks   The caller should not overwrite the memory referenced by input pointers until the
 *             callback is triggered.
 */
/*************************************************************************************************/
meshSecToolRetVal_t MeshSecToolK1Derive(uint8_t *pPlainText, uint16_t plainTextSize,
                                        uint8_t *pSalt, uint8_t *pTempKeyMaterial,
                                        uint16_t tempKeyMaterialSize,
                                        meshSecToolKeyDerivationCback_t derivCompleteCback,
                                        void *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Mesh Security Toolbox k2 derivation primitive.
 *
 *  Used to derive Encryption and Privacy Keys, and NID.
 *  Description:
 *  k2(N, P) = (T1 || T2 || T3) mod (1 << 263), size(N) = 16B, size(P) >= 1B
 *  T = CMAC(SALT, N), SALT = s1("smk2")
 *  T0 = empty string, Tn = CMAC(T, (Tn-1 || P || n)), 0 < n < 4
 *
 *  \param[in] pPlainText          Pointer to plaintext data (P).
 *  \param[in] plainTextSize       Size of the plaintext (at least 1 byte).
 *  \param[in] pTempKeyMaterial    Pointer to 128-bit plaintext used to compute the T key (N).
 *  \param[in] derivCompleteCback  Callback invoked after derivation is complete.
 *  \param[in] pParam              Pointer to generic callback parameter.
 *
 *  \see       meshSecToolKeyDerivationCback_t
 *
 *  \retval    MESH_SUCCESS                  Request processed succesfully.
 *  \retval    MESH_SEC_TOOL_INVALID_PARAMS  Invalid parameters in request.
 *  \retval    MESH_SEC_TOOL_OUT_OF_MEMORY   No resources to process request.
 *  \retval    MESH_SEC_TOOL_UNINITIALIZED   The algorithm used by this request is not initialized.
 *  \retval    MESH_SEC_TOOL_UNKNOWN_ERROR   An error occurred in the PAL layer.
 *
 *  \remarks   The caller should not overwrite the memory referenced by input pointers until the
 *             callback is triggered.
 *
 *  \note      This implementation does not allow plainTextSize larger than 15 bytes.
 */
/*************************************************************************************************/
meshSecToolRetVal_t MeshSecToolK2Derive(uint8_t *pPlainText, uint16_t plainTextSize,
                                        uint8_t *pTempKeyMaterial,
                                        meshSecToolKeyDerivationCback_t derivCompleteCback,
                                        void *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Mesh Security Toolbox k3 derivation primitive.
 *
 *  Used to derive 64 bit Network ID
 *  Description:
 *  k3(N) = CMAC(T, ("id64" || 0x01)) mod (1 << 64); size(N) = 16B
 *  T = CMAC(SALT, N), SALT = s1("smk3")
 *
 *  \param[in] pPlainText          Pointer to 128-bit plaintext used to compute the T key (N).
 *  \param[in] derivCompleteCback  Callback invoked after derivation is complete.
 *  \param[in] pParam              Pointer to generic callback parameter.
 *
 *  \see       meshSecToolKeyDerivationCback_t
 *
 *  \retval    MESH_SUCCESS                  Request processed succesfully.
 *  \retval    MESH_SEC_TOOL_INVALID_PARAMS  Invalid parameters in request.
 *  \retval    MESH_SEC_TOOL_OUT_OF_MEMORY   No resources to process request.
 *  \retval    MESH_SEC_TOOL_UNINITIALIZED   The algorithm used by this request is not initialized.
 *  \retval    MESH_SEC_TOOL_UNKNOWN_ERROR   An error occurred in the PAL layer.
 *
 *  \remarks   The caller should not overwrite the memory referenced by input pointers until the
 *             callback is triggered.
 */
/*************************************************************************************************/
meshSecToolRetVal_t MeshSecToolK3Derive(uint8_t *pPlainText,
                                        meshSecToolKeyDerivationCback_t derivCompleteCback,
                                        void *pParam);

/*************************************************************************************************/
/*!
 *  \brief     Mesh Security Toolbox k4 derivation primitive.
 *
 *  Used to derive 6 bits AID
 *  Description:
 *  k4(N) = CMAC(T, ("id6" || 0x01)) mod (1 << 6); size(N) = 16B
 *  T = CMAC(SALT, N), SALT = s1("smk4")
 *
 *  \param[in] pPlainText           Pointer to 128-bit plaintext used to compute the T key (N).
 *  \param[in] derivCompleteCaback  Callback invoked after derivation is complete.
 *  \param[in] pParam               Pointer to generic callback parameter.
 *
 *  \see       meshSecToolKeyDerivationCback_t
 *
 *  \retval    MESH_SUCCESS                  Request processed succesfully.
 *  \retval    MESH_SEC_TOOL_INVALID_PARAMS  Invalid parameters in request.
 *  \retval    MESH_SEC_TOOL_OUT_OF_MEMORY   No resources to process request.
 *  \retval    MESH_SEC_TOOL_UNINITIALIZED   The algorithm used by this request is not initialized.
 *  \retval    MESH_SEC_TOOL_UNKNOWN_ERROR   An error occurred in the PAL layer.
 *
 *  \remarks   The caller should not overwrite the memory referenced by input pointers until the
 *             callback is triggered.
 */
/*************************************************************************************************/
meshSecToolRetVal_t MeshSecToolK4Derive(uint8_t *pPlainText,
                                        meshSecToolKeyDerivationCback_t derivCompleteCback,
                                        void *pParam);

#ifdef __cplusplus
}
#endif

#endif /* MESH_SECURITY_TOOLBOX_H */

/*! ***********************************************************************************************
 * @}
 *************************************************************************************************/
