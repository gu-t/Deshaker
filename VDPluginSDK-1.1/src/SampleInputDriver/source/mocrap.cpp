#include <vd2/plugin/vdplugin.h>
#include <vd2/plugin/vdinputdriver.h>
#include <vector>
#include <string>
#include <string.h>
#include <math.h>
#include "resource.h"
#include <vd2/VDXFrame/Unknown.h>
#include <vd2/VDXFrame/VideoFilterDialog.h>

struct MoCrapHeader {
	uint32	mSignature;
	uint32	mVersion;
	uint32	mWidth;
	uint32	mHeight;
	uint32	mFrames;
	uint32	mFrameRateNumerator;
	uint32	mFrameRateDenominator;
	uint32	mUnused0;
	uint64	mIndexOffset;
	uint64	mFrameDataOffset;
};

struct MoCrapIndexEntry {
	uint64	mFrameOffset;
	uint32	mSize;
	uint8	mFrameType;
	uint8	mUnused[3];
};

class MoCrapData {
public:
	FILE *mFile;
	MoCrapHeader	mHeader;
	std::vector<MoCrapIndexEntry> mIndex;
};

///////////////////////////////////////////////////////////////////////////////

class VDVideoDecoderPluginMOCRAP : public vdxunknown<IVDXVideoDecoder> {
public:
	VDVideoDecoderPluginMOCRAP(const MoCrapData& data);
	~VDVideoDecoderPluginMOCRAP();

	const void *VDXAPIENTRY DecodeFrame(const void *inputBuffer, uint32 data_len, bool is_preroll, sint64 streamFrame, sint64 targetFrame);
	uint32		VDXAPIENTRY GetDecodePadding();
	void		VDXAPIENTRY Reset();
	bool		VDXAPIENTRY IsFrameBufferValid();
	const VDXPixmap& VDXAPIENTRY GetFrameBuffer();
	bool		VDXAPIENTRY SetTargetFormat(int format, bool useDIBAlignment);
	bool		VDXAPIENTRY SetDecompressedFormat(const VDXBITMAPINFOHEADER *pbih);

	const void *VDXAPIENTRY GetFrameBufferBase();
	bool		VDXAPIENTRY IsDecodable(sint64 sample_num);

protected:
	sint64	mLastFrame;
	int		mBlockW;
	int		mBlockH;

	VDXPixmap	mPixmap;

	std::vector<uint8>	mFrameBuffer;

	VDXPixmap			mYCbCrBufferBidir;
	VDXPixmap			mYCbCrBufferPrev;
	VDXPixmap			mYCbCrBufferNext;
	std::vector<uint8>	mYCbCrBufferMemory;
	sint64				mBidirFrame;
	sint64				mPrevFrame;
	sint64				mNextFrame;

	const MoCrapData&	mData;
};

VDVideoDecoderPluginMOCRAP::VDVideoDecoderPluginMOCRAP(const MoCrapData& data)
	: mData(data)
	, mBlockW((data.mHeader.mWidth  + 7) >> 3)
	, mBlockH((data.mHeader.mHeight + 7) >> 3)
{
	mFrameBuffer.resize(data.mHeader.mWidth * data.mHeader.mHeight * 4);

	mYCbCrBufferMemory.resize(mBlockW * mBlockH * 64 * 3);

	mYCbCrBufferBidir.data = &mYCbCrBufferMemory[mBlockW*mBlockH*64*0];
	mYCbCrBufferBidir.format = nsVDXPixmap::kPixFormat_Y8;
	mYCbCrBufferBidir.w = mBlockW*8;
	mYCbCrBufferBidir.h = mBlockW*8;
	mYCbCrBufferBidir.pitch = mBlockW*8;

	mYCbCrBufferPrev.data = &mYCbCrBufferMemory[mBlockW*mBlockH*64*1];
	mYCbCrBufferPrev.format = nsVDXPixmap::kPixFormat_Y8;
	mYCbCrBufferPrev.w = mBlockW*8;
	mYCbCrBufferPrev.h = mBlockW*8;
	mYCbCrBufferPrev.pitch = mBlockW*8;

	mYCbCrBufferNext.data = &mYCbCrBufferMemory[mBlockW*mBlockH*64*2];
	mYCbCrBufferNext.format = nsVDXPixmap::kPixFormat_Y8;
	mYCbCrBufferNext.w = mBlockW*8;
	mYCbCrBufferNext.h = mBlockW*8;
	mYCbCrBufferNext.pitch = mBlockW*8;
}

VDVideoDecoderPluginMOCRAP::~VDVideoDecoderPluginMOCRAP() {
}

namespace {
	void CopyRef8x8(const VDXPixmap& dst, int dstx, int dsty, const VDXPixmap& src, int srcx, int srcy) {
		const uint8 *srcY = (const uint8 *)src.data + src.pitch * srcy + srcx;
		uint8 *dstY = (uint8 *)dst.data + dst.pitch * dsty + dstx;

		for(int by=0; by<8; ++by) {
			for(int bx=0; bx<8; ++bx)
				dstY[bx] = srcY[bx];
			srcY += src.pitch;
			dstY += dst.pitch;
		}
	}

	void AvgRef8x8(const VDXPixmap& dst, int dstx, int dsty, const VDXPixmap& src, int srcx, int srcy) {
		const uint8 *srcY = (const uint8 *)src.data + src.pitch * srcy + srcx;
		uint8 *dstY = (uint8 *)dst.data + dst.pitch * dsty + dstx;

		for(int by=0; by<8; ++by) {
			for(int bx=0; bx<8; ++bx)
				dstY[bx] = (uint8)(((uint32)dstY[bx] + (uint32)srcY[bx] + 1) >> 1);
			srcY += src.pitch;
			dstY += dst.pitch;
		}
	}

