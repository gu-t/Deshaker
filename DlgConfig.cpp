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



// The config dialog.

#include "stdafx.h"
#include <io.h>
#include "DeShaker.h"
#include "DlgConfig.h"
#include "DeShaker.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CDlgConfig::CDlgConfig(UINT uiDialogResource, CWnd* pParent /*=NULL*/)
	: CDialog(uiDialogResource, pParent)
{
	//{{AFX_DATA_INIT(CDlgConfig)
	m_iEdgeComp = 0;
	m_iResampling = 0;
	m_iScale = 0;
	m_iUsePixels = 0;
	m_iVideoOutput = 0;
	m_iBlockSize = 0;
	m_iDestHeight = 0;
	m_iDestWidth = 0;
	m_dfSkipFrame = 0.0;
	m_bNewScene = FALSE;
	m_dfNewScene = 0.0;
	m_iHorPan = 0;
	m_iInitSearchRange = 0;
	m_iRotation = 0;
	m_iSearchRange = 0;
	m_iVerPan = 0;
	m_iZoom = 0;
	m_iPass = 0;
	m_iDiscardPixelDiff = 0;
	m_dfDiscardBlockPixels = 0.0;
	m_dfDiscard2ndMatch = 0.0;
	m_dfDiscardMatch = 0.0;
	m_strLogFilePath = _T("");
	m_strLogFileName = _T("");
	m_bIgnoreInside = FALSE;
	m_bIgnoreOutside = FALSE;
	m_bLogFileAppend = FALSE;
	m_iIgnoreInsideBottom = 0;
	m_iIgnoreInsideLeft = 0;
	m_iIgnoreInsideRight = 0;
	m_iIgnoreInsideTop = 0;
	m_iIgnoreOutsideBottom = 0;
	m_iIgnoreOutsideLeft = 0;
	m_iIgnoreOutsideRight = 0;
	m_iIgnoreOutsideTop = 0;
	m_dfExtraZoom = 0.0;
	m_dfLimitHorPan = 0.0;
	m_dfLimitRotation = 0.0;
	m_dfLimitVerPan = 0.0;
	m_dfLimitZoom = 0.0;
	m_iVideoType = 0;
	m_bUseOldAndFutureFrames = FALSE;
	m_iFutureFrames = 0;
	m_iOldFrames = 0;
	m_bFollowIgnoreOutside = FALSE;
	m_iDeepAnalysisKickIn = 0;
	m_bERS = FALSE;
	m_bInterlacedProgressiveDest = FALSE;
	m_bDestSameAsSrc = FALSE;
	m_bExtrapolateBorder = FALSE;
	m_bMultiFrameBorder = TRUE;
	m_iEdgeTransitionPixels = 0;
	m_dfAbsPixelsToDiscardBlock = 0.0;
	m_bRememberDiscardedAreas = FALSE;
	m_iCPUs = 0;
	m_dfERSAmount = 0.0;
	m_bDetectRotation = FALSE;
	m_bDetectZoom = FALSE;
	m_bUseColorMask = FALSE;
	//}}AFX_DATA_INIT
}


