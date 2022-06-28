//Copyright (C) 2003-2022 Gunnar Thalin
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either version 2
//of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.



#if !defined(AFX_DESHAKER_H__ABA11033_4913_4B7B_93F6_18ADE7057717__INCLUDED_)
#define AFX_DESHAKER_H__ABA11033_4913_4B7B_93F6_18ADE7057717__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols
#include "filterVDub.h"

// An obviously invalid value. ;)
#define INVALID_VALUE -583265354.623

/// <summary>
/// Enum for types of edge compensation.
/// </summary>
enum
{
	EDGECOMP_NONE,
	EDGECOMP_ZOOM_ADAPTIVE_AVG,		// Was just called "adaptive zoom" in versions 2.7 and earlier.
	__EDGECOMP_ZOOM_ADAPTIVE_ONLY,	// Not used anymore
	EDGECOMP_ZOOM_FIXED,
	EDGECOMP_ZOOM_ADAPTIVE_AVG_PLUS_FIXED,
	__EDGECOMP_ZOOM_ADAPTIVE_FIXED_ONLY,	// Not used anymore
	EDGECOMP_ZOOM_ADAPTIVE_FULL
};

/// <summary>
/// Enum for types of pass 1 video output
/// </summary>
enum
{
	VIDEOOUTPUT_NONE,
	VIDEOOUTPUT_MOTION_VECTORS_1X,
	VIDEOOUTPUT_MOTION_VECTORS_2X,
	VIDEOOUTPUT_MOTION_VECTORS_5X
};

/// <summary>
/// Describes transformation info for a frame.
/// </summary>
struct RGlobalMotionInfo
{
	double dfShiftX;
	double dfShiftY;
	double dfRotation;
	double dfZoom;
};

/// <summary>
/// Contains projection mapping info between two frames in pass 2. Represented in __int64 for speed. Divide by (1024 * 65536) to get real value.
/// </summary>
struct RProjectInfo
{
	__int64 iStartX;
	__int64 iStartY;
	__int64 iDeltaXForXAxis;
	__int64 iDeltaYForXAxis;
	__int64 iDeltaXForYAxis;
	__int64 iDeltaYForYAxis;
};

/// <summary>
/// During pass 2 this struct contains frame image data and projection mappings for all frames processed in pass 1.
/// </summary>
struct RPass2FrameInfo
{
	RProjectInfo projDestToSrc;				// Projects output pixels to corresponding pixels in source video
	RProjectInfo projThisSrcToPrevSrc;		// Projects source pixels to corresponding pixels in previous frame in source video
	RProjectInfo projThisSrcToNextSrc;		// Projects source pixels to corresponding pixels in next frame in source video
	RProjectInfo *pprojERSCompSrcToOrgSrc;	// Projects pixels in rolling shutter compensated source video to original source video. One projection per line.
	RProjectInfo *pprojERSThisSrcToPrevSrc;	// Projects source pixels to corresponding pixels in previous frame in source video for rolling shutter video. One projection per line.
	RProjectInfo *pprojERSThisSrcToNextSrc;	// Projects source pixels to corresponding pixels in next frame in source video for rolling shutter video. One projection per line.
	// Since the three pprojERS* arrays above require lots of space, they are calculated when needed from the six gm's below.
	RGlobalMotionInfo *pgmERSCompSrcToOrgSrcFrom, *pgmERSCompSrcToOrgSrcTo;
	RGlobalMotionInfo *pgmERSThisSrcToPrevSrcFrom, *pgmERSThisSrcToPrevSrcTo;
	RGlobalMotionInfo *pgmERSThisSrcToNextSrcFrom, *pgmERSThisSrcToNextSrcTo;
	VBitmap *pvbmp;	// This contains an old or future frame when they are in range (-30 to +30 frames for default setting). NULL otherwise.
	BOOL bFirstFrameOfScene;
};

#define round(x) ((x) >= 0 ? (long)((x) + 0.5) : (long)((x) - 0.5))

/// <summary>
/// Just a rectangle with 'double' as type.
/// </summary>
class CRectDbl
{
public:
	CRectDbl(const CRectDbl &srcRect) {*this = srcRect;}
	CRectDbl(int l, int t, int r, int b) {SetRect(l, t, r, b);}
	void operator = (const CRectDbl &srcRect) {SetRect(srcRect.left, srcRect.top, srcRect.right, srcRect.bottom);}
	void SetRect(double l, double t, double r, double b) {left = l; top = t; right = r; bottom = b;}
	operator CRect() {return CRect(round(left), round(top), round(right), round(bottom));}

	double left, right, top, bottom;
};

class CVideoImage;

