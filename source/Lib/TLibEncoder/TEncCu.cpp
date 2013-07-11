/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2013, ITU/ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/** \file     TEncCu.cpp
    \brief    Coding Unit (CU) encoder class
*/

#include <stdio.h>
#include "TEncTop.h"
#include "TEncCu.h"
#include "TEncAnalyze.h"
#include "TEncPic.h"
#include "PPA/ppa.h"
#include "primitives.h"
#include "common.h"

#include <cmath>
#include <algorithm>
using namespace std;

using namespace x265;

//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

/**
 \param    uiTotalDepth  total number of allowable depth
 \param    uiMaxWidth    largest CU width
 \param    uiMaxHeight   largest CU height
 */
Void TEncCu::create(UChar uhTotalDepth, UInt uiMaxWidth, UInt uiMaxHeight)
{
    m_uhTotalDepth   = uhTotalDepth + 1;
    m_InterCU_2Nx2N  = new TComDataCU*[m_uhTotalDepth - 1];
    m_InterCU_2NxN   = new TComDataCU*[m_uhTotalDepth - 1];
    m_InterCU_Nx2N   = new TComDataCU*[m_uhTotalDepth - 1];
    m_IntraInInterCU = new TComDataCU*[m_uhTotalDepth - 1];
    m_MergeCU        = new TComDataCU*[m_uhTotalDepth - 1];
    m_MergeBestCU    = new TComDataCU*[m_uhTotalDepth - 1];
    m_ppcBestCU      = new TComDataCU*[m_uhTotalDepth - 1];
    m_ppcTempCU      = new TComDataCU*[m_uhTotalDepth - 1];

    m_ppcPredYuvBest = new TComYuv*[m_uhTotalDepth - 1];
    m_ppcResiYuvBest = new TShortYUV*[m_uhTotalDepth - 1];
    m_ppcRecoYuvBest = new TComYuv*[m_uhTotalDepth - 1];
    m_ppcPredYuvTemp = new TComYuv*[m_uhTotalDepth - 1];

    m_ppcPredYuvMode[0] = new TComYuv*[m_uhTotalDepth - 1];
    m_ppcPredYuvMode[1] = new TComYuv*[m_uhTotalDepth - 1];
    m_ppcPredYuvMode[2] = new TComYuv*[m_uhTotalDepth - 1];
    m_ppcPredYuvMode[3] = new TComYuv*[m_uhTotalDepth - 1];
    m_ppcPredYuvMode[4] = new TComYuv*[m_uhTotalDepth - 1];
    m_ppcPredYuvMode[5] = new TComYuv*[m_uhTotalDepth - 1];

    m_ppcResiYuvTemp = new TShortYUV*[m_uhTotalDepth - 1];
    m_ppcRecoYuvTemp = new TComYuv*[m_uhTotalDepth - 1];

    m_RecoYuvMergeBest = new TComYuv*[m_uhTotalDepth - 1];
    
    m_ppcOrigYuv     = new TComYuv*[m_uhTotalDepth - 1];

    for (Int i = 0; i < m_uhTotalDepth - 1; i++)
    {
        UInt uiNumPartitions = 1 << ((m_uhTotalDepth - i - 1) << 1);
        UInt width  = uiMaxWidth  >> i;
        UInt height = uiMaxHeight >> i;

        m_ppcBestCU[i] = new TComDataCU;
        m_ppcBestCU[i]->create(uiNumPartitions, width, height, false, uiMaxWidth >> (m_uhTotalDepth - 1));
        m_ppcTempCU[i] = new TComDataCU;
        m_ppcTempCU[i]->create(uiNumPartitions, width, height, false, uiMaxWidth >> (m_uhTotalDepth - 1));

        m_InterCU_2Nx2N[i] = new TComDataCU;
        m_InterCU_2Nx2N[i]->create(uiNumPartitions, width, height, false, uiMaxWidth >> (m_uhTotalDepth - 1));
        m_InterCU_2NxN[i] = new TComDataCU;
        m_InterCU_2NxN[i]->create(uiNumPartitions, width, height, false, uiMaxWidth >> (m_uhTotalDepth - 1));
        m_InterCU_Nx2N[i] = new TComDataCU;
        m_InterCU_Nx2N[i]->create(uiNumPartitions, width, height, false, uiMaxWidth >> (m_uhTotalDepth - 1));
        m_IntraInInterCU[i] = new TComDataCU;
        m_IntraInInterCU[i]->create(uiNumPartitions, width, height, false, uiMaxWidth >> (m_uhTotalDepth - 1));
        m_MergeCU[i] = new TComDataCU;
        m_MergeCU[i]->create(uiNumPartitions, width, height, false, uiMaxWidth >> (m_uhTotalDepth - 1));
        m_MergeBestCU[i] = new TComDataCU;
        m_MergeBestCU[i]->create(uiNumPartitions, width, height, false, uiMaxWidth >> (m_uhTotalDepth - 1));
        m_ppcPredYuvBest[i] = new TComYuv;
        m_ppcPredYuvBest[i]->create(width, height);
        m_ppcResiYuvBest[i] = new TShortYUV;
        m_ppcResiYuvBest[i]->create(width, height);
        m_ppcRecoYuvBest[i] = new TComYuv;
        m_ppcRecoYuvBest[i]->create(width, height);

        m_ppcPredYuvTemp[i] = new TComYuv;
        m_ppcPredYuvTemp[i]->create(width, height);

        for (int j = 0; j < MAX_PRED_TYPES; j++)
        {
            m_ppcPredYuvMode[j][i] = new TComYuv;
            m_ppcPredYuvMode[j][i]->create(width, height);
        }

        m_ppcResiYuvTemp[i] = new TShortYUV;
        m_ppcResiYuvTemp[i]->create(width, height);

        m_ppcRecoYuvTemp[i] = new TComYuv;
        m_ppcRecoYuvTemp[i]->create(width, height);
                
        m_RecoYuvMergeBest[i] = new TComYuv;
        m_RecoYuvMergeBest[i]->create(width, height);
        
        m_ppcOrigYuv[i] = new TComYuv;
        m_ppcOrigYuv[i]->create(width, height);
    }

    m_bEncodeDQP = false;
    m_LCUPredictionSAD = 0;
    m_addSADDepth      = 0;
    m_temporalSAD      = 0;
}

Void TEncCu::destroy()
{
    for (Int i = 0; i < m_uhTotalDepth - 1; i++)
    {
        if (m_InterCU_2Nx2N[i])
        {
            m_InterCU_2Nx2N[i]->destroy();
            delete m_InterCU_2Nx2N[i];
            m_InterCU_2Nx2N[i] = NULL;
        }
        if (m_InterCU_2NxN[i])
        {
            m_InterCU_2NxN[i]->destroy();
            delete m_InterCU_2NxN[i];
            m_InterCU_2NxN[i] = NULL;
        }
        if (m_InterCU_Nx2N[i])
        {
            m_InterCU_Nx2N[i]->destroy();
            delete m_InterCU_Nx2N[i];
            m_InterCU_Nx2N[i] = NULL;
        }
        if (m_IntraInInterCU[i])
        {
            m_IntraInInterCU[i]->destroy();
            delete m_IntraInInterCU[i];
            m_IntraInInterCU[i] = NULL;
        }
        if (m_MergeCU[i])
        {
            m_MergeCU[i]->destroy();
            delete m_MergeCU[i];
            m_MergeCU[i] = NULL;
        }
        if (m_MergeBestCU[i])
        {
            m_MergeBestCU[i]->destroy();
            delete m_MergeBestCU[i];
            m_MergeBestCU[i] = NULL;
        }
        if (m_ppcBestCU[i])
        {
            m_ppcBestCU[i]->destroy();
            delete m_ppcBestCU[i];
            m_ppcBestCU[i] = NULL;
        }
        if (m_ppcTempCU[i])
        {
            m_ppcTempCU[i]->destroy();
            delete m_ppcTempCU[i];
            m_ppcTempCU[i] = NULL;
        }
        if (m_ppcPredYuvBest[i])
        {
            m_ppcPredYuvBest[i]->destroy();
            delete m_ppcPredYuvBest[i];
            m_ppcPredYuvBest[i] = NULL;
        }
        if (m_ppcResiYuvBest[i])
        {
            m_ppcResiYuvBest[i]->destroy();
            delete m_ppcResiYuvBest[i];
            m_ppcResiYuvBest[i] = NULL;
        }
        if (m_ppcRecoYuvBest[i])
        {
            m_ppcRecoYuvBest[i]->destroy();
            delete m_ppcRecoYuvBest[i];
            m_ppcRecoYuvBest[i] = NULL;
        }
        if (m_ppcPredYuvTemp[i])
        {
            m_ppcPredYuvTemp[i]->destroy();
            delete m_ppcPredYuvTemp[i];
            m_ppcPredYuvTemp[i] = NULL;
        }
        for (int j = 0; j < MAX_PRED_TYPES; j++)
        {
            m_ppcPredYuvMode[j][i]->destroy();
            delete m_ppcPredYuvMode[j][i];
            m_ppcPredYuvMode[j][i] = NULL;
        }

        if (m_ppcResiYuvTemp[i])
        {
            m_ppcResiYuvTemp[i]->destroy();
            delete m_ppcResiYuvTemp[i];
            m_ppcResiYuvTemp[i] = NULL;
        }
        if (m_ppcRecoYuvTemp[i])
        {
            m_ppcRecoYuvTemp[i]->destroy();
            delete m_ppcRecoYuvTemp[i];
            m_ppcRecoYuvTemp[i] = NULL;
        }
        if (m_RecoYuvMergeBest[i])
        {
            m_RecoYuvMergeBest[i]->destroy();
            delete m_RecoYuvMergeBest[i];
            m_RecoYuvMergeBest[i] = NULL;
        }

        if (m_ppcOrigYuv[i])
        {
            m_ppcOrigYuv[i]->destroy();
            delete m_ppcOrigYuv[i];
            m_ppcOrigYuv[i] = NULL;
        }
    }

    if (m_InterCU_2Nx2N)
    {
        delete [] m_InterCU_2Nx2N;
        m_InterCU_2Nx2N = NULL;
    }

    if (m_InterCU_2NxN)
    {
        delete [] m_InterCU_2NxN;
        m_InterCU_2NxN = NULL;
    }
    if (m_InterCU_Nx2N)
    {
        delete [] m_InterCU_Nx2N;
        m_InterCU_Nx2N = NULL;
    }
    if (m_IntraInInterCU)
    {
        delete [] m_IntraInInterCU;
        m_IntraInInterCU = NULL;
    }
    if (m_MergeCU)
    {
        delete [] m_MergeCU;
        m_MergeCU = NULL;
    }
    if (m_MergeBestCU)
    {
        delete [] m_MergeBestCU;
        m_MergeBestCU = NULL;
    }
    if (m_ppcBestCU)
    {
        delete [] m_ppcBestCU;
        m_ppcBestCU = NULL;
    }
    if (m_ppcTempCU)
    {
        delete [] m_ppcTempCU;
        m_ppcTempCU = NULL;
    }

    if (m_ppcPredYuvBest)
    {
        delete [] m_ppcPredYuvBest;
        m_ppcPredYuvBest = NULL;
    }
    if (m_ppcResiYuvBest)
    {
        delete [] m_ppcResiYuvBest;
        m_ppcResiYuvBest = NULL;
    }
    if (m_ppcRecoYuvBest)
    {
        delete [] m_ppcRecoYuvBest;
        m_ppcRecoYuvBest = NULL;
    }
    if (m_RecoYuvMergeBest)
    {
        delete []m_RecoYuvMergeBest;
        m_RecoYuvMergeBest = NULL;
    }
    if (m_ppcPredYuvTemp)
    {
        delete [] m_ppcPredYuvTemp;
        m_ppcPredYuvTemp = NULL;
    }
    if (m_ppcPredYuvMode[0])
    {
        delete [] m_ppcPredYuvMode[0];
        m_ppcPredYuvMode[0] = NULL;
    }
    if (m_ppcPredYuvMode[1])
    {
        delete [] m_ppcPredYuvMode[1];
        m_ppcPredYuvMode[1] = NULL;
    }
    if (m_ppcPredYuvMode[2])
    {
        delete [] m_ppcPredYuvMode[2];
        m_ppcPredYuvMode[2] = NULL;
    }
    if (m_ppcPredYuvMode[3])
    {
        delete [] m_ppcPredYuvMode[3];
        m_ppcPredYuvMode[3] = NULL;
    }
    if (m_ppcPredYuvMode[4])
    {
        delete [] m_ppcPredYuvMode[4];
        m_ppcPredYuvMode[4] = NULL;
    }
    if (m_ppcPredYuvMode[5])
    {
        delete [] m_ppcPredYuvMode[5];
        m_ppcPredYuvMode[5] = NULL;
    }

    if (m_ppcResiYuvTemp)
    {
        delete [] m_ppcResiYuvTemp;
        m_ppcResiYuvTemp = NULL;
    }
    if (m_ppcRecoYuvTemp)
    {
        delete [] m_ppcRecoYuvTemp;
        m_ppcRecoYuvTemp = NULL;
    }
    if (m_ppcOrigYuv)
    {
        delete [] m_ppcOrigYuv;
        m_ppcOrigYuv = NULL;
    }
}

