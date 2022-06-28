#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <vd2/VDXFrame/VideoFilter.h>
#include <vd2/VDXFrame/VideoFilterDialog.h>
#include "../resource.h"

extern int g_VFVAPIVersion;

///////////////////////////////////////////////////////////////////////////////

class YUVTransformFilterConfig {
public:
	YUVTransformFilterConfig()
		: mYScale(1.0f)
		, mUScale(1.0f)
		, mVScale(1.0f)
	{
	}

public:
	float mYScale;
	float mUScale;
	float mVScale;
};

///////////////////////////////////////////////////////////////////////////////

class YUVTransformFilterDialog : public VDXVideoFilterDialog {
public:
	YUVTransformFilterDialog(YUVTransformFilterConfig& config, IVDXFilterPreview *ifp) : mConfig(config), mifp(ifp) {}

	bool Show(HWND parent) {
		return 0 != VDXVideoFilterDialog::Show(NULL, MAKEINTRESOURCE(IDD_FILTER_YUVTRANSFORM), parent);
	}

	virtual INT_PTR DlgProc(UINT msg, WPARAM wParam, LPARAM lParam);

protected:
	bool OnInit();
	bool OnCommand(int cmd);
	void OnDestroy();

	void LoadFromConfig();
	bool SaveToConfig();

	YUVTransformFilterConfig& mConfig;
	YUVTransformFilterConfig mOldConfig;
	IVDXFilterPreview *const mifp;
};

INT_PTR YUVTransformFilterDialog::DlgProc(UINT msg, WPARAM wParam, LPARAM lParam) {
	switch(msg) {
		case WM_INITDIALOG:
			return !OnInit();

		case WM_DESTROY:
			OnDestroy();
			break;

		case WM_COMMAND:
			if (OnCommand(LOWORD(wParam)))
				return TRUE;
			break;

		case WM_HSCROLL:
			if (mifp && SaveToConfig())
				mifp->RedoFrame();
			return TRUE;
	}

	return FALSE;
}

bool YUVTransformFilterDialog::OnInit() {
	mOldConfig = mConfig;

	SendDlgItemMessage(mhdlg, IDC_YSCALE, TBM_SETRANGE, TRUE, MAKELONG(0, 2000));
	SendDlgItemMessage(mhdlg, IDC_USCALE, TBM_SETRANGE, TRUE, MAKELONG(0, 2000));
	SendDlgItemMessage(mhdlg, IDC_VSCALE, TBM_SETRANGE, TRUE, MAKELONG(0, 2000));

	LoadFromConfig();

	HWND hwndFirst = GetDlgItem(mhdlg, IDC_YSCALE);
	if (hwndFirst)
		SendMessage(mhdlg, WM_NEXTDLGCTL, (WPARAM)hwndFirst, TRUE);

	HWND hwndPreview = GetDlgItem(mhdlg, IDC_PREVIEW);
	if (hwndPreview && mifp) {
		EnableWindow(hwndPreview, TRUE);
		mifp->InitButton((VDXHWND)hwndPreview);
	}

	return false;
}

void YUVTransformFilterDialog::OnDestroy() {
	if (mifp)
		mifp->InitButton(NULL);
}

bool YUVTransformFilterDialog::OnCommand(int cmd) {
	switch(cmd) {
		case IDOK:
			SaveToConfig();
			EndDialog(mhdlg, true);
			return true;

		case IDCANCEL:
			mConfig = mOldConfig;
			EndDialog(mhdlg, false);
			return true;

		case IDC_PREVIEW:
			if (mifp)
				mifp->Toggle((VDXHWND)mhdlg);
			return true;
	}

	return false;
}

void YUVTransformFilterDialog::LoadFromConfig() {
	SendDlgItemMessage(mhdlg, IDC_YSCALE, TBM_SETPOS, TRUE, (int)(0.5f + mConfig.mYScale * 1000.0f));
	SendDlgItemMessage(mhdlg, IDC_USCALE, TBM_SETPOS, TRUE, (int)(0.5f + mConfig.mUScale * 1000.0f));
	SendDlgItemMessage(mhdlg, IDC_VSCALE, TBM_SETPOS, TRUE, (int)(0.5f + mConfig.mVScale * 1000.0f));
}