/// <summary>
/// All variables that deshaker needs to remember between calls from VDub are collected here.
/// </summary>
struct MyFilterData
{
	// User settings
	int iPass;
	int iBlockSize;
	int iSearchRange;
	double dfPixelAspectSrc;
	double dfPixelAspectDest;
	int iPixelAspectSrc;	// Type of aspect ratio. Same as index of combobox (values 0-6)
	int iPixelAspectDest;
	int iDestWidth;
	int iDestHeight;
	int iStopScale;		// 0 = full scale, 1 = half scale, 2 = quarter scale.
	int iMatchPixelInc;	// 1 = use all pixels, 2 = every 4th, 3 = every 9th, 4 = every 16th
	int iShiftXSmoothness;
	int iShiftYSmoothness;
	int iRotateSmoothness;
	int iZoomSmoothness;
	double dfPixelsOffToDiscardBlock;
	int iVideoOutput;
	int iResampleAlg;	// RESAMPLE_NEAREST_NEIGHBOR, RESAMPLE_BILINEAR or RESAMPLE_BICUBIC
	double dfSkipFrameBlockPercent;
	BOOL bDetectScenes;
	double dfNewSceneThreshold;
	int iInitialSearchRange;
	int iDiscardPixelDiff;
	double dfDiscardMatch;
	double dfDiscard2ndMatch;
	CString *pstrLogFile;
	BOOL bLogFileAppend;
	BOOL bInterlaced;
	BOOL bTFF;	// Top field first
	BOOL bIgnoreOutside;
	CRectDbl rectIgnoreOutside;
	BOOL bIgnoreInside;
	CRect rectIgnoreInside;
	int iEdgeComp;
	double dfLimitShiftX;
	double dfLimitShiftY;
	double dfLimitRotation;
	double dfLimitZoom;
	double dfExtraZoom;
	BOOL bUseOldFrames;
	int iOldFrames;
	BOOL bUseFutureFrames;
	int iFutureFrames;
	BOOL bFollowIgnoreOutside;
	int iDeepAnalysisKickIn;
	BOOL bERS;
	BOOL bInterlacedProgressiveDest;
	BOOL bDestSameAsSrc;
	BOOL bMultiFrameBorder;
	BOOL bExtrapolateBorder;
	int iEdgeTransitionPixels;
	double dfAbsPixelsToDiscardBlock;
	BOOL bRememberDiscardedAreas;
	double dfERSAmount;	// For rolling shutter, if A is time for first line, B last line and C first line of next field, dfERSAmount should be 100*(B-A)/(C-A). Is approximately 88 for Sony HDR-HC1(E)
	BOOL bDetectRotation;
	BOOL bDetectZoom;
	double dfAdaptiveZoomSmoothness;
	double dfAdaptiveZoomAmount;
	BOOL bUseColorMask;
	unsigned int uiMaskColor;
	
	// Used in both passes
	int iFrameNo;
	HANDLE hLog;
	CFont *pfontInfoText, *pfontOld;
	BOOL bAllowProc;

	// Used in pass1
	CArray<CVideoImage *, CVideoImage *> *pimgScaled1, *pimgScaled2;
	double *adfDistTimesCosOfBlockAngle, *adfDistTimesSinOfBlockAngle;
	CPen *ppenVectorGood, *ppenVectorBad, *ppenOld;
	int iPrevSourceFrame;
	CArray<CRectDbl *, CRectDbl *> *aprectIgnoreOutside;
	CString *pstrFirstLogRow;
	BOOL bCannotWriteToLogfileMsgWasShown;
	CArray<BYTE *, BYTE *> *apbyPrevFramesUsedBlocks;	// Stores used blocks for all processed frames (one bit per frame; used=1, discarded=0). Used by "remember discarded areas" feature.
	int iUsedBlocksX, iUsedBlocksY;	// Dimension of apbyPrevFramesUsedBlocks contents.
	CArray<double, double> *adfSceneDetectValues;	// Stores scene detect values for all processed frames. Set to -1 if it's a new scene.
	int iTextHeight;

	// Used in pass2
	int iStartFrame;
	int iNoOfFrames;	// or fields for interlaced video
	CVideoImage *pimg1, *pimg2;
	RPass2FrameInfo *aFrameInfo;
	FILETIME filetimeLog;
	double dfSrcToDestScale;
};

/// <summary>
/// Shared work data for threads that find shifts between two frames in parallel.
/// </summary>
struct RFindShiftsWorkData
{
	MyFilterData *mfd;
	CVideoImage *pimgThis;
	CVideoImage *pimgPrev;
	int iScale;
	int iMatchBlocksX;
	int iMatchBlocksY;
	int iMatchBlocksXPrevScale;
	int iMatchBlocksYPrevScale;
	double *adfXShifts;
	double *adfYShifts;
	double *adfXShiftsPrevScale;
	double *adfYShiftsPrevScale;

	int iNextItemToProcess;
	int iTotalItemsToProcess;
	CCriticalSection csNextItem;
};

/// <summary>
/// Work data per scene when creating smooth paths.
/// </summary>
struct RMakePathWorkDataPerScene
{
	int iN;
	const double *pdfPath;
	double *pdfX;
};

/// <summary>
/// Work data for threads when creating smooth path using curvature minimization, in parallel.
/// </summary>
struct RMakePathMinimizeCurvatureWorkData
{
	double dfSmoothness;
	double dfMaxDiff;
	double dfErrorTolerance;
	CArray<RMakePathWorkDataPerScene *, RMakePathWorkDataPerScene *> aSceneData;
	BOOL bStopProcessing;

	int iNextItemToProcess;
	int iTotalItemsToProcess;
	int iStepProgress;
	CCriticalSection csWorkItem;
};

/// <summary>
/// Work data for threads when creating smooth path using Gaussian filter, in parallel.
/// </summary>
struct RMakePathGaussianFilterWorkData
{
	double dfMaxDiffBelow;
	double dfMaxDiffAbove;
	double dfMinAdjust;
	int iKernelSize;
	double *adfGaussKernel;
	CArray<RMakePathWorkDataPerScene *, RMakePathWorkDataPerScene *> aSceneData;
	BOOL bStopProcessing;

	int iNextItemToProcess;
	int iTotalItemsToProcess;
	int iStepProgress;
	CCriticalSection csWorkItem;
};

/////////////////////////////////////////////////////////////////////////////

class CDeShakerApp : public CWinApp
{
public:
	CDeShakerApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDeShakerApp)
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CDeShakerApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern int g_iCPUs;

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DESHAKER_H__ABA11033_4913_4B7B_93F6_18ADE7057717__INCLUDED_)
