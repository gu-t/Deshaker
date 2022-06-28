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



// Contains various algorithms etc. for working with a video frame.

#include "stdafx.h"
#include <math.h>
#include "VideoImage.h"

#define BORDER_COLOR 0x000000

int *CVideoImage::sm_aiCubicTable = NULL;
RBorderPixel **CVideoImage::sm_apBorderPixels = NULL;

CVideoImage::CVideoImage()
{
	m_bContainsImage = FALSE;
	m_iWidth = 0;
	m_iHeight = 0;
	m_bRGB = TRUE;
}

CVideoImage::~CVideoImage()
{
}

const CVideoImage& CVideoImage::operator=(const CVideoImage &rgbImgSrc)
{
	NewImage(rgbImgSrc.m_iWidth, rgbImgSrc.m_iHeight);
	memcpy(GetData(), rgbImgSrc.GetData(), m_iWidth * m_iHeight * sizeof(RGBPixel));
	m_bContainsImage = rgbImgSrc.m_bContainsImage;
	return *this;
}

/// <summary>
/// Allocates space for new image.
/// </summary>
void CVideoImage::NewImage(int iWidth, int iHeight, BOOL bRGB /*=TRUE*/)
{
	m_iWidth = iWidth;
	m_iHeight = iHeight;

	m_bContainsImage = TRUE;
	m_bRGB = bRGB;
	SetSize(m_iWidth * m_iHeight);
}

/// <summary>
/// Quickly reduces size of this image object by 2 into imgDest.
/// </summary>
/// <param name="imgDest">Source image.</param>
void CVideoImage::DownscaleBy2(CVideoImage &imgDest)
{
	if(!m_bContainsImage || !imgDest.m_bContainsImage)
		return;

	int iIndexSrc, iIndexSrcRow = 0, iIndexDest = 0;

	if(m_bRGB)
	{
		for(int iY = 0; iY < imgDest.m_iHeight; iY++)
		{
			iIndexSrc = iIndexSrcRow;
			for(int iX = 0; iX < imgDest.m_iWidth; iX++)
			{
				RGBPixel &pixDest = imgDest.ElementAt(iIndexDest++);
				RGBPixel &pixSrc00 = ElementAt(iIndexSrc);
				RGBPixel &pixSrc10 = ElementAt(iIndexSrc + 1);
				RGBPixel &pixSrc01 = ElementAt(iIndexSrc + m_iWidth);
				RGBPixel &pixSrc11 = ElementAt(iIndexSrc + m_iWidth + 1);
				if(pixSrc00.byIgnored == 0 && pixSrc10.byIgnored == 0 && pixSrc01.byIgnored == 0 && pixSrc11.byIgnored == 0)
				{
					pixDest.r = (BYTE)(((unsigned short)pixSrc00.r + pixSrc10.r + pixSrc01.r + pixSrc11.r) >> 2);
					pixDest.g = (BYTE)(((unsigned short)pixSrc00.g + pixSrc10.g + pixSrc01.g + pixSrc11.g) >> 2);
					pixDest.b = (BYTE)(((unsigned short)pixSrc00.b + pixSrc10.b + pixSrc01.b + pixSrc11.b) >> 2);
					pixDest.r2 = (unsigned short)pixDest.r * pixDest.r;
					pixDest.g2 = (unsigned short)pixDest.g * pixDest.g;
					pixDest.b2 = (unsigned short)pixDest.b * pixDest.b;
				}
				else
					pixDest.byIgnored = 1;

				iIndexSrc += 2;
			}
			iIndexSrcRow += m_iWidth + m_iWidth;
		}
	}
	else
	{
		for(int iY = 0; iY < imgDest.m_iHeight; iY++)
		{
			iIndexSrc = iIndexSrcRow;
			for(int iX = 0; iX < imgDest.m_iWidth; iX++)
			{
				RGBPixel &pixDest = imgDest.ElementAt(iIndexDest++);
				RGBPixel &pixSrc00 = ElementAt(iIndexSrc);
				RGBPixel &pixSrc10 = ElementAt(iIndexSrc + 1);
				RGBPixel &pixSrc01 = ElementAt(iIndexSrc + m_iWidth);
				RGBPixel &pixSrc11 = ElementAt(iIndexSrc + m_iWidth + 1);
				if(pixSrc00.byIgnored == 0 && pixSrc10.byIgnored == 0 && pixSrc01.byIgnored == 0 && pixSrc11.byIgnored == 0)
				{
					pixDest.g = (BYTE)(((unsigned short)pixSrc00.g + pixSrc10.g + pixSrc01.g + pixSrc11.g) >> 2);
					pixDest.g2 = (unsigned short)pixDest.g * pixDest.g;
				}
				else
					pixDest.byIgnored = 1;

				iIndexSrc += 2;
			}
			iIndexSrcRow += m_iWidth + m_iWidth;
		}
	}
}

/// <summary>
/// Takes a block in this image and moves it over img2 to find the shift that results in the best match.
/// "Cross-correlation matching" is used to calculate similarities between blocks.
/// </summary>
/// <param name="img2">Block will be compared to same-sized blocks of this image.</param>
/// <param name="iMaxShiftX">Max allowed shift in X-axis.</param>
/// <param name="iMaxShiftY">Max allowed shift in Y-axis.</param>
/// <param name="iBlockSizeX">Width of block.</param>
/// <param name="iBlockSizeY">Height of block.</param>
/// <param name="iBlockStart1X">X-position of start of block in this image.</param>
/// <param name="iBlockStart1Y">Y-position of start of block in this image.</param>
/// <param name="iBlockStart2X">X-position of start of block of where to *start* searching for a match in img2. It will search *around* this position.</param>
/// <param name="iBlockStart2Y">Y-position of start of block of where to *start* searching for a match in img2. It will search *around* this position.</param>
/// <param name="dfMinAllowedMatch">Fail if best match value is below this.</param>
/// <param name="dfMaxAllowedSecondMatchBelowFirst">Fail if difference between best and 2nd best match value is smaller than this.</param>
/// <param name="iPixelsInc">Increment by this many pixels (in both axis) when comparing blocks.</param>
/// <param name="dfShiftX">Resulting X shift.</param>
/// <param name="dfShiftY">Resulting Y shift.</param>
/// <param name="iMinAllowedIntensityDiffInBlock">Fail if max pixel value difference in block is smaller than this.</param>
/// <param name="iMinNoOfPixels">Ignore shifts where If number of non-ignored pixels in blocks are smaller than this value.</param>
/// <param name="pdfBestMatchValue">Output best match value if not NULL.</param>
/// <param name="piIntensityDiff">Output intensity diff if not NULL.</param>
/// <returns>TRUE if a shift was found.</returns>
BOOL CVideoImage::FindShift(CVideoImage &img2, int iMaxShiftX, int iMaxShiftY, int iBlockSizeX, int iBlockSizeY,
									  int iBlockStart1X, int iBlockStart1Y, int iBlockStart2X, int iBlockStart2Y,
									  double dfMinAllowedMatch, double dfMaxAllowedSecondMatchBelowFirst, int iPixelsInc,
									  double &dfShiftX, double &dfShiftY, int iMinAllowedIntensityDiffInBlock, int iMinNoOfPixels,
									  double *pdfBestMatchValue/*=NULL*/, int *piIntensityDiff/*=NULL*/)
{
	// First test if the source block contains enough real data, and not only noise, by examining the range of R, G and B values within the block. Skip block if max range is too small.
	int iY, iX, iIndex1Y, iIndex1X;
	if(iMinAllowedIntensityDiffInBlock > 0 || piIntensityDiff != NULL)
	{
		BYTE byRMin = 255, byRMax = 0, byGMin = 255, byGMax = 0, byBMin = 255, byBMax = 0;
		iIndex1Y = iBlockStart1Y * m_iWidth + iBlockStart1X;
		BOOL bFoundEnoughDiff = FALSE;
		for(iY = 0; iY < iBlockSizeY && !bFoundEnoughDiff; iY += iPixelsInc)
		{
			iIndex1X = iIndex1Y;
			for(iX = 0; iX < iBlockSizeX; iX += iPixelsInc)
			{
				const RGBPixel &pix1 = ElementAt(iIndex1X);
				if(pix1.byIgnored == 0)
				{
					if(pix1.r < byRMin)
						byRMin = pix1.r;
					if(pix1.r > byRMax)
						byRMax = pix1.r;
					if(pix1.g < byGMin)
						byGMin = pix1.g;
					if(pix1.g > byGMax)
						byGMax = pix1.g;
					if(pix1.b < byBMin)
						byBMin = pix1.b;
					if(pix1.b > byBMax)
						byBMax = pix1.b;

					// If we already found enough difference, and caller isn't interested in the actual max diff (piIntensityDiff == NULL), we can exit loop.
					if(piIntensityDiff == NULL && (byRMax - byRMin >= iMinAllowedIntensityDiffInBlock || byGMax - byGMin >= iMinAllowedIntensityDiffInBlock || byBMax - byBMin >= iMinAllowedIntensityDiffInBlock))
					{
						bFoundEnoughDiff = TRUE;
						break;
					}
				}

				iIndex1X += iPixelsInc;
			}

			iIndex1Y += iPixelsInc * m_iWidth;
		}

		if(piIntensityDiff != NULL)
		{
			*piIntensityDiff = max(byRMax - byRMin, max(byGMax - byGMin, byBMax - byBMin));
			if(*piIntensityDiff >= iMinAllowedIntensityDiffInBlock)
				bFoundEnoughDiff = TRUE;
		}

		if(!bFoundEnoughDiff)
		{
			if(pdfBestMatchValue != NULL)
				*pdfBestMatchValue = 0;
			return FALSE;
		}
	}

	int iG1G2, iG1, iG2, iG12, iG22;
	int iSideShiftsX = 2 * iMaxShiftX + 1;
	int iSideShiftsY = 2 * iMaxShiftY + 1;
	double *adfMatchValues = new double[iSideShiftsX * iSideShiftsY];	// Match values for all tested shifts
	int iMatchIndex = 0, iMatchXMax, iMatchYMax;
	double dfMaxMatch = -9, dfTmp;
	int iPixel2X, iPixel2Y, iComparedPixels, iShiftY, iShiftX, iIndex2Y, iIndex2X, iBlockStart2YWithShift, iBlockStart2XWithShift;

	// Loop for different Y shifts
	for(iShiftY = -iMaxShiftY; iShiftY <= iMaxShiftY; iShiftY++)
	{
		iBlockStart2YWithShift = iBlockStart2Y + iShiftY;
		// Loop for different X shifts
		for(iShiftX = -iMaxShiftX; iShiftX <= iMaxShiftX; iShiftX++)
		{
			iG1G2 = iG1 = iG12 = iG2 = iG22 = 0;
			iComparedPixels = 0;
			iBlockStart2XWithShift = iBlockStart2X + iShiftX;
			iIndex1Y = iBlockStart1Y * m_iWidth + iBlockStart1X;
			iIndex2Y = iBlockStart2YWithShift * img2.m_iWidth + iBlockStart2X;
			if(m_bRGB)
			{
				// Color matching
				// Loop for rows in block
				for(iY = 0; iY < iBlockSizeY; iY += iPixelsInc)
				{
					ASSERT(iBlockStart1Y + iY >= 0 && iBlockStart1Y + iY < m_iHeight);
					iPixel2Y = iBlockStart2YWithShift + iY;
					if(iPixel2Y >= 0 && iPixel2Y < img2.m_iHeight)
					{
						iIndex1X = iIndex1Y;
						iIndex2X = iIndex2Y + iShiftX;
						// Loop for pixels on a row in block
						for(iX = 0; iX < iBlockSizeX; iX += iPixelsInc)
						{
							ASSERT(iBlockStart1X + iX >= 0 && iBlockStart1X + iX < m_iWidth);
							iPixel2X = iBlockStart2XWithShift + iX;
							if(iPixel2X >= 0 && iPixel2X < img2.m_iWidth)
							{
								const RGBPixel &pix1 = ElementAt(iIndex1X);
								const RGBPixel &pix2 = img2.ElementAt(iIndex2X);
								if(pix1.byIgnored == 0 && pix2.byIgnored == 0)
								{
									// Cross-correlation matching.
									iG1G2 += (int)pix1.r * pix2.r + (int)pix1.g * pix2.g + (int)pix1.b * pix2.b;	// can get overflow here if block is too big (> ca 100x100)
									iG1 += (int)pix1.r + pix1.g + pix1.b;
									iG12 += (int)pix1.r2 + pix1.g2 + pix1.b2;
									iG2 += (int)pix2.r + pix2.g + pix2.b;
									iG22 += (int)pix2.r2 + pix2.g2 + pix2.b2;

									iComparedPixels++;
								}
							}
							iIndex1X += iPixelsInc;
							iIndex2X += iPixelsInc;
						}
					}
					iIndex1Y += iPixelsInc * m_iWidth;
					iIndex2Y += iPixelsInc * img2.m_iWidth;
				}
			}
			else
			{
				// Grayscale matching
				// Loop for rows in block
				for(iY = 0; iY < iBlockSizeY; iY += iPixelsInc)
				{
					ASSERT(iBlockStart1Y + iY >= 0 && iBlockStart1Y + iY < m_iHeight);
					iPixel2Y = iBlockStart2YWithShift + iY;
					if(iPixel2Y >= 0 && iPixel2Y < img2.m_iHeight)
					{
						iIndex1X = iIndex1Y;
						iIndex2X = iIndex2Y + iShiftX;
						// Loop for pixels on a row in block
						for(iX = 0; iX < iBlockSizeX; iX += iPixelsInc)
						{
							ASSERT(iBlockStart1X + iX >= 0 && iBlockStart1X + iX < m_iWidth);
							iPixel2X = iBlockStart2XWithShift + iX;
							if(iPixel2X >= 0 && iPixel2X < img2.m_iWidth)
							{
								const RGBPixel &pix1 = ElementAt(iIndex1X);
								const RGBPixel &pix2 = img2.ElementAt(iIndex2X);
								if(pix1.byIgnored == 0 && pix2.byIgnored == 0)
								{
									// Cross-correlation matching.
									iG1G2 += (int)pix1.g * pix2.g;
									iG1 += pix1.g;
									iG12 += pix1.g2;
									iG2 += pix2.g;
									iG22 += pix2.g2;

									iComparedPixels++;
								}
							}
							iIndex1X += iPixelsInc;
							iIndex2X += iPixelsInc;
						}
					}
					iIndex1Y += iPixelsInc * m_iWidth;
					iIndex2Y += iPixelsInc * img2.m_iWidth;
				}
			}

			if(iComparedPixels < iMinNoOfPixels)
				adfMatchValues[iMatchIndex] = -1.0;		// Too few pixels examined -> ignore shift
			else
			{
				if(m_bRGB)
					iComparedPixels *= 3;

				dfTmp = (iG12 - (double)iG1 * iG1 / iComparedPixels) * (iG22 - (double)iG2 * iG2 / iComparedPixels);
				if(dfTmp > 0.0)
					adfMatchValues[iMatchIndex] = (iG1G2 - (double)iG1 * iG2 / iComparedPixels) / sqrt(dfTmp);
				else
					adfMatchValues[iMatchIndex] = 0.0;	// Shouldn't occur

				if(adfMatchValues[iMatchIndex] > 1.0 || adfMatchValues[iMatchIndex] < -1.0)
					adfMatchValues[iMatchIndex] = 0.0;	// Shouldn't occur?
			}
			
			if(adfMatchValues[iMatchIndex] > dfMaxMatch)
			{
				dfMaxMatch = adfMatchValues[iMatchIndex];
				iMatchXMax = iShiftX;
				iMatchYMax = iShiftY;
			}
			iMatchIndex++;
		}
	}

	if(pdfBestMatchValue != NULL)
		*pdfBestMatchValue = dfMaxMatch;

	if(dfMaxMatch < dfMinAllowedMatch)
	{
		delete[] adfMatchValues;
		return FALSE;
	}

	// Find the next best match.
	iMatchIndex = 0;
	int iMatchXOtherMax, iMatchYOtherMax;
	double dfMaxMatchOther = -9.0;
	for(iShiftY = -iMaxShiftY; iShiftY <= iMaxShiftY; iShiftY++)
	{
		for(int iShiftX = -iMaxShiftX; iShiftX <= iMaxShiftX; iShiftX++)
		{
			if(iShiftY < iMatchYMax - 1 || iShiftY > iMatchYMax + 1 || iShiftX < iMatchXMax - 1 || iShiftX > iMatchXMax + 1)
			{
				// Examine only shifts that are more than one pixel away from best match.
				if(adfMatchValues[iMatchIndex] > dfMaxMatchOther)
				{
					dfMaxMatchOther = adfMatchValues[iMatchIndex];
					iMatchXOtherMax = iShiftX;
					iMatchYOtherMax = iShiftY;
				}
			}
			iMatchIndex++;
		}
	}

	// Next best match is almost as good as best match -> match failed (could be a blue sky or something)
	if(dfMaxMatchOther > dfMaxMatch - dfMaxAllowedSecondMatchBelowFirst)
	{
		delete[] adfMatchValues;
		return FALSE;
	}

	// Fit a 2nd degree function through the best match and its two neighbor shifts and find its max to get subpixel accuracy on shift (sort of)
	iMatchIndex = (iMatchYMax + iMaxShiftY) * iSideShiftsX + (iMatchXMax + iMaxShiftX);
	if(iMatchXMax == -iMaxShiftX || iMatchXMax == iMaxShiftX)
		dfShiftX = iMatchXMax;
	else
		dfShiftX = iMatchXMax - (adfMatchValues[iMatchIndex + 1] - adfMatchValues[iMatchIndex - 1]) / 2.0 / (adfMatchValues[iMatchIndex + 1] - 2.0 * dfMaxMatch + adfMatchValues[iMatchIndex - 1]);

	if(iMatchYMax == -iMaxShiftY || iMatchYMax == iMaxShiftY)
		dfShiftY = iMatchYMax;
	else
		dfShiftY = iMatchYMax - (adfMatchValues[iMatchIndex + iSideShiftsX] - adfMatchValues[iMatchIndex - iSideShiftsX]) / 2.0 / (adfMatchValues[iMatchIndex + iSideShiftsX] - 2.0 * dfMaxMatch + adfMatchValues[iMatchIndex - iSideShiftsX]);

	delete[] adfMatchValues;

	return TRUE;
}

