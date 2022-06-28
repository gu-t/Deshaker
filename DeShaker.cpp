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



// Contains most of the main routines.

#include "stdafx.h"
#include "DeShaker.h"
#include <math.h>
#include "DlgConfig.h"
#include "VideoImage.h"
#include "DlgProgress.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define INITIAL_MATCH_PIXELS_MIN 480	// Minimum number of pixels in the initial (scale reduced) block during matching.
#define BICUBIC_A -0.6					// Value of "A" to use in bicubic resampling.
#define CONFIG_VERSION 19				// Current version of config settings.

/////////////////////////////////////////////////////////////////////////////
// CDeShakerApp

BEGIN_MESSAGE_MAP(CDeShakerApp, CWinApp)
	//{{AFX_MSG_MAP(CDeShakerApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDeShakerApp construction

CDeShakerApp::CDeShakerApp()
{
}

// The one and only CDeShakerApp object
CDeShakerApp theApp;

// Progress dialog window, when it's visible.
CDlgProgress *g_pdlgProgress = NULL;

// No. of detected CPU's.
int g_iCPUs = 1;

///////////////////////////////////////////////////////////////////////////

int InitProc(FilterActivation *fa, const FilterFunctions *ff);
void DeinitProc(FilterActivation *fa, const FilterFunctions *ff);
int StartProc(FilterActivation *fa, const FilterFunctions *ff);
int EndProc(FilterActivation *fa, const FilterFunctions *ff);
int RunProc(const FilterActivation *fa, const FilterFunctions *ff);
long ParamProc(FilterActivation *fa, const FilterFunctions *ff);
int ConfigProc(FilterActivation *fa, const FilterFunctions *ff, VDXHWND hwnd);
void StringProc(const FilterActivation *fa, const FilterFunctions *ff, char *str);
void ScriptConfig(IScriptInterpreter *isi, void *lpVoid, CScriptValue *argv, int argc);
bool FssProc(FilterActivation *fa, const FilterFunctions *ff, char *buf, int buflen);
void RunProcPass1(const FilterActivation *fa, const FilterFunctions *ff, BOOL bFirstField = TRUE);
void RunProcPass2(const FilterActivation *fa, const FilterFunctions *ff, BOOL bFirstField = TRUE);
int gauss_elim(int n, double a[], double x[]);
BOOL MakePathMinimizeCurvature(int iN, const double adfPath[], double adfX[], double dfSmoothness, double dfMaxDiff, double dfErrorTolerance, const BOOL abNewScene[]);
UINT MakePathMinimizeCurvatureThread(LPVOID pParam);
BOOL GaussSeidelSolvePath(int iN, const double *adfPath, double *adfX, double dfSmoothness, double dfMaxDiff, double dfErrorTolerance, BOOL *pbStopProcessing);
BOOL GaussSeidelSolvePath2(int iN, const double *adfPath, double *adfX, const double *adfSmoothness, double dfErrorTolerance, BOOL *pbStopProcessing);
BOOL MakePathGaussianFilter(int iN, const double adfPath[], double adfX[], double dfSmoothness, double dfMaxDiffBelow, double dfMaxDiffAbove, double dfMinAdjust, const BOOL abNewScene[]);
UINT MakePathGaussianFilterThread(LPVOID pParam);
BOOL GaussianFilterMakePath(int iN, const double *adfPath, double *adfX, int iKernelSize, double *adfGaussKernel, double dfMaxDiffBelow, double dfMaxDiffAbove, double dfMinAdjust, BOOL *pbStopProcessing);
int MakePathWorkDataSceneLengthComparer(const void *arg1, const void *arg2);
CString GetNextItem(CString &str, char *szSeparators = " \t\r\n");
void DeleteFrameInfo(MyFilterData *mfd);
void FixForInterlacedProcessing(FilterActivation *fa, BOOL bFix, BOOL bTopField, BOOL bFirstField, int iPass, BOOL bInterlacedProgressiveDest);
double CalcBlockShiftDiffFromGlobalMotion(const RGlobalMotionInfo &gm, double dfXShift, double dfYShift, MyFilterData *mfd, int iBlockIndex);
void CalcGlobalMotion(const CArray<int, int> &aiValidBlocks, int iStartBlock, int iMatchBlocksX, int iMatchBlocksY, int iImageHeight, const double *adfAb, const double adfXShifts[], const double adfYShifts[], MyFilterData *mfd, int aiUseBlock[], BOOL bERS, RGlobalMotionInfo &gm, RGlobalMotionInfo &gm2, BOOL bDetectRotation, BOOL bDetectZoom);
void CalcGlobalMotion2(const CArray<int, int> &aiValidBlocks, const double *adfAb, BOOL abUseBlock[], BOOL bERS, RGlobalMotionInfo &gm, RGlobalMotionInfo &gm2, BOOL bDetectRotation, BOOL bDetectZoom);
void FreeOneFrameInfo(RPass2FrameInfo *aFrameInfo, int iFrameIndex, BOOL bFreeAll);
UINT FindShiftsThread(LPVOID pParam);
int GetTextHeight(int iVideoHeight);

///////////////////////////////////////////////////////////////////////////

/// <summary>
/// Called by VDub. Creates the string that is saved when user "saves processing settings" in VDub. See VDub documentation for details.
/// </summary>
bool FssProc(FilterActivation *fa, const FilterFunctions *ff, char *buf, int buflen)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	MyFilterData *mfd = (MyFilterData *)fa->filter_data;

	CString strLogFile = *mfd->pstrLogFile;
	strLogFile.Replace("\\", "\\\\");
	_snprintf_s(buf, buflen, buflen, "Config(\"%i|%i|%i|%i|%lg|%i|%lg|%i|%i|%i|%i|%i|%i|%i|%i|%i|%lg|%i|%i|%i|%lg|%i|%lg|%lg|%s|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%lg|%lg|%lg|%lg|%lg|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%lg|%i|%lg|%i|%i|%lg|%lg|%lg|%i|%i|%i|%x\")",
		CONFIG_VERSION,
		mfd->iPass,
		mfd->iBlockSize,
		mfd->iSearchRange,
		mfd->dfPixelAspectSrc,
		mfd->iPixelAspectSrc,
		mfd->dfPixelAspectDest,
		mfd->iPixelAspectDest,
		mfd->iDestWidth,
		mfd->iDestHeight,
		mfd->iStopScale,
		mfd->iMatchPixelInc,
		mfd->iShiftXSmoothness,
		mfd->iShiftYSmoothness,
		mfd->iRotateSmoothness,
		mfd->iZoomSmoothness,
		mfd->dfPixelsOffToDiscardBlock,
		mfd->iVideoOutput,
		mfd->iEdgeComp,
		mfd->iResampleAlg,
		mfd->dfSkipFrameBlockPercent,
		mfd->iInitialSearchRange,
		mfd->dfDiscardMatch,
		mfd->dfDiscard2ndMatch,
		(LPCSTR)strLogFile,
		mfd->bLogFileAppend ? 1 : 0,
		mfd->bIgnoreOutside ? 1 : 0,
		round(mfd->rectIgnoreOutside.left),
		round(mfd->rectIgnoreOutside.right),
		round(mfd->rectIgnoreOutside.top),
		round(mfd->rectIgnoreOutside.bottom),
		mfd->bIgnoreInside ? 1 : 0,
		mfd->rectIgnoreInside.left,
		mfd->rectIgnoreInside.right,
		mfd->rectIgnoreInside.top,
		mfd->rectIgnoreInside.bottom,
		mfd->bInterlaced ? 1 : 0,
		mfd->bTFF ? 1 : 0,
		mfd->dfExtraZoom,
		mfd->dfLimitShiftX,
		mfd->dfLimitShiftY,
		mfd->dfLimitRotation,
		mfd->dfLimitZoom,
		mfd->bUseOldFrames ? 1 : 0,
		mfd->bUseFutureFrames ? 1 : 0,
		mfd->iOldFrames,
		mfd->iFutureFrames,
		mfd->bFollowIgnoreOutside ? 1 : 0,
		mfd->iDeepAnalysisKickIn,
		mfd->bERS ? 1 : 0,
		mfd->bInterlacedProgressiveDest ? 1 : 0,
		mfd->bDestSameAsSrc ? 1 : 0,
		mfd->bMultiFrameBorder ? 1 : 0,
		mfd->bExtrapolateBorder ? 1 : 0,
		mfd->iEdgeTransitionPixels,
		mfd->dfAbsPixelsToDiscardBlock,
		mfd->bRememberDiscardedAreas ? 1 : 0,
		mfd->dfERSAmount,
		mfd->bDetectRotation ? 1 : 0,
		mfd->bDetectZoom ? 1 : 0,
		mfd->dfNewSceneThreshold,
		mfd->dfAdaptiveZoomSmoothness,
		mfd->dfAdaptiveZoomAmount,
		mfd->iDiscardPixelDiff,
		mfd->bDetectScenes ? 1 : 0,
		mfd->bUseColorMask,
		mfd->uiMaskColor);

	return true;
}

/// <summary>
/// Called by VDub. Parses the string created by FssProc. See VDub documentation for details.
/// </summary>
void ScriptConfig(IScriptInterpreter *isi, void *lpVoid, CScriptValue *argv, int argc)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	FilterActivation *fa = (FilterActivation *)lpVoid;
	MyFilterData *mfd = (MyFilterData *)fa->filter_data;

	CString strConfig = *(argv[0].asString());
	int iVersion = atoi(GetNextItem(strConfig, "|"));
	if(iVersion > CONFIG_VERSION)
	{
		AfxMessageBox("Deshaker: The config file is created with a newer version of Deshaker than you have.");
		return;
	}
	mfd->iPass = atoi(GetNextItem(strConfig, "|"));
	mfd->iBlockSize = atoi(GetNextItem(strConfig, "|"));
	mfd->iSearchRange = atoi(GetNextItem(strConfig, "|"));
	mfd->dfPixelAspectSrc = atof(GetNextItem(strConfig, "|"));
	mfd->iPixelAspectSrc = atoi(GetNextItem(strConfig, "|"));
	mfd->dfPixelAspectDest = atof(GetNextItem(strConfig, "|"));
	mfd->iPixelAspectDest = atoi(GetNextItem(strConfig, "|"));
	mfd->iDestWidth = atoi(GetNextItem(strConfig, "|"));
	mfd->iDestHeight = atoi(GetNextItem(strConfig, "|"));
	mfd->iStopScale = atoi(GetNextItem(strConfig, "|"));
	mfd->iMatchPixelInc = atoi(GetNextItem(strConfig, "|"));
	if(iVersion < 17)
		GetNextItem(strConfig, "|");	// Discard removed setting. (RGB or grayscale matching.)
	mfd->iShiftXSmoothness = atoi(GetNextItem(strConfig, "|"));
	mfd->iShiftYSmoothness = atoi(GetNextItem(strConfig, "|"));
	mfd->iRotateSmoothness = atoi(GetNextItem(strConfig, "|"));
	mfd->iZoomSmoothness = atoi(GetNextItem(strConfig, "|"));
	mfd->dfPixelsOffToDiscardBlock = atof(GetNextItem(strConfig, "|"));
	mfd->iVideoOutput = atoi(GetNextItem(strConfig, "|"));
	mfd->iEdgeComp = atoi(GetNextItem(strConfig, "|"));
	mfd->iResampleAlg = atoi(GetNextItem(strConfig, "|"));
	mfd->dfSkipFrameBlockPercent = atof(GetNextItem(strConfig, "|"));
	mfd->iInitialSearchRange = atoi(GetNextItem(strConfig, "|"));
	mfd->dfDiscardMatch = atof(GetNextItem(strConfig, "|"));
	mfd->dfDiscard2ndMatch = atof(GetNextItem(strConfig, "|"));
	*mfd->pstrLogFile = GetNextItem(strConfig, "|");
	mfd->bLogFileAppend = atoi(GetNextItem(strConfig, "|")) == 1;
	mfd->bIgnoreOutside = atoi(GetNextItem(strConfig, "|")) == 1;
	mfd->rectIgnoreOutside.left = atoi(GetNextItem(strConfig, "|"));
	mfd->rectIgnoreOutside.right = atoi(GetNextItem(strConfig, "|"));
	mfd->rectIgnoreOutside.top = atoi(GetNextItem(strConfig, "|"));
	mfd->rectIgnoreOutside.bottom = atoi(GetNextItem(strConfig, "|"));
	mfd->bIgnoreInside = atoi(GetNextItem(strConfig, "|")) == 1;
	mfd->rectIgnoreInside.left = atoi(GetNextItem(strConfig, "|"));
	mfd->rectIgnoreInside.right = atoi(GetNextItem(strConfig, "|"));
	mfd->rectIgnoreInside.top = atoi(GetNextItem(strConfig, "|"));
	mfd->rectIgnoreInside.bottom = atoi(GetNextItem(strConfig, "|"));
	if(iVersion >= 2)
	{
		mfd->bInterlaced = atoi(GetNextItem(strConfig, "|")) == 1;
		mfd->bTFF = atoi(GetNextItem(strConfig, "|")) == 1;
		mfd->dfExtraZoom = atof(GetNextItem(strConfig, "|"));
		mfd->dfLimitShiftX = atof(GetNextItem(strConfig, "|"));
		mfd->dfLimitShiftY = atof(GetNextItem(strConfig, "|"));
		mfd->dfLimitRotation = atof(GetNextItem(strConfig, "|"));
		mfd->dfLimitZoom = atof(GetNextItem(strConfig, "|"));
	}
	else
	{
		mfd->bInterlaced = FALSE;
		mfd->bTFF = FALSE;
		mfd->dfExtraZoom = 1.0;
		mfd->dfLimitShiftX = 100.0;
		mfd->dfLimitShiftY = 100.0;
		mfd->dfLimitRotation = 100.0;
		mfd->dfLimitZoom = 100.0;
	}
	if(iVersion >= 3)
	{
		mfd->bUseOldFrames = atoi(GetNextItem(strConfig, "|")) == 1;
		mfd->bUseFutureFrames = atoi(GetNextItem(strConfig, "|")) == 1;
		mfd->iOldFrames = atoi(GetNextItem(strConfig, "|"));
		mfd->iFutureFrames = atoi(GetNextItem(strConfig, "|"));
	}
	else
	{
		mfd->bUseOldFrames = FALSE;
		mfd->bUseFutureFrames = FALSE;
		mfd->iOldFrames = 0;
		mfd->iFutureFrames = 0;
	}
	if(iVersion >= 4)
	{
		mfd->bFollowIgnoreOutside = atoi(GetNextItem(strConfig, "|")) == 1;
	}
	else
	{
		mfd->bFollowIgnoreOutside = FALSE;
	}
	if(iVersion >= 6)
	{
		mfd->iDeepAnalysisKickIn = atoi(GetNextItem(strConfig, "|"));
	}
	else
	{
		mfd->iDeepAnalysisKickIn = 100;
	}
	if(iVersion >= 7)
	{
		mfd->bERS = atoi(GetNextItem(strConfig, "|")) == 1;
		mfd->bInterlacedProgressiveDest = atoi(GetNextItem(strConfig, "|")) == 1;
	}
	else
	{
		mfd->bERS = FALSE;
		mfd->bInterlacedProgressiveDest = FALSE;
	}
	if(iVersion >= 8)
	{
		mfd->bDestSameAsSrc = atoi(GetNextItem(strConfig, "|")) == 1;
		mfd->bMultiFrameBorder = atoi(GetNextItem(strConfig, "|")) == 1;
		mfd->bExtrapolateBorder = atoi(GetNextItem(strConfig, "|")) == 1;
	}
	else
	{
		mfd->bDestSameAsSrc = FALSE;
		mfd->bMultiFrameBorder = FALSE;
		mfd->bExtrapolateBorder = FALSE;
	}
	if(iVersion >= 9)
	{
		mfd->iEdgeTransitionPixels = atoi(GetNextItem(strConfig, "|"));
		if(iVersion < 17)
		{
			GetNextItem(strConfig, "|");	// Discard removed setting.
			GetNextItem(strConfig, "|");	// Discard removed setting.
		}
	}
	else
	{
		mfd->iEdgeTransitionPixels = 0;
		mfd->iDeepAnalysisKickIn = 100 - mfd->iDeepAnalysisKickIn;
	}
	if(iVersion >= 10)
	{
		mfd->dfAbsPixelsToDiscardBlock = atof(GetNextItem(strConfig, "|"));
	}
	else
	{
		mfd->dfAbsPixelsToDiscardBlock = 1000.0;
	}
	if(iVersion >= 11)
	{
		mfd->bRememberDiscardedAreas = atoi(GetNextItem(strConfig, "|")) == 1;
	}
	else
	{
		mfd->bRememberDiscardedAreas = FALSE;
	}
	if(iVersion >= 12)
	{
		mfd->dfERSAmount = atof(GetNextItem(strConfig, "|"));
	}
	else
	{
		mfd->dfERSAmount = 88.0;
	}
	if(iVersion >= 13)
	{
		mfd->bDetectRotation = atoi(GetNextItem(strConfig, "|")) == 1;
		mfd->bDetectZoom = atoi(GetNextItem(strConfig, "|")) == 1;
	}
	else
	{
		mfd->bDetectRotation = TRUE;
		mfd->bDetectZoom = TRUE;
	}
	if(iVersion >= 14)
	{
		mfd->dfNewSceneThreshold = atof(GetNextItem(strConfig, "|"));
	}
	else
	{
		mfd->dfNewSceneThreshold = 20.0;
	}
	if(iVersion >= 15)
	{
		mfd->dfAdaptiveZoomSmoothness = atof(GetNextItem(strConfig, "|"));
		if(iVersion < 17)
			GetNextItem(strConfig, "|");	// Discard removed setting.
	}
	else
	{
		mfd->dfAdaptiveZoomSmoothness = max(1, mfd->iZoomSmoothness);
		if(mfd->iEdgeComp == __EDGECOMP_ZOOM_ADAPTIVE_ONLY)
		{
			mfd->iEdgeComp = EDGECOMP_ZOOM_ADAPTIVE_AVG;
			mfd->iZoomSmoothness = 0;
		}
		else if(mfd->iEdgeComp == __EDGECOMP_ZOOM_ADAPTIVE_FIXED_ONLY)
		{
			mfd->iEdgeComp = EDGECOMP_ZOOM_ADAPTIVE_AVG_PLUS_FIXED;
			mfd->iZoomSmoothness = 0;
		}
	}
	if(iVersion >= 16)
	{
		mfd->dfAdaptiveZoomAmount = atof(GetNextItem(strConfig, "|"));
	}
	else
	{
		mfd->dfAdaptiveZoomAmount = 100.0;
	}
	if(iVersion >= 17)
	{
		mfd->iDiscardPixelDiff = atoi(GetNextItem(strConfig, "|"));
	}
	else
	{
		mfd->iDiscardPixelDiff = 20;
	}
	if(iVersion >= 18)
	{
		mfd->bDetectScenes = atoi(GetNextItem(strConfig, "|")) == 1;
	}
	else
	{
		mfd->bDetectScenes = TRUE;
	}
	if(iVersion >= 19)
	{
		mfd->bUseColorMask = atoi(GetNextItem(strConfig, "|")) == 1;
		mfd->uiMaskColor = strtoul(GetNextItem(strConfig, "|"), NULL, 16);
	}
	else
	{
		mfd->bUseColorMask = FALSE;
		mfd->uiMaskColor = 255 * 65536 + 255;	// Purple.
	}
}

ScriptFunctionDef func_defs[] =
{
	{ (ScriptFunctionPtr)ScriptConfig, "Config", "0s" },
	{ NULL },
};

CScriptObject script_obj =
{
	NULL, func_defs
};


struct VDXFilterDefinition filterDef =
{
	NULL, NULL, NULL,		// next, prev, module
	"Deshaker v3.1",		// name
#ifdef _DEBUG
	"Debug version! Stabilizes video. Eliminates camera shakiness and makes panning, rotation and zooming smoother.",	// desc
#else
	"Stabilizes video. Eliminates camera shakiness and makes panning, rotation and zooming smoother.",	// desc
#endif
	"Gunnar Thalin",		// maker
	NULL,					// private_data
	sizeof(MyFilterData),	// inst_data_size

	InitProc,	// initProc
	DeinitProc,	// deinitProc
	RunProc,	// runProc
	ParamProc,	// paramProc
	ConfigProc,	// configProc
	StringProc,	// stringProc
	StartProc,	// startProc
	EndProc,	// endProc

	&script_obj,// script_obj
	FssProc,	// fssProc
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};


///////////////////////////////////////////////////////////////////////////

extern "C" int __declspec(dllexport) __cdecl VirtualdubFilterModuleInit2(FilterModule *fm, const FilterFunctions *ff, int& vdfd_ver, int& vdfd_compat);
extern "C" void __declspec(dllexport) __cdecl VirtualdubFilterModuleDeinit(FilterModule *fm, const FilterFunctions *ff);

static VDXFilterDefinition *fd;

///////////////////////////////////////////////////////////////////////////

int __declspec(dllexport) __cdecl VirtualdubFilterModuleInit2(FilterModule *fm, const FilterFunctions *ff, int& vdfd_ver, int& vdfd_compat)
{
	if(!(fd = ff->addFilter(fm, &filterDef, sizeof(VDXFilterDefinition))))
		return 1;

	vdfd_ver = VIRTUALDUB_FILTERDEF_VERSION;
	vdfd_compat = VIRTUALDUB_FILTERDEF_COMPATIBLE;

	return 0;
}

void __declspec(dllexport) __cdecl VirtualdubFilterModuleDeinit(FilterModule *fm, const FilterFunctions *ff)
{
	ff->removeFilter(fd);
}

///////////////////////////////////////////////////////////////////////////

