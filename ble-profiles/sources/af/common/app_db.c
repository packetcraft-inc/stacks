/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Application framework device database example, using simple RAM-based storage.
 *
 *  Copyright (c) 2011-2019 Arm Ltd. All Rights Reserved.
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
 */
/*************************************************************************************************/

#include <string.h>
#include "wsf_types.h"
#include "wsf_assert.h"
#include "wsf_nvm.h"
#include "util/bda.h"
#include "app_api.h"
#include "app_main.h"
#include "app_db.h"
#include "app_cfg.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! App DB NVM version id. */
#define APP_DB_NVM_VERSION                    0x0001

/*! App DB NVM base identifiers. */
#define APP_DB_NVM_BASE                       0x1000
#define APP_DB_NVM_RECORD_BASE                (APP_DB_NVM_BASE)

/*! App DB NVM record parameter indicies. */
#define APP_DB_NVM_IN_USE_ID                  0
#define APP_DB_NVM_PEER_ADDR_ID               1
#define APP_DB_NVM_ADDR_TYPE_ID               2
#define APP_DB_NVM_PEER_IRK_ID                3
#define APP_DB_NVM_PEER_CSRK_ID               4
#define APP_DB_NVM_KV_MASK_ID                 5
#define APP_DB_NVM_VALID_ID                   6
#define APP_DB_NVM_PEER_RAPO_ID               7
#define APP_DB_NVM_LOCAL_LTK_ID               8
#define APP_DB_NVM_LOCAL_SEC_LVL_ID           9
#define APP_DB_NVM_PEER_ADDR_RES_ID           10
#define APP_DB_NVM_PEER_LTK_ID                11
#define APP_DB_NVM_PEER_SEC_LVL_ID            12
#define APP_DB_NVM_CCC_TBL_ID                 13
#define APP_DB_NVM_PEER_SIGN_CTR_ID           14
#define APP_DB_NVM_CAS_ID                     15
#define APP_DB_NVM_CSF_ID                     16
#define APP_DB_NVM_CACHE_HASH_ID              17
#define APP_DB_NVM_HASH_ID                    18
#define APP_DB_NVM_HDL_LIST_ID                19
#define APP_DB_NVM_DISC_STATUS_ID             20

/* Max parameter index. */
#define APP_DB_NVM_HDL_MAX                    64

/*! Macro to generate NVM id from a record parameter index and record index. */
#define DBNV_ID(p, i)                         (APP_DB_NVM_RECORD_BASE + (APP_DB_NVM_HDL_MAX * i) + p)

/*! Invalid index */
#define APP_DB_INDEX_INVALID                  0xFF

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Database record */
typedef struct
{
  /*! Common for all roles */
  bdAddr_t     peerAddr;                      /*! Peer address */
  uint8_t      addrType;                      /*! Peer address type */
  dmSecIrk_t   peerIrk;                       /*! Peer IRK */
  dmSecCsrk_t  peerCsrk;                      /*! Peer CSRK */
  uint8_t      keyValidMask;                  /*! Valid keys in this record */
  bool_t       inUse;                         /*! TRUE if record in use */
  bool_t       valid;                         /*! TRUE if record is valid */
  bool_t       peerAddedToRl;                 /*! TRUE if peer device's been added to resolving list */
  bool_t       peerRpao;                      /*! TRUE if RPA Only attribute's present on peer device */

  /*! For slave local device */
  dmSecLtk_t   localLtk;                      /*! Local LTK */
  uint8_t      localLtkSecLevel;              /*! Local LTK security level */
  bool_t       peerAddrRes;                   /*! TRUE if address resolution's supported on peer device (master) */

  /*! For master local device */
  dmSecLtk_t   peerLtk;                       /*! Peer LTK */
  uint8_t      peerLtkSecLevel;               /*! Peer LTK security level */

  /*! for ATT server local device */
  uint16_t     cccTbl[APP_DB_NUM_CCCD];       /*! Client characteristic configuration descriptors */
  uint32_t     peerSignCounter;               /*! Peer Sign Counter */
  uint8_t      changeAwareState;              /*! Peer client awareness to state change in database */
  uint8_t      csf[ATT_CSF_LEN];              /*! Peer client supported features record */

  /*! for ATT client */
  bool_t       cacheByHash;                   /*! TRUE if cached handles are maintained by comparing database hash */
  uint8_t      dbHash[ATT_DATABASE_HASH_LEN]; /*! Peer database hash */
  uint16_t     hdlList[APP_DB_HDL_LIST_LEN];  /*! Cached handle list */
  uint8_t      discStatus;                    /*! Service discovery and configuration status */
} appDbRec_t;

/*! Database type */
typedef struct
{
  appDbRec_t  rec[APP_DB_NUM_RECS];               /*! Device database records */
  char        devName[ATT_DEFAULT_PAYLOAD_LEN];   /*! Device name */
  uint8_t     devNameLen;                         /*! Device name length */
  uint8_t     dbHash[ATT_DATABASE_HASH_LEN];      /*! Device GATT database hash */
} appDb_t;

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Database */
static appDb_t appDb;

/*! When all records are allocated use this index to determine which to overwrite */
static appDbRec_t *pAppDbNewRec = appDb.rec;

