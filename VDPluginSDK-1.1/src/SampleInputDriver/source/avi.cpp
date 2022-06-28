#include <vd2/plugin/vdplugin.h>
#include <vd2/plugin/vdinputdriver.h>
#include <vd2/VDXFrame/Unknown.h>
#include <windows.h>
#include <vfw.h>

///////////////////////////////////////////////////////////////////////////////

class VDStreamSourcePluginAVI : public vdxunknown<IVDXStreamSource> {
public:
	VDStreamSourcePluginAVI(const VDXInputDriverContext& context);
	~VDStreamSourcePluginAVI();

	bool Init(PAVIFILE avifile, FOURCC streamType, int index);

	void		VDXAPIENTRY GetStreamSourceInfo(VDXStreamSourceInfo&);
	bool		VDXAPIENTRY Read(sint64 lStart, uint32 lCount, void *lpBuffer, uint32 cbBuffer, uint32 *lBytesRead, uint32 *lSamplesRead);

	const void *VDXAPIENTRY GetDirectFormat();
	int			VDXAPIENTRY GetDirectFormatLen();

	ErrorMode	VDXAPIENTRY GetDecodeErrorMode();
	void		VDXAPIENTRY SetDecodeErrorMode(ErrorMode mode);
	bool		VDXAPIENTRY IsDecodeErrorModeSupported(ErrorMode mode);

	bool		VDXAPIENTRY IsVBR();
	sint64		VDXAPIENTRY TimeToPositionVBR(sint64 us);
	sint64		VDXAPIENTRY PositionToTimeVBR(sint64 samples);

protected:
	PAVISTREAM mAVIStream;
	AVISTREAMINFO	mStreamInfo;

	sint64		mStart;
	sint64		mLength;

	void		*mpFormat;
	int			mFormatLen;

	const VDXInputDriverContext& mContext;
};

VDStreamSourcePluginAVI::VDStreamSourcePluginAVI(const VDXInputDriverContext& context)
	: mAVIStream(NULL)
	, mpFormat(NULL)
	, mFormatLen(0)
	, mContext(context)
{
}

VDStreamSourcePluginAVI::~VDStreamSourcePluginAVI() {
	if (mpFormat)
		free(mpFormat);

	if (mAVIStream)
		AVIStreamRelease(mAVIStream);
}

bool VDStreamSourcePluginAVI::Init(PAVIFILE avifile, FOURCC streamType, int index) {
	HRESULT hr = AVIFileGetStream(avifile, &mAVIStream, streamType, index);
	if (FAILED(hr))
		return false;

	memset(&mStreamInfo, 0, sizeof mStreamInfo);
	hr = AVIStreamInfo(mAVIStream, &mStreamInfo, sizeof mStreamInfo);
	if (FAILED(hr))
		return false;

	LONG formatSize;
	hr = AVIStreamFormatSize(mAVIStream, 0, &formatSize);
	if (FAILED(hr))
		return false;

	mpFormat = malloc(formatSize);
	mFormatLen = formatSize;
	if (!mpFormat)
		return false;

	LONG len = mFormatLen;
	hr = AVIStreamReadFormat(mAVIStream, 0, mpFormat, &len);
	if (FAILED(hr))
		return false;

	mStart = AVIStreamStart(mAVIStream);
	mLength = AVIStreamLength(mAVIStream);

	return true;
}

void VDXAPIENTRY VDStreamSourcePluginAVI::GetStreamSourceInfo(VDXStreamSourceInfo& srcInfo) {
	srcInfo.mSampleRate.mNumerator		= mStreamInfo.dwRate;
	srcInfo.mSampleRate.mDenominator	= mStreamInfo.dwScale;
	srcInfo.mSampleCount				= mLength;
}

bool VDStreamSourcePluginAVI::Read(sint64 lStart64, uint32 lCount, void *lpBuffer, uint32 cbBuffer, uint32 *lBytesRead, uint32 *lSamplesRead) {
	LONG actualBytes;
	LONG actualSamples;

	HRESULT hr = AVIStreamRead(mAVIStream, (LONG)(lStart64 + mStart), (LONG)lCount, lpBuffer, cbBuffer, &actualBytes, &actualSamples);

	*lBytesRead = actualBytes;
	*lSamplesRead = actualSamples;

	if (hr == AVIERR_BUFFERTOOSMALL)
		return false;

	if (FAILED(hr))
		mContext.mpCallbacks->SetError("Unable to read from AVI file: hr=%08x", hr);

	return true;
}