/// <summary>
/// InitProc is called by VDub once when first loading the filter. See VDub documentation for details.
/// </summary>
int InitProc(FilterActivation *fa, const FilterFunctions *ff)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	MyFilterData *mfd = (MyFilterData *)fa->filter_data;

	// Set default settings.
	mfd->iPass = 1;
	mfd->iBlockSize = 30;
	mfd->iSearchRange = 4;
	mfd->bDestSameAsSrc = FALSE;
	mfd->dfPixelAspectSrc = 1.0;
	mfd->iPixelAspectSrc = 0;
	mfd->dfPixelAspectDest = 1.0;
	mfd->iPixelAspectDest = 0;
	mfd->iDestWidth = 640;
	mfd->iDestHeight = 480;
	mfd->iStopScale = 1;
	mfd->iMatchPixelInc = 2;
	mfd->iShiftXSmoothness = 1000;
	mfd->iShiftYSmoothness = 1000;
	mfd->iRotateSmoothness = 1000;
	mfd->iZoomSmoothness = 1000;
	mfd->dfPixelsOffToDiscardBlock = 4.0;
	mfd->iVideoOutput = VIDEOOUTPUT_MOTION_VECTORS_1X;
	mfd->iEdgeComp = EDGECOMP_NONE;
	mfd->iResampleAlg = RESAMPLE_BICUBIC;
	mfd->dfSkipFrameBlockPercent = 8.0;
	mfd->bDetectScenes = TRUE;
	mfd->dfNewSceneThreshold = 20.0;
	mfd->iInitialSearchRange = 30;
	mfd->iDiscardPixelDiff = 20;
	mfd->dfDiscardMatch = 300.0;
	mfd->dfDiscard2ndMatch = 4.0;
	mfd->pstrLogFile = new CString;
	LPSTR pszBuffer = mfd->pstrLogFile->GetBuffer(MAX_PATH);
	if(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, pszBuffer) == S_OK)
	{
		mfd->pstrLogFile->ReleaseBuffer();
		if(mfd->pstrLogFile->Right(1) != "\\")
			*mfd->pstrLogFile += '\\';
		*mfd->pstrLogFile += "Deshaker\\";
		CreateDirectory(*mfd->pstrLogFile, NULL);
	}
	else
	{
		GetWindowsDirectory(pszBuffer, MAX_PATH);
		mfd->pstrLogFile->ReleaseBuffer(1);
		*mfd->pstrLogFile += ":\\";
	}
	*mfd->pstrLogFile += "Deshaker.log";
	mfd->bLogFileAppend = FALSE;
	mfd->bIgnoreOutside = FALSE;
	mfd->rectIgnoreOutside.SetRect(0, 0, 0, 0);
	mfd->bIgnoreInside = FALSE;
	mfd->bFollowIgnoreOutside = FALSE;
	mfd->rectIgnoreInside.SetRect(0, 0, 0, 0);
	mfd->bInterlaced = FALSE;
	mfd->bTFF = FALSE;
	mfd->dfExtraZoom = 1.0;
	mfd->dfLimitShiftX = 15.0;
	mfd->dfLimitShiftY = 15.0;
	mfd->dfLimitRotation = 5.0;
	mfd->dfLimitZoom = 15.0;
	mfd->bUseOldFrames = FALSE;
	mfd->bUseFutureFrames = FALSE;
	mfd->iOldFrames = 30;
	mfd->iFutureFrames = 30;
	mfd->iDeepAnalysisKickIn = 0;
	mfd->bERS = FALSE;
	mfd->bInterlacedProgressiveDest = FALSE;
	mfd->bDestSameAsSrc = TRUE;
	mfd->bMultiFrameBorder = TRUE;
	mfd->bExtrapolateBorder = FALSE;
	mfd->iEdgeTransitionPixels = 10;
	mfd->dfAbsPixelsToDiscardBlock = 1000.0;
	mfd->bRememberDiscardedAreas = TRUE;
	mfd->dfERSAmount = 88.0;
	mfd->bDetectRotation = TRUE;
	mfd->bDetectZoom = TRUE;
	mfd->dfAdaptiveZoomSmoothness = 5000;
	mfd->dfAdaptiveZoomAmount = 100.0;
	mfd->bUseColorMask = FALSE;
	mfd->uiMaskColor = 255 * 65536 + 255;	// Purple.

	mfd->filetimeLog.dwLowDateTime = mfd->filetimeLog.dwHighDateTime = 0;
	mfd->aFrameInfo = NULL;
	mfd->hLog = INVALID_HANDLE_VALUE;
	mfd->bCannotWriteToLogfileMsgWasShown = FALSE;
	mfd->pstrFirstLogRow = NULL;
	mfd->bAllowProc = TRUE;

	mfd->aprectIgnoreOutside = new CArray<CRectDbl *, CRectDbl *>;
	mfd->apbyPrevFramesUsedBlocks = new CArray<BYTE *, BYTE *>;
	mfd->iUsedBlocksX = 0;
	mfd->iUsedBlocksY = 0;
	mfd->adfSceneDetectValues = new CArray<double, double>;

	// Find out how many CPU's there are.
	g_iCPUs = 1;
	char *szCPUs = NULL;
	errno_t err = _dupenv_s(&szCPUs, NULL, "NUMBER_OF_PROCESSORS");
	if(err == 0 && szCPUs != NULL)
	{
		g_iCPUs = atoi(szCPUs);
		if(g_iCPUs <= 0)
			g_iCPUs = 1;
		free(szCPUs);
	}

	return 0;
}

/// <summary>
/// DeinitProc is called by VDub when unloading the filter. See VDub documentation for details.
/// </summary>
void DeinitProc(FilterActivation *fa, const FilterFunctions *ff)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	MyFilterData *mfd = (MyFilterData *)fa->filter_data;

	delete mfd->pstrLogFile;
	mfd->pstrLogFile = NULL;

	DeleteFrameInfo(mfd);

	for(int i = 0; i < mfd->aprectIgnoreOutside->GetSize(); i++)
		delete mfd->aprectIgnoreOutside->GetAt(i);
	delete mfd->aprectIgnoreOutside;

	for(int i = 0; i < mfd->apbyPrevFramesUsedBlocks->GetSize(); i++)
		delete[] mfd->apbyPrevFramesUsedBlocks->GetAt(i);
	delete mfd->apbyPrevFramesUsedBlocks;

	delete mfd->adfSceneDetectValues;
}

/// <summary>
/// ParamProc is called now and then by VDub. It sets some attributes for the output video such as video size. See VDub documentation for details.
/// </summary>
long ParamProc(FilterActivation *fa, const FilterFunctions *ff)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	MyFilterData *mfd = (MyFilterData *)fa->filter_data;

	long lRetVal = FILTERPARAM_SWAP_BUFFERS;
	if(mfd->iPass == 1)
	{
		mfd->iTextHeight = fa->src.h <= 800 ? 11 : 11 + (fa->src.h - 800) / 120;
		if(mfd->iVideoOutput != VIDEOOUTPUT_NONE)
		{
			if(mfd->bInterlaced && fa->dst.h % 2 == 1)
				fa->dst.h--;	// Don't allow odd height if interlaced.

			fa->dst.h += 2 * mfd->iTextHeight;
			if(mfd->bInterlaced)
				fa->dst.h += 2 * mfd->iTextHeight;
			fa->dst.dwFlags = VFBitmap::NEEDS_HDC;	// To be able to draw arrows and text on DC of output video
		}
		else
		{
			fa->dst.w = 8;
			fa->dst.h = 8;
		}
	}
	else
	{
		if(mfd->bDestSameAsSrc)
		{
			fa->dst.w = fa->src.w;
			fa->dst.h = fa->src.h;
		}
		else
		{
			fa->dst.w = mfd->iDestWidth;
			fa->dst.h = mfd->iDestHeight;
		}
		if(mfd->bInterlaced && fa->dst.h % 2 == 1)
			fa->dst.h--;	// Don't allow odd height if interlaced.
		if(mfd->bInterlaced && mfd->bInterlacedProgressiveDest)
			fa->dst.h *= 2;
		fa->dst.dwFlags = VFBitmap::NEEDS_HDC;	// To be able to draw text on DC of output video

		if(mfd->bUseFutureFrames)
			lRetVal |= FILTERPARAM_HAS_LAG(mfd->iFutureFrames);
	}

	// pitch is the address offset, in bytes, between rows.
	//fa->dst.pitch = (fa->dst.w*4 + 7) & -8;	// As suggested in VDub documentation. Old variant?
	fa->dst.AlignTo4();	// As suggested in VDub documentation. Change in version 2.6: Use this instead of line above.

	return lRetVal;
}

