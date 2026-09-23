#ifndef MF_DECODER_H
#define MF_DECODER_H

#ifndef COBJMACROS
#define COBJMACROS
#endif
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

typedef struct {
    IMFSourceReader* reader;
    UINT32           videoWidth;
    UINT32           videoHeight;
    BYTE*            frameBuffer;   // Internal BGRA pixel buffer
    LONG             framePitch;    // Row stride in bytes
} Decoder;

// Initialises Media Foundation and opens the video file.
int  Decoder_Init(Decoder* d, const wchar_t* videoPath);

// Decodes the next frame. On success *ppPixels points to an internal
// BGRA buffer valid until the next call.  Returns 0 on success, -1 on error.
int  Decoder_ReadFrame(Decoder* d, const BYTE** ppPixels, LONG* pPitch);

// Releases all Media Foundation resources.
void Decoder_Shutdown(Decoder* d);

#endif // MF_DECODER_H
