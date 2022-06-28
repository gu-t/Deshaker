#include "ScriptInterpreter.h"
#include "ScriptError.h"
#include "ScriptValue.h"
#include "DeShaker.h"

#define VIMAGE_OK					0
#define VIMAGE_CANT_OPEN_FILE		-1
#define VIMAGE_UNSUPPORTED_JPEG		-2
#define VIMAGE_FAILED				-99

// Must match combobox order
enum
{
	RESAMPLE_NEAREST_NEIGHBOR,
	RESAMPLE_BILINEAR,
	RESAMPLE_BICUBIC
};

#ifndef PI
#define PI 3.14159265358979323846264
#endif

// RGB of a pixel. For grayscale, only g is used.
struct RGBPixel
{
	BYTE byIgnored;		// 1 if pixel is ignored.
	BYTE r;
	BYTE g;
	BYTE b;
	unsigned short r2;	// r squared
	unsigned short g2;	// g squared
	unsigned short b2;	// b squared
};

struct RBorderPixel
{
	int iX;
	int iY;
	double dfR;
	double dfG;
	double dfB;
	double dfTotWeight;
};

struct RVDubMorphImage2WorkData
{
	const FilterActivation *fa;
	RPass2FrameInfo *aFrameInfo;
	int iCurFrame;
	int iLastFrame;
	int iResampleAlg;
	BOOL bUseStoredFrame;
	BOOL bUseCurFrameOnly;
	BOOL bExtrapolateBorder;
	BOOL bMultiFrameBorder;
	int iEdgeTransitionPixelsX;
	int iEdgeTransitionPixelsY;
	CList<int, int> alstBorderPixelIndexes;
	CCriticalSection csBorderPixel;
	Pixel32 *ppixMask;

	int iNextItemToProcess;
	int iTotalItemsToProcess;
	CCriticalSection csNextItem;
};

// CVideoImage represents a video frame. It's an array of pixels.
class CVideoImage : public CArray<RGBPixel, RGBPixel &>
{
public:
	CVideoImage();
	~CVideoImage();
	const CVideoImage& operator=(const CVideoImage &rgbImgSrc);
	void NewImage(int iWidth, int iHeight, BOOL bRGB = TRUE);
	void DownscaleBy2(CVideoImage &imgDest);
	BOOL FindShift(CVideoImage &img2, int iMaxShiftX, int iMaxShiftY, int iBlockSizeX, int iBlockSizeY,
					  int iBlockStart1X, int iBlockStart1Y, int iBlockStart2X, int iBlockStart2Y,
					  double dfMinAllowedMatch, double dfMaxAllowedSecondMatchBelowFirst, int iPixelsInc,
					  double &dfShiftX, double &dfShiftY, int iMinAllowedIntensityDiffInBlock, int iMinNoOfPixels, double *pdfBestMatchValue = NULL, int *piIntensityDiff = NULL);
	void ProjectDestToSrc(int iImageWidth, int iImageHeight, double dfRotate, double dfZoom, double dfShiftX, double dfShiftY, double dfScale, double dfPixelAspectSrc, double dfPixelAspectDest, BOOL bInterlaced, double dfDestX, double dfDestY, double &dfSrcX, double &dfSrcY);
	double FindZoomFactorToAvoidBorder(int iImageWidth, int iImageHeight, double dfRotate, double dfShiftX, double dfShiftY, double dfScale, double dfPixelAspectSrc, double dfPixelAspectDest, BOOL bInterlaced);
	double FindZoomFactorForCorner(int iImageWidth, int iImageHeight, double dfRotate, double dfShiftX, double dfShiftY, double dfScale, double dfPixelAspectSrc, double dfPixelAspectDest, BOOL bInterlaced, int iX, int iY);