/** \param    pcEncTop      pointer of encoder class
 */
Void TEncCu::init(TEncTop* pcEncTop)
{
    m_pcEncCfg        = pcEncTop;
    m_pcPredSearch    = NULL;
    m_pcTrQuant       = NULL;
    m_pcRdCost        = NULL;
    m_entropyCoder    = NULL;
    m_rdSbacCoders    = NULL;
    m_rdGoOnSbacCoder = NULL;
    m_pcBitCounter    = NULL;
    m_pcRateCtrl      = pcEncTop->getRateCtrl();
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/** \param  rpcCU pointer of CU data class
 */
#if CU_STAT_LOGFILE
extern int totalCU;
extern int cntInter[4], cntIntra[4], cntSplit[4], cntIntraNxN;
extern  int cuInterDistribution[4][4], cuIntraDistribution[4][3];
extern  int cntSkipCu[4],  cntTotalCu[4];
extern FILE* fp1;
bool mergeFlag = 0;
#endif

Void TEncCu::compressCU(TComDataCU* pcCu)
{
    // initialize CU data
    m_ppcBestCU[0]->initCU(pcCu->getPic(), pcCu->getAddr());
    m_ppcTempCU[0]->initCU(pcCu->getPic(), pcCu->getAddr());
#if CU_STAT_LOGFILE
    totalCU++;
    if (pcCu->getSlice()->getSliceType() != I_SLICE)
        fprintf(fp1, "CU number : %d \n", totalCU);
#endif
    //printf("compressCU[%2d]: Best=0x%08X, Temp=0x%08X\n", omp_get_thread_num(), m_ppcBestCU[0], m_ppcTempCU[0]);

    m_addSADDepth      = 0;
    m_LCUPredictionSAD = 0;
    m_temporalSAD      = 0;

    m_pcPredSearch->setRDSbacCoder(m_rdSbacCoders);
    m_pcPredSearch->setEntropyCoder(m_entropyCoder);
    m_pcPredSearch->setRDGoOnSbacCoder(m_rdGoOnSbacCoder);

    // analysis of CU

    if (m_ppcBestCU[0]->getSlice()->getSliceType() == I_SLICE)
        xCompressIntraCU(m_ppcBestCU[0], m_ppcTempCU[0], NULL, 0);
    else
    {
        if (!m_pcEncCfg->getUseRDO())
        {
            TComDataCU* rpcBestCU = NULL;

            /* At the start of analysis, the best CU is a null pointer
            On return, it points to the CU encode with best chosen mode*/
            xCompressInterCU(rpcBestCU, m_ppcTempCU[0], pcCu, 0, 0);
        }
        else
            xCompressCU(m_ppcBestCU[0], m_ppcTempCU[0], pcCu, 0, 0);
    }
    if (m_pcEncCfg->getUseAdaptQpSelect())
    {
        if (pcCu->getSlice()->getSliceType() != I_SLICE) //IIII
        {
            xLcuCollectARLStats(pcCu);
        }
    }
}

/** \param  cu  pointer of CU data class
 */
Void TEncCu::encodeCU(TComDataCU* cu)
{
    if (cu->getSlice()->getPPS()->getUseDQP())
    {
        setdQPFlag(true);
    }

    // Encode CU data
    xEncodeCU(cu, 0, 0);
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

/** Derive small set of test modes for AMP encoder speed-up
 *\param   rpcBestCU
 *\param   eParentPartSize
 *\param   bTestAMP_Hor
 *\param   bTestAMP_Ver
 *\param   bTestMergeAMP_Hor
 *\param   bTestMergeAMP_Ver
 *\returns Void
*/
Void TEncCu::deriveTestModeAMP(TComDataCU *&rpcBestCU, PartSize eParentPartSize, Bool &bTestAMP_Hor, Bool &bTestAMP_Ver, Bool &bTestMergeAMP_Hor, Bool &bTestMergeAMP_Ver)
{
    if (rpcBestCU->getPartitionSize(0) == SIZE_2NxN)
    {
        bTestAMP_Hor = true;
    }
    else if (rpcBestCU->getPartitionSize(0) == SIZE_Nx2N)
    {
        bTestAMP_Ver = true;
    }
    else if (rpcBestCU->getPartitionSize(0) == SIZE_2Nx2N && rpcBestCU->getMergeFlag(0) == false && rpcBestCU->isSkipped(0) == false)
    {
        bTestAMP_Hor = true;
        bTestAMP_Ver = true;
    }

    //! Utilizing the partition size of parent PU
    if (eParentPartSize >= SIZE_2NxnU && eParentPartSize <= SIZE_nRx2N)
    {
        bTestMergeAMP_Hor = true;
        bTestMergeAMP_Ver = true;
    }

    if (eParentPartSize == SIZE_NONE) //! if parent is intra
    {
        if (rpcBestCU->getPartitionSize(0) == SIZE_2NxN)
        {
            bTestMergeAMP_Hor = true;
        }
        else if (rpcBestCU->getPartitionSize(0) == SIZE_Nx2N)
        {
            bTestMergeAMP_Ver = true;
        }
    }

    if (rpcBestCU->getPartitionSize(0) == SIZE_2Nx2N && rpcBestCU->isSkipped(0) == false)
    {
        bTestMergeAMP_Hor = true;
        bTestMergeAMP_Ver = true;
    }

    if (rpcBestCU->getWidth(0) == 64)
    {
        bTestAMP_Hor = false;
        bTestAMP_Ver = false;
    }
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

/** Compress a CU block recursively with enabling sub-LCU-level delta QP
 *\param   rpcBestCU
 *\param   rpcTempCU
 *\param   depth
 *\returns Void
 *
 *- for loop of QP value to compress the current CU with all possible QP
*/

Void TEncCu::xCompressIntraCU(TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, TComDataCU* rpcParentBestCU, UInt depth, PartSize eParentPartSize)
{
#if CU_STAT_LOGFILE
    cntTotalCu[depth]++;
    int boundaryCu = 0;
#endif
    m_abortFlag = false;
    TComPic* pcPic = rpcBestCU->getPic();

    //PPAScopeEvent(TEncCu_xCompressIntraCU + depth);

    // get Original YUV data from picture
    m_ppcOrigYuv[depth]->copyFromPicYuv(pcPic->getPicYuvOrg(), rpcBestCU->getAddr(), rpcBestCU->getZorderIdxInCU());

    // variables for fast encoder decision
    Bool    bTrySplit    = true;

    // variable for Early CU determination
    Bool    bSubBranch = true;

    // variable for Cbf fast mode PU decision
    Bool doNotBlockPu = true;
    Bool earlyDetectionSkipMode = false;

    Bool bTrySplitDQP  = true;

    Bool bBoundary = false;
    UInt uiLPelX   = rpcBestCU->getCUPelX();
    UInt uiRPelX   = uiLPelX + rpcBestCU->getWidth(0)  - 1;
    UInt uiTPelY   = rpcBestCU->getCUPelY();
    UInt uiBPelY   = uiTPelY + rpcBestCU->getHeight(0) - 1;

    Int iQP = m_pcEncCfg->getUseRateCtrl() ? m_pcRateCtrl->getRCQP() : rpcTempCU->getQP(0);

    // If slice start or slice end is within this cu...
    TComSlice * pcSlice = rpcTempCU->getPic()->getSlice();
    Bool bSliceEnd = (pcSlice->getSliceCurEndCUAddr() > rpcTempCU->getSCUAddr() && pcSlice->getSliceCurEndCUAddr() < rpcTempCU->getSCUAddr() + rpcTempCU->getTotalNumPart());
    Bool bInsidePicture = (uiRPelX < rpcBestCU->getSlice()->getSPS()->getPicWidthInLumaSamples()) && (uiBPelY < rpcBestCU->getSlice()->getSPS()->getPicHeightInLumaSamples());

    //Data for splitting
    UChar uhNextDepth = depth + 1;
    UInt uiPartUnitIdx = 0;
    TComDataCU* pcSubBestPartCU[4], *pcSubTempPartCU[4];

    //We need to split; so dont try these modes
    if (!bSliceEnd && bInsidePicture)
    {
        // variables for fast encoder decision
        bTrySplit = true;

        rpcTempCU->initEstData(depth, iQP);

        bTrySplitDQP = bTrySplit;

        if (depth <= m_addSADDepth)
        {
            m_LCUPredictionSAD += m_temporalSAD;
            m_addSADDepth = depth;
        }

        rpcTempCU->initEstData(depth, iQP);

        xCheckRDCostIntra(rpcBestCU, rpcTempCU, SIZE_2Nx2N);
        rpcTempCU->initEstData(depth, iQP);

        if (depth == g_maxCUDepth - g_addCUDepth)
        {
            if (rpcTempCU->getWidth(0) > (1 << rpcTempCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize()))
            {
                xCheckRDCostIntra(rpcBestCU, rpcTempCU, SIZE_NxN);
                rpcTempCU->initEstData(depth, iQP);
            }
        }

        m_entropyCoder->resetBits();
        m_entropyCoder->encodeSplitFlag(rpcBestCU, 0, depth, true);
        rpcBestCU->getTotalBits() += m_entropyCoder->getNumberOfWrittenBits(); // split bits
        rpcBestCU->getTotalBins() += ((TEncBinCABAC*)((TEncSbac*)m_entropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
        rpcBestCU->getTotalCost()  = m_pcRdCost->calcRdCost(rpcBestCU->getTotalDistortion(), rpcBestCU->getTotalBits());

        // Early CU determination
        if (rpcBestCU->isSkipped(0))
        {
#if CU_STAT_LOGFILE
            cntSkipCu[depth]++;
#endif
            bSubBranch = false;
        }
        else
        {
            bSubBranch = true;
        }
    }
    else if (!(bSliceEnd && bInsidePicture))
    {
        bBoundary = true;
        m_addSADDepth++;
    }

    rpcTempCU->initEstData(depth, iQP);

    // further split
    if (bSubBranch && bTrySplitDQP && depth < g_maxCUDepth - g_addCUDepth)
    {
        for (; uiPartUnitIdx < 4; uiPartUnitIdx++)
        {
            pcSubBestPartCU[uiPartUnitIdx]     = m_ppcBestCU[uhNextDepth];
            pcSubTempPartCU[uiPartUnitIdx]     = m_ppcTempCU[uhNextDepth];

            pcSubBestPartCU[uiPartUnitIdx]->initSubCU(rpcTempCU, uiPartUnitIdx, uhNextDepth, iQP);     // clear sub partition datas or init.
            pcSubTempPartCU[uiPartUnitIdx]->initSubCU(rpcTempCU, uiPartUnitIdx, uhNextDepth, iQP);     // clear sub partition datas or init.

            Bool bInSlice = pcSubBestPartCU[uiPartUnitIdx]->getSCUAddr() < pcSlice->getSliceCurEndCUAddr();
            if (bInSlice && (pcSubBestPartCU[uiPartUnitIdx]->getCUPelX() < pcSlice->getSPS()->getPicWidthInLumaSamples()) && (pcSubBestPartCU[uiPartUnitIdx]->getCUPelY() < pcSlice->getSPS()->getPicHeightInLumaSamples()))
            {
                if (0 == uiPartUnitIdx) //initialize RD with previous depth buffer
                {
                    m_rdSbacCoders[uhNextDepth][CI_CURR_BEST]->load(m_rdSbacCoders[depth][CI_CURR_BEST]);
                }
                else
                {
                    m_rdSbacCoders[uhNextDepth][CI_CURR_BEST]->load(m_rdSbacCoders[uhNextDepth][CI_NEXT_BEST]);
                }

                // The following if condition has to be commented out in case the early Abort based on comparison of parentCu cost, childCU cost is not required.
                if (rpcBestCU->isIntra(0))
                {
                    xCompressIntraCU(pcSubBestPartCU[uiPartUnitIdx], pcSubTempPartCU[uiPartUnitIdx], rpcBestCU, uhNextDepth, SIZE_NONE);
                }
                else
                {
                    xCompressIntraCU(pcSubBestPartCU[uiPartUnitIdx], pcSubTempPartCU[uiPartUnitIdx], rpcBestCU, uhNextDepth, rpcBestCU->getPartitionSize(0));
                }
                {
                    rpcTempCU->copyPartFrom(pcSubBestPartCU[uiPartUnitIdx], uiPartUnitIdx, uhNextDepth); // Keep best part data to current temporary data.
                    xCopyYuv2Tmp(pcSubBestPartCU[uiPartUnitIdx]->getTotalNumPart() * uiPartUnitIdx, uhNextDepth);
                }
            }
            else if (bInSlice)
            {
                pcSubBestPartCU[uiPartUnitIdx]->copyToPic(uhNextDepth);
                rpcTempCU->copyPartFrom(pcSubBestPartCU[uiPartUnitIdx], uiPartUnitIdx, uhNextDepth);
#if CU_STAT_LOGFILE
                boundaryCu++;
#endif
            }
        }

        if (!bBoundary)
        {
            m_entropyCoder->resetBits();
            m_entropyCoder->encodeSplitFlag(rpcTempCU, 0, depth, true);

            rpcTempCU->getTotalBits() += m_entropyCoder->getNumberOfWrittenBits();     // split bits
            rpcTempCU->getTotalBins() += ((TEncBinCABAC*)((TEncSbac*)m_entropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
        }
        rpcTempCU->getTotalCost() = m_pcRdCost->calcRdCost(rpcTempCU->getTotalDistortion(), rpcTempCU->getTotalBits());

        if ((g_maxCUWidth >> depth) == rpcTempCU->getSlice()->getPPS()->getMinCuDQPSize() && rpcTempCU->getSlice()->getPPS()->getUseDQP())
        {
            Bool hasResidual = false;
            for (UInt uiBlkIdx = 0; uiBlkIdx < rpcTempCU->getTotalNumPart(); uiBlkIdx++)
            {
                if (rpcTempCU->getCbf(uiBlkIdx, TEXT_LUMA) || rpcTempCU->getCbf(uiBlkIdx, TEXT_CHROMA_U) || rpcTempCU->getCbf(uiBlkIdx, TEXT_CHROMA_V))
                {
                    hasResidual = true;
                    break;
                }
            }

            UInt uiTargetPartIdx;
            uiTargetPartIdx = 0;
            if (hasResidual)
            {
                Bool foundNonZeroCbf = false;
                rpcTempCU->setQPSubCUs(rpcTempCU->getRefQP(uiTargetPartIdx), rpcTempCU, 0, depth, foundNonZeroCbf);
                assert(foundNonZeroCbf);
            }
            else
            {
                rpcTempCU->setQPSubParts(rpcTempCU->getRefQP(uiTargetPartIdx), 0, depth);     // set QP to default QP
            }
        }

        m_rdSbacCoders[uhNextDepth][CI_NEXT_BEST]->store(m_rdSbacCoders[depth][CI_TEMP_BEST]);
#if CU_STAT_LOGFILE
        if (rpcBestCU->getTotalCost() < rpcTempCU->getTotalCost())
        {
            cntIntra[depth]++;
            cntIntra[depth + 1] = cntIntra[depth + 1] - 4 + boundaryCu;
        }
#endif
        xCheckBestMode(rpcBestCU, rpcTempCU, depth);                                 // RD compare current larger prediction
        // with sub partitioned prediction.
    }

#if CU_STAT_LOGFILE
    if (depth == 3 && bSubBranch)
    {
        cntIntra[depth]++;
    }
#endif
    rpcBestCU->copyToPic(depth);                                                   // Copy Best data to Picture for next partition prediction.
    xCopyYuv2Pic(rpcBestCU->getPic(), rpcBestCU->getAddr(), rpcBestCU->getZorderIdxInCU(), depth, depth, rpcBestCU, uiLPelX, uiTPelY);   // Copy Yuv data to picture Yuv

    if (bBoundary || (bSliceEnd && bInsidePicture))
    {
        return;
    }

    // Assert if Best prediction mode is NONE
    // Selected mode's RD-cost must be not MAX_DOUBLE.
    assert(rpcBestCU->getPartitionSize(0) != SIZE_NONE);
    assert(rpcBestCU->getPredictionMode(0) != MODE_NONE);
    assert(rpcBestCU->getTotalCost() != MAX_DOUBLE);
}

Void TEncCu::xCompressCU(TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, TComDataCU* rpcParentBestCU, UInt depth, UInt /*uiPartUnitIdx*/, PartSize eParentPartSize)
{
#if CU_STAT_LOGFILE
    cntTotalCu[depth]++;
#endif
    m_abortFlag = false;
    TComPic* pcPic = rpcBestCU->getPic();

    //PPAScopeEvent(TEncCu_xCompressCU + depth);

    // get Original YUV data from picture
    m_ppcOrigYuv[depth]->copyFromPicYuv(pcPic->getPicYuvOrg(), rpcBestCU->getAddr(), rpcBestCU->getZorderIdxInCU());

    // variables for fast encoder decision
    Bool    bTrySplit    = true;

    // variable for Early CU determination
    Bool    bSubBranch = true;

    // variable for Cbf fast mode PU decision
    Bool doNotBlockPu = true;
    Bool earlyDetectionSkipMode = false;

    Bool bTrySplitDQP  = true;

    Bool bBoundary = false;
    UInt uiLPelX   = rpcBestCU->getCUPelX();
    UInt uiRPelX   = uiLPelX + rpcBestCU->getWidth(0)  - 1;
    UInt uiTPelY   = rpcBestCU->getCUPelY();
    UInt uiBPelY   = uiTPelY + rpcBestCU->getHeight(0) - 1;

    Int iQP = m_pcEncCfg->getUseRateCtrl() ? m_pcRateCtrl->getRCQP() : rpcTempCU->getQP(0);

    // If slice start or slice end is within this cu...
    TComSlice * pcSlice = rpcTempCU->getPic()->getSlice();
    Bool bSliceEnd = (pcSlice->getSliceCurEndCUAddr() > rpcTempCU->getSCUAddr() && pcSlice->getSliceCurEndCUAddr() < rpcTempCU->getSCUAddr() + rpcTempCU->getTotalNumPart());
    Bool bInsidePicture = (uiRPelX < rpcBestCU->getSlice()->getSPS()->getPicWidthInLumaSamples()) && (uiBPelY < rpcBestCU->getSlice()->getSPS()->getPicHeightInLumaSamples());
    // We need to split, so don't try these modes.
    if (!bSliceEnd && bInsidePicture)
    {
        // variables for fast encoder decision
        bTrySplit    = true;

        rpcTempCU->initEstData(depth, iQP);

        // do inter modes, SKIP and 2Nx2N
        if (rpcBestCU->getSlice()->getSliceType() != I_SLICE)
        {
            // 2Nx2N
            if (m_pcEncCfg->getUseEarlySkipDetection())
            {
                xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_2Nx2N);
                rpcTempCU->initEstData(depth, iQP);                              //by Competition for inter_2Nx2N
            }
#if CU_STAT_LOGFILE
            mergeFlag = 1;
#endif
            // SKIP
            xCheckRDCostMerge2Nx2N(rpcBestCU, rpcTempCU, &earlyDetectionSkipMode, m_ppcPredYuvBest[depth], m_ppcRecoYuvBest[depth]); //by Merge for inter_2Nx2N
#if CU_STAT_LOGFILE
            mergeFlag = 0;
#endif
            rpcTempCU->initEstData(depth, iQP);

            if (!m_pcEncCfg->getUseEarlySkipDetection())
            {
                // 2Nx2N, NxN
                xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_2Nx2N);
                rpcTempCU->initEstData(depth, iQP);
                if (m_pcEncCfg->getUseCbfFastMode())
                {
                    doNotBlockPu = rpcBestCU->getQtRootCbf(0) != 0;
                }
            }
        }

        bTrySplitDQP = bTrySplit;

        if (depth <= m_addSADDepth)
        {
            m_LCUPredictionSAD += m_temporalSAD;
            m_addSADDepth = depth;
        }

        if (!earlyDetectionSkipMode)
        {
            rpcTempCU->initEstData(depth, iQP);

            // do inter modes, NxN, 2NxN, and Nx2N
            if (rpcBestCU->getSlice()->getSliceType() != I_SLICE)
            {
                // 2Nx2N, NxN
                if (!((rpcBestCU->getWidth(0) == 8) && (rpcBestCU->getHeight(0) == 8)))
                {
                    if (depth == g_maxCUDepth - g_addCUDepth && doNotBlockPu)
                    {
                        xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_NxN);
                        rpcTempCU->initEstData(depth, iQP);
                    }
                }

                if (m_pcEncCfg->getUseRectInter())
                {
                    // 2NxN, Nx2N
                    if (doNotBlockPu)
                    {
                        xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_Nx2N);
                        rpcTempCU->initEstData(depth, iQP);
                        if (m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_Nx2N)
                        {
                            doNotBlockPu = rpcBestCU->getQtRootCbf(0) != 0;
                        }
                    }
                    if (doNotBlockPu)
                    {
                        xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_2NxN);
                        rpcTempCU->initEstData(depth, iQP);
                        if (m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxN)
                        {
                            doNotBlockPu = rpcBestCU->getQtRootCbf(0) != 0;
                        }
                    }
                }

                //! Try AMP (SIZE_2NxnU, SIZE_2NxnD, SIZE_nLx2N, SIZE_nRx2N)
                if (pcPic->getSlice()->getSPS()->getAMPAcc(depth))
                {
                    Bool bTestAMP_Hor = false, bTestAMP_Ver = false;
                    Bool bTestMergeAMP_Hor = false, bTestMergeAMP_Ver = false;

                    deriveTestModeAMP(rpcBestCU, eParentPartSize, bTestAMP_Hor, bTestAMP_Ver, bTestMergeAMP_Hor, bTestMergeAMP_Ver);

                    //! Do horizontal AMP
                    if (bTestAMP_Hor)
                    {
                        if (doNotBlockPu)
                        {
                            xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_2NxnU);
                            rpcTempCU->initEstData(depth, iQP);
                            if (m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxnU)
                            {
                                doNotBlockPu = rpcBestCU->getQtRootCbf(0) != 0;
                            }
                        }
                        if (doNotBlockPu)
                        {
                            xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_2NxnD);
                            rpcTempCU->initEstData(depth, iQP);
                            if (m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxnD)
                            {
                                doNotBlockPu = rpcBestCU->getQtRootCbf(0) != 0;
                            }
                        }
                    }
                    else if (bTestMergeAMP_Hor)
                    {
                        if (doNotBlockPu)
                        {
                            xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_2NxnU, true);
                            rpcTempCU->initEstData(depth, iQP);
                            if (m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxnU)
                            {
                                doNotBlockPu = rpcBestCU->getQtRootCbf(0) != 0;
                            }
                        }
                        if (doNotBlockPu)
                        {
                            xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_2NxnD, true);
                            rpcTempCU->initEstData(depth, iQP);
                            if (m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxnD)
                            {
                                doNotBlockPu = rpcBestCU->getQtRootCbf(0) != 0;
                            }
                        }
                    }

                    //! Do horizontal AMP
                    if (bTestAMP_Ver)
                    {
                        if (doNotBlockPu)
                        {
                            xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_nLx2N);
                            rpcTempCU->initEstData(depth, iQP);
                            if (m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_nLx2N)
                            {
                                doNotBlockPu = rpcBestCU->getQtRootCbf(0) != 0;
                            }
                        }
                        if (doNotBlockPu)
                        {
                            xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_nRx2N);
                            rpcTempCU->initEstData(depth, iQP);
                        }
                    }
                    else if (bTestMergeAMP_Ver)
                    {
                        if (doNotBlockPu)
                        {
                            xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_nLx2N, true);
                            rpcTempCU->initEstData(depth, iQP);
                            if (m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_nLx2N)
                            {
                                doNotBlockPu = rpcBestCU->getQtRootCbf(0) != 0;
                            }
                        }
                        if (doNotBlockPu)
                        {
                            xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_nRx2N, true);
                            rpcTempCU->initEstData(depth, iQP);
                        }
                    }
                }
            }

            // do normal intra modes
            // speedup for inter frames
            if (rpcBestCU->getSlice()->getSliceType() == I_SLICE ||
                rpcBestCU->getCbf(0, TEXT_LUMA) != 0   ||
                rpcBestCU->getCbf(0, TEXT_CHROMA_U) != 0   ||
                rpcBestCU->getCbf(0, TEXT_CHROMA_V) != 0) // avoid very complex intra if it is unlikely
            {
                xCheckRDCostIntrainInter(rpcBestCU, rpcTempCU, SIZE_2Nx2N);
                rpcTempCU->initEstData(depth, iQP);

                if (depth == g_maxCUDepth - g_addCUDepth)
                {
                    if (rpcTempCU->getWidth(0) > (1 << rpcTempCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize()))
                    {
                        xCheckRDCostIntrainInter(rpcBestCU, rpcTempCU, SIZE_NxN);
                        rpcTempCU->initEstData(depth, iQP);
                    }
                }
            }
            // test PCM
            if (pcPic->getSlice()->getSPS()->getUsePCM()
                && rpcTempCU->getWidth(0) <= (1 << pcPic->getSlice()->getSPS()->getPCMLog2MaxSize())
                && rpcTempCU->getWidth(0) >= (1 << pcPic->getSlice()->getSPS()->getPCMLog2MinSize()))
            {
                UInt uiRawBits = (2 * g_bitDepthY + g_bitDepthC) * rpcBestCU->getWidth(0) * rpcBestCU->getHeight(0) / 2;
                UInt uiBestBits = rpcBestCU->getTotalBits();
                if ((uiBestBits > uiRawBits) || (rpcBestCU->getTotalCost() > m_pcRdCost->calcRdCost(0, uiRawBits)))
                {
                    xCheckIntraPCM(rpcBestCU, rpcTempCU);
                    rpcTempCU->initEstData(depth, iQP);
                }
            }
        }

        m_entropyCoder->resetBits();
        m_entropyCoder->encodeSplitFlag(rpcBestCU, 0, depth, true);
        rpcBestCU->getTotalBits() += m_entropyCoder->getNumberOfWrittenBits(); // split bits
        rpcBestCU->getTotalBins() += ((TEncBinCABAC*)((TEncSbac*)m_entropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
        rpcBestCU->getTotalCost()  = m_pcRdCost->calcRdCost(rpcBestCU->getTotalDistortion(), rpcBestCU->getTotalBits());

        // Early CU determination
        if (rpcBestCU->isSkipped(0))
        {
#if CU_STAT_LOGFILE
            cntSkipCu[depth]++;
#endif
            bSubBranch = false;
        }
        else
        {
            bSubBranch = true;
        }
    }
    else if (!(bSliceEnd && bInsidePicture))
    {
        bBoundary = true;
        m_addSADDepth++;
    }

    // copy original YUV samples to PCM buffer
    if (rpcBestCU->isLosslessCoded(0) && (rpcBestCU->getIPCMFlag(0) == false))
    {
        xFillPCMBuffer(rpcBestCU, m_ppcOrigYuv[depth]);
    }

    rpcTempCU->initEstData(depth, iQP);

    // further split
    if (bSubBranch && bTrySplitDQP && depth < g_maxCUDepth - g_addCUDepth)
    {
        UChar       uhNextDepth         = depth + 1;
        TComDataCU* pcSubBestPartCU     = m_ppcBestCU[uhNextDepth];
        TComDataCU* pcSubTempPartCU     = m_ppcTempCU[uhNextDepth];
        UInt uiPartUnitIdx = 0;
        for (; uiPartUnitIdx < 4; uiPartUnitIdx++)
        {
            pcSubBestPartCU->initSubCU(rpcTempCU, uiPartUnitIdx, uhNextDepth, iQP);     // clear sub partition datas or init.
            pcSubTempPartCU->initSubCU(rpcTempCU, uiPartUnitIdx, uhNextDepth, iQP);     // clear sub partition datas or init.

            Bool bInSlice = pcSubBestPartCU->getSCUAddr() < pcSlice->getSliceCurEndCUAddr();
            if (bInSlice && (pcSubBestPartCU->getCUPelX() < pcSlice->getSPS()->getPicWidthInLumaSamples()) && (pcSubBestPartCU->getCUPelY() < pcSlice->getSPS()->getPicHeightInLumaSamples()))
            {
                if (0 == uiPartUnitIdx) //initialize RD with previous depth buffer
                {
                    m_rdSbacCoders[uhNextDepth][CI_CURR_BEST]->load(m_rdSbacCoders[depth][CI_CURR_BEST]);
                }
                else
                {
                    m_rdSbacCoders[uhNextDepth][CI_CURR_BEST]->load(m_rdSbacCoders[uhNextDepth][CI_NEXT_BEST]);
                }

                // The following if condition has to be commented out in case the early Abort based on comparison of parentCu cost, childCU cost is not required.
                if (rpcBestCU->isIntra(0))
                {
                    xCompressCU(pcSubBestPartCU, pcSubTempPartCU, rpcBestCU, uhNextDepth, SIZE_NONE);
                }
                else
                {
                    xCompressCU(pcSubBestPartCU, pcSubTempPartCU, rpcBestCU, uhNextDepth, rpcBestCU->getPartitionSize(0));
                }
                {
                    rpcTempCU->copyPartFrom(pcSubBestPartCU, uiPartUnitIdx, uhNextDepth); // Keep best part data to current temporary data.
                    xCopyYuv2Tmp(pcSubBestPartCU->getTotalNumPart() * uiPartUnitIdx, uhNextDepth);
                }
            }
            else if (bInSlice)
            {
                pcSubBestPartCU->copyToPic(uhNextDepth);
                rpcTempCU->copyPartFrom(pcSubBestPartCU, uiPartUnitIdx, uhNextDepth);
            }
        }

        if (!bBoundary)
        {
            m_entropyCoder->resetBits();
            m_entropyCoder->encodeSplitFlag(rpcTempCU, 0, depth, true);

            rpcTempCU->getTotalBits() += m_entropyCoder->getNumberOfWrittenBits();     // split bits
            rpcTempCU->getTotalBins() += ((TEncBinCABAC*)((TEncSbac*)m_entropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
        }
        rpcTempCU->getTotalCost() = m_pcRdCost->calcRdCost(rpcTempCU->getTotalDistortion(), rpcTempCU->getTotalBits());

        if ((g_maxCUWidth >> depth) == rpcTempCU->getSlice()->getPPS()->getMinCuDQPSize() && rpcTempCU->getSlice()->getPPS()->getUseDQP())
        {
            Bool hasResidual = false;
            for (UInt uiBlkIdx = 0; uiBlkIdx < rpcTempCU->getTotalNumPart(); uiBlkIdx++)
            {
                if (rpcTempCU->getCbf(uiBlkIdx, TEXT_LUMA) || rpcTempCU->getCbf(uiBlkIdx, TEXT_CHROMA_U) || rpcTempCU->getCbf(uiBlkIdx, TEXT_CHROMA_V))
                {
                    hasResidual = true;
                    break;
                }
            }

            UInt uiTargetPartIdx;
            uiTargetPartIdx = 0;
            if (hasResidual)
            {
                Bool foundNonZeroCbf = false;
                rpcTempCU->setQPSubCUs(rpcTempCU->getRefQP(uiTargetPartIdx), rpcTempCU, 0, depth, foundNonZeroCbf);
                assert(foundNonZeroCbf);
            }
            else
            {
                rpcTempCU->setQPSubParts(rpcTempCU->getRefQP(uiTargetPartIdx), 0, depth);     // set QP to default QP
            }
        }

        m_rdSbacCoders[uhNextDepth][CI_NEXT_BEST]->store(m_rdSbacCoders[depth][CI_TEMP_BEST]);
#if  CU_STAT_LOGFILE
        if (rpcBestCU->getTotalCost() < rpcTempCU->getTotalCost())
        {
            if (rpcBestCU->getPredictionMode(0) == MODE_INTER)
            {
                cntInter[depth]++;
                if (rpcBestCU->getPartitionSize(0) < 3)
                {
                    cuInterDistribution[depth][rpcBestCU->getPartitionSize(0)]++;
                }
                else
                {
                    cuInterDistribution[depth][3]++;
                }
            }
            else if (rpcBestCU->getPredictionMode(0) == MODE_INTRA)
            {
                cntIntra[depth]++;
                if (rpcBestCU->getLumaIntraDir()[0] > 1)
                {
                    cuIntraDistribution[depth][2]++;
                }
                else
                {
                    cuIntraDistribution[depth][rpcBestCU->getLumaIntraDir()[0]]++;
                }
            }
        }
        else
        {
            cntSplit[depth]++;
        }
#endif // if  LOGGING
        xCheckBestMode(rpcBestCU, rpcTempCU, depth);                                 // RD compare current larger prediction
        // with sub partitioned prediction.
    }
#if CU_STAT_LOGFILE
    if (depth == 3)
    {
        if (!rpcBestCU->isSkipped(0))
        {
            if (rpcBestCU->getPredictionMode(0) == MODE_INTER)
            {
                cntInter[depth]++;
                if (rpcBestCU->getPartitionSize(0) < 3)
                {
                    cuInterDistribution[depth][rpcBestCU->getPartitionSize(0)]++;
                }
                else
                {
                    cuInterDistribution[depth][3]++;
                }
            }
            else if (rpcBestCU->getPredictionMode(0) == MODE_INTRA)
            {
                cntIntra[depth]++;
                if (rpcBestCU->getPartitionSize(0) == SIZE_2Nx2N)
                {
                    if (rpcBestCU->getLumaIntraDir()[0] > 1)
                    {
                        cuIntraDistribution[depth][2]++;
                    }
                    else
                    {
                        cuIntraDistribution[depth][rpcBestCU->getLumaIntraDir()[0]]++;
                    }
                }
                else if (rpcBestCU->getPartitionSize(0) == SIZE_NxN)
                {
                    cntIntraNxN++;
                }
            }
        }
    }
#endif // if LOGGING
    rpcBestCU->copyToPic(depth);                                                   // Copy Best data to Picture for next partition prediction.
    xCopyYuv2Pic(rpcBestCU->getPic(), rpcBestCU->getAddr(), rpcBestCU->getZorderIdxInCU(), depth, depth, rpcBestCU, uiLPelX, uiTPelY);   // Copy Yuv data to picture Yuv

    if (bBoundary || (bSliceEnd && bInsidePicture))
    {
        return;
    }

    // Assert if Best prediction mode is NONE
    // Selected mode's RD-cost must be not MAX_DOUBLE.
    assert(rpcBestCU->getPartitionSize(0) != SIZE_NONE);
    assert(rpcBestCU->getPredictionMode(0) != MODE_NONE);
    assert(rpcBestCU->getTotalCost() != MAX_DOUBLE);
}

/** finish encoding a cu and handle end-of-slice conditions
 * \param cu
 * \param absPartIdx
 * \param depth
 * \returns Void
 */
Void TEncCu::finishCU(TComDataCU* cu, UInt absPartIdx, UInt depth)
{
    TComPic* pcPic = cu->getPic();
    TComSlice * pcSlice = cu->getPic()->getSlice();

    //Calculate end address
    UInt uiCUAddr = cu->getSCUAddr() + absPartIdx;

    UInt uiInternalAddress = (pcSlice->getSliceCurEndCUAddr() - 1) % pcPic->getNumPartInCU();
    UInt uiExternalAddress = (pcSlice->getSliceCurEndCUAddr() - 1) / pcPic->getNumPartInCU();
    UInt uiPosX = (uiExternalAddress % pcPic->getFrameWidthInCU()) * g_maxCUWidth + g_rasterToPelX[g_zscanToRaster[uiInternalAddress]];
    UInt uiPosY = (uiExternalAddress / pcPic->getFrameWidthInCU()) * g_maxCUHeight + g_rasterToPelY[g_zscanToRaster[uiInternalAddress]];
    UInt width = pcSlice->getSPS()->getPicWidthInLumaSamples();
    UInt height = pcSlice->getSPS()->getPicHeightInLumaSamples();

    while (uiPosX >= width || uiPosY >= height)
    {
        uiInternalAddress--;
        uiPosX = (uiExternalAddress % pcPic->getFrameWidthInCU()) * g_maxCUWidth + g_rasterToPelX[g_zscanToRaster[uiInternalAddress]];
        uiPosY = (uiExternalAddress / pcPic->getFrameWidthInCU()) * g_maxCUHeight + g_rasterToPelY[g_zscanToRaster[uiInternalAddress]];
    }

    uiInternalAddress++;
    if (uiInternalAddress == cu->getPic()->getNumPartInCU())
    {
        uiInternalAddress = 0;
        uiExternalAddress = (uiExternalAddress + 1);
    }
    UInt uiRealEndAddress = (uiExternalAddress * pcPic->getNumPartInCU() + uiInternalAddress);

    // Encode slice finish
    Bool bTerminateSlice = false;
    if (uiCUAddr + (cu->getPic()->getNumPartInCU() >> (depth << 1)) == uiRealEndAddress)
    {
        bTerminateSlice = true;
    }
    UInt uiGranularityWidth = g_maxCUWidth;
    uiPosX = cu->getCUPelX() + g_rasterToPelX[g_zscanToRaster[absPartIdx]];
    uiPosY = cu->getCUPelY() + g_rasterToPelY[g_zscanToRaster[absPartIdx]];
    Bool granularityBoundary = ((uiPosX + cu->getWidth(absPartIdx)) % uiGranularityWidth == 0 || (uiPosX + cu->getWidth(absPartIdx) == width))
        && ((uiPosY + cu->getHeight(absPartIdx)) % uiGranularityWidth == 0 || (uiPosY + cu->getHeight(absPartIdx) == height));

    if (granularityBoundary)
    {
        // The 1-terminating bit is added to all streams, so don't add it here when it's 1.
        if (!bTerminateSlice)
            m_entropyCoder->encodeTerminatingBit(bTerminateSlice ? 1 : 0);
    }

    Int numberOfWrittenBits = 0;
    if (m_pcBitCounter)
    {
        numberOfWrittenBits = m_entropyCoder->getNumberOfWrittenBits();
    }

    // Calculate slice end IF this CU puts us over slice bit size.
    UInt iGranularitySize = cu->getPic()->getNumPartInCU();
    Int iGranularityEnd = ((cu->getSCUAddr() + absPartIdx) / iGranularitySize) * iGranularitySize;
    if (iGranularityEnd <= 0)
    {
        iGranularityEnd += max(iGranularitySize, (cu->getPic()->getNumPartInCU() >> (depth << 1)));
    }
    if (granularityBoundary)
    {
        pcSlice->setSliceBits((UInt)(pcSlice->getSliceBits() + numberOfWrittenBits));
        pcSlice->setSliceSegmentBits(pcSlice->getSliceSegmentBits() + numberOfWrittenBits);
        if (m_pcBitCounter)
        {
            m_entropyCoder->resetBits();
        }
    }
}

/** Compute QP for each CU
 * \param cu Target CU
 * \param depth CU depth
 * \returns quantization parameter
 */
Int TEncCu::xComputeQP(TComDataCU* cu, UInt depth)
{
    Int iBaseQp = cu->getSlice()->getSliceQp();
    Int iQpOffset = 0;

    if (m_pcEncCfg->getUseAdaptiveQP())
    {
        TEncPic* pcEPic = dynamic_cast<TEncPic*>(cu->getPic());
        UInt uiAQDepth = min(depth, pcEPic->getMaxAQDepth() - 1);
        TEncPicQPAdaptationLayer* pcAQLayer = pcEPic->getAQLayer(uiAQDepth);
        UInt uiAQUPosX = cu->getCUPelX() / pcAQLayer->getAQPartWidth();
        UInt uiAQUPosY = cu->getCUPelY() / pcAQLayer->getAQPartHeight();
        UInt uiAQUStride = pcAQLayer->getAQPartStride();
        TEncQPAdaptationUnit* acAQU = pcAQLayer->getQPAdaptationUnit();

        Double dMaxQScale = pow(2.0, m_pcEncCfg->getQPAdaptationRange() / 6.0);
        Double dAvgAct = pcAQLayer->getAvgActivity();
        Double dCUAct = acAQU[uiAQUPosY * uiAQUStride + uiAQUPosX].getActivity();
        Double dNormAct = (dMaxQScale * dCUAct + dAvgAct) / (dCUAct + dMaxQScale * dAvgAct);
        Double dQpOffset = log(dNormAct) / log(2.0) * 6.0;
        iQpOffset = Int(floor(dQpOffset + 0.49999));
    }
    return Clip3(-cu->getSlice()->getSPS()->getQpBDOffsetY(), MAX_QP, iBaseQp + iQpOffset);
}

/** encode a CU block recursively
 * \param cu
 * \param absPartIdx
 * \param depth
 * \returns Void
 */
Void TEncCu::xEncodeCU(TComDataCU* cu, UInt absPartIdx, UInt depth)
{
    TComPic* pcPic = cu->getPic();

    Bool bBoundary = false;
    UInt uiLPelX   = cu->getCUPelX() + g_rasterToPelX[g_zscanToRaster[absPartIdx]];
    UInt uiRPelX   = uiLPelX + (g_maxCUWidth >> depth)  - 1;
    UInt uiTPelY   = cu->getCUPelY() + g_rasterToPelY[g_zscanToRaster[absPartIdx]];
    UInt uiBPelY   = uiTPelY + (g_maxCUHeight >> depth) - 1;

    TComSlice * pcSlice = cu->getPic()->getSlice();

    // If slice start is within this cu...

    // We need to split, so don't try these modes.
    if ((uiRPelX < pcSlice->getSPS()->getPicWidthInLumaSamples()) && (uiBPelY < pcSlice->getSPS()->getPicHeightInLumaSamples()))
    {
        m_entropyCoder->encodeSplitFlag(cu, absPartIdx, depth);
    }
    else
    {
        bBoundary = true;
    }

    if (((depth < cu->getDepth(absPartIdx)) && (depth < (g_maxCUDepth - g_addCUDepth))) || bBoundary)
    {
        UInt uiQNumParts = (pcPic->getNumPartInCU() >> (depth << 1)) >> 2;
        if ((g_maxCUWidth >> depth) == cu->getSlice()->getPPS()->getMinCuDQPSize() && cu->getSlice()->getPPS()->getUseDQP())
        {
            setdQPFlag(true);
        }
        for (UInt uiPartUnitIdx = 0; uiPartUnitIdx < 4; uiPartUnitIdx++, absPartIdx += uiQNumParts)
        {
            uiLPelX   = cu->getCUPelX() + g_rasterToPelX[g_zscanToRaster[absPartIdx]];
            uiTPelY   = cu->getCUPelY() + g_rasterToPelY[g_zscanToRaster[absPartIdx]];
            Bool bInSlice = cu->getSCUAddr() + absPartIdx < pcSlice->getSliceCurEndCUAddr();
            if (bInSlice && (uiLPelX < pcSlice->getSPS()->getPicWidthInLumaSamples()) && (uiTPelY < pcSlice->getSPS()->getPicHeightInLumaSamples()))
            {
                xEncodeCU(cu, absPartIdx, depth + 1);
            }
        }

        return;
    }

    if ((g_maxCUWidth >> depth) >= cu->getSlice()->getPPS()->getMinCuDQPSize() && cu->getSlice()->getPPS()->getUseDQP())
    {
        setdQPFlag(true);
    }
    if (cu->getSlice()->getPPS()->getTransquantBypassEnableFlag())
    {
        m_entropyCoder->encodeCUTransquantBypassFlag(cu, absPartIdx);
    }
    if (!cu->getSlice()->isIntra())
    {
        m_entropyCoder->encodeSkipFlag(cu, absPartIdx);
    }

    if (cu->isSkipped(absPartIdx))
    {
        m_entropyCoder->encodeMergeIndex(cu, absPartIdx);
        finishCU(cu, absPartIdx, depth);
        return;
    }
    m_entropyCoder->encodePredMode(cu, absPartIdx);

    m_entropyCoder->encodePartSize(cu, absPartIdx, depth);

    if (cu->isIntra(absPartIdx) && cu->getPartitionSize(absPartIdx) == SIZE_2Nx2N)
    {
        m_entropyCoder->encodeIPCMInfo(cu, absPartIdx);

        if (cu->getIPCMFlag(absPartIdx))
        {
            // Encode slice finish
            finishCU(cu, absPartIdx, depth);
            return;
        }
    }

    // prediction Info ( Intra : direction mode, Inter : Mv, reference idx )
    m_entropyCoder->encodePredInfo(cu, absPartIdx);

    // Encode Coefficients
    Bool bCodeDQP = getdQPFlag();
    m_entropyCoder->encodeCoeff(cu, absPartIdx, depth, cu->getWidth(absPartIdx), cu->getHeight(absPartIdx), bCodeDQP);
    setdQPFlag(bCodeDQP);

    // --- write terminating bit ---
    finishCU(cu, absPartIdx, depth);
}

/** check RD costs for a CU block encoded with merge
 * \param rpcBestCU
 * \param rpcTempCU
 * \returns Void
 */
Void TEncCu::xCheckRDCostMerge2Nx2N(TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, Bool *earlyDetectionSkipMode, TComYuv*& rpcYuvPredBest, TComYuv*& rpcYuvReconBest)
{
    assert(rpcTempCU->getSlice()->getSliceType() != I_SLICE);
    TComMvField  cMvFieldNeighbours[MRG_MAX_NUM_CANDS << 1]; // double length for mv of both lists
    UChar uhInterDirNeighbours[MRG_MAX_NUM_CANDS];
    Int numValidMergeCand = 0;

    for (UInt ui = 0; ui < rpcTempCU->getSlice()->getMaxNumMergeCand(); ++ui)
    {
        uhInterDirNeighbours[ui] = 0;
    }

    UChar uhDepth = rpcTempCU->getDepth(0);
    rpcTempCU->setPartSizeSubParts(SIZE_2Nx2N, 0, uhDepth); // interprets depth relative to LCU level
    rpcTempCU->setCUTransquantBypassSubParts(m_pcEncCfg->getCUTransquantBypassFlagValue(), 0, uhDepth);
    rpcTempCU->getInterMergeCandidates(0, 0, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand);

    Int mergeCandBuffer[MRG_MAX_NUM_CANDS];
    for (UInt ui = 0; ui < numValidMergeCand; ++ui)
    {
        mergeCandBuffer[ui] = 0;
    }

    Bool bestIsSkip = false;

    UInt iteration;
    if (rpcTempCU->isLosslessCoded(0))
    {
        iteration = 1;
    }
    else
    {
        iteration = 2;
    }

    for (UInt uiNoResidual = 0; uiNoResidual < iteration; ++uiNoResidual)
    {
        for (UInt uiMergeCand = 0; uiMergeCand < numValidMergeCand; ++uiMergeCand)
        {
            if (!(uiNoResidual == 1 && mergeCandBuffer[uiMergeCand] == 1))
            {
                if (!(bestIsSkip && uiNoResidual == 0))
                {
                    // set MC parameters
                    rpcTempCU->setPredModeSubParts(MODE_INTER, 0, uhDepth); // interprets depth relative to LCU level
                    rpcTempCU->setCUTransquantBypassSubParts(m_pcEncCfg->getCUTransquantBypassFlagValue(),     0, uhDepth);
                    rpcTempCU->setPartSizeSubParts(SIZE_2Nx2N, 0, uhDepth); // interprets depth relative to LCU level
                    rpcTempCU->setMergeFlagSubParts(true, 0, 0, uhDepth); // interprets depth relative to LCU level
                    rpcTempCU->setMergeIndexSubParts(uiMergeCand, 0, 0, uhDepth); // interprets depth relative to LCU level
                    rpcTempCU->setInterDirSubParts(uhInterDirNeighbours[uiMergeCand], 0, 0, uhDepth); // interprets depth relative to LCU level
                    rpcTempCU->getCUMvField(REF_PIC_LIST_0)->setAllMvField(cMvFieldNeighbours[0 + 2 * uiMergeCand], SIZE_2Nx2N, 0, 0); // interprets depth relative to rpcTempCU level
                    rpcTempCU->getCUMvField(REF_PIC_LIST_1)->setAllMvField(cMvFieldNeighbours[1 + 2 * uiMergeCand], SIZE_2Nx2N, 0, 0); // interprets depth relative to rpcTempCU level

                    // do MC
                    m_pcPredSearch->motionCompensation(rpcTempCU, m_ppcPredYuvTemp[uhDepth]);
                    // estimate residual and encode everything
                    m_pcPredSearch->encodeResAndCalcRdInterCU(rpcTempCU,
                                                              m_ppcOrigYuv[uhDepth],
                                                              m_ppcPredYuvTemp[uhDepth],
                                                              m_ppcResiYuvTemp[uhDepth],
                                                              m_ppcResiYuvBest[uhDepth],
                                                              m_ppcRecoYuvTemp[uhDepth],
                                                              (uiNoResidual ? true : false));
                    /*Todo: Fix the satd cost estimates. Why is merge being chosen in high motion areas: estimated distortion is too low?*/
                    if (uiNoResidual == 0)
                    {
                        if (rpcTempCU->getQtRootCbf(0) == 0)
                        {
                            mergeCandBuffer[uiMergeCand] = 1;
                        }
                    }

                    rpcTempCU->setSkipFlagSubParts(rpcTempCU->getQtRootCbf(0) == 0, 0, uhDepth);
                    Int orgQP = rpcTempCU->getQP(0);
                    xCheckDQP(rpcTempCU);
                    if (rpcTempCU->getTotalCost() < rpcBestCU->getTotalCost())
                    {
                        TComDataCU* tmp = rpcTempCU;
                        rpcTempCU = rpcBestCU;
                        rpcBestCU = tmp;
                        // Change Prediction data
                        TComYuv* pcYuv = NULL;
                        pcYuv = rpcYuvPredBest;
                        rpcYuvPredBest  = m_ppcPredYuvTemp[uhDepth];
                        m_ppcPredYuvTemp[uhDepth] = pcYuv;
                        
                        pcYuv = rpcYuvReconBest;
                        rpcYuvReconBest = m_ppcRecoYuvTemp[uhDepth];
                        m_ppcRecoYuvTemp[uhDepth] = pcYuv;
                    }
                    rpcTempCU->initEstData(uhDepth, orgQP);
                    if (!bestIsSkip)
                    {
                        bestIsSkip = rpcBestCU->getQtRootCbf(0) == 0;
                    }
                }
            }
        }

        if (uiNoResidual == 0 && m_pcEncCfg->getUseEarlySkipDetection())
        {
            if (rpcBestCU->getQtRootCbf(0) == 0)
            {
                if (rpcBestCU->getMergeFlag(0))
                {
                    *earlyDetectionSkipMode = true;
                }
                else
                {
                    Int absoulte_MV = 0;
                    for (UInt uiRefListIdx = 0; uiRefListIdx < 2; uiRefListIdx++)
                    {
                        if (rpcBestCU->getSlice()->getNumRefIdx(RefPicList(uiRefListIdx)) > 0)
                        {
                            TComCUMvField* pcCUMvField = rpcBestCU->getCUMvField(RefPicList(uiRefListIdx));
                            Int iHor = abs(pcCUMvField->getMvd(0).x);
                            Int iVer = abs(pcCUMvField->getMvd(0).y);
                            absoulte_MV += iHor + iVer;
                        }
                    }

                    if (absoulte_MV == 0)
                    {
                        *earlyDetectionSkipMode = true;
                    }
                }
            }
        }
    }
}

Void TEncCu::xCheckRDCostInter(TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, PartSize ePartSize, Bool bUseMRG)
{
    UChar uhDepth = rpcTempCU->getDepth(0);

    rpcTempCU->setDepthSubParts(uhDepth, 0);

    rpcTempCU->setSkipFlagSubParts(false, 0, uhDepth);

    rpcTempCU->setPartSizeSubParts(ePartSize,  0, uhDepth);
    rpcTempCU->setPredModeSubParts(MODE_INTER, 0, uhDepth);
    rpcTempCU->setCUTransquantBypassSubParts(m_pcEncCfg->getCUTransquantBypassFlagValue(),      0, uhDepth);

    rpcTempCU->setMergeAMP(true);
    m_ppcRecoYuvTemp[uhDepth]->clear();
    m_ppcResiYuvTemp[uhDepth]->clear();
    m_pcPredSearch->predInterSearch(rpcTempCU, m_ppcOrigYuv[uhDepth], m_ppcPredYuvTemp[uhDepth], bUseMRG);

    if (m_pcEncCfg->getUseRateCtrl() && m_pcEncCfg->getLCULevelRC() && ePartSize == SIZE_2Nx2N && uhDepth <= m_addSADDepth)
    {
        /* TODO: this needs to be tested with RC enabled, currently RC enabled x265 is not working */
        UInt partEnum = PartitionFromSizes(rpcTempCU->getWidth(0), rpcTempCU->getHeight(0));
        UInt SAD = primitives.sad[partEnum]((pixel*)m_ppcOrigYuv[uhDepth]->getLumaAddr(), m_ppcOrigYuv[uhDepth]->getStride(),
                                            (pixel*)m_ppcPredYuvTemp[uhDepth]->getLumaAddr(), m_ppcPredYuvTemp[uhDepth]->getStride());
        m_temporalSAD = (Int)SAD;
        x265_emms();
    }

    m_pcPredSearch->encodeResAndCalcRdInterCU(rpcTempCU, m_ppcOrigYuv[uhDepth], m_ppcPredYuvTemp[uhDepth], m_ppcResiYuvTemp[uhDepth], m_ppcResiYuvBest[uhDepth], m_ppcRecoYuvTemp[uhDepth], false);

    xCheckDQP(rpcTempCU);

    xCheckBestMode(rpcBestCU, rpcTempCU, uhDepth);
}

Void TEncCu::xCheckRDCostIntra(TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, PartSize eSize)
{
    //PPAScopeEvent(TEncCU_xCheckRDCostIntra + depth);
    UInt depth = rpcTempCU->getDepth(0);
    UInt uiPreCalcDistC = 0;

    rpcTempCU->setSkipFlagSubParts(false, 0, depth);
    rpcTempCU->setPartSizeSubParts(eSize, 0, depth);
    rpcTempCU->setPredModeSubParts(MODE_INTRA, 0, depth);
    rpcTempCU->setCUTransquantBypassSubParts(m_pcEncCfg->getCUTransquantBypassFlagValue(), 0, depth);

    m_pcPredSearch->estIntraPredQT(rpcTempCU, m_ppcOrigYuv[depth], m_ppcPredYuvTemp[depth], m_ppcResiYuvTemp[depth], m_ppcRecoYuvTemp[depth], uiPreCalcDistC, true);

    m_ppcRecoYuvTemp[depth]->copyToPicLuma(rpcTempCU->getPic()->getPicYuvRec(), rpcTempCU->getAddr(), rpcTempCU->getZorderIdxInCU());

    m_pcPredSearch->estIntraPredChromaQT(rpcTempCU, m_ppcOrigYuv[depth], m_ppcPredYuvTemp[depth], m_ppcResiYuvTemp[depth], m_ppcRecoYuvTemp[depth], uiPreCalcDistC);

    m_entropyCoder->resetBits();
    if (rpcTempCU->getSlice()->getPPS()->getTransquantBypassEnableFlag())
    {
        m_entropyCoder->encodeCUTransquantBypassFlag(rpcTempCU, 0, true);
    }
    m_entropyCoder->encodeSkipFlag(rpcTempCU, 0,          true);
    m_entropyCoder->encodePredMode(rpcTempCU, 0,          true);
    m_entropyCoder->encodePartSize(rpcTempCU, 0, depth, true);
    m_entropyCoder->encodePredInfo(rpcTempCU, 0,          true);
    m_entropyCoder->encodeIPCMInfo(rpcTempCU, 0, true);

    // Encode Coefficients
    Bool bCodeDQP = getdQPFlag();
    m_entropyCoder->encodeCoeff(rpcTempCU, 0, depth, rpcTempCU->getWidth(0), rpcTempCU->getHeight(0), bCodeDQP);
    setdQPFlag(bCodeDQP);

    m_rdGoOnSbacCoder->store(m_rdSbacCoders[depth][CI_TEMP_BEST]);

    rpcTempCU->getTotalBits() = m_entropyCoder->getNumberOfWrittenBits();
    rpcTempCU->getTotalBins() = ((TEncBinCABAC*)((TEncSbac*)m_entropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
    rpcTempCU->getTotalCost() = m_pcRdCost->calcRdCost(rpcTempCU->getTotalDistortion(), rpcTempCU->getTotalBits());

    xCheckDQP(rpcTempCU);
    xCheckBestMode(rpcBestCU, rpcTempCU, depth);
}

Void TEncCu::xCheckRDCostIntrainInter(TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, PartSize eSize)
{
    UInt depth = rpcTempCU->getDepth(0);

    PPAScopeEvent(TEncCU_xCheckRDCostIntra + depth);

    rpcTempCU->setSkipFlagSubParts(false, 0, depth);

    rpcTempCU->setPartSizeSubParts(eSize, 0, depth);
    rpcTempCU->setPredModeSubParts(MODE_INTRA, 0, depth);
    rpcTempCU->setCUTransquantBypassSubParts(m_pcEncCfg->getCUTransquantBypassFlagValue(), 0, depth);

    Bool bSeparateLumaChroma = true; // choose estimation mode
    UInt uiPreCalcDistC      = 0;
    if (!bSeparateLumaChroma)
    {
        m_pcPredSearch->preestChromaPredMode(rpcTempCU, m_ppcOrigYuv[depth], m_ppcPredYuvTemp[depth]);
    }
    m_pcPredSearch->estIntraPredQT(rpcTempCU, m_ppcOrigYuv[depth], m_ppcPredYuvTemp[depth], m_ppcResiYuvTemp[depth], m_ppcRecoYuvTemp[depth], uiPreCalcDistC, bSeparateLumaChroma);

    m_ppcRecoYuvTemp[depth]->copyToPicLuma(rpcTempCU->getPic()->getPicYuvRec(), rpcTempCU->getAddr(), rpcTempCU->getZorderIdxInCU());

    m_pcPredSearch->estIntraPredChromaQT(rpcTempCU, m_ppcOrigYuv[depth], m_ppcPredYuvTemp[depth], m_ppcResiYuvTemp[depth], m_ppcRecoYuvTemp[depth], uiPreCalcDistC);

    m_entropyCoder->resetBits();
    if (rpcTempCU->getSlice()->getPPS()->getTransquantBypassEnableFlag())
    {
        m_entropyCoder->encodeCUTransquantBypassFlag(rpcTempCU, 0, true);
    }
    m_entropyCoder->encodeSkipFlag(rpcTempCU, 0,          true);
    m_entropyCoder->encodePredMode(rpcTempCU, 0,          true);
    m_entropyCoder->encodePartSize(rpcTempCU, 0, depth, true);
    m_entropyCoder->encodePredInfo(rpcTempCU, 0,          true);
    m_entropyCoder->encodeIPCMInfo(rpcTempCU, 0, true);

    // Encode Coefficients
    Bool bCodeDQP = getdQPFlag();
    m_entropyCoder->encodeCoeff(rpcTempCU, 0, depth, rpcTempCU->getWidth(0), rpcTempCU->getHeight(0), bCodeDQP);
    setdQPFlag(bCodeDQP);

    m_rdGoOnSbacCoder->store(m_rdSbacCoders[depth][CI_TEMP_BEST]);

    rpcTempCU->getTotalBits() = m_entropyCoder->getNumberOfWrittenBits();
    rpcTempCU->getTotalBins() = ((TEncBinCABAC*)((TEncSbac*)m_entropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
    if (!m_pcEncCfg->getUseRDO())
    {
        UInt partEnum = PartitionFromSizes(rpcTempCU->getWidth(0), rpcTempCU->getHeight(0));
        UInt SATD = primitives.satd[partEnum]((pixel*)m_ppcOrigYuv[depth]->getLumaAddr(), m_ppcOrigYuv[depth]->getStride(),
                                              (pixel*)m_ppcPredYuvTemp[depth]->getLumaAddr(), m_ppcPredYuvTemp[depth]->getStride());
        x265_emms();
        rpcTempCU->getTotalDistortion() = SATD;
    }
    rpcTempCU->getTotalCost() = m_pcRdCost->calcRdCost(rpcTempCU->getTotalDistortion(), rpcTempCU->getTotalBits());

    xCheckDQP(rpcTempCU);
    xCheckBestMode(rpcBestCU, rpcTempCU, depth);
}

/** Check R-D costs for a CU with PCM mode.
 * \param rpcBestCU pointer to best mode CU data structure
 * \param rpcTempCU pointer to testing mode CU data structure
 * \returns Void
 *
 * \note Current PCM implementation encodes sample values in a lossless way. The distortion of PCM mode CUs are zero. PCM mode is selected if the best mode yields bits greater than that of PCM mode.
 */
Void TEncCu::xCheckIntraPCM(TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU)
{
    UInt depth = rpcTempCU->getDepth(0);

    //PPAScopeEvent(TEncCU_xCheckIntraPCM);

    rpcTempCU->setSkipFlagSubParts(false, 0, depth);

    rpcTempCU->setIPCMFlag(0, true);
    rpcTempCU->setIPCMFlagSubParts(true, 0, rpcTempCU->getDepth(0));
    rpcTempCU->setPartSizeSubParts(SIZE_2Nx2N, 0, depth);
    rpcTempCU->setPredModeSubParts(MODE_INTRA, 0, depth);
    rpcTempCU->setTrIdxSubParts(0, 0, depth);
    rpcTempCU->setCUTransquantBypassSubParts(m_pcEncCfg->getCUTransquantBypassFlagValue(), 0, depth);

    m_pcPredSearch->IPCMSearch(rpcTempCU, m_ppcOrigYuv[depth], m_ppcPredYuvTemp[depth], m_ppcResiYuvTemp[depth], m_ppcRecoYuvTemp[depth]);

    m_rdGoOnSbacCoder->load(m_rdSbacCoders[depth][CI_CURR_BEST]);

    m_entropyCoder->resetBits();
    if (rpcTempCU->getSlice()->getPPS()->getTransquantBypassEnableFlag())
    {
        m_entropyCoder->encodeCUTransquantBypassFlag(rpcTempCU, 0,          true);
    }
    m_entropyCoder->encodeSkipFlag(rpcTempCU, 0,          true);
    m_entropyCoder->encodePredMode(rpcTempCU, 0,          true);
    m_entropyCoder->encodePartSize(rpcTempCU, 0, depth, true);
    m_entropyCoder->encodeIPCMInfo(rpcTempCU, 0, true);

    m_rdGoOnSbacCoder->store(m_rdSbacCoders[depth][CI_TEMP_BEST]);

    rpcTempCU->getTotalBits() = m_entropyCoder->getNumberOfWrittenBits();
    rpcTempCU->getTotalBins() = ((TEncBinCABAC*)((TEncSbac*)m_entropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
    rpcTempCU->getTotalCost() = m_pcRdCost->calcRdCost(rpcTempCU->getTotalDistortion(), rpcTempCU->getTotalBits());

    xCheckDQP(rpcTempCU);
    xCheckBestMode(rpcBestCU, rpcTempCU, depth);
}

/** check whether current try is the best with identifying the depth of current try
 * \param rpcBestCU
 * \param rpcTempCU
 * \returns Void
 */
Void TEncCu::xCheckBestMode(TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, UInt depth)
{
    if (rpcTempCU->getTotalCost() < rpcBestCU->getTotalCost())
    {
        TComYuv* pcYuv;
        // Change Information data
        TComDataCU* cu = rpcBestCU;
        rpcBestCU = rpcTempCU;
        rpcTempCU = cu;

        // Change Prediction data
        pcYuv = m_ppcPredYuvBest[depth];
        m_ppcPredYuvBest[depth] = m_ppcPredYuvTemp[depth];
        m_ppcPredYuvTemp[depth] = pcYuv;

        // Change Reconstruction data
        pcYuv = m_ppcRecoYuvBest[depth];
        m_ppcRecoYuvBest[depth] = m_ppcRecoYuvTemp[depth];
        m_ppcRecoYuvTemp[depth] = pcYuv;

        pcYuv = NULL;
        cu  = NULL;

        m_rdSbacCoders[depth][CI_TEMP_BEST]->store(m_rdSbacCoders[depth][CI_NEXT_BEST]);
    }
}

Void TEncCu::xCheckDQP(TComDataCU* cu)
{
    UInt depth = cu->getDepth(0);

    if (cu->getSlice()->getPPS()->getUseDQP() && (g_maxCUWidth >> depth) >= cu->getSlice()->getPPS()->getMinCuDQPSize())
    {
        cu->setQPSubParts(cu->getRefQP(0), 0, depth); // set QP to default QP
    }
}

Void TEncCu::xCopyAMVPInfo(AMVPInfo* src, AMVPInfo* dst)
{
    dst->iN = src->iN;
    for (Int i = 0; i < src->iN; i++)
    {
        dst->m_acMvCand[i] = src->m_acMvCand[i];
    }
}

Void TEncCu::xCopyYuv2Pic(TComPic* rpcPic, UInt uiCUAddr, UInt absPartIdx, UInt depth, UInt uiSrcDepth, TComDataCU* cu, UInt uiLPelX, UInt uiTPelY)
{
    UInt uiRPelX   = uiLPelX + (g_maxCUWidth >> depth)  - 1;
    UInt uiBPelY   = uiTPelY + (g_maxCUHeight >> depth) - 1;
    TComSlice * pcSlice = cu->getPic()->getSlice();
    Bool bSliceEnd   = pcSlice->getSliceCurEndCUAddr() > (cu->getAddr()) * cu->getPic()->getNumPartInCU() + absPartIdx &&
        pcSlice->getSliceCurEndCUAddr() < (cu->getAddr()) * cu->getPic()->getNumPartInCU() + absPartIdx + (cu->getPic()->getNumPartInCU() >> (depth << 1));

    if (!bSliceEnd && (uiRPelX < pcSlice->getSPS()->getPicWidthInLumaSamples()) && (uiBPelY < pcSlice->getSPS()->getPicHeightInLumaSamples()))
    {
        UInt uiAbsPartIdxInRaster = g_zscanToRaster[absPartIdx];
        UInt uiSrcBlkWidth = rpcPic->getNumPartInWidth() >> (uiSrcDepth);
        UInt uiBlkWidth    = rpcPic->getNumPartInWidth() >> (depth);
        UInt uiPartIdxX = ((uiAbsPartIdxInRaster % rpcPic->getNumPartInWidth()) % uiSrcBlkWidth) / uiBlkWidth;
        UInt uiPartIdxY = ((uiAbsPartIdxInRaster / rpcPic->getNumPartInWidth()) % uiSrcBlkWidth) / uiBlkWidth;
        UInt partIdx = uiPartIdxY * (uiSrcBlkWidth / uiBlkWidth) + uiPartIdxX;
        m_ppcRecoYuvBest[uiSrcDepth]->copyToPicYuv(rpcPic->getPicYuvRec(), uiCUAddr, absPartIdx, depth - uiSrcDepth, partIdx);
    }
    else
    {
        UInt uiQNumParts = (cu->getPic()->getNumPartInCU() >> (depth << 1)) >> 2;

        for (UInt uiPartUnitIdx = 0; uiPartUnitIdx < 4; uiPartUnitIdx++, absPartIdx += uiQNumParts)
        {
            UInt uiSubCULPelX   = uiLPelX + (g_maxCUWidth >> (depth + 1)) * (uiPartUnitIdx &  1);
            UInt uiSubCUTPelY   = uiTPelY + (g_maxCUHeight >> (depth + 1)) * (uiPartUnitIdx >> 1);

            Bool bInSlice = cu->getAddr() * cu->getPic()->getNumPartInCU() + absPartIdx < pcSlice->getSliceCurEndCUAddr();
            if (bInSlice && (uiSubCULPelX < pcSlice->getSPS()->getPicWidthInLumaSamples()) && (uiSubCUTPelY < pcSlice->getSPS()->getPicHeightInLumaSamples()))
            {
                xCopyYuv2Pic(rpcPic, uiCUAddr, absPartIdx, depth + 1, uiSrcDepth, cu, uiSubCULPelX, uiSubCUTPelY); // Copy Yuv data to picture Yuv
            }
        }
    }
}

Void TEncCu::xCopyYuv2Tmp(UInt uiPartUnitIdx, UInt uiNextDepth)
{
    UInt uiCurrDepth = uiNextDepth - 1;

    m_ppcRecoYuvBest[uiNextDepth]->copyToPartYuv(m_ppcRecoYuvTemp[uiCurrDepth], uiPartUnitIdx);
}

Void TEncCu::xCopyYuv2Best(UInt uiPartUnitIdx, UInt uiNextDepth)
{
    UInt uiCurrDepth = uiNextDepth - 1;

    m_ppcRecoYuvTemp[uiNextDepth]->copyToPartYuv(m_ppcRecoYuvBest[uiCurrDepth], uiPartUnitIdx);
}

/** Function for filling the PCM buffer of a CU using its original sample array
 * \param cu pointer to current CU
 * \param fencYuv pointer to original sample array
 * \returns Void
 */
Void TEncCu::xFillPCMBuffer(TComDataCU*& pCU, TComYuv* pOrgYuv)
{
    UInt   width        = pCU->getWidth(0);
    UInt   height       = pCU->getHeight(0);

    Pel*   pSrcY = pOrgYuv->getLumaAddr(0, width);
    Pel*   pDstY = pCU->getPCMSampleY();
    UInt   srcStride = pOrgYuv->getStride();

    for (Int y = 0; y < height; y++)
    {
        for (Int x = 0; x < width; x++)
        {
            pDstY[x] = pSrcY[x];
        }

        pDstY += width;
        pSrcY += srcStride;
    }

    Pel* pSrcCb       = pOrgYuv->getCbAddr();
    Pel* pSrcCr       = pOrgYuv->getCrAddr();

    Pel* pDstCb       = pCU->getPCMSampleCb();
    Pel* pDstCr       = pCU->getPCMSampleCr();

    UInt srcStrideC = pOrgYuv->getCStride();
    UInt heightC   = height >> 1;
    UInt widthC    = width  >> 1;

    for (Int y = 0; y < heightC; y++)
    {
        for (Int x = 0; x < widthC; x++)
        {
            pDstCb[x] = pSrcCb[x];
            pDstCr[x] = pSrcCr[x];
        }

        pDstCb += widthC;
        pDstCr += widthC;
        pSrcCb += srcStrideC;
        pSrcCr += srcStrideC;
    }
}

/** Collect ARL statistics from one block
  */
Int TEncCu::xTuCollectARLStats(TCoeff* rpcCoeff, Int* rpcArlCoeff, Int NumCoeffInCU, Double* cSum, UInt* numSamples)
{
    for (Int n = 0; n < NumCoeffInCU; n++)
    {
        Int u = abs(rpcCoeff[n]);
        Int absc = rpcArlCoeff[n];

        if (u != 0)
        {
            if (u < LEVEL_RANGE)
            {
                cSum[u] += (Double)absc;
                numSamples[u]++;
            }
            else
            {
                cSum[LEVEL_RANGE] += (Double)absc - (Double)(u << ARL_C_PRECISION);
                numSamples[LEVEL_RANGE]++;
            }
        }
    }

    return 0;
}

/** Collect ARL statistics from one LCU
 * \param cu
 */
Void TEncCu::xLcuCollectARLStats(TComDataCU* rpcCU)
{
    Double cSum[LEVEL_RANGE + 1];     //: the sum of DCT coefficients corresponding to datatype and quantization output
    UInt numSamples[LEVEL_RANGE + 1]; //: the number of coefficients corresponding to datatype and quantization output

    TCoeff* pCoeffY = rpcCU->getCoeffY();
    Int* pArlCoeffY = rpcCU->getArlCoeffY();

    UInt uiMinCUWidth = g_maxCUWidth >> g_maxCUDepth;
    UInt uiMinNumCoeffInCU = 1 << uiMinCUWidth;

    memset(cSum, 0, sizeof(Double) * (LEVEL_RANGE + 1));
    memset(numSamples, 0, sizeof(UInt) * (LEVEL_RANGE + 1));

    // Collect stats to cSum[][] and numSamples[][]
    for (Int i = 0; i < rpcCU->getTotalNumPart(); i++)
    {
        UInt uiTrIdx = rpcCU->getTransformIdx(i);

        if (rpcCU->getPredictionMode(i) == MODE_INTER)
        {
            if (rpcCU->getCbf(i, TEXT_LUMA, uiTrIdx))
            {
                xTuCollectARLStats(pCoeffY, pArlCoeffY, uiMinNumCoeffInCU, cSum, numSamples);
            } //Note that only InterY is processed. QP rounding is based on InterY data only.
        }

        pCoeffY  += uiMinNumCoeffInCU;
        pArlCoeffY  += uiMinNumCoeffInCU;
    }

    for (Int u = 1; u < LEVEL_RANGE; u++)
    {
        m_pcTrQuant->getSliceSumC()[u] += cSum[u];
        m_pcTrQuant->getSliceNSamples()[u] += numSamples[u];
    }

    m_pcTrQuant->getSliceSumC()[LEVEL_RANGE] += cSum[LEVEL_RANGE];
    m_pcTrQuant->getSliceNSamples()[LEVEL_RANGE] += numSamples[LEVEL_RANGE];
}

//! \}