/// <summary>
/// Calculates what zoom factor is needed to avoid black borders for this frame when projection mapping is applied.
/// </summary>
double CVideoImage::FindZoomFactorToAvoidBorder(int iImageWidth, int iImageHeight, double dfRotate, double dfShiftX, double dfShiftY, double dfScale, double dfPixelAspectSrc, double dfPixelAspectDest, BOOL bInterlaced)
{
	double dfZoomTmp, dfZoomMax = 10.0;

	dfZoomTmp = FindZoomFactorForCorner(iImageWidth, iImageHeight, dfRotate, dfShiftX, dfShiftY, dfScale, dfPixelAspectSrc, dfPixelAspectDest, bInterlaced, 0, 0);
	if(dfZoomTmp < dfZoomMax)
		dfZoomMax = dfZoomTmp;

	dfZoomTmp = FindZoomFactorForCorner(iImageWidth, iImageHeight, dfRotate, dfShiftX, dfShiftY, dfScale, dfPixelAspectSrc, dfPixelAspectDest, bInterlaced, iImageWidth, 0);
	if(dfZoomTmp < dfZoomMax)
		dfZoomMax = dfZoomTmp;

	dfZoomTmp = FindZoomFactorForCorner(iImageWidth, iImageHeight, dfRotate, dfShiftX, dfShiftY, dfScale, dfPixelAspectSrc, dfPixelAspectDest, bInterlaced, 0, iImageHeight);
	if(dfZoomTmp < dfZoomMax)
		dfZoomMax = dfZoomTmp;

	dfZoomTmp = FindZoomFactorForCorner(iImageWidth, iImageHeight, dfRotate, dfShiftX, dfShiftY, dfScale, dfPixelAspectSrc, dfPixelAspectDest, bInterlaced, iImageWidth, iImageHeight);
	if(dfZoomTmp < dfZoomMax)
		dfZoomMax = dfZoomTmp;

	return dfZoomMax;
}

/// <summary>
/// Calculate what zoom factor is needed to make the morphed pixel X, Y appear on the border of this CVideoFrame.
/// </summary>
double CVideoImage::FindZoomFactorForCorner(int iImageWidth, int iImageHeight, double dfRotate, double dfShiftX, double dfShiftY, double dfScale, double dfPixelAspectSrc, double dfPixelAspectDest, BOOL bInterlaced, int iX, int iY)
{
	double dfXMidSrc = m_iWidth / 2.0;
	double dfYMidSrc = m_iHeight / 2.0;
	double dfXMidDest = iImageWidth / 2.0;
	double dfYMidDest = iImageHeight / 2.0;

	double dfRangeMin = 0.1, dfRangeMax = 10.0, dfRangeMid, dfX, dfY;
	// Search for zoom by trying different zoom values and divide range by two until we're there.
	while(dfRangeMax - dfRangeMin > 0.000001 && dfRangeMin < 1.0)
	{
		dfRangeMid = (dfRangeMin + dfRangeMax) / 2.0;
		ProjectDestToSrc(iImageWidth, iImageHeight, dfRotate, dfRangeMid, dfShiftX, dfShiftY, dfScale, dfPixelAspectSrc, dfPixelAspectDest, bInterlaced, iX, iY, dfX, dfY);
		
		if(dfX >= 0 && dfX < m_iWidth && dfY >= 0 && dfY < m_iHeight)
			dfRangeMin = dfRangeMid;
		else
			dfRangeMax = dfRangeMid;
	}

	return dfRangeMin;
}

/// <summary>
/// Finds the source pixel given a destination pixel and panning, rotation and zoom.
/// </summary>
void CVideoImage::ProjectDestToSrc(int iImageWidth, int iImageHeight, double dfRotate, double dfZoom, double dfShiftX, double dfShiftY, double dfScale, double dfPixelAspectSrc, double dfPixelAspectDest, BOOL bInterlaced, double dfDestX, double dfDestY, double &dfSrcX, double &dfSrcY)
{
	if(bInterlaced)
	{
		dfDestY *= 2.0;
//		dfDestY += !bTopField ? 0.0 : 1.0;	// Should be something like this here, but it doesn't matter much for current uses of this function

		dfRotate *= PI / 180.0;
		double dfXMidSrc = m_iWidth / 2.0;
		double dfYMidSrc = m_iHeight;
		double dfXMidDest = iImageWidth / 2.0;
		double dfYMidDest = iImageHeight;
		double dfDiffY = -(dfDestY - dfYMidDest) / dfScale / dfPixelAspectDest;
		double dfDiffX = (dfDestX - dfXMidDest) / dfScale;
		dfSrcX = dfDiffX * cos(dfRotate) + dfDiffY * sin(dfRotate);
		dfSrcY = dfDiffX * sin(dfRotate) - dfDiffY * cos(dfRotate);
		dfSrcX *= dfZoom;
		dfSrcY *= dfZoom;
		dfSrcX += dfShiftX;
		dfSrcY += dfShiftY;
		dfSrcY *= dfPixelAspectSrc;
		dfSrcX += dfXMidSrc;
		dfSrcY += dfYMidSrc;

//		dfSrcY += !bTopField ? 0.0 : -1.0;
		dfSrcY /= 2.0;
	}
	else
	{
		dfRotate *= PI / 180.0;
		double dfXMidSrc = m_iWidth / 2.0;
		double dfYMidSrc = m_iHeight / 2.0;
		double dfXMidDest = iImageWidth / 2.0;
		double dfYMidDest = iImageHeight / 2.0;
		double dfDiffY = -(dfDestY - dfYMidDest) / dfScale / dfPixelAspectDest;
		double dfDiffX = (dfDestX - dfXMidDest) / dfScale;
		dfSrcX = dfDiffX * cos(dfRotate) + dfDiffY * sin(dfRotate);
		dfSrcY = dfDiffX * sin(dfRotate) - dfDiffY * cos(dfRotate);
		dfSrcX *= dfZoom;
		dfSrcY *= dfZoom;
		dfSrcX += dfShiftX;
		dfSrcY += dfShiftY;
		dfSrcY *= dfPixelAspectSrc;
		dfSrcX += dfXMidSrc;
		dfSrcY += dfYMidSrc;
	}
}