	void DecodeY8ToX8R8G8B8(const VDXPixmap& dst, const VDXPixmap& src) {
		uint32 *dstp = (uint32 *)dst.data;
		const uint8 *srcp = (const uint8 *)src.data;

		for(int y=0; y<dst.h; ++y) {
			for(int x=0; x<dst.w; ++x)
				dstp[x] = srcp[x] * 0x010101 + 0xFF000000;

			dstp = (uint32 *)((char *)dstp + dst.pitch);
			srcp += src.pitch;
		}
	}
}

const void *VDVideoDecoderPluginMOCRAP::DecodeFrame(const void *inputBuffer, uint32 data_len, bool is_preroll, sint64 streamFrame, sint64 targetFrame) {
	if (data_len) {
		uint8 frameType = mData.mIndex[(uint32)streamFrame].mFrameType;
		bool ip_frame = frameType <= 1;
		bool pb_frame = frameType >= 1;

		const uint8 *src = (const uint8 *)inputBuffer;

		if (ip_frame) {
			// swap nearest previous reference into place
			if ((uint64)(streamFrame - mNextFrame) < (uint64)(streamFrame - mPrevFrame)) {
				std::swap(mYCbCrBufferPrev.data, mYCbCrBufferNext.data);
				std::swap(mPrevFrame, mNextFrame);
			}
		}

		uint8 *dstY = (uint8 *)mYCbCrBufferBidir.data;
		for(int y=0; y<mBlockH; ++y) {
			for(int x=0; x<mBlockW; ++x) {
				uint8 code = *src++;

				static const int kDeltaOffsets[15]={-21, -13, -8, -5, -3, -2, -1, 0, 1, 2, 3, 5, 8, 13, 21 };

				uint8 *dstY2 = dstY;
				if (code == 0) {
					for(int by=0; by<8; ++by) {
						for(int bx=0; bx<8; ++bx)
							dstY2[bx] = *src++;

						dstY2 += mYCbCrBufferBidir.pitch;
					}
				} else if (code == 0x01) {
					code = *src++;
					
					for(int by=0; by<8; ++by) {
						for(int bx=0; bx<8; ++bx)
							dstY2[bx] = code;

						dstY2 += mYCbCrBufferBidir.pitch;
					}
				} else if (code == 0x02) {
					for(int by=0; by<8; ++by) {
						for(int bx=0; bx<8; bx += 2) {
							uint8 c = *src++;
							dstY2[bx] = (c << 4) + 0x08;
							dstY2[bx+1] = (c & 0xf0) + 0x08;
						}

						dstY2 += mYCbCrBufferBidir.pitch;
					}
				} else if (code == 0x03) {
					uint8 *dstY1 = dstY2;
					dstY2 += mYCbCrBufferBidir.pitch;
					for(int by=0; by<8; by += 2) {
						for(int bx=0; bx<8; bx += 4) {
							uint8 c = *src++;

							uint8 a = (c << 4) + 8;
							uint8 b = (c & 0xf0) + 8;
							dstY1[bx  ] = a;
							dstY1[bx+1] = a;
							dstY2[bx  ] = a;
							dstY2[bx+1] = a;
							dstY1[bx+2] = b;
							dstY1[bx+3] = b;
							dstY2[bx+2] = b;
							dstY2[bx+3] = b;
						}

						dstY1 += mYCbCrBufferBidir.pitch*2;
						dstY2 += mYCbCrBufferBidir.pitch*2;
					}
				} else if (code == 0x1D) {
					code = *src++;
					int refx2 = x*8 + kDeltaOffsets[((int)code - 31) % 15];
					int refy2 = y*8 + kDeltaOffsets[((int)code - 31) / 15];

					CopyRef8x8(mYCbCrBufferBidir, x*8, y*8, mYCbCrBufferNext, refx2, refy2);
				} else if (code == 0x1E) {
					code = *src++;
					int refx1 = x*8 + kDeltaOffsets[((int)code - 31) % 15];
					int refy1 = y*8 + kDeltaOffsets[((int)code - 31) / 15];
					code = *src++;
					int refx2 = x*8 + kDeltaOffsets[((int)code - 31) % 15];
					int refy2 = y*8 + kDeltaOffsets[((int)code - 31) / 15];

					CopyRef8x8(mYCbCrBufferBidir, x*8, y*8, mYCbCrBufferPrev, refx1, refy1);
					AvgRef8x8(mYCbCrBufferBidir, x*8, y*8, mYCbCrBufferNext, refx2, refy2);
				} else if (code >= 31) {
					int refx = x*8 + kDeltaOffsets[((int)code - 31) % 15];
					int refy = y*8 + kDeltaOffsets[((int)code - 31) / 15];

					CopyRef8x8(mYCbCrBufferBidir, x*8, y*8, mYCbCrBufferPrev, refx, refy);
				}

				dstY += 8;
			}

			dstY += mYCbCrBufferBidir.pitch * 8 - mBlockW * 8;
		}

		mBidirFrame = streamFrame;

		if (ip_frame) {
			std::swap(mYCbCrBufferBidir.data, mYCbCrBufferNext.data);
			std::swap(mBidirFrame, mNextFrame);
		}
	}

	if (!is_preroll) {
		if (mBidirFrame == targetFrame)
			DecodeY8ToX8R8G8B8(mPixmap, mYCbCrBufferBidir);
		else if (mNextFrame == targetFrame)
			DecodeY8ToX8R8G8B8(mPixmap, mYCbCrBufferNext);
		else if (mPrevFrame == targetFrame)
			DecodeY8ToX8R8G8B8(mPixmap, mYCbCrBufferPrev);

		mLastFrame = targetFrame;
	}

	return &mFrameBuffer[0];
}