void CDlgConfig::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgConfig)
	DDX_Control(pDX, CB_DEST_ASPECT, m_CBDestAspect);
	DDX_Control(pDX, CB_SRC_ASPECT, m_CBSrcAspect);
	DDX_CBIndex(pDX, CB_EDGE_COMP, m_iEdgeComp);
	DDX_CBIndex(pDX, CB_RESAMPLING, m_iResampling);
	DDX_CBIndex(pDX, CB_SCALE, m_iScale);
	DDX_CBIndex(pDX, CB_USE_PIXELS, m_iUsePixels);
	DDX_CBIndex(pDX, CB_VIDEO_OUTPUT, m_iVideoOutput);
	DDX_Text(pDX, ED_BLOCK_SIZE, m_iBlockSize);
	DDV_MinMaxInt(pDX, m_iBlockSize, 1, 100);
	DDX_Text(pDX, ED_DEST_HEIGHT, m_iDestHeight);
	DDV_MinMaxInt(pDX, m_iDestHeight, 1, 1000000);
	DDX_Text(pDX, ED_DEST_WIDTH, m_iDestWidth);
	DDV_MinMaxInt(pDX, m_iDestWidth, 1, 1000000);
	DDX_Text(pDX, ED_SKIP_FRAME, m_dfSkipFrame);
	DDV_MinMaxDouble(pDX, m_dfSkipFrame, 0.0, 100.0);
	DDX_Check(pDX, CX_NEW_SCENE, m_bNewScene);
	DDX_Text(pDX, ED_NEW_SCENE, m_dfNewScene);
	DDV_MinMaxDouble(pDX, m_dfNewScene, 0.0, 1000.0);
	DDX_Text(pDX, ED_HOR_PAN, m_iHorPan);
	DDV_MinMaxInt(pDX, m_iHorPan, -1, 1000000);
	DDX_Text(pDX, ED_INIT_SEARCH_RANGE, m_iInitSearchRange);
	DDV_MinMaxInt(pDX, m_iInitSearchRange, 1, 99);
	DDX_Text(pDX, ED_ROTATION, m_iRotation);
	DDV_MinMaxInt(pDX, m_iRotation, -1, 1000000);
	DDX_Text(pDX, ED_SEARCH_RANGE, m_iSearchRange);
	DDV_MinMaxInt(pDX, m_iSearchRange, 1, 50);
	DDX_Text(pDX, ED_VER_PAN, m_iVerPan);
	DDV_MinMaxInt(pDX, m_iVerPan, -1, 1000000);
	DDX_Text(pDX, ED_ZOOM, m_iZoom);
	DDV_MinMaxInt(pDX, m_iZoom, -1, 1000000);
	DDX_Radio(pDX, RA_PASS1, m_iPass);
	DDX_Text(pDX, ED_DISCARD_PIXEL_DIFF, m_iDiscardPixelDiff);
	DDV_MinMaxInt(pDX, m_iDiscardPixelDiff, 0, 255);
	DDX_Text(pDX, ED_DISCARD_BLOCK_PIXELS, m_dfDiscardBlockPixels);
	DDV_MinMaxDouble(pDX, m_dfDiscardBlockPixels, 1.e-002, 1000.);
	DDX_Text(pDX, ED_DISCARD_2ND_MATCH, m_dfDiscard2ndMatch);
	DDV_MinMaxDouble(pDX, m_dfDiscard2ndMatch, 0., 1000.);
	DDX_Text(pDX, ED_DISCARD_MATCH, m_dfDiscardMatch);
	DDV_MinMaxDouble(pDX, m_dfDiscardMatch, -1000., 1000.);
	DDX_Text(pDX, ED_LOGFILE, m_strLogFilePath);
	DDX_Text(pDX, ED_LOGFILE2, m_strLogFileName);
	DDX_Check(pDX, CX_IGNORE_INSIDE, m_bIgnoreInside);
	DDX_Check(pDX, CX_IGNORE_OUTSIDE, m_bIgnoreOutside);
	DDX_Check(pDX, CX_LOGFILE_APPEND, m_bLogFileAppend);
	DDX_Text(pDX, ED_IGNORE_INSIDE_BOTTOM, m_iIgnoreInsideBottom);
	DDX_Text(pDX, ED_IGNORE_INSIDE_LEFT, m_iIgnoreInsideLeft);
	DDX_Text(pDX, ED_IGNORE_INSIDE_RIGHT, m_iIgnoreInsideRight);
	DDX_Text(pDX, ED_IGNORE_INSIDE_TOP, m_iIgnoreInsideTop);
	DDX_Text(pDX, ED_IGNORE_OUTSIDE_BOTTOM, m_iIgnoreOutsideBottom);
	DDX_Text(pDX, ED_IGNORE_OUTSIDE_LEFT, m_iIgnoreOutsideLeft);
	DDX_Text(pDX, ED_IGNORE_OUTSIDE_RIGHT, m_iIgnoreOutsideRight);
	DDX_Text(pDX, ED_IGNORE_OUTSIDE_TOP, m_iIgnoreOutsideTop);
	DDX_Text(pDX, ED_EXTRA_ZOOM, m_dfExtraZoom);
	DDV_MinMaxDouble(pDX, m_dfExtraZoom, 1.e-002, 100.);
	DDX_Text(pDX, ED_LIMIT_HOR_PAN, m_dfLimitHorPan);
	DDV_MinMaxDouble(pDX, m_dfLimitHorPan, 0.0001, 1000.);
	DDX_Text(pDX, ED_LIMIT_ROTATION, m_dfLimitRotation);
	DDV_MinMaxDouble(pDX, m_dfLimitRotation, 0.0001, 1000.);
	DDX_Text(pDX, ED_LIMIT_VER_PAN, m_dfLimitVerPan);
	DDV_MinMaxDouble(pDX, m_dfLimitVerPan, 0.0001, 1000.);
	DDX_Text(pDX, ED_LIMIT_ZOOM, m_dfLimitZoom);
	DDV_MinMaxDouble(pDX, m_dfLimitZoom, 0.0001, 1000.);
	DDX_CBIndex(pDX, CB_VIDEO_TYPE, m_iVideoType);
	DDX_Check(pDX, CX_USE_OLD_AND_FUTURE_FRAMES, m_bUseOldAndFutureFrames);
	DDX_Text(pDX, ED_FUTURE_FRAMES, m_iFutureFrames);
	DDV_MinMaxInt(pDX, m_iFutureFrames, 0, 1000);
	DDX_Text(pDX, ED_OLD_FRAMES, m_iOldFrames);
	DDV_MinMaxInt(pDX, m_iOldFrames, 0, 1000);
	DDX_Check(pDX, CX_FOLLOW_IGNORE_OUTSIDE, m_bFollowIgnoreOutside);
	DDX_Text(pDX, ED_DEEP_ANALYSIS_KICK_IN, m_iDeepAnalysisKickIn);
	DDV_MinMaxInt(pDX, m_iDeepAnalysisKickIn, 0, 100);
	DDX_Check(pDX, CX_ERS, m_bERS);
	DDX_Check(pDX, CX_INTERLACED_PROGRESSIVE_DEST, m_bInterlacedProgressiveDest);
	DDX_Check(pDX, CX_DEST_SAME_AS_SRC, m_bDestSameAsSrc);
	DDX_Check(pDX, CX_EXTRAPOLATE_BORDER, m_bExtrapolateBorder);
	DDX_Check(pDX, CX_MULTI_FRAME_BORDER, m_bMultiFrameBorder);
	DDX_Text(pDX, ED_EDGE_TRANSITION_PIXELS, m_iEdgeTransitionPixels);
	DDV_MinMaxInt(pDX, m_iEdgeTransitionPixels, 0, 100);
	DDX_Text(pDX, ED_DISCARD_BLOCK_ABS_PIXELS, m_dfAbsPixelsToDiscardBlock);
	DDX_Check(pDX, CX_REMEMBER_DISCARDED_AREAS, m_bRememberDiscardedAreas);
	DDX_Text(pDX, TE_CPUS, m_iCPUs);
	DDX_Text(pDX, ED_ERS, m_dfERSAmount);
	DDX_Check(pDX, CX_DETECT_ROTATION, m_bDetectRotation);
	DDX_Check(pDX, CX_DETECT_ZOOM, m_bDetectZoom);
	DDX_Text(pDX, ED_ADAPTIVE_ZOOM_SMOOTHNESS, m_dfAdaptiveZoomSmoothness);
	DDV_MinMaxDouble(pDX, m_dfAdaptiveZoomSmoothness, 0.1, 1000000);
	DDX_Text(pDX, ED_ADAPTIVE_ZOOM_AMOUNT, m_dfAdaptiveZoomAmount);
	DDV_MinMaxDouble(pDX, m_dfAdaptiveZoomAmount, -1000, 1000);
	DDX_Check(pDX, CX_USE_COLOR_MASK, m_bUseColorMask);
	DDX_Control(pDX, CBT_MASK_COLOR, m_CBTMaskColor);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgConfig, CDialog)
	//{{AFX_MSG_MAP(CDlgConfig)
	ON_BN_CLICKED(RA_PASS1, OnPass)
	ON_BN_CLICKED(CX_IGNORE_OUTSIDE, OnIgnoreOutside)
	ON_BN_CLICKED(CX_IGNORE_INSIDE, OnIgnoreInside)
	ON_BN_CLICKED(PB_LOGFILE_BROWSE, OnLogFileBrowse)
	ON_BN_CLICKED(CX_USE_OLD_AND_FUTURE_FRAMES, OnUseOldAndFutureFrames)
	ON_CBN_SELCHANGE(CB_EDGE_COMP, OnSelchangeEdgeComp)
	ON_BN_CLICKED(CX_ERS, OnERS)
	ON_CBN_SELCHANGE(CB_VIDEO_TYPE, OnSelchangeVideoType)
	ON_BN_CLICKED(CX_DEST_SAME_AS_SRC, OnDestSameAsSrc)
	ON_BN_CLICKED(PB_VISIT_WEB, OnVisitWeb)
	ON_BN_CLICKED(RA_PASS2, OnPass)
	ON_BN_CLICKED(CX_MULTI_FRAME_BORDER, OnMultiFrameBorder)
	ON_BN_CLICKED(PB_LOGFILE_OPEN, OnBnClickedLogfileOpen)
	ON_BN_CLICKED(PB_LOGFILE_DELETE, OnBnClickedLogfileDelete)
	ON_EN_KILLFOCUS(ED_LOGFILE, OnEnKillfocusLogfile)
	ON_BN_CLICKED(PB_LOGFILE_OPEN_FOLDER, OnBnClickedLogfileOpenFolder)
	ON_BN_CLICKED(CX_REMEMBER_DISCARDED_AREAS, OnRememberDiscardedAreas)
	ON_BN_CLICKED(CX_NEW_SCENE, OnNewScene)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(CX_USE_COLOR_MASK, &CDlgConfig::OnBnClickedUseColorMask)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgConfig message handlers

