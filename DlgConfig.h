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



#if !defined(AFX_DLGCONFIG_H__2F56B216_2680_46B4_897E_BBD80973AB6E__INCLUDED_)
#define AFX_DLGCONFIG_H__2F56B216_2680_46B4_897E_BBD80973AB6E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DlgConfig.h : header file
//

struct MyFilterData;

/////////////////////////////////////////////////////////////////////////////
// CDlgConfig dialog

class CDlgConfig : public CDialog
{
// Construction
public:
	CDlgConfig(UINT uiDialogResource, CWnd* pParent = NULL);   // standard constructor
	void EnableDisableCtrls();

// Dialog Data
	//{{AFX_DATA(CDlgConfig)
	enum { IDD = IDD_CONFIG };
	CComboBox	m_CBDestAspect;
	CComboBox	m_CBSrcAspect;
	int		m_iEdgeComp;
	int		m_iResampling;
	int		m_iScale;
	int		m_iUsePixels;
	int		m_iVideoOutput;
	int		m_iBlockSize;
	int		m_iDestHeight;
	int		m_iDestWidth;
	double	m_dfSkipFrame;
	BOOL	m_bNewScene;
	double	m_dfNewScene;
	int		m_iHorPan;
	int		m_iInitSearchRange;
	int		m_iRotation;
	int		m_iSearchRange;
	int		m_iVerPan;
	int		m_iZoom;
	int		m_iPass;
	int		m_iDiscardPixelDiff;
	double	m_dfDiscardBlockPixels;
	double	m_dfDiscard2ndMatch;
	double	m_dfDiscardMatch;
	CString	m_strLogFilePath;
	CString m_strLogFileName;
	BOOL	m_bIgnoreInside;
	BOOL	m_bIgnoreOutside;
	BOOL	m_bLogFileAppend;
	int		m_iIgnoreInsideBottom;
	int		m_iIgnoreInsideLeft;
	int		m_iIgnoreInsideRight;
	int		m_iIgnoreInsideTop;
	int		m_iIgnoreOutsideBottom;
	int		m_iIgnoreOutsideLeft;
	int		m_iIgnoreOutsideRight;
	int		m_iIgnoreOutsideTop;
	double	m_dfExtraZoom;
	double	m_dfLimitHorPan;
	double	m_dfLimitRotation;
	double	m_dfLimitVerPan;
	double	m_dfLimitZoom;
	int		m_iVideoType;
	BOOL	m_bUseOldAndFutureFrames;
	int		m_iFutureFrames;
	int		m_iOldFrames;
	BOOL	m_bFollowIgnoreOutside;
	int		m_iDeepAnalysisKickIn;
	BOOL	m_bERS;
	BOOL	m_bInterlacedProgressiveDest;
	BOOL	m_bDestSameAsSrc;
	BOOL	m_bExtrapolateBorder;
	BOOL	m_bMultiFrameBorder;
	int		m_iEdgeTransitionPixels;
	double	m_dfAbsPixelsToDiscardBlock;
	BOOL	m_bRememberDiscardedAreas;
	int		m_iCPUs;
	double	m_dfERSAmount;
	BOOL	m_bDetectRotation;
	BOOL	m_bDetectZoom;
	double	m_dfAdaptiveZoomSmoothness;
	double	m_dfAdaptiveZoomAmount;
	BOOL	m_bUseColorMask;
	CMFCColorButton m_CBTMaskColor;
	//}}AFX_DATA

	MyFilterData *m_mfd;	// Current settings. Altered when presing OK.

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgConfig)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgConfig)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnPass();
	afx_msg void OnIgnoreOutside();
	afx_msg void OnIgnoreInside();
	afx_msg void OnLogFileBrowse();
	afx_msg void OnUseOldAndFutureFrames();
	afx_msg void OnSelchangeEdgeComp();
	afx_msg void OnERS();
	afx_msg void OnSelchangeVideoType();
	afx_msg void OnDestSameAsSrc();
	afx_msg void OnVisitWeb();
	afx_msg void OnMultiFrameBorder();
	afx_msg void OnBnClickedLogfileDelete();
	afx_msg void OnEnKillfocusLogfile();
	afx_msg void OnBnClickedLogfileOpen();
	afx_msg void OnRememberDiscardedAreas();
	afx_msg void OnBnClickedLogfileOpenFolder();
	afx_msg void OnNewScene();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void FixLogFilePath();
public:
	afx_msg void OnBnClickedUseColorMask();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGCONFIG_H__2F56B216_2680_46B4_897E_BBD80973AB6E__INCLUDED_)
