/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Example Mesh Proxy Service Server implementation.
 *
 *  Copyright (c) 2016-2018 Arm Ltd. All Rights Reserved.
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

#include "wsf_types.h"
#include "att_api.h"
#include "att_uuid.h"
#include "wsf_trace.h"
#include "util/bstream.h"
#include "svc_ch.h"
#include "svc_mprxs.h"
#include "svc_cfg.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Characteristic read permissions */
#ifndef MPRXS_SEC_PERMIT_READ
#define MPRXS_SEC_PERMIT_READ (ATTS_PERMIT_READ)
#endif

/*! Characteristic write permissions */
#ifndef MPRXS_SEC_PERMIT_WRITE
#define MPRXS_SEC_PERMIT_WRITE  (ATTS_PERMIT_WRITE)
#endif

/**************************************************************************************************
 Service variables
**************************************************************************************************/

/* Mesh Proxy service declaration */
static const uint8_t mprxsValSvc[] = {UINT16_TO_BYTES(ATT_UUID_MESH_PROXY_SERVICE)};
static const uint16_t mprxsLenSvc = sizeof(mprxsValSvc);

/* Mesh Proxy Data In characteristic */
static const uint8_t mprxsValDataInCh[] = {ATT_PROP_WRITE_NO_RSP, UINT16_TO_BYTES(MPRXS_DIN_HDL), UINT16_TO_BYTES(ATT_UUID_MESH_PROXY_DATA_IN)};
static const uint16_t mprxsLenDataInCh = sizeof(mprxsValDataInCh);

/* Mesh Proxy Data In */
/* Note these are dummy values */
static const uint8_t mprxsValDataIn[] = { 0 };
static const uint16_t mprxsLenDataIn = sizeof(mprxsValDataIn);

/* Mesh Proxy Data Out characteristic */
static const uint8_t mprxsValDataOutCh[] = {ATT_PROP_NOTIFY, UINT16_TO_BYTES(MPRXS_DOUT_HDL), UINT16_TO_BYTES(ATT_UUID_MESH_PROXY_DATA_OUT)};
static const uint16_t mprxsLenDataOutCh = sizeof(mprxsValDataOutCh);

/* Mesh Proxy Data Out */
/* Note these are dummy values */
static const uint8_t mprxsValDataOut[] = { 0 };
static const uint16_t mprxsLenDataOut = sizeof(mprxsValDataOut);

/* Mesh Proxy Data Out client characteristic configuration */
static uint8_t mprxsValDataOutChCcc[] = {UINT16_TO_BYTES(0x0000) };
static const uint16_t mprxsLenDataOutChCcc = sizeof(mprxsValDataOutChCcc);


/* Attribute list for MPRXS group */
static const attsAttr_t mprxsList[] =
{
  /* Mesh Proxy Service declaration */
  {
    attPrimSvcUuid,
    (uint8_t *) mprxsValSvc,
    (uint16_t *) &mprxsLenSvc,
    sizeof(mprxsValSvc),
    0,
    ATTS_PERMIT_READ
  },
  /* Mesh Proxy DataIn characteristic */
  {
    attChUuid,
    (uint8_t *)mprxsValDataInCh,
    (uint16_t *) &mprxsLenDataInCh,
    sizeof(mprxsValDataInCh),
    0,
    ATTS_PERMIT_READ
  },
  /* Mesh Proxy DataIn value */
  {
    attMprxDinChUuid,
    (uint8_t *)mprxsValDataIn,
    (uint16_t *) &mprxsLenDataIn,
    ATT_DEFAULT_PAYLOAD_LEN,
    (ATTS_SET_VARIABLE_LEN | ATTS_SET_WRITE_CBACK),
    MPRXS_SEC_PERMIT_WRITE
  },
  /* Mesh Proxy DataOut characteristic */
  {
    attChUuid,
    (uint8_t *)mprxsValDataOutCh,
    (uint16_t *)&mprxsLenDataOutCh,
    sizeof(mprxsValDataOutCh),
    0,
    ATTS_PERMIT_READ
  },
  /* Mesh Proxy DataOut value */
  {
    attMprxDoutChUuid,
    (uint8_t *)mprxsValDataOut,
    (uint16_t *)&mprxsLenDataOut,
    ATT_DEFAULT_PAYLOAD_LEN,
    ATTS_SET_VARIABLE_LEN,
    0
  },
  /* Characteristic CCC descriptor */
  {
    attCliChCfgUuid,
    mprxsValDataOutChCcc,
    (uint16_t *)&mprxsLenDataOutChCcc,
    sizeof(mprxsValDataOutChCcc),
    ATTS_SET_CCC,
    (ATTS_PERMIT_READ | MPRXS_SEC_PERMIT_WRITE)
  },
};

/* MPRXS group structure */
static attsGroup_t svcMprxsGroup =
{
  NULL,
  (attsAttr_t *) mprxsList,
  NULL,
  NULL,
  MPRXS_START_HDL,
  MPRXS_END_HDL
};

/*************************************************************************************************/
/*!
 *  \brief  Add the services to the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcMprxsAddGroup(void)
{
  AttsAddGroup(&svcMprxsGroup);
}

/*************************************************************************************************/
/*!
 *  \brief  Remove the services from the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcMprxsRemoveGroup(void)
{
  AttsRemoveGroup(MPRXS_START_HDL);
}

/*************************************************************************************************/
/*!
 *  \brief  Register write callback for the service.
 *
 *  \param  writeCback  Write callback function.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcMprxsRegister(attsWriteCback_t writeCback)
{
  svcMprxsGroup.writeCback = writeCback;
}