/*************************************************************************************************/
/*!
 *  \brief  Find the index of the record in the app DB.
 *
 *  \param  hdl           Record handle.
 *
 *  \return Index of record.
 */
/*************************************************************************************************/
static uint8_t appDbFindIndx(appDbHdl_t hdl)
{
  appDbRec_t  *pRec = (appDbRec_t *) hdl;
  uint8_t     i;

  if (pRec && pRec->inUse)
  {
    /* Find record index. */
    for (i = 0; i < APP_DB_NUM_RECS; i++)
    {
      if (&appDb.rec[i] == pRec)
      {
        return i;
      }
    }
  }

  /* Record does not exist */
  return APP_DB_INDEX_INVALID;
}

/*************************************************************************************************/
/*!
 *  \brief  Initialize the device database.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppDbInit(void)
{
  return;
}

/*************************************************************************************************/
/*!
 *  \brief  Create a new device database record.
 *
 *  \param  addrType  Address type.
 *  \param  pAddr     Peer device address.
 *
 *  \return Database record handle.
 */
/*************************************************************************************************/
appDbHdl_t AppDbNewRecord(uint8_t addrType, uint8_t *pAddr)
{
  appDbRec_t  *pRec = appDb.rec;
  uint8_t     i;

  /* find a free record */
  for (i = APP_DB_NUM_RECS; i > 0; i--, pRec++)
  {
    if (!pRec->inUse)
    {
      break;
    }
  }

  /* if all records were allocated */
  if (i == 0)
  {
    /* overwrite a record */
    pRec = pAppDbNewRec;

    /* get next record to overwrite */
    pAppDbNewRec++;
    if (pAppDbNewRec == &appDb.rec[APP_DB_NUM_RECS])
    {
      pAppDbNewRec = appDb.rec;
    }
  }

  /* initialize record */
  memset(pRec, 0, sizeof(appDbRec_t));
  pRec->inUse = TRUE;
  pRec->addrType = addrType;
  BdaCpy(pRec->peerAddr, pAddr);
  pRec->peerAddedToRl = FALSE;
  pRec->peerRpao = FALSE;

  return (appDbHdl_t) pRec;
}

/*************************************************************************************************/
/*!
*  \brief  Get the next database record for a given record. For the first record, the function
*          should be called with 'hdl' set to 'APP_DB_HDL_NONE'.
*
*  \param  hdl  Database record handle.
*
*  \return Next record handle found. APP_DB_HDL_NONE, otherwise.
*/
/*************************************************************************************************/
appDbHdl_t AppDbGetNextRecord(appDbHdl_t hdl)
{
  appDbRec_t  *pRec;

  /* if first record is requested */
  if (hdl == APP_DB_HDL_NONE)
  {
    pRec = appDb.rec;
  }
  /* if valid record passed in */
  else if (AppDbRecordInUse(hdl))
  {
    pRec = (appDbRec_t *)hdl;
    pRec++;
  }
  /* invalid record passed in */
  else
  {
    return APP_DB_HDL_NONE;
  }

  /* look for next valid record */
  while (pRec < &appDb.rec[APP_DB_NUM_RECS])
  {
    /* if record is in use */
    if (pRec->inUse && pRec->valid)
    {
      /* record found */
      return (appDbHdl_t)pRec;
    }

    /* look for next record */
    pRec++;
  }

  /* end of records */
  return APP_DB_HDL_NONE;
}