bool YUVTransformFilterDialog::SaveToConfig() {
	float yscale = (float)SendDlgItemMessage(mhdlg, IDC_YSCALE, TBM_GETPOS, 0, 0) / 1000.0f;
	float uscale = (float)SendDlgItemMessage(mhdlg, IDC_USCALE, TBM_GETPOS, 0, 0) / 1000.0f;
	float vscale = (float)SendDlgItemMessage(mhdlg, IDC_VSCALE, TBM_GETPOS, 0, 0) / 1000.0f;

	if (yscale != mConfig.mYScale
		|| uscale != mConfig.mUScale
		|| vscale != mConfig.mVScale)
	{
		mConfig.mYScale	= yscale;
		mConfig.mUScale	= uscale;
		mConfig.mVScale	= vscale;
		return true;
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////

class YUVTransformFilter : public VDXVideoFilter {
public:
	virtual uint32 GetParams();
	virtual void Start();
	virtual void Run();
	virtual bool Configure(VDXHWND hwnd);
	virtual void GetSettingString(char *buf, int maxlen);
	virtual void GetScriptString(char *buf, int maxlen);

	VDXVF_DECLARE_SCRIPT_METHODS();

protected:
	void TransformRGB32(void *dst, ptrdiff_t dstpitch, uint32 w, uint32 h);
	void TransformY8(void *dst, ptrdiff_t dstpitch, uint32 w, uint32 h, float scale);

	void ScriptConfig(IVDXScriptInterpreter *isi, const VDXScriptValue *argv, int argc);

	YUVTransformFilterConfig mConfig;
};

VDXVF_BEGIN_SCRIPT_METHODS(YUVTransformFilter)
VDXVF_DEFINE_SCRIPT_METHOD(YUVTransformFilter, ScriptConfig, "iii")
VDXVF_END_SCRIPT_METHODS()

uint32 YUVTransformFilter::GetParams() {
	if (g_VFVAPIVersion >= 12) {
		switch(fa->src.mpPixmapLayout->format) {
			case nsVDXPixmap::kPixFormat_XRGB8888:
			case nsVDXPixmap::kPixFormat_YUV444_Planar:
			case nsVDXPixmap::kPixFormat_YUV422_Planar:
			case nsVDXPixmap::kPixFormat_YUV420_Planar:
			case nsVDXPixmap::kPixFormat_YUV411_Planar:
			case nsVDXPixmap::kPixFormat_YUV410_Planar:
				break;

			default:
				return FILTERPARAM_NOT_SUPPORTED;
		}
	}

	fa->dst.offset = fa->src.offset;
	return FILTERPARAM_SUPPORTS_ALTFORMATS;
}

void YUVTransformFilter::Start() {
}

void YUVTransformFilter::Run() {
	if (g_VFVAPIVersion >= 12) {
		const VDXPixmap& pxdst = *fa->dst.mpPixmap;
		const VDXPixmap& pxsrc = *fa->src.mpPixmap;
		int sw;
		int sh;

		switch(pxdst.format) {
			case nsVDXPixmap::kPixFormat_XRGB8888:
				TransformRGB32(pxdst.data, pxdst.pitch, pxdst.w, pxdst.h);
				break;
			case nsVDXPixmap::kPixFormat_YUV444_Planar:
				TransformY8(pxdst.data, pxdst.pitch, pxdst.w, pxdst.h, mConfig.mYScale);
				TransformY8(pxdst.data2, pxdst.pitch2, pxdst.w, pxdst.h, mConfig.mUScale);
				TransformY8(pxdst.data3, pxdst.pitch3, pxdst.w, pxdst.h, mConfig.mVScale);
				break;
			case nsVDXPixmap::kPixFormat_YUV422_Planar:
				TransformY8(pxdst.data, pxdst.pitch, pxdst.w, pxdst.h, mConfig.mYScale);

				sw = (pxsrc.w + 1) >> 1;
				sh = pxsrc.h;
				TransformY8(pxdst.data2, pxdst.pitch2, sw, sh, mConfig.mUScale);
				TransformY8(pxdst.data3, pxdst.pitch3, sw, sh, mConfig.mVScale);
				break;
			case nsVDXPixmap::kPixFormat_YUV420_Planar:
				TransformY8(pxdst.data, pxdst.pitch, pxdst.w, pxdst.h, mConfig.mYScale);

				sw = (pxsrc.w + 1) >> 1;
				sh = (pxsrc.h + 1) >> 1;
				TransformY8(pxdst.data2, pxdst.pitch2, sw, sh, mConfig.mUScale);
				TransformY8(pxdst.data3, pxdst.pitch3, sw, sh, mConfig.mVScale);
				break;
			case nsVDXPixmap::kPixFormat_YUV411_Planar:
				TransformY8(pxdst.data, pxdst.pitch, pxdst.w, pxdst.h, mConfig.mYScale);

				sw = (pxsrc.w + 3) >> 2;
				sh = pxsrc.h;
				TransformY8(pxdst.data2, pxdst.pitch2, sw, sh, mConfig.mUScale);
				TransformY8(pxdst.data3, pxdst.pitch3, sw, sh, mConfig.mVScale);
				break;
			case nsVDXPixmap::kPixFormat_YUV410_Planar:
				TransformY8(pxdst.data, pxdst.pitch, pxdst.w, pxdst.h, mConfig.mYScale);

				sw = (pxsrc.w + 3) >> 2;
				sh = (pxsrc.h + 3) >> 2;
				TransformY8(pxdst.data2, pxdst.pitch2, sw, sh, mConfig.mUScale);
				TransformY8(pxdst.data3, pxdst.pitch3, sw, sh, mConfig.mVScale);
				break;
		}
	} else {
		TransformRGB32(fa->dst.data, fa->dst.pitch, fa->dst.w, fa->dst.h);
	}
}

bool YUVTransformFilter::Configure(VDXHWND hwnd) {
	YUVTransformFilterDialog dlg(mConfig, fa->ifp);

	return dlg.Show((HWND)hwnd);
}

void YUVTransformFilter::GetSettingString(char *buf, int maxlen) {
	SafePrintf(buf, maxlen, " (Y%.1f%%, U%.1f%%, V%.1f%%)"
			, mConfig.mYScale * 100.0f
			, mConfig.mUScale * 100.0f
			, mConfig.mVScale * 100.0f
			);
}

void YUVTransformFilter::GetScriptString(char *buf, int maxlen) {
	SafePrintf(buf, maxlen, "Config(%u, %u, %u)"
			, (unsigned)(mConfig.mYScale * 1000.0f + 0.5f)
			, (unsigned)(mConfig.mUScale * 1000.0f + 0.5f)
			, (unsigned)(mConfig.mVScale * 1000.0f + 0.5f)
			);
}

void YUVTransformFilter::TransformRGB32(void *dst, ptrdiff_t dstpitch, uint32 w, uint32 h) {
	//	R = 1.164(Y-16) + 1.596(Cr-128)
	//	G = 1.164(Y-16) - 0.813(Cr-128) - 0.391(Cb-128)
	//	B = 1.164(Y-16) + 2.018(Cb-128)
	//	Y = (0.299R + 0.587G + 0.114B) * 219/255 + 16
	//	Cr = (0.713(R-Y)) * 224/255 + 128
	//	Cb = (0.564(B-Y)) * 224/255 + 128

	uint8 *dst8 = (uint8 *)dst;
	float yscale = mConfig.mYScale;
	float uscale = mConfig.mUScale;
	float vscale = mConfig.mVScale;

	for(uint32 y2=0; y2<h; ++y2) {
		uint8 *p = dst8;
		for(uint32 x=0; x<w; ++x) {
			float r = p[2] / 255.0f;
			float g = p[1] / 255.0f;
			float b = p[0] / 255.0f;
			float y = 0.299f*r + 0.587f*g + 0.114f*b;
			float u = 0.5f*(b - y);
			float v = 0.5f*(r - y);

			y = (y - 0.5f) * yscale + 0.5f;
			u *= uscale;
			v *= vscale;

			r = y + 1.402f*v;
			g = y - 0.714f*v - 0.344f*u;
			b = y + 1.772f*u;

			sint32 ir = (uint32)(0.5f + r*255.0f);
			sint32 ig = (uint32)(0.5f + g*255.0f);
			sint32 ib = (uint32)(0.5f + b*255.0f);

			ir &= ~(ir >> 31);
			ir |= (255 - ir) >> 31;
			ig &= ~(ig >> 31);
			ig |= (255 - ig) >> 31;
			ib &= ~(ib >> 31);
			ib |= (255 - ib) >> 31;

			p[0] = (uint8)ib;
			p[1] = (uint8)ig;
			p[2] = (uint8)ir;

			p += 4;
		}

		dst8 += dstpitch;
	}
}

void YUVTransformFilter::TransformY8(void *dst, ptrdiff_t dstpitch, uint32 w, uint32 h, float scale) {
	//	R = 1.164(Y-16) + 1.596(Cr-128)
	//	G = 1.164(Y-16) - 0.813(Cr-128) - 0.391(Cb-128)
	//	B = 1.164(Y-16) + 2.018(Cb-128)
	//	Y = (0.299R + 0.587G + 0.114B) * 219/255 + 16
	//	Cr = (0.713(R-Y)) * 224/255 + 128
	//	Cb = (0.564(B-Y)) * 224/255 + 128

	uint8 *dst8 = (uint8 *)dst;

	for(uint32 y2=0; y2<h; ++y2) {
		uint8 *p = dst8;
		for(uint32 x=0; x<w; ++x) {
			float y = p[x];

			y = (y - 128.0f) * scale + 128.0f;

			sint32 iy = (uint32)(0.5f + y);

			iy &= ~(iy >> 31);
			iy |= (255 - iy) >> 31;

			p[x] = (uint8)iy;
		}

		dst8 += dstpitch;
	}
}

void YUVTransformFilter::ScriptConfig(IVDXScriptInterpreter *isi, const VDXScriptValue *argv, int argc) {
	mConfig.mYScale = argv[0].asInt() / 1000.0f;
	mConfig.mUScale = argv[1].asInt() / 1000.0f;
	mConfig.mVScale = argv[2].asInt() / 1000.0f;
}

///////////////////////////////////////////////////////////////////////////////

extern VDXFilterDefinition filterDef_yuvTransform = VDXVideoFilterDefinition<YUVTransformFilter>("Avery Lee", "Sample: YUV transform", "Sample filter from the VirtualDub Plugin SDK: Scales luminance and chrominance components.");
