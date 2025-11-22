// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>

#include <arith_uint256.h>
#include <chain.h>
#include <primitives/block.h>
#include <uint256.h>

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    assert(pindexLast != nullptr);
    unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();

    // reset difficulty for new diff algorithm's average + Segwit/CSV activation
    if ((pindexLast->nHeight >= 28930) && (pindexLast->nHeight <= 28999))
        return nProofOfWorkLimit;

    // Special difficulty rule for testnet: If the new block's timestamp is more than 20 minutes
    // then allow mining of a min-difficulty block. This rule has priority over LWMA.
    if (params.fPowAllowMinDifficultyBlocks && pblock->GetBlockTime() > pindexLast->GetBlockTime() + 20 * 60)
        return nProofOfWorkLimit;

    // For testnet, use LWMA from beginning to recalculate every block like mainnet
    if (params.fPowAllowMinDifficultyBlocks && pindexLast->nHeight >= 0)
        return LwmaCalculateNextWorkRequired(pindexLast, params);

    // Palladium: Hard Fork at 350,000 for fast difficulty adjustment
    if (pindexLast->nHeight + 1 >= 350000) {
        int64_t nIntervalNew = 60;
        if ((pindexLast->nHeight + 1) % nIntervalNew != 0)
            return pindexLast->nBits;

        int nHeightFirst = pindexLast->nHeight - (nIntervalNew - 1);
        assert(nHeightFirst >= 0);
        const CBlockIndex* pindexFirst = pindexLast->GetAncestor(nHeightFirst);
        assert(pindexFirst);

        return CalculateNextWorkRequired(pindexLast, pindexFirst->GetBlockTime(), params);
    }

    if (pindexLast->nHeight >= 29000)
        return LwmaCalculateNextWorkRequired(pindexLast, params);

    // Only change once per difficulty adjustment interval
    if ((pindexLast->nHeight+1) % params.DifficultyAdjustmentInterval() != 0)
    {
        if (params.fPowAllowMinDifficultyBlocks)
        {
            // Return the last non-special-min-difficulty-rules-block
            const CBlockIndex* pindex = pindexLast;
            while (pindex->pprev && pindex->nHeight % params.DifficultyAdjustmentInterval() != 0 && pindex->nBits == nProofOfWorkLimit)
                pindex = pindex->pprev;
            return pindex->nBits;
        }
        return pindexLast->nBits;
    }

    // Go back by what we want to be 14 days worth of blocks
    int nHeightFirst = pindexLast->nHeight - (params.DifficultyAdjustmentInterval()-1);
    assert(nHeightFirst >= 0);
    const CBlockIndex* pindexFirst = pindexLast->GetAncestor(nHeightFirst);
    assert(pindexFirst);

    return CalculateNextWorkRequired(pindexLast, pindexFirst->GetBlockTime(), params);
}

unsigned int LwmaCalculateNextWorkRequired(const CBlockIndex* pindexLast, const Consensus::Params& params)
{
    const int64_t T = params.nPowTargetSpacingV2;
    const int64_t N = 240;
    const int64_t k = N * (N + 1) * T / 2; // For T=120, 240, 600 use approx N=100, 75, 50
    const int64_t height = pindexLast->nHeight;
    const arith_uint256 powLimit = UintToArith256(params.powLimit);
    
    if (height < N) { return powLimit.GetCompact(); }

    arith_uint256 sumTarget, nextTarget;
    int64_t thisTimestamp, previousTimestamp;
    int64_t t = 0, j = 0;

    // Uncomment next 2 lines to use LWMA-3 jump rule.
    //arith_uint256 previousTarget = 0;
    //int64_t sumLast3Solvetimes = 0; 

    const CBlockIndex* blockPreviousTimestamp = pindexLast->GetAncestor(height - N);
    previousTimestamp = blockPreviousTimestamp->GetBlockTime();

    // Loop through N most recent blocks. 
    for (int64_t i = height - N + 1; i <= height; i++) {
        const CBlockIndex* block = pindexLast->GetAncestor(i);
        thisTimestamp = (block->GetBlockTime() > previousTimestamp) ? 
                            block->GetBlockTime() : previousTimestamp + 1;

        int64_t solvetime = std::min(6 * T, thisTimestamp - previousTimestamp);
        previousTimestamp = thisTimestamp;

        j++;
        t += solvetime * j; // Weighted solvetime sum.
        arith_uint256 target;
        target.SetCompact(block->nBits);
        sumTarget += target / (k * N);
 
      // Uncomment next 2 lines to use LWMA-3.
      //  if (i > height - 3) { sumLast3Solvetimes  += solvetime; }  
      //  if (i == height) { previousTarget = target.SetCompact(block->nBits); }
    }
    nextTarget = t * sumTarget;

   // Uncomment the following to use LWMA-3.
   // This is a "memory-less" jump in difficulty approximately 2x normal
   // if (sumLast3Solvetimes < (8 * T) / 10) { nextTarget = (previousTarget*100)/(100+(N*26)/200); } 

    if (nextTarget > powLimit) { nextTarget = powLimit; }

    return nextTarget.GetCompact();
}

unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params)
{
    if (params.fPowNoRetargeting)
        return pindexLast->nBits;

    // Palladium: Hard Fork at 350,000 for fast difficulty adjustment
    int64_t nTargetTimespan = params.nPowTargetTimespan;
    if (pindexLast->nHeight + 1 >= 350000) {
        nTargetTimespan = 7200; // 60 blocks * 120 seconds
    }

    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
    if (nActualTimespan < nTargetTimespan/4)
        nActualTimespan = nTargetTimespan/4;
    if (nActualTimespan > nTargetTimespan*4)
        nActualTimespan = nTargetTimespan*4;

    // Retarget
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= nActualTimespan;
    bnNew /= nTargetTimespan;

    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;

    return bnNew.GetCompact();
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}