BOOL CDlgConfig::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_iCPUs = g_iCPUs;

	CString str;

	// Copy values from m_mfd to dialog variables
	switch(m_mfd->iEdgeComp)
	{
	case EDGECOMP_NONE:
		m_iEdgeComp = 0;
		break;
	case EDGECOMP_ZOOM_ADAPTIVE_AVG:
		m_iEdgeComp = 1;
		break;
	case EDGECOMP_ZOOM_ADAPTIVE_FULL:
		m_iEdgeComp = 2;
		break;
	case EDGECOMP_ZOOM_FIXED:
		m_iEdgeComp = 3;
		break;
	case EDGECOMP_ZOOM_ADAPTIVE_AVG_PLUS_FIXED:
		m_iEdgeComp = 4;
		break;
	default:
		ASSERT(FALSE);
		m_iEdgeComp = 0;
	}
	
	m_iVideoOutput = m_mfd->iVideoOutput;

	m_CBDestAspect.SetCurSel(m_mfd->iPixelAspectDest);
	if(m_mfd->iPixelAspectDest < 0)
	{
		str.Format("%lg", m_mfd->dfPixelAspectDest);
		m_CBDestAspect.SetWindowText(str);
	}

	m_CBSrcAspect.SetCurSel(m_mfd->iPixelAspectSrc);
	if(m_mfd->iPixelAspectSrc < 0)
	{
		str.Format("%lg", m_mfd->dfPixelAspectSrc);
		m_CBSrcAspect.SetWindowText(str);
	}

	m_iBlockSize = m_mfd->iBlockSize;

	m_iDestHeight = m_mfd->iDestHeight;
	m_iDestWidth = m_mfd->iDestWidth;

	m_iUsePixels = m_mfd->iMatchPixelInc - 1;

	m_iPass = m_mfd->iPass - 1;

	m_iDiscardPixelDiff = m_mfd->iDiscardPixelDiff;
	m_dfDiscardBlockPixels = m_mfd->dfPixelsOffToDiscardBlock;
	m_dfDiscardMatch = m_mfd->dfDiscardMatch;
	m_dfDiscard2ndMatch = m_mfd->dfDiscard2ndMatch;

	m_iRotation = m_mfd->iRotateSmoothness;
	m_iZoom = m_mfd->iZoomSmoothness;
	m_iHorPan = m_mfd->iShiftXSmoothness;
	m_iVerPan = m_mfd->iShiftYSmoothness;

	m_dfLimitRotation = m_mfd->dfLimitRotation;
	m_dfLimitZoom = m_mfd->dfLimitZoom;
	m_dfLimitHorPan = m_mfd->dfLimitShiftX;
	m_dfLimitVerPan = m_mfd->dfLimitShiftY;

	m_dfExtraZoom = m_mfd->dfExtraZoom;

	m_iSearchRange = m_mfd->iSearchRange;
	m_iInitSearchRange = m_mfd->iInitialSearchRange;

	m_iScale = m_mfd->iStopScale;

	m_iResampling = m_mfd->iResampleAlg;

	m_dfSkipFrame = m_mfd->dfSkipFrameBlockPercent;
	m_bNewScene = m_mfd->bDetectScenes;
	m_dfNewScene = m_mfd->dfNewSceneThreshold;

	m_strLogFilePath = "";
	m_strLogFileName = *m_mfd->pstrLogFile;
	int iIndex = m_strLogFileName.ReverseFind('\\');
	if(iIndex >= 0)
	{
		m_strLogFilePath = m_strLogFileName.Left(iIndex + 1);
		m_strLogFileName = m_strLogFileName.Mid(iIndex + 1);
	}

	m_bLogFileAppend = m_mfd->bLogFileAppend;

	m_bIgnoreInside = m_mfd->bIgnoreInside;
	m_iIgnoreInsideLeft = m_mfd->rectIgnoreInside.left;
	m_iIgnoreInsideRight = m_mfd->rectIgnoreInside.right;
	m_iIgnoreInsideTop = m_mfd->rectIgnoreInside.top;
	m_iIgnoreInsideBottom = m_mfd->rectIgnoreInside.bottom;
	m_bIgnoreOutside = m_mfd->bIgnoreOutside;
	m_iIgnoreOutsideLeft = round(m_mfd->rectIgnoreOutside.left);
	m_iIgnoreOutsideRight = round(m_mfd->rectIgnoreOutside.right);
	m_iIgnoreOutsideTop = round(m_mfd->rectIgnoreOutside.top);
	m_iIgnoreOutsideBottom = round(m_mfd->rectIgnoreOutside.bottom);
	m_bFollowIgnoreOutside = m_mfd->bFollowIgnoreOutside;

	m_iVideoType = m_mfd->bInterlaced ? (m_mfd->bTFF ? 1 : 2) : 0;

	m_bUseOldAndFutureFrames = m_mfd->bUseOldFrames || m_mfd->bUseFutureFrames;
	m_iOldFrames = m_mfd->iOldFrames;
	m_iFutureFrames = m_mfd->iFutureFrames;

	m_iDeepAnalysisKickIn = m_mfd->iDeepAnalysisKickIn;

	m_bERS = m_mfd->bERS;

	m_bInterlacedProgressiveDest = m_mfd->bInterlacedProgressiveDest;

	m_bDestSameAsSrc = m_mfd->bDestSameAsSrc;
	m_bMultiFrameBorder = m_mfd->bMultiFrameBorder;
	m_bExtrapolateBorder = m_mfd->bExtrapolateBorder;
	m_iEdgeTransitionPixels = m_mfd->iEdgeTransitionPixels;
	m_dfAbsPixelsToDiscardBlock = m_mfd->dfAbsPixelsToDiscardBlock;
	m_bRememberDiscardedAreas = m_mfd->bRememberDiscardedAreas;
	m_dfERSAmount = m_mfd->dfERSAmount;
	m_bDetectRotation = m_mfd->bDetectRotation;
	m_bDetectZoom = m_mfd->bDetectZoom;
	m_dfAdaptiveZoomSmoothness = m_mfd->dfAdaptiveZoomSmoothness;
	m_dfAdaptiveZoomAmount = m_mfd->dfAdaptiveZoomAmount;
	m_bUseColorMask = m_mfd->bUseColorMask;

	UpdateData(FALSE);

	m_CBTMaskColor.SetColor(m_mfd->uiMaskColor >> 16 & 0xff | m_mfd->uiMaskColor & 0xff00 | m_mfd->uiMaskColor << 16 & 0xff0000);

	EnableDisableCtrls();
	GetDlgItem(IDOK)->SetFocus();
	
	return FALSE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CDlgConfig::OnOK() 
{
	FixLogFilePath();

	if(!UpdateData())
		return;

	CString str;

	// Copy values from dialog variables to m_mfd.
	switch(m_iEdgeComp)
	{
	case 0:
		m_mfd->iEdgeComp = EDGECOMP_NONE;
		break;
	case 1:
		m_mfd->iEdgeComp = EDGECOMP_ZOOM_ADAPTIVE_AVG;
		break;
	case 2:
		m_mfd->iEdgeComp = EDGECOMP_ZOOM_ADAPTIVE_FULL;
		break;
	case 3:
		m_mfd->iEdgeComp = EDGECOMP_ZOOM_FIXED;
		break;
	case 4:
		m_mfd->iEdgeComp = EDGECOMP_ZOOM_ADAPTIVE_AVG_PLUS_FIXED;
		break;
	default:
		ASSERT(FALSE);
		m_mfd->iEdgeComp = EDGECOMP_NONE;
	}

	m_mfd->iVideoOutput = m_iVideoOutput;

	m_mfd->iPixelAspectDest = m_CBDestAspect.GetCurSel();
	switch(m_mfd->iPixelAspectDest)
	{
	case 0:
		m_mfd->dfPixelAspectDest = 1.0;
		break;
	case 1:
		m_mfd->dfPixelAspectDest = 59.0 / 54.0;
		break;
	case 2:
		m_mfd->dfPixelAspectDest = 10.0 / 11.0;
		break;
	case 3:
		m_mfd->dfPixelAspectDest = 118.0 / 81.0;
		break;
	case 4:
		m_mfd->dfPixelAspectDest = 40.0 / 33.0;
		break;
	case 5:
		m_mfd->dfPixelAspectDest = 59.0 / 36.0;
		break;
	case 6:
		m_mfd->dfPixelAspectDest = 15.0 / 11.0;
		break;
	case 7:
		m_mfd->dfPixelAspectDest = 4.0 / 3.0;
		break;
	default:
		m_CBDestAspect.GetWindowText(str);
		m_mfd->dfPixelAspectDest = atof(str);
		break;
	}
	
	m_mfd->iPixelAspectSrc = m_CBSrcAspect.GetCurSel();
	switch(m_mfd->iPixelAspectSrc)
	{
	case 0:
		m_mfd->dfPixelAspectSrc = 1.0;
		break;
	case 1:
		m_mfd->dfPixelAspectSrc = 59.0 / 54.0;
		break;
	case 2:
		m_mfd->dfPixelAspectSrc = 10.0 / 11.0;
		break;
	case 3:
		m_mfd->dfPixelAspectSrc = 118.0 / 81.0;
		break;
	case 4:
		m_mfd->dfPixelAspectSrc = 40.0 / 33.0;
		break;
	case 5:
		m_mfd->dfPixelAspectSrc = 59.0 / 36.0;
		break;
	case 6:
		m_mfd->dfPixelAspectSrc = 15.0 / 11.0;
		break;
	case 7:
		m_mfd->dfPixelAspectSrc = 4.0 / 3.0;
		break;
	default:
		m_CBSrcAspect.GetWindowText(str);
		m_mfd->dfPixelAspectSrc = atof(str);
		break;
	}

	m_mfd->iBlockSize = m_iBlockSize;

	m_mfd->iDestHeight = m_iDestHeight;
	m_mfd->iDestWidth = m_iDestWidth;

	m_mfd->iMatchPixelInc = m_iUsePixels + 1;

	m_mfd->iPass = m_iPass + 1;

	m_mfd->iDiscardPixelDiff = m_iDiscardPixelDiff;
	m_mfd->dfPixelsOffToDiscardBlock = m_dfDiscardBlockPixels;
	m_mfd->dfDiscardMatch = m_dfDiscardMatch;
	m_mfd->dfDiscard2ndMatch = m_dfDiscard2ndMatch;

	m_mfd->iRotateSmoothness = m_iRotation;
	m_mfd->iZoomSmoothness = m_iZoom;
	m_mfd->iShiftXSmoothness = m_iHorPan;
	m_mfd->iShiftYSmoothness = m_iVerPan;

	m_mfd->dfLimitRotation = m_dfLimitRotation;
	m_mfd->dfLimitZoom = m_dfLimitZoom;
	m_mfd->dfLimitShiftX = m_dfLimitHorPan;
	m_mfd->dfLimitShiftY = m_dfLimitVerPan;

	m_mfd->dfExtraZoom = m_dfExtraZoom;

	m_mfd->iSearchRange = m_iSearchRange;
	m_mfd->iInitialSearchRange = m_iInitSearchRange;

	m_mfd->iStopScale = m_iScale;

	m_mfd->iResampleAlg = m_iResampling;

	m_mfd->dfSkipFrameBlockPercent = m_dfSkipFrame;
	m_mfd->bDetectScenes = m_bNewScene;
	m_mfd->dfNewSceneThreshold = m_dfNewScene;

	*m_mfd->pstrLogFile = m_strLogFilePath + m_strLogFileName;
	m_mfd->bLogFileAppend = m_bLogFileAppend;

	m_mfd->bIgnoreInside = m_bIgnoreInside;
	m_mfd->rectIgnoreInside.left = m_iIgnoreInsideLeft;
	m_mfd->rectIgnoreInside.right = m_iIgnoreInsideRight;
	m_mfd->rectIgnoreInside.top = m_iIgnoreInsideTop;
	m_mfd->rectIgnoreInside.bottom = m_iIgnoreInsideBottom;
	m_mfd->bIgnoreOutside = m_bIgnoreOutside;
	m_mfd->rectIgnoreOutside.left = m_iIgnoreOutsideLeft;
	m_mfd->rectIgnoreOutside.right = m_iIgnoreOutsideRight;
	m_mfd->rectIgnoreOutside.top = m_iIgnoreOutsideTop;
	m_mfd->rectIgnoreOutside.bottom = m_iIgnoreOutsideBottom;
	m_mfd->bFollowIgnoreOutside = m_bFollowIgnoreOutside;

	m_mfd->bInterlaced = m_iVideoType > 0;
	m_mfd->bTFF = m_iVideoType == 1;

	m_mfd->bUseOldFrames = m_bUseOldAndFutureFrames;
	m_mfd->bUseFutureFrames = m_bUseOldAndFutureFrames;
	m_mfd->iOldFrames = m_iOldFrames;
	m_mfd->iFutureFrames = m_iFutureFrames;

	m_mfd->iDeepAnalysisKickIn = m_iDeepAnalysisKickIn;

	m_mfd->bERS = m_bERS;

	m_mfd->bInterlacedProgressiveDest = m_bInterlacedProgressiveDest;

	m_mfd->bDestSameAsSrc = m_bDestSameAsSrc;
	m_mfd->bMultiFrameBorder = m_bMultiFrameBorder;
	m_mfd->bExtrapolateBorder = m_bExtrapolateBorder;
	m_mfd->iEdgeTransitionPixels = m_iEdgeTransitionPixels;
	m_mfd->dfAbsPixelsToDiscardBlock = m_dfAbsPixelsToDiscardBlock;
	m_mfd->bRememberDiscardedAreas = m_bRememberDiscardedAreas;
	m_mfd->dfERSAmount = m_dfERSAmount;
	m_mfd->bDetectRotation = m_bDetectRotation;
	m_mfd->bDetectZoom = m_bDetectZoom;
	m_mfd->dfAdaptiveZoomSmoothness = m_dfAdaptiveZoomSmoothness;
	m_mfd->dfAdaptiveZoomAmount = m_dfAdaptiveZoomAmount;
	m_mfd->bUseColorMask = m_bUseColorMask;
	m_mfd->uiMaskColor = m_CBTMaskColor.GetColor();
	m_mfd->uiMaskColor = m_mfd->uiMaskColor >> 16 & 0xff | m_mfd->uiMaskColor & 0xff00 | m_mfd->uiMaskColor << 16 & 0xff0000;

	CDialog::OnOK();
}