	// The following functions are used during pass 2. Initially they also used the CVidoeImage object, but it was too slow, so they now operate directly on the VDub image data and don't use any CVideoImage member variables. Hence they are static. Hehe :)
	static void CalcProjection(int iSrcWidth, int iSrcHeight, int iDestWidth, int iDestHeight, double dfRotate, double dfZoom, double dfShiftX, double dfShiftY, double dfScale, double dfPixelAspectSrc, double dfPixelAspectDest, BOOL bInterlacedSrc, BOOL bInterlacedDest, BOOL bTopField, BOOL bPlainProj, int iResampleAlg, RProjectInfo *pproj);
	static void CalcReversedProjection(int iSrcWidth, int iSrcHeight, int iDestWidth, int iDestHeight, double dfRotate, double dfZoom, double dfShiftX, double dfShiftY, double dfScale, double dfPixelAspectSrc, double dfPixelAspectDest, BOOL bInterlacedSrc, BOOL bInterlacedDest, BOOL bTopField, BOOL bPlainProj, int iResampleAlg, RProjectInfo *pproj);
	static void CalcERSProjection(int iSrcWidth, int iSrcHeight, double dfRotateFrom, double dfRotateTo, double dfZoomFrom, double dfZoomTo, double dfShiftXFrom, double dfShiftXTo, double dfShiftYFrom, double dfShiftYTo, double dfPixelAspectSrc, BOOL bInterlaced, BOOL bTopField, RProjectInfo *pproj);
	static void CalcReversedERSProjection(int iSrcWidth, int iSrcHeight, double dfRotateFrom, double dfRotateTo, double dfZoomFrom, double dfZoomTo, double dfShiftXFrom, double dfShiftXTo, double dfShiftYFrom, double dfShiftYTo, double dfPixelAspectSrc, BOOL bInterlaced, BOOL bTopField, RProjectInfo *pproj);
	static void VDubMorphImage2(const FilterActivation *fa, RPass2FrameInfo *aFrameInfo, int iCurFrame, int iLastFrame, int iResampleAlg, BOOL bUseStoredFrame, BOOL bUseCurFrameOnly, BOOL bExtrapolateBorder, BOOL bMultiFrameBorder, int iEdgeTransitionPixelsX, int iEdgeTransitionPixelsY, Pixel32 *ppixMask);
	static UINT VDubMorphImage2Thread(LPVOID pParam);
	static void VDubBilinearInterpolatePoint(const VBitmap *pbmp, int iX, int iY, Pixel32 &pix, Pixel32 *ppixMask = NULL);
	static void VDubBicubicInterpolatePoint(const VBitmap *pbmp, int iX, int iY, Pixel32 &pix, Pixel32 *ppixMask = NULL);
	static void VDubProjectDestToSrc(int iSrcWidth, int iSrcHeight, int iDestWidth, int iDestHeight, double dfRotate, double dfZoom, double dfShiftX, double dfShiftY, double dfScale, double dfPixelAspectSrc, double dfPixelAspectDest, BOOL bInterlacedSrc, BOOL bInterlacedDest, BOOL bTopField, double dfDestX, double dfDestY, double &dfSrcX, double &dfSrcY);
	static void VDubProjectSrcToDest(int iSrcWidth, int iSrcHeight, int iDestWidth, int iDestHeight, double dfRotate, double dfZoom, double dfShiftX, double dfShiftY, double dfScale, double dfPixelAspectSrc, double dfPixelAspectDest, BOOL bInterlacedSrc, BOOL bInterlacedDest, BOOL bTopField, double &dfDestX, double &dfDestY, double dfSrcX, double dfSrcY);
	static void PrepareCubicTable(double dfA);
	static void FreeCubicTable();
	static void PrepareBorderPixelArray(int iSize);
	static void FreeBorderPixelArray();
private:
	static int *sm_aiCubicTable;
	static RBorderPixel **sm_apBorderPixels;

public:
	BOOL m_bContainsImage;
	int m_iWidth;
	int m_iHeight;
	BOOL m_bRGB;
};