const void *VDStreamSourcePluginAVI::GetDirectFormat() {
	return mpFormat;
}

int VDStreamSourcePluginAVI::GetDirectFormatLen() {
	return mFormatLen;
}

IVDXStreamSource::ErrorMode VDStreamSourcePluginAVI::GetDecodeErrorMode() {
	return IVDXStreamSource::kErrorModeReportAll;
}

void VDStreamSourcePluginAVI::SetDecodeErrorMode(IVDXStreamSource::ErrorMode mode) {
}

bool VDStreamSourcePluginAVI::IsDecodeErrorModeSupported(IVDXStreamSource::ErrorMode mode) {
	return mode == IVDXStreamSource::kErrorModeReportAll;
}

bool VDStreamSourcePluginAVI::IsVBR() {
	return false;
}

sint64 VDStreamSourcePluginAVI::TimeToPositionVBR(sint64 us) {
	return (sint64)(0.5 + us / 1000000.0 * (double)mStreamInfo.dwRate / (double)mStreamInfo.dwScale);
}

sint64 VDStreamSourcePluginAVI::PositionToTimeVBR(sint64 samples) {
	return (sint64)(0.5 + samples * 1000000.0 * (double)mStreamInfo.dwScale / (double)mStreamInfo.dwRate);
}

///////////////////////////////////////////////////////////////////////////////

class VDVideoSourcePluginAVI : public VDStreamSourcePluginAVI, public IVDXVideoSource {
public:
	VDVideoSourcePluginAVI(const VDXInputDriverContext& context);
	~VDVideoSourcePluginAVI();

	bool		Init(PAVIFILE avifile, int index);

	int			VDXAPIENTRY AddRef();
	int			VDXAPIENTRY Release();
	void *		VDXAPIENTRY AsInterface(uint32 iid);

	void		VDXAPIENTRY GetVideoSourceInfo(VDXVideoSourceInfo& info);

	bool		VDXAPIENTRY CreateVideoDecoderModel(IVDXVideoDecoderModel **ppModel);
	bool		VDXAPIENTRY CreateVideoDecoder(IVDXVideoDecoder **ppDecoder);

	void		VDXAPIENTRY GetSampleInfo(sint64 sample_num, VDXVideoFrameInfo& frameInfo);

	bool		VDXAPIENTRY IsKey(sint64 lSample);

	sint64		VDXAPIENTRY GetFrameNumberForSample(sint64 sample_num);
	sint64		VDXAPIENTRY GetSampleNumberForFrame(sint64 display_num);
	sint64		VDXAPIENTRY GetRealFrame(sint64 display_num);

	sint64		VDXAPIENTRY GetSampleBytePosition(sint64 sample_num);
};

VDVideoSourcePluginAVI::VDVideoSourcePluginAVI(const VDXInputDriverContext& context)
	: VDStreamSourcePluginAVI(context)
{
}

VDVideoSourcePluginAVI::~VDVideoSourcePluginAVI() {
}

bool VDVideoSourcePluginAVI::Init(PAVIFILE avifile, int index) {
	return VDStreamSourcePluginAVI::Init(avifile, streamtypeVIDEO, index);
}

int VDXAPIENTRY VDVideoSourcePluginAVI::AddRef() {
	return VDStreamSourcePluginAVI::AddRef();
}

int VDXAPIENTRY VDVideoSourcePluginAVI::Release() {
	return VDStreamSourcePluginAVI::Release();
}

void *VDXAPIENTRY VDVideoSourcePluginAVI::AsInterface(uint32 iid) {
	if (iid == IVDXVideoSource::kIID)
		return static_cast<IVDXVideoSource *>(this);

	return VDStreamSourcePluginAVI::AsInterface(iid);
}

void VDXAPIENTRY VDVideoSourcePluginAVI::GetVideoSourceInfo(VDXVideoSourceInfo& info) {
	const VDXBITMAPINFOHEADER *hdr = (const VDXBITMAPINFOHEADER *)GetDirectFormat();

	info.mFlags			= 0;
	info.mWidth			= hdr->mWidth;
	info.mHeight		= hdr->mHeight;
	info.mDecoderModel	= VDXVideoSourceInfo::kDecoderModelDefaultIP;
}

bool VDXAPIENTRY VDVideoSourcePluginAVI::CreateVideoDecoderModel(IVDXVideoDecoderModel **ppModel) {
	return false;
}