void CDlgConfig::OnPass() 
{
	EnableDisableCtrls();
}

void CDlgConfig::OnIgnoreOutside() 
{
	EnableDisableCtrls();
}

void CDlgConfig::OnIgnoreInside() 
{
	EnableDisableCtrls();
}

void CDlgConfig::OnUseOldAndFutureFrames() 
{
	EnableDisableCtrls();
}

void CDlgConfig::OnSelchangeEdgeComp() 
{
	EnableDisableCtrls();
}

void CDlgConfig::OnERS() 
{
	EnableDisableCtrls();
}

void CDlgConfig::OnSelchangeVideoType() 
{
	EnableDisableCtrls();
}

void CDlgConfig::OnDestSameAsSrc() 
{
	EnableDisableCtrls();
}

void CDlgConfig::OnMultiFrameBorder() 
{
	EnableDisableCtrls();
}

void CDlgConfig::OnNewScene() 
{
	EnableDisableCtrls();
}

void CDlgConfig::OnBnClickedUseColorMask()
{
	EnableDisableCtrls();
}

/// <summary>
/// Enables/disables certain dialog controls depending on the state of other controls.
/// </summary>
void CDlgConfig::EnableDisableCtrls()
{
	BOOL bPass1 = ((CButton *)GetDlgItem(RA_PASS1))->GetCheck() == 1;
	BOOL bIgnoreOutside = ((CButton *)GetDlgItem(CX_IGNORE_OUTSIDE))->GetCheck() == 1;
	BOOL bIgnoreInside = ((CButton *)GetDlgItem(CX_IGNORE_INSIDE))->GetCheck() == 1;
	BOOL bUseOldAndFutureFrames = ((CButton *)GetDlgItem(CX_USE_OLD_AND_FUTURE_FRAMES))->GetCheck() == 1;
	BOOL bERS = ((CButton *)GetDlgItem(CX_ERS))->GetCheck() == 1;
	BOOL bInterlaced = ((CComboBox *)GetDlgItem(CB_VIDEO_TYPE))->GetCurSel() > 0;
	BOOL bDestSameAsSrc = ((CButton *)GetDlgItem(CX_DEST_SAME_AS_SRC))->GetCheck() == 1;
	BOOL bMultiFrameBorder = ((CButton *)GetDlgItem(CX_MULTI_FRAME_BORDER))->GetCheck() == 1;
	int iAdaptiveZoomSel = ((CComboBox *)GetDlgItem(CB_EDGE_COMP))->GetCurSel();
	BOOL bAdaptiveZoom = iAdaptiveZoomSel == 1 || iAdaptiveZoomSel == 2 || iAdaptiveZoomSel == 4;
	BOOL bNewScene = ((CButton *)GetDlgItem(CX_NEW_SCENE))->GetCheck() == 1;
	BOOL bUseColorMask = ((CButton *)GetDlgItem(CX_USE_COLOR_MASK))->GetCheck() == 1;

	GetDlgItem(ST_ERS)->EnableWindow(bERS);
	GetDlgItem(ED_ERS)->EnableWindow(bERS);
	GetDlgItem(ST_ERS2)->EnableWindow(bERS);
	GetDlgItem(CBT_MASK_COLOR)->EnableWindow(bUseColorMask);
	
	GetDlgItem(GR_PASS1)->EnableWindow(bPass1);
	GetDlgItem(ST_VIDEO_OUTPUT)->EnableWindow(bPass1);
	GetDlgItem(CB_VIDEO_OUTPUT)->EnableWindow(bPass1);
	GetDlgItem(GR_MATCHING_PARAMS)->EnableWindow(bPass1);
	GetDlgItem(ST_BLOCK_SIZE)->EnableWindow(bPass1);
	GetDlgItem(ED_BLOCK_SIZE)->EnableWindow(bPass1);
	GetDlgItem(ST_BLOCK_SIZE2)->EnableWindow(bPass1);
	GetDlgItem(ST_SCALE)->EnableWindow(bPass1);
	GetDlgItem(CB_SCALE)->EnableWindow(bPass1);
	GetDlgItem(ST_USE_PIXELS)->EnableWindow(bPass1);
	GetDlgItem(CB_USE_PIXELS)->EnableWindow(bPass1);
	GetDlgItem(ST_INIT_SEARCH_RANGE)->EnableWindow(bPass1);
	GetDlgItem(ED_INIT_SEARCH_RANGE)->EnableWindow(bPass1);
	GetDlgItem(ST_INIT_SEARCH_RANGE2)->EnableWindow(bPass1);
	GetDlgItem(ST_SEARCH_RANGE)->EnableWindow(bPass1);
	GetDlgItem(ED_SEARCH_RANGE)->EnableWindow(bPass1);
	GetDlgItem(ST_SEARCH_RANGE2)->EnableWindow(bPass1);
	GetDlgItem(CX_DETECT_ROTATION)->EnableWindow(bPass1);
	GetDlgItem(CX_DETECT_ZOOM)->EnableWindow(bPass1);
	GetDlgItem(GR_DISCARD)->EnableWindow(bPass1);
	GetDlgItem(ST_DISCARD_PIXEL_DIFF)->EnableWindow(bPass1);
	GetDlgItem(ED_DISCARD_PIXEL_DIFF)->EnableWindow(bPass1);
	GetDlgItem(ST_DISCARD_PIXEL_DIFF2)->EnableWindow(bPass1);
	GetDlgItem(ST_DISCARD_MATCH)->EnableWindow(bPass1);
	GetDlgItem(ED_DISCARD_MATCH)->EnableWindow(bPass1);
	GetDlgItem(ST_DISCARD_MATCH2)->EnableWindow(bPass1);
	GetDlgItem(ST_DISCARD_2ND_MATCH)->EnableWindow(bPass1);
	GetDlgItem(ED_DISCARD_2ND_MATCH)->EnableWindow(bPass1);
	GetDlgItem(ST_DISCARD_BLOCK_PIXELS)->EnableWindow(bPass1);
	GetDlgItem(ED_DISCARD_BLOCK_PIXELS)->EnableWindow(bPass1);
	GetDlgItem(ST_DISCARD_BLOCK_PIXELS2)->EnableWindow(bPass1);
	GetDlgItem(ST_DISCARD_BLOCK_ABS_PIXELS)->EnableWindow(bPass1);
	GetDlgItem(ED_DISCARD_BLOCK_ABS_PIXELS)->EnableWindow(bPass1);
	GetDlgItem(ST_DISCARD_BLOCK_ABS_PIXELS2)->EnableWindow(bPass1);
	GetDlgItem(CX_REMEMBER_DISCARDED_AREAS)->EnableWindow(bPass1);
	GetDlgItem(ST_DEEP_ANALYSIS_KICK_IN)->EnableWindow(bPass1 && !bERS);
	GetDlgItem(ED_DEEP_ANALYSIS_KICK_IN)->EnableWindow(bPass1 && !bERS);
	GetDlgItem(ST_DEEP_ANALYSIS_KICK_IN2)->EnableWindow(bPass1 && !bERS);
	GetDlgItem(ST_SKIP_FRAME)->EnableWindow(bPass1);
	GetDlgItem(ED_SKIP_FRAME)->EnableWindow(bPass1);
	GetDlgItem(ST_SKIP_FRAME2)->EnableWindow(bPass1);
	GetDlgItem(CX_NEW_SCENE)->EnableWindow(bPass1);
	GetDlgItem(ED_NEW_SCENE)->EnableWindow(bPass1 && bNewScene);
	GetDlgItem(GR_IGNORE_AREA)->EnableWindow(bPass1);
	GetDlgItem(ST_FROM_BORDER)->EnableWindow(bPass1);
	GetDlgItem(ST_IGNORE_LEFT)->EnableWindow(bPass1);
	GetDlgItem(ST_IGNORE_RIGHT)->EnableWindow(bPass1);
	GetDlgItem(ST_IGNORE_TOP)->EnableWindow(bPass1);
	GetDlgItem(ST_IGNORE_BOTTOM)->EnableWindow(bPass1);
	GetDlgItem(CX_IGNORE_OUTSIDE)->EnableWindow(bPass1);
	GetDlgItem(ED_IGNORE_OUTSIDE_LEFT)->EnableWindow(bPass1 && bIgnoreOutside);
	GetDlgItem(ED_IGNORE_OUTSIDE_RIGHT)->EnableWindow(bPass1 && bIgnoreOutside);
	GetDlgItem(ED_IGNORE_OUTSIDE_TOP)->EnableWindow(bPass1 && bIgnoreOutside);
	GetDlgItem(ED_IGNORE_OUTSIDE_BOTTOM)->EnableWindow(bPass1 && bIgnoreOutside);
	GetDlgItem(CX_FOLLOW_IGNORE_OUTSIDE)->EnableWindow(bPass1 && bIgnoreOutside);
	GetDlgItem(CX_IGNORE_INSIDE)->EnableWindow(bPass1);
	GetDlgItem(ED_IGNORE_INSIDE_LEFT)->EnableWindow(bPass1 && bIgnoreInside);
	GetDlgItem(ED_IGNORE_INSIDE_RIGHT)->EnableWindow(bPass1 && bIgnoreInside);
	GetDlgItem(ED_IGNORE_INSIDE_TOP)->EnableWindow(bPass1 && bIgnoreInside);
	GetDlgItem(ED_IGNORE_INSIDE_BOTTOM)->EnableWindow(bPass1 && bIgnoreInside);

	GetDlgItem(GR_PASS2)->EnableWindow(!bPass1);
	GetDlgItem(CX_DEST_SAME_AS_SRC)->EnableWindow(!bPass1);
	GetDlgItem(ST_DEST_ASPECT)->EnableWindow(!bPass1 && !bDestSameAsSrc);
	GetDlgItem(CB_DEST_ASPECT)->EnableWindow(!bPass1 && !bDestSameAsSrc);
	GetDlgItem(ST_DEST_SIZE)->EnableWindow(!bPass1 && !bDestSameAsSrc);
	GetDlgItem(ED_DEST_WIDTH)->EnableWindow(!bPass1 && !bDestSameAsSrc);
	GetDlgItem(ST_DEST_SIZE2)->EnableWindow(!bPass1 && !bDestSameAsSrc);
	GetDlgItem(ED_DEST_HEIGHT)->EnableWindow(!bPass1 && !bDestSameAsSrc);
	GetDlgItem(CX_INTERLACED_PROGRESSIVE_DEST)->EnableWindow(!bPass1 && bInterlaced);
	GetDlgItem(ST_RESAMPLING)->EnableWindow(!bPass1);
	GetDlgItem(CB_RESAMPLING)->EnableWindow(!bPass1);
	GetDlgItem(ST_EDGE_COMP)->EnableWindow(!bPass1);
	GetDlgItem(CB_EDGE_COMP)->EnableWindow(!bPass1);
	GetDlgItem(CX_USE_OLD_AND_FUTURE_FRAMES)->EnableWindow(!bPass1);
	GetDlgItem(ST_OLD_FRAMES)->EnableWindow(!bPass1 && bUseOldAndFutureFrames);
	GetDlgItem(ED_OLD_FRAMES)->EnableWindow(!bPass1 && bUseOldAndFutureFrames);
	GetDlgItem(ST_FUTURE_FRAMES)->EnableWindow(!bPass1 && bUseOldAndFutureFrames);
	GetDlgItem(ED_FUTURE_FRAMES)->EnableWindow(!bPass1 && bUseOldAndFutureFrames);
	GetDlgItem(CX_MULTI_FRAME_BORDER)->EnableWindow(!bPass1 && bUseOldAndFutureFrames);
	GetDlgItem(ST_MULTI_FRAME_BORDER)->EnableWindow(!bPass1 && bUseOldAndFutureFrames && bMultiFrameBorder);
	GetDlgItem(ED_EDGE_TRANSITION_PIXELS)->EnableWindow(!bPass1 && bUseOldAndFutureFrames && bMultiFrameBorder);
	GetDlgItem(ST_MULTI_FRAME_BORDER2)->EnableWindow(!bPass1 && bUseOldAndFutureFrames && bMultiFrameBorder);
	GetDlgItem(CX_EXTRAPOLATE_BORDER)->EnableWindow(!bPass1);
	GetDlgItem(ST_EXTRA_ZOOM)->EnableWindow(!bPass1);
	GetDlgItem(ED_EXTRA_ZOOM)->EnableWindow(!bPass1);
	GetDlgItem(GR_SMOOTHNESS)->EnableWindow(!bPass1);
	GetDlgItem(ST_HOR_PAN)->EnableWindow(!bPass1);
	GetDlgItem(ED_HOR_PAN)->EnableWindow(!bPass1);
	GetDlgItem(ST_VER_PAN)->EnableWindow(!bPass1);
	GetDlgItem(ED_VER_PAN)->EnableWindow(!bPass1);
	GetDlgItem(ST_ROTATION)->EnableWindow(!bPass1);
	GetDlgItem(ED_ROTATION)->EnableWindow(!bPass1);
	GetDlgItem(ST_ZOOM)->EnableWindow(!bPass1);
	GetDlgItem(ED_ZOOM)->EnableWindow(!bPass1);
	GetDlgItem(ST_ADAPTIVE_ZOOM)->EnableWindow(!bPass1 && bAdaptiveZoom);
	GetDlgItem(ED_ADAPTIVE_ZOOM_SMOOTHNESS)->EnableWindow(!bPass1 && bAdaptiveZoom);
	GetDlgItem(ST_ADAPTIVE_ZOOM2)->EnableWindow(!bPass1 && bAdaptiveZoom);
	GetDlgItem(ED_ADAPTIVE_ZOOM_AMOUNT)->EnableWindow(!bPass1 && bAdaptiveZoom);
	GetDlgItem(ST_ADAPTIVE_ZOOM3)->EnableWindow(!bPass1 && bAdaptiveZoom);
	GetDlgItem(GR_LIMITS)->EnableWindow(!bPass1);
	GetDlgItem(ST_LIMIT_HOR_PAN)->EnableWindow(!bPass1);
	GetDlgItem(ED_LIMIT_HOR_PAN)->EnableWindow(!bPass1);
	GetDlgItem(ST_LIMIT_VER_PAN)->EnableWindow(!bPass1);
	GetDlgItem(ED_LIMIT_VER_PAN)->EnableWindow(!bPass1);
	GetDlgItem(ST_LIMIT_ROTATION)->EnableWindow(!bPass1);
	GetDlgItem(ED_LIMIT_ROTATION)->EnableWindow(!bPass1);
	GetDlgItem(ST_LIMIT_ZOOM)->EnableWindow(!bPass1);
	GetDlgItem(ED_LIMIT_ZOOM)->EnableWindow(!bPass1);
	GetDlgItem(GR_LIMITS)->EnableWindow(!bPass1);
}

