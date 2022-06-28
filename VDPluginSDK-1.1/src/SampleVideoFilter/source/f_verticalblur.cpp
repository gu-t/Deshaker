#include <emmintrin.h>
#include <vd2/VDXFrame/VideoFilter.h>

extern int g_VFVAPIVersion;

class VerticalBlurFilter : public VDXVideoFilter {
public:
	virtual uint32 GetParams();
	virtual void Start();
	virtual void Run();

protected:
	void BlurPlane8(void *dst, ptrdiff_t dstpitch, const void *src, ptrdiff_t srcpitch, uint32 w, uint32 h);

#ifdef _M_IX86
	bool mbUsingMMX;
#endif

	bool mbUsingSSE2;

	void Blur(void *dst, const void *src1, const void *src2, const void *src3, uint32 count);
	static void Blur8(void *dst, const void *src1, const void *src2, const void *src3, uint32 count);
	static void Blur32(void *dst, const void *src1, const void *src2, const void *src3, uint32 count);

#ifdef _M_IX86
	static void Blur64_MMX(void *dst, const void *src1, const void *src2, const void *src3, uint32 count);
#endif

	static void Blur128_SSE2(void *dst, const void *src1, const void *src2, const void *src3, uint32 count);
};

uint32 VerticalBlurFilter::GetParams() {
	if (g_VFVAPIVersion >= 12) {
		switch(fa->src.mpPixmapLayout->format) {
			case nsVDXPixmap::kPixFormat_XRGB8888:
			case nsVDXPixmap::kPixFormat_YUV422_UYVY:
			case nsVDXPixmap::kPixFormat_YUV422_YUYV:
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

	fa->dst.offset = 0;
	return FILTERPARAM_SWAP_BUFFERS | FILTERPARAM_SUPPORTS_ALTFORMATS;
}

void VerticalBlurFilter::Start() {
#ifdef _M_IX86
	mbUsingMMX = 0 != (ff->isMMXEnabled());
#endif

	mbUsingSSE2 = 0 != (ff->getCPUFlags() & CPUF_SUPPORTS_SSE2);
}

void VerticalBlurFilter::Run() {
	if (g_VFVAPIVersion >= 12) {
		const VDXPixmap& pxdst = *fa->dst.mpPixmap;
		const VDXPixmap& pxsrc = *fa->src.mpPixmap;
		int sw;
		int sh;

		switch(pxdst.format) {
			case nsVDXPixmap::kPixFormat_XRGB8888:
				BlurPlane8(pxdst.data, pxdst.pitch, pxsrc.data, pxsrc.pitch, pxsrc.w * 4, pxsrc.h);
				break;
			case nsVDXPixmap::kPixFormat_YUV422_UYVY:
			case nsVDXPixmap::kPixFormat_YUV422_YUYV:
				BlurPlane8(pxdst.data, pxdst.pitch, pxsrc.data, pxsrc.pitch, ((pxsrc.w + 1) >> 1) * 4, pxsrc.h);
				break;
			case nsVDXPixmap::kPixFormat_YUV444_Planar:
				BlurPlane8(pxdst.data, pxdst.pitch, pxsrc.data, pxsrc.pitch, pxsrc.w, pxsrc.h);
				BlurPlane8(pxdst.data2, pxdst.pitch2, pxsrc.data2, pxsrc.pitch2, pxsrc.w, pxsrc.h);
				BlurPlane8(pxdst.data3, pxdst.pitch3, pxsrc.data3, pxsrc.pitch3, pxsrc.w, pxsrc.h);
				break;
			case nsVDXPixmap::kPixFormat_YUV422_Planar:
				BlurPlane8(pxdst.data, pxdst.pitch, pxsrc.data, pxsrc.pitch, pxsrc.w, pxsrc.h);

				sw = (pxsrc.w + 1) >> 1;
				sh = pxsrc.h;
				BlurPlane8(pxdst.data2, pxdst.pitch2, pxsrc.data2, pxsrc.pitch2, sw, sh);
				BlurPlane8(pxdst.data3, pxdst.pitch3, pxsrc.data3, pxsrc.pitch3, sw, sh);
				break;
			case nsVDXPixmap::kPixFormat_YUV420_Planar:
				BlurPlane8(pxdst.data, pxdst.pitch, pxsrc.data, pxsrc.pitch, pxsrc.w, pxsrc.h);

				sw = (pxsrc.w + 1) >> 1;
				sh = (pxsrc.h + 1) >> 1;
				BlurPlane8(pxdst.data2, pxdst.pitch2, pxsrc.data2, pxsrc.pitch2, sw, sh);
				BlurPlane8(pxdst.data3, pxdst.pitch3, pxsrc.data3, pxsrc.pitch3, sw, sh);
				break;
			case nsVDXPixmap::kPixFormat_YUV411_Planar:
				BlurPlane8(pxdst.data, pxdst.pitch, pxsrc.data, pxsrc.pitch, pxsrc.w, pxsrc.h);

				sw = (pxsrc.w + 3) >> 2;
				sh = pxsrc.h;
				BlurPlane8(pxdst.data2, pxdst.pitch2, pxsrc.data2, pxsrc.pitch2, sw, sh);
				BlurPlane8(pxdst.data3, pxdst.pitch3, pxsrc.data3, pxsrc.pitch3, sw, sh);
				break;
			case nsVDXPixmap::kPixFormat_YUV410_Planar:
				BlurPlane8(pxdst.data, pxdst.pitch, pxsrc.data, pxsrc.pitch, pxsrc.w, pxsrc.h);

				sw = (pxsrc.w + 3) >> 2;
				sh = (pxsrc.h + 3) >> 2;
				BlurPlane8(pxdst.data2, pxdst.pitch2, pxsrc.data2, pxsrc.pitch2, sw, sh);
				BlurPlane8(pxdst.data3, pxdst.pitch3, pxsrc.data3, pxsrc.pitch3, sw, sh);
				break;
		}
	} else {
		BlurPlane8(fa->dst.data, fa->dst.pitch, fa->src.data, fa->src.pitch, fa->dst.w * 4, fa->dst.h);
	}
}

void VerticalBlurFilter::BlurPlane8(void *dst0, ptrdiff_t dstpitch, const void *src0, ptrdiff_t srcpitch, uint32 w, uint32 h) {
	char *dst = (char *)dst0;
	const char *src = (const char *)src0;

	for(uint32 y=0; y<h; ++y) {
		const char *src1 = y ? src - srcpitch : src;
		const char *src2 = src;
		const char *src3 = y < h-1 ? src + srcpitch : src;

		Blur(dst, src1, src2, src3, w);

		src += srcpitch;
		dst += dstpitch;
	}
}

void VerticalBlurFilter::Blur(void *dst, const void *src1, const void *src2, const void *src3, uint32 count) {
	if (mbUsingSSE2) {
		uint32 count128 = count >> 4;
		uint32 count8 = count & 15;

		if (count128) {
			Blur128_SSE2(dst, src1, src2, src3, count128);
			dst = (char *)dst + count128*16;
			src1 = (char *)src1 + count128*16;
			src2 = (char *)src2 + count128*16;
			src3 = (char *)src3 + count128*16;
		}

		if (count8)
			Blur8(dst, src1, src2, src3, count8);
	}
#ifdef _M_IX86
	else if (mbUsingMMX) {
		uint32 count64 = count >> 3;
		uint32 count8 = count & 7;

		if (count64) {
			Blur64_MMX(dst, src1, src2, src3, count64);
			dst = (char *)dst + count64*8;
			src1 = (char *)src1 + count64*8;
			src2 = (char *)src2 + count64*8;
			src3 = (char *)src3 + count64*8;
		}

		if (count8)
			Blur8(dst, src1, src2, src3, count8);
	}
#endif
	else {
		uint32 count32 = count >> 2;
		uint32 count8 = count & 3;

		if (count32) {
			Blur32(dst, src1, src2, src3, count32);
			dst = (char *)dst + count32*4;
			src1 = (char *)src1 + count32*4;
			src2 = (char *)src2 + count32*4;
			src3 = (char *)src3 + count32*4;
		}

		if (count8)
			Blur8(dst, src1, src2, src3, count8);
	}
}

void VerticalBlurFilter::Blur8(void *dst, const void *src1, const void *src2, const void *src3, uint32 count) {
	uint8 *dst8 = (uint8 *)dst;
	const uint8 *src8_1 = (const uint8 *)src1;
	const uint8 *src8_2 = (const uint8 *)src2;
	const uint8 *src8_3 = (const uint8 *)src3;

	for(uint32 i=0; i<count; ++i) {
		uint32 p0 = src8_1[i];
		uint32 p1 = src8_2[i];
		uint32 p2 = src8_3[i];

		dst8[i] = ((p0 + p2) + p1*2 + 2) >> 2;
	}
}

void VerticalBlurFilter::Blur32(void *dst, const void *src1, const void *src2, const void *src3, uint32 count) {
	uint32 *dst32 = (uint32 *)dst;
	const uint32 *src32_1 = (const uint32 *)src1;
	const uint32 *src32_2 = (const uint32 *)src2;
	const uint32 *src32_3 = (const uint32 *)src3;

	for(uint32 i=0; i<count; ++i) {
		uint32 p0 = src32_1[i];
		uint32 p1 = src32_2[i];
		uint32 p2 = src32_3[i];
		uint32 p02 = (p0 & p2) + (((p0 ^ p2) & 0xfefefefe) >> 1);
		uint32 result = (p02 | p1) - (((p02 ^ p1) & 0xfefefefe) >> 1);

		dst32[i] = result;
	}
}

#ifdef _M_IX86
	void VerticalBlurFilter::Blur64_MMX(void *dst, const void *src1, const void *src2, const void *src3, uint32 count) {
		static const __declspec(align(8)) __int64 maskval = 0xfefefefefefefefe;

		__m64 *dst64 = (__m64 *)dst;
		const __m64 *src64_1 = (const __m64 *)src1;
		const __m64 *src64_2 = (const __m64 *)src2;
		const __m64 *src64_3 = (const __m64 *)src3;
		const __m64 mask = *(const __m64 *)&maskval;

		for(uint32 i=0; i<count; ++i) {
			__m64 p0 = src64_1[i];
			__m64 p1 = src64_2[i];
			__m64 p2 = src64_3[i];
			__m64 p02 = _mm_add_pi8(_mm_and_si64(p0, p2), _mm_srli_pi16(_mm_and_si64(_mm_xor_si64(p0, p2), mask), 1));
			__m64 res = _mm_sub_pi8(_mm_or_si64(p02, p1), _mm_srli_pi16(_mm_and_si64(_mm_xor_si64(p02, p1), mask), 1));

			dst64[i] = res;
		}

		_mm_empty();
	}
#endif

void VerticalBlurFilter::Blur128_SSE2(void *dst, const void *src1, const void *src2, const void *src3, uint32 count) {
	static const __declspec(align(16)) __int64 maskval[2] = { 0xffffffffffffffff, 0xffffffffffffffff };

	__int64 *dst64 = (__int64 *)dst;
	const __int64 *src64_1 = (const __int64 *)src1;
	const __int64 *src64_2 = (const __int64 *)src2;
	const __int64 *src64_3 = (const __int64 *)src3;
	const __m128i mask = *(const __m128i *)&maskval;

	for(uint32 i=0; i<count; ++i) {
		__m128i p0 = _mm_unpacklo_epi64(_mm_loadl_epi64((const __m128i *)&src64_1[0]), _mm_loadl_epi64((const __m128i *)&src64_1[1]));
		__m128i p1 = _mm_unpacklo_epi64(_mm_loadl_epi64((const __m128i *)&src64_2[0]), _mm_loadl_epi64((const __m128i *)&src64_2[1]));
		__m128i p2 = _mm_unpacklo_epi64(_mm_loadl_epi64((const __m128i *)&src64_3[0]), _mm_loadl_epi64((const __m128i *)&src64_3[1]));
		__m128i p02 = _mm_xor_si128(_mm_avg_epu8(_mm_xor_si128(p0, mask), _mm_xor_si128(p2, mask)), mask);
		__m128i res = _mm_avg_epu8(p02, p1);

		_mm_storel_epi64((__m128i *)&dst64[0], res);
		_mm_storel_epi64((__m128i *)&dst64[1], _mm_srli_si128(res, 8));

		dst64 += 2;
		src64_1 += 2;
		src64_2 += 2;
		src64_3 += 2;
	}
}

extern VDXFilterDefinition filterDef_verticalBlur = VDXVideoFilterDefinition<VerticalBlurFilter>("Avery Lee", "Sample: vertical blur", "Sample filter from the VirtualDub Plugin SDK: Applies a vertical blur to the video.");