uint32 VDVideoDecoderPluginMOCRAP::GetDecodePadding() {
	return 0;
}

void VDVideoDecoderPluginMOCRAP::Reset() {
	mLastFrame = -1;
	mPrevFrame = -1;
	mBidirFrame = -1;
	mNextFrame = -1;
}

bool VDVideoDecoderPluginMOCRAP::IsFrameBufferValid() {
	return mLastFrame >= 0;
}

const VDXPixmap& VDVideoDecoderPluginMOCRAP::GetFrameBuffer() {
	return mPixmap;
}

bool VDVideoDecoderPluginMOCRAP::SetTargetFormat(int format, bool useDIBAlignment) {
	if (format == 0)
		format = nsVDXPixmap::kPixFormat_XRGB8888;

	if (format != nsVDXPixmap::kPixFormat_XRGB8888)
		return false;

	mPixmap.data			= &mFrameBuffer[0];
	mPixmap.palette			= NULL;
	mPixmap.format			= format;
	mPixmap.w				= mData.mHeader.mWidth;
	mPixmap.h				= mData.mHeader.mHeight;
	mPixmap.pitch			= mData.mHeader.mWidth * 4;
	mPixmap.data2			= NULL;
	mPixmap.pitch2			= 0;
	mPixmap.data3			= NULL;
	mPixmap.pitch3			= 0;

	if (useDIBAlignment) {
		mPixmap.data	= (char *)mPixmap.data + mPixmap.pitch*(mPixmap.h - 1);
		mPixmap.pitch	= -mPixmap.pitch;
	}

	Reset();

	return true;
}

bool VDVideoDecoderPluginMOCRAP::SetDecompressedFormat(const VDXBITMAPINFOHEADER *pbih) {
	return false;
}

const void *VDVideoDecoderPluginMOCRAP::GetFrameBufferBase() {
	return &mFrameBuffer[0];
}