/// <summary>
/// Browses for logfile.
/// </summary>
void CDlgConfig::OnLogFileBrowse() 
{
	FixLogFilePath();
	CString strFilename, strFilepath;
	GetDlgItem(ED_LOGFILE)->GetWindowText(strFilepath);
	GetDlgItem(ED_LOGFILE2)->GetWindowText(strFilename);
	CFileDialog dlgFileSel(FALSE, NULL, strFilepath + strFilename, OFN_EXPLORER | OFN_PATHMUSTEXIST, "Log Files (*.log)|*.log|All Files (*.*)|*.*||", this);
	dlgFileSel.m_ofn.lpstrTitle = "Select Deshaker Log File";
	if(dlgFileSel.DoModal() == IDOK)
	{
		strFilename = dlgFileSel.GetPathName();
		if(dlgFileSel.GetFileExt().IsEmpty())
			strFilename += ".log";

		int iIndex = strFilename.ReverseFind('\\');
		if(iIndex >= 0)
		{
			strFilepath = strFilename.Left(iIndex + 1);
			strFilename = strFilename.Mid(iIndex + 1);
		}
		GetDlgItem(ED_LOGFILE)->SetWindowText(strFilepath);
		GetDlgItem(ED_LOGFILE2)->SetWindowText(strFilename);
	}
}