/// <summary>
/// The startProc procedure of a filter is called by VDub right before processing begins.
/// Its job is to perform one-time initializations before video frames are processed.
/// See VDub documentation for details.
int StartProc(FilterActivation *fa, const FilterFunctions *ff)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	MyFilterData *mfd = (MyFilterData *)fa->filter_data;

	if(!mfd->bAllowProc)	// We're already in another critical deshaker function. (Can happen when "deshaking..." window is open.)
		return 0;

	mfd->bAllowProc = FALSE;

	char* szError = NULL;
	try
	{
		mfd->pimgScaled1 = NULL;
		mfd->pimgScaled2 = NULL;
		mfd->adfDistTimesCosOfBlockAngle = NULL;
		mfd->adfDistTimesSinOfBlockAngle = NULL;
		mfd->ppenVectorGood = NULL;
		mfd->ppenVectorBad = NULL;
		mfd->pfontInfoText = NULL;
		mfd->hLog = INVALID_HANDLE_VALUE;
		mfd->pstrFirstLogRow = NULL;

		mfd->pimg1 = NULL;
		mfd->pimg2 = NULL;
		mfd->pfontInfoText = NULL;

		szError = new char[200];
		szError[0] = '\0';

		if(mfd->bInterlaced)
			// Fix stuff to manage interlaced processing without too many changes in rest of code.
			FixForInterlacedProcessing(fa, TRUE, TRUE, TRUE, mfd->iPass, mfd->bInterlacedProgressiveDest);

		BOOL bBreak = FALSE;

		mfd->iFrameNo = 0;
		mfd->iPrevSourceFrame = -999;
		if(mfd->iPass == 1)
		{
			// Create a couple of arrays of CVideoImage objects used when downscaling frames. First image in arrays has full size. Next one is downscaled by 2, etc.
			mfd->pimgScaled1 = new CArray<CVideoImage *, CVideoImage *>;
			mfd->pimgScaled2 = new CArray<CVideoImage *, CVideoImage *>;

			int iWidth = fa->src.w;
			int iHeight = fa->src.h;
			while(iWidth * iHeight >= INITIAL_MATCH_PIXELS_MIN && iWidth * 2 >= mfd->iBlockSize && iHeight * 2 >= mfd->iBlockSize)
			{
				CVideoImage *pimg = new CVideoImage;
				pimg->NewImage(iWidth, iHeight, TRUE);
				mfd->pimgScaled1->Add(pimg);
				pimg = new CVideoImage;
				pimg->NewImage(iWidth, iHeight, TRUE);
				mfd->pimgScaled2->Add(pimg);
				iWidth /= 2;
				iHeight /= 2;
			}

			// Precalculate values for each block (based on their positions on the image) that are used later when determining rotation and zoom. (Makes processing faster.)
			int iMatchBlocksX = fa->src.w / mfd->iBlockSize;
			int iMatchBlocksY = fa->src.h / mfd->iBlockSize;
			if(mfd->bInterlaced)
				iMatchBlocksY *= 2;
			double dfX, dfY, dfDiffX, dfDiffY, dfAngleBlock, dfDistToBlock;
			mfd->adfDistTimesCosOfBlockAngle = new double[iMatchBlocksX * iMatchBlocksY];
			mfd->adfDistTimesSinOfBlockAngle = new double[iMatchBlocksX * iMatchBlocksY];
			double dfXMid = fa->src.w / 2.0;
			double dfYMid = mfd->bInterlaced ? fa->src.h : fa->src.h / 2.0;
			int iBlockIndex = 0;
			for(int iBlockY = 0; iBlockY < iMatchBlocksY; iBlockY++)
			{
				if(mfd->bInterlaced)
				{
					// For interlaced video, block info for first field are put in the first half of the arrays and 2nd field in last half.
					if(iBlockY < iMatchBlocksY / 2)
						dfY = 2.0 * (iBlockY * mfd->iBlockSize + mfd->iBlockSize / 2.0) - 0.5;
					else
						dfY = 2.0 * ((iBlockY - iMatchBlocksY / 2) * mfd->iBlockSize + mfd->iBlockSize / 2.0) + 0.5;
				}
				else
					dfY = iBlockY * mfd->iBlockSize + mfd->iBlockSize / 2.0;

				dfDiffY = (dfYMid - dfY) / mfd->dfPixelAspectSrc;
				
				for(int iBlockX = 0; iBlockX < iMatchBlocksX; iBlockX++)
				{
					dfX = iBlockX * mfd->iBlockSize + mfd->iBlockSize / 2.0;
					dfDiffX = dfX - dfXMid;
					dfAngleBlock = atan2(dfDiffX, dfDiffY);		// clockwise, 0 degrees == upwards
					dfDistToBlock = sqrt(dfDiffX * dfDiffX + dfDiffY * dfDiffY);
					mfd->adfDistTimesCosOfBlockAngle[iBlockIndex] = dfDistToBlock * cos(dfAngleBlock);
					mfd->adfDistTimesSinOfBlockAngle[iBlockIndex++] = dfDistToBlock * sin(dfAngleBlock);
				}
			}

			// Create stuff used for drawing arrows and text on output video.
			if(mfd->iVideoOutput != VIDEOOUTPUT_NONE)
			{
				HDC hdc = GetDC(GetDesktopWindow());
				CDC *pdc = CDC::FromHandle(hdc);
				mfd->ppenVectorGood = new CPen(PS_SOLID, 0, RGB(255, 255, 255));
				mfd->ppenVectorBad = new CPen(PS_SOLID, 0, RGB(255, 0, 0));
				mfd->pfontInfoText = new CFont;
				mfd->pfontInfoText->CreatePointFont(mfd->iTextHeight * 80 / 11, "Lucida Console", pdc);
				ReleaseDC(GetDesktopWindow(), hdc);
			}

			if(mfd->hLog != INVALID_HANDLE_VALUE)
			{
				CloseHandle(mfd->hLog);
				mfd->hLog = INVALID_HANDLE_VALUE;
			}
			mfd->bCannotWriteToLogfileMsgWasShown = FALSE;

			delete mfd->pstrFirstLogRow;
			mfd->pstrFirstLogRow = NULL;
		}
		else
		{
			CVideoImage::PrepareCubicTable(BICUBIC_A);
			if(mfd->bExtrapolateBorder)
				CVideoImage::PrepareBorderPixelArray(fa->dst.w * fa->dst.h);

			// These video images aren't really used for anything anymore during pass 2, but they are kind of needed anyway... (I'm lazy)
			mfd->pimg1 = new CVideoImage;
			mfd->pimg1->NewImage(fa->src.w, fa->src.h);
			mfd->pimg2 = new CVideoImage;
			mfd->pimg2->NewImage(fa->dst.w, fa->dst.h);

			mfd->dfSrcToDestScale = min((double)fa->dst.w / fa->src.w, (fa->dst.h / (mfd->bDestSameAsSrc ? mfd->dfPixelAspectSrc : mfd->dfPixelAspectDest)) / (fa->src.h / mfd->dfPixelAspectSrc));

			// Read contents of logfile and parse into internal arrays.
			CArray<double, double> adfShiftX, adfShiftY, adfRotate, adfZoom, adfERSShiftX, adfERSShiftY, adfERSRotate, adfERSZoom;
			CArray<BOOL, BOOL> abNewScene, abSkipped;
			BOOL bNoChange = FALSE;
			mfd->hLog = CreateFile(*mfd->pstrLogFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if(mfd->hLog != INVALID_HANDLE_VALUE)
			{
				FILETIME filetime;
				if(GetFileTime(mfd->hLog, NULL, NULL, &filetime))
				{
					bNoChange = TRUE;
					if(CompareFileTime(&filetime, &mfd->filetimeLog) != 0)
					{
						// Time stamp on logfile shows that user has altered it manually... reload it!
						bNoChange = FALSE;
						DeleteFrameInfo(mfd);
						mfd->iNoOfFrames = -1;
						mfd->filetimeLog = filetime;
						DWORD dwFileLen = GetFileSize(mfd->hLog, NULL);
						if(dwFileLen != INVALID_FILE_SIZE)
						{
							CString strLog, strRow, strItem;
							DWORD dw;
							int iPos = 0, iPosOld = 0;
							mfd->iStartFrame = -999999999;
							int iFrame;
							double dfShiftX = 0, dfShiftY = 0, dfRotate = 0, dfZoom = 0;
							double dfERSShiftX = 0, dfERSShiftY = 0, dfERSRotate = 0, dfERSZoom = 0;
							BOOL bNewScene, bSkipped;

							// Read whole file into strLog and parse it.
							if(ReadFile(mfd->hLog, strLog.GetBuffer(dwFileLen), dwFileLen, &dw, NULL))
							{
								strLog.ReleaseBuffer(dwFileLen);
								while(iPos >= 0)
								{
									iPosOld = iPos;
									iPos = strLog.Find('\n', iPosOld);
									if(iPos >= 0)
									{
										strRow = strLog.Mid(iPosOld, iPos++ - iPosOld);
										int iPosComment = strRow.Find('#');
										if(iPosComment >= 0)
											strRow = strRow.Left(iPosComment);
										strItem = GetNextItem(strRow);
										if(!strItem.IsEmpty() && isdigit(strItem[0]))
										{
											char chField = strItem[strItem.GetLength() - 1];
											if(chField == 'A' || chField == 'B')
												iFrame = 2 * atoi(strItem) + (chField == 'A' ? 0 : 1);
											else
												iFrame = atoi(strItem);
											if(mfd->iStartFrame == -999999999)
												mfd->iStartFrame = iFrame - 1;
											iFrame -= mfd->iStartFrame;
											if(iFrame < 1000000)	// Just to be safe. Hopefully noone will deshake 1000000 frames.
											{
												if(iFrame <= 0)		// If new lowest frame number, insert room for new frames in the beginning.
												{
													adfShiftX.InsertAt(0, 0.0, 1 - iFrame);
													adfShiftY.InsertAt(0, 0.0, 1 - iFrame);
													adfRotate.InsertAt(0, 0.0, 1 - iFrame);
													adfZoom.InsertAt(0, 0.0, 1 - iFrame);
													adfERSShiftX.InsertAt(0, INVALID_VALUE, 1 - iFrame);
													adfERSShiftY.InsertAt(0, INVALID_VALUE, 1 - iFrame);
													adfERSRotate.InsertAt(0, INVALID_VALUE, 1 - iFrame);
													adfERSZoom.InsertAt(0, INVALID_VALUE, 1 - iFrame);
													abNewScene.InsertAt(0, FALSE, 1 - iFrame);
													abSkipped.InsertAt(0, TRUE, 1 - iFrame);
													mfd->iStartFrame += iFrame - 1;
													iFrame = 1;
												}

												if(adfShiftX.GetSize() < iFrame)	// If a gap in frame number sequence, insert default values for those missing frames.
												{
													adfShiftX.InsertAt(adfShiftX.GetSize(), 0.0, iFrame - adfShiftX.GetSize());
													adfShiftY.InsertAt(adfShiftY.GetSize(), 0.0, iFrame - adfShiftY.GetSize());
													adfRotate.InsertAt(adfRotate.GetSize(), 0.0, iFrame - adfRotate.GetSize());
													adfZoom.InsertAt(adfZoom.GetSize(), 0.0, iFrame - adfZoom.GetSize());
													adfERSShiftX.InsertAt(adfERSShiftX.GetSize(), INVALID_VALUE, iFrame - adfERSShiftX.GetSize());
													adfERSShiftY.InsertAt(adfERSShiftY.GetSize(), INVALID_VALUE, iFrame - adfERSShiftY.GetSize());
													adfERSRotate.InsertAt(adfERSRotate.GetSize(), INVALID_VALUE, iFrame - adfERSRotate.GetSize());
													adfERSZoom.InsertAt(adfERSZoom.GetSize(), INVALID_VALUE, iFrame - adfERSZoom.GetSize());
													abNewScene.InsertAt(abNewScene.GetSize(), FALSE, iFrame - abNewScene.GetSize());
													abSkipped.InsertAt(abSkipped.GetSize(), TRUE, iFrame - abSkipped.GetSize());
												}

												bNewScene = strRow.Replace("new_scene", "") > 0 || strRow.Replace("n_scene", "") > 0;
												bSkipped = strRow.Replace("skipped", "") > 0;

												strItem = GetNextItem(strRow);
												dfShiftX = !bSkipped && !strItem.IsEmpty() ? atof(strItem) : 0.0;
												strItem = GetNextItem(strRow);
												dfShiftY = !bSkipped && !strItem.IsEmpty() ? atof(strItem) : 0.0;
												strItem = GetNextItem(strRow);
												dfRotate = !bSkipped && !strItem.IsEmpty() ? atof(strItem) : 0.0;
												strItem = GetNextItem(strRow);
												dfZoom = !bSkipped && !strItem.IsEmpty() ? atof(strItem) : 1.0;
												
												if(mfd->bERS)
												{
													strItem = GetNextItem(strRow);
													dfERSShiftX = !bSkipped && !strItem.IsEmpty() ? atof(strItem) : INVALID_VALUE;
													strItem = GetNextItem(strRow);
													dfERSShiftY = !bSkipped && !strItem.IsEmpty() ? atof(strItem) : INVALID_VALUE;
													strItem = GetNextItem(strRow);
													dfERSRotate = !bSkipped && !strItem.IsEmpty() ? atof(strItem) : INVALID_VALUE;
													strItem = GetNextItem(strRow);
													dfERSZoom = !bSkipped && !strItem.IsEmpty() ? atof(strItem) : INVALID_VALUE;
												}
												else
												{
													dfERSShiftX = INVALID_VALUE;
													dfERSShiftY = INVALID_VALUE;
													dfERSRotate = INVALID_VALUE;
													dfERSZoom = INVALID_VALUE;
												}

												abNewScene.SetAtGrow(iFrame, bNewScene);
												abSkipped.SetAtGrow(iFrame, bSkipped);
												adfShiftX.SetAtGrow(iFrame, dfShiftX);
												adfShiftY.SetAtGrow(iFrame, dfShiftY);
												adfRotate.SetAtGrow(iFrame, dfRotate);
												adfZoom.SetAtGrow(iFrame, log(dfZoom)); // The logarithm of the zoom factor is used in a lot of places to make it "additive". We can then treat those values pretty much like the values for panning and rotation.
												adfERSShiftX.SetAtGrow(iFrame, dfERSShiftX);
												adfERSShiftY.SetAtGrow(iFrame, dfERSShiftY);
												adfERSRotate.SetAtGrow(iFrame, dfERSRotate);
												adfERSZoom.SetAtGrow(iFrame, dfERSZoom);
											}
										}
									}
								}
							}
						}
					}
				}
				CloseHandle(mfd->hLog);
				mfd->hLog = INVALID_HANDLE_VALUE;
			}

			if(!bNoChange)
			{
				// Something was changed in the logfile since last deshake calculation. Do it again.
				mfd->iNoOfFrames = (int)adfShiftX.GetSize();
				if(mfd->iNoOfFrames > 0)
				{
					int iNoOfSmoothPathMakings = mfd->iEdgeComp == EDGECOMP_ZOOM_ADAPTIVE_FULL ? 5 : 4;

					// Show progress dialog while deshaking for pass 2.
					g_pdlgProgress = new CDlgProgress;
					g_pdlgProgress->Create();
					g_pdlgProgress->SetStatus("Deshaking...");
					g_pdlgProgress->SetRange(0, iNoOfSmoothPathMakings * mfd->iNoOfFrames);
					g_pdlgProgress->SetStep(1);
					g_pdlgProgress->SetPos(0);

					// Interpolate values for skipped frames
					int iPrevNonSkippedFrame = 0;
					for(int i = 1; i <= mfd->iNoOfFrames; i++)
					{
						if(i == mfd->iNoOfFrames || !abSkipped[i] || abNewScene[i])		// Treat new scenes as non-skipped frames in this block of code.
						{
							if(i - iPrevNonSkippedFrame > 1)	// We have passed one or more skipped frames, do something about them...
							{
								if(i == mfd->iNoOfFrames || abNewScene[i])
								{
									// If there's a new scene (or no frames at all) after the skipped group, set values for skipped frames to values that start at last non-skipped frame and decreases towards 0.
									double dfFactor = 1.0;
									for(int j = iPrevNonSkippedFrame + 1; j < i; j++)
									{
										dfFactor *= 0.92;
										adfShiftX[j] = dfFactor * adfShiftX[iPrevNonSkippedFrame];
										adfShiftY[j] = dfFactor * adfShiftY[iPrevNonSkippedFrame];
										adfRotate[j] = dfFactor * adfRotate[iPrevNonSkippedFrame];
										adfZoom[j] = dfFactor * adfZoom[iPrevNonSkippedFrame];
										if(mfd->bERS)
										{
											adfERSShiftX[j] = adfShiftX[j];
											adfERSShiftY[j] = adfShiftY[j];
											adfERSRotate[j] = adfRotate[j];
											adfERSZoom[j] = exp(adfZoom[j]);
										}
									}
								}
								else if(i < mfd->iNoOfFrames)
								{
									if(iPrevNonSkippedFrame == 0 || abNewScene[iPrevNonSkippedFrame])
									{
										// If clip begins with skipped frames, or previous valid frame was a new scene, set values for skipped frames to values that end at first non-skipped frame (or 0 if that frame is a new scene) and decrease backwards towards 0.
										double dfFactor = 1.0;
										for(int j = i - 1; j > iPrevNonSkippedFrame; j--)
										{
											dfFactor *= 0.92;
											adfShiftX[j] = dfFactor * (!abNewScene[i] ? adfShiftX[i] : 0.0);
											adfShiftY[j] = dfFactor * (!abNewScene[i] ? adfShiftY[i] : 0.0);
											adfRotate[j] = dfFactor * (!abNewScene[i] ? adfRotate[i] : 0.0);
											adfZoom[j] = dfFactor * (!abNewScene[i] ? adfZoom[i] : 0.0);
											if(mfd->bERS)
											{
												adfERSShiftX[j] = adfShiftX[j];
												adfERSShiftY[j] = adfShiftY[j];
												adfERSRotate[j] = adfRotate[j];
												adfERSZoom[j] = exp(adfZoom[j]);
											}
										}
									}
									else
									{
										// Interpolate (linearly) values for the skipped group of frames.
										for(int j = iPrevNonSkippedFrame + 1; j < i; j++)
										{
											adfShiftX[j] = (adfShiftX[iPrevNonSkippedFrame] * (i - j) + adfShiftX[i] * (j - iPrevNonSkippedFrame)) / (i - iPrevNonSkippedFrame);
											adfShiftY[j] = (adfShiftY[iPrevNonSkippedFrame] * (i - j) + adfShiftY[i] * (j - iPrevNonSkippedFrame)) / (i - iPrevNonSkippedFrame);
											adfRotate[j] = (adfRotate[iPrevNonSkippedFrame] * (i - j) + adfRotate[i] * (j - iPrevNonSkippedFrame)) / (i - iPrevNonSkippedFrame);
											adfZoom[j] = (adfZoom[iPrevNonSkippedFrame] * (i - j) + adfZoom[i] * (j - iPrevNonSkippedFrame)) / (i - iPrevNonSkippedFrame);
											if(mfd->bERS)
											{
												adfERSShiftX[j] = adfShiftX[j];
												adfERSShiftY[j] = adfShiftY[j];
												adfERSRotate[j] = adfRotate[j];
												adfERSZoom[j] = exp(adfZoom[j]);
											}
										}
									}
								}
							}

							iPrevNonSkippedFrame = i;
						}
					}

					// Calculate and store projection info between successive frames if "use previous and future frames to fill in borders" option is used.
					// Also calculate projection info for rolling shutter distortion.
					mfd->aFrameInfo = new RPass2FrameInfo[mfd->iNoOfFrames];
					for(int i = 0; i <= mfd->iNoOfFrames; i++)
					{
						BOOL bFirstField = i % 2 == 0;
						BOOL bTopField = (!bFirstField && !mfd->bTFF || bFirstField && mfd->bTFF);

						if(i < mfd->iNoOfFrames)
						{
							mfd->aFrameInfo[i].pvbmp = NULL;
							mfd->aFrameInfo[i].pprojERSCompSrcToOrgSrc = NULL;
							mfd->aFrameInfo[i].pprojERSThisSrcToPrevSrc = NULL;
							mfd->aFrameInfo[i].pprojERSThisSrcToNextSrc = NULL;
							mfd->aFrameInfo[i].pgmERSCompSrcToOrgSrcFrom = NULL;
							mfd->aFrameInfo[i].pgmERSCompSrcToOrgSrcTo = NULL;
							mfd->aFrameInfo[i].pgmERSThisSrcToPrevSrcFrom = NULL;
							mfd->aFrameInfo[i].pgmERSThisSrcToPrevSrcTo = NULL;
							mfd->aFrameInfo[i].pgmERSThisSrcToNextSrcFrom = NULL;
							mfd->aFrameInfo[i].pgmERSThisSrcToNextSrcTo = NULL;
							mfd->aFrameInfo[i].bFirstFrameOfScene = abNewScene[i];
							if(mfd->bUseOldFrames)
							{
								if(adfERSShiftX[i] != INVALID_VALUE && adfERSShiftY[i] != INVALID_VALUE && adfERSRotate[i] != INVALID_VALUE && adfERSZoom[i] != INVALID_VALUE)
								{
									// Using rolling shutter.
									// Remember start and stop points for global motion parameters. They are used later to create projection mappings only when they are needed (cause they require a lot of memory)
									RGlobalMotionInfo *pgm = new RGlobalMotionInfo;
									pgm->dfShiftX = adfERSShiftX[i];
									pgm->dfShiftY = adfERSShiftY[i];
									pgm->dfRotation = adfERSRotate[i];
									pgm->dfZoom = adfERSZoom[i];
									mfd->aFrameInfo[i].pgmERSThisSrcToPrevSrcFrom = pgm;
									pgm = new RGlobalMotionInfo;
									pgm->dfShiftX = 2.0 * adfShiftX[i] - adfERSShiftX[i];
									pgm->dfShiftY = 2.0 * adfShiftY[i] - adfERSShiftY[i];
									pgm->dfRotation = 2.0 * adfRotate[i] - adfERSRotate[i];
									pgm->dfZoom = exp(2.0 * adfZoom[i] - log(adfERSZoom[i]));
									mfd->aFrameInfo[i].pgmERSThisSrcToPrevSrcTo = pgm;
								}
								else
									CVideoImage::CalcProjection(fa->src.w, fa->src.h, fa->src.w, fa->src.h, adfRotate[i], exp(adfZoom[i]), adfShiftX[i], adfShiftY[i], 1.0, mfd->dfPixelAspectSrc, mfd->dfPixelAspectSrc, mfd->bInterlaced, mfd->bInterlaced, bTopField, TRUE, mfd->iResampleAlg, &mfd->aFrameInfo[i].projThisSrcToPrevSrc);
							}
						}

						if(i > 0)
						{
							if(i < mfd->iNoOfFrames && mfd->bUseFutureFrames)
							{
								if(adfERSShiftX[i] != INVALID_VALUE && adfERSShiftY[i] != INVALID_VALUE && adfERSRotate[i] != INVALID_VALUE && adfERSZoom[i] != INVALID_VALUE)
								{
									// Using rolling shutter.
									// Remember start and stop points for global motion parameters. They are used later to create projection mappings only when they are needed (cause they require a lot of memory)
									RGlobalMotionInfo *pgm = new RGlobalMotionInfo;
									pgm->dfShiftX = adfERSShiftX[i];
									pgm->dfShiftY = adfERSShiftY[i];
									pgm->dfRotation = adfERSRotate[i];
									pgm->dfZoom = adfERSZoom[i];
									mfd->aFrameInfo[i - 1].pgmERSThisSrcToNextSrcFrom = pgm;
									pgm = new RGlobalMotionInfo;
									pgm->dfShiftX = 2.0 * adfShiftX[i] - adfERSShiftX[i];
									pgm->dfShiftY = 2.0 * adfShiftY[i] - adfERSShiftY[i];
									pgm->dfRotation = 2.0 * adfRotate[i] - adfERSRotate[i];
									pgm->dfZoom = exp(2.0 * adfZoom[i] - log(adfERSZoom[i]));
									mfd->aFrameInfo[i - 1].pgmERSThisSrcToNextSrcTo = pgm;
								}
								else
									CVideoImage::CalcReversedProjection(fa->src.w, fa->src.h, fa->src.w, fa->src.h, adfRotate[i], exp(adfZoom[i]), adfShiftX[i], adfShiftY[i], 1.0, mfd->dfPixelAspectSrc, mfd->dfPixelAspectSrc, mfd->bInterlaced, mfd->bInterlaced, bTopField, TRUE, mfd->iResampleAlg, &mfd->aFrameInfo[i - 1].projThisSrcToNextSrc);
							}

							if(i < mfd->iNoOfFrames && (adfERSShiftX[i] != INVALID_VALUE && adfERSShiftY[i] != INVALID_VALUE && adfERSRotate[i] != INVALID_VALUE && adfERSZoom[i] != INVALID_VALUE))
							{
								// Using rolling shutter.
								// The camera moved (mfd->dfERSAmount * <the difference between first line in this frame and first line in previous frame>) during previous frame. This must be compensated.
								RGlobalMotionInfo *pgm = new RGlobalMotionInfo;
								pgm->dfShiftX = adfERSShiftX[i] * mfd->dfERSAmount / 200.0;
								pgm->dfShiftY = adfERSShiftY[i] * mfd->dfERSAmount / 200.0;
								pgm->dfRotation = adfERSRotate[i] * mfd->dfERSAmount / 200.0;
								pgm->dfZoom = exp(log(adfERSZoom[i]) * mfd->dfERSAmount / 200.0);
								mfd->aFrameInfo[i - 1].pgmERSCompSrcToOrgSrcFrom = pgm;		// Set GlobalMotionInfo used to compensate first line of *previous* frame.
								pgm = new RGlobalMotionInfo;
								pgm->dfShiftX = -adfERSShiftX[i] * mfd->dfERSAmount / 200.0;
								pgm->dfShiftY = -adfERSShiftY[i] * mfd->dfERSAmount / 200.0;
								pgm->dfRotation = -adfERSRotate[i] * mfd->dfERSAmount / 200.0;
								pgm->dfZoom = exp(-log(adfERSZoom[i]) * mfd->dfERSAmount / 200.0);
								mfd->aFrameInfo[i - 1].pgmERSCompSrcToOrgSrcTo = pgm;		// Set GlobalMotionInfo used to compensate last line of *previous* frame.
							}
							else if(mfd->bERS && i > 2)
							{
								// Using rolling shutter.
								// Since correct rolling shutter correction for a frame depends on the frame after, the very last frame (and any frame that has a skipped frame after it) will be compensated a little differently, and less accurately, by looking at the difference of its middle line and the previous frame's middle line.
								RGlobalMotionInfo *pgm = new RGlobalMotionInfo;
								pgm->dfShiftX = (adfShiftX[i - 1] - adfShiftX[i - 2]) * mfd->dfERSAmount / 200.0;
								pgm->dfShiftY = (adfShiftY[i - 1] - adfShiftY[i - 2]) * mfd->dfERSAmount / 200.0;
								pgm->dfRotation = (adfRotate[i - 1] - adfRotate[i - 2]) * mfd->dfERSAmount / 200.0;
								pgm->dfZoom = exp((adfZoom[i - 1] - adfZoom[i - 2]) * mfd->dfERSAmount / 200.0);
								mfd->aFrameInfo[i - 1].pgmERSCompSrcToOrgSrcFrom = pgm;
								pgm = new RGlobalMotionInfo;
								pgm->dfShiftX = -(adfShiftX[i - 1] - adfShiftX[i - 2]) * mfd->dfERSAmount / 200.0;
								pgm->dfShiftY = -(adfShiftY[i - 1] - adfShiftY[i - 2]) * mfd->dfERSAmount / 200.0;
								pgm->dfRotation = -(adfRotate[i - 1] - adfRotate[i - 2]) * mfd->dfERSAmount / 200.0;
								pgm->dfZoom = exp(-(adfZoom[i - 1] - adfZoom[i - 2]) * mfd->dfERSAmount / 200.0);
								mfd->aFrameInfo[i - 1].pgmERSCompSrcToOrgSrcTo = pgm;
							}

							if(i < mfd->iNoOfFrames)
							{
								// Make the arrays contain not the differences between two successive frames, but the whole path (to be smoothed below).
								adfShiftX[i] += adfShiftX[i - 1];
								adfShiftY[i] += adfShiftY[i - 1];
								adfRotate[i] += adfRotate[i - 1];
								adfZoom[i] += adfZoom[i - 1];
							}
						}
					}

					double *adfCompShiftX = new double[mfd->iNoOfFrames];
					double *adfCompShiftY = new double[mfd->iNoOfFrames];
					double *adfCompRotate = new double[mfd->iNoOfFrames];
					double *adfCompZoom = new double[mfd->iNoOfFrames];

					// Calculate smooth paths based on the observed paths. (Could the error tolerance be multiplied by, say, 100? Probably! Done!!)
					if(!bBreak)
						bBreak = !MakePathMinimizeCurvature(mfd->iNoOfFrames, adfShiftX.GetData(), adfCompShiftX, mfd->iShiftXSmoothness > 0 ? 10.0 / mfd->iShiftXSmoothness : (mfd->iShiftXSmoothness < 0 ? -999.0 : -1.0), fa->dst.w * mfd->dfLimitShiftX / 100.0, 0.01, abNewScene.GetData());
					if(!bBreak)
						bBreak = !MakePathMinimizeCurvature(mfd->iNoOfFrames, adfShiftY.GetData(), adfCompShiftY, mfd->iShiftYSmoothness > 0 ? 10.0 / mfd->iShiftYSmoothness : (mfd->iShiftYSmoothness < 0 ? -999.0 : -1.0), fa->dst.h * mfd->dfLimitShiftY / 100.0, 0.01, abNewScene.GetData());
					if(!bBreak)
						bBreak = !MakePathMinimizeCurvature(mfd->iNoOfFrames, adfRotate.GetData(), adfCompRotate, mfd->iRotateSmoothness > 0 ? 10.0 / mfd->iRotateSmoothness : (mfd->iRotateSmoothness < 0 ? -999.0 : -1.0), mfd->dfLimitRotation, 0.001, abNewScene.GetData());
					if(!bBreak)
						bBreak = !MakePathMinimizeCurvature(mfd->iNoOfFrames, adfZoom.GetData(), adfCompZoom, mfd->iZoomSmoothness > 0 ? 10.0 / mfd->iZoomSmoothness : (mfd->iZoomSmoothness < 0 ? -999.0 : -1.0), log(1.0 + mfd->dfLimitZoom / 100.0), 0.00001, abNewScene.GetData());

					// Subtract the observed path positions from the smoothed ones to get the compensations to be applied to each frame. (Zoom will be done later.)
					if(!bBreak)
						for(int i = 0; i < mfd->iNoOfFrames; i++)
						{
							adfCompShiftX[i] -= adfShiftX[i];
							adfCompShiftY[i] -= adfShiftY[i];
							adfCompRotate[i] -= adfRotate[i];
						}

					// Recalculate smooth zoom if adaptive zooming should be used.
					if(!bBreak && mfd->iEdgeComp != EDGECOMP_NONE)
					{
						if(mfd->iEdgeComp != EDGECOMP_ZOOM_FIXED)
						{
							double *adfAdaptiveZoom = new double[mfd->iNoOfFrames];
							for(int i = 0; i < mfd->iNoOfFrames; i++)
							{
								double dfZoomNoBorderFact;
								if(mfd->aFrameInfo[i].pgmERSCompSrcToOrgSrcFrom != NULL)
								{
									RGlobalMotionInfo *pgmFrom = mfd->aFrameInfo[i].pgmERSCompSrcToOrgSrcFrom;
									RGlobalMotionInfo *pgmTo = mfd->aFrameInfo[i].pgmERSCompSrcToOrgSrcTo;
									dfZoomNoBorderFact = mfd->pimg1->FindZoomFactorToAvoidBorder(mfd->pimg2->m_iWidth, mfd->pimg2->m_iHeight, adfCompRotate[i] + pgmFrom->dfRotation, adfCompShiftX[i] + pgmFrom->dfShiftX, adfCompShiftY[i] + pgmFrom->dfShiftY, mfd->dfSrcToDestScale, mfd->dfPixelAspectSrc, (mfd->bDestSameAsSrc ? mfd->dfPixelAspectSrc : mfd->dfPixelAspectDest) * (mfd->bInterlaced && mfd->bInterlacedProgressiveDest ? 2.0 : 1.0), mfd->bInterlaced && !mfd->bInterlacedProgressiveDest) / pgmFrom->dfZoom;
									double dfZoomNoBorderFact2 = mfd->pimg1->FindZoomFactorToAvoidBorder(mfd->pimg2->m_iWidth, mfd->pimg2->m_iHeight, adfCompRotate[i] + pgmTo->dfRotation, adfCompShiftX[i] + pgmTo->dfShiftX, adfCompShiftY[i] + pgmTo->dfShiftY, mfd->dfSrcToDestScale, mfd->dfPixelAspectSrc, (mfd->bDestSameAsSrc ? mfd->dfPixelAspectSrc : mfd->dfPixelAspectDest) * (mfd->bInterlaced && mfd->bInterlacedProgressiveDest ? 2.0 : 1.0), mfd->bInterlaced && !mfd->bInterlacedProgressiveDest) / pgmTo->dfZoom;
									if(dfZoomNoBorderFact > dfZoomNoBorderFact2)
										dfZoomNoBorderFact = dfZoomNoBorderFact2;
								}
								else
								{
									dfZoomNoBorderFact = mfd->pimg1->FindZoomFactorToAvoidBorder(mfd->pimg2->m_iWidth, mfd->pimg2->m_iHeight,  adfCompRotate[i], adfCompShiftX[i], adfCompShiftY[i], mfd->dfSrcToDestScale, mfd->dfPixelAspectSrc, (mfd->bDestSameAsSrc ? mfd->dfPixelAspectSrc : mfd->dfPixelAspectDest) * (mfd->bInterlaced && mfd->bInterlacedProgressiveDest ? 2.0 : 1.0), mfd->bInterlaced && !mfd->bInterlacedProgressiveDest);
								}

								double dfZoomNoBorderDiff = log(dfZoomNoBorderFact);
								adfAdaptiveZoom[i] = dfZoomNoBorderDiff - (adfCompZoom[i] - adfZoom[i]);
								if(adfAdaptiveZoom[i] > 0)
									adfAdaptiveZoom[i] = 0;	// Only zoom IN for adaptive zoom. Never OUT.
							}

							double *adfSmoothAdaptiveZoom = new double[mfd->iNoOfFrames];
							//bBreak = GaussSeidelSolvePath(mfd->iNoOfFrames, adfAdaptiveZoom, adfSmoothAdaptiveZoom, mfd->iAdaptiveZoomSmoothness > 0 ? 10.0 / mfd->iAdaptiveZoomSmoothness : (mfd->iAdaptiveZoomSmoothness < 0 ? -999.0 : -1.0), log(1.0 + 100.0/*mfd->dfLimitAdaptiveZoom*/ / 100.0), 0.00001, FALSE, abNewScene.GetData()) == 0;	// Was used in versions 2.7 and earlier.
							bBreak = !MakePathGaussianFilter(mfd->iNoOfFrames, adfAdaptiveZoom, adfSmoothAdaptiveZoom, mfd->dfAdaptiveZoomSmoothness, -1, mfd->iEdgeComp == EDGECOMP_ZOOM_ADAPTIVE_FULL ? 0 : -1, 0.001, abNewScene.GetData());

							// Apply the smoothed adaptive zoom.
							for(int i = 0; i < mfd->iNoOfFrames; i++)
								adfCompZoom[i] += adfSmoothAdaptiveZoom[i] * mfd->dfAdaptiveZoomAmount / 100;

							delete[] adfAdaptiveZoom;
							delete[] adfSmoothAdaptiveZoom;
						}

						// Calculate fixed zoom.
						if(mfd->iEdgeComp == EDGECOMP_ZOOM_FIXED || mfd->iEdgeComp == EDGECOMP_ZOOM_ADAPTIVE_AVG_PLUS_FIXED)
						{
							double dfMaxZoom = 0.0;
							for(int i = 0; i <= mfd->iNoOfFrames; i++)
							{
								// Apply the max zoom found to previous scene.
								if(i == mfd->iNoOfFrames || mfd->aFrameInfo[i].bFirstFrameOfScene)
								{
									for(int i2 = i - 1; i2 >= 0; i2--)
									{
										adfCompZoom[i2] += dfMaxZoom;
										if(mfd->aFrameInfo[i2].bFirstFrameOfScene)
											break;
									}
									dfMaxZoom = 0.0;
								}

								if(i < mfd->iNoOfFrames)
								{
									double dfZoomNoBorderFact;
									if(mfd->aFrameInfo[i].pgmERSCompSrcToOrgSrcFrom != NULL)
									{
										RGlobalMotionInfo *pgmFrom = mfd->aFrameInfo[i].pgmERSCompSrcToOrgSrcFrom;
										RGlobalMotionInfo *pgmTo = mfd->aFrameInfo[i].pgmERSCompSrcToOrgSrcTo;
										dfZoomNoBorderFact = mfd->pimg1->FindZoomFactorToAvoidBorder(mfd->pimg2->m_iWidth, mfd->pimg2->m_iHeight, adfCompRotate[i] + pgmFrom->dfRotation, adfCompShiftX[i] + pgmFrom->dfShiftX, adfCompShiftY[i] + pgmFrom->dfShiftY, mfd->dfSrcToDestScale, mfd->dfPixelAspectSrc, (mfd->bDestSameAsSrc ? mfd->dfPixelAspectSrc : mfd->dfPixelAspectDest) * (mfd->bInterlaced && mfd->bInterlacedProgressiveDest ? 2.0 : 1.0), mfd->bInterlaced && !mfd->bInterlacedProgressiveDest) / pgmFrom->dfZoom;
										double dfZoomNoBorderFact2 = mfd->pimg1->FindZoomFactorToAvoidBorder(mfd->pimg2->m_iWidth, mfd->pimg2->m_iHeight, adfCompRotate[i] + pgmTo->dfRotation, adfCompShiftX[i] + pgmTo->dfShiftX, adfCompShiftY[i] + pgmTo->dfShiftY, mfd->dfSrcToDestScale, mfd->dfPixelAspectSrc, (mfd->bDestSameAsSrc ? mfd->dfPixelAspectSrc : mfd->dfPixelAspectDest) * (mfd->bInterlaced && mfd->bInterlacedProgressiveDest ? 2.0 : 1.0), mfd->bInterlaced && !mfd->bInterlacedProgressiveDest) / pgmTo->dfZoom;
										if(dfZoomNoBorderFact > dfZoomNoBorderFact2)
											dfZoomNoBorderFact = dfZoomNoBorderFact2;
									}
									else
									{
										dfZoomNoBorderFact = mfd->pimg1->FindZoomFactorToAvoidBorder(mfd->pimg2->m_iWidth, mfd->pimg2->m_iHeight, adfCompRotate[i], adfCompShiftX[i], adfCompShiftY[i], mfd->dfSrcToDestScale, mfd->dfPixelAspectSrc, (mfd->bDestSameAsSrc ? mfd->dfPixelAspectSrc : mfd->dfPixelAspectDest) * (mfd->bInterlaced && mfd->bInterlacedProgressiveDest ? 2.0 : 1.0), mfd->bInterlaced && !mfd->bInterlacedProgressiveDest);
									}

									double dfZoomNoBorderDiff = log(dfZoomNoBorderFact);
									if(dfZoomNoBorderDiff - (adfCompZoom[i] - adfZoom[i]) < dfMaxZoom)
										dfMaxZoom = dfZoomNoBorderDiff - (adfCompZoom[i] - adfZoom[i]);
								}
							}
						}
					}

					// Subtract the observed zoom path positions from the smoothed ones to get the zoom compensation factor to be applied to each frame.
					if(!bBreak)
						for(int i = 0; i < mfd->iNoOfFrames; i++)
							adfCompZoom[i] = exp(adfCompZoom[i] - adfZoom[i]);

					// Calculate projection info from compensated frames to original frames.
					if(!bBreak)
					{
						for(int i = 0; i < mfd->iNoOfFrames; i++)
						{
							BOOL bFirstField = i % 2 == 0;
							BOOL bTopField = (!bFirstField && !mfd->bTFF || bFirstField && mfd->bTFF);
							CVideoImage::CalcProjection(fa->src.w, fa->src.h, fa->dst.w, fa->dst.h, adfCompRotate[i], adfCompZoom[i] / mfd->dfExtraZoom, adfCompShiftX[i], adfCompShiftY[i], mfd->dfSrcToDestScale, mfd->dfPixelAspectSrc, mfd->bDestSameAsSrc ? mfd->dfPixelAspectSrc : mfd->dfPixelAspectDest, mfd->bInterlaced, mfd->bInterlaced && !mfd->bInterlacedProgressiveDest, bTopField, FALSE, mfd->iResampleAlg, &mfd->aFrameInfo[i].projDestToSrc);
						}
					}

					delete[] adfCompShiftX;
					delete[] adfCompShiftY;
					delete[] adfCompRotate;
					delete[] adfCompZoom;

					delete g_pdlgProgress;
					g_pdlgProgress = NULL;
				}
			}

			// Create font for drawing text on output video.
			HDC hdc = GetDC(GetDesktopWindow());
			CDC *pdc = CDC::FromHandle(hdc);
			mfd->pfontInfoText = new CFont;
			mfd->pfontInfoText->CreatePointFont(90, "Verdana", pdc);

			ReleaseDC(GetDesktopWindow(), hdc);
		}

		if(mfd->bInterlaced)
			// Undo fix for interlaced processing.
			FixForInterlacedProcessing(fa, FALSE, TRUE, TRUE, mfd->iPass, mfd->bInterlacedProgressiveDest);

		if(bBreak)
		{
			mfd->iNoOfFrames = -1;
			mfd->filetimeLog.dwLowDateTime = mfd->filetimeLog.dwHighDateTime = 0;
		}
	}
	catch(CException* pe)
	{
		if(szError != NULL)
			pe->GetErrorMessage(szError, 200);

		pe->Delete();
	}

	mfd->bAllowProc = TRUE;

	if(szError == NULL)
		ff->ExceptOutOfMemory();
	else if(szError[0] != '\0')
		ff->Except(szError);
	else
		delete[] szError;

	return 0;
}