bool VDVideoDecoderPluginMOCRAP::IsDecodable(sint64 sample_num64) {
	uint32 sample_num = (uint32)sample_num64;
	uint8 frameType = mData.mIndex[sample_num].mFrameType;

	switch(frameType) {
		// B frames require two predictive frames
		case 2:
			for(;;) {
				--sample_num;

				if (mData.mIndex[sample_num].mFrameType < 2)
					break;
			}

			if (mPrevFrame != sample_num && mNextFrame != sample_num)
				return false;

			// fall through

		case 1:
			// P and B frames require one predictive frame
			for(;;) {
				--sample_num;

				if (mData.mIndex[sample_num].mFrameType < 2)
					break;
			}

			if (mPrevFrame != sample_num && mNextFrame != sample_num)
				return false;

		// I frames are always decodable
		case 0:
		default:
			return true;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////

class VDVideoDecoderModelPluginMOCRAP : public vdxunknown<IVDXVideoDecoderModel> {
public:
	VDVideoDecoderModelPluginMOCRAP(const MoCrapData& data);
	~VDVideoDecoderModelPluginMOCRAP();

	void	VDXAPIENTRY Reset();
	void	VDXAPIENTRY SetDesiredFrame(sint64 frame_num);
	sint64	VDXAPIENTRY GetNextRequiredSample(bool& is_preroll);
	int		VDXAPIENTRY GetRequiredCount();

protected:
	sint64	mLastPrevFrame;
	sint64	mLastNextFrame;
	sint64	mLastBidirFrame;
	sint64	mTargetFrame;
	sint64	mNextFrame;

	const MoCrapData& mData;
};

VDVideoDecoderModelPluginMOCRAP::VDVideoDecoderModelPluginMOCRAP(const MoCrapData& data)
	: mData(data)
{
}

VDVideoDecoderModelPluginMOCRAP::~VDVideoDecoderModelPluginMOCRAP() {
}

void VDVideoDecoderModelPluginMOCRAP::Reset() {
	mLastPrevFrame = -1;
	mLastNextFrame = -1;
	mLastBidirFrame = -1;
}

void VDVideoDecoderModelPluginMOCRAP::SetDesiredFrame(sint64 frame_num) {
	mTargetFrame = frame_num;

	if (mLastPrevFrame == frame_num || mLastNextFrame == frame_num || mLastBidirFrame == frame_num) {
		mNextFrame = -1;
		return;
	}

	uint8 frameType = mData.mIndex[(uint32)frame_num].mFrameType;

	// I-frame is trivial
	if (frameType == 0) {
		mNextFrame = frame_num;
		return;
	}

	// P-frame is more annoying
	if (frameType == 1) {
		// find previous IP frame
		sint64 prevP = frame_num ? frame_num - 1 : 0;
		while(prevP > 0 && mData.mIndex[(uint32)prevP].mFrameType == 2)
			--prevP;

		// find previous I frame
		sint64 prevI = prevP;
		while(prevI > 0 && mData.mIndex[(uint32)prevI].mFrameType != 0)
			--prevI;

		// what's the nearest frame that we have?
		sint64 validPrev = mLastPrevFrame >= prevI && mLastPrevFrame < frame_num ? mLastPrevFrame : -1;
		sint64 validNext = mLastNextFrame >= prevI && mLastNextFrame < frame_num ? mLastNextFrame : -1;

		if (validPrev > validNext)
			mNextFrame = validPrev + 1;
		else if (validNext > validPrev)
			mNextFrame = validNext + 1;
		else
			mNextFrame = prevI;
		return;
	}

	// B-frame is very annoying

	// find previous IP frame
	sint64 prevP2 = frame_num;
	while(prevP2 > 0 && mData.mIndex[(uint32)prevP2].mFrameType == 2)
		--prevP2;

	// find previous IP frame before that
	sint64 prevP1 = prevP2 ? prevP2 - 1 : 0;
	while(prevP1 > 0 && mData.mIndex[(uint32)prevP1].mFrameType == 2)
		--prevP1;

	// If we have next & prev, we can just decode off that.
	// If we have only prev, then we can decode from that.
	// Otherwise, we need to decode from the nearest reference to prev.

	bool havePrev = (mLastPrevFrame == prevP1 || mLastNextFrame == prevP1);
	bool haveNext = (mLastPrevFrame == prevP2 || mLastNextFrame == prevP2);

	if (havePrev) {
		if (haveNext)
			mNextFrame = prevP2 + 1;
		else
			mNextFrame = prevP1 + 1;
		return;
	}

	// Okay, we need to decode the prev frame. Find the nearest I frame.
	sint64 prevI = prevP1;
	while(prevI > 0 && mData.mIndex[(uint32)prevI].mFrameType != 0)
		--prevI;

	// what's the nearest frame that we have?
	sint64 validPrev = mLastPrevFrame >= prevI && mLastPrevFrame < prevP1 ? mLastPrevFrame : -1;
	sint64 validNext = mLastNextFrame >= prevI && mLastNextFrame < prevP1 ? mLastNextFrame : -1;

	if (validPrev > validNext) {
		std::swap(mLastPrevFrame, mLastNextFrame);
		mNextFrame = validPrev + 1;
	} else if (validNext > validPrev) {
		std::swap(mLastNextFrame, mLastPrevFrame);
		mNextFrame = validNext + 1;
	} else {
		mNextFrame = prevI;
	}
}

sint64 VDVideoDecoderModelPluginMOCRAP::GetNextRequiredSample(bool& is_preroll) {
	if (mNextFrame < 0) {
		is_preroll = false;
		sint64 frame = mNextFrame;
		mNextFrame = -1;
		return frame;
	}

	is_preroll = true;
	sint64 frame;
	
	do {
		frame = mNextFrame++;
	} while(frame != mTargetFrame && mData.mIndex[(uint32)frame].mFrameType == 2);

	if (mData.mIndex[(uint32)frame].mFrameType == 2) {
		mLastBidirFrame = frame;
	} else {
		if ((uint64)(frame - mLastNextFrame) < (uint64)(frame - mLastPrevFrame))
			std::swap(mLastPrevFrame, mLastNextFrame);

		mLastNextFrame = frame;
	}

	if (frame == mTargetFrame) {
		mNextFrame = -1;
		is_preroll = false;
	}

	return frame;
}

int VDVideoDecoderModelPluginMOCRAP::GetRequiredCount() {
	return mNextFrame == -1 ? 0 : 1;
}

///////////////////////////////////////////////////////////////////////////////

class VDVideoSourcePluginMOCRAP : public vdxunknown<IVDXStreamSource>, public IVDXVideoSource {
public:
	VDVideoSourcePluginMOCRAP(MoCrapData& data);
	~VDVideoSourcePluginMOCRAP();

	int VDXAPIENTRY AddRef();
	int VDXAPIENTRY Release();
	void *VDXAPIENTRY AsInterface(uint32 iid);

	void		VDXAPIENTRY GetStreamSourceInfo(VDXStreamSourceInfo&);
	bool		VDXAPIENTRY Read(sint64 lStart, uint32 lCount, void *lpBuffer, uint32 cbBuffer, uint32 *lBytesRead, uint32 *lSamplesRead);

	const void *VDXAPIENTRY GetDirectFormat();
	int			VDXAPIENTRY GetDirectFormatLen();

	ErrorMode VDXAPIENTRY GetDecodeErrorMode();
	void VDXAPIENTRY SetDecodeErrorMode(ErrorMode mode);
	bool VDXAPIENTRY IsDecodeErrorModeSupported(ErrorMode mode);

	bool VDXAPIENTRY IsVBR();
	sint64 VDXAPIENTRY TimeToPositionVBR(sint64 us);
	sint64 VDXAPIENTRY PositionToTimeVBR(sint64 samples);

	void VDXAPIENTRY GetVideoSourceInfo(VDXVideoSourceInfo& info);

	bool VDXAPIENTRY CreateVideoDecoderModel(IVDXVideoDecoderModel **ppModel);
	bool VDXAPIENTRY CreateVideoDecoder(IVDXVideoDecoder **ppDecoder);

	void		VDXAPIENTRY GetSampleInfo(sint64 sample_num, VDXVideoFrameInfo& frameInfo);

	bool		VDXAPIENTRY IsKey(sint64 lSample);

	sint64		VDXAPIENTRY GetFrameNumberForSample(sint64 sample_num);
	sint64		VDXAPIENTRY GetSampleNumberForFrame(sint64 display_num);
	sint64		VDXAPIENTRY GetRealFrame(sint64 display_num);

	sint64		VDXAPIENTRY GetSampleBytePosition(sint64 sample_num);

protected:
	MoCrapData&	mData;
};

VDVideoSourcePluginMOCRAP::VDVideoSourcePluginMOCRAP(MoCrapData& data)
	: mData(data)
{
}

VDVideoSourcePluginMOCRAP::~VDVideoSourcePluginMOCRAP() {
}

int VDVideoSourcePluginMOCRAP::AddRef() {
	return vdxunknown<IVDXStreamSource>::AddRef();
}

int VDVideoSourcePluginMOCRAP::Release() {
	return vdxunknown<IVDXStreamSource>::Release();
}

void *VDXAPIENTRY VDVideoSourcePluginMOCRAP::AsInterface(uint32 iid) {
	if (iid == IVDXVideoSource::kIID)
		return static_cast<IVDXVideoSource *>(this);

	return vdxunknown<IVDXStreamSource>::AsInterface(iid);
}

void VDXAPIENTRY VDVideoSourcePluginMOCRAP::GetStreamSourceInfo(VDXStreamSourceInfo& srcInfo) {
	srcInfo.mSampleRate.mNumerator = mData.mHeader.mFrameRateNumerator;
	srcInfo.mSampleRate.mDenominator = mData.mHeader.mFrameRateDenominator;
	srcInfo.mSampleCount = mData.mHeader.mFrames;
}

bool VDVideoSourcePluginMOCRAP::Read(sint64 lStart64, uint32 lCount, void *lpBuffer, uint32 cbBuffer, uint32 *lBytesRead, uint32 *lSamplesRead) {
	uint32 start = (uint32)lStart64;

	const MoCrapIndexEntry& ent = mData.mIndex[start];

	*lBytesRead = ent.mSize;
	*lSamplesRead = 1;

	if (lpBuffer) {
		if (cbBuffer < ent.mSize)
			return false;

		fseek(mData.mFile, (long)(mData.mHeader.mFrameDataOffset + ent.mFrameOffset), SEEK_SET);
		fread(lpBuffer, ent.mSize, 1, mData.mFile);
	}

	return true;
}

const void *VDVideoSourcePluginMOCRAP::GetDirectFormat() {
	return NULL;
}

int VDVideoSourcePluginMOCRAP::GetDirectFormatLen() {
	return 0;
}

IVDXStreamSource::ErrorMode VDVideoSourcePluginMOCRAP::GetDecodeErrorMode() {
	return IVDXStreamSource::kErrorModeReportAll;
}

void VDVideoSourcePluginMOCRAP::SetDecodeErrorMode(IVDXStreamSource::ErrorMode mode) {
}

bool VDVideoSourcePluginMOCRAP::IsDecodeErrorModeSupported(IVDXStreamSource::ErrorMode mode) {
	return mode == IVDXStreamSource::kErrorModeReportAll;
}

bool VDVideoSourcePluginMOCRAP::IsVBR() {
	return false;
}

sint64 VDVideoSourcePluginMOCRAP::TimeToPositionVBR(sint64 us) {
	return (sint64)(0.5 + us / 1000000.0 * (double)mData.mHeader.mFrameRateNumerator / (double)mData.mHeader.mFrameRateDenominator);
}

sint64 VDVideoSourcePluginMOCRAP::PositionToTimeVBR(sint64 samples) {
	return (sint64)(0.5 + samples * 1000000.0 * (double)mData.mHeader.mFrameRateDenominator / (double)mData.mHeader.mFrameRateNumerator);
}

void VDVideoSourcePluginMOCRAP::GetVideoSourceInfo(VDXVideoSourceInfo& info) {
	info.mFlags = 0;
	info.mWidth = mData.mHeader.mWidth;
	info.mHeight = mData.mHeader.mHeight;
	info.mDecoderModel = VDXVideoSourceInfo::kDecoderModelCustom;
}

bool VDVideoSourcePluginMOCRAP::CreateVideoDecoderModel(IVDXVideoDecoderModel **ppModel) {
	VDVideoDecoderModelPluginMOCRAP *p = new VDVideoDecoderModelPluginMOCRAP(mData);
	if (!p)
		return false;

	p->AddRef();
	*ppModel = p;
	return true;
}

bool VDVideoSourcePluginMOCRAP::CreateVideoDecoder(IVDXVideoDecoder **ppDecoder) {
	VDVideoDecoderPluginMOCRAP *p = new VDVideoDecoderPluginMOCRAP(mData);
	if (!p)
		return false;
	p->AddRef();
	*ppDecoder = p;
	return true;
}

void VDVideoSourcePluginMOCRAP::GetSampleInfo(sint64 sample_num, VDXVideoFrameInfo& frameInfo) {
	frameInfo.mBytePosition = -1;

	switch(mData.mIndex[(uint32)sample_num].mFrameType) {
		case 0:
			frameInfo.mFrameType = kVDXVFT_Independent;
			frameInfo.mTypeChar = 'I';
			break;
		case 1:
			frameInfo.mFrameType = kVDXVFT_Predicted;
			frameInfo.mTypeChar = 'P';
			break;
		case 2:
			frameInfo.mFrameType = kVDXVFT_Bidirectional;
			frameInfo.mTypeChar = 'B';
			break;
	}
}

bool VDVideoSourcePluginMOCRAP::IsKey(sint64 sample) {
	return (mData.mIndex[(uint32)sample].mFrameType == 0);
}

sint64 VDVideoSourcePluginMOCRAP::GetFrameNumberForSample(sint64 sample_num) {
	if (mData.mIndex[(uint32)sample_num].mFrameType == 2)
		return sample_num - 1;
	else {
		uint32 n = (uint32)mData.mIndex.size();

		for(uint32 i = (uint32)sample_num + 1; i<n && mData.mIndex[i].mFrameType == 2; ++i)
			++sample_num;

		return sample_num;
	}
}

sint64 VDVideoSourcePluginMOCRAP::GetSampleNumberForFrame(sint64 display_num) {
	uint32 n = (uint32)mData.mIndex.size();
	uint32 i = (uint32)display_num + 1;

	if (i < n && mData.mIndex[i].mFrameType == 2) {
		return i;
	} else {
		do {
			--i;

			if (mData.mIndex[i].mFrameType != 2)
				break;

			--display_num;
		} while(i > 0);

		return display_num;
	}
}

sint64 VDVideoSourcePluginMOCRAP::GetRealFrame(sint64 display_num) {
	return display_num;
}

sint64 VDVideoSourcePluginMOCRAP::GetSampleBytePosition(sint64 sample_num) {
	return -1;
}

///////////////////////////////////////////////////////////////////////////////

class VDAudioSourcePluginMOCRAP : public vdxunknown<IVDXStreamSource>, public IVDXAudioSource {
public:
	VDAudioSourcePluginMOCRAP(MoCrapData& data);
	~VDAudioSourcePluginMOCRAP();

	int VDXAPIENTRY AddRef();
	int VDXAPIENTRY Release();
	void *VDXAPIENTRY AsInterface(uint32 iid);

	void		VDXAPIENTRY GetStreamSourceInfo(VDXStreamSourceInfo&);
	bool		VDXAPIENTRY Read(sint64 lStart, uint32 lCount, void *lpBuffer, uint32 cbBuffer, uint32 *lBytesRead, uint32 *lSamplesRead);

	const void *VDXAPIENTRY GetDirectFormat();
	int			VDXAPIENTRY GetDirectFormatLen();

	ErrorMode VDXAPIENTRY GetDecodeErrorMode();
	void VDXAPIENTRY SetDecodeErrorMode(ErrorMode mode);
	bool VDXAPIENTRY IsDecodeErrorModeSupported(ErrorMode mode);

	bool VDXAPIENTRY IsVBR();
	sint64 VDXAPIENTRY TimeToPositionVBR(sint64 us);
	sint64 VDXAPIENTRY PositionToTimeVBR(sint64 samples);

	void VDXAPIENTRY GetAudioSourceInfo(VDXAudioSourceInfo& info);

protected:
	VDXWAVEFORMATEX mRawFormat;

	MoCrapData&	mData;

	uint64		mLength;
};

VDAudioSourcePluginMOCRAP::VDAudioSourcePluginMOCRAP(MoCrapData& data)
	: mData(data)
{
	mRawFormat.mFormatTag		= VDXWAVEFORMATEX::kFormatPCM;
	mRawFormat.mChannels		= 1;
	mRawFormat.mSamplesPerSec	= 44100;
	mRawFormat.mAvgBytesPerSec	= 44100*2;
	mRawFormat.mBlockAlign		= 2;
	mRawFormat.mBitsPerSample	= 16;
	mRawFormat.mExtraSize		= 0;

	mLength = (uint64)(0.5 + data.mHeader.mFrames * 44100.0 * (double)data.mHeader.mFrameRateDenominator / (double)data.mHeader.mFrameRateNumerator);
}

VDAudioSourcePluginMOCRAP::~VDAudioSourcePluginMOCRAP() {
}

int VDAudioSourcePluginMOCRAP::AddRef() {
	return vdxunknown<IVDXStreamSource>::AddRef();
}

int VDAudioSourcePluginMOCRAP::Release() {
	return vdxunknown<IVDXStreamSource>::Release();
}

void *VDXAPIENTRY VDAudioSourcePluginMOCRAP::AsInterface(uint32 iid) {
	if (iid == IVDXAudioSource::kIID)
		return static_cast<IVDXAudioSource *>(this);

	return vdxunknown<IVDXStreamSource>::AsInterface(iid);
}

void VDXAPIENTRY VDAudioSourcePluginMOCRAP::GetStreamSourceInfo(VDXStreamSourceInfo& srcInfo) {
	srcInfo.mSampleRate.mNumerator = 44100;
	srcInfo.mSampleRate.mDenominator = 1;
	srcInfo.mSampleCount = mLength;
}

bool VDAudioSourcePluginMOCRAP::Read(sint64 lStart64, uint32 lCount, void *lpBuffer, uint32 cbBuffer, uint32 *lBytesRead, uint32 *lSamplesRead) {
	uint32 start = (uint32)lStart64;

	uint32 count = cbBuffer >> 1;
	if (count > lCount)
		count = lCount;

	if (!count) {
		*lBytesRead = 2;
		*lSamplesRead = 1;
		return false;
	}

	*lBytesRead = count * 2;
	*lSamplesRead = count;

	if (!lpBuffer)
		return true;

	sint16 *dst = (sint16 *)lpBuffer;

	// Generate linear frequency sweep that hits 440Hz after two seconds
	static const double hzPerSecond = 440.0 / 2.0;
	static const double radiansPerSecond = hzPerSecond * 6.28318;

	double factor = 0.5 / 44100.0 * radiansPerSecond * (1.0 / 44100.0);
	while(count--) {
		double t = (double)lStart64++;
		*dst++ = (sint16)(15000.0 * sin(t*t * factor));
	}

	return true;
}

const void *VDAudioSourcePluginMOCRAP::GetDirectFormat() {
	return &mRawFormat;
}

int VDAudioSourcePluginMOCRAP::GetDirectFormatLen() {
	return sizeof(mRawFormat);
}

IVDXStreamSource::ErrorMode VDAudioSourcePluginMOCRAP::GetDecodeErrorMode() {
	return IVDXStreamSource::kErrorModeReportAll;
}

void VDAudioSourcePluginMOCRAP::SetDecodeErrorMode(IVDXStreamSource::ErrorMode mode) {
}

bool VDAudioSourcePluginMOCRAP::IsDecodeErrorModeSupported(IVDXStreamSource::ErrorMode mode) {
	return mode == IVDXStreamSource::kErrorModeReportAll;
}

bool VDAudioSourcePluginMOCRAP::IsVBR() {
	return false;
}

sint64 VDAudioSourcePluginMOCRAP::TimeToPositionVBR(sint64 us) {
	return (sint64)(0.5 + us / 1000000.0 * (double)mData.mHeader.mFrameRateNumerator / (double)mData.mHeader.mFrameRateDenominator);
}

sint64 VDAudioSourcePluginMOCRAP::PositionToTimeVBR(sint64 samples) {
	return (sint64)(0.5 + samples * 1000000.0 * (double)mData.mHeader.mFrameRateDenominator / (double)mData.mHeader.mFrameRateNumerator);
}

void VDAudioSourcePluginMOCRAP::GetAudioSourceInfo(VDXAudioSourceInfo& info) {
	info.mFlags = 0;
}

///////////////////////////////////////////////////////////////////////////////

class VDInputFileOptionsPluginMOCRAP : public vdxunknown<IVDXInputOptions> {
public:
	bool Read(const void *src, uint32 len);
	uint32 VDXAPIENTRY Write(void *buf, uint32 buflen);

	std::string mArguments;

protected:
	struct Header {
		uint32 signature;
		uint32 size;
		uint16 version;
		uint16 arglen;
	};

	enum { kSignature = VDXMAKEFOURCC('m', 'c', 'r', 'p') };
};

bool VDInputFileOptionsPluginMOCRAP::Read(const void *src, uint32 len) {
	if (len < sizeof(Header))
		return false;

	Header hdr = {0};
	memcpy(&hdr, src, sizeof(Header));

	if (hdr.signature != kSignature || hdr.version != 1)
		return false;

	if (hdr.size > len || sizeof(Header) + hdr.arglen > len)
		return false;

	const char *args = (const char *)src + sizeof(Header);
	mArguments.assign(args, args + hdr.arglen);
	return true;
}

uint32 VDXAPIENTRY VDInputFileOptionsPluginMOCRAP::Write(void *buf, uint32 buflen) {
	uint32 required = sizeof(Header) + (uint32)mArguments.size() + 1;
	if (buf) {
		const Header hdr = { kSignature, required, 1, (uint32)mArguments.size() };

		memcpy(buf, &hdr, sizeof(Header));
		memcpy((char *)buf + sizeof(Header), mArguments.data(), mArguments.size());
	}

	return required;
}

///////////////////////////////////////////////////////////////////////////////

class VDInputFilePluginMOCRAP : public vdxunknown<IVDXInputFile> {
public:
	VDInputFilePluginMOCRAP(const VDXInputDriverContext& context);

	void VDXAPIENTRY Init(const wchar_t *szFile, IVDXInputOptions *opts);
	bool VDXAPIENTRY Append(const wchar_t *szFile);

	bool VDXAPIENTRY PromptForOptions(VDXHWND, IVDXInputOptions **);
	bool VDXAPIENTRY CreateOptions(const void *buf, uint32 len, IVDXInputOptions **);
	void VDXAPIENTRY DisplayInfo(VDXHWND hwndParent);

	bool VDXAPIENTRY GetVideoSource(int index, IVDXVideoSource **);
	bool VDXAPIENTRY GetAudioSource(int index, IVDXAudioSource **);

protected:
	MoCrapData	mData;
	const VDXInputDriverContext& mContext;
};

VDInputFilePluginMOCRAP::VDInputFilePluginMOCRAP(const VDXInputDriverContext& context)
	: mContext(context)
{
}

void VDInputFilePluginMOCRAP::Init(const wchar_t *szFile, IVDXInputOptions *opts) {
	char abuf[1024];

	wcstombs(abuf, szFile, 1024);
	abuf[1023] = 0;

	mData.mFile = fopen(abuf, "rb");
	if (!mData.mFile) {
		mContext.mpCallbacks->SetError("Unable to open file: %ls", szFile);
		return;
	}

	fread(&mData.mHeader, sizeof mData.mHeader, 1, mData.mFile);

	if (mData.mHeader.mFrames) {
		mData.mIndex.resize(mData.mHeader.mFrames);
		fseek(mData.mFile, (long)mData.mHeader.mIndexOffset, SEEK_SET);
		fread(&mData.mIndex[0], mData.mHeader.mFrames * sizeof(MoCrapIndexEntry), 1, mData.mFile);
	}
}

bool VDInputFilePluginMOCRAP::Append(const wchar_t *szFile) {
	return false;
}

class VDInputFileOptionsDialogPluginMOCRAP : public VDXVideoFilterDialog {
public:
	bool Show(VDXHWND parent, std::string& args);

	virtual INT_PTR DlgProc(UINT msg, WPARAM wParam, LPARAM lParam);

protected:
	std::string mArguments;
};

bool VDInputFileOptionsDialogPluginMOCRAP::Show(VDXHWND parent, std::string& args) {
	bool ret = 0 != VDXVideoFilterDialog::Show(NULL, MAKEINTRESOURCE(IDD_MOCRAP_OPTIONS), (HWND)parent);
	if (ret)
		args = mArguments;

	return ret;
}

INT_PTR VDInputFileOptionsDialogPluginMOCRAP::DlgProc(UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_COMMAND) {
		switch(LOWORD(wParam)) {
			case IDOK:
				{
					HWND hwnd = GetDlgItem(mhdlg, IDC_ARGS);

					int len = GetWindowTextLength(hwnd);

					std::vector<char> buf(len + 1);
					char *s = &buf[0];

					len = GetWindowText(hwnd, s, len+1);
					mArguments.assign(s, s+len);
				}
				EndDialog(mhdlg, TRUE);
				return TRUE;

			case IDCANCEL:
				EndDialog(mhdlg, FALSE);
				return TRUE;
		}
	}

	return FALSE;
}

bool VDInputFilePluginMOCRAP::PromptForOptions(VDXHWND parent, IVDXInputOptions **ppOptions) {
	VDInputFileOptionsPluginMOCRAP *opts = new(std::nothrow) VDInputFileOptionsPluginMOCRAP;
	if (!opts)
		return false;

	opts->AddRef();

	VDInputFileOptionsDialogPluginMOCRAP dlg;
	if (dlg.Show(parent, opts->mArguments)) {
		*ppOptions = opts;
		return true;
	} else {
		delete opts;
		*ppOptions = NULL;
		return false;
	}
}

bool VDInputFilePluginMOCRAP::CreateOptions(const void *buf, uint32 len, IVDXInputOptions **ppOptions) {
	VDInputFileOptionsPluginMOCRAP *opts = new(std::nothrow) VDInputFileOptionsPluginMOCRAP;
	if (!opts)
		return false;

	if (!opts->Read(buf, len)) {
		mContext.mpCallbacks->SetError("Invalid options structure.");
		delete opts;
		return false;
	}

	*ppOptions = opts;
	opts->AddRef();
	return false;
}

void VDInputFilePluginMOCRAP::DisplayInfo(VDXHWND hwndParent) {
}

bool VDInputFilePluginMOCRAP::GetVideoSource(int index, IVDXVideoSource **ppVS) {
	*ppVS = NULL;

	if (index)
		return false;

	IVDXVideoSource *pVS = new VDVideoSourcePluginMOCRAP(mData);
	if (!pVS)
		return false;

	*ppVS = pVS;
	pVS->AddRef();
	return true;
}

bool VDInputFilePluginMOCRAP::GetAudioSource(int index, IVDXAudioSource **ppAS) {
	*ppAS = NULL;

	if (index)
		return false;

	IVDXAudioSource *pAS = new VDAudioSourcePluginMOCRAP(mData);
	if (!pAS)
		return false;

	*ppAS = pAS;
	pAS->AddRef();
	return true;
}

///////////////////////////////////////////////////////////////////////////////

class VDInputFileDriverPluginMOCRAP : public vdxunknown<IVDXInputFileDriver> {
public:
	VDInputFileDriverPluginMOCRAP(const VDXInputDriverContext& context);
	~VDInputFileDriverPluginMOCRAP();

	int		VDXAPIENTRY DetectBySignature(const void *pHeader, sint32 nHeaderSize, const void *pFooter, sint32 nFooterSize, sint64 nFileSize);
	bool	VDXAPIENTRY CreateInputFile(uint32 flags, IVDXInputFile **ppFile);

protected:
	const VDXInputDriverContext& mContext;
};

VDInputFileDriverPluginMOCRAP::VDInputFileDriverPluginMOCRAP(const VDXInputDriverContext& context)
	: mContext(context)
{
}

VDInputFileDriverPluginMOCRAP::~VDInputFileDriverPluginMOCRAP() {
}

int VDXAPIENTRY VDInputFileDriverPluginMOCRAP::DetectBySignature(const void *pHeader, sint32 nHeaderSize, const void *pFooter, sint32 nFooterSize, sint64 nFileSize) {
	return -1;
}

bool VDXAPIENTRY VDInputFileDriverPluginMOCRAP::CreateInputFile(uint32 flags, IVDXInputFile **ppFile) {
	IVDXInputFile *p = new VDInputFilePluginMOCRAP(mContext);
	if (!p)
		return false;

	*ppFile = p;
	p->AddRef();
	return true;
}

///////////////////////////////////////////////////////////////////////////////

bool VDXAPIENTRY mocrap_create(const VDXInputDriverContext *pContext, IVDXInputFileDriver **ppDriver) {
	IVDXInputFileDriver *p = new VDInputFileDriverPluginMOCRAP(*pContext);
	if (!p)
		return false;
	*ppDriver = p;
	p->AddRef();
	return true;
}

const uint8 mocrap_sig[]={
	'M', 255,
	'C', 255,
	'R', 255,
	'P', 255,
};

const VDXInputDriverDefinition mocrap_input={
	sizeof(VDXInputDriverDefinition),
	VDXInputDriverDefinition::kFlagSupportsVideo,
	0,
	sizeof mocrap_sig,
	mocrap_sig,
	L"*.mocrap",
	L"MoCrap files (*.mocrap)|*.mocrap",
	L"mocrap",
	mocrap_create
};

extern const VDXPluginInfo mocrap_plugin={
	sizeof(VDXPluginInfo),
	L"MoCrap",
	L"Avery Lee",
	L"Loads MoCrap files.",
	0x01000000,
	kVDXPluginType_Input,
	0,
	kVDXPlugin_APIVersion,
	kVDXPlugin_APIVersion,
	kVDXPlugin_InputDriverAPIVersion,
	kVDXPlugin_InputDriverAPIVersion,
	&mocrap_input
};