bool VDXAPIENTRY VDVideoSourcePluginAVI::CreateVideoDecoder(IVDXVideoDecoder **ppDecoder) {
	return false;
}

void VDXAPIENTRY VDVideoSourcePluginAVI::GetSampleInfo(sint64 sample_num, VDXVideoFrameInfo& frameInfo) {
	frameInfo.mBytePosition = -1;

	if (AVIStreamIsKeyFrame(mAVIStream, (LONG)(sample_num + mStart))) {
		frameInfo.mFrameType = kVDXVFT_Independent;
		frameInfo.mTypeChar = 'K';
	} else {
		frameInfo.mFrameType = kVDXVFT_Predicted;
		frameInfo.mTypeChar = ' ';
	}
}

bool VDXAPIENTRY VDVideoSourcePluginAVI::IsKey(sint64 sample) {
	return AVIStreamIsKeyFrame(mAVIStream, (LONG)(sample + mStart));
}

sint64 VDXAPIENTRY VDVideoSourcePluginAVI::GetFrameNumberForSample(sint64 sample_num) {
	return sample_num;
}

sint64 VDXAPIENTRY VDVideoSourcePluginAVI::GetSampleNumberForFrame(sint64 frame_num) {
	return frame_num;
}

sint64 VDXAPIENTRY VDVideoSourcePluginAVI::GetRealFrame(sint64 frame_num) {
	return frame_num;
}

sint64 VDXAPIENTRY VDVideoSourcePluginAVI::GetSampleBytePosition(sint64 sample_num) {
	return -1;
}

///////////////////////////////////////////////////////////////////////////////

class VDAudioSourcePluginAVI : public VDStreamSourcePluginAVI, public IVDXAudioSource {
public:
	VDAudioSourcePluginAVI(const VDXInputDriverContext& context);
	~VDAudioSourcePluginAVI();

	bool Init(PAVIFILE avifile, int index);

	int VDXAPIENTRY AddRef();
	int VDXAPIENTRY Release();
	void *VDXAPIENTRY AsInterface(uint32 iid);

	void VDXAPIENTRY GetAudioSourceInfo(VDXAudioSourceInfo& info);
};

VDAudioSourcePluginAVI::VDAudioSourcePluginAVI(const VDXInputDriverContext& context)
	: VDStreamSourcePluginAVI(context)
{
}

VDAudioSourcePluginAVI::~VDAudioSourcePluginAVI() {
}

bool VDAudioSourcePluginAVI::Init(PAVIFILE avifile, int index) {
	return VDStreamSourcePluginAVI::Init(avifile, streamtypeAUDIO, index);
}

int VDXAPIENTRY VDAudioSourcePluginAVI::AddRef() {
	return VDStreamSourcePluginAVI::AddRef();
}

int VDXAPIENTRY VDAudioSourcePluginAVI::Release() {
	return VDStreamSourcePluginAVI::Release();
}

void *VDXAPIENTRY VDAudioSourcePluginAVI::AsInterface(uint32 iid) {
	if (iid == IVDXAudioSource::kIID)
		return static_cast<IVDXAudioSource *>(this);

	return VDStreamSourcePluginAVI::AsInterface(iid);
}

void VDXAPIENTRY VDAudioSourcePluginAVI::GetAudioSourceInfo(VDXAudioSourceInfo& info) {
	info.mFlags = 0;
}

///////////////////////////////////////////////////////////////////////////////

class VDInputFilePluginAVI : public vdxunknown<IVDXInputFile> {
public:
	VDInputFilePluginAVI(const VDXInputDriverContext& context);

	void VDXAPIENTRY Init(const wchar_t *szFile, IVDXInputOptions *opts);
	bool VDXAPIENTRY Append(const wchar_t *szFile);

	bool VDXAPIENTRY PromptForOptions(VDXHWND, IVDXInputOptions **);
	bool VDXAPIENTRY CreateOptions(const void *buf, uint32 len, IVDXInputOptions **);
	void VDXAPIENTRY DisplayInfo(VDXHWND hwndParent);

	bool VDXAPIENTRY GetVideoSource(int index, IVDXVideoSource **);
	bool VDXAPIENTRY GetAudioSource(int index, IVDXAudioSource **);

protected:
	PAVIFILE	mAVIFile;
	const VDXInputDriverContext& mContext;
};

VDInputFilePluginAVI::VDInputFilePluginAVI(const VDXInputDriverContext& context)
	: mContext(context)
{
}