/// <summary>
/// Visits Deshaker web page.
/// </summary>
void CDlgConfig::OnVisitWeb() 
{
	ShellExecute(NULL, NULL, "https://www.guthspot.se/video/deshaker.htm", NULL, NULL, SW_SHOWNORMAL);

	/*	// This code opens the web page in a *new* web browser, but doesn't seem to work for all users.
	HKEY Key;
	if(RegOpenKeyEx(HKEY_CLASSES_ROOT,".htm",0,KEY_READ,&Key)==ERROR_SUCCESS)
	{
		DWORD dwSize=2*MAX_PATH;
		char Browser[2*MAX_PATH];
		if(RegQueryValueEx(Key,"",NULL,NULL,(BYTE*)&Browser,&dwSize)==ERROR_SUCCESS)
		{
			RegCloseKey(Key);
			strcat_s(Browser, 2*MAX_PATH, "\\shell\\open\\command");
			if(RegOpenKeyEx(HKEY_CLASSES_ROOT,Browser,0,KEY_READ,&Key)==ERROR_SUCCESS)
			{
				dwSize=2*MAX_PATH;
				if(RegQueryValueEx(Key,"",NULL,NULL,(BYTE*)&Browser,&dwSize)==ERROR_SUCCESS)
				{
					strcat_s(Browser, 2*MAX_PATH, " \"https://www.guthspot.se/video/deshaker.htm\"");
					WinExec(Browser, SW_SHOWNORMAL);
				}
			}
		}
		RegCloseKey(Key);
	}*/
}