/// <summary>
/// Cleans up what startProc did. See VDub documentation for details.
/// </summary>
int EndProc(FilterActivation *fa, const FilterFunctions *ff)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	MyFilterData *mfd = (MyFilterData *)fa->filter_data;

	// Don't rely on mfd->iPass in this function. When running jobs, VirtualDub might change the settings to pass2 before calling EndProc for pass1.
	if(mfd->pimgScaled1 != NULL)
	{	// Free pass 1 stuff.
		for(int i = 0; i < mfd->pimgScaled1->GetSize(); i++)
			delete mfd->pimgScaled1->GetAt(i);
		delete mfd->pimgScaled1;
		mfd->pimgScaled1 = NULL;

		for(int i = 0; i < mfd->pimgScaled2->GetSize(); i++)
			delete mfd->pimgScaled2->GetAt(i);
		delete mfd->pimgScaled2;
		mfd->pimgScaled2 = NULL;

		delete[] mfd->adfDistTimesCosOfBlockAngle;
		mfd->adfDistTimesCosOfBlockAngle = NULL;
		delete[] mfd->adfDistTimesSinOfBlockAngle;
		mfd->adfDistTimesSinOfBlockAngle = NULL;

		if(mfd->iVideoOutput != VIDEOOUTPUT_NONE)
		{
			if(mfd->ppenVectorGood != NULL)
			{
				mfd->ppenVectorGood->DeleteObject();
				delete mfd->ppenVectorGood;
				mfd->ppenVectorGood = NULL;
			}

			if(mfd->ppenVectorBad != NULL)
			{
				mfd->ppenVectorBad->DeleteObject();
				delete mfd->ppenVectorBad;
				mfd->ppenVectorBad = NULL;
			}

			if(mfd->pfontInfoText != NULL)
			{
				mfd->pfontInfoText->DeleteObject();
				delete mfd->pfontInfoText;
				mfd->pfontInfoText = NULL;
			}
		}

		if(mfd->hLog != INVALID_HANDLE_VALUE)
		{
			CloseHandle(mfd->hLog);
			mfd->hLog = INVALID_HANDLE_VALUE;
		}

		delete mfd->pstrFirstLogRow;
		mfd->pstrFirstLogRow = NULL;
	}

	if(mfd->pimg1 != NULL)
	{	// Free pass 2 stuff.
		CVideoImage::FreeCubicTable();
		CVideoImage::FreeBorderPixelArray();
		
		delete mfd->pimg1;
		mfd->pimg1 = NULL;
		delete mfd->pimg2;
		mfd->pimg2 = NULL;

		mfd->pfontInfoText->DeleteObject();
		delete mfd->pfontInfoText;
		mfd->pfontInfoText = NULL;
	}

	return 0;
}

/// <summary>
/// Called by VDub once for each frame during processing. Runs RunProcPass1 or RunProcPass2 depending on pass.
/// Note that images in fa->src and fa->dst are store BOTTOM-UP! In CVideoImage they are TOP-DOWN.
/// See VDub documentation for more details.
/// </summary>
int RunProc(const FilterActivation *fa, const FilterFunctions *ff)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	MyFilterData *mfd = (MyFilterData *)fa->filter_data;

	if(!mfd->bAllowProc)		// We're already in another critical deshaker function. (Can happen when "deshaking..." window is open.)
		return 0;

	if(fa->pfsi->lCurrentSourceFrame < 0)	// Can actually happen sometimes
		return 0;

	if(fa->src.data == NULL)	// Can happen too on some VirtualDub versions. At least a VirtualDub-MPEG2 version I tried.
		return 0;

	mfd->bAllowProc = FALSE;

	char* szError = NULL;
	try
	{
		szError = new char[200];
		szError[0] = '\0';
		if(mfd->iPass == 1)
		{
			if(mfd->bInterlaced)
			{
				FixForInterlacedProcessing((FilterActivation *)fa, TRUE, mfd->bTFF, TRUE, 1, mfd->bInterlacedProgressiveDest);
				RunProcPass1(fa, ff, TRUE);
				FixForInterlacedProcessing((FilterActivation *)fa, FALSE, mfd->bTFF, TRUE, 1, mfd->bInterlacedProgressiveDest);

				FixForInterlacedProcessing((FilterActivation *)fa, TRUE, !mfd->bTFF, FALSE, 1, mfd->bInterlacedProgressiveDest);
				RunProcPass1(fa, ff, FALSE);
				FixForInterlacedProcessing((FilterActivation *)fa, FALSE, !mfd->bTFF, FALSE, 1, mfd->bInterlacedProgressiveDest);
			}
			else
				RunProcPass1(fa, ff);
		}
		else
		{
			if(mfd->bInterlaced)
			{
				FixForInterlacedProcessing((FilterActivation *)fa, TRUE, mfd->bTFF, TRUE, 2, mfd->bInterlacedProgressiveDest);
				RunProcPass2(fa, ff, TRUE);
				FixForInterlacedProcessing((FilterActivation *)fa, FALSE, mfd->bTFF, TRUE, 2, mfd->bInterlacedProgressiveDest);

				FixForInterlacedProcessing((FilterActivation *)fa, TRUE, !mfd->bTFF, FALSE, 2, mfd->bInterlacedProgressiveDest);
				RunProcPass2(fa, ff, FALSE);
				FixForInterlacedProcessing((FilterActivation *)fa, FALSE, !mfd->bTFF, FALSE, 2, mfd->bInterlacedProgressiveDest);
			}
			else
				RunProcPass2(fa, ff);
		}
	}
	catch(CException* pe)
	{
		if(szError != NULL)
			pe->GetErrorMessage(szError, 200);

		pe->Delete();
	}

	mfd->bAllowProc = TRUE;

	if(szError == NULL)
		ff->ExceptOutOfMemory();
	else if(szError[0] != '\0')
		ff->Except(szError);
	else
		delete[] szError;
		
	return 0;
}

