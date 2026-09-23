#include "mf_decoder.h"
#include "../utils/logger.h"

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <propvarutil.h>
#include <string.h>
#include <stdlib.h>

/* ── Helper: unpack MF_MT_FRAME_SIZE (UINT64 → width, height) ────── */
static HRESULT GetAttributeSize(IMFMediaType* pType, REFGUID key,
                                UINT32* pw, UINT32* ph) {
    UINT64 packed = 0;
    HRESULT hr = IMFMediaType_GetUINT64(pType, key, &packed);
    if (SUCCEEDED(hr)) {
        *pw = (UINT32)(packed >> 32);
        *ph = (UINT32)(packed & 0xFFFFFFFF);
    }
    return hr;
}

/* ── Init ─────────────────────────────────────────────────────────── */
int Decoder_Init(Decoder* d, const wchar_t* videoPath) {
    memset(d, 0, sizeof(*d));

    HRESULT hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    if (FAILED(hr)) {
        Logger_Log(LOG_ERROR, "MFStartup failed: 0x%08lX", (unsigned long)hr);
        return -1;
    }

    /* Source-reader attributes: enable built-in video processing
       (colour-space conversion to RGB32). */
    IMFAttributes* attrs = NULL;
    hr = MFCreateAttributes(&attrs, 1);
    if (FAILED(hr)) { MFShutdown(); return -1; }

    IMFAttributes_SetUINT32(attrs,
        &MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);

    hr = MFCreateSourceReaderFromURL(videoPath, attrs, &d->reader);
    IMFAttributes_Release(attrs);
    if (FAILED(hr)) {
        Logger_Log(LOG_ERROR, "MFCreateSourceReaderFromURL failed: 0x%08lX",
                   (unsigned long)hr);
        MFShutdown();
        return -1;
    }

    /* Query native video dimensions. */
    IMFMediaType* nativeType = NULL;
    hr = IMFSourceReader_GetNativeMediaType(
            d->reader,
            (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
            0, &nativeType);
    if (SUCCEEDED(hr)) {
        GetAttributeSize(nativeType, &MF_MT_FRAME_SIZE,
                         &d->videoWidth, &d->videoHeight);
    }
    int scrW = GetSystemMetrics(SM_CXSCREEN);
    int scrH = GetSystemMetrics(SM_CYSCREEN);

    /* We no longer rely on native dimensions because we will force resize. */
    d->videoWidth  = (UINT32)scrW;
    d->videoHeight = (UINT32)scrH;

    /* Request RGB32 (BGRA) output at SCREEN dimensions to match the D3D11 backbuffer. */

    /* We must use the native type as a base so we don't miss required attributes 
       like interlace mode or aspect ratio, which causes MF_E_INVALIDMEDIATYPE. */
    IMFMediaType* outType = NULL;
    hr = MFCreateMediaType(&outType);
    if (SUCCEEDED(hr)) {
        if (nativeType) {
            IMFMediaType_CopyAllItems(nativeType, (IMFAttributes*)outType);
        }
        IMFMediaType_SetGUID(outType, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
        IMFMediaType_SetGUID(outType, &MF_MT_SUBTYPE, &MFVideoFormat_RGB32);

        UINT64 packedSize = ((UINT64)scrW << 32) | (UINT32)scrH;
        IMFMediaType_SetUINT64(outType, &MF_MT_FRAME_SIZE, packedSize);

        /* Reset the stride if copied, so MF calculates the correct new stride for RGB32 */
        IMFMediaType_DeleteItem(outType, &MF_MT_DEFAULT_STRIDE);
        IMFMediaType_DeleteItem(outType, &MF_MT_SAMPLE_SIZE);
    }
    if (nativeType) {
        IMFMediaType_Release(nativeType);
    }

    hr = IMFSourceReader_SetCurrentMediaType(
            d->reader,
            (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
            NULL, outType);
    if (outType) IMFMediaType_Release(outType);

    if (FAILED(hr)) {
        Logger_Log(LOG_ERROR, "SetCurrentMediaType(RGB32) failed: 0x%08lX",
                   (unsigned long)hr);
        Decoder_Shutdown(d);
        return -1;
    }

    /* Allocate an internal pixel buffer (BGRA = 4 Bpp). */
    d->framePitch  = (LONG)(d->videoWidth * 4);
    d->frameBuffer = (BYTE*)malloc((size_t)d->framePitch * d->videoHeight);
    if (!d->frameBuffer) {
        Logger_Log(LOG_ERROR, "Failed to allocate frame buffer.");
        Decoder_Shutdown(d);
        return -1;
    }

    Logger_Log(LOG_INFO, "MF decoder initialised: %ux%u RGB32",
               d->videoWidth, d->videoHeight);
    return 0;
}

/* ── Read one frame ───────────────────────────────────────────────── */
int Decoder_ReadFrame(Decoder* d, const BYTE** ppPixels, LONG* pPitch) {
    if (!d->reader) return -1;

    DWORD  streamIndex = 0;
    DWORD  flags       = 0;
    LONGLONG timestamp = 0;
    IMFSample* sample  = NULL;

    HRESULT hr = IMFSourceReader_ReadSample(
        d->reader,
        (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
        0, &streamIndex, &flags, &timestamp, &sample);

    if (FAILED(hr)) return -1;

    /* End-of-stream → seamless loop back to frame 0. */
    if (flags & MF_SOURCE_READERF_ENDOFSTREAM) {
        PROPVARIANT pos;
        PropVariantInit(&pos);
        pos.vt = VT_I8;
        pos.hVal.QuadPart = 0;
        IMFSourceReader_SetCurrentPosition(d->reader, &GUID_NULL, &pos);
        PropVariantClear(&pos);

        /* Read again from the top. */
        hr = IMFSourceReader_ReadSample(
            d->reader,
            (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
            0, &streamIndex, &flags, &timestamp, &sample);
        if (FAILED(hr) || !sample) return -1;
    }

    if (!sample) return -1;

    /* Extract pixel bytes from the sample. */
    IMFMediaBuffer* buffer = NULL;
    hr = IMFSample_ConvertToContiguousBuffer(sample, &buffer);
    if (FAILED(hr)) {
        IMFSample_Release(sample);
        return -1;
    }

    BYTE* src  = NULL;
    DWORD curLen = 0;
    hr = IMFMediaBuffer_Lock(buffer, &src, NULL, &curLen);
    if (SUCCEEDED(hr)) {
        DWORD expected = (DWORD)d->framePitch * d->videoHeight;
        DWORD toCopy   = (curLen < expected) ? curLen : expected;
        memcpy(d->frameBuffer, src, toCopy);
        IMFMediaBuffer_Unlock(buffer);
    }

    IMFMediaBuffer_Release(buffer);
    IMFSample_Release(sample);

    *ppPixels = d->frameBuffer;
    *pPitch   = d->framePitch;
    return 0;
}

/* ── Shutdown ─────────────────────────────────────────────────────── */
void Decoder_Shutdown(Decoder* d) {
    if (d->reader)      { IMFSourceReader_Release(d->reader); d->reader = NULL; }
    if (d->frameBuffer) { free(d->frameBuffer); d->frameBuffer = NULL; }
    MFShutdown();
}
