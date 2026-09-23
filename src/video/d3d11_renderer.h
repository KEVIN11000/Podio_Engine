#ifndef D3D11_RENDERER_H
#define D3D11_RENDERER_H

#ifndef COBJMACROS
#define COBJMACROS
#endif
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>

typedef struct {
    ID3D11Device*        device;
    ID3D11DeviceContext* context;
    IDXGISwapChain*      swapChain;
    ID3D11Texture2D*     backBuffer;
    UINT                 width;
    UINT                 height;
} Renderer;

// Creates D3D11 device + swap chain bound to the given HWND.
int  Renderer_Init(Renderer* r, HWND hwnd);

// Copies BGRA pixel data into the back buffer and presents with VSync.
void Renderer_UploadAndPresent(Renderer* r, const void* bgraData, UINT rowPitch);

// Releases all D3D11 resources.
void Renderer_Shutdown(Renderer* r);

#endif // D3D11_RENDERER_H