/// <summary>
/// Opens logfile folder in explorer.
/// </summary>
void CDlgConfig::OnBnClickedLogfileOpenFolder()
{
	FixLogFilePath();
	CString strFilepath;
	GetDlgItem(ED_LOGFILE)->GetWindowText(strFilepath);
	if(_access(strFilepath, 0) == 0)
		ShellExecute(NULL, NULL, strFilepath, NULL, NULL, SW_SHOWNORMAL);
	else
		AfxMessageBox("The folder does not exist.", MB_ICONINFORMATION);
}

/// <summary>
/// Opens logfile.
/// </summary>
void CDlgConfig::OnBnClickedLogfileOpen()
{
	FixLogFilePath();
	CString strFilename, strFilepath;
	GetDlgItem(ED_LOGFILE)->GetWindowText(strFilepath);
	GetDlgItem(ED_LOGFILE2)->GetWindowText(strFilename);
	CString strFile = strFilepath + strFilename;
	if(_access(strFile, 0) == 0)
		ShellExecute(NULL, NULL, strFile, NULL, NULL, SW_SHOWNORMAL);
	else
		AfxMessageBox("The file does not exist.", MB_ICONINFORMATION);
}

/// <summary>
/// Deletes logfile.
/// </summary>
void CDlgConfig::OnBnClickedLogfileDelete()
{
	FixLogFilePath();
	CString strFilename, strFilepath;
	GetDlgItem(ED_LOGFILE)->GetWindowText(strFilepath);
	GetDlgItem(ED_LOGFILE2)->GetWindowText(strFilename);
	CString strFile = strFilepath + strFilename;
	if(_access(strFile, 0) == 0)
	{
		if(AfxMessageBox("Are you sure you want to delete the file?\n\n" + strFile, MB_ICONQUESTION | MB_YESNO) == IDYES)
		{
			try
			{
				CFile::Remove(strFile);
			}
			catch(CException* pe)
			{
				pe->ReportError();
				pe->Delete();
			}
		}
	}
	else
		AfxMessageBox("The file does not exist.", MB_ICONINFORMATION);
}

void CDlgConfig::OnEnKillfocusLogfile()
{
	FixLogFilePath();
}

/// <summary>
/// Makes sure logfile path end with "\".
/// </summary>
void CDlgConfig::FixLogFilePath()
{
	CString strFilepath;
	GetDlgItem(ED_LOGFILE)->GetWindowText(strFilepath);
	if(!strFilepath.IsEmpty() && strFilepath.Right(1) != "\\")
		GetDlgItem(ED_LOGFILE)->SetWindowText(strFilepath + "\\");
}

void CDlgConfig::OnRememberDiscardedAreas() 
{
	// Clear remembered areas for current frame if this checkbox is clicked.
	if(m_mfd->iPrevSourceFrame >= 0 && m_mfd->iPrevSourceFrame < m_mfd->apbyPrevFramesUsedBlocks->GetSize())
	{
		delete[] m_mfd->apbyPrevFramesUsedBlocks->GetAt(m_mfd->iPrevSourceFrame);
		m_mfd->apbyPrevFramesUsedBlocks->SetAt(m_mfd->iPrevSourceFrame, NULL);
	}
}
