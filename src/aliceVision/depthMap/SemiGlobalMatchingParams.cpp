// This file is part of the AliceVision project.
// Copyright (c) 2017 AliceVision contributors.
// This Source Code Form is subject to the terms of the Mozilla Public License,
// v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "SemiGlobalMatchingParams.hpp"
#include <aliceVision/mvsData/Pixel.hpp>
#include <aliceVision/mvsData/Point2d.hpp>
#include <aliceVision/mvsData/Point3d.hpp>
#include <aliceVision/mvsUtils/common.hpp>
#include <aliceVision/mvsUtils/fileIO.hpp>

#include <boost/filesystem.hpp>

namespace aliceVision {
namespace depthMap {

namespace bfs = boost::filesystem;

SemiGlobalMatchingParams::SemiGlobalMatchingParams(mvsUtils::MultiViewParams* _mp, mvsUtils::PreMatchCams* _pc, PlaneSweepingCuda* _cps)
{
    mp = _mp;
    pc = _pc;
    cps = _cps;
    prt = new RcTc(mp, cps);

    visualizeDepthMaps = mp->_ini.get<bool>("semiGlobalMatching.visualizeDepthMaps", false);
    visualizePartialDepthMaps =
        mp->_ini.get<bool>("semiGlobalMatching.visualizePartialDepthMaps", false);

    doSmooth = mp->_ini.get<bool>("semiGlobalMatching.smooth", true);


    doRefine = mp->_ini.get<bool>("semiGlobalMatching.doRefine", true);
    refineUseTcOrPixSize = mp->_ini.get<bool>("semiGlobalMatching.refineUseTcOrPixSize", true);

    ndepthsToRefine = mp->_ini.get<int>("semiGlobalMatching.ndepthsToRefine", 15);

    P1 = (unsigned char)mp->_ini.get<int>("semiGlobalMatching.P1", 10);
    P2 = (unsigned char)mp->_ini.get<int>("semiGlobalMatching.P2", 125);
    P3 = (unsigned char)mp->_ini.get<int>("semiGlobalMatching.P3", 0);

    maxDepthsToStore = mp->_ini.get<int>("semiGlobalMatching.maxDepthsToStore", 3000);
    maxDepthsToSweep = mp->_ini.get<int>("semiGlobalMatching.maxDepthsToSweep", 1500);
    rcTcDepthsHalfLimit = mp->_ini.get<int>("semiGlobalMatching.rcTcDepthsHalfLimit", 2048);

    rcDepthsCompStep = mp->_ini.get<int>("semiGlobalMatching.rcDepthsCompStep", 6);

    useSeedsToCompDepthsToSweep =
        mp->_ini.get<bool>("semiGlobalMatching.useSeedsToCompDepthsToSweep", true);
    seedsRangePercentile = (float)mp->_ini.get<double>("semiGlobalMatching.seedsRangePercentile", 0.001);
    seedsRangeInflate = (float)mp->_ini.get<double>("semiGlobalMatching.seedsRangeInflate", 0.2);

    saveDepthsToSweepToTxtForVis =
        mp->_ini.get<bool>("semiGlobalMatching.saveDepthsToSweepToTxtForVis", false);

    doSGMoptimizeVolume = mp->_ini.get<bool>("semiGlobalMatching.doSGMoptimizeVolume", true);
    doRefineRc = mp->_ini.get<bool>("semiGlobalMatching.doRefineRc", true);

    modalsMapDistLimit = mp->_ini.get<int>("semiGlobalMatching.modalsMapDistLimit", 2);
    minNumOfConsistentCams = mp->_ini.get<int>("semiGlobalMatching.minNumOfConsistentCams", 2);
    minObjectThickness = mp->_ini.get<int>("semiGlobalMatching.minObjectThickness", 8);
    maxTcRcPixSizeInVoxRatio =
        (float)mp->_ini.get<double>("semiGlobalMatching.maxTcRcPixSizeInVoxRatio", 2.0f);
    nSGGCIters = mp->_ini.get<int>("semiGlobalMatching.nSGGCIters", 0);

    SGMoutDirName = mp->_ini.get<std::string>("semiGlobalMatching.outDirName", "SGM");
    SGMtmpDirName = mp->_ini.get<std::string>("semiGlobalMatching.tmpDirName", "_tmp");

    useSilhouetteMaskCodedByColor = mp->_ini.get<bool>("global.useSilhouetteMaskCodedByColor", false);
    silhouetteMaskColor.r = mp->_ini.get<int>("global.silhouetteMaskColorR", 0);
    silhouetteMaskColor.g = mp->_ini.get<int>("global.silhouetteMaskColorG", 0);
    silhouetteMaskColor.b = mp->_ini.get<int>("global.silhouetteMaskColorB", 0);
}

SemiGlobalMatchingParams::~SemiGlobalMatchingParams()
{
    delete prt;
}

std::string SemiGlobalMatchingParams::getREFINE_photo_depthMapFileName(IndexT viewId, int scale, int step)
{
    return mp->getDepthMapFolder() + std::to_string(viewId) + "_depthMap_scale" + mvsUtils::num2str(scale) + "_step" + mvsUtils::num2str(step) + "_refinePhoto.exr";
}

std::string SemiGlobalMatchingParams::getREFINE_photo_simMapFileName(IndexT viewId, int scale, int step)
{
    return mp->getDepthMapFolder() + std::to_string(viewId) + "_simMap_scale" + mvsUtils::num2str(scale) + "_step" + mvsUtils::num2str(step) + "_refinePhoto.exr";
}

std::string SemiGlobalMatchingParams::getREFINE_opt_depthMapFileName(IndexT viewId, int scale, int step)
{
    return mp->getDepthMapFolder() + std::to_string(viewId) + "_depthMap_scale" + mvsUtils::num2str(scale) + "_step" + mvsUtils::num2str(step) + "_refineOpt.exr";
}

std::string SemiGlobalMatchingParams::getREFINE_opt_simMapFileName(IndexT viewId, int scale, int step)
{
    return mp->getDepthMapFolder() + std::to_string(viewId) + "_simMap_scale" + mvsUtils::num2str(scale) + "_step" + mvsUtils::num2str(step) + "_refineOpt.exr";
}

std::string SemiGlobalMatchingParams::getSGMTmpDir()
{
    return mp->getDepthMapFolder() + SGMoutDirName + "/" + SGMtmpDirName + "/";
}

std::string SemiGlobalMatchingParams::getSGM_depthMapFileName(IndexT viewId, int scale, int step)
{
    return mp->getDepthMapFolder() + std::to_string(viewId) + "_depthMap_scale" + mvsUtils::num2str(scale) + "_step" + mvsUtils::num2str(step) + "_SGM.bin";
}

std::string SemiGlobalMatchingParams::getSGM_simMapFileName(IndexT viewId, int scale, int step)
{
    return mp->getDepthMapFolder() + std::to_string(viewId) + "_simMap_scale" + mvsUtils::num2str(scale) + "_step" + mvsUtils::num2str(step) + "_SGM.bin";
}

std::string SemiGlobalMatchingParams::getSGM_idDepthMapFileName(IndexT viewId, int scale, int step)
{
    return mp->getDepthMapFolder() + std::to_string(viewId) + "_idDepthMap_scale" + mvsUtils::num2str(scale) + "_step" + mvsUtils::num2str(step) + "_SGM.png";
}

std::string SemiGlobalMatchingParams::getSGM_tcamsFileName(IndexT viewId)
{
    return mp->getDepthMapFolder() + std::to_string(viewId) + "_tcams.bin";
}

std::string SemiGlobalMatchingParams::getSGM_depthsFileName(IndexT viewId)
{
    return mp->getDepthMapFolder() + std::to_string(viewId) + "_depths.bin";
}

DepthSimMap* SemiGlobalMatchingParams::getDepthSimMapFromBestIdVal(int w, int h, StaticVector<IdValue>* volumeBestIdVal,
                                                           int scale, int step, int rc, int zborder,
                                                           StaticVector<float>* planesDepths)
{
    long tall = clock();

    int volDimX = w;
    int volDimY = h;

    DepthSimMap* depthSimMap = new DepthSimMap(rc, mp, scale, step);

#pragma omp parallel for
    for(int y = 0; y < volDimY; y++)
    {
        for(int x = 0; x < volDimX; x++)
        {
            Pixel pix = Pixel(x * step, y * step);
            Pixel pixScale1 = Pixel(pix.x * scale, pix.y * scale);
            float sim = (*volumeBestIdVal)[y * volDimX + x].value;
            int fpdepthId = (*volumeBestIdVal)[y * volDimX + x].id;
            if((fpdepthId >= zborder) && (fpdepthId < planesDepths->size() - zborder))
            {
                float fpPlaneDepth = (*planesDepths)[fpdepthId];
                Point3d planen = (mp->iRArr[rc] * Point3d(0.0f, 0.0f, 1.0f)).normalize();
                Point3d planep = mp->CArr[rc] + planen * fpPlaneDepth;
                Point3d v = (mp->iCamArr[rc] * Point2d((float)(pixScale1.x), (float)(pixScale1.y))).normalize();
                Point3d p = linePlaneIntersect(mp->CArr[rc], v, planep, planen);
                float depth = (mp->CArr[rc] - p).size();

                // printf("fpdepthId %i, fpPlaneDepth %f, depth %f, x %i y
                // %i\n",fpdepthId,fpPlaneDepth,depth,pixScale1.x,pixScale1.y);

                //(*depthSimMap->dsm)[(pix.y/step)*(depthSimMap->w)+(pix.x/step)].x = depth;
                //(*depthSimMap->dsm)[(pix.y/step)*(depthSimMap->w)+(pix.x/step)].y = sim;
                (*depthSimMap->dsm)[y * volDimX + x].depth = depth;
                (*depthSimMap->dsm)[y * volDimX + x].sim = sim;
            }
            else
            {
                // border cases
                // printf("WARNING fpdepthId == %i\n",fpdepthId);
                // exit(1);
            }
        }
    }

    if(mp->verbose)
        mvsUtils::printfElapsedTime(tall, "getDepthSimMapFromBestIdVal");

    return depthSimMap;
}

} // namespace depthMap
} // namespace aliceVision