/// <summary>
/// Calculates projection mapping from destination to source.
/// </summary>
/// <param name="pproj">Resulting projection mapping.</param>
void CVideoImage::CalcProjection(int iSrcWidth, int iSrcHeight, int iDestWidth, int iDestHeight, double dfRotate, double dfZoom, double dfShiftX, double dfShiftY, double dfScale, double dfPixelAspectSrc, double dfPixelAspectDest, BOOL bInterlacedSrc, BOOL bInterlacedDest, BOOL bTopField, BOOL bPlainProj, int iResampleAlg, RProjectInfo *pproj)
{
	double dfX00, dfY00, dfX01, dfY01, dfX10, dfY10;
	double dfAddToPixel = bPlainProj ? 0.0 : 0.5;

	// Project top/left pixel.
	VDubProjectDestToSrc(iSrcWidth, iSrcHeight, iDestWidth, iDestHeight, dfRotate, dfZoom, dfShiftX, dfShiftY, dfScale, dfPixelAspectSrc, dfPixelAspectDest, bInterlacedSrc, bInterlacedDest, bTopField, dfAddToPixel, dfAddToPixel, dfX00, dfY00);

	// One pixel to the right.
	VDubProjectDestToSrc(iSrcWidth, iSrcHeight, iDestWidth, iDestHeight, dfRotate, dfZoom, dfShiftX, dfShiftY, dfScale, dfPixelAspectSrc, dfPixelAspectDest, bInterlacedSrc, bInterlacedDest, bTopField, 1.0 + dfAddToPixel, dfAddToPixel, dfX01, dfY01);

	// And one pixel down instead.
	VDubProjectDestToSrc(iSrcWidth, iSrcHeight, iDestWidth, iDestHeight, dfRotate, dfZoom, dfShiftX, dfShiftY, dfScale, dfPixelAspectSrc, dfPixelAspectDest, bInterlacedSrc, bInterlacedDest, bTopField, dfAddToPixel, 1.0 + dfAddToPixel, dfX10, dfY10);

	// Get per-pixel deltas in source image.
	double dfDeltaXForXAxis = dfX01 - dfX00;
	double dfDeltaYForXAxis = dfY01 - dfY00;
	double dfDeltaXForYAxis = dfX10 - dfX00;
	double dfDeltaYForYAxis = dfY10 - dfY00;

	if(!bPlainProj && iResampleAlg != RESAMPLE_NEAREST_NEIGHBOR)
	{
		dfX00 -= 0.5;
		dfY00 -= 0.5;
	}

	// Convert double to __int64 for faster calculations elsewhere.
	pproj->iStartX = (__int64)floor(dfX00 * 1024 * 65536 + 0.5);
	pproj->iStartY = (__int64)floor(dfY00 * 1024 * 65536 + 0.5);
	pproj->iDeltaXForXAxis = (__int64)floor(dfDeltaXForXAxis * 1024 * 65536 + 0.5);
	pproj->iDeltaYForXAxis = (__int64)floor(dfDeltaYForXAxis * 1024 * 65536 + 0.5);
	pproj->iDeltaXForYAxis = (__int64)floor(dfDeltaXForYAxis * 1024 * 65536 + 0.5);
	pproj->iDeltaYForYAxis = (__int64)floor(dfDeltaYForYAxis * 1024 * 65536 + 0.5);
}

/// <summary>
/// Calculates projection mapping from source to destination.
/// </summary>
/// <param name="pproj">Resulting projection mapping.</param>
void CVideoImage::CalcReversedProjection(int iSrcWidth, int iSrcHeight, int iDestWidth, int iDestHeight, double dfRotate, double dfZoom, double dfShiftX, double dfShiftY, double dfScale, double dfPixelAspectSrc, double dfPixelAspectDest, BOOL bInterlacedSrc, BOOL bInterlacedDest, BOOL bTopField, BOOL bPlainProj, int iResampleAlg, RProjectInfo *pproj)
{
	double dfX00, dfY00, dfX01, dfY01, dfX10, dfY10;
	double dfAddToPixel = bPlainProj ? 0.0 : 0.5;

	// Project top/left pixel.
	VDubProjectSrcToDest(iSrcWidth, iSrcHeight, iDestWidth, iDestHeight, dfRotate, dfZoom, dfShiftX, dfShiftY, dfScale, dfPixelAspectSrc, dfPixelAspectDest, bInterlacedSrc, bInterlacedDest, bTopField, dfX00, dfY00, dfAddToPixel, dfAddToPixel);

	// One pixel to the right.
	VDubProjectSrcToDest(iSrcWidth, iSrcHeight, iDestWidth, iDestHeight, dfRotate, dfZoom, dfShiftX, dfShiftY, dfScale, dfPixelAspectSrc, dfPixelAspectDest, bInterlacedSrc, bInterlacedDest, bTopField, dfX01, dfY01, 1.0 + dfAddToPixel, dfAddToPixel);

	// And one pixel down instead.
	VDubProjectSrcToDest(iSrcWidth, iSrcHeight, iDestWidth, iDestHeight, dfRotate, dfZoom, dfShiftX, dfShiftY, dfScale, dfPixelAspectSrc, dfPixelAspectDest, bInterlacedSrc, bInterlacedDest, bTopField, dfX10, dfY10, dfAddToPixel, 1.0 + dfAddToPixel);

	// Get per-pixel deltas in destination image.
	double dfDeltaXForXAxis = dfX01 - dfX00;
	double dfDeltaYForXAxis = dfY01 - dfY00;
	double dfDeltaXForYAxis = dfX10 - dfX00;
	double dfDeltaYForYAxis = dfY10 - dfY00;

	if(!bPlainProj && iResampleAlg != RESAMPLE_NEAREST_NEIGHBOR)
	{
		dfX00 -= 0.5;
		dfY00 -= 0.5;
	}

	// Convert double to __int64 for faster calculations elsewhere.
	pproj->iStartX = (__int64)floor(dfX00 * 1024 * 65536 + 0.5);
	pproj->iStartY = (__int64)floor(dfY00 * 1024 * 65536 + 0.5);
	pproj->iDeltaXForXAxis = (__int64)floor(dfDeltaXForXAxis * 1024 * 65536 + 0.5);
	pproj->iDeltaYForXAxis = (__int64)floor(dfDeltaYForXAxis * 1024 * 65536 + 0.5);
	pproj->iDeltaXForYAxis = (__int64)floor(dfDeltaXForYAxis * 1024 * 65536 + 0.5);
	pproj->iDeltaYForYAxis = (__int64)floor(dfDeltaYForYAxis * 1024 * 65536 + 0.5);
}

/// <summary>
/// Calculates ERS projection mapping to previous frame. The mapping is different for each line, so pproj will contain an array of projections, sized 3 times the height in order to be able to use map pixels outside the image.
/// </summary>
/// <param name="pproj">Resulting projection mapping.</param>
void CVideoImage::CalcERSProjection(int iSrcWidth, int iSrcHeight, double dfRotateFrom, double dfRotateTo, double dfZoomFrom, double dfZoomTo, double dfShiftXFrom, double dfShiftXTo, double dfShiftYFrom, double dfShiftYTo, double dfPixelAspectSrc, BOOL bInterlaced, BOOL bTopField, RProjectInfo *pproj)
{
	double dfX00, dfY00, dfX01, dfY01, dfX10, dfY10, dfDeltaXForXAxis, dfDeltaYForXAxis, dfDeltaXForYAxis, dfDeltaYForYAxis;
	double dfRotate, dfZoom, dfShiftX, dfShiftY;
	double dfLogZoomFrom = log(dfZoomFrom);
	double dfLogZoomTo = log(dfZoomTo);
	for(int iY = -iSrcHeight; iY < 2 * iSrcHeight; iY++)
	{
		dfRotate = dfRotateTo + (dfRotateFrom - dfRotateTo) * iY / (iSrcHeight - 1);
		dfZoom = exp(dfLogZoomTo + (dfLogZoomFrom - dfLogZoomTo) * iY / (iSrcHeight - 1));
		dfShiftX = dfShiftXTo + (dfShiftXFrom - dfShiftXTo) * iY / (iSrcHeight - 1);
		dfShiftY = dfShiftYTo + (dfShiftYFrom - dfShiftYTo) * iY / (iSrcHeight - 1);

		// Project first pixel of line.
		VDubProjectDestToSrc(iSrcWidth, iSrcHeight, iSrcWidth, iSrcHeight, dfRotate, dfZoom, dfShiftX, dfShiftY, 1.0, dfPixelAspectSrc, dfPixelAspectSrc, bInterlaced, bInterlaced, bTopField, 0, iY, dfX00, dfY00);

		// One pixel to the right.
		VDubProjectDestToSrc(iSrcWidth, iSrcHeight, iSrcWidth, iSrcHeight, dfRotate, dfZoom, dfShiftX, dfShiftY, 1.0, dfPixelAspectSrc, dfPixelAspectSrc, bInterlaced, bInterlaced, bTopField, 1, iY, dfX01, dfY01);

		// And one pixel down instead.
		VDubProjectDestToSrc(iSrcWidth, iSrcHeight, iSrcWidth, iSrcHeight, dfRotate, dfZoom, dfShiftX, dfShiftY, 1.0, dfPixelAspectSrc, dfPixelAspectSrc, bInterlaced, bInterlaced, bTopField, 0, iY + 1, dfX10, dfY10);

		// Get per-pixel deltas in previous frame.
		dfDeltaXForXAxis = dfX01 - dfX00;
		dfDeltaYForXAxis = dfY01 - dfY00;
		dfDeltaXForYAxis = dfX10 - dfX00;
		dfDeltaYForYAxis = dfY10 - dfY00;

		// Convert double to __int64 for faster calculations elsewhere.
		pproj[iSrcHeight + iY].iStartX = (__int64)floor(dfX00 * 1024 * 65536 + 0.5);
		pproj[iSrcHeight + iY].iStartY = (__int64)floor(dfY00 * 1024 * 65536 + 0.5);
		pproj[iSrcHeight + iY].iDeltaXForXAxis = (__int64)floor(dfDeltaXForXAxis * 1024 * 65536 + 0.5);
		pproj[iSrcHeight + iY].iDeltaYForXAxis = (__int64)floor(dfDeltaYForXAxis * 1024 * 65536 + 0.5);
		pproj[iSrcHeight + iY].iDeltaXForYAxis = (__int64)floor(dfDeltaXForYAxis * 1024 * 65536 + 0.5);
		pproj[iSrcHeight + iY].iDeltaYForYAxis = (__int64)floor(dfDeltaYForYAxis * 1024 * 65536 + 0.5);
	}
}

