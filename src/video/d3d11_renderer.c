#include "d3d11_renderer.h"
#include "../utils/logger.h"
#include <string.h>

int Renderer_Init(Renderer* r, HWND hwnd, UINT videoWidth, UINT videoHeight) {
    memset(r, 0, sizeof(*r));

    r->width  = videoWidth;
    r->height = videoHeight;

    // Swap chain descriptor — DXGI 1.0 compatible
    DXGI_SWAP_CHAIN_DESC scd;
    memset(&scd, 0, sizeof(scd));
    scd.BufferDesc.Width            = r->width;
    scd.BufferDesc.Height           = r->height;
    scd.BufferDesc.Format           = DXGI_FORMAT_B8G8R8A8_UNORM;
    scd.BufferDesc.RefreshRate.Numerator   = 60;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.SampleDesc.Count  = 1;
    scd.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.BufferCount        = 1;
    scd.OutputWindow       = hwnd;
    scd.Windowed           = TRUE;
    scd.SwapEffect         = DXGI_SWAP_EFFECT_DISCARD;

    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_VIDEO_SUPPORT;
    D3D_FEATURE_LEVEL featureLevel;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        NULL,                       // default adapter
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,                       // no software rasteriser
        flags,
        NULL, 0,                    // default feature levels
        D3D11_SDK_VERSION,
        &scd,
        &r->swapChain,
        &r->device,
        &featureLevel,
        &r->context);

    if (FAILED(hr)) {
        Logger_Log(LOG_ERROR, "D3D11CreateDeviceAndSwapChain failed: 0x%08lX", (unsigned long)hr);
        return -1;
    }

    // Enable multithread protection (required for Media Foundation)
    ID3D10Multithread* mt = NULL;
    hr = ID3D11Device_QueryInterface(r->device, &IID_ID3D10Multithread, (void**)&mt);
    if (SUCCEEDED(hr) && mt) {
        ID3D10Multithread_SetMultithreadProtected(mt, TRUE);
        ID3D10Multithread_Release(mt);
    }

    // Get the back buffer texture
    hr = IDXGISwapChain_GetBuffer(r->swapChain, 0,
                                  &IID_ID3D11Texture2D, (void**)&r->backBuffer);
    if (FAILED(hr)) {
        Logger_Log(LOG_ERROR, "SwapChain GetBuffer failed: 0x%08lX", (unsigned long)hr);
        Renderer_Shutdown(r);
        return -1;
    }

    Logger_Log(LOG_INFO, "D3D11 renderer initialised (%ux%u, FL 0x%X)",
               r->width, r->height, (unsigned)featureLevel);
    return 0;
}

void Renderer_UploadAndPresent(Renderer* r, const void* bgraData, UINT rowPitch) {
    if (!r->backBuffer || !bgraData) return;

    // Upload BGRA pixels straight into the back buffer
    ID3D11DeviceContext_UpdateSubresource(
        r->context,
        (ID3D11Resource*)r->backBuffer, 0, NULL,
        bgraData, rowPitch, 0);

    // Present with VSync (SyncInterval = 1)
    IDXGISwapChain_Present(r->swapChain, 1, 0);
}

void Renderer_Shutdown(Renderer* r) {
    if (r->backBuffer) { ID3D11Texture2D_Release(r->backBuffer);     r->backBuffer = NULL; }
    if (r->swapChain)  { IDXGISwapChain_Release(r->swapChain);       r->swapChain  = NULL; }
    if (r->context)    { ID3D11DeviceContext_Release(r->context);     r->context    = NULL; }
    if (r->device)     { ID3D11Device_Release(r->device);             r->device     = NULL; }
}
