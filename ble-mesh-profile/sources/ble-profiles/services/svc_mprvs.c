/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Example Mesh Provisioning Service Server implementation.
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
#include "svc_mprvs.h"
#include "svc_cfg.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Characteristic read permissions */
#ifndef MPRVS_SEC_PERMIT_READ
#define MPRVS_SEC_PERMIT_READ (ATTS_PERMIT_READ)
#endif

/*! Characteristic write permissions */
#ifndef MPRVS_SEC_PERMIT_WRITE
#define MPRVS_SEC_PERMIT_WRITE  (ATTS_PERMIT_WRITE)
#endif

/**************************************************************************************************
 Service variables
**************************************************************************************************/

/* Mesh Provisioning service declaration */
static const uint8_t mprvsValSvc[] = {UINT16_TO_BYTES(ATT_UUID_MESH_PRV_SERVICE)};
static const uint16_t mprvsLenSvc = sizeof(mprvsValSvc);

/* Mesh Provisioning Data In characteristic */
static const uint8_t mprvsValDataInCh[] = {ATT_PROP_WRITE_NO_RSP, UINT16_TO_BYTES(MPRVS_DIN_HDL), UINT16_TO_BYTES(ATT_UUID_MESH_PRV_DATA_IN)};
static const uint16_t mprvsLenDataInCh = sizeof(mprvsValDataInCh);

/* Mesh Provisioning Data In */
/* Note these are dummy values */
static const uint8_t mprvsValDataIn[] = { 0 };
static const uint16_t mprvsLenDataIn = sizeof(mprvsValDataIn);

/* Mesh Provisioning Data Out characteristic */
static const uint8_t mprvsValDataOutCh[] = {ATT_PROP_NOTIFY, UINT16_TO_BYTES(MPRVS_DOUT_HDL), UINT16_TO_BYTES(ATT_UUID_MESH_PRV_DATA_OUT)};
static const uint16_t mprvsLenDataOutCh = sizeof(mprvsValDataOutCh);

/* Mesh Provisioning Data Out */
/* Note these are dummy values */
static const uint8_t mprvsValDataOut[] = { 0 };
static const uint16_t mprvsLenDataOut = sizeof(mprvsValDataOut);

/* Mesh Provisioning Data Out client characteristic configuration */
static uint8_t mprvsValDataOutChCcc[] = {UINT16_TO_BYTES(0x0000) };
static const uint16_t mprvsLenDataOutChCcc = sizeof(mprvsValDataOutChCcc);


/* Attribute list for MPRVS group */
static const attsAttr_t mprvsList[] =
{
  /* Mesh Provisioning Service declaration */
  {
    attPrimSvcUuid,
    (uint8_t *) mprvsValSvc,
    (uint16_t *) &mprvsLenSvc,
    sizeof(mprvsValSvc),
    0,
    ATTS_PERMIT_READ
  },
  /* Mesh Provisioning DataIn characteristic */
  {
    attChUuid,
    (uint8_t *)mprvsValDataInCh,
    (uint16_t *) &mprvsLenDataInCh,
    sizeof(mprvsValDataInCh),
    0,
    ATTS_PERMIT_READ
  },
  /* Mesh Provisioning DataIn value */
  {
    attMprvDinChUuid,
    (uint8_t *)mprvsValDataIn,
    (uint16_t *) &mprvsLenDataIn,
    66,
    (ATTS_SET_VARIABLE_LEN | ATTS_SET_WRITE_CBACK),
    MPRVS_SEC_PERMIT_WRITE
  },
  /* Mesh Provisioning DataOut characteristic */
  {
    attChUuid,
    (uint8_t *)mprvsValDataOutCh,
    (uint16_t *)&mprvsLenDataOutCh,
    sizeof(mprvsValDataOutCh),
    0,
    ATTS_PERMIT_READ
  },
  /* Mesh Provisioning DataOut value */
  {
    attMprvDoutChUuid,
    (uint8_t *)mprvsValDataOut,
    (uint16_t *)&mprvsLenDataOut,
    66,
    ATTS_SET_VARIABLE_LEN,
    0
  },
  /* Characteristic CCC descriptor */
  {
    attCliChCfgUuid,
    mprvsValDataOutChCcc,
    (uint16_t *)&mprvsLenDataOutChCcc,
    sizeof(mprvsValDataOutChCcc),
    ATTS_SET_CCC,
    (ATTS_PERMIT_READ | MPRVS_SEC_PERMIT_WRITE)
  },
};

/* MPRVS group structure */
static attsGroup_t svcMprvsGroup =
{
  NULL,
  (attsAttr_t *) mprvsList,
  NULL,
  NULL,
  MPRVS_START_HDL,
  MPRVS_END_HDL
};

/*************************************************************************************************/
/*!
 *  \brief  Add the services to the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcMprvsAddGroup(void)
{
  AttsAddGroup(&svcMprvsGroup);
}

/*************************************************************************************************/
/*!
 *  \brief  Remove the services from the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcMprvsRemoveGroup(void)
{
  AttsRemoveGroup(MPRVS_START_HDL);
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
void SvcMprvsRegister(attsWriteCback_t writeCback)
{
  svcMprvsGroup.writeCback = writeCback;
}