/// <summary>
/// Calculates ERS projection mapping to next frame. The mapping is different for each line, so pproj will contain an array of projections, sized 3 times the height in order to be able to use map pixels outside the image.
/// </summary>
/// <param name="pproj">Resulting projection mapping.</param>
void CVideoImage::CalcReversedERSProjection(int iSrcWidth, int iSrcHeight, double dfRotateFrom, double dfRotateTo, double dfZoomFrom, double dfZoomTo, double dfShiftXFrom, double dfShiftXTo, double dfShiftYFrom, double dfShiftYTo, double dfPixelAspectSrc, BOOL bInterlaced, BOOL bTopField, RProjectInfo *pproj)
{
	double dfX00, dfY00, dfX01, dfY01, dfX10, dfY10, dfDeltaXForXAxis, dfDeltaYForXAxis, dfDeltaXForYAxis, dfDeltaYForYAxis;
	double dfRotate, dfZoom, dfShiftX, dfShiftY;
	double dfLogZoomFrom = log(dfZoomFrom);
	double dfLogZoomTo = log(dfZoomTo);
	for(int iY = -iSrcHeight; iY < 2 * iSrcHeight; iY++)
	{
		dfRotate = dfRotateTo + (dfRotateFrom - dfRotateTo) * iY / (iSrcHeight - 1);
		dfZoom = exp(dfLogZoomTo + (dfLogZoomFrom - dfLogZoomTo) * iY / (iSrcHeight - 1));
		dfShiftX = dfShiftXTo + (dfShiftXFrom - dfShiftXTo) * iY / (iSrcHeight - 1);
		dfShiftY = dfShiftYTo + (dfShiftYFrom - dfShiftYTo) * iY / (iSrcHeight - 1);

		// Project first pixel of line.
		VDubProjectSrcToDest(iSrcWidth, iSrcHeight, iSrcWidth, iSrcHeight, dfRotate, dfZoom, dfShiftX, dfShiftY, 1.0, dfPixelAspectSrc, dfPixelAspectSrc, bInterlaced, bInterlaced, bTopField, dfX00, dfY00, 0, iY);

		// One pixel to the right.
		VDubProjectSrcToDest(iSrcWidth, iSrcHeight, iSrcWidth, iSrcHeight, dfRotate, dfZoom, dfShiftX, dfShiftY, 1.0, dfPixelAspectSrc, dfPixelAspectSrc, bInterlaced, bInterlaced, bTopField, dfX01, dfY01, 1, iY);

		// And one pixel down instead.
		VDubProjectSrcToDest(iSrcWidth, iSrcHeight, iSrcWidth, iSrcHeight, dfRotate, dfZoom, dfShiftX, dfShiftY, 1.0, dfPixelAspectSrc, dfPixelAspectSrc, bInterlaced, bInterlaced, bTopField, dfX10, dfY10, 0, iY + 1);

		// Get per-pixel deltas in previous frame.
		dfDeltaXForXAxis = dfX01 - dfX00;
		dfDeltaYForXAxis = dfY01 - dfY00;
		dfDeltaXForYAxis = dfX10 - dfX00;
		dfDeltaYForYAxis = dfY10 - dfY00;

		// Convert double to __int64 for faster calculations elsewhere.
		pproj[iSrcHeight + iY].iStartX = (__int64)floor(dfX00 * 1024 * 65536 + 0.5);
		pproj[iSrcHeight + iY].iStartY = (__int64)floor(dfY00 * 1024 * 65536 + 0.5);
		pproj[iSrcHeight + iY].iDeltaXForXAxis = (__int64)floor(dfDeltaXForXAxis * 1024 * 65536 + 0.5);
		pproj[iSrcHeight + iY].iDeltaYForXAxis = (__int64)floor(dfDeltaYForXAxis * 1024 * 65536 + 0.5);
		pproj[iSrcHeight + iY].iDeltaXForYAxis = (__int64)floor(dfDeltaXForYAxis * 1024 * 65536 + 0.5);
		pproj[iSrcHeight + iY].iDeltaYForYAxis = (__int64)floor(dfDeltaYForYAxis * 1024 * 65536 + 0.5);
	}
}

/// <summary>
/// Finds the source pixel (position) given a destination pixel and panning, rotation and zoom.
/// </summary>
void CVideoImage::VDubProjectDestToSrc(int iSrcWidth, int iSrcHeight, int iDestWidth, int iDestHeight, double dfRotate, double dfZoom, double dfShiftX, double dfShiftY, double dfScale, double dfPixelAspectSrc, double dfPixelAspectDest, BOOL bInterlacedSrc, BOOL bInterlacedDest, BOOL bTopField, double dfDestX, double dfDestY, double &dfSrcX, double &dfSrcY)
{
	// Note that this code is very slow, but thankfully it's only called a few times per frame.

	dfRotate *= PI / 180.0;
	double dfXMidSrc = iSrcWidth / 2.0;
	double dfYMidSrc = iSrcHeight / 2.0;
	double dfXMidDest = iDestWidth / 2.0;
	double dfYMidDest = iDestHeight / 2.0;

	if(bInterlacedSrc)
		dfYMidSrc *= 2.0;
	
	if(bInterlacedDest)
	{
		dfYMidDest *= 2.0;
		dfDestY *= 2.0;
		dfDestY += !bTopField ? 0.0 : 1.0;
	}

	dfDestX -= dfXMidDest;
	dfDestY -= dfYMidDest;
	dfDestX /= dfScale;
	dfDestY /= dfScale;
	dfDestY /= dfPixelAspectDest;
	dfSrcX = dfDestX * cos(dfRotate) + dfDestY * sin(dfRotate);
	dfSrcY = -dfDestX * sin(dfRotate) + dfDestY * cos(dfRotate);
	dfSrcX *= dfZoom;
	dfSrcY *= dfZoom;
	dfSrcX += dfShiftX;
	dfSrcY -= dfShiftY;
	dfSrcY *= dfPixelAspectSrc;
	dfSrcX += dfXMidSrc;
	dfSrcY += dfYMidSrc;

	if(bInterlacedSrc)
	{
		dfSrcY += !bTopField ? 0.0 : -1.0;
		dfSrcY /= 2.0;
	}
}

/// <summary>
/// Finds the destination pixel (position) given a source pixel and panning, rotation and zoom.
/// </summary>
void CVideoImage::VDubProjectSrcToDest(int iSrcWidth, int iSrcHeight, int iDestWidth, int iDestHeight, double dfRotate, double dfZoom, double dfShiftX, double dfShiftY, double dfScale, double dfPixelAspectSrc, double dfPixelAspectDest, BOOL bInterlacedSrc, BOOL bInterlacedDest, BOOL bTopField, double &dfDestX, double &dfDestY, double dfSrcX, double dfSrcY)
{
	// Note that this code is very slow, but thankfully it's only called a few times per frame.

	dfRotate *= PI / 180.0;
	double dfXMidSrc = iSrcWidth / 2.0;
	double dfYMidSrc = iSrcHeight / 2.0;
	double dfXMidDest = iDestWidth / 2.0;
	double dfYMidDest = iDestHeight / 2.0;
	
	if(bInterlacedSrc)
	{
		dfYMidSrc *= 2.0;
		dfSrcY *= 2.0;
		dfSrcY += !bTopField ? 0.0 : 1.0;
	}

	if(bInterlacedDest)
		dfYMidDest *= 2.0;

	dfSrcX -= dfXMidSrc;
	dfSrcY -= dfYMidSrc;
	dfSrcY /= dfPixelAspectSrc;
	dfSrcX -= dfShiftX;
	dfSrcY += dfShiftY;
	dfSrcX /= dfZoom;
	dfSrcY /= dfZoom;
	dfDestX = dfSrcX * cos(-dfRotate) + dfSrcY * sin(-dfRotate);
	dfDestY = -dfSrcX * sin(-dfRotate) + dfSrcY * cos(-dfRotate);
	dfDestY *= dfPixelAspectDest;
	dfDestX *= dfScale;
	dfDestY *= dfScale;
	dfDestX += dfXMidDest;
	dfDestY += dfYMidDest;

	if(bInterlacedDest)
	{
		dfDestY += !bTopField ? 0.0 : -1.0;
		dfDestY /= 2.0;
	}
}