void VDInputFilePluginAVI::Init(const wchar_t *szFile, IVDXInputOptions *opts) {
	HRESULT hr = AVIFileOpenW(&mAVIFile, szFile, OF_READ | OF_SHARE_DENY_WRITE, NULL);
	if (FAILED(hr))
		mContext.mpCallbacks->SetError("Unable to open AVI file: hr=%08x", hr);
}

bool VDInputFilePluginAVI::Append(const wchar_t *szFile) {
	return false;
}

bool VDInputFilePluginAVI::PromptForOptions(VDXHWND, IVDXInputOptions **ppOptions) {
	*ppOptions = NULL;
	return false;
}

bool VDInputFilePluginAVI::CreateOptions(const void *buf, uint32 len, IVDXInputOptions **ppOptions) {
	*ppOptions = NULL;
	return false;
}

void VDInputFilePluginAVI::DisplayInfo(VDXHWND hwndParent) {
}

bool VDInputFilePluginAVI::GetVideoSource(int index, IVDXVideoSource **ppVS) {
	*ppVS = NULL;

	if (index)
		return false;

	VDVideoSourcePluginAVI *pVS = new VDVideoSourcePluginAVI(mContext);
	if (!pVS)
		return false;

	pVS->AddRef();
	if (!pVS->Init(mAVIFile, index)) {
		pVS->Release();
		return false;
	}

	*ppVS = pVS;
	return true;
}

bool VDInputFilePluginAVI::GetAudioSource(int index, IVDXAudioSource **ppAS) {
	*ppAS = NULL;

	if (index)
		return false;

	VDAudioSourcePluginAVI *pAS = new VDAudioSourcePluginAVI(mContext);
	if (!pAS)
		return false;

	pAS->AddRef();
	if (!pAS->Init(mAVIFile, index)) {
		pAS->Release();
		return false;
	}

	*ppAS = pAS;
	return true;
}

///////////////////////////////////////////////////////////////////////////////

class VDInputFileDriverPluginAVI : public vdxunknown<IVDXInputFileDriver> {
public:
	VDInputFileDriverPluginAVI(const VDXInputDriverContext& context);
	~VDInputFileDriverPluginAVI();

	int		VDXAPIENTRY DetectBySignature(const void *pHeader, sint32 nHeaderSize, const void *pFooter, sint32 nFooterSize, sint64 nFileSize);
	bool	VDXAPIENTRY CreateInputFile(uint32 flags, IVDXInputFile **ppFile);

protected:
	const VDXInputDriverContext& mContext;
};

VDInputFileDriverPluginAVI::VDInputFileDriverPluginAVI(const VDXInputDriverContext& context)
	: mContext(context)
{
}

VDInputFileDriverPluginAVI::~VDInputFileDriverPluginAVI() {
}

int VDXAPIENTRY VDInputFileDriverPluginAVI::DetectBySignature(const void *pHeader, sint32 nHeaderSize, const void *pFooter, sint32 nFooterSize, sint64 nFileSize) {
	return -1;
}

bool VDXAPIENTRY VDInputFileDriverPluginAVI::CreateInputFile(uint32 flags, IVDXInputFile **ppFile) {
	IVDXInputFile *p = new VDInputFilePluginAVI(mContext);
	if (!p)
		return false;

	*ppFile = p;
	p->AddRef();
	return true;
}

///////////////////////////////////////////////////////////////////////////////

bool VDXAPIENTRY avi_create(const VDXInputDriverContext *pContext, IVDXInputFileDriver **ppDriver) {
	IVDXInputFileDriver *p = new VDInputFileDriverPluginAVI(*pContext);
	if (!p)
		return false;
	*ppDriver = p;
	p->AddRef();
	return true;
}

const VDXInputDriverDefinition avi_input={
	sizeof(VDXInputDriverDefinition),
	VDXInputDriverDefinition::kFlagSupportsVideo | VDXInputDriverDefinition::kFlagSupportsAudio,
	-1,
	0,
	NULL,
	NULL,
	L"AVI files (*.avi)|*.avi",
	L"SampleAVI",
	avi_create
};

extern const VDXPluginInfo avi_plugin={
	sizeof(VDXPluginInfo),
	L"SampleAVI",
	L"Avery Lee",
	L"Loads AVI files through Video for Windows (VFW).",
	0x01000000,
	kVDXPluginType_Input,
	0,
	kVDXPlugin_APIVersion,
	kVDXPlugin_APIVersion,
	kVDXPlugin_InputDriverAPIVersion,
	kVDXPlugin_InputDriverAPIVersion,
	&avi_input
};