/// <summary>
/// Main processing of a frame (or interlaced field) during pass 1.
/// </summary>
/// <param name="fa">See VDub documentation.</param>
/// <param name="ff">See VDub documentation.</param>
/// <param name="bFirstField">Pass TRUE if this is the first field, chronologically, in case of interlaced video. Pass FALSE if 2nd field. </param>
void RunProcPass1(const FilterActivation *fa, const FilterFunctions *ff, BOOL bFirstField /*=TRUE*/)
{
	MyFilterData *mfd = (MyFilterData *)fa->filter_data;
	CArray<CVideoImage *, CVideoImage *> *paimgThis = (mfd->iFrameNo % 2) == 0 ? mfd->pimgScaled1 : mfd->pimgScaled2;
	CArray<CVideoImage *, CVideoImage *> *paimgPrev = (mfd->iFrameNo % 2) == 0 ? mfd->pimgScaled2 : mfd->pimgScaled1;

	BOOL bTopField = (!bFirstField && !mfd->bTFF || bFirstField && mfd->bTFF);

	Pixel32 *psrc = fa->src.data;
	CVideoImage *pimgFullPrev = paimgPrev->GetAt(0);
	CVideoImage *pimgFullThis = paimgThis->GetAt(0);
	int iIndexY = pimgFullThis->m_iHeight * pimgFullThis->m_iWidth;
	int iIndexX, iY, iX;

	// Copy current frame from VDub format into CVideoImage pimgFullThis to simplify processing.
	for(iY = 0; iY < fa->src.h; iY++)
	{
		iIndexY -= pimgFullThis->m_iWidth;
		iIndexX = iIndexY;
		for(iX = 0; iX < fa->src.w; iX++)
		{
			RGBPixel &pix = pimgFullThis->ElementAt(iIndexX++);
			pix.byIgnored = mfd->bUseColorMask && (*psrc & 0xffffff) == mfd->uiMaskColor ? 1 : 0;	// & 0xffffff is needed because it seems VirtualDub sometimes puts trash in the alhpa channel(?)
			pix.r = (*psrc >> 16) & 0xff;
			pix.g = (*psrc >> 8) & 0xff;
			pix.b = *psrc++ & 0xff;
			pix.r2 = (unsigned short)pix.r * pix.r;
			pix.g2 = (unsigned short)pix.g * pix.g;
			pix.b2 = (unsigned short)pix.b * pix.b;
		}
		psrc = (Pixel32 *)((char *)psrc + fa->src.modulo);
	}

	// Mark areas to ignore in pimgFullThis
	if(mfd->bIgnoreOutside || mfd->bIgnoreInside)
	{
		if(mfd->bIgnoreOutside && mfd->bFollowIgnoreOutside)
		{
			if(fa->pfsi->lCurrentSourceFrame < mfd->aprectIgnoreOutside->GetSize() && mfd->aprectIgnoreOutside->GetAt(fa->pfsi->lCurrentSourceFrame) != NULL)
				mfd->rectIgnoreOutside = *mfd->aprectIgnoreOutside->GetAt(fa->pfsi->lCurrentSourceFrame);
			else
				mfd->aprectIgnoreOutside->SetAtGrow(fa->pfsi->lCurrentSourceFrame, new CRectDbl(mfd->rectIgnoreOutside));
		}

		CRect rectIgnoreOutside = mfd->rectIgnoreOutside;
		rectIgnoreOutside.right = fa->src.w - 1 - rectIgnoreOutside.right;
		rectIgnoreOutside.bottom = fa->src.h - 1 - rectIgnoreOutside.bottom;
		CRect rectIgnoreInside = mfd->rectIgnoreInside;
		rectIgnoreInside.right = fa->src.w - 1 - rectIgnoreInside.right;
		rectIgnoreInside.bottom = fa->src.h - 1 - rectIgnoreInside.bottom;
		iIndexY = 0;
		for(iY = 0; iY < fa->src.h; iY++)
		{
			iIndexX = iIndexY;
			for(iX = 0; iX < fa->src.w; iX++)
			{
				if(mfd->bIgnoreOutside && (iY < rectIgnoreOutside.top || iY > rectIgnoreOutside.bottom || iX < rectIgnoreOutside.left || iX > rectIgnoreOutside.right) ||
					mfd->bIgnoreInside && (iY >= rectIgnoreInside.top && iY <= rectIgnoreInside.bottom && iX >= rectIgnoreInside.left && iX <= rectIgnoreInside.right))
					pimgFullThis->ElementAt(iIndexX).byIgnored = 1;
				iIndexX++;
			}
			psrc = (Pixel32 *)((char *)psrc + fa->src.modulo);
			iIndexY += pimgFullThis->m_iWidth;
		}
	}

	// Make smaller (downscaled by 2, 4, 8 etc) copies of current frame.
	for(int i = 1; i < paimgThis->GetSize(); i++)
		paimgThis->GetAt(i - 1)->DownscaleBy2(*paimgThis->GetAt(i));

	// Match frame to previous frame.
	if(mfd->iFrameNo > 0 && fa->pfsi->lCurrentSourceFrame == mfd->iPrevSourceFrame + 1)
	{
		double dfShiftDiff;
		int iScale = (int)paimgThis->GetSize() - 1;
		int iMatchBlocksX, iMatchBlocksY, iMatchBlocksXPrevScale, iMatchBlocksYPrevScale, iBlockY, iBlockX;
		int iBlockIndex, iValidBlock;
		double *adfXShifts, *adfYShifts;
		iMatchBlocksXPrevScale = 1;
		iMatchBlocksYPrevScale = 1;
		CVideoImage *pimgThis = paimgThis->GetAt(iScale);
		CVideoImage *pimgPrev = paimgPrev->GetAt(iScale);
		double *adfXShiftsPrevScale = new double[iMatchBlocksXPrevScale * iMatchBlocksYPrevScale];
		double *adfYShiftsPrevScale = new double[iMatchBlocksXPrevScale * iMatchBlocksYPrevScale];

		// Find shift for initial (most reduced) image.
		double dfSceneDetectValue, dfSceneDetectValueAdaptive;
 		BOOL bBlankFrame = FALSE;
		int iIntensityDiff;
		if(!pimgThis->FindShift(*pimgPrev, pimgThis->m_iWidth * mfd->iInitialSearchRange / 100, pimgThis->m_iHeight * mfd->iInitialSearchRange / 100, pimgThis->m_iWidth, pimgThis->m_iHeight, 0, 0, 0, 0, -1.0, 0.0, 1, adfXShiftsPrevScale[0], adfYShiftsPrevScale[0], 0, 1, &dfSceneDetectValue, &iIntensityDiff))
		{
			// We normally shoudn't get here, because we the initial match regardless of how bad it is.
			adfXShiftsPrevScale[0] = INVALID_VALUE;
			adfYShiftsPrevScale[0] = INVALID_VALUE;
			dfSceneDetectValue = 100.0;
			dfSceneDetectValueAdaptive = 1000.0;
		}
		else
		{
			if (iIntensityDiff < 4)
				bBlankFrame = TRUE;

			// Do some scene detection calculation.
			dfSceneDetectValue = (1.0 - dfSceneDetectValue) * 49.9999 + 0.0001;	// Turn -1.0 to 1.0 range into appr. 100 to 0 range (extreme values excluded), where 0 is a good match, and use it as detector for new scene.

			double dfGuessedSceneDetectValue = 0.0;
			double dfPrevSceneDetectValue = 0.0;
			double dfPrevPrevSceneDetectValue = 0.0;
			// Go through previous 5 frames and check their scene detect values. Use max value of those as "guess value" for this frame.
			for(int iPrevFrame = fa->pfsi->lCurrentSourceFrame - 1; iPrevFrame >= fa->pfsi->lCurrentSourceFrame - 5 && iPrevFrame > 0; iPrevFrame--)
			{
				if(mfd->adfSceneDetectValues->GetSize() > iPrevFrame)
				{
					double dfSceneDetectValueTmp = mfd->adfSceneDetectValues->GetAt(iPrevFrame);
					if(dfSceneDetectValueTmp >= 0)
					{
						if(dfSceneDetectValueTmp < 0.08)
							dfSceneDetectValueTmp = 0.08;

						if(dfGuessedSceneDetectValue < dfSceneDetectValueTmp)
							dfGuessedSceneDetectValue = dfSceneDetectValueTmp;
						if(iPrevFrame == fa->pfsi->lCurrentSourceFrame - 1)
							dfPrevSceneDetectValue = dfSceneDetectValueTmp;
						else if(iPrevFrame == fa->pfsi->lCurrentSourceFrame - 2)
							dfPrevPrevSceneDetectValue = dfSceneDetectValueTmp;
					}
					else
						break;
				}
			}

			// Also, extrapolate values linearly from previous two frames. If that value is larger than the current "guess value", use that value as guess instead.
			if(dfPrevSceneDetectValue > 0.0 && dfPrevPrevSceneDetectValue > 0.0)
			{
				double dfExtrapolatedSceneDetectValue = 2 * dfPrevSceneDetectValue - dfPrevPrevSceneDetectValue;
				if(dfGuessedSceneDetectValue < dfExtrapolatedSceneDetectValue)
					dfGuessedSceneDetectValue = dfExtrapolatedSceneDetectValue;
			}

			// Calculate the adaptive scene detect value, which will be compared to the scene detect threshold later.
			// There's no real science behind this formula, but it seems to work pretty well.
			dfSceneDetectValueAdaptive = dfSceneDetectValue * dfSceneDetectValue;
			if(dfGuessedSceneDetectValue > 0.0)
				dfSceneDetectValueAdaptive /= dfGuessedSceneDetectValue;

			dfSceneDetectValueAdaptive = sqrt(dfSceneDetectValueAdaptive * 8.0);	// The multiplication by 8 is just to get dfSceneDetectValueAdaptive in the 0-1000 range.
		}

		int iTotBlocks = 0;
		int iValidBlocks = 0;
		int iOkMatches = 0;
		RGlobalMotionInfo gmBest, gmBest1, gmBest2;
		BOOL *abUseBlock = NULL;
		BOOL bUsedDeepAnalysis = FALSE;

		// Find shifts for blocks in each scale.
		RFindShiftsWorkData wd;
		wd.mfd = mfd;
		while(--iScale >= 0)
		{
			pimgThis = paimgThis->GetAt(iScale);
			pimgPrev = paimgPrev->GetAt(iScale);
			iMatchBlocksX = pimgThis->m_iWidth / mfd->iBlockSize;
			iMatchBlocksY = pimgThis->m_iHeight / mfd->iBlockSize;
			ASSERT(iMatchBlocksX > 0 && iMatchBlocksY > 0);

			adfXShifts = new double[iMatchBlocksX * iMatchBlocksY];
			adfYShifts = new double[iMatchBlocksX * iMatchBlocksY];

			// Fill in data for worker threads.
			wd.pimgThis = pimgThis;
			wd.pimgPrev = pimgPrev;
			wd.iScale = iScale;
			wd.iMatchBlocksX = iMatchBlocksX;
			wd.iMatchBlocksY = iMatchBlocksY;
			wd.iMatchBlocksXPrevScale = iMatchBlocksXPrevScale;
			wd.iMatchBlocksYPrevScale = iMatchBlocksYPrevScale;
			wd.adfXShifts = adfXShifts;
			wd.adfYShifts = adfYShifts;
			wd.adfXShiftsPrevScale = adfXShiftsPrevScale;
			wd.adfYShiftsPrevScale = adfYShiftsPrevScale;
			wd.iNextItemToProcess = 0;
			wd.iTotalItemsToProcess = iMatchBlocksX * iMatchBlocksY;

			// Find the best match for all blocks. Multithreaded to allow for higher speed on multi-cpu computers.
			int iThreads = min(g_iCPUs, wd.iTotalItemsToProcess);
			CWinThread **apthreads = new CWinThread*[iThreads];
			HANDLE *ahthreads = new HANDLE[iThreads];
			for(int i = 0; i < iThreads; i++)
			{
				apthreads[i] = AfxBeginThread(FindShiftsThread, &wd, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
				apthreads[i]->m_bAutoDelete = FALSE;
				ahthreads[i] = apthreads[i]->m_hThread;
				apthreads[i]->ResumeThread();
			}
			WaitForMultipleObjects(iThreads, ahthreads, TRUE, INFINITE);
			for(int i = 0; i < iThreads; i++)
				delete apthreads[i];
			delete[] ahthreads;
			delete[] apthreads;

			delete[] adfXShiftsPrevScale;
			delete[] adfYShiftsPrevScale;
			adfXShiftsPrevScale = adfXShifts;
			adfYShiftsPrevScale = adfYShifts;
			iMatchBlocksXPrevScale = iMatchBlocksX;
			iMatchBlocksYPrevScale = iMatchBlocksY;
		}

		iTotBlocks = iMatchBlocksX * iMatchBlocksY;

		// Double Y shifts if frames are interlaced.
		if(mfd->bInterlaced)
		{
			double dfYShiftAdd = bTopField ? 1.0 : -1.0;
			for(iBlockIndex = 0; iBlockIndex < iTotBlocks; iBlockIndex++)
				if(adfYShifts[iBlockIndex] != INVALID_VALUE)
					adfYShifts[iBlockIndex] = 2.0 * adfYShifts[iBlockIndex] + dfYShiftAdd;
		}

		if(iMatchBlocksX != mfd->iUsedBlocksX || iMatchBlocksY != mfd->iUsedBlocksY)
		{	// Remembered discarded blocks have wrong dimension. Clear all remembered used blocks and start fresh. (Would be possible to "transform" the info, though.)
			for(int i = 0; i < mfd->apbyPrevFramesUsedBlocks->GetSize(); i++)
				delete[] mfd->apbyPrevFramesUsedBlocks->GetAt(i);
			mfd->apbyPrevFramesUsedBlocks->SetSize(0);
			mfd->iUsedBlocksX = iMatchBlocksX;
			mfd->iUsedBlocksY = iMatchBlocksY;
		}

		// Build an array, aiValidBlocks, with all blocks that got a successful match (to be able to iterate through them faster).
		// Also update the abUseBlock array with a bool for each block saying if it's good (white) or not.
		// Currently all valid ones are good unless bRememberDiscardedAreas is true, then initially don't use blocks that were ignored in the previous frame.
		abUseBlock = new BOOL[iTotBlocks];
		CArray<int, int> aiValidBlocks;
		BOOL bValidBlock, bOkMatch;
		iOkMatches = 0;
		BOOL bUseRememberedAreasForThisFrame = mfd->bRememberDiscardedAreas && mfd->iPrevSourceFrame < mfd->apbyPrevFramesUsedBlocks->GetSize() && mfd->apbyPrevFramesUsedBlocks->GetAt(mfd->iPrevSourceFrame) != NULL;
		for(iBlockIndex = 0; iBlockIndex < iTotBlocks; iBlockIndex++)
		{
			bValidBlock = adfXShifts[iBlockIndex] != INVALID_VALUE &&
							adfXShifts[iBlockIndex] * adfXShifts[iBlockIndex] + adfYShifts[iBlockIndex] * adfYShifts[iBlockIndex] <= mfd->dfAbsPixelsToDiscardBlock * mfd->dfAbsPixelsToDiscardBlock;
			bOkMatch = bValidBlock && (!bUseRememberedAreasForThisFrame || (mfd->apbyPrevFramesUsedBlocks->GetAt(mfd->iPrevSourceFrame)[iBlockIndex / 8] & (1 << (iBlockIndex % 8))) > 0);

			if(bValidBlock)
				aiValidBlocks.Add(iBlockIndex);
			abUseBlock[iBlockIndex] = bOkMatch;
			if(bOkMatch)
				iOkMatches++;
		}

		iValidBlocks = (int)aiValidBlocks.GetSize();
		int iStartBlock = mfd->bInterlaced && !bTopField ? iTotBlocks : 0;

		if(iValidBlocks > 0)
		{
			// This block of code determines which motion vectors don't fit with the general motion and marks them bad (red).
			// It uses an equation system to find the xpan, ypan, rotation and zoom that fit all block shifts best.
			double *adfAb;
			if(mfd->bERS)
			{
				// Rolling shutter video needs to calculate two sets of general motion parameters. One for the first line and one for the last. Everything in between is interpolated between these two.
				adfAb = new double[(2 * 4 + 1) * (2 * iValidBlocks)];
				int iMatrixItem = 0;
				for(iValidBlock = 0; iValidBlock < iValidBlocks; iValidBlock++)
				{
					iBlockIndex = aiValidBlocks[iValidBlock];
					double dfStartFactor = 1.0 - (iBlockIndex / iMatchBlocksX + 0.5) * mfd->iBlockSize / pimgFullThis->m_iHeight; // Factor that goes from 1.0 for first line to 0 for last line of frame.

					// First equation for block states how the block should shift in X direction based on the four unknowns (xpan, ypan, rotation, zoom)
					adfAb[iMatrixItem++] = dfStartFactor;		// xpan for first line
					adfAb[iMatrixItem++] = 0.0;		// ypan for first line
					adfAb[iMatrixItem++] = dfStartFactor * mfd->adfDistTimesCosOfBlockAngle[iStartBlock + iBlockIndex];		// rotation for first line (given as no. of pixels in rotational direction (ie around center))
					adfAb[iMatrixItem++] = dfStartFactor * mfd->adfDistTimesSinOfBlockAngle[iStartBlock + iBlockIndex];		// zoom for first line (given as no. of pixels in zoom direction (ie out from center))
					adfAb[iMatrixItem++] = (1.0 - dfStartFactor);		// xpan for last line
					adfAb[iMatrixItem++] = 0.0;		// ypan for last line
					adfAb[iMatrixItem++] = (1.0 - dfStartFactor) * mfd->adfDistTimesCosOfBlockAngle[iStartBlock + iBlockIndex];		// rotation for last line (given as no. of pixels in rotational direction (ie around center))
					adfAb[iMatrixItem++] = (1.0 - dfStartFactor) * mfd->adfDistTimesSinOfBlockAngle[iStartBlock + iBlockIndex];		// zoom for last line (given as no. of pixels in zoom direction (ie out from center))
					adfAb[iMatrixItem++] = adfXShifts[iBlockIndex];		// equals this shift

					// Second equation for block states how the block should shift in Y direction based on the four unknowns (xpan, ypan, rotation, zoom)
					adfAb[iMatrixItem++] = 0.0;		// xpan for first line
					adfAb[iMatrixItem++] = dfStartFactor;		// ypan for first line
					adfAb[iMatrixItem++] = dfStartFactor * mfd->adfDistTimesSinOfBlockAngle[iStartBlock + iBlockIndex];		// rotation for first line (given as no. of pixels in rotational direction (ie around center))
					adfAb[iMatrixItem++] = -dfStartFactor * mfd->adfDistTimesCosOfBlockAngle[iStartBlock + iBlockIndex];	// zoom for first line (given as no. of pixels in zoom direction (ie out from center))
					adfAb[iMatrixItem++] = 0.0;		// xpan for last line
					adfAb[iMatrixItem++] = (1.0 - dfStartFactor);		// ypan for last line
					adfAb[iMatrixItem++] = (1.0 - dfStartFactor) * mfd->adfDistTimesSinOfBlockAngle[iStartBlock + iBlockIndex];		// rotation for last line (given as no. of pixels in rotational direction (ie around center))
					adfAb[iMatrixItem++] = -(1.0 - dfStartFactor) * mfd->adfDistTimesCosOfBlockAngle[iStartBlock + iBlockIndex];	// zoom for last line (given as no. of pixels in zoom direction (ie out from center))
					adfAb[iMatrixItem++] = adfYShifts[iBlockIndex] / mfd->dfPixelAspectSrc;		// equals this shift
				}
			}
			else
			{
				// Same as above, but for global shutter.
				adfAb = new double[(4 + 1) * (2 * iValidBlocks)];
				int iMatrixItem = 0;
				for(iValidBlock = 0; iValidBlock < iValidBlocks; iValidBlock++)
				{
					iBlockIndex = aiValidBlocks[iValidBlock];

					// First equation for block states how the block should shift in X direction based on the four unknowns (xpan, ypan, rotation, zoom)
					adfAb[iMatrixItem++] = 1.0;		// xpan
					adfAb[iMatrixItem++] = 0.0;		// ypan
					adfAb[iMatrixItem++] = mfd->adfDistTimesCosOfBlockAngle[iStartBlock + iBlockIndex];		// rotation (given as no. of pixels in rotational direction (ie around center))
					adfAb[iMatrixItem++] = mfd->adfDistTimesSinOfBlockAngle[iStartBlock + iBlockIndex];		// zoom (given as no. of pixels in zoom direction (ie out from center))
					adfAb[iMatrixItem++] = adfXShifts[iBlockIndex];		// equals this shift

					// Second equation for block states how the block should shift in Y direction based on the four unknowns (xpan, ypan, rotation, zoom)
					adfAb[iMatrixItem++] = 0.0;		// xpan
					adfAb[iMatrixItem++] = 1.0;		// ypan
					adfAb[iMatrixItem++] = mfd->adfDistTimesSinOfBlockAngle[iStartBlock + iBlockIndex];		// rotation (given as no. of pixels in rotational direction (ie around center))
					adfAb[iMatrixItem++] = -mfd->adfDistTimesCosOfBlockAngle[iStartBlock + iBlockIndex];	// zoom (given as no. of pixels in zoom direction (ie out from center))
					adfAb[iMatrixItem++] = adfYShifts[iBlockIndex] / mfd->dfPixelAspectSrc;		// equals this shift
				}
			}

			for(int iRetry = 0; iRetry <= 1; iRetry++)
			{
				// Calculate overall xpan, ypan, rotation and zoom and remove the block with shift that deviates the most from that.
				CalcGlobalMotion(aiValidBlocks, iStartBlock, iMatchBlocksX, iMatchBlocksY, pimgFullThis->m_iHeight, adfAb, adfXShifts, adfYShifts, mfd, abUseBlock, mfd->bERS, gmBest1, gmBest2, mfd->bDetectRotation, mfd->bDetectZoom);
				if(mfd->bERS)
				{
					// Set gmBest to the global motion of the middle line for rolling shutter.
					gmBest.dfShiftX = (gmBest1.dfShiftX + gmBest2.dfShiftX) / 2.0;
					gmBest.dfShiftY = (gmBest1.dfShiftY + gmBest2.dfShiftY) / 2.0;
					gmBest.dfRotation = (gmBest1.dfRotation + gmBest2.dfRotation) / 2.0;
					gmBest.dfZoom = (gmBest1.dfZoom + gmBest2.dfZoom) / 2.0;
				}
				else
					gmBest = gmBest1;

				iOkMatches = 0;
				for(iValidBlock = 0; iValidBlock < iValidBlocks; iValidBlock++)
				{
					if(abUseBlock[aiValidBlocks[iValidBlock]])
						iOkMatches++;
				}

				if(iRetry == 0 && mfd->bRememberDiscardedAreas && iOkMatches < mfd->dfSkipFrameBlockPercent * 0.5 * iTotBlocks / 100)
				{
					// Too few blocks are ok, but since we didn't start by considering all valid blocks being ok (due to bRememberDiscardedBlocks being TRUE), try again with all valid blocks being ok.
					// But don't do this until the number of ok matches is below halv of threshold for skipped frame, because it's usually better to get skipped frames now and hope that things get better in future frames, than to have the ok vectors jump to a new object suddenly.
					bUseRememberedAreasForThisFrame = FALSE;
					for(iBlockIndex = 0; iBlockIndex < iTotBlocks; iBlockIndex++)
					{
						abUseBlock[iBlockIndex] = adfXShifts[iBlockIndex] != INVALID_VALUE &&
										adfXShifts[iBlockIndex] * adfXShifts[iBlockIndex] + adfYShifts[iBlockIndex] * adfYShifts[iBlockIndex] <= mfd->dfAbsPixelsToDiscardBlock * mfd->dfAbsPixelsToDiscardBlock;
					}
				}
				else
					break;
			}

			// If number of ok matches is low enough, do a deep analysis. New feature in version 1.7
			// It divides the "global motion space", the four dimensional space with all reasonable values of xpan, ypan, rotation and zoom, into a grid.
			// Each grid box is repeatedly subdivided into 16 new smaller grid boxes until we find the grid box with the global motion parameters that fit more of the motion vectors than any other grid box.
			// New in version 3.0 is that we never use deep analysis if we are using remembered areas from previuos frame, becuase it's probably better to keep tracking that same object.
			if(!mfd->bERS && !bUseRememberedAreasForThisFrame && iOkMatches < mfd->iDeepAnalysisKickIn * iValidBlocks / 100)
			{
				bUsedDeepAnalysis = TRUE;
				RGlobalMotionInfo gmNew;
				int iBlocksInGridBest = -1;
				// This is a slow process, so we start by only dividing grid boxes that contain a certain amount (iMinMatches) of motion vectors.
				// If there is no tiny grid box left in the end of all this, divide iMinMatches by 2 and try again etc.
				for(int iMinMatches = iValidBlocks / 2; iMinMatches >= iValidBlocks / 16 && iMinMatches >= 8 && iBlocksInGridBest == -1; iMinMatches /= 2)
				{
					CList<RGlobalMotionInfo *, RGlobalMotionInfo *> lstgm1, lstgm2, *plstgmThisGridSize, *plstgmPrevGridSize;
					plstgmThisGridSize = &lstgm1;
					plstgmPrevGridSize = &lstgm2;
					RGlobalMotionInfo *pgm = new RGlobalMotionInfo;
					pgm->dfShiftX = 0.0;
					pgm->dfShiftY = 0.0;
					pgm->dfRotation = 0.0;
					pgm->dfZoom = 0.0;
					plstgmPrevGridSize->AddTail(pgm);

					int iMaxGridSize = max(pimgFullThis->m_iWidth, pimgFullThis->m_iHeight);
					double dfGridSize = mfd->dfPixelsOffToDiscardBlock;
					int iDividings = 0;
					while(dfGridSize < iMaxGridSize * 2)
					{
						dfGridSize *= 2.0;
						iDividings++;
					}

					// The following loop divides all current interesting 4D-grid boxes into 16 as many smaller ones. (You get 16 if you divide a 4D "grid box" in half in all dimensions.)
					while(--iDividings >= 0)
					{
						dfGridSize /= 2.0;
						POSITION pos = plstgmPrevGridSize->GetHeadPosition();
						while(pos != NULL)
						{
							pgm = plstgmPrevGridSize->GetNext(pos);
							for(int iGridShiftX = 0; iGridShiftX < 2; iGridShiftX++)
							{
								gmNew.dfShiftX = pgm->dfShiftX + (iGridShiftX - 0.5) * dfGridSize;
								for(int iGridShiftY = 0; iGridShiftY < 2; iGridShiftY++)
								{
									gmNew.dfShiftY = pgm->dfShiftY + (iGridShiftY - 0.5) * dfGridSize;
									for(int iGridRotation = 0; iGridRotation < (mfd->bDetectRotation ? 2 : 1); iGridRotation++)
									{
										gmNew.dfRotation = mfd->bDetectRotation ? (pgm->dfRotation + (iGridRotation - 0.5) * dfGridSize / iMaxGridSize) : 0;
										for(int iGridZoom = 0; iGridZoom < (mfd->bDetectZoom ? 2 : 1); iGridZoom++)
										{
											gmNew.dfZoom = mfd->bDetectZoom ? (pgm->dfZoom + (iGridZoom - 0.5) * dfGridSize / iMaxGridSize) : 0;
											// Now we have a smaller grid box with corresponding global motion parameters (xpan, ypan, rotation, zoom). See how many motion vectors that agree with this gm.
											int iBlocksInGrid = 0;
											for(iValidBlock = 0; iValidBlock < iValidBlocks && (iBlocksInGrid < iMinMatches || iDividings == 0); iValidBlock++)
											{
												iBlockIndex = aiValidBlocks[iValidBlock];
												dfShiftDiff = CalcBlockShiftDiffFromGlobalMotion(gmNew, adfXShifts[iBlockIndex], adfYShifts[iBlockIndex], mfd, iStartBlock + iBlockIndex);
												if(dfShiftDiff < dfGridSize * dfGridSize)
													iBlocksInGrid++;
											}

											// If enough blocks are within the 4D grid box add it to a list to subdivide it even further later.
											// Or if it's the last subdividing, just remember the gm parameters that has most blocks.
											if(iBlocksInGrid >= iMinMatches)
											{
												if(iDividings > 0)
												{
													RGlobalMotionInfo *pgmNew = new RGlobalMotionInfo;
													*pgmNew = gmNew;
													plstgmThisGridSize->AddTail(pgmNew);
												}
												else if(iBlocksInGrid > iBlocksInGridBest)
												{
													gmBest = gmNew;
													iBlocksInGridBest = iBlocksInGrid;
												}
											}
										}
									}
								}
							}
							delete pgm;
						}

						// Swap lists.
						plstgmPrevGridSize->RemoveAll();
						plstgmPrevGridSize = plstgmThisGridSize;
						plstgmThisGridSize = plstgmPrevGridSize == &lstgm1 ? &lstgm2 : &lstgm1;
					}
				}

				if(iBlocksInGridBest >= 8)
				{
					// We successfully found a nice 4D grid box. Update abUseBlock array to contain only the blocks that fit within the 4D grid box we found.
					for(iBlockIndex = 0; iBlockIndex < iTotBlocks; iBlockIndex++)
						abUseBlock[iBlockIndex] = adfXShifts[iBlockIndex] != INVALID_VALUE && CalcBlockShiftDiffFromGlobalMotion(gmBest, adfXShifts[iBlockIndex], adfYShifts[iBlockIndex], mfd, iStartBlock + iBlockIndex) < mfd->dfPixelsOffToDiscardBlock * mfd->dfPixelsOffToDiscardBlock;
				}

				// Finetune the gm parameters and which blocks to use (abUseBlock) with the same old gm finding algorithm as in version <= 1.6.
				// With the better abUseBlock we have now, it should find the gm without problem.
				CalcGlobalMotion(aiValidBlocks, iStartBlock, iMatchBlocksX, iMatchBlocksY, pimgFullThis->m_iHeight, adfAb, adfXShifts, adfYShifts, mfd, abUseBlock, mfd->bERS, gmBest, gmBest2, mfd->bDetectRotation, mfd->bDetectZoom);
			}

			delete[] adfAb;

			iOkMatches = 0;
			for(iValidBlock = 0; iValidBlock < iValidBlocks; iValidBlock++)
			{
				if(abUseBlock[aiValidBlocks[iValidBlock]])
					iOkMatches++;
			}

			int iOkMatchesTmp = 0;
			for(iBlockIndex = 0; iBlockIndex < iTotBlocks; iBlockIndex++)
			{
				if(abUseBlock[iBlockIndex])
					iOkMatchesTmp++;
			}
		}

		BOOL bNewScene = FALSE;
		BOOL bFrameSkipped = FALSE;
		if(iValidBlocks == 0 || iOkMatches < mfd->dfSkipFrameBlockPercent * iTotBlocks / 100)
		{
			// Too few ok blocks -> assume no shift etc.
			bFrameSkipped = TRUE;
			gmBest.dfShiftX = 0.0;
			gmBest.dfShiftY = 0.0;
			gmBest.dfRotation = 0.0;
			gmBest.dfZoom = 0.0;
			if(mfd->bERS)
			{
				gmBest1 = gmBest;
				gmBest2 = gmBest;
			}

			// It's a new scene only if a frame is skipped due to too few ok blocks AND new scene detection is enabled AND it's a blank frame or the adaptive scene detect value is larger than the new scene threshold.
			bNewScene = mfd->bDetectScenes && (bBlankFrame || dfSceneDetectValueAdaptive > mfd->dfNewSceneThreshold);
		}

		// Remember the (non-adaptive) scene detect value if it's not a new scene; otherwise -1.
		mfd->adfSceneDetectValues->SetAtGrow(fa->pfsi->lCurrentSourceFrame, bNewScene ? -1.0 : dfSceneDetectValue);

		// Calculate the real zoom factor and rotation angle based on the pixel-based ones already calculated.
		double dfRealBestZoomTmp = sqrt(gmBest.dfRotation * gmBest.dfRotation + (1.0 + gmBest.dfZoom) * (1.0 + gmBest.dfZoom));
		double dfTmp1 = (1 + dfRealBestZoomTmp * dfRealBestZoomTmp - gmBest.dfRotation * gmBest.dfRotation - gmBest.dfZoom * gmBest.dfZoom) / (2.0 * dfRealBestZoomTmp);
		double dfRealBestRotationTmp = dfTmp1 < 1.0 ? (acos(dfTmp1) * 180.0 / PI) : 0.0;	// dfTmp1 can apparently get very slightly larger than 1 due to rounding errors. Fixed after v1.8.1.
		if(gmBest.dfRotation < 0.0)
			dfRealBestRotationTmp = -dfRealBestRotationTmp;
		gmBest.dfRotation = dfRealBestRotationTmp;
		gmBest.dfZoom = dfRealBestZoomTmp;

		if(mfd->bERS)
		{
			dfRealBestZoomTmp = sqrt(gmBest1.dfRotation * gmBest1.dfRotation + (1.0 + gmBest1.dfZoom) * (1.0 + gmBest1.dfZoom));
			dfTmp1 = (1 + dfRealBestZoomTmp * dfRealBestZoomTmp - gmBest1.dfRotation * gmBest1.dfRotation - gmBest1.dfZoom * gmBest1.dfZoom) / (2.0 * dfRealBestZoomTmp);
			dfRealBestRotationTmp = dfTmp1 < 1.0 ? (acos(dfTmp1) * 180.0 / PI) : 0.0;
			if(gmBest1.dfRotation < 0.0)
				dfRealBestRotationTmp = -dfRealBestRotationTmp;
			gmBest1.dfRotation = dfRealBestRotationTmp;
			gmBest1.dfZoom = dfRealBestZoomTmp;

			dfRealBestZoomTmp = sqrt(gmBest2.dfRotation * gmBest2.dfRotation + (1.0 + gmBest2.dfZoom) * (1.0 + gmBest2.dfZoom));
			dfTmp1 = (1 + dfRealBestZoomTmp * dfRealBestZoomTmp - gmBest2.dfRotation * gmBest2.dfRotation - gmBest2.dfZoom * gmBest2.dfZoom) / (2.0 * dfRealBestZoomTmp);
			dfRealBestRotationTmp = dfTmp1 < 1.0 ? (acos(dfTmp1) * 180.0 / PI) : 0.0;
			if(gmBest2.dfRotation < 0.0)
				dfRealBestRotationTmp = -dfRealBestRotationTmp;
			gmBest2.dfRotation = dfRealBestRotationTmp;
			gmBest2.dfZoom = dfRealBestZoomTmp;
		}

		if(mfd->iVideoOutput != VIDEOOUTPUT_NONE)
		{
			// Copy previous frame to VDub output.
			Pixel32 *pdst = fa->dst.data;
			iIndexY = (pimgFullPrev->m_iHeight + 2 * mfd->iTextHeight) * pimgFullPrev->m_iWidth;
			for(iY = fa->src.h + 2 * mfd->iTextHeight - 1; iY >= 0; iY--)
			{
				iIndexY -= pimgFullPrev->m_iWidth;
				iIndexX = iIndexY;
				for(int iX = 0; iX < fa->src.w; iX++)
				{
					if(iY < fa->src.h)
					{
						const RGBPixel &pix = pimgFullPrev->ElementAt(iIndexX);
						const RGBPixel &pixNext = pimgFullThis->ElementAt(iIndexX++);
						int iBrightnessPercent = 75;
						if(pix.byIgnored > 0)
							iBrightnessPercent = iBrightnessPercent * 3 / 4;
						if(pixNext.byIgnored > 0)
							iBrightnessPercent = iBrightnessPercent * 3 / 4;

						// Copy pixel and make it a little darker.
						if(pimgFullPrev->m_bRGB)
							*pdst++ = (pix.r * iBrightnessPercent / 100 * 0x010000) + (pix.g * iBrightnessPercent / 100 * 0x0100) + (pix.b * iBrightnessPercent / 100);
						else
							*pdst++ = (pix.g * iBrightnessPercent / 100 * 0x010000) + (pix.g * iBrightnessPercent / 100 * 0x0100) + (pix.g * iBrightnessPercent / 100);
					}
					else
					{
						*pdst++ = 0x000000;
						iIndexX++;
					}
				}
				pdst = (Pixel32 *)((char *)pdst + fa->dst.modulo);
			}

			if(fa->dst.hdc != NULL)
			{
				CDC *pdc = CDC::FromHandle((HDC)fa->dst.hdc);
				int iYStart = mfd->bInterlaced && !bFirstField ? fa->dst.h : 0;

				// Draw motion vectors for bad blocks (red).
				CPen *ppenOld = pdc->SelectObject(mfd->ppenVectorBad);
				double dfVectorLengthFactor = mfd->iVideoOutput == VIDEOOUTPUT_MOTION_VECTORS_5X ? 5.0 : (mfd->iVideoOutput == VIDEOOUTPUT_MOTION_VECTORS_2X ? 2.0 : 1.0);
				iBlockIndex = 0;
				for(iBlockY = 0; iBlockY < iMatchBlocksY; iBlockY++)
					for(iBlockX = 0; iBlockX < iMatchBlocksX; iBlockX++)
					{
						iX = iBlockX * mfd->iBlockSize + mfd->iBlockSize / 2;
						iY = iYStart + iBlockY * mfd->iBlockSize + mfd->iBlockSize / 2;
						if(adfXShifts[iBlockIndex] != INVALID_VALUE && !abUseBlock[iBlockIndex])
						{
							pdc->MoveTo(iX, iY);
							iX += (int)(floor(adfXShifts[iBlockIndex] * dfVectorLengthFactor + 0.5) + 0.5);
							iY += (int)(floor(adfYShifts[iBlockIndex] * dfVectorLengthFactor / (mfd->bInterlaced ? 2.0 : 1.0) + 0.5) + 0.5);
							pdc->LineTo(iX, iY);
							pdc->FillSolidRect(iX - 1, iY - 1, 3, 3, RGB(255, 0, 0));
						}
						iBlockIndex++;
					}

				// Draw motion vectors for good blocks (white).
				pdc->SelectObject(mfd->ppenVectorGood);
				iBlockIndex = 0;
				for(iBlockY = 0; iBlockY < iMatchBlocksY; iBlockY++)
					for(iBlockX = 0; iBlockX < iMatchBlocksX; iBlockX++)
					{
						iX = iBlockX * mfd->iBlockSize + mfd->iBlockSize / 2;
						iY = iYStart + iBlockY * mfd->iBlockSize + mfd->iBlockSize / 2;
						if(abUseBlock[iBlockIndex])
						{
							pdc->MoveTo(iX, iY);
							iX += (int)(floor(adfXShifts[iBlockIndex] * dfVectorLengthFactor + 0.5) + 0.5);
							iY += (int)(floor(adfYShifts[iBlockIndex] * dfVectorLengthFactor / (mfd->bInterlaced ? 2.0 : 1.0) + 0.5) + 0.5);
							pdc->LineTo(iX, iY);
							pdc->FillSolidRect(iX - 1, iY - 1, 3, 3, RGB(255, 255, 255));
						}
						iBlockIndex++;
					}

				pdc->SelectObject(ppenOld);

				// Draw text at bottom of output frame.
				CString strInfo1, strInfo2;
				COLORREF crBackColor;
				if(bFrameSkipped)
				{
					strInfo1 = "Skipped (too few ok blocks)";
					if(bNewScene)
					{
						strInfo1 += "   New scene";
						if(bBlankFrame)
							strInfo1 += " (blank frame)";
						else
							strInfo1 += " (above threshold)";
						crBackColor = RGB(255, 100, 100);
					}
					else
						crBackColor = RGB(255, 177, 100);
				}
				else
				{
					if(mfd->bERS)
						strInfo1.Format("Pan X: %.2lf - %.2lf   Pan Y: %.2lf - %.2lf   Rotate: %.3lf - %.3lf   Zoom: %.5lf - %.5lf", gmBest1.dfShiftX, gmBest2.dfShiftX, gmBest1.dfShiftY, gmBest2.dfShiftY, gmBest1.dfRotation, gmBest2.dfRotation, gmBest1.dfZoom, gmBest2.dfZoom);
					else
						strInfo1.Format("Pan X: %.2lf   Pan Y: %.2lf   Rotate: %.3lf   Zoom: %.5lf", gmBest.dfShiftX, gmBest.dfShiftY, gmBest.dfRotation, gmBest.dfZoom);

					crBackColor = RGB(194, 231, 194);
				}

				strInfo2.Format("Scene detection: %.1lf   Ok blocks: %.1lf%%%s", dfSceneDetectValueAdaptive, iOkMatches * 100.0 / iTotBlocks, bUsedDeepAnalysis ? "   Deep analysis" : "");

				pdc->FillSolidRect(0, iYStart + pimgFullPrev->m_iHeight, pimgFullPrev->m_iWidth, 2 * mfd->iTextHeight, crBackColor);
				pdc->SetBkMode(TRANSPARENT);
				CFont *pfontOld = pdc->SelectObject(mfd->pfontInfoText);
				pdc->TextOut(0, iYStart + pimgFullPrev->m_iHeight, strInfo1);
				pdc->TextOut(0, iYStart + pimgFullPrev->m_iHeight + mfd->iTextHeight, strInfo2);
				pdc->SelectObject(pfontOld);
				GdiFlush();
			}
		}

		if(mfd->bRememberDiscardedAreas)
		{	// Remember which blocks were used in this frame.
			if(bNewScene)
			{	// Nothing to remember if new scene.
				if(fa->pfsi->lCurrentSourceFrame < mfd->apbyPrevFramesUsedBlocks->GetSize())
				{
					delete[] mfd->apbyPrevFramesUsedBlocks->GetAt(fa->pfsi->lCurrentSourceFrame);
					mfd->apbyPrevFramesUsedBlocks->SetAt(fa->pfsi->lCurrentSourceFrame, NULL);
				}
			}
			else
			{
				BYTE *pbyPrevFrameUsedBlocks = NULL;
				if(fa->pfsi->lCurrentSourceFrame < mfd->apbyPrevFramesUsedBlocks->GetSize())
					pbyPrevFrameUsedBlocks = mfd->apbyPrevFramesUsedBlocks->GetAt(fa->pfsi->lCurrentSourceFrame);
				if(pbyPrevFrameUsedBlocks == NULL)
				{
					pbyPrevFrameUsedBlocks = new BYTE[(iTotBlocks + 7) / 8];
					mfd->apbyPrevFramesUsedBlocks->SetAtGrow(fa->pfsi->lCurrentSourceFrame, pbyPrevFrameUsedBlocks);
				}

				memset(pbyPrevFrameUsedBlocks, 0, (iTotBlocks + 7) / 8);
				for(iBlockIndex = 0; iBlockIndex < iTotBlocks; iBlockIndex++)
				{
					if(abUseBlock[iBlockIndex])
						pbyPrevFrameUsedBlocks[iBlockIndex / 8] |= 1 << (iBlockIndex % 8);
				}
			}
		}

		delete[] abUseBlock;

		if(mfd->hLog == INVALID_HANDLE_VALUE)
		{
			if(!mfd->bInterlaced || mfd->pstrFirstLogRow != NULL)
			{
				// Create or open logfile for writing.
				if(mfd->bLogFileAppend)
				{
					mfd->hLog = CreateFile(*mfd->pstrLogFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
					if(mfd->hLog != INVALID_HANDLE_VALUE)
					{
						SetFilePointer(mfd->hLog, 0, NULL, FILE_END);
						DWORD dw;
						WriteFile(mfd->hLog, "\r\n", 2, &dw, NULL);
					}
				}
				else
				{
					mfd->hLog = CreateFile(*mfd->pstrLogFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
				}

				if(mfd->hLog == INVALID_HANDLE_VALUE && !mfd->bCannotWriteToLogfileMsgWasShown)
				{
					AfxMessageBox("Cannot write to log file!");
					mfd->bCannotWriteToLogfileMsgWasShown = TRUE;
				}
			}
		}

		// Write stuff to logfile.
		CString strSourceFrame;
		if(mfd->bInterlaced)
			strSourceFrame.Format("%6li%c", fa->pfsi->lCurrentSourceFrame / 2, fa->pfsi->lCurrentSourceFrame % 2 == 0 ? 'A' : 'B');
		else
			strSourceFrame.Format("%7li", fa->pfsi->lCurrentSourceFrame);

		CString strLog;
		if(bFrameSkipped)
			strLog.Format("%s\t\t\t\t\t\t\t\t\tskipped\t%s\t#\t%7.2lf\t%7.2lf\t%s\r\n", strSourceFrame, bNewScene ? "n_scene" : "", dfSceneDetectValueAdaptive, iOkMatches * 100.0 / iTotBlocks, bBlankFrame ? "blank" : "");
		else if(mfd->bERS)
			strLog.Format("%s\t%7.2lf\t%7.2lf\t%7.3lf\t%7.5lf\t%7.2lf\t%7.2lf\t%7.3lf\t%7.5lf\t\t%s\t#\t%7.2lf\t%7.2lf\t%s\r\n", strSourceFrame, gmBest.dfShiftX, gmBest.dfShiftY, gmBest.dfRotation, gmBest.dfZoom, gmBest1.dfShiftX, gmBest1.dfShiftY, gmBest1.dfRotation, gmBest1.dfZoom, bNewScene ? "n_scene" : "", dfSceneDetectValueAdaptive, iOkMatches * 100.0 / iTotBlocks, bBlankFrame ? "blank" : "");
		else
			strLog.Format("%s\t%7.2lf\t%7.2lf\t%7.3lf\t%7.5lf\t\t\t\t\t\t%s\t#\t%7.2lf\t%7.2lf\t%s\r\n", strSourceFrame, gmBest.dfShiftX, gmBest.dfShiftY, gmBest.dfRotation, gmBest.dfZoom, bNewScene ? "n_scene" : "", dfSceneDetectValueAdaptive, iOkMatches * 100.0 / iTotBlocks, bBlankFrame ? "blank" : "");
		
		DWORD dw;
		if(mfd->hLog != INVALID_HANDLE_VALUE)
		{
			if(mfd->pstrFirstLogRow != NULL)
			{
				WriteFile(mfd->hLog, *mfd->pstrFirstLogRow, (DWORD)strlen(*mfd->pstrFirstLogRow), &dw, NULL);
				delete mfd->pstrFirstLogRow;
				mfd->pstrFirstLogRow = NULL;
			}
			WriteFile(mfd->hLog, strLog, strLog.GetLength(), &dw, NULL);
		}
		else
		{
			delete mfd->pstrFirstLogRow;	// Just in case
			mfd->pstrFirstLogRow = new CString((LPCSTR)strLog);
		}

		// Calculate new ignore outside area if it should follow matched motion.
		if(mfd->bIgnoreOutside && mfd->bFollowIgnoreOutside)
		{
			CRectDbl *prect;
			if(fa->pfsi->lCurrentSourceFrame + 1 >= mfd->aprectIgnoreOutside->GetSize() || (prect = mfd->aprectIgnoreOutside->GetAt(fa->pfsi->lCurrentSourceFrame + 1)) == NULL)
			{
				prect = new CRectDbl(0, 0, 0, 0);
				mfd->aprectIgnoreOutside->SetAtGrow(fa->pfsi->lCurrentSourceFrame + 1, prect);
			}

			double dfNewX, dfNewY;
			CVideoImage::VDubProjectSrcToDest(fa->src.w, fa->src.h, fa->src.w, fa->src.h, gmBest.dfRotation, gmBest.dfZoom, gmBest.dfShiftX, gmBest.dfShiftY, 1.0, mfd->dfPixelAspectSrc, mfd->dfPixelAspectSrc, mfd->bInterlaced, mfd->bInterlaced, bTopField, dfNewX, dfNewY, mfd->rectIgnoreOutside.left, fa->src.h - 1 - mfd->rectIgnoreOutside.top);
			prect->left = dfNewX;
			prect->top = fa->src.h - 1 - dfNewY;
			CVideoImage::VDubProjectSrcToDest(fa->src.w, fa->src.h, fa->src.w, fa->src.h, gmBest.dfRotation, gmBest.dfZoom, gmBest.dfShiftX, gmBest.dfShiftY, 1.0, mfd->dfPixelAspectSrc, mfd->dfPixelAspectSrc, mfd->bInterlaced, mfd->bInterlaced, bTopField, dfNewX, dfNewY, fa->src.w - 1 - mfd->rectIgnoreOutside.right, mfd->rectIgnoreOutside.bottom);
			prect->right = fa->src.w - 1 - dfNewX;
			prect->bottom = dfNewY;
		}

		delete[] adfXShiftsPrevScale;
		delete[] adfYShiftsPrevScale;
	}
	else
	{	// No previous frame to match on
		if(mfd->iVideoOutput != VIDEOOUTPUT_NONE)
		{
			Pixel32 *pdst = fa->dst.data;
			for(iY = fa->dst.h - 1; iY >= 0; iY--)
			{
				for(int iX = fa->dst.w - 1; iX >= 0; iX--)
					*pdst++ = 0x000000;
				pdst = (Pixel32 *)((char *)pdst + fa->dst.modulo);
			}

			int iYStart = mfd->bInterlaced && !bFirstField ? fa->dst.h : 0;
			if(fa->dst.hdc != NULL)
			{
				CDC *pdc = CDC::FromHandle((HDC)fa->dst.hdc);
				pdc->FillSolidRect(0, iYStart + pimgFullPrev->m_iHeight + mfd->iTextHeight, pimgFullPrev->m_iWidth, mfd->iTextHeight, RGB(255, 255, 100));
				pdc->SetBkMode(TRANSPARENT);
				CFont *pfontOld = pdc->SelectObject(mfd->pfontInfoText);
				pdc->TextOut(0, iYStart + pimgFullPrev->m_iHeight + mfd->iTextHeight, "No previous frame to match on");
				pdc->SelectObject(pfontOld);
				GdiFlush();
			}
		}

		delete mfd->pstrFirstLogRow;
		mfd->pstrFirstLogRow = NULL;
	}

	mfd->iPrevSourceFrame = fa->pfsi->lCurrentSourceFrame;
	mfd->iFrameNo++;
}

/// <summary>
/// Main processing of a frame (or interlaced field) during pass 2.
/// </summary>
/// <param name="fa">See VDub documentation.</param>
/// <param name="ff">See VDub documentation.</param>
/// <param name="bFirstField">Pass TRUE if this is the first field, chronologically, in case of interlaced video. Pass FALSE if 2nd field. </param>
void RunProcPass2(const FilterActivation *fa, const FilterFunctions *ff, BOOL bFirstField /*=TRUE*/)
{
	MyFilterData *mfd = (MyFilterData *)fa->filter_data;
	int iNewFrame = fa->pfsi->lCurrentSourceFrame - mfd->iStartFrame;	// The current source frame
	int iCurFrame = iNewFrame - (mfd->bUseFutureFrames ? mfd->iFutureFrames : 0);	// The current output frame (which can be older than current source frame if future frames are used)

	BOOL bTopField = (!bFirstField && !mfd->bTFF || bFirstField && mfd->bTFF);

	int iFrameHeight = fa->dst.h;
	if(mfd->bInterlaced)
		iFrameHeight *= 2;

	BOOL bFrameWasOutput = FALSE;
	if(mfd->iNoOfFrames >= 0 && iNewFrame >= 0 && iCurFrame < mfd->iNoOfFrames)
	{
		if(mfd->bUseOldFrames || mfd->bUseFutureFrames)
		{
			int iActiveRangeStart = iCurFrame - (mfd->bUseOldFrames ? mfd->iOldFrames : 0);
			if(iActiveRangeStart < 0)
				iActiveRangeStart = 0;
			int iActiveRangeEnd = iNewFrame;
			if(iActiveRangeEnd >= mfd->iNoOfFrames)
				iActiveRangeEnd = mfd->iNoOfFrames - 1;

			if(fa->pfsi->lCurrentSourceFrame != mfd->iPrevSourceFrame + 1)
			{	// User jumped. Free out-of-range images.
				for(int i = 0; i < iActiveRangeStart; i++)
					FreeOneFrameInfo(mfd->aFrameInfo, i, FALSE);

				for(int i = iActiveRangeEnd + 1; i < mfd->iNoOfFrames; i++)
					FreeOneFrameInfo(mfd->aFrameInfo, i, FALSE);
			}
			else
			{	// Sequential stepping. Free out-of-range images (faster than above)
				int i = iActiveRangeStart - 1;
				if(i >= 0 && i < mfd->iNoOfFrames)
					FreeOneFrameInfo(mfd->aFrameInfo, i, FALSE);
			}

			if(iNewFrame < mfd->iNoOfFrames)
			{
				FreeOneFrameInfo(mfd->aFrameInfo, iNewFrame, FALSE);
				
				// Copy new input frame to internal array.
				VBitmap *pvbmp = new VBitmap;
				*pvbmp = fa->src;
				if(mfd->bInterlaced)
				{
					pvbmp->data = (Pixel *)new BYTE[fa->src.size / 2];
					pvbmp->pitch /= 2;
					pvbmp->modulo -= pvbmp->pitch;
					BYTE *psrc = (BYTE *)fa->src.data;
					BYTE *pdest = (BYTE *)pvbmp->data;
					for(int i = 0; i < fa->src.h; i++)
					{
						memcpy(pdest, psrc, pvbmp->w * 4);
						psrc += fa->src.pitch;
						pdest += pvbmp->pitch;
					}
				}
				else
				{
					pvbmp->data = (Pixel *)new BYTE[fa->src.size];
					BYTE *psrc = (BYTE *)fa->src.data;
					BYTE *pdest = (BYTE *)pvbmp->data;
					for(int i = 0; i < fa->src.h; i++)
					{
						memcpy(pdest, psrc, pvbmp->w * 4);
						psrc += fa->src.pitch;
						pdest += pvbmp->pitch;
					}
				}
				pvbmp->palette = NULL;

				mfd->aFrameInfo[iNewFrame].pvbmp = pvbmp;

				// For rolling shutter, create projection mapping to previous and next frame based on global motion parameters stored away at the beginning of pass 2.
				if(mfd->aFrameInfo[iNewFrame].pgmERSThisSrcToPrevSrcFrom != NULL)
				{
					RGlobalMotionInfo *pgmFrom = mfd->aFrameInfo[iNewFrame].pgmERSThisSrcToPrevSrcFrom;
					RGlobalMotionInfo *pgmTo = mfd->aFrameInfo[iNewFrame].pgmERSThisSrcToPrevSrcTo;
					mfd->aFrameInfo[iNewFrame].pprojERSThisSrcToPrevSrc = new RProjectInfo[3 * fa->src.h];
					CVideoImage::CalcERSProjection(fa->src.w, fa->src.h, pgmFrom->dfRotation, pgmTo->dfRotation, pgmFrom->dfZoom, pgmTo->dfZoom, pgmFrom->dfShiftX, pgmTo->dfShiftX, pgmFrom->dfShiftY, pgmTo->dfShiftY, mfd->dfPixelAspectSrc, mfd->bInterlaced, bTopField, mfd->aFrameInfo[iNewFrame].pprojERSThisSrcToPrevSrc);
				}
				if(mfd->aFrameInfo[iNewFrame].pgmERSThisSrcToNextSrcFrom != NULL)
				{
					RGlobalMotionInfo *pgmFrom = mfd->aFrameInfo[iNewFrame].pgmERSThisSrcToNextSrcFrom;
					RGlobalMotionInfo *pgmTo = mfd->aFrameInfo[iNewFrame].pgmERSThisSrcToNextSrcTo;
					mfd->aFrameInfo[iNewFrame].pprojERSThisSrcToNextSrc = new RProjectInfo[3 * fa->src.h];
					CVideoImage::CalcReversedERSProjection(fa->src.w, fa->src.h, pgmFrom->dfRotation, pgmTo->dfRotation, pgmFrom->dfZoom, pgmTo->dfZoom, pgmFrom->dfShiftX, pgmTo->dfShiftX, pgmFrom->dfShiftY, pgmTo->dfShiftY, mfd->dfPixelAspectSrc, mfd->bInterlaced, !bTopField, mfd->aFrameInfo[iNewFrame].pprojERSThisSrcToNextSrc);
				}
			}
		}

		if(iCurFrame == iNewFrame || iCurFrame >= 0 && mfd->aFrameInfo[iCurFrame].pvbmp != NULL)
		{	// Morph frame according to projection mappings calculated at start of pass 2 and output it.
			if(mfd->aFrameInfo[iCurFrame].pgmERSCompSrcToOrgSrcFrom != NULL)
			{
				// For rolling shutter, create projection mapping from RS corrected source frame to original source frame.
				RGlobalMotionInfo *pgmFrom = mfd->aFrameInfo[iCurFrame].pgmERSCompSrcToOrgSrcFrom;
				RGlobalMotionInfo *pgmTo = mfd->aFrameInfo[iCurFrame].pgmERSCompSrcToOrgSrcTo;
				mfd->aFrameInfo[iCurFrame].pprojERSCompSrcToOrgSrc = new RProjectInfo[3 * fa->src.h];
				CVideoImage::CalcERSProjection(fa->src.w, fa->src.h, pgmFrom->dfRotation, pgmTo->dfRotation, pgmFrom->dfZoom, pgmTo->dfZoom, pgmFrom->dfShiftX, pgmTo->dfShiftX, pgmFrom->dfShiftY, pgmTo->dfShiftY, mfd->dfPixelAspectSrc, mfd->bInterlaced, bTopField, mfd->aFrameInfo[iCurFrame].pprojERSCompSrcToOrgSrc);
			}

			CVideoImage::VDubMorphImage2(fa, mfd->aFrameInfo, iCurFrame, mfd->iNoOfFrames - 1, mfd->iResampleAlg, iCurFrame != iNewFrame, !mfd->bUseOldFrames && !mfd->bUseFutureFrames, mfd->bExtrapolateBorder, mfd->bMultiFrameBorder, mfd->iEdgeTransitionPixels, mfd->bInterlaced ? mfd->iEdgeTransitionPixels / 2 : mfd->iEdgeTransitionPixels, mfd->bUseColorMask ? &mfd->uiMaskColor : NULL);
			bFrameWasOutput = TRUE;

			if(mfd->aFrameInfo[iCurFrame].pgmERSCompSrcToOrgSrcFrom != NULL)
			{
				delete[] mfd->aFrameInfo[iCurFrame].pprojERSCompSrcToOrgSrc;
				mfd->aFrameInfo[iCurFrame].pprojERSCompSrcToOrgSrc = NULL;
			}
		}
	}

	if(!bFrameWasOutput)
	{	// Frame was not processed in pass 1.
		BYTE *pdest = (BYTE *)fa->dst.data;
		for(int i = 0; i < fa->dst.h; i++)
		{
			memset(pdest, 0, fa->dst.w * 4);
			pdest += fa->dst.pitch;
		}

		if(fa->dst.hdc != NULL)
		{
			CDC *pdc = CDC::FromHandle((HDC)fa->dst.hdc);
			pdc->SetBkColor(RGB(0, 0, 0));
			CFont *pfontOld = pdc->SelectObject(mfd->pfontInfoText);
			pdc->SetTextColor(RGB(255, 255, 255));
			pdc->DrawText("Deshaker:", CRect(0, 0, fa->dst.w, iFrameHeight - 20), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			pdc->SetTextColor(RGB(208, 208, 208));
			pdc->DrawText("This frame was not processed in pass 1", CRect(0, 20, fa->dst.w, iFrameHeight), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			pdc->SelectObject(pfontOld);
			GdiFlush();
		}
	}

	mfd->iPrevSourceFrame = fa->pfsi->lCurrentSourceFrame;
	mfd->iFrameNo++;
}

/// <summary>
/// Shows config dialog. See VDub documentation for details.
/// </summary>
int ConfigProc(FilterActivation *fa, const FilterFunctions *ff, VDXHWND hwnd)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	MyFilterData *mfd = (MyFilterData *)fa->filter_data;

	HDC hdc = GetDC(GetDesktopWindow());
	int iScreenWidth = GetDeviceCaps(hdc, HORZRES);
	CDlgConfig dlgConfig(iScreenWidth < 1024 ? IDD_CONFIG_SMALL : IDD_CONFIG);
	dlgConfig.m_mfd = mfd;
	if(dlgConfig.DoModal() == IDOK)
	{
		mfd->filetimeLog.dwLowDateTime = mfd->filetimeLog.dwHighDateTime = 0;	// Trick to make pass 2 realize it needs to redo deshaking.
		if(mfd->bIgnoreOutside && mfd->bFollowIgnoreOutside && mfd->iPrevSourceFrame >= 0)
		{
			CRectDbl *prect;
			if(mfd->bInterlaced)
			{
				if(mfd->iPrevSourceFrame >= mfd->aprectIgnoreOutside->GetSize() || (prect = mfd->aprectIgnoreOutside->GetAt(mfd->iPrevSourceFrame)) == NULL)
				{
					prect = new CRectDbl(0, 0, 0, 0);
					mfd->aprectIgnoreOutside->SetAtGrow(mfd->iPrevSourceFrame, prect);
				}
			}
			else
			{
				if(mfd->iPrevSourceFrame + 1 >= mfd->aprectIgnoreOutside->GetSize() || (prect = mfd->aprectIgnoreOutside->GetAt(mfd->iPrevSourceFrame + 1)) == NULL)
				{
					prect = new CRectDbl(0, 0, 0, 0);
					mfd->aprectIgnoreOutside->SetAtGrow(mfd->iPrevSourceFrame + 1, prect);
				}
			}

			*prect = mfd->rectIgnoreOutside;
			if(mfd->bInterlaced)
			{
				prect->top /= 2;
				prect->bottom /= 2;
			}
		}
		return 0;
	}
	else
		return 1;
}

/// <summary>
/// Creates string shown in filter dialog in VDub.
/// </summary>
void StringProc(const FilterActivation *fa, const FilterFunctions *ff, char *str)
{
	sprintf_s(str, 50, " (Pass %i)", ((MyFilterData *)(fa->filter_data))->iPass);
}

/// <summary>
/// Frees all allocated pass 2 info about frames.
/// </summary>
void DeleteFrameInfo(MyFilterData *mfd)
{
	if(mfd->aFrameInfo != NULL)
	{
		for(int i = 0; i < mfd->iNoOfFrames; i++)
			FreeOneFrameInfo(mfd->aFrameInfo, i, TRUE);

		delete[] mfd->aFrameInfo;
		mfd->aFrameInfo = NULL;
	}
}

/// <summary>
/// Frees allocated pass 2 info about a frame.
/// </summary>
void FreeOneFrameInfo(RPass2FrameInfo *aFrameInfo, int iFrameIndex, BOOL bFreeAll)
{
	if(aFrameInfo[iFrameIndex].pvbmp != NULL)
	{
		delete[] (BYTE *)aFrameInfo[iFrameIndex].pvbmp->data;
		delete aFrameInfo[iFrameIndex].pvbmp;
		aFrameInfo[iFrameIndex].pvbmp = NULL;
	}

	delete[] aFrameInfo[iFrameIndex].pprojERSCompSrcToOrgSrc;
	aFrameInfo[iFrameIndex].pprojERSCompSrcToOrgSrc = NULL;
	delete[] aFrameInfo[iFrameIndex].pprojERSThisSrcToPrevSrc;
	aFrameInfo[iFrameIndex].pprojERSThisSrcToPrevSrc = NULL;
	delete[] aFrameInfo[iFrameIndex].pprojERSThisSrcToNextSrc;
	aFrameInfo[iFrameIndex].pprojERSThisSrcToNextSrc = NULL;
	
	if(bFreeAll)
	{
		delete aFrameInfo[iFrameIndex].pgmERSCompSrcToOrgSrcFrom;
		aFrameInfo[iFrameIndex].pgmERSCompSrcToOrgSrcFrom = NULL;
		delete aFrameInfo[iFrameIndex].pgmERSCompSrcToOrgSrcTo;
		aFrameInfo[iFrameIndex].pgmERSCompSrcToOrgSrcTo = NULL;
		delete aFrameInfo[iFrameIndex].pgmERSThisSrcToPrevSrcFrom;
		aFrameInfo[iFrameIndex].pgmERSThisSrcToPrevSrcFrom = NULL;
		delete aFrameInfo[iFrameIndex].pgmERSThisSrcToPrevSrcTo;
		aFrameInfo[iFrameIndex].pgmERSThisSrcToPrevSrcTo = NULL;
		delete aFrameInfo[iFrameIndex].pgmERSThisSrcToNextSrcFrom;
		aFrameInfo[iFrameIndex].pgmERSThisSrcToNextSrcFrom = NULL;
		delete aFrameInfo[iFrameIndex].pgmERSThisSrcToNextSrcTo;
		aFrameInfo[iFrameIndex].pgmERSThisSrcToNextSrcTo = NULL;
	}
}

/// <summary>
/// Solves an equation system using Gaussian elimination (*very* old and ugly code :).
/// </summary>
int gauss_elim(int n, double a[], double x[])
{
  int i,j,m,p,*nrow;
  double *s,f,max,tmp;

  nrow=(int*)calloc(n,sizeof(int));
  s=(double*)calloc(n,sizeof(double));
  if(!nrow||!s)
  {
    free(s);
    free(nrow);
    return 2;
  }
  m=n+1;
  for(i=0;i<n;i++){
    for(j=0;j<n;j++)
      if(fabs(a[i*m+j])>*(s+i))*(s+i)=fabs(a[i*m+j]);
    if(*(s+i)==0)
	{
	  free(s);
	  free(nrow);
	  return 1;
	}
    *(nrow+i)=i;
  }
  for(i=0;i<n-1;i++){
    max=0;
    for(j=i;j<n;j++){
      tmp=fabs(a[*(nrow+j)*m+i])/(*(s+*(nrow+j)));
      if(tmp>max){
	max=tmp;
	p=j;
      }
    }
    if(max==0)
	{
	  free(s);
	  free(nrow);
	  return 1;
	}
    if(i!=p){
      j=*(nrow+i);
      *(nrow+i)=*(nrow+p);
      *(nrow+p)=j;
    }
    for(j=i+1;j<n;j++){
      f=a[*(nrow+j)*m+i]/a[*(nrow+i)*m+i];
      for(p=i;p<m;p++)
	a[*(nrow+j)*m+p]-=f*a[*(nrow+i)*m+p];
    }
  }
  if(a[*(nrow+n-1)*m+n-1]==0)
  {
	free(s);
	free(nrow);
	return 1;
  }
  x[n-1]=a[*(nrow+n-1)*m+n]/a[*(nrow+n-1)*m+n-1];
  for(i=n-2;i>=0;i--){
    tmp=a[*(nrow+i)*m+n];
    for(p=i+1;p<n;p++)tmp-=a[*(nrow+i)*m+p]*x[p];
    x[i]=tmp/a[*(nrow+i)*m+i];
  }
  free(s);
  free(nrow);
  return 0;
}

/// <summary>
/// Calculates a smooth path for pass 2 by solving a huge equation system, using Gauss-Seidel, that tries to minimize both path curvature and amount of correction.
/// It will make the ends fit the original path (i.e. zero correction). Each scene is processed individually, in parallel.
/// </summary>
/// <param name="iN">Number of frames.</param>
/// <param name="adfPath">Original path.</param>
/// <param name="adfX">Smoothed path.</param>
/// <param name="dfSmoothness">Controls smoothness. LOWER values give smoother paths. Special cases: -100 gives "infinite" smoothness and -1 gives no smoothness.</param>
/// <param name="dfMaxDiff">Max. allowed difference between original and smoothed path, at any frame.</param>
/// <param name="dfErrorTolerance">This is an iterative method. Stop when the smoothed path doesn't change more than this value, in one iteration.</param>
/// <param name="abNewScene">Array telling if a certain frame is the first frame in a new scene.</param>
/// <returns>TRUE if ok, FALSE if aborted.</returns>
BOOL MakePathMinimizeCurvature(int iN, const double adfPath[], double adfX[], double dfSmoothness, double dfMaxDiff, double dfErrorTolerance, const BOOL abNewScene[])
{
	if(dfSmoothness < -100)
	{
		// For infinite smoothness:
		double dfSceneStartValue = 0.0;
		for (int i = 0; i < iN; i++)
		{
			if (abNewScene[i])
				dfSceneStartValue = adfPath[i];
			adfX[i] = dfSceneStartValue;
		}
		return TRUE;
	}

	if(dfSmoothness < 0)
	{
		// For no smoothness
		for(int i = 0; i < iN; i++)
			adfX[i] = adfPath[i];
		return TRUE;
	}

	RMakePathMinimizeCurvatureWorkData wd;
	wd.dfSmoothness = dfSmoothness;
	wd.dfMaxDiff = dfMaxDiff;
	wd.dfErrorTolerance = dfErrorTolerance;
	wd.bStopProcessing = FALSE;

	// Set up per scene work data.
	RMakePathWorkDataPerScene *pwdpc = NULL;
	int iSceneStartIndex = 0;
	int iStepProgress = 0;
	for(int i = 0; i < iN; i++)
	{
		if(i == 0 || abNewScene[i])
		{
			if(pwdpc != NULL)
			{
				pwdpc->iN = i - iSceneStartIndex;
				pwdpc = NULL;
			}

			if(i == iN - 1 || abNewScene[i + 1])
			{
				adfX[i] = adfPath[i];	// The scene has only one frame, follow path exactly.
				iStepProgress++;
			}
			else if(i == iN - 2 || abNewScene[i + 2])
			{
				adfX[i] = adfPath[i];	// The scene has only two frames, follow path exactly.
				i++;
				adfX[i] = adfPath[i];
				iStepProgress += 2;
			}
			else
			{
				// Create work data for scene.
				iSceneStartIndex = i;
				pwdpc = new RMakePathWorkDataPerScene;
				pwdpc->pdfPath = adfPath + i;
				pwdpc->pdfX = adfX + i;
				wd.aSceneData.Add(pwdpc);
			}
		}
	}

	if(pwdpc != NULL)
		pwdpc->iN = iN - iSceneStartIndex;

	if(iStepProgress > 0)
		g_pdlgProgress->OffsetPos(iStepProgress);

	// We want to process the longest scenes first. That way, we hopefully won't end up with one single thread handling a huge scene in the end, while the other CPU's are doing nothing.
	qsort(wd.aSceneData.GetData(), wd.aSceneData.GetSize(), sizeof(RMakePathWorkDataPerScene *), MakePathWorkDataSceneLengthComparer);

	wd.iStepProgress = 0;
	wd.iNextItemToProcess = 0;
	wd.iTotalItemsToProcess = (int)wd.aSceneData.GetCount();

	// Start threads.
	int iThreads = min(g_iCPUs, wd.iTotalItemsToProcess);
	CWinThread **apthreads = new CWinThread*[iThreads];
	HANDLE *ahthreads = new HANDLE[iThreads];
	for(int i = 0; i < iThreads; i++)
	{
		apthreads[i] = AfxBeginThread(MakePathMinimizeCurvatureThread, &wd, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
		apthreads[i]->m_bAutoDelete = FALSE;
		ahthreads[i] = apthreads[i]->m_hThread;
		apthreads[i]->ResumeThread();
	}

	// Wait for threads.
	while(TRUE)
	{
		BOOL bDone = WaitForMultipleObjects(iThreads, ahthreads, TRUE, 200) != WAIT_TIMEOUT;

		wd.csWorkItem.Lock();
		iStepProgress = wd.iStepProgress;
		wd.iStepProgress = 0;
		wd.csWorkItem.Unlock();

		if(iStepProgress > 0)
			g_pdlgProgress->OffsetPos(iStepProgress);

		if(bDone)
			break;

		if(g_pdlgProgress->CheckCancelButton())
		{
			if(AfxMessageBox("Really abort processing?", MB_YESNO) == IDYES)
			{
				wd.bStopProcessing = TRUE;
				WaitForMultipleObjects(iThreads, ahthreads, TRUE, INFINITE);
				break;
			}
		}
	}

	for(int i = 0; i < iThreads; i++)
		delete apthreads[i];
	delete[] ahthreads;
	delete[] apthreads;
	for(int i = 0; i < wd.aSceneData.GetSize(); i++)
		delete wd.aSceneData[i];

	return !wd.bStopProcessing;
}

/// <summary>
/// Thread for smoothing a scene using the algorithm to minimize curvature.
/// </summary>
UINT MakePathMinimizeCurvatureThread(LPVOID pParam)
{
	RMakePathMinimizeCurvatureWorkData *pwd = (RMakePathMinimizeCurvatureWorkData *)pParam;
	int iProcessingItem;

	pwd->csWorkItem.Lock();
	while(!pwd->bStopProcessing)
	{
		if(pwd->iNextItemToProcess >= pwd->iTotalItemsToProcess)
			break;

		iProcessingItem = pwd->iNextItemToProcess++;
		pwd->csWorkItem.Unlock();

		RMakePathWorkDataPerScene *pwdps = pwd->aSceneData[iProcessingItem];
		GaussSeidelSolvePath(pwdps->iN, pwdps->pdfPath, pwdps->pdfX, pwd->dfSmoothness, pwd->dfMaxDiff, pwd->dfErrorTolerance, &(pwd->bStopProcessing));

		pwd->csWorkItem.Lock();
		pwd->iStepProgress += pwdps->iN;
	}
	pwd->csWorkItem.Unlock();

	return 0;
}

/// <summary>
/// Calculates a smooth path for an individual scene in pass 2 by solving a huge equation system, using Gauss-Seidel, that tries to minimize both path curvature and amount of correction.
/// It will make the ends fit the original path (i.e. zero correction).
/// </summary>
/// <param name="iN">Number of frames.</param>
/// <param name="adfPath">Original path.</param>
/// <param name="adfX">Smoothed path.</param>
/// <param name="dfSmoothness">Controls smoothness. LOWER values give smoother paths. Special cases: -100 gives "infinite" smoothness and -1 gives no smoothness.</param>
/// <param name="dfMaxDiff">Max. allowed difference between original and smoothed path, at any frame.</param>
/// <param name="dfErrorTolerance">This is an iterative method. Stop when the smoothed path doesn't change more than this value, in one iteration.</param>
/// <param name="pbStopProcessing">Processing will stop if this points to TRUE.</param>
/// <returns>TRUE if ok, FALSE if aborted.</returns>
BOOL GaussSeidelSolvePath(int iN, const double *adfPath, double *adfX, double dfSmoothness, double dfMaxDiff, double dfErrorTolerance, BOOL *pbStopProcessing)
{
	double *adfSmoothness = new double[iN];
	for(int i = 0; i < iN; i++)
	{
		adfX[i] = 0.0;
		adfSmoothness[i] = dfSmoothness;
	}

	// Repeat Gauss-Seidel loop until corrections are within limits set by user.
	BOOL bContinue = TRUE;
	BOOL bOk;
	while(bContinue)
	{
		bContinue = FALSE;
		bOk = GaussSeidelSolvePath2(iN, adfPath, adfX, adfSmoothness, dfErrorTolerance, pbStopProcessing);
		if(!bOk)
			break;
		for(int i = 1; i < iN - 1; i++)
		{
			if(fabs(adfPath[i] - adfX[i]) > dfMaxDiff)
			{
				// Compensation is too big (bigger than correction limits set by user), increase weight for low compensation and recalculate (this can be very time consuming sometimes)
				adfSmoothness[i] = adfSmoothness[i] * 1.05;
				bContinue = TRUE;
			}
		}
	}

	delete[] adfSmoothness;
	return bOk;
}

/// <summary>
/// See GaussSeidelSolvePath above. Loops until error tolerance is reached.
/// </summary>
/// <returns>TRUE if ok, FALSE if aborted.</returns>
BOOL GaussSeidelSolvePath2(int iN, const double *adfPath, double *adfX, const double *adfSmoothness, double dfErrorTolerance, BOOL *pbStopProcessing)
{
	double dfDiff, dfXi;
	int iLastIndex = iN - 1;
	while(TRUE)
	{
		dfDiff = 0.0;
		adfX[0] = adfPath[0];	// Start frame has zero correction.
		for(int i = 1; i < iLastIndex; i++)
		{
			// We want to minimize this function: <smoothness factor> * <difference between smoothed position and original position>^2 + <output motion curvature at frame>^2
			// I.e. to minimize both correction and curvature, and control the importance between these with a smoothness factor weight.
			// To minimize it we take the derivative of that funcion and tell it to be zero, and (hopefully) get the following (I did this a loong time ago):
			dfXi = (adfSmoothness[i] * adfPath[i] + adfX[i - 1] + adfX[i + 1]) / (2.0 + adfSmoothness[i]);	// Minimize curvature and correction.
			dfDiff += fabs(dfXi - adfX[i]);
			adfX[i] = dfXi;
		}
		adfX[iLastIndex] = adfPath[iLastIndex];	// End frame has zero correction.
		
		if(dfDiff < dfErrorTolerance)
			return TRUE;

		if(*pbStopProcessing)
			return FALSE;
	}
}

/// <summary>
/// Calculates a smooth path for pass 2 by applying a gaussian averaging filter.
/// It will *not* make the ends fit the original path.
/// Each scene is processed individually, in parallel.
/// </summary>
/// <param name="iN">Number of frames.</param>
/// <param name="adfPath">Original path.</param>
/// <param name="adfX">Smoothed path.</param>
/// <param name="dfSmoothness">Controls smoothness. LOWER values give smoother paths. Special cases: -100 gives "infinite" smoothness and -1 gives no smoothness.</param>
/// <param name="dfMaxDiffBelow">Max. allowed difference between original and smoothed path, when smooth path is lower than original, or -1 for no limit.</param>
/// <param name="dfMaxDiffAbove">Max. allowed difference between original and smoothed path, when smooth path is higher than original, or -1 for no limit.</param>
/// <param name="dfMinAdjust">When adjusting path if max allowed diff limits are broken, modify by *at least* this amount.</param>
/// <param name="abNewScene">Array telling if a certain frame is the first frame in a new scene.</param>
/// <returns>TRUE if ok, FALSE if aborted.</returns>
BOOL MakePathGaussianFilter(int iN, const double adfPath[], double adfX[], double dfSmoothness, double dfMaxDiffBelow, double dfMaxDiffAbove, double dfMinAdjust, const BOOL abNewScene[])
{
	dfSmoothness /= 2.0;	// Make smoothness effect similar to the other algorithm.
	int iKernelSize = (int)(sqrt(-log(0.002) * dfSmoothness) + 1);	// Ignore weights below 0.002.
	double *adfGaussKernel = new double[2 * iKernelSize + 1];
	for(int i = 0; i <= iKernelSize; i++)
	{
		adfGaussKernel[iKernelSize + i] = exp((double)-i * i / dfSmoothness);
		adfGaussKernel[iKernelSize - i] = adfGaussKernel[iKernelSize + i];
	}

	RMakePathGaussianFilterWorkData wd;
	wd.dfMaxDiffBelow = dfMaxDiffBelow;
	wd.dfMaxDiffAbove = dfMaxDiffAbove;
	wd.dfMinAdjust = dfMinAdjust;
	wd.iKernelSize = iKernelSize;
	wd.adfGaussKernel = adfGaussKernel + iKernelSize;
	wd.bStopProcessing = FALSE;

	// Set up per scene work data.
	RMakePathWorkDataPerScene *pwdpc = NULL;
	int iSceneStartIndex = 0;
	int iStepProgress = 0;
	for(int i = 0; i < iN; i++)
	{
		if(i == 0 || abNewScene[i])
		{
			if(pwdpc != NULL)
			{
				pwdpc->iN = i - iSceneStartIndex;
				pwdpc = NULL;
			}

			if(i == iN - 1 || abNewScene[i + 1])
			{
				adfX[i] = adfPath[i];	// The scene has only one frame, follow path exactly.
				iStepProgress++;
			}
			else if(i == iN - 2 || abNewScene[i + 2])
			{
				adfX[i] = adfPath[i];	// The scene has only two frames, follow path exactly.
				i++;
				adfX[i] = adfPath[i];
				iStepProgress += 2;
			}
			else
			{
				// Create work data for scene.
				iSceneStartIndex = i;
				pwdpc = new RMakePathWorkDataPerScene;
				pwdpc->pdfPath = adfPath + i;
				pwdpc->pdfX = adfX + i;
				wd.aSceneData.Add(pwdpc);
			}
		}
	}

	if(pwdpc != NULL)
		pwdpc->iN = iN - iSceneStartIndex;

	if(iStepProgress > 0)
		g_pdlgProgress->OffsetPos(iStepProgress);

	// We want to process the longest scenes first. That way, we hopefully won't end up with one single thread handling a huge scene in the end, while the other CPU's are doing nothing.
	qsort(wd.aSceneData.GetData(), wd.aSceneData.GetSize(), sizeof(RMakePathWorkDataPerScene *), MakePathWorkDataSceneLengthComparer);

	wd.iStepProgress = 0;
	wd.iNextItemToProcess = 0;
	wd.iTotalItemsToProcess = (int)wd.aSceneData.GetCount();

	// Start threads.
	int iThreads = min(g_iCPUs, wd.iTotalItemsToProcess);
	CWinThread **apthreads = new CWinThread*[iThreads];
	HANDLE *ahthreads = new HANDLE[iThreads];
	for(int i = 0; i < iThreads; i++)
	{
		apthreads[i] = AfxBeginThread(MakePathGaussianFilterThread, &wd, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
		apthreads[i]->m_bAutoDelete = FALSE;
		ahthreads[i] = apthreads[i]->m_hThread;
		apthreads[i]->ResumeThread();
	}

	// Wait for threads.
	while(TRUE)
	{
		BOOL bDone = WaitForMultipleObjects(iThreads, ahthreads, TRUE, 200) != WAIT_TIMEOUT;

		wd.csWorkItem.Lock();
		iStepProgress = wd.iStepProgress;
		wd.iStepProgress = 0;
		wd.csWorkItem.Unlock();

		if(iStepProgress > 0)
			g_pdlgProgress->OffsetPos(iStepProgress);

		if(bDone)
			break;

		if(g_pdlgProgress->CheckCancelButton())
		{
			if(AfxMessageBox("Really abort processing?", MB_YESNO) == IDYES)
			{
				wd.bStopProcessing = TRUE;
				WaitForMultipleObjects(iThreads, ahthreads, TRUE, INFINITE);
				break;
			}
		}
	}

	for(int i = 0; i < iThreads; i++)
		delete apthreads[i];
	delete[] ahthreads;
	delete[] apthreads;
	for(int i = 0; i < wd.aSceneData.GetSize(); i++)
		delete wd.aSceneData[i];
	delete[] adfGaussKernel;

	return !wd.bStopProcessing;
}

/// <summary>
/// Thread for smoothing a scene using the Gaussian filter algorithm.
/// </summary>
UINT MakePathGaussianFilterThread(LPVOID pParam)
{
	RMakePathGaussianFilterWorkData *pwd = (RMakePathGaussianFilterWorkData *)pParam;
	int iProcessingItem;

	pwd->csWorkItem.Lock();
	while(!pwd->bStopProcessing)
	{
		if(pwd->iNextItemToProcess >= pwd->iTotalItemsToProcess)
			break;

		iProcessingItem = pwd->iNextItemToProcess++;
		pwd->csWorkItem.Unlock();

		RMakePathWorkDataPerScene *pwdps = pwd->aSceneData[iProcessingItem];
		GaussianFilterMakePath(pwdps->iN, pwdps->pdfPath, pwdps->pdfX, pwd->iKernelSize, pwd->adfGaussKernel, pwd->dfMaxDiffBelow, pwd->dfMaxDiffAbove, pwd->dfMinAdjust, &(pwd->bStopProcessing));

		pwd->csWorkItem.Lock();
		pwd->iStepProgress += pwdps->iN;
	}
	pwd->csWorkItem.Unlock();

	return 0;
}

/// <summary>
/// Calculates a smooth path for an individual scene in pass 2 by applying a gaussian averaging filter.
/// It will *not* make the ends fit the original path.
/// </summary>
/// <param name="iN">Number of frames.</param>
/// <param name="adfPath">Original path.</param>
/// <param name="adfX">Smoothed path.</param>
/// <param name="dfSmoothness">Controls smoothness. LOWER values give smoother paths. Special cases: -100 gives "infinite" smoothness and -1 gives no smoothness.</param>
/// <param name="dfMaxDiffBelow">Max. allowed difference between original and smoothed path, when smooth path is lower than original, or -1 for no limit.</param>
/// <param name="dfMaxDiffAbove">Max. allowed difference between original and smoothed path, when smooth path is higher than original, or -1 for no limit.</param>
/// <param name="dfMinAdjust">When adjusting path if max allowed diff limits are broken, modify by *at least* this amount.</param>
/// <param name="abNewScene">Array telling if a certain frame is the first frame in a new scene.</param>
/// <returns>TRUE if ok, FALSE if aborted.</returns>
BOOL GaussianFilterMakePath(int iN, const double *adfPath, double *adfX, int iKernelSize, double *adfGaussKernel, double dfMaxDiffBelow, double dfMaxDiffAbove, double dfMinAdjust, BOOL *pbStopProcessing)
{
	BOOL bOk = TRUE;
	BOOL bContinue = TRUE;

	double *adfPathTmp = new double[iN];
	for(int i = 0; i < iN; i++)
		adfPathTmp[i] = adfPath[i];

	while(bOk && bContinue)
	{
		bContinue = FALSE;
		double dfSum = 0;
		int iItems = 0;
		for(int i = 0; i < iN; i++)
		{
			int iFrom = max(i - iKernelSize, 0);
			int iTo = min(i + iKernelSize, iN - 1);
			double dfSum = 0.0, dfTotWeight = 0.0, dfWeight;
			for(int j = iFrom; j <= iTo; j++)
			{
				dfWeight = adfGaussKernel[j - i];
				dfSum += adfPathTmp[j] * dfWeight;
				dfTotWeight += dfWeight;
			}

			adfX[i] = dfSum / dfTotWeight;
			if(*pbStopProcessing)
			{
				bOk = FALSE;
				break;
			}
		}

		if(dfMaxDiffBelow >= 0 || dfMaxDiffAbove >= 0)
		{
			// Adjust path to follow and redo smoothing above. Eventually the smooth path will be within bounds.
			for(int i = 0; i < iN; i++)
			{
				if(dfMaxDiffBelow >= 0 && adfX[i] < adfPath[i] - dfMaxDiffBelow)
				{
					adfPathTmp[i] += max((adfPath[i] - adfX[i]) * 0.8, dfMinAdjust);
					bContinue = TRUE;
				}
				if(dfMaxDiffAbove >= 0 && adfX[i] > adfPath[i] + dfMaxDiffAbove)
				{
					adfPathTmp[i] -= max((adfX[i] - adfPath[i]) * 0.8, dfMinAdjust);
					bContinue = TRUE;
				}
			}
		}
	}

	delete[] adfPathTmp;
	return bOk;
}

/// <summary>
/// Comparer (for qsort) making longer scenes come before shorter ones.
/// </summary>
int MakePathWorkDataSceneLengthComparer(const void *arg1, const void *arg2)
{
	int iN1 = (*(RMakePathWorkDataPerScene **)arg1)->iN;
	int iN2 = (*(RMakePathWorkDataPerScene **)arg2)->iN;

	if(iN1 < iN2)
		return 1;
	else if(iN2 < iN1)
		return -1;
	else
		return 0;
}

/// <summary>
/// Helper function for picking the next "item" from a string.
/// </summary>
CString GetNextItem(CString &str, char *szSeparators /*=" \t\r\n"*/)
{
	str.TrimLeft();
	int iPos = str.FindOneOf(szSeparators);
	if(iPos > 0)
	{
		CString strItem = str.Left(iPos);
		str = str.Mid(iPos + 1);
		return strItem;
	}
	else
	{
		CString strItem = str;
		str.Empty();
		return strItem;
	}
}

/// <summary>
/// Alters some variables to make rest of code work with interlaced video instead of progressive without too many changes.
/// </summary>
/// <param name="fa">See VDub documentation.</param>
/// <param name="bFix">TRUE to fix or FALSE to undo fix.</param>
/// <param name="bTopField">TRUE if it's the top field, or FALSE if bottom field.</param>
/// <param name="bFirstField">TRUE if it's the first field chronologically, or FALSE if 2nd field.</param>
/// <param name="iPass">Pass 1 or 2.</param>
/// <param name="bInterlacedProgressiveDest">TRUE to make interlaced video progressive with doubled framerate.</param>
void FixForInterlacedProcessing(FilterActivation *fa, BOOL bFix, BOOL bTopField, BOOL bFirstField, int iPass, BOOL bInterlacedProgressiveDest)
{
	static long src_h, dst_h, lCurrentSourceFrame, lIgnoreInsideTop, lIgnoreInsideBottom;
	static ptrdiff_t src_modulo, dst_modulo, src_pitch, dst_pitch;
	static Pixel *src_data, *dst_data;
	static int iOldFrames, iFutureFrames;

	MyFilterData *mfd = (MyFilterData *)fa->filter_data;

	if(bFix)
	{
		src_h = fa->src.h;
		src_modulo = fa->src.modulo;
		src_pitch = fa->src.pitch;
		src_data = fa->src.data;
		dst_h = fa->dst.h;
		dst_modulo = fa->dst.modulo;
		dst_pitch = fa->dst.pitch;
		dst_data = fa->dst.data;
		lCurrentSourceFrame = fa->pfsi->lCurrentSourceFrame;
		lIgnoreInsideTop = mfd->rectIgnoreInside.top;
		lIgnoreInsideBottom = mfd->rectIgnoreInside.bottom;
		iOldFrames = mfd->iOldFrames;
		iFutureFrames = mfd->iFutureFrames;
		
		fa->src.h /= 2;		// A field has half as many rows.
		if(bTopField)
			fa->src.data = (Pixel32 *)((char *)fa->src.data + fa->src.pitch);	// Start row is one row up for top field.
		// Alter modulo and pitch to make processing everywhere skip every other row (ie just use one field).
		fa->src.modulo += fa->src.pitch;
		fa->src.pitch *= 2;

		if(iPass == 1)
		{
			fa->dst.h /= 2;
			if(bFirstField)
				fa->dst.data = (Pixel32 *)((char *)fa->dst.data + fa->dst.h * fa->dst.pitch);	// Position first field above last during pass 1.
		}
		else
		{
			fa->dst.h /= 2;
			if(bTopField)
				fa->dst.data = (Pixel32 *)((char *)fa->dst.data + fa->dst.pitch);	// Start row is one row up for top field.
			// Alter modulo and pitch to make processing everywhere skip every other row (ie just use one field).
			fa->dst.modulo += fa->dst.pitch;
			fa->dst.pitch *= 2;
		}

		// Make lCurrentFrame count fields instead of frames.
		fa->pfsi->lCurrentSourceFrame *= 2;
		if(!bFirstField)
			fa->pfsi->lCurrentSourceFrame += 1;

		// Recalc ignored areas.
		mfd->rectIgnoreInside.top /= 2;
		mfd->rectIgnoreInside.bottom /= 2;
		mfd->rectIgnoreOutside.top /= 2;
		mfd->rectIgnoreOutside.bottom /= 2;

		// Count fields instead.
		mfd->iOldFrames *= 2;
		mfd->iFutureFrames *= 2;
	}
	else	// unfix
	{
		// Reset everything altered above.
		fa->src.h = src_h;
		fa->src.modulo = src_modulo;
		fa->src.pitch = src_pitch;
		fa->src.data = src_data;
		fa->dst.h = dst_h;
		fa->dst.modulo = dst_modulo;
		fa->dst.pitch = dst_pitch;
		fa->dst.data = dst_data;
		fa->pfsi->lCurrentSourceFrame = lCurrentSourceFrame;
		mfd->rectIgnoreInside.top = lIgnoreInsideTop;
		mfd->rectIgnoreInside.bottom = lIgnoreInsideBottom;
		mfd->rectIgnoreOutside.top *= 2;
		mfd->rectIgnoreOutside.bottom *= 2;
		mfd->iOldFrames = iOldFrames;
		mfd->iFutureFrames = iFutureFrames;
	}
}

/// <summary>
/// Calculates overall xpan, ypan, rotation and zoom for pass 1 based on the equation system in adfAb and adds and removes blocks to get good values.
/// </summary>
/// <param name="aiValidBlocks">Array of valid block indexes.</param>
/// <param name="iStartBlock">Block index of first block in frame (or field), when looking up values in MyFilterData.adfDistTimesCosOfBlockAngle. (Should be 0 unless it's the 2nd field of interlaced video.) </param>
/// <param name="iMatchBlocksX">No. of blocks in X direction.</param>
/// <param name="iMatchBlocksY">No. of blocks in Y direction.</param>
/// <param name="iImageHeight">Height of frame.</param>
/// <param name="adfAb">Equation system. Several "rows" (one for each block) of 5 values, being no. of pixels for xpan, ypan, rotation, zoom and resulting shift in x or y direction.</param>
/// <param name="adfXShifts">Detected X shift, per block.</param>
/// <param name="adfYShifts">Detected X shift, per block.</param>
/// <param name="mfd">MyFilterData</param>
/// <param name="abUseBlock">Syas, per block, if it should be used for GM calculation or not. (Only "trusted" ones should be used.)</param>
/// <param name="bERS">TRUE if rolling shutter.</param>
/// <param name="gm">Global motion (for top line in frame in case of rolling shutter video). </param>
/// <param name="gm2">Globel motion for bottom line in frame in case of rolling shutter video. (Only used for rolling shutter video.)</param>
/// <param name="bDetectRotation">TRUE to detect rotation, or FALSE to assume rotation is 0.</param>
/// <param name="bDetectZoom">TRUE to detect zoom, or FALSE to assume zoom is 0.</param>
void CalcGlobalMotion(const CArray<int, int> &aiValidBlocks, int iStartBlock, int iMatchBlocksX, int iMatchBlocksY, int iImageHeight, const double *adfAb, const double adfXShifts[], const double adfYShifts[],
	MyFilterData *mfd, BOOL abUseBlock[], BOOL bERS, RGlobalMotionInfo &gm, RGlobalMotionInfo &gm2, BOOL bDetectRotation, BOOL bDetectZoom)
{
	double dfShiftDiff;
	int iValidBlocks = (int)aiValidBlocks.GetSize();
	int iBlockIndex;
	BOOL bChangeWasMade;
	RGlobalMotionInfo *agmRows;
	if(bERS)
		agmRows = new RGlobalMotionInfo[iMatchBlocksY];

	for(int i = 0; i < 100; i++)	// Limit to 100 just in case it would loop forever.
	{
		// Remove all blocks that are "out of range" of mfd->dfPixelsOffToDiscardBlock.
		bChangeWasMade = FALSE;
		for(;;)
		{
			// Calculate new gm values every time we remove a block.
			CalcGlobalMotion2(aiValidBlocks, adfAb, abUseBlock, bERS, gm, gm2, bDetectRotation, bDetectZoom);
			if(bERS)
			{
				// For rolling shutter, calculate the gm parameters for each block row. (Otherwise they are the same for all rows.)
				// Interpolate the parameters linearly from gm (top of frame) to gm2 (bottom of frame).
				for(int iBlockRow = 0; iBlockRow < iMatchBlocksY; iBlockRow++)
				{
					double dfStartFactor = 1.0 - (iBlockRow + 0.5) * mfd->iBlockSize / iImageHeight; // Factor that goes from 1.0 for first line to 0 for last line of frame.
					agmRows[iBlockRow].dfShiftX = dfStartFactor * gm.dfShiftX + (1.0 - dfStartFactor) * gm2.dfShiftX;
					agmRows[iBlockRow].dfShiftY = dfStartFactor * gm.dfShiftY + (1.0 - dfStartFactor) * gm2.dfShiftY;
					agmRows[iBlockRow].dfRotation = dfStartFactor * gm.dfRotation + (1.0 - dfStartFactor) * gm2.dfRotation;
					agmRows[iBlockRow].dfZoom = dfStartFactor * gm.dfZoom + (1.0 - dfStartFactor) * gm2.dfZoom;
				}
			}

			// Find block that deviates the most.
			double dfShiftDiffMax = -1;
			int iWorstBlockIndex;
			for(int iValidBlock = 0; iValidBlock < iValidBlocks; iValidBlock++)
			{
				iBlockIndex = aiValidBlocks[iValidBlock];
				if(abUseBlock[iBlockIndex])
				{
					dfShiftDiff = CalcBlockShiftDiffFromGlobalMotion(bERS ? agmRows[iBlockIndex / iMatchBlocksX] : gm, adfXShifts[iBlockIndex], adfYShifts[iBlockIndex], mfd, iStartBlock + iBlockIndex);
					if(dfShiftDiff > dfShiftDiffMax)
					{
						dfShiftDiffMax = dfShiftDiff;
						iWorstBlockIndex = iBlockIndex;
					}
				}
			}

			if(dfShiftDiffMax >= mfd->dfPixelsOffToDiscardBlock * mfd->dfPixelsOffToDiscardBlock)
			{
				abUseBlock[iWorstBlockIndex] = FALSE;	// Exclude the worst block.
				bChangeWasMade = TRUE;
			}
			else
				break;
		}
		if(!bChangeWasMade && i > 0)
			break;

		// Add all blocks that get "in range of" mfd->dfPixelsOffToDiscardBlock.
		// This is new in version 1.7. Previously blocks could be removed in a bad order so that we ended up in a local maximum that wasn't very good.
		// This should take care of that to some extent. The new deep analysis algorithm takes care of it even more.
		bChangeWasMade = FALSE;
		for(int iValidBlock = 0; iValidBlock < iValidBlocks; iValidBlock++)
		{
			iBlockIndex = aiValidBlocks[iValidBlock];
			if(!abUseBlock[iBlockIndex])
			{
				dfShiftDiff = CalcBlockShiftDiffFromGlobalMotion(bERS ? agmRows[iBlockIndex / iMatchBlocksX] : gm, adfXShifts[iBlockIndex], adfYShifts[iBlockIndex], mfd, iStartBlock + iBlockIndex);
				if(dfShiftDiff < mfd->dfPixelsOffToDiscardBlock * mfd->dfPixelsOffToDiscardBlock)
				{
					abUseBlock[iBlockIndex] = TRUE;
					bChangeWasMade = TRUE;
				}
			}
		}
		if(!bChangeWasMade)
			break;
	}

	if(bERS)
		delete[] agmRows;
}

/// <summary>
/// See CalcGlobalMotion.
/// Finds the 4 unknowns (xpan, ypan, rotation, zoom) based on an equation system with two equations per image block, describing how it has moved in X and Y.
/// 4 unknowns become 8 for rolling shutter video, though. One set for top line of frame and one for bottom line. Linear interpolation is assumed between these.
/// Since the equation system has many more rows than unknowns, it is solved using least squares fit.
/// </summary>
/// <param name="aiValidBlocks">Array of valid block indexes.</param>
/// <param name="adfAb">Equation system. Several "rows" (one for each block) of 5 values, being no. of pixels for xpan, ypan, rotation, zoom and resulting shift in x or y direction.</param>
/// <param name="abUseBlock">Syas, per block, if it should be used for GM calculation or not. (Only "trusted" ones should be used.)</param>
/// <param name="bERS">TRUE if rolling shutter.</param>
/// <param name="gm">Global motion (for top line in frame in case of rolling shutter video). </param>
/// <param name="gm2">Globel motion for bottom line in frame in case of rolling shutter video. (Only used for rolling shutter video.)</param>
/// <param name="bDetectRotation">TRUE to detect rotation, or FALSE to assume rotation is 0.</param>
/// <param name="bDetectZoom">TRUE to detect zoom, or FALSE to assume zoom is 0.</param>
void CalcGlobalMotion2(const CArray<int, int> &aiValidBlocks, const double *adfAb, BOOL abUseBlock[], BOOL bERS, RGlobalMotionInfo &gm, RGlobalMotionInfo &gm2, BOOL bDetectRotation, BOOL bDetectZoom)
{
	if(bERS)
	{
		// For rolling shutter video...

		// Remap column indexes and skip columns for rotation and/or zoom if they shoudn't be detected.
		int aiFullIndexToLimitedIndex[9];
		int iIndexCount = 0;
		aiFullIndexToLimitedIndex[0] = iIndexCount++;
		aiFullIndexToLimitedIndex[1] = iIndexCount++;
		aiFullIndexToLimitedIndex[2] = bDetectRotation ? iIndexCount++ : -1;
		aiFullIndexToLimitedIndex[3] = bDetectZoom ? iIndexCount++ : -1;
		aiFullIndexToLimitedIndex[4] = iIndexCount++;
		aiFullIndexToLimitedIndex[5] = iIndexCount++;
		aiFullIndexToLimitedIndex[6] = bDetectRotation ? iIndexCount++ : -1;
		aiFullIndexToLimitedIndex[7] = bDetectZoom ? iIndexCount++ : -1;
		aiFullIndexToLimitedIndex[8] = iIndexCount;	// b

		// Create a least squares matrix: (A(transposed) * A) * X = A(transposed) * b
		double *adfATAATb = new double[(iIndexCount + 1) * iIndexCount], *adfX = new double[iIndexCount], dfMatrixItem;
		int iValidBlocks = (int)aiValidBlocks.GetSize();
		for(int iMatrixCol = 0; iMatrixCol < 9; iMatrixCol++)
		{
			if(aiFullIndexToLimitedIndex[iMatrixCol] >= 0)
			{
				for(int iMatrixRow = 0; iMatrixRow < 8 && iMatrixRow <= iMatrixCol; iMatrixRow++)
				{
					if(aiFullIndexToLimitedIndex[iMatrixRow] >= 0)
					{
						dfMatrixItem = 0.0;
						for(int iMatrixItem = (2 * iValidBlocks - 1) * 9; iMatrixItem >= 0; iMatrixItem -= 9)
							if(abUseBlock[aiValidBlocks[iMatrixItem / 18]])
								dfMatrixItem += adfAb[iMatrixItem + iMatrixCol] * adfAb[iMatrixItem + iMatrixRow];
						
						adfATAATb[aiFullIndexToLimitedIndex[iMatrixRow] * (iIndexCount + 1) + aiFullIndexToLimitedIndex[iMatrixCol]] = dfMatrixItem;
						if(iMatrixCol < 8)
							adfATAATb[aiFullIndexToLimitedIndex[iMatrixCol] * (iIndexCount + 1) + aiFullIndexToLimitedIndex[iMatrixRow]] = dfMatrixItem;
					}
				}
			}
		}

		// Solve equation system (iIndexCount equations, iIndexCount unknowns).
		gauss_elim(iIndexCount, adfATAATb, adfX);

		gm.dfShiftX = aiFullIndexToLimitedIndex[0] >= 0 ? adfX[aiFullIndexToLimitedIndex[0]] : 0;
		gm.dfShiftY = aiFullIndexToLimitedIndex[1] >= 0 ? adfX[aiFullIndexToLimitedIndex[1]] : 0;
		gm.dfRotation = aiFullIndexToLimitedIndex[2] >= 0 ? adfX[aiFullIndexToLimitedIndex[2]] : 0;
		gm.dfZoom = aiFullIndexToLimitedIndex[3] >= 0 ? adfX[aiFullIndexToLimitedIndex[3]] : 0;

		gm2.dfShiftX = aiFullIndexToLimitedIndex[4] >= 0 ? adfX[aiFullIndexToLimitedIndex[4]] : 0;
		gm2.dfShiftY = aiFullIndexToLimitedIndex[5] >= 0 ? adfX[aiFullIndexToLimitedIndex[5]] : 0;
		gm2.dfRotation = aiFullIndexToLimitedIndex[6] >= 0 ? adfX[aiFullIndexToLimitedIndex[6]] : 0;
		gm2.dfZoom = aiFullIndexToLimitedIndex[7] >= 0 ? adfX[aiFullIndexToLimitedIndex[7]] : 0;

		delete[] adfATAATb;
		delete[] adfX;
	}
	else
	{
		// For global shutter video...
		
		// Remap column indexes and skip columns for rotation and/or zoom if they shoudn't be detected.
		int aiFullIndexToLimitedIndex[5];
		int iIndexCount = 0;
		aiFullIndexToLimitedIndex[0] = iIndexCount++;
		aiFullIndexToLimitedIndex[1] = iIndexCount++;
		aiFullIndexToLimitedIndex[2] = bDetectRotation ? iIndexCount++ : -1;
		aiFullIndexToLimitedIndex[3] = bDetectZoom ? iIndexCount++ : -1;
		aiFullIndexToLimitedIndex[4] = iIndexCount;	// b

		// Create a least squares matrix: (A(transposed) * A) * X = A(transposed) * b
		double *adfATAATb = new double[(iIndexCount + 1) * iIndexCount], *adfX = new double[iIndexCount], dfMatrixItem;
		int iValidBlocks = (int)aiValidBlocks.GetSize();
		for(int iMatrixCol = 0; iMatrixCol < 5; iMatrixCol++)
		{
			if(aiFullIndexToLimitedIndex[iMatrixCol] >= 0)
			{
				for(int iMatrixRow = 0; iMatrixRow < 4 && iMatrixRow <= iMatrixCol; iMatrixRow++)
				{
					if(aiFullIndexToLimitedIndex[iMatrixRow] >= 0)
					{
						dfMatrixItem = 0.0;
						for(int iMatrixItem = (2 * iValidBlocks - 1) * 5; iMatrixItem >= 0; iMatrixItem -= 5)
							if(abUseBlock[aiValidBlocks[iMatrixItem / 10]])
								dfMatrixItem += adfAb[iMatrixItem + iMatrixCol] * adfAb[iMatrixItem + iMatrixRow];
						
						adfATAATb[aiFullIndexToLimitedIndex[iMatrixRow] * (iIndexCount + 1) + aiFullIndexToLimitedIndex[iMatrixCol]] = dfMatrixItem;
						if(iMatrixCol < 4)
							adfATAATb[aiFullIndexToLimitedIndex[iMatrixCol] * (iIndexCount + 1) + aiFullIndexToLimitedIndex[iMatrixRow]] = dfMatrixItem;
					}
				}
			}
		}

		// Solve equation system (iIndexCount equations, iIndexCount unknowns).
		gauss_elim(iIndexCount, adfATAATb, adfX);

		gm.dfShiftX = aiFullIndexToLimitedIndex[0] >= 0 ? adfX[aiFullIndexToLimitedIndex[0]] : 0;
		gm.dfShiftY = aiFullIndexToLimitedIndex[1] >= 0 ? adfX[aiFullIndexToLimitedIndex[1]] : 0;
		gm.dfRotation = aiFullIndexToLimitedIndex[2] >= 0 ? adfX[aiFullIndexToLimitedIndex[2]] : 0;
		gm.dfZoom = aiFullIndexToLimitedIndex[3] >= 0 ? adfX[aiFullIndexToLimitedIndex[3]] : 0;

		delete[] adfATAATb;
		delete[] adfX;
	}
}

/// <summary>
/// Calculates the squared shift offset from global motion parameters for a certain block.
/// </summary>
/// <param name="gm">Global motion</param>
/// <param name="dfXShift">Detected X shift of block.</param>
/// <param name="dfYShift">Detected X shift of block.</param>
/// <param name="mfd">MyFilterData</param>
/// <param name="iBlockIndex">Index of block.</param>
/// <returns>Squared difference between a detected shift and a shift that gm "describes" for a certain block. </returns>
inline double CalcBlockShiftDiffFromGlobalMotion(const RGlobalMotionInfo &gm, double dfXShift, double dfYShift, MyFilterData *mfd, int iBlockIndex)
{
	double dfDiffX = dfXShift -
					 gm.dfRotation * mfd->adfDistTimesCosOfBlockAngle[iBlockIndex] -
					 gm.dfZoom * mfd->adfDistTimesSinOfBlockAngle[iBlockIndex] -
					 gm.dfShiftX;
	double dfDiffY = dfYShift / mfd->dfPixelAspectSrc -
					 gm.dfRotation * mfd->adfDistTimesSinOfBlockAngle[iBlockIndex] -
					 gm.dfZoom * -mfd->adfDistTimesCosOfBlockAngle[iBlockIndex] -
					 gm.dfShiftY;
	return dfDiffX * dfDiffX + dfDiffY * dfDiffY;
}

/// <summary>
/// Thread function used in pass 1 to find the best match for all blocks.
/// </summary>
UINT FindShiftsThread(LPVOID pParam)
{
	RFindShiftsWorkData *pwd = (RFindShiftsWorkData *)pParam;
	int iProcessingItem;

	while(TRUE)
	{
		// Critical section start... get new processing item.
		pwd->csNextItem.Lock();
		BOOL bContinueWorking = pwd->iNextItemToProcess < pwd->iTotalItemsToProcess;
		if(bContinueWorking)
			iProcessingItem = pwd->iNextItemToProcess++;
		pwd->csNextItem.Unlock();
		// Critical section end...

		if(!bContinueWorking)
			break;

		int iBlockIndexThisScale = iProcessingItem;
		int iBlockY = iBlockIndexThisScale / pwd->iMatchBlocksX;
		int iBlockX = iBlockIndexThisScale % pwd->iMatchBlocksX;
		int iBlockStart1X = iBlockX * pwd->mfd->iBlockSize;
		int iBlockStart1Y = iBlockY * pwd->mfd->iBlockSize;
		int iBlockIndexPrevScale = min(pwd->iMatchBlocksYPrevScale - 1, iBlockY / 2) * pwd->iMatchBlocksXPrevScale + min(pwd->iMatchBlocksXPrevScale - 1, iBlockX / 2);
		BOOL bOk = FALSE;
		if(pwd->adfXShiftsPrevScale[iBlockIndexPrevScale] != INVALID_VALUE)
		{
			if(pwd->iScale < pwd->mfd->iStopScale)
			{
				// Don't find shifts at this scale, just transform the shifts from previous scale to this scale.
				pwd->adfXShifts[iBlockIndexThisScale] = 2.0 * pwd->adfXShiftsPrevScale[iBlockIndexPrevScale];
				pwd->adfYShifts[iBlockIndexThisScale] = 2.0 * pwd->adfYShiftsPrevScale[iBlockIndexPrevScale];
				bOk = TRUE;
			}
			else
			{
				// Find shift for a block. Use shift of previous scale as start and finetune it.
				int iInitShiftX = (int)(floor(2.0 * pwd->adfXShiftsPrevScale[iBlockIndexPrevScale] + 0.5) + 0.5);
				int iInitShiftY = (int)(floor(2.0 * pwd->adfYShiftsPrevScale[iBlockIndexPrevScale] + 0.5) + 0.5);
				int iBlockStart2X = iBlockStart1X + iInitShiftX;
				int iBlockStart2Y = iBlockStart1Y + iInitShiftY;
				double dfShiftX, dfShiftY;
				// Use a factor on some settings, to let more blocks through in smaller scales (because things might get better in larger scales).
				double dfFilterEffectFactor = max(0.0, 1.0 - (pwd->iScale - pwd->mfd->iStopScale) * 0.25);
				if(pwd->pimgThis->FindShift(*(pwd->pimgPrev), pwd->mfd->iSearchRange, pwd->mfd->iSearchRange, pwd->mfd->iBlockSize, pwd->mfd->iBlockSize, iBlockStart1X, iBlockStart1Y, iBlockStart2X, iBlockStart2Y, pwd->mfd->dfDiscardMatch / 1000.0, pwd->mfd->dfDiscard2ndMatch / 1000.0 * dfFilterEffectFactor, pwd->mfd->iMatchPixelInc, dfShiftX, dfShiftY, (int)(pwd->mfd->iDiscardPixelDiff * dfFilterEffectFactor + 0.5), min(36, pwd->mfd->iBlockSize * pwd->mfd->iBlockSize / pwd->mfd->iMatchPixelInc / pwd->mfd->iMatchPixelInc)))
				{
					pwd->adfXShifts[iBlockIndexThisScale] = iInitShiftX + dfShiftX;
					pwd->adfYShifts[iBlockIndexThisScale] = iInitShiftY + dfShiftY;
					bOk = TRUE;
				}
			}
		}

		if(!bOk)
		{
			pwd->adfXShifts[iBlockIndexThisScale] = INVALID_VALUE;
			pwd->adfYShifts[iBlockIndexThisScale] = INVALID_VALUE;
		}
	}

	return 0;
}