/// <summary>
/// Pans, rotates and zooms image in fa->src into fa->dst, using resampling.
/// </summary>
/// <param name="fa">See VDub documentation.</param>
/// <param name="aFrameInfo">Pass 2 frame info.</param>
/// <param name="iCurFrame">Index of current video frame.</param>
/// <param name="iLastFrame">Index of last frame in clip.</param>
/// <param name="iResampleAlg">Resamling algorithm.</param>
/// <param name="bUseStoredFrame">Use stored (remembered) frame instead of current source frame, as source. Needed when using future frames to fill in borders.</param>
/// <param name="bUseCurFrameOnly">Pass TRUE if neither past or future frames should be used to fill in borders.</param>
/// <param name="bExtrapolateBorder">Pass TRUE to extrapolate colors onto border where we can't fill it in other ways.</param>
/// <param name="bMultiFrameBorder">Pass TRUE to take weighted pixel data from all past and future frames, instead of only from the closest one chronologically.</param>
/// <param name="iEdgeTransitionPixelsX">Edge transition width. How far away from a frame's border it shall start to fade (into other frames). Can only be used if bMultiFrameBorder is TRUE.</param>
/// <param name="iEdgeTransitionPixelsY">Same but for Y.</param>
/// <param name="ppixMask">Pointer to a mask color, if masking should be used, or NULL if not. (Masked pixels are treated pretty much like border pixels.)</param>
void CVideoImage::VDubMorphImage2(const FilterActivation *fa, RPass2FrameInfo *aFrameInfo, int iCurFrame, int iLastFrame, int iResampleAlg, BOOL bUseStoredFrame, BOOL bUseCurFrameOnly, BOOL bExtrapolateBorder, BOOL bMultiFrameBorder, int iEdgeTransitionPixelsX, int iEdgeTransitionPixelsY, Pixel32 *ppixMask)
{
	if(bUseCurFrameOnly)
	{
		bMultiFrameBorder = FALSE;
		iEdgeTransitionPixelsX = 0;
		iEdgeTransitionPixelsY = 0;
	}

	RPass2FrameInfo *pCurFrameInfo = aFrameInfo + iCurFrame;
	Pixel32 *pdst = fa->dst.data;
	VBitmap *pvbmp = bUseStoredFrame ? pCurFrameInfo->pvbmp : &fa->src;
	Pixel32 *psrc = pvbmp->data;
	ptrdiff_t iSrcPitch = pvbmp->pitch;
	int iSrcWidth = pvbmp->w;
	int iSrcHeight = pvbmp->h;
	int iDestWidth = fa->dst.w;
	int iDestHeight = fa->dst.h;
	RBorderPixel *pbp;
	double dfWeight;
	Pixel32 pix32;
	CList<RBorderPixel *, RBorderPixel *> aBorderPixels;

	// For each destination pixel, find the corresponding pixel position in the source and resample from source pixels.
	// Multithreaded to allow for higher speed on multi-cpu computers.
	RVDubMorphImage2WorkData wd;
	wd.fa = fa;
	wd.aFrameInfo = aFrameInfo;
	wd.iCurFrame = iCurFrame;
	wd.iLastFrame = iLastFrame;
	wd.iResampleAlg = iResampleAlg;
	wd.bUseStoredFrame = bUseStoredFrame;
	wd.bUseCurFrameOnly = bUseCurFrameOnly;
	wd.bExtrapolateBorder = bExtrapolateBorder;
	wd.bMultiFrameBorder = bMultiFrameBorder;
	wd.iEdgeTransitionPixelsX = iEdgeTransitionPixelsX;
	wd.iEdgeTransitionPixelsY = iEdgeTransitionPixelsY;
	wd.ppixMask = ppixMask;
	wd.iNextItemToProcess = 0;
	wd.iTotalItemsToProcess = iDestWidth * iDestHeight;

	int iThreads = min(g_iCPUs, wd.iTotalItemsToProcess);
	CWinThread **apthreads = new CWinThread*[iThreads];
	HANDLE *ahthreads = new HANDLE[iThreads];
	for(int i = 0; i < iThreads; i++)
	{
		apthreads[i] = AfxBeginThread(VDubMorphImage2Thread, &wd, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
		apthreads[i]->m_bAutoDelete = FALSE;
		ahthreads[i] = apthreads[i]->m_hThread;
		apthreads[i]->ResumeThread();
	}
	WaitForMultipleObjects(iThreads, ahthreads, TRUE, INFINITE);
	for(int i = 0; i < iThreads; i++)
		delete apthreads[i];
	delete[] ahthreads;
	delete[] apthreads;

	if(wd.alstBorderPixelIndexes.GetCount() > 0)
	{
		// Find "edge pixels", i.e. ok pixels that have border pixels as their neighbors.
		CMap<int, int, BYTE, BYTE> mapEdgePixels;
		mapEdgePixels.InitHashTable(1009);
		BYTE byDummy;
		int iIndex;
		POSITION pos = wd.alstBorderPixelIndexes.GetHeadPosition();
		while(pos != NULL)
		{
			iIndex = wd.alstBorderPixelIndexes.GetNext(pos);
			if(iIndex >= iDestWidth && sm_apBorderPixels[iIndex - iDestWidth] == NULL && !mapEdgePixels.Lookup(iIndex - iDestWidth, byDummy))
				mapEdgePixels.SetAt(iIndex - iDestWidth, 0);
			if(iIndex < (iDestHeight - 1) * iDestWidth && sm_apBorderPixels[iIndex + iDestWidth] == NULL && !mapEdgePixels.Lookup(iIndex + iDestWidth, byDummy))
				mapEdgePixels.SetAt(iIndex + iDestWidth, 0);
			if((iIndex % iDestWidth) > 0 && sm_apBorderPixels[iIndex - 1] == NULL && !mapEdgePixels.Lookup(iIndex - 1, byDummy))
				mapEdgePixels.SetAt(iIndex - 1, 0);
			if((iIndex % iDestWidth) < iDestWidth - 1 && sm_apBorderPixels[iIndex + 1] == NULL && !mapEdgePixels.Lookup(iIndex + 1, byDummy))
				mapEdgePixels.SetAt(iIndex + 1, 0);
		}

		// Go through all edge pixels and let them influence all border pixels based on the squared distance to them.
		pdst = fa->dst.data;
		ptrdiff_t iDestPitch = fa->dst.pitch;
		int iXEdge, iYEdge, dfR, dfG, dfB;
		pos = mapEdgePixels.GetStartPosition();
		while(pos != NULL)
		{
			mapEdgePixels.GetNextAssoc(pos, iIndex, byDummy);
			iYEdge = iIndex / iDestWidth;
			iXEdge = iIndex % iDestWidth;
			pix32 = *((Pixel32 *)((char *)pdst + iYEdge * iDestPitch) + iXEdge);
			dfR = (pix32 >> 16) & 0xff;
			dfG = (pix32 >> 8) & 0xff;
			dfB = pix32 & 0xff;
			POSITION pos2 = wd.alstBorderPixelIndexes.GetHeadPosition();
			while(pos2 != NULL)
			{
				iIndex = wd.alstBorderPixelIndexes.GetNext(pos2);
				pbp = sm_apBorderPixels[iIndex];
				dfWeight = 1.0 / ((pbp->iX - iXEdge) * (pbp->iX - iXEdge) + (pbp->iY - iYEdge) * (pbp->iY - iYEdge));
				pbp->dfR += dfWeight * dfR;
				pbp->dfG += dfWeight * dfG;
				pbp->dfB += dfWeight * dfB;
				pbp->dfTotWeight += dfWeight;
			}
		}

		// Color the border pixels
		pos = wd.alstBorderPixelIndexes.GetHeadPosition();
		while(pos != NULL)
		{
			iIndex = wd.alstBorderPixelIndexes.GetNext(pos);
			pbp = sm_apBorderPixels[iIndex];
			pix32 = ((int)(pbp->dfR / pbp->dfTotWeight + 0.5) << 16) +
					((int)(pbp->dfG / pbp->dfTotWeight + 0.5) << 8) +
					(int)(pbp->dfB / pbp->dfTotWeight + 0.5);
			*((Pixel32 *)((char *)pdst + pbp->iY * iDestPitch) + pbp->iX) = pix32;
			delete pbp;
			sm_apBorderPixels[iIndex] = NULL;
		}
	}
}

/// <summary>
/// Thread to pan, rotate and zoom image using resampling.
/// </summary>
UINT CVideoImage::VDubMorphImage2Thread(LPVOID pParam)
{
	RVDubMorphImage2WorkData *pwd = (RVDubMorphImage2WorkData *)pParam;
	int iProcessingItem, iDestX, iDestY;
	int iOkEdgeOffset = 2;
	RPass2FrameInfo *pCurFrameInfo = pwd->aFrameInfo + pwd->iCurFrame;
	__int64 iStartX = pCurFrameInfo->projDestToSrc.iStartX;
	__int64 iStartY = pCurFrameInfo->projDestToSrc.iStartY;
	__int64 iDeltaXForXAxis = pCurFrameInfo->projDestToSrc.iDeltaXForXAxis;
	__int64 iDeltaYForXAxis = pCurFrameInfo->projDestToSrc.iDeltaYForXAxis;
	__int64 iDeltaXForYAxis = pCurFrameInfo->projDestToSrc.iDeltaXForYAxis;
	__int64 iDeltaYForYAxis = pCurFrameInfo->projDestToSrc.iDeltaYForYAxis;
	__int64 iX, iY, iXPast1, iYPast1, iXPast2, iYPast2, iXFuture1, iYFuture1, iXFuture2, iYFuture2, iXOrg, iYOrg;
	VBitmap *pvbmp = pwd->bUseStoredFrame ? pCurFrameInfo->pvbmp : &pwd->fa->src;
	Pixel32 *psrcStart = pvbmp->data;
	ptrdiff_t iSrcPitch = pvbmp->pitch;
	int iSrcWidth = pvbmp->w;
	int iSrcHeight = pvbmp->h;
	Pixel32 *pdstStart = pwd->fa->dst.data;
	ptrdiff_t iDestPitch = pwd->fa->dst.pitch;
	int iDestWidth = pwd->fa->dst.w;
	int iDestHeight = pwd->fa->dst.h;
	int iXWhole, iYWhole;
	int iPastFrame = pwd->iCurFrame;
	int iFutureFrame = pwd->iCurFrame;
	Pixel32 *pdst, pix32, pix32CurFrame;
	RBorderPixel *pbp;
	double dfWeight;
	BOOL bPixelOk;
	int iPixelsAtOnce = 200;

	while(TRUE)
	{
		// Critical section start... get new processing item.
		pwd->csNextItem.Lock();
		BOOL bContinueWorking = pwd->iNextItemToProcess < pwd->iTotalItemsToProcess;
		if(bContinueWorking)
		{
			iProcessingItem = pwd->iNextItemToProcess;
			pwd->iNextItemToProcess += iPixelsAtOnce;
		}
		pwd->csNextItem.Unlock();
		// Critical section end...

		if(!bContinueWorking)
			break;

		iDestY = iProcessingItem / iDestWidth;
		iDestX = iProcessingItem % iDestWidth;
		iXOrg = iStartX + iDestY * iDeltaXForYAxis + iDestX * iDeltaXForXAxis;
		iYOrg = iStartY + iDestY * iDeltaYForYAxis + iDestX * iDeltaYForXAxis;
		pdst = (Pixel32 *)((char *)pdstStart + iDestY * iDestPitch) + iDestX;
		for(int iPixel = 0; iPixel < iPixelsAtOnce && iProcessingItem < pwd->iTotalItemsToProcess; iPixel++, iProcessingItem++)
		{
			if(pCurFrameInfo->pprojERSCompSrcToOrgSrc != NULL)
			{
				iYWhole = (int)(iYOrg >> 26);
				if(iYWhole < -iSrcHeight)
					iYWhole = -iSrcHeight;
				else if(iYWhole >= 2 * iSrcHeight)
					iYWhole = 2 * iSrcHeight - 1;

				iX = pCurFrameInfo->pprojERSCompSrcToOrgSrc[iSrcHeight + iYWhole].iStartX +
						(((iXOrg >> 8) * (pCurFrameInfo->pprojERSCompSrcToOrgSrc[iSrcHeight + iYWhole].iDeltaXForXAxis >> 8) +
						  ((iYOrg >> 8) - ((__int64)iYWhole << 18)) * (pCurFrameInfo->pprojERSCompSrcToOrgSrc[iSrcHeight + iYWhole].iDeltaXForYAxis >> 8)) >> 10);
				iY = pCurFrameInfo->pprojERSCompSrcToOrgSrc[iSrcHeight + iYWhole].iStartY +
						(((iXOrg >> 8) * (pCurFrameInfo->pprojERSCompSrcToOrgSrc[iSrcHeight + iYWhole].iDeltaYForXAxis >> 8) +
						  ((iYOrg >> 8) - ((__int64)iYWhole << 18)) * (pCurFrameInfo->pprojERSCompSrcToOrgSrc[iSrcHeight + iYWhole].iDeltaYForYAxis >> 8)) >> 10);
			}
			else
			{
				iX = iXOrg;
				iY = iYOrg;
			}
			iXWhole = (int)(iX >> 26);
			iYWhole = (int)(iY >> 26);
			if(pwd->iResampleAlg == RESAMPLE_NEAREST_NEIGHBOR)
				*pdst = iXWhole >= 0 && iYWhole >= 0 && iXWhole < iSrcWidth && iYWhole < iSrcHeight ? *((Pixel32 *)((char *)psrcStart + iYWhole * iSrcPitch) + iXWhole) : BORDER_COLOR;
			else if(pwd->iResampleAlg == RESAMPLE_BILINEAR)
				VDubBilinearInterpolatePoint(pvbmp, (int)(iX >> 16), (int)(iY >> 16), *pdst, pwd->ppixMask);
			else
				VDubBicubicInterpolatePoint(pvbmp, (int)(iX >> 16), (int)(iY >> 16), *pdst, pwd->ppixMask);
			
			BOOL bPixelIsMasked = FALSE;
			if(pwd->ppixMask != NULL && (*pdst & 0xffffff) == *(pwd->ppixMask))	// & 0xffffff is needed because it seems VirtualDub sometimes puts trash in the alhpa channel(?)
			{
				bPixelOk = FALSE;
				bPixelIsMasked = TRUE;
			}
			else if(pwd->bMultiFrameBorder)
				bPixelOk = iXWhole >= iOkEdgeOffset + pwd->iEdgeTransitionPixelsX && iXWhole < iSrcWidth - iOkEdgeOffset - pwd->iEdgeTransitionPixelsX && iYWhole >= iOkEdgeOffset + pwd->iEdgeTransitionPixelsY && iYWhole < iSrcHeight - iOkEdgeOffset - pwd->iEdgeTransitionPixelsY;
			else
				bPixelOk = iXWhole >= iOkEdgeOffset && iXWhole < iSrcWidth - iOkEdgeOffset && iYWhole >= iOkEdgeOffset && iYWhole < iSrcHeight - iOkEdgeOffset;

			if(!bPixelOk && !pwd->bUseCurFrameOnly)
			{	// Current image had no data (black border). Look through past and future frames to find information.
				// If current frame number is 100, then look through 99, 101, 98, 102, 97, 103 etc. until pixel info is found.
				// ThisToNext and ThisToPrev projection mappings are used to map pixels between frames.
				pix32CurFrame = *pdst;
				iXPast1 = iX;
				iYPast1 = iY;
				iXFuture1 = iX;
				iYFuture1 = iY;
				BOOL bGiveUp = FALSE;
				double dfEdgeTransitionFactorCurFrame = 0.0;
				if(pwd->bMultiFrameBorder)
				{
					pbp = new RBorderPixel;
					pbp->dfR = 0.0;
					pbp->dfG = 0.0;
					pbp->dfB = 0.0;
					pbp->dfTotWeight = 0.0;
					dfWeight = 1.0;
					if(!bPixelIsMasked && pwd->iEdgeTransitionPixelsX > 0 && iXWhole >= iOkEdgeOffset && iXWhole < iSrcWidth - iOkEdgeOffset && iYWhole >= iOkEdgeOffset && iYWhole < iSrcHeight - iOkEdgeOffset)
					{	// Inside edge transition area
						dfEdgeTransitionFactorCurFrame = 1.0;

						double dfTransitionFactorTmp = (iX / 67108864.0 - iOkEdgeOffset) / (double)pwd->iEdgeTransitionPixelsX;
						if(dfTransitionFactorTmp < 1.0)
							dfEdgeTransitionFactorCurFrame *= dfTransitionFactorTmp;

						dfTransitionFactorTmp = (iSrcWidth - iOkEdgeOffset - iX / 67108864.0) / (double)pwd->iEdgeTransitionPixelsX;
						if(dfTransitionFactorTmp < 1.0)
							dfEdgeTransitionFactorCurFrame *= dfTransitionFactorTmp;

						dfTransitionFactorTmp = (iY / 67108864.0 - iOkEdgeOffset) / (double)pwd->iEdgeTransitionPixelsY;
						if(dfTransitionFactorTmp < 1.0)
							dfEdgeTransitionFactorCurFrame *= dfTransitionFactorTmp;

						dfTransitionFactorTmp = (iSrcHeight - iOkEdgeOffset - iY / 67108864.0) / (double)pwd->iEdgeTransitionPixelsY;
						if(dfTransitionFactorTmp < 1.0)
							dfEdgeTransitionFactorCurFrame *= dfTransitionFactorTmp;

						if(dfEdgeTransitionFactorCurFrame < 0.00001)
							dfEdgeTransitionFactorCurFrame = 0.00001;
					}
				}
				while(!bPixelOk && !bGiveUp)
				{
					bGiveUp = TRUE;
					if(iPastFrame > 0 && pwd->aFrameInfo[iPastFrame - 1].pvbmp != NULL && !pwd->aFrameInfo[iPastFrame].bFirstFrameOfScene)
					{	// Try past frame.
						bGiveUp = FALSE;
						if(pwd->aFrameInfo[iPastFrame].pprojERSThisSrcToPrevSrc != NULL)
						{	// Rolling shutter
							iYWhole = (int)(iYPast1 >> 26);
							if(iYWhole < -iSrcHeight)
								iYWhole = -iSrcHeight;
							else if(iYWhole >= 2 * iSrcHeight)
								iYWhole = 2 * iSrcHeight - 1;

							iXPast2 = pwd->aFrameInfo[iPastFrame].pprojERSThisSrcToPrevSrc[iSrcHeight + iYWhole].iStartX +
									(((iXPast1 >> 8) * (pwd->aFrameInfo[iPastFrame].pprojERSThisSrcToPrevSrc[iSrcHeight + iYWhole].iDeltaXForXAxis >> 8) +
									  ((iYPast1 >> 8) - ((__int64)iYWhole << 18)) * (pwd->aFrameInfo[iPastFrame].pprojERSThisSrcToPrevSrc[iSrcHeight + iYWhole].iDeltaXForYAxis >> 8)) >> 10);
							iYPast2 = pwd->aFrameInfo[iPastFrame].pprojERSThisSrcToPrevSrc[iSrcHeight + iYWhole].iStartY +
									(((iXPast1 >> 8) * (pwd->aFrameInfo[iPastFrame].pprojERSThisSrcToPrevSrc[iSrcHeight + iYWhole].iDeltaYForXAxis >> 8) +
									  ((iYPast1 >> 8) - ((__int64)iYWhole << 18)) * (pwd->aFrameInfo[iPastFrame].pprojERSThisSrcToPrevSrc[iSrcHeight + iYWhole].iDeltaYForYAxis >> 8)) >> 10);
						}
						else
						{
							iXPast2 = pwd->aFrameInfo[iPastFrame].projThisSrcToPrevSrc.iStartX +
									(((iXPast1 >> 8) * (pwd->aFrameInfo[iPastFrame].projThisSrcToPrevSrc.iDeltaXForXAxis >> 8) +
									  (iYPast1 >> 8) * (pwd->aFrameInfo[iPastFrame].projThisSrcToPrevSrc.iDeltaXForYAxis >> 8)) >> 10);
							iYPast2 = pwd->aFrameInfo[iPastFrame].projThisSrcToPrevSrc.iStartY +
									(((iXPast1 >> 8) * (pwd->aFrameInfo[iPastFrame].projThisSrcToPrevSrc.iDeltaYForXAxis >> 8) +
									  (iYPast1 >> 8) * (pwd->aFrameInfo[iPastFrame].projThisSrcToPrevSrc.iDeltaYForYAxis >> 8)) >> 10);
						}
						iXPast1 = iXPast2;
						iYPast1 = iYPast2;
						--iPastFrame;

						iXWhole = (int)(iXPast2 >> 26);
						iYWhole = (int)(iYPast2 >> 26);
						bPixelOk = iXWhole >= iOkEdgeOffset && iXWhole < iSrcWidth - iOkEdgeOffset && iYWhole >= iOkEdgeOffset && iYWhole < iSrcHeight - iOkEdgeOffset;
						if(bPixelOk)
						{
							if(pwd->iResampleAlg == RESAMPLE_NEAREST_NEIGHBOR)
								*pdst = *((Pixel32 *)((char *)pwd->aFrameInfo[iPastFrame].pvbmp->data + iYWhole * iSrcPitch) + iXWhole);
							else if(pwd->iResampleAlg == RESAMPLE_BILINEAR)
								VDubBilinearInterpolatePoint(pwd->aFrameInfo[iPastFrame].pvbmp, (int)(iXPast2 >> 16), (int)(iYPast2 >> 16), *pdst, pwd->ppixMask);
							else
								VDubBicubicInterpolatePoint(pwd->aFrameInfo[iPastFrame].pvbmp, (int)(iXPast2 >> 16), (int)(iYPast2 >> 16), *pdst, pwd->ppixMask);

							if(pwd->ppixMask != NULL && (*pdst & 0xffffff) == *(pwd->ppixMask))	// & 0xffffff is needed because it seems VirtualDub sometimes puts trash in the alhpa channel(?)
								bPixelOk = FALSE;

							if(bPixelOk)
							{
								if(pwd->bMultiFrameBorder)
								{
									double dfTransitionFactor = 1.0;
									if(!bPixelIsMasked && pwd->iEdgeTransitionPixelsX > 0)
									{
										if(!(iXWhole >= iOkEdgeOffset + pwd->iEdgeTransitionPixelsX && iXWhole < iSrcWidth - iOkEdgeOffset - pwd->iEdgeTransitionPixelsX && iYWhole >= iOkEdgeOffset + pwd->iEdgeTransitionPixelsY && iYWhole < iSrcHeight - iOkEdgeOffset - pwd->iEdgeTransitionPixelsY))
										{	// Inside edge transition area
											double dfTransitionFactorTmp = (iXPast2 / 67108864.0 - iOkEdgeOffset) / (double)pwd->iEdgeTransitionPixelsX;
											if(dfTransitionFactorTmp < 1.0)
												dfTransitionFactor *= dfTransitionFactorTmp;

											dfTransitionFactorTmp = (iSrcWidth - iOkEdgeOffset - iXPast2 / 67108864.0) / (double)pwd->iEdgeTransitionPixelsX;
											if(dfTransitionFactorTmp < 1.0)
												dfTransitionFactor *= dfTransitionFactorTmp;

											dfTransitionFactorTmp = (iYPast2 / 67108864.0 - iOkEdgeOffset) / (double)pwd->iEdgeTransitionPixelsY;
											if(dfTransitionFactorTmp < 1.0)
												dfTransitionFactor *= dfTransitionFactorTmp;

											dfTransitionFactorTmp = (iSrcHeight - iOkEdgeOffset - iYPast2 / 67108864.0) / (double)pwd->iEdgeTransitionPixelsY;
											if(dfTransitionFactorTmp < 1.0)
												dfTransitionFactor *= dfTransitionFactorTmp;

											if(dfTransitionFactor < 0.00001)
												dfTransitionFactor = 0.00001;
										}
									}

									pix32 = *pdst;
									pbp->dfR += dfWeight * dfTransitionFactor * ((pix32 >> 16) & 0xff);
									pbp->dfG += dfWeight * dfTransitionFactor * ((pix32 >> 8) & 0xff);
									pbp->dfB += dfWeight * dfTransitionFactor * (pix32 & 0xff);
									pbp->dfTotWeight += dfWeight * dfTransitionFactor;
									bPixelOk = FALSE;
								}
							}
						}
					}
					if(!bPixelOk && iFutureFrame < pwd->iLastFrame && pwd->aFrameInfo[iFutureFrame + 1].pvbmp != NULL && !pwd->aFrameInfo[iFutureFrame + 1].bFirstFrameOfScene)
					{	// Try future frame.
						bGiveUp = FALSE;
						if(pwd->aFrameInfo[iFutureFrame].pprojERSThisSrcToNextSrc != NULL)
						{	// Rolling shutter
							iYWhole = (int)(iYFuture1 >> 26);
							if(iYWhole < -iSrcHeight)
								iYWhole = -iSrcHeight;
							else if(iYWhole >= 2 * iSrcHeight)
								iYWhole = 2 * iSrcHeight - 1;

							iXFuture2 = pwd->aFrameInfo[iFutureFrame].pprojERSThisSrcToNextSrc[iSrcHeight + iYWhole].iStartX +
									(((iXFuture1 >> 8) * (pwd->aFrameInfo[iFutureFrame].pprojERSThisSrcToNextSrc[iSrcHeight + iYWhole].iDeltaXForXAxis >> 8) +
									  ((iYFuture1 >> 8) - ((__int64)iYWhole << 18)) * (pwd->aFrameInfo[iFutureFrame].pprojERSThisSrcToNextSrc[iSrcHeight + iYWhole].iDeltaXForYAxis >> 8)) >> 10);
							iYFuture2 = pwd->aFrameInfo[iFutureFrame].pprojERSThisSrcToNextSrc[iSrcHeight + iYWhole].iStartY +
									(((iXFuture1 >> 8) * (pwd->aFrameInfo[iFutureFrame].pprojERSThisSrcToNextSrc[iSrcHeight + iYWhole].iDeltaYForXAxis >> 8) +
									  ((iYFuture1 >> 8) - ((__int64)iYWhole << 18)) * (pwd->aFrameInfo[iFutureFrame].pprojERSThisSrcToNextSrc[iSrcHeight + iYWhole].iDeltaYForYAxis >> 8)) >> 10);
						}
						else
						{
							iXFuture2 = pwd->aFrameInfo[iFutureFrame].projThisSrcToNextSrc.iStartX +
									(((iXFuture1 >> 8) * (pwd->aFrameInfo[iFutureFrame].projThisSrcToNextSrc.iDeltaXForXAxis >> 8) +
									  (iYFuture1 >> 8) * (pwd->aFrameInfo[iFutureFrame].projThisSrcToNextSrc.iDeltaXForYAxis >> 8)) >> 10);
							iYFuture2 = pwd->aFrameInfo[iFutureFrame].projThisSrcToNextSrc.iStartY +
									(((iXFuture1 >> 8) * (pwd->aFrameInfo[iFutureFrame].projThisSrcToNextSrc.iDeltaYForXAxis >> 8) +
									  (iYFuture1 >> 8) * (pwd->aFrameInfo[iFutureFrame].projThisSrcToNextSrc.iDeltaYForYAxis >> 8)) >> 10);
						}
						iXFuture1 = iXFuture2;
						iYFuture1 = iYFuture2;
						++iFutureFrame;

						iXWhole = (int)(iXFuture2 >> 26);
						iYWhole = (int)(iYFuture2 >> 26);
						bPixelOk = iXWhole >= iOkEdgeOffset && iXWhole < iSrcWidth - iOkEdgeOffset && iYWhole >= iOkEdgeOffset && iYWhole < iSrcHeight - iOkEdgeOffset;
						if(bPixelOk)
						{
							if(pwd->iResampleAlg == RESAMPLE_NEAREST_NEIGHBOR)
								*pdst = *((Pixel32 *)((char *)pwd->aFrameInfo[iFutureFrame].pvbmp->data + iYWhole * iSrcPitch) + iXWhole);
							else if(pwd->iResampleAlg == RESAMPLE_BILINEAR)
								VDubBilinearInterpolatePoint(pwd->aFrameInfo[iFutureFrame].pvbmp, (int)(iXFuture2 >> 16), (int)(iYFuture2 >> 16), *pdst, pwd->ppixMask);
							else
								VDubBicubicInterpolatePoint(pwd->aFrameInfo[iFutureFrame].pvbmp, (int)(iXFuture2 >> 16), (int)(iYFuture2 >> 16), *pdst, pwd->ppixMask);

							if(pwd->ppixMask != NULL && (*pdst & 0xffffff) == *(pwd->ppixMask))	// & 0xffffff is needed because it seems VirtualDub sometimes puts trash in the alhpa channel(?)
								bPixelOk = FALSE;

							if(bPixelOk)
							{
								if(pwd->bMultiFrameBorder)
								{
									double dfTransitionFactor = 1.0;
									if(pwd->iEdgeTransitionPixelsX > 0)
									{
										if(!(iXWhole >= iOkEdgeOffset + pwd->iEdgeTransitionPixelsX && iXWhole < iSrcWidth - iOkEdgeOffset - pwd->iEdgeTransitionPixelsX && iYWhole >= iOkEdgeOffset + pwd->iEdgeTransitionPixelsY && iYWhole < iSrcHeight - iOkEdgeOffset - pwd->iEdgeTransitionPixelsY))
										{	// Inside edge transition area
											double dfTransitionFactorTmp = (iXFuture2 / 67108864.0 - iOkEdgeOffset) / (double)pwd->iEdgeTransitionPixelsX;
											ASSERT(dfTransitionFactorTmp >= 0.0);
											if(dfTransitionFactorTmp < 1.0)
												dfTransitionFactor *= dfTransitionFactorTmp;

											dfTransitionFactorTmp = (iSrcWidth - iOkEdgeOffset - iXFuture2 / 67108864.0) / (double)pwd->iEdgeTransitionPixelsX;
											ASSERT(dfTransitionFactorTmp >= 0.0);
											if(dfTransitionFactorTmp < 1.0)
												dfTransitionFactor *= dfTransitionFactorTmp;

											dfTransitionFactorTmp = (iYFuture2 / 67108864.0 - iOkEdgeOffset) / (double)pwd->iEdgeTransitionPixelsY;
											ASSERT(dfTransitionFactorTmp >= 0.0);
											if(dfTransitionFactorTmp < 1.0)
												dfTransitionFactor *= dfTransitionFactorTmp;

											dfTransitionFactorTmp = (iSrcHeight - iOkEdgeOffset - iYFuture2 / 67108864.0) / (double)pwd->iEdgeTransitionPixelsY;
											ASSERT(dfTransitionFactorTmp >= 0.0);
											if(dfTransitionFactorTmp < 1.0)
												dfTransitionFactor *= dfTransitionFactorTmp;

											if(dfTransitionFactor < 0.00001)
												dfTransitionFactor = 0.00001;
										}
									}

									pix32 = *pdst;
									pbp->dfR += dfWeight * dfTransitionFactor * ((pix32 >> 16) & 0xff);
									pbp->dfG += dfWeight * dfTransitionFactor * ((pix32 >> 8) & 0xff);
									pbp->dfB += dfWeight * dfTransitionFactor * (pix32 & 0xff);
									pbp->dfTotWeight += dfWeight * dfTransitionFactor;
									bPixelOk = FALSE;
								}
							}
						}
					}

					if(!bPixelOk && pwd->bMultiFrameBorder)
					{
						dfWeight /= 1.3;
						bPixelOk = pbp->dfTotWeight > 256.0 * dfWeight;	// Exit loop when the weight has gotten too small to have any real influence on the pixel color.
					}
				}

				if(pwd->bMultiFrameBorder)
				{
					if(dfEdgeTransitionFactorCurFrame >= 1.0)
					{
						*pdst = pix32CurFrame;	// Shouldn't happen(?)
						bPixelOk = TRUE;
					}
					else
					{
						if(dfEdgeTransitionFactorCurFrame > 0.0)
						{
							double dfCurFrameWeight = pbp->dfTotWeight > 0.0 ? pbp->dfTotWeight / (1.0 / dfEdgeTransitionFactorCurFrame - 1.0) : 1.0;
							pbp->dfR += dfCurFrameWeight * ((pix32CurFrame >> 16) & 0xff);
							pbp->dfG += dfCurFrameWeight * ((pix32CurFrame >> 8) & 0xff);
							pbp->dfB += dfCurFrameWeight * (pix32CurFrame & 0xff);
							pbp->dfTotWeight += dfCurFrameWeight;
						}
						bPixelOk = pbp->dfTotWeight > 0.0;
						if(bPixelOk)
						{
							*pdst = ((int)(pbp->dfR / pbp->dfTotWeight + 0.5) << 16) +
									((int)(pbp->dfG / pbp->dfTotWeight + 0.5) << 8) +
									(int)(pbp->dfB / pbp->dfTotWeight + 0.5);
						}
					}
					delete pbp;
				}

				iPastFrame = pwd->iCurFrame;
				iFutureFrame = pwd->iCurFrame;
			}

			if(!bPixelOk && pwd->bExtrapolateBorder)
			{
				pbp = new RBorderPixel;
				pbp->iX = iDestX;
				pbp->iY = iDestY;
				pbp->dfR = 0.0;
				pbp->dfG = 0.0;
				pbp->dfB = 0.0;
				pbp->dfTotWeight = 0.0;
				pwd->csBorderPixel.Lock();
				sm_apBorderPixels[iDestY * iDestWidth + iDestX] = pbp;
				pwd->alstBorderPixelIndexes.AddTail(iDestY * iDestWidth + iDestX);
				pwd->csBorderPixel.Unlock();
			}

			if(++iDestX < iDestWidth)
			{
				iXOrg += iDeltaXForXAxis;
				iYOrg += iDeltaYForXAxis;
				pdst++;
			}
			else
			{
				iDestX = 0;
				iXOrg = iStartX + ++iDestY * iDeltaXForYAxis;
				iYOrg = iStartY + iDestY * iDeltaYForYAxis;
				pdst = (Pixel32 *)((char *)pdstStart + iDestY * iDestPitch);
			}
		}
	}

	return 0;
}

/// <summary>
/// Calculates pixel value at a certain point using bilinear interpolation. Only integer values are used for improved speed.
/// </summary>
void CVideoImage::VDubBilinearInterpolatePoint(const VBitmap *pbmp, int iX, int iY, Pixel32 &pix, Pixel32 *ppixMask/*=NULL*/)
{
	int iXF0 = iX & 1023;
	int iYF0 = iY & 1023;
	iX >>= 10;
	iY >>= 10;
	int iXF1 = 1024 - iXF0;
	int iYF1 = 1024 - iYF0;

	Pixel32 *psrc = (Pixel32 *)((char *)pbmp->data + iY * pbmp->pitch) + iX;

	Pixel32 pix00 = iY < 0 || iY >= pbmp->h || iX < 0 || iX >= pbmp->w ? BORDER_COLOR : *psrc;
	Pixel32 pix01 = iY < -1 || iY >= pbmp->h - 1 || iX < 0 || iX >= pbmp->w ? BORDER_COLOR : *((Pixel32 *)((char *)psrc + pbmp->pitch));
	Pixel32 pix10 = iY < 0 || iY >= pbmp->h || iX < -1 || iX >= pbmp->w - 1 ? BORDER_COLOR : *(psrc + 1);
	Pixel32 pix11 = iY < -1 || iY >= pbmp->h - 1 || iX < -1 || iX >= pbmp->w - 1 ? BORDER_COLOR : *((Pixel32 *)((char *)psrc + pbmp->pitch) + 1);

	if(ppixMask != NULL)
	{
		if((pix00 & 0xffffff) == *ppixMask || (pix01 & 0xffffff) == *ppixMask || (pix10 & 0xffffff) == *ppixMask || (pix11 & 0xffffff) == *ppixMask)	// & 0xffffff is needed because it seems VirtualDub sometimes puts trash in the alhpa channel(?)
		{
			// A source pixel is masked. Set destination pixel masked.
			pix = *ppixMask;
			return;
		}
	}

	int iF11 = iXF1 * iYF1;
	int iF10 = iXF1 * iYF0;
	int iF01 = iXF0 * iYF1;
	int iF00 = iXF0 * iYF0;
	pix = (((iF11 * (int)((pix00 >> 16) & 255) +
				iF10 * (int)((pix01 >> 16) & 255) +
				iF01 * (int)((pix10 >> 16) & 255) +
				iF00 * (int)((pix11 >> 16) & 255) + 524288) >> 20) << 16) +
			(((iF11 * (int)((pix00 >> 8) & 255) +
				iF10 * (int)((pix01 >> 8) & 255) +
				iF01 * (int)((pix10 >> 8) & 255) +
				iF00 * (int)((pix11 >> 8) & 255) + 524288) >> 20) << 8) +
			 ((iF11 * (int)(pix00 & 255) +
				iF10 * (int)(pix01 & 255) +
				iF01 * (int)(pix10 & 255) +
				iF00 * (int)(pix11 & 255) + 524288) >> 20);
}

/// <summary>
/// Calculates pixel value at a certain point using bicubic interpolation. Only integer values are used for improved speed.
/// </summary>
void CVideoImage::VDubBicubicInterpolatePoint(const VBitmap *pbmp, int iX, int iY, Pixel32 &pix, Pixel32 *ppixMask/*=NULL*/)
{
	int iXOrg = iX;
	int iYOrg = iY;

	int iXf = iX & 1023;
	int iYf = iY & 1023;
	iX >>= 10;
	iY >>= 10;

	if(iX < -2 || iY < -2 || iX > pbmp->w || iY > pbmp->h)
	{
		pix = BORDER_COLOR;
		return;
	}

	Pixel32 *psrc0 = (Pixel32 *)((char *)pbmp->data + iY * pbmp->pitch) + iX;
	Pixel32 *psrc9 = (Pixel32 *)((char *)psrc0 - pbmp->pitch);
	Pixel32 *psrc1 = (Pixel32 *)((char *)psrc0 + pbmp->pitch);
	Pixel32 *psrc2 = (Pixel32 *)((char *)psrc1 + pbmp->pitch);

	Pixel32 pix99, pix90, pix91, pix92, pix09, pix00, pix01, pix02, pix19, pix10, pix11, pix12, pix29, pix20, pix21, pix22;
	if(iX > 0 && iY > 0 && iX < pbmp->w - 2 && iY < pbmp->h - 2)
	{
		pix99 = *(psrc9 - 1);
		pix90 = *(psrc0 - 1);
		pix91 = *(psrc1 - 1);
		pix92 = *(psrc2 - 1);
		pix09 = *(psrc9);
		pix00 = *(psrc0);
		pix01 = *(psrc1);
		pix02 = *(psrc2);
		pix19 = *(psrc9 + 1);
		pix10 = *(psrc0 + 1);
		pix11 = *(psrc1 + 1);
		pix12 = *(psrc2 + 1);
		pix29 = *(psrc9 + 2);
		pix20 = *(psrc0 + 2);
		pix21 = *(psrc1 + 2);
		pix22 = *(psrc2 + 2);
	}
	else
	{
		pix99 = iY < 1 || iY >= pbmp->h + 1 || iX < 1 || iX >= pbmp->w + 1 ? BORDER_COLOR : *(psrc9 - 1);
		pix90 = iY < 0 || iY >= pbmp->h || iX < 1 || iX >= pbmp->w + 1 ? BORDER_COLOR : *(psrc0 - 1);
		pix91 = iY < -1 || iY >= pbmp->h -1 || iX < 1 || iX >= pbmp->w + 1 ? BORDER_COLOR : *(psrc1 - 1);
		pix92 = iY < -2 || iY >= pbmp->h - 2 || iX < 1 || iX >= pbmp->w + 1 ? BORDER_COLOR : *(psrc2 - 1);
		pix09 = iY < 1 || iY >= pbmp->h + 1 || iX < 0 || iX >= pbmp->w ? BORDER_COLOR : *(psrc9);
		pix00 = iY < 0 || iY >= pbmp->h || iX < 0 || iX >= pbmp->w ? BORDER_COLOR : *(psrc0);
		pix01 = iY < -1 || iY >= pbmp->h -1 || iX < 0 || iX >= pbmp->w ? BORDER_COLOR : *(psrc1);
		pix02 = iY < -2 || iY >= pbmp->h - 2 || iX < 0 || iX >= pbmp->w ? BORDER_COLOR : *(psrc2);
		pix19 = iY < 1 || iY >= pbmp->h + 1 || iX < -1 || iX >= pbmp->w - 1 ? BORDER_COLOR : *(psrc9 + 1);
		pix10 = iY < 0 || iY >= pbmp->h || iX < -1 || iX >= pbmp->w - 1 ? BORDER_COLOR : *(psrc0 + 1);
		pix11 = iY < -1 || iY >= pbmp->h -1 || iX < -1 || iX >= pbmp->w - 1 ? BORDER_COLOR : *(psrc1 + 1);
		pix12 = iY < -2 || iY >= pbmp->h - 2 || iX < -1 || iX >= pbmp->w - 1 ? BORDER_COLOR : *(psrc2 + 1);
		pix29 = iY < 1 || iY >= pbmp->h + 1 || iX < -2 || iX >= pbmp->w - 2 ? BORDER_COLOR : *(psrc9 + 2);
		pix20 = iY < 0 || iY >= pbmp->h || iX < -2 || iX >= pbmp->w - 2 ? BORDER_COLOR : *(psrc0 + 2);
		pix21 = iY < -1 || iY >= pbmp->h -1 || iX < -2 || iX >= pbmp->w - 2 ? BORDER_COLOR : *(psrc1 + 2);
		pix22 = iY < -2 || iY >= pbmp->h - 2 || iX < -2 || iX >= pbmp->w - 2 ? BORDER_COLOR : *(psrc2 + 2);
	}

	if(ppixMask != NULL)
	{
		if((pix00 & 0xffffff) == *ppixMask || (pix01 & 0xffffff) == *ppixMask || (pix10 & 0xffffff) == *ppixMask || (pix11 & 0xffffff) == *ppixMask)	// & 0xffffff is needed because it seems VirtualDub sometimes puts trash in the alhpa channel(?)
		{
			// A source pixel is masked. Set destination pixel masked.
			pix = *ppixMask;
			return;
		}

		if((pix99 & 0xffffff) == *ppixMask || (pix90 & 0xffffff) == *ppixMask || (pix91 & 0xffffff) == *ppixMask || (pix92 & 0xffffff) == *ppixMask ||
			(pix09 & 0xffffff) == *ppixMask || (pix02 & 0xffffff) == *ppixMask ||
			(pix19 & 0xffffff) == *ppixMask || (pix12 & 0xffffff) == *ppixMask ||
			(pix29 & 0xffffff) == *ppixMask || (pix20 & 0xffffff) == *ppixMask || (pix21 & 0xffffff) == *ppixMask || (pix22 & 0xffffff) == *ppixMask)
		{
			// A source pixel is masked, but bilinear will work here, because it doesn't use any source pixels that are masked.
			VDubBilinearInterpolatePoint(pbmp, iXOrg, iYOrg, pix, ppixMask);
			return;
		}
	}

	int iIndex = iXf << 2;
	int iC0X = sm_aiCubicTable[iIndex++];
	int iC1X = sm_aiCubicTable[iIndex++];
	int iC2X = sm_aiCubicTable[iIndex++];
	int iC3X = sm_aiCubicTable[iIndex];
	iIndex = iYf << 2;
	int iC0Y = sm_aiCubicTable[iIndex++];
	int iC1Y = sm_aiCubicTable[iIndex++];
	int iC2Y = sm_aiCubicTable[iIndex++];
	int iC3Y = sm_aiCubicTable[iIndex];

	int iH9 = (iC3X * (int)((pix99 >> 16) & 255) + iC2X * (int)((pix09 >> 16) & 255) + iC1X * (int)((pix19 >> 16) & 255) + iC0X * (int)((pix29 >> 16) & 255) + 2048) >> 12;
	int iH0 = (iC3X * (int)((pix90 >> 16) & 255) + iC2X * (int)((pix00 >> 16) & 255) + iC1X * (int)((pix10 >> 16) & 255) + iC0X * (int)((pix20 >> 16) & 255) + 2048) >> 12;
	int iH1 = (iC3X * (int)((pix91 >> 16) & 255) + iC2X * (int)((pix01 >> 16) & 255) + iC1X * (int)((pix11 >> 16) & 255) + iC0X * (int)((pix21 >> 16) & 255) + 2048) >> 12;
	int iH2 = (iC3X * (int)((pix92 >> 16) & 255) + iC2X * (int)((pix02 >> 16) & 255) + iC1X * (int)((pix12 >> 16) & 255) + iC0X * (int)((pix22 >> 16) & 255) + 2048) >> 12;
	int iTmp = (iC3Y * iH9 + iC2Y * iH0 + iC1Y * iH1 + iC0Y * iH2 + 32768) >> 16;
	if(iTmp < 0)
		iTmp = 0;
	else if(iTmp > 255)
		iTmp = 255;
	pix = iTmp << 16;
	
	iH9 = (iC3X * (int)((pix99 >> 8) & 255) + iC2X * (int)((pix09 >> 8) & 255) + iC1X * (int)((pix19 >> 8) & 255) + iC0X * (int)((pix29 >> 8) & 255) + 2048) >> 12;
	iH0 = (iC3X * (int)((pix90 >> 8) & 255) + iC2X * (int)((pix00 >> 8) & 255) + iC1X * (int)((pix10 >> 8) & 255) + iC0X * (int)((pix20 >> 8) & 255) + 2048) >> 12;
	iH1 = (iC3X * (int)((pix91 >> 8) & 255) + iC2X * (int)((pix01 >> 8) & 255) + iC1X * (int)((pix11 >> 8) & 255) + iC0X * (int)((pix21 >> 8) & 255) + 2048) >> 12;
	iH2 = (iC3X * (int)((pix92 >> 8) & 255) + iC2X * (int)((pix02 >> 8) & 255) + iC1X * (int)((pix12 >> 8) & 255) + iC0X * (int)((pix22 >> 8) & 255) + 2048) >> 12;
	iTmp = (iC3Y * iH9 + iC2Y * iH0 + iC1Y * iH1 + iC0Y * iH2 + 32768) >> 16;
	if(iTmp < 0)
		iTmp = 0;
	else if(iTmp > 255)
		iTmp = 255;
	pix |= iTmp << 8;
	
	iH9 = (iC3X * (int)(pix99 & 255) + iC2X * (int)(pix09 & 255) + iC1X * (int)(pix19 & 255) + iC0X * (int)(pix29 & 255) + 2048) >> 12;
	iH0 = (iC3X * (int)(pix90 & 255) + iC2X * (int)(pix00 & 255) + iC1X * (int)(pix10 & 255) + iC0X * (int)(pix20 & 255) + 2048) >> 12;
	iH1 = (iC3X * (int)(pix91 & 255) + iC2X * (int)(pix01 & 255) + iC1X * (int)(pix11 & 255) + iC0X * (int)(pix21 & 255) + 2048) >> 12;
	iH2 = (iC3X * (int)(pix92 & 255) + iC2X * (int)(pix02 & 255) + iC1X * (int)(pix12 & 255) + iC0X * (int)(pix22 & 255) + 2048) >> 12;
	iTmp = (iC3Y * iH9 + iC2Y * iH0 + iC1Y * iH1 + iC0Y * iH2 + 32768) >> 16;
	if(iTmp < 0)
		iTmp = 0;
	else if(iTmp > 255)
		iTmp = 255;
	pix |= iTmp;
}

/// <summary>
/// Precalculates stuff for the bicubic resampling.
/// </summary>
void CVideoImage::PrepareCubicTable(double dfA)
{
	delete[] sm_aiCubicTable;
	sm_aiCubicTable = new int[4 * 1024];

	int iIndex = 0;
	for(int i = 0; i < 1024; i++)
	{
		double dfFrac = i / 1024.0;
		double dfFrac2 = dfFrac * dfFrac;
		double dfFrac3 = dfFrac2 * dfFrac;

		sm_aiCubicTable[iIndex++] = (int)floor((-dfA * dfFrac3 + dfA * dfFrac2) * 16384 + 0.5);
		sm_aiCubicTable[iIndex++] = (int)floor((-(dfA + 2.0) * dfFrac3 + (2.0 * dfA + 3.0) * dfFrac2 - dfA * dfFrac) * 16384 + 0.5);
		sm_aiCubicTable[iIndex++] = (int)floor(((dfA + 2.0) * dfFrac3 - (dfA + 3.0) * dfFrac2 + 1) * 16384 + 0.5);
		sm_aiCubicTable[iIndex++] = (int)floor((dfA * dfFrac3 - (2.0 * dfA) * dfFrac2 + dfA * dfFrac) * 16384 + 0.5);
	}
}

void CVideoImage::FreeCubicTable()
{
	delete[] sm_aiCubicTable;
	sm_aiCubicTable = NULL;
}

void CVideoImage::PrepareBorderPixelArray(int iSize)
{
	delete[] sm_apBorderPixels;
	sm_apBorderPixels = new (RBorderPixel *[iSize]);
	for(int i = 0; i < iSize; i++)
		sm_apBorderPixels[i] = NULL;
}

void CVideoImage::FreeBorderPixelArray()
{
	delete[] sm_apBorderPixels;
	sm_apBorderPixels = NULL;
}

// Can be used for testing.
//int CVideoImage::SaveBitmap(LPCSTR szBMPFile)
//{
//	if(!m_bContainsImage)
//		return VIMAGE_FAILED;
//
//	CFile fileBMP;
//	CFileException e;
//	if(!fileBMP.Open(szBMPFile, CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive | CFile::typeBinary, &e))
//	{
//		return VIMAGE_FAILED;
//	}
//
//	CByteArray abyBits;
//	Make24BitArray(abyBits);
//
//	BITMAPFILEHEADER bmfHdr;
//	bmfHdr.bfType = ((WORD)('M' << 8) | 'B');
//	bmfHdr.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + abyBits.GetSize();
//	bmfHdr.bfReserved1 = 0;
//	bmfHdr.bfReserved2 = 0;
//	bmfHdr.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
//
//	BITMAPINFOHEADER bmiHdr;
//	bmiHdr.biSize = sizeof(BITMAPINFOHEADER);
//	bmiHdr.biWidth = m_iWidth;
//	bmiHdr.biHeight = m_iHeight;
//	bmiHdr.biPlanes = 1;
//	bmiHdr.biBitCount = 24;
//	bmiHdr.biCompression = BI_RGB;
//	bmiHdr.biSizeImage = abyBits.GetSize();
//	bmiHdr.biXPelsPerMeter = 0;
//	bmiHdr.biYPelsPerMeter = 0;
//	bmiHdr.biClrUsed = 0;
//	bmiHdr.biClrImportant = 0;
//
//	try
//	{
//		fileBMP.Write(&bmfHdr, sizeof(BITMAPFILEHEADER));
//		fileBMP.Write(&bmiHdr, sizeof(BITMAPINFOHEADER));
//		fileBMP.Write(abyBits.GetData(), abyBits.GetSize());
//	}
//	catch(CException *pe)
//	{
//		pe->Delete();
//		return VIMAGE_FAILED;
//	}
//
//	return VIMAGE_OK;
//}
//
//void CVideoImage::Make24BitArray(CByteArray &abyBits)
//{
//	long lBytesPerRow = 3 * m_iWidth;
//	int iPadBytes = (4 - (lBytesPerRow % 4)) % 4;
//	abyBits.SetSize((3 * m_iWidth + iPadBytes) * m_iHeight);
//
//	long lPos = 0;
//	RGBPixel pix;
//	for(int iY = m_iHeight - 1; iY >= 0; iY--)
//	{
//		for(int iX = 0; iX < m_iWidth; iX++)
//		{
//			pix = ElementAt(iY * m_iWidth + iX);
//			abyBits[lPos++] = pix.b;
//			abyBits[lPos++] = (BYTE)pix.g;
//			abyBits[lPos++] = pix.r;
//		}
//
//		for(int iX = 0; iX < iPadBytes; iX++)
//			abyBits[lPos++] = 0;
//	}
//}