/*************************************************************************************************/
/*!
 *  \brief  Delete a new device database record.
 *
 *  \param  hdl       Database record handle.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppDbDeleteRecord(appDbHdl_t hdl)
{
  ((appDbRec_t *) hdl)->inUse = FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief  Validate a new device database record.  This function is called when pairing is
 *          successful and the devices are bonded.
 *
 *  \param  hdl       Database record handle.
 *  \param  keyMask   Bitmask of keys to validate.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppDbValidateRecord(appDbHdl_t hdl, uint8_t keyMask)
{
  ((appDbRec_t *) hdl)->valid = TRUE;
  ((appDbRec_t *) hdl)->keyValidMask = keyMask;
}

/*************************************************************************************************/
/*!
 *  \brief  Check if a record has been validated.  If it has not, delete it.  This function
 *          is typically called when the connection is closed.
 *
 *  \param  hdl       Database record handle.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppDbCheckValidRecord(appDbHdl_t hdl)
{
  if (((appDbRec_t *) hdl)->valid == FALSE)
  {
    AppDbDeleteRecord(hdl);
  }
}

/*************************************************************************************************/
/*!
*  \brief  Check if a database record is in use.

*  \param  hdl       Database record handle.
*
*  \return TURE if record in use. FALSE, otherwise.
*/
/*************************************************************************************************/
bool_t AppDbRecordInUse(appDbHdl_t hdl)
{
  appDbRec_t  *pRec = appDb.rec;
  uint8_t     i;

  /* see if record is in database record list */
  for (i = APP_DB_NUM_RECS; i > 0; i--, pRec++)
  {
    if (pRec->inUse && pRec->valid && (pRec == ((appDbRec_t *)hdl)))
    {
      return TRUE;
    }
  }

  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief  Check if there is a stored bond with any device.
 *
 *  \param  hdl       Database record handle.
 *
 *  \return TRUE if a bonded device is found, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t AppDbCheckBonded(void)
{
  appDbRec_t  *pRec = appDb.rec;
  uint8_t     i;

  /* find a record */
  for (i = APP_DB_NUM_RECS; i > 0; i--, pRec++)
  {
    if (pRec->inUse)
    {
      return TRUE;
    }
  }

  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief  Delete all database records.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppDbDeleteAllRecords(void)
{
  appDbRec_t  *pRec = appDb.rec;
  uint8_t     i;

  /* set in use to false for all records */
  for (i = APP_DB_NUM_RECS; i > 0; i--, pRec++)
  {
    pRec->inUse = FALSE;
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Find a device database record by peer address.
 *
 *  \param  addrType  Address type.
 *  \param  pAddr     Peer device address.
 *
 *  \return Database record handle or APP_DB_HDL_NONE if not found.
 */
/*************************************************************************************************/
appDbHdl_t AppDbFindByAddr(uint8_t addrType, uint8_t *pAddr)
{
  appDbRec_t  *pRec = appDb.rec;
  uint8_t     peerAddrType = DmHostAddrType(addrType);
  uint8_t     i;

  /* find matching record */
  for (i = APP_DB_NUM_RECS; i > 0; i--, pRec++)
  {
    if (pRec->inUse && (pRec->addrType == peerAddrType) && BdaCmp(pRec->peerAddr, pAddr))
    {
      return (appDbHdl_t) pRec;
    }
  }

  return APP_DB_HDL_NONE;
}

/*************************************************************************************************/
/*!
 *  \brief  Find a device database record by data in an LTK request.
 *
 *  \param  encDiversifier  Encryption diversifier associated with key.
 *  \param  pRandNum        Pointer to random number associated with key.
 *
 *  \return Database record handle or APP_DB_HDL_NONE if not found.
 */
/*************************************************************************************************/
appDbHdl_t AppDbFindByLtkReq(uint16_t encDiversifier, uint8_t *pRandNum)
{
  appDbRec_t  *pRec = appDb.rec;
  uint8_t     i;

  /* find matching record */
  for (i = APP_DB_NUM_RECS; i > 0; i--, pRec++)
  {
    if (pRec->inUse && (pRec->localLtk.ediv == encDiversifier) &&
        (memcmp(pRec->localLtk.rand, pRandNum, SMP_RAND8_LEN) == 0))
    {
      return (appDbHdl_t) pRec;
    }
  }

  return APP_DB_HDL_NONE;
}

/*************************************************************************************************/
/*!
 *  \brief  Get a key from a device database record.
 *
 *  \param  hdl       Database record handle.
 *  \param  type      Type of key to get.
 *  \param  pSecLevel If the key is valid, the security level of the key.
 *
 *  \return Pointer to key if key is valid or NULL if not valid.
 */
/*************************************************************************************************/
dmSecKey_t *AppDbGetKey(appDbHdl_t hdl, uint8_t type, uint8_t *pSecLevel)
{
  dmSecKey_t *pKey = NULL;

  /* if key valid */
  if ((type & ((appDbRec_t *) hdl)->keyValidMask) != 0)
  {
    switch(type)
    {
      case DM_KEY_LOCAL_LTK:
        *pSecLevel = ((appDbRec_t *) hdl)->localLtkSecLevel;
        pKey = (dmSecKey_t *) &((appDbRec_t *) hdl)->localLtk;
        break;

      case DM_KEY_PEER_LTK:
        *pSecLevel = ((appDbRec_t *) hdl)->peerLtkSecLevel;
        pKey = (dmSecKey_t *) &((appDbRec_t *) hdl)->peerLtk;
        break;

      case DM_KEY_IRK:
        pKey = (dmSecKey_t *)&((appDbRec_t *)hdl)->peerIrk;
        break;

      case DM_KEY_CSRK:
        pKey = (dmSecKey_t *)&((appDbRec_t *)hdl)->peerCsrk;
        break;

      default:
        break;
    }
  }

  return pKey;
}

/*************************************************************************************************/
/*!
 *  \brief  Set a key in a device database record.
 *
 *  \param  hdl       Database record handle.
 *  \param  pKey      Key data.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppDbSetKey(appDbHdl_t hdl, dmSecKeyIndEvt_t *pKey)
{
  switch(pKey->type)
  {
    case DM_KEY_LOCAL_LTK:
      ((appDbRec_t *) hdl)->localLtkSecLevel = pKey->secLevel;
      ((appDbRec_t *) hdl)->localLtk = pKey->keyData.ltk;
      break;

    case DM_KEY_PEER_LTK:
      ((appDbRec_t *) hdl)->peerLtkSecLevel = pKey->secLevel;
      ((appDbRec_t *) hdl)->peerLtk = pKey->keyData.ltk;
      break;

    case DM_KEY_IRK:
      ((appDbRec_t *)hdl)->peerIrk = pKey->keyData.irk;

      /* make sure peer record is stored using its identity address */
      ((appDbRec_t *)hdl)->addrType = pKey->keyData.irk.addrType;
      BdaCpy(((appDbRec_t *)hdl)->peerAddr, pKey->keyData.irk.bdAddr);
      break;

    case DM_KEY_CSRK:
      ((appDbRec_t *)hdl)->peerCsrk = pKey->keyData.csrk;

      /* sign counter must be initialized to zero when CSRK is generated */
      ((appDbRec_t *)hdl)->peerSignCounter = 0;
      break;

    default:
      break;
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Get the peer's database hash.
 *
 *  \param  hdl       Database record handle.
 *
 *  \return Pointer to database hash.
 */
/*************************************************************************************************/
uint8_t *AppDbGetPeerDbHash(appDbHdl_t hdl)
{
  return ((appDbRec_t *) hdl)->dbHash;
}

/*************************************************************************************************/
/*!
 *  \brief  Set a new peer database hash.
 *
 *  \param  hdl       Database record handle.
 *  \param  pDbHash   Pointer to new hash.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppDbSetPeerDbHash(appDbHdl_t hdl, uint8_t *pDbHash)
{
  WSF_ASSERT(pDbHash != NULL);

  memcpy(((appDbRec_t *) hdl)->dbHash, pDbHash, ATT_DATABASE_HASH_LEN);
}

/*************************************************************************************************/
/*!
 *  \brief  Check if cached handles' validity is determined by reading the peer's database hash
 *
 *  \param  hdl       Database record handle.
 *
 *  \return \ref TRUE if peer's database hash must be read to verify handles have not changed.
 */
/*************************************************************************************************/
bool_t AppDbIsCacheCheckedByHash(appDbHdl_t hdl)
{
  return ((appDbRec_t *) hdl)->cacheByHash;
}

/*************************************************************************************************/
/*!
 *  \brief  Set if cached handles' validity is determined by reading the peer's database hash.
 *
 *  \param  hdl           Database record handle.
 *  \param  cacheByHash   \ref TRUE if peer's database must be read to verify cached handles.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppDbSetCacheByHash(appDbHdl_t hdl, bool_t cacheByHash)
{
  ((appDbRec_t *) hdl)->cacheByHash = cacheByHash;
}

/*************************************************************************************************/
/*!
 *  \brief  Get the client characteristic configuration descriptor table.
 *
 *  \param  hdl       Database record handle.
 *
 *  \return Pointer to client characteristic configuration descriptor table.
 */
/*************************************************************************************************/
uint16_t *AppDbGetCccTbl(appDbHdl_t hdl)
{
  return ((appDbRec_t *) hdl)->cccTbl;
}

/*************************************************************************************************/
/*!
 *  \brief  Set a value in the client characteristic configuration table.
 *
 *  \param  hdl       Database record handle.
 *  \param  idx       Table index.
 *  \param  value     client characteristic configuration value.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppDbSetCccTblValue(appDbHdl_t hdl, uint16_t idx, uint16_t value)
{
  WSF_ASSERT(idx < APP_DB_NUM_CCCD);

  ((appDbRec_t *) hdl)->cccTbl[idx] = value;
}

/*************************************************************************************************/
/*!
 *  \brief  Get the client supported features record.
 *
 *  \param  hdl                Database record handle.
 *  \param  pChangeAwareState  Pointer to peer client's change aware status to a change in the database.
 *  \param  pCsf               Pointer to csf value pointer.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppDbGetCsfRecord(appDbHdl_t hdl, uint8_t *pChangeAwareState, uint8_t **pCsf)
{
  *pChangeAwareState = ((appDbRec_t *)hdl)->changeAwareState;
  *pCsf = ((appDbRec_t *) hdl)->csf;
}

/*************************************************************************************************/
/*!
 *  \brief  Set a client supported features record.
 *
 *  \param  hdl               Database record handle.
 *  \param  changeAwareState  The state of awareness to a change, see ::attClientAwareStates.
 *  \param  pCsf              Pointer to the client supported features value.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppDbSetCsfRecord(appDbHdl_t hdl, uint8_t changeAwareState, uint8_t *pCsf)
{
  if ((pCsf != NULL) && (hdl != APP_DB_HDL_NONE))
  {
    ((appDbRec_t *) hdl)->changeAwareState = changeAwareState;
    memcpy(&((appDbRec_t *) hdl)->csf, pCsf, ATT_CSF_LEN);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Set client's state of awareness to a change in the database.
 *
 *  \param  hdl        Database record handle. If \ref hdl == \ref NULL, state is set for all
 *                     clients.
 *  \param  state      The state of awareness to a change, see ::attClientAwareStates.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppDbSetClientChangeAwareState(appDbHdl_t hdl, uint8_t state)
{
  if (hdl == APP_DB_HDL_NONE)
  {
    appDbRec_t  *pRec = appDb.rec;
    uint8_t     i;

    /* Set all clients status to change-unaware. */
    for (i = APP_DB_NUM_RECS; i > 0; i--, pRec++)
    {
      pRec->changeAwareState = state;
    }
  }
  else
  {
    ((appDbRec_t *) hdl)->changeAwareState = state;
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Get device's GATT database hash.
 *
 *  \return Pointer to database hash.
 */
/*************************************************************************************************/
uint8_t *AppDbGetDbHash(void)
{
  return appDb.dbHash;
}

/*************************************************************************************************/
/*!
 *  \brief  Set device's  GATT database hash.
 *
 *  \param  pHash    GATT database hash to store.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppDbSetDbHash(uint8_t *pHash)
{
  if (pHash != NULL)
  {
    memcpy(appDb.dbHash, pHash, ATT_DATABASE_HASH_LEN);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Get the discovery status.
 *
 *  \param  hdl       Database record handle.
 *
 *  \return Discovery status.
 */
/*************************************************************************************************/
uint8_t AppDbGetDiscStatus(appDbHdl_t hdl)
{
  return ((appDbRec_t *) hdl)->discStatus;
}

/*************************************************************************************************/
/*!
 *  \brief  Set the discovery status.
 *
 *  \param  hdl       Database record handle.
 *  \param  state     Discovery status.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppDbSetDiscStatus(appDbHdl_t hdl, uint8_t status)
{
  ((appDbRec_t *) hdl)->discStatus = status;
}

/*************************************************************************************************/
/*!
 *  \brief  Get the cached handle list.
 *
 *  \param  hdl       Database record handle.
 *
 *  \return Pointer to handle list.
 */
/*************************************************************************************************/
uint16_t *AppDbGetHdlList(appDbHdl_t hdl)
{
  return ((appDbRec_t *) hdl)->hdlList;
}

/*************************************************************************************************/
/*!
 *  \brief  Set the cached handle list.
 *
 *  \param  hdl       Database record handle.
 *  \param  pHdlList  Pointer to handle list.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppDbSetHdlList(appDbHdl_t hdl, uint16_t *pHdlList)
{
  memcpy(((appDbRec_t *) hdl)->hdlList, pHdlList, sizeof(((appDbRec_t *) hdl)->hdlList));
}

/*************************************************************************************************/
/*!
 *  \brief  Get the device name.
 *
 *  \param  pLen      Returned device name length.
 *
 *  \return Pointer to UTF-8 string containing device name or NULL if not set.
 */
/*************************************************************************************************/
char *AppDbGetDevName(uint8_t *pLen)
{
  /* if first character of name is NULL assume it is uninitialized */
  if (appDb.devName[0] == 0)
  {
    *pLen = 0;
    return NULL;
  }
  else
  {
    *pLen = appDb.devNameLen;
    return appDb.devName;
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Set the device name.
 *
 *  \param  len       Device name length.
 *  \param  pStr      UTF-8 string containing device name.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppDbSetDevName(uint8_t len, char *pStr)
{
  /* check for maximum device length */
  len = (len <= sizeof(appDb.devName)) ? len : sizeof(appDb.devName);

  memcpy(appDb.devName, pStr, len);
}

/*************************************************************************************************/
/*!
 *  \brief  Get address resolution attribute value read from a peer device.
 *
 *  \param  hdl        Database record handle.
 *
 *  \return TRUE if address resolution is supported in peer device. FALSE, otherwise.
 */
/*************************************************************************************************/
bool_t AppDbGetPeerAddrRes(appDbHdl_t hdl)
{
  return ((appDbRec_t *)hdl)->peerAddrRes;
}

/*************************************************************************************************/
/*!
 *  \brief  Set address resolution attribute value for a peer device.
 *
 *  \param  hdl        Database record handle.
 *  \param  addrRes    Address resolution attribue value.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppDbSetPeerAddrRes(appDbHdl_t hdl, uint8_t addrRes)
{
  ((appDbRec_t *)hdl)->peerAddrRes = addrRes;
}

/*************************************************************************************************/
/*!
 *  \brief  Get sign counter for a peer device.
 *
 *  \param  hdl        Database record handle.
 *
 *  \return Sign counter for peer device.
 */
/*************************************************************************************************/
uint32_t AppDbGetPeerSignCounter(appDbHdl_t hdl)
{
  return ((appDbRec_t *)hdl)->peerSignCounter;
}

/*************************************************************************************************/
/*!
 *  \brief  Set sign counter for a peer device.
 *
 *  \param  hdl          Database record handle.
 *  \param  signCounter  Sign counter for peer device.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppDbSetPeerSignCounter(appDbHdl_t hdl, uint32_t signCounter)
{
  ((appDbRec_t *)hdl)->peerSignCounter = signCounter;
}

/*************************************************************************************************/
/*!
 *  \brief  Get the peer device added to resolving list flag value.
 *
 *  \param  hdl        Database record handle.
 *
 *  \return TRUE if peer device's been added to resolving list. FALSE, otherwise.
 */
/*************************************************************************************************/
bool_t AppDbGetPeerAddedToRl(appDbHdl_t hdl)
{
  return ((appDbRec_t *)hdl)->peerAddedToRl;
}

/*************************************************************************************************/
/*!
 *  \brief  Set the peer device added to resolving list flag to a given value.
 *
 *  \param  hdl           Database record handle.
 *  \param  peerAddedToRl Peer device added to resolving list flag value.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppDbSetPeerAddedToRl(appDbHdl_t hdl, bool_t peerAddedToRl)
{
  ((appDbRec_t *)hdl)->peerAddedToRl = peerAddedToRl;
}

/*************************************************************************************************/
/*!
 *  \brief  Get the resolvable private address only attribute flag for a given peer device.
 *
 *  \param  hdl        Database record handle.
 *
 *  \return TRUE if RPA Only attribute is present on peer device. FALSE, otherwise.
 */
/*************************************************************************************************/
bool_t AppDbGetPeerRpao(appDbHdl_t hdl)
{
  return ((appDbRec_t *)hdl)->peerRpao;
}

/*************************************************************************************************/
/*!
 *  \brief  Set the resolvable private address only attribute flag for a given peer device.
 *
 *  \param  hdl        Database record handle.
 *  \param  peerRpao   Resolvable private address only attribute flag value.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppDbSetPeerRpao(appDbHdl_t hdl, bool_t peerRpao)
{
  ((appDbRec_t *)hdl)->peerRpao = peerRpao;
}

/*************************************************************************************************/
/*!
 *  \brief  Store the resolvable private address only attribute flag for a device record in NVM.
 *
 *  \param  hdl        Database record handle.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppDbNvmStorePeerRpao(appDbHdl_t hdl)
{
  uint8_t recIndex = appDbFindIndx(hdl);

  if (recIndex != APP_DB_INDEX_INVALID)
  {
    appDbRec_t *pRec = &appDb.rec[recIndex];
    uint32_t nvmId = DBNV_ID(APP_DB_NVM_PEER_RAPO_ID, recIndex);

    WsfNvmWriteData(nvmId, &pRec->peerRpao, sizeof(bool_t), NULL);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Store the client characteristic configuration table for a device record in NVM.
 *
 *  \param  hdl       Database record handle.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppDbNvmStoreCccTbl(appDbHdl_t hdl)
{
  uint8_t recIndex = appDbFindIndx(hdl);

  if (recIndex != APP_DB_INDEX_INVALID)
  {
    appDbRec_t *pRec = &appDb.rec[recIndex];
    uint32_t nvmId = DBNV_ID(APP_DB_NVM_CCC_TBL_ID, recIndex);

    WsfNvmWriteData(nvmId, (uint8_t*) &pRec->cccTbl, sizeof(uint16_t) * APP_DB_NUM_CCCD, NULL);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Store the cached attribute handle list for a device record in NVM.
 *
 *  \param  hdl       Database record handle.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppDbNvmStoreHdlList(appDbHdl_t hdl)
{
  uint8_t recIndex = appDbFindIndx(hdl);

  if (recIndex != APP_DB_INDEX_INVALID)
  {
    appDbRec_t *pRec = &appDb.rec[recIndex];
    uint32_t nvmId = DBNV_ID(APP_DB_NVM_HDL_LIST_ID, recIndex);

    WsfNvmWriteData(nvmId, (uint8_t*) &pRec->hdlList, sizeof(uint16_t) * APP_DB_HDL_LIST_LEN, NULL);

    nvmId = DBNV_ID(APP_DB_NVM_DISC_STATUS_ID, recIndex);

    WsfNvmWriteData(nvmId, &pRec->discStatus, sizeof(uint8_t), NULL);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Store the peer sign counter for a device record in NVM.
 *
 *  \param  hdl       Database record handle.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppDbNvmStorePeerSignCounter(appDbHdl_t hdl)
{
  uint8_t recIndex = appDbFindIndx(hdl);

  if (recIndex != APP_DB_INDEX_INVALID)
  {
    appDbRec_t *pRec = &appDb.rec[recIndex];
    uint32_t nvmId = DBNV_ID(APP_DB_NVM_PEER_SIGN_CTR_ID, recIndex);

    WsfNvmWriteData(nvmId, (uint8_t*) &pRec->peerSignCounter, sizeof(uint32_t), NULL);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Store the address resolution attribute value for a device record in NVM.
 *
 *  \param  hdl       Database record handle.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppDbNvmStorePeerAddrRes(appDbHdl_t hdl)
{
  uint8_t recIndex = appDbFindIndx(hdl);

  if (recIndex != APP_DB_INDEX_INVALID)
  {
    appDbRec_t *pRec = &appDb.rec[recIndex];
    uint32_t nvmId = DBNV_ID(APP_DB_NVM_PEER_ADDR_RES_ID, recIndex);

    WsfNvmWriteData(nvmId, (uint8_t*) &pRec->peerAddrRes, sizeof(bool_t), NULL);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Store the client's state of awareness to a change for a device record in NVM.
 *
 *  \param  hdl       Database record handle.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppDbNvmStoreChangeAwareState(appDbHdl_t hdl)
{
  uint8_t recIndex = appDbFindIndx(hdl);

  if (recIndex != APP_DB_INDEX_INVALID)
  {
    appDbRec_t *pRec = &appDb.rec[recIndex];
    uint32_t nvmId = DBNV_ID(APP_DB_NVM_CAS_ID, recIndex);

    WsfNvmWriteData(nvmId, (uint8_t*) &pRec->changeAwareState, sizeof(uint8_t), NULL);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Store the client supported features for a device record in NVM.
 *
 *  \param  hdl       Database record handle.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppDbNvmStoreCsfRecord(appDbHdl_t hdl)
{
  uint8_t recIndex = appDbFindIndx(hdl);

  if (recIndex != APP_DB_INDEX_INVALID)
  {
    appDbRec_t *pRec = &appDb.rec[recIndex];
    uint32_t nvmId = DBNV_ID(APP_DB_NVM_CSF_ID, recIndex);

    WsfNvmWriteData(nvmId, (uint8_t*) pRec->csf, ATT_CSF_LEN, NULL);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Store the method of validating the cache handle for a device record in NVM.
 *
 *  \param  hdl       Database record handle.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppDbNvmStoreCacheByHash(appDbHdl_t hdl)
{
  uint8_t recIndex = appDbFindIndx(hdl);

  if (recIndex != APP_DB_INDEX_INVALID)
  {
    appDbRec_t *pRec = &appDb.rec[recIndex];
    uint32_t nvmId = DBNV_ID(APP_DB_NVM_CACHE_HASH_ID, recIndex);

    WsfNvmWriteData(nvmId, (uint8_t*) &pRec->cacheByHash, sizeof(bool_t), NULL);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Store the GATT database hash for a device record in NVM.
 *
 *  \param  hdl       Database record handle.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppDbNvmStoreDbHash(appDbHdl_t hdl)
{
  uint8_t recIndex = appDbFindIndx(hdl);

  if (recIndex != APP_DB_INDEX_INVALID)
  {
    appDbRec_t *pRec = &appDb.rec[recIndex];
    uint32_t nvmId = DBNV_ID(APP_DB_NVM_HASH_ID, recIndex);

    WsfNvmWriteData(nvmId, pRec->dbHash, ATT_DATABASE_HASH_LEN, NULL);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Store bonding information for device record in NVM.
 *
 *  \param  hdl       Database record handle.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppDbNvmStoreBond(appDbHdl_t hdl)
{
  uint8_t i = appDbFindIndx(hdl);

  if (i != APP_DB_INDEX_INVALID)
  {
    appDbRec_t *pRec = &appDb.rec[i];

    if (pRec->inUse && pRec->valid)
    {
      bool_t valid = FALSE;

      /* Protect against corrupt bond state due to incomplete writes (power failure, crash, etc.). */
      /*  - First ensure valid FALSE before writing parameters. */
      WsfNvmWriteData(DBNV_ID(APP_DB_NVM_VALID_ID, i), &valid, sizeof(bool_t), NULL);

      /* Write record parameters. */
      WsfNvmWriteData(DBNV_ID(APP_DB_NVM_KV_MASK_ID, i), &pRec->keyValidMask, sizeof(uint8_t), NULL);

      if (pRec->keyValidMask & DM_KEY_LOCAL_LTK)
      {
        WsfNvmWriteData(DBNV_ID(APP_DB_NVM_LOCAL_LTK_ID, i), (uint8_t*) &pRec->localLtk, sizeof(dmSecLtk_t), NULL);
        WsfNvmWriteData(DBNV_ID(APP_DB_NVM_LOCAL_SEC_LVL_ID, i), &pRec->localLtkSecLevel, sizeof(uint8_t), NULL);
      }

      if (pRec->keyValidMask & DM_KEY_PEER_LTK)
      {
        WsfNvmWriteData(DBNV_ID(APP_DB_NVM_PEER_LTK_ID, i), (uint8_t*) &pRec->peerLtk, sizeof(dmSecLtk_t), NULL);
        WsfNvmWriteData(DBNV_ID(APP_DB_NVM_PEER_SEC_LVL_ID, i), &pRec->peerLtkSecLevel, sizeof(uint8_t), NULL);
      }

      if (pRec->keyValidMask & DM_KEY_IRK)
      {
        WsfNvmWriteData(DBNV_ID(APP_DB_NVM_PEER_IRK_ID, i), (uint8_t*) &pRec->peerIrk, sizeof(dmSecIrk_t), NULL);
      }

      if (pRec->keyValidMask & DM_KEY_CSRK)
      {
        WsfNvmWriteData(DBNV_ID(APP_DB_NVM_PEER_CSRK_ID, i), (uint8_t*) &pRec->peerCsrk, sizeof(dmSecCsrk_t), NULL);
      }

      WsfNvmWriteData(DBNV_ID(APP_DB_NVM_PEER_ADDR_ID, i), pRec->peerAddr, sizeof(bdAddr_t), NULL);
      WsfNvmWriteData(DBNV_ID(APP_DB_NVM_ADDR_TYPE_ID, i), &pRec->addrType, sizeof(uint8_t), NULL);
      WsfNvmWriteData(DBNV_ID(APP_DB_NVM_CACHE_HASH_ID, i), &pRec->cacheByHash, sizeof(bool_t), NULL);

      /* Protect against corrupt bond state due to incomplete writes (power failure, crash, etc.). */
      /*  - Second set valid TRUE after writing parameters. */
      WsfNvmWriteData(DBNV_ID(APP_DB_NVM_VALID_ID, i), &pRec->valid, sizeof(bool_t), NULL);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Read all device database records from NVM.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppDbNvmReadAll(void)
{
  uint8_t i;

  /* Read all records. */
  for (i = 0; i < APP_DB_NUM_RECS; i++)
  {
    bool_t valid = FALSE;
    appDbRec_t *pRec = &appDb.rec[i];

    /* Verify record is valid. */
    WsfNvmReadData(DBNV_ID(APP_DB_NVM_VALID_ID, i), &valid, sizeof(bool_t), NULL);

    if (valid && valid != 0xFF)
    {
      pRec->inUse = TRUE;
      pRec->valid = TRUE;

      /* Read bonding parameters. */
      WsfNvmReadData(DBNV_ID(APP_DB_NVM_PEER_ADDR_ID, i), pRec->peerAddr, sizeof(bdAddr_t), NULL);
      WsfNvmReadData(DBNV_ID(APP_DB_NVM_ADDR_TYPE_ID, i), &pRec->addrType, sizeof(uint8_t), NULL);

      WsfNvmReadData(DBNV_ID(APP_DB_NVM_KV_MASK_ID, i), &pRec->keyValidMask, sizeof(uint8_t), NULL);

      if (pRec->keyValidMask & DM_KEY_LOCAL_LTK)
      {
        WsfNvmReadData(DBNV_ID(APP_DB_NVM_LOCAL_LTK_ID, i), (uint8_t*) &pRec->localLtk, sizeof(dmSecLtk_t), NULL);
        WsfNvmReadData(DBNV_ID(APP_DB_NVM_LOCAL_SEC_LVL_ID, i), &pRec->localLtkSecLevel, sizeof(uint8_t), NULL);
      }

      if (pRec->keyValidMask & DM_KEY_PEER_LTK)
      {
        WsfNvmReadData(DBNV_ID(APP_DB_NVM_PEER_LTK_ID, i), (uint8_t*) &pRec->peerLtk, sizeof(dmSecLtk_t), NULL);
        WsfNvmReadData(DBNV_ID(APP_DB_NVM_PEER_SEC_LVL_ID, i), &pRec->peerLtkSecLevel, sizeof(uint8_t), NULL);
      }

      if (pRec->keyValidMask & DM_KEY_IRK)
      {
        WsfNvmReadData(DBNV_ID(APP_DB_NVM_PEER_IRK_ID, i), (uint8_t*) &pRec->peerIrk, sizeof(dmSecIrk_t), NULL);
      }

      if (pRec->keyValidMask & DM_KEY_CSRK)
      {
        WsfNvmReadData(DBNV_ID(APP_DB_NVM_PEER_CSRK_ID, i), (uint8_t*) &pRec->peerCsrk, sizeof(dmSecCsrk_t), NULL);
      }

      /* Read additional parameters. */
      WsfNvmReadData(DBNV_ID(APP_DB_NVM_PEER_RAPO_ID, i), &pRec->peerRpao, sizeof(bool_t), NULL);
      WsfNvmReadData(DBNV_ID(APP_DB_NVM_CCC_TBL_ID, i), (uint8_t*) pRec->cccTbl, sizeof(uint16_t) * APP_DB_NUM_CCCD, NULL);

      WsfNvmReadData(DBNV_ID(APP_DB_NVM_HDL_LIST_ID, i), (uint8_t*) &pRec->hdlList, sizeof(uint16_t) * APP_DB_HDL_LIST_LEN, NULL);
      WsfNvmReadData(DBNV_ID(APP_DB_NVM_DISC_STATUS_ID, i), &pRec->discStatus, sizeof(uint8_t), NULL);
      WsfNvmReadData(DBNV_ID(APP_DB_NVM_PEER_ADDR_RES_ID, i), &pRec->peerAddrRes, sizeof(bool_t), NULL);

      WsfNvmReadData(DBNV_ID(APP_DB_NVM_CAS_ID, i), &pRec->changeAwareState, sizeof(uint8_t), NULL);
      WsfNvmReadData(DBNV_ID(APP_DB_NVM_CSF_ID, i), pRec->csf, ATT_CSF_LEN, NULL);
      WsfNvmReadData(DBNV_ID(APP_DB_NVM_CACHE_HASH_ID, i), &pRec->cacheByHash, sizeof(bool_t), NULL);
      WsfNvmReadData(DBNV_ID(APP_DB_NVM_HASH_ID, i), pRec->dbHash, ATT_DATABASE_HASH_LEN, NULL);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Delete the device database record with the given handle from NVM.
 *
 *  \param  hdl           Record handle.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppDbNvmDeleteRec(appDbHdl_t hdl)
{
  uint8_t recIndex = appDbFindIndx(hdl);

  if (recIndex != APP_DB_INDEX_INVALID)
  {
    WsfNvmEraseData(DBNV_ID(APP_DB_NVM_VALID_ID, recIndex), NULL);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Delete all device database records from NVM.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppDbNvmDeleteAll(void)
{
  uint8_t i;

  /* Delete all records. */
  for (i = 0; i < APP_DB_NUM_RECS; i++)
  {
    WsfNvmEraseData(DBNV_ID(APP_DB_NVM_VALID_ID, i), NULL);
  }
}
