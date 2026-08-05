/*
 * iOS Metal Graphics Backend Implementation
 *
 * Copyright 2024 Wine Project
 *
 * Implements GDI/GDI+ functions using Apple's Metal framework.
 * This allows Wine to render Windows graphics on iOS devices.
 */

#include "ios_graphics.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dispatch/dispatch.h>

#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#if HAVE_METAL
#include <Metal/Metal.h>
#include <MetalKit/MetalKit.h>
#endif

#define TRACE_GRAPHICS 0

#if TRACE_GRAPHICS
#define TRACE(fmt, ...) fprintf(stderr, "[ios-graphics] " fmt "\n", ##__VA_ARGS__)
#else
#define TRACE(fmt, ...) do {} while(0)
#endif

#define WARN(fmt, ...) fprintf(stderr, "[ios-graphics:warn] " fmt "\n", ##__VA_ARGS__)
#define ERR(fmt, ...) fprintf(stderr, "[ios-graphics:err] " fmt "\n", ##__VA_ARGS__)

/* Global Metal resources */
#if HAVE_METAL
static id<MTLDevice> g_metal_device = nil;
static id<MTLCommandQueue> g_metal_queue = nil;
#endif

/* Surface cache */
#define MAX_SURFACES 256
static WINE_SURFACE *g_surface_cache[MAX_SURFACES];
static int g_surface_count = 0;
static pthread_mutex_t g_surface_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Utility functions
 */

static inline uint32_t blend_pixels(uint32_t dest, uint32_t src, uint32_t alpha)
{
    uint32_t inv_alpha = 256 - alpha;
    uint32_t r = ((dest & 0x00FF0000) >> 16) * inv_alpha + ((src & 0x00FF0000) >> 16) * alpha;
    uint32_t g = ((dest & 0x0000FF00) >> 8) * inv_alpha + ((src & 0x0000FF00) >> 8) * alpha;
    uint32_t b = (dest & 0x000000FF) * inv_alpha + (src & 0x000000FF) * alpha;
    return 0xFF000000 | ((r >> 8) << 16) | ((g >> 8) << 8) | (b >> 8);
}

static inline void swap_int(int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

static inline int min_int(int a, int b) { return a < b ? a : b; }
static inline int max_int(int a, int b) { return a > b ? a : b; }

static inline float min_float(float a, float b) { return a < b ? a : b; }
static inline float max_float(float a, float b) { return a > b ? a : b; }

/*
 * Surface functions
 */

WINE_SURFACE *ios_surface_create(int width, int height, int bpp, const void *data)
{
    TRACE("ios_surface_create(%d, %d, %d)", width, height, bpp);
    
    if (width <= 0 || height <= 0) {
        ERR("Invalid surface dimensions: %dx%d", width, height);
        return NULL;
    }
    
    int bytes_per_pixel = (bpp + 7) / 8;
    int stride = (width * bytes_per_pixel + 3) & ~3;  /* Align to 4 bytes */
    int size = stride * height;
    
    WINE_SURFACE *surface = calloc(1, sizeof(WINE_SURFACE));
    if (!surface) {
        ERR("Failed to allocate surface");
        return NULL;
    }
    
    surface->type = SURFACE_TYPE_DIB;
    surface->width = width;
    surface->height = height;
    surface->stride = stride;
    surface->data = calloc(1, size);
    surface->dirty = false;
    surface->refcount = 1;
    
    if (!surface->data) {
        ERR("Failed to allocate surface data");
        free(surface);
        return NULL;
    }
    
    if (data) {
        memcpy(surface->data, data, min_int(size, stride * height));
    }
    
#if HAVE_METAL
    /* Create Metal texture if Metal is available */
    if (g_metal_device) {
        MTLTextureDescriptor *desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:
                                       MTLPixelFormatBGRA8Unorm width:width height:height mipmapped:NO];
        desc.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite | MTLTextureUsageRenderTarget;
        surface->texture = [g_metal_device newTextureWithDescriptor:desc];
        
        if (surface->texture) {
            /* Copy data to texture */
            MTLRegion region = MTLRegionMake2D(0, 0, width, height);
            [surface->texture replaceRegion:region mipmapLevel:0 withBytes:surface->data bytesPerRow:stride];
        }
    }
#endif
    
    /* Add to cache */
    pthread_mutex_lock(&g_surface_mutex);
    if (g_surface_count < MAX_SURFACES) {
        g_surface_cache[g_surface_count++] = surface;
    }
    pthread_mutex_unlock(&g_surface_mutex);
    
    TRACE("Created surface: %p, %dx%d, stride=%d", surface, width, height, stride);
    
    return surface;
}

WINE_SURFACE *ios_surface_create_dib(int width, int height, int bpp, void **bits)
{
    TRACE("ios_surface_create_dib(%d, %d, %d)", width, height, bpp);
    
    WINE_SURFACE *surface = ios_surface_create(width, height, bpp, NULL);
    if (surface && bits) {
        *bits = surface->data;
    }
    
    return surface;
}

WINE_SURFACE *ios_surface_create_from_cgimage(CGImageRef image)
{
    if (!image) return NULL;
    
    size_t width = CGImageGetWidth(image);
    size_t height = CGImageGetHeight(image);
    
    WINE_SURFACE *surface = ios_surface_create(width, height, 32, NULL);
    if (!surface) return NULL;
    
    /* Create a context to draw the image into the surface */
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate(
        surface->data, width, height, 8, surface->stride,
        colorSpace, kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little
    );
    
    if (context) {
        /* Flip the context for correct orientation */
        CGContextTranslateCTM(context, 0, height);
        CGContextScaleCTM(context, 1.0, -1.0);
        
        /* Draw the image */
        CGContextDrawImage(context, CGRectMake(0, 0, width, height), image);
        CGContextRelease(context);
        
        surface->dirty = true;
    }
    
    CGColorSpaceRelease(colorSpace);
    
    return surface;
}

void ios_surface_destroy(WINE_SURFACE *surface)
{
    if (!surface) return;
    
    TRACE("ios_surface_destroy(%p), refcount=%d", surface, surface->refcount);
    
    surface->refcount--;
    if (surface->refcount > 0) return;
    
    /* Remove from cache */
    pthread_mutex_lock(&g_surface_mutex);
    for (int i = 0; i < g_surface_count; i++) {
        if (g_surface_cache[i] == surface) {
            g_surface_cache[i] = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&g_surface_mutex);
    
    /* Release resources */
    if (surface->data) {
        free(surface->data);
    }
    
#if HAVE_METAL
    if (surface->texture) {
        [surface->texture release];
    }
    if (surface->buffer) {
        [surface->buffer release];
    }
#endif
    
    free(surface);
}

void ios_surface_get_size(WINE_SURFACE *surface, int *width, int *height)
{
    if (!surface) return;
    if (width) *width = surface->width;
    if (height) *height = surface->height;
}

void *ios_surface_get_data(WINE_SURFACE *surface)
{
    if (!surface) return NULL;
    return surface->data;
}

int ios_surface_get_stride(WINE_SURFACE *surface)
{
    if (!surface) return 0;
    return surface->stride;
}

BOOL ios_surface_lock(WINE_SURFACE *surface)
{
    if (!surface) return FALSE;
    return TRUE;
}

void ios_surface_unlock(WINE_SURFACE *surface)
{
    if (!surface) return;
    surface->dirty = true;
}

void ios_surface_mark_dirty(WINE_SURFACE *surface)
{
    if (!surface) return;
    surface->dirty = true;
}

CGImageRef ios_surface_to_cgimage(WINE_SURFACE *surface)
{
    if (!surface) return NULL;
    
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    
    CGImageRef image = CGBitmapContextCreateImage(
        CGBitmapContextCreate(
            surface->data, surface->width, surface->height, 8, surface->stride,
            colorSpace, kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little
        )
    );
    
    CGColorSpaceRelease(colorSpace);
    return image;
}

/*
 * DC functions
 */

static WINE_DC *ios_alloc_dc(void)
{
    WINE_DC *dc = calloc(1, sizeof(WINE_DC));
    if (dc) {
        dc->rop2 = R2_COPYPEN;
        dc->bg_mode = OPAQUE;
        dc->map_mode = MM_TEXT;
        dc->text_color = RGB(0, 0, 0);
        dc->bg_color = RGB(255, 255, 255);
        dc->poly_fill_mode = ALTERNATE;
        dc->stretch_blt_mode = BLACKONWHITE;
        dc->arc_direction = AD_COUNTERCLOCKWISE;
        dc->charset = ANSI_CHARSET;
        dc->miter_limit = 10.0;
        dc->viewport_ext.cx = 1;
        dc->viewport_ext.cy = 1;
        dc->window_ext.cx = 1;
        dc->window_ext.cy = 1;
        dc->refcount = 1;
    }
    return dc;
}

HDC ios_create_dc(void)
{
    TRACE("ios_create_dc()");
    
    WINE_DC *dc = ios_alloc_dc();
    if (dc) {
        /* Create a default 1x1 surface */
        dc->surface = ios_surface_create(1, 1, 32, NULL);
    }
    
    return (HDC)dc;
}

HDC ios_create_compatible_dc(HDC hdc)
{
    TRACE("ios_create_compatible_dc(%p)", hdc);
    
    WINE_DC *dc = ios_alloc_dc();
    if (dc && hdc) {
        WINE_DC *src = (WINE_DC *)hdc;
        if (src->surface) {
            dc->surface = ios_surface_create(src->surface->width, src->surface->height, 32, NULL);
        }
    } else {
        dc->surface = ios_surface_create(1, 1, 32, NULL);
    }
    
    return (HDC)dc;
}

BOOL ios_delete_dc(HDC hdc)
{
    if (!hdc) return FALSE;
    
    WINE_DC *dc = (WINE_DC *)hdc;
    
    if (dc->surface) {
        ios_surface_destroy(dc->surface);
    }
    
    free(dc);
    return TRUE;
}

WINE_SURFACE *ios_dc_get_surface(HDC hdc)
{
    if (!hdc) return NULL;
    WINE_DC *dc = (WINE_DC *)hdc;
    return dc->surface;
}

HBITMAP ios_select_surface(HDC hdc, WINE_SURFACE *surface)
{
    if (!hdc) return NULL;
    WINE_DC *dc = (WINE_DC *)hdc;
    
    HBITMAP old = (HBITMAP)dc->surface;
    if (surface) {
        dc->surface = surface;
        surface->refcount++;
    }
    
    return old;
}

/*
 * Drawing functions
 */

COLORREF ios_set_pixel(HDC hdc, int x, int y, COLORREF color)
{
    if (!hdc) return CLR_INVALID;
    WINE_DC *dc = (WINE_DC *)hdc;
    if (!dc->surface || !dc->surface->data) return CLR_INVALID;
    
    if (x < 0 || x >= dc->surface->width || y < 0 || y >= dc->surface->height) {
        return CLR_INVALID;
    }
    
    uint32_t *row = (uint32_t *)((char *)dc->surface->data + y * dc->surface->stride);
    uint32_t old = row[x];
    row[x] = color | 0xFF000000;  /* Ensure alpha is 255 */
    dc->surface->dirty = true;
    
    return old;
}

COLORREF ios_get_pixel(HDC hdc, int x, int y)
{
    if (!hdc) return CLR_INVALID;
    WINE_DC *dc = (WINE_DC *)hdc;
    if (!dc->surface || !dc->surface->data) return CLR_INVALID;
    
    if (x < 0 || x >= dc->surface->width || y < 0 || y >= dc->surface->height) {
        return CLR_INVALID;
    }
    
    uint32_t *row = (uint32_t *)((char *)dc->surface->data + y * dc->surface->stride);
    return row[x];
}

BOOL ios_move_to(HDC hdc, int x, int y, POINT *old_point)
{
    if (!hdc) return FALSE;
    WINE_DC *dc = (WINE_DC *)hdc;
    
    if (old_point) {
        old_point->x = dc->cur_pos.x;
        old_point->y = dc->cur_pos.y;
    }
    
    dc->cur_pos.x = x;
    dc->cur_pos.y = y;
    
    return TRUE;
}

BOOL ios_line_to(HDC hdc, int x, int y)
{
    if (!hdc) return FALSE;
    WINE_DC *dc = (WINE_DC *)hdc;
    if (!dc->surface || !dc->surface->data) return FALSE;
    
    /* Bresenham's line algorithm */
    int x0 = dc->cur_pos.x;
    int y0 = dc->cur_pos.y;
    
    int dx = abs(x - x0);
    int dy = abs(y - y0);
    int sx = x0 < x ? 1 : -1;
    int sy = y0 < y ? 1 : -1;
    int err = dx - dy;
    
    uint32_t color = dc->text_color | 0xFF000000;
    int width = dc->surface->width;
    int height = dc->surface->height;
    int stride = dc->surface->stride;
    uint32_t *data = (uint32_t *)dc->surface->data;
    
    while (1) {
        if (x0 >= 0 && x0 < width && y0 >= 0 && y0 < height) {
            data[y0 * (stride / 4) + x0] = color;
        }
        
        if (x0 == x && y0 == y) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
    
    dc->cur_pos.x = x;
    dc->cur_pos.y = y;
    dc->surface->dirty = true;
    
    return TRUE;
}

BOOL ios_polyline(HDC hdc, const POINT *points, int count)
{
    if (!hdc || !points || count < 2) return FALSE;
    WINE_DC *dc = (WINE_DC *)hdc;
    
    for (int i = 0; i < count - 1; i++) {
        ios_move_to(hdc, points[i].x, points[i].y, NULL);
        ios_line_to(hdc, points[i + 1].x, points[i + 1].y);
    }
    
    return TRUE;
}

BOOL ios_rectangle(HDC hdc, int left, int top, int right, int bottom)
{
    if (!hdc) return FALSE;
    
    /* Normalize coordinates */
    if (left > right) swap_int(&left, &right);
    if (top > bottom) swap_int(&top, &bottom);
    
    WINE_DC *dc = (WINE_DC *)hdc;
    if (!dc->surface || !dc->surface->data) return FALSE;
    
    uint32_t color = dc->text_color | 0xFF000000;
    int stride = dc->surface->stride;
    uint32_t *data = (uint32_t *)dc->surface->data;
    
    /* Draw top and bottom lines */
    for (int x = left; x <= right; x++) {
        if (x >= 0 && x < dc->surface->width) {
            if (top >= 0 && top < dc->surface->height) {
                data[top * (stride / 4) + x] = color;
            }
            if (bottom >= 0 && bottom < dc->surface->height) {
                data[bottom * (stride / 4) + x] = color;
            }
        }
    }
    
    /* Draw left and right lines */
    for (int y = top; y <= bottom; y++) {
        if (y >= 0 && y < dc->surface->height) {
            if (left >= 0 && left < dc->surface->width) {
                data[y * (stride / 4) + left] = color;
            }
            if (right >= 0 && right < dc->surface->width) {
                data[y * (stride / 4) + right] = color;
            }
        }
    }
    
    dc->surface->dirty = true;
    
    return TRUE;
}

BOOL ios_round_rect(HDC hdc, int left, int top, int right, int bottom,
                    int ellipsis_width, int ellipsis_height)
{
    /* Simplified: just draw a regular rectangle */
    return ios_rectangle(hdc, left, top, right, bottom);
}

BOOL ios_ellipse(HDC hdc, int left, int top, int right, int bottom)
{
    if (!hdc) return FALSE;
    
    if (left > right) swap_int(&left, &right);
    if (top > bottom) swap_int(&top, &bottom);
    
    WINE_DC *dc = (WINE_DC *)hdc;
    if (!dc->surface || !dc->surface->data) return FALSE;
    
    int cx = (left + right) / 2;
    int cy = (top + bottom) / 2;
    int rx = (right - left) / 2;
    int ry = (bottom - top) / 2;
    
    uint32_t color = dc->text_color | 0xFF000000;
    int stride = dc->surface->stride;
    uint32_t *data = (uint32_t *)dc->surface->data;
    
    /* Midpoint ellipse algorithm */
    int x = 0, y = ry;
    int rx2 = rx * rx;
    int ry2 = ry * ry;
    int two_rx2 = 2 * rx2;
    int two_ry2 = 2 * ry2;
    
    int px = 0, py = two_rx2 * y;
    
    void plot_ellipse_points(int x, int y) {
        if (cx + x >= 0 && cx + x < dc->surface->width && cy + y >= 0 && cy + y < dc->surface->height)
            data[(cy + y) * (stride / 4) + (cx + x)] = color;
        if (cx - x >= 0 && cx - x < dc->surface->width && cy + y >= 0 && cy + y < dc->surface->height)
            data[(cy + y) * (stride / 4) + (cx - x)] = color;
        if (cx + x >= 0 && cx + x < dc->surface->width && cy - y >= 0 && cy - y < dc->surface->height)
            data[(cy - y) * (stride / 4) + (cx + x)] = color;
        if (cx - x >= 0 && cx - x < dc->surface->width && cy - y >= 0 && cy - y < dc->surface->height)
            data[(cy - y) * (stride / 4) + (cx - x)] = color;
    }
    
    plot_ellipse_points(x, y);
    
    /* Region 1 */
    int p = (int)(ry2 - rx2 * ry + 0.25 * rx2);
    while (px < py) {
        x++;
        px += two_ry2;
        if (p < 0) {
            p += ry2 + px;
        } else {
            y--;
            py -= two_rx2;
            p += ry2 + px - py;
        }
        plot_ellipse_points(x, y);
    }
    
    /* Region 2 */
    p = (int)(ry2 * (x + 0.5) * (x + 0.5) + rx2 * (y - 1) * (y - 1) - rx2 * ry2);
    while (y > 0) {
        y--;
        py -= two_rx2;
        if (p > 0) {
            p += rx2 - py;
        } else {
            x++;
            px += two_ry2;
            p += rx2 - py + px;
        }
        plot_ellipse_points(x, y);
    }
    
    dc->surface->dirty = true;
    
    return TRUE;
}

BOOL ios_bit_blt(HDC hdc_dst, int x_dst, int y_dst, int width, int height,
                 HDC hdc_src, int x_src, int y_src, DWORD rop)
{
    if (!hdc_dst) return FALSE;
    WINE_DC *dc_dst = (WINE_DC *)hdc_dst;
    if (!dc_dst->surface || !dc_dst->surface->data) return FALSE;
    
    if (rop == SRCCOPY) {
        /* Simple copy operation */
        uint32_t *dst_data = (uint32_t *)dc_dst->surface->data;
        int dst_stride = dc_dst->surface->stride;
        
        if (hdc_src) {
            WINE_DC *dc_src = (WINE_DC *)hdc_src;
            if (dc_src->surface && dc_src->surface->data) {
                uint32_t *src_data = (uint32_t *)dc_src->surface->data;
                int src_stride = dc_src->surface->stride;
                
                for (int y = 0; y < height; y++) {
                    int dst_y = y_dst + y;
                    int src_y = y_src + y;
                    if (dst_y < 0 || dst_y >= dc_dst->surface->height) continue;
                    if (src_y < 0 || src_y >= dc_src->surface->height) continue;
                    
                    for (int x = 0; x < width; x++) {
                        int dst_x = x_dst + x;
                        int src_x = x_src + x;
                        if (dst_x < 0 || dst_x >= dc_dst->surface->width) continue;
                        if (src_x < 0 || src_x >= dc_src->surface->width) continue;
                        
                        dst_data[dst_y * (dst_stride / 4) + dst_x] = 
                            src_data[src_y * (src_stride / 4) + src_x];
                    }
                }
            }
        } else {
            /* Fill with current background color */
            uint32_t color = dc_dst->bg_color | 0xFF000000;
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    int dst_x = x_dst + x;
                    int dst_y = y_dst + y;
                    if (dst_x >= 0 && dst_x < dc_dst->surface->width &&
                        dst_y >= 0 && dst_y < dc_dst->surface->height) {
                        dst_data[dst_y * (dst_stride / 4) + dst_x] = color;
                    }
                }
            }
        }
        
        dc_dst->surface->dirty = true;
    }
    
    return TRUE;
}

BOOL ios_stretch_blt(HDC hdc_dst, int x_dst, int y_dst, int width, int height,
                     HDC hdc_src, int x_src, int y_src, int src_width, int src_height,
                     DWORD rop)
{
    if (!hdc_dst) return FALSE;
    WINE_DC *dc_dst = (WINE_DC *)hdc_dst;
    if (!dc_dst->surface || !dc_dst->surface->data) return FALSE;
    
    if (rop == SRCCOPY && hdc_src) {
        WINE_DC *dc_src = (WINE_DC *)hdc_src;
        if (!dc_src->surface || !dc_src->surface->data) return FALSE;
        
        uint32_t *dst_data = (uint32_t *)dc_dst->surface->data;
        int dst_stride = dc_dst->surface->stride;
        uint32_t *src_data = (uint32_t *)dc_src->surface->data;
        int src_stride = dc_src->surface->stride;
        
        /* Bilinear interpolation would be ideal, but for simplicity use nearest neighbor */
        for (int y = 0; y < height; y++) {
            int src_y = y_src + (y * src_height) / height;
            if (src_y < 0) src_y = 0;
            if (src_y >= dc_src->surface->height) src_y = dc_src->surface->height - 1;
            
            int dst_y = y_dst + y;
            if (dst_y < 0 || dst_y >= dc_dst->surface->height) continue;
            
            for (int x = 0; x < width; x++) {
                int src_x = x_src + (x * src_width) / width;
                if (src_x < 0) src_x = 0;
                if (src_x >= dc_src->surface->width) src_x = dc_src->surface->width - 1;
                
                int dst_x = x_dst + x;
                if (dst_x < 0 || dst_x >= dc_dst->surface->width) continue;
                
                dst_data[dst_y * (dst_stride / 4) + dst_x] = 
                    src_data[src_y * (src_stride / 4) + src_x];
            }
        }
        
        dc_dst->surface->dirty = true;
    }
    
    return TRUE;
}

BOOL ios_pat_blt(HDC hdc, int x, int y, int width, int height, DWORD rop)
{
    return ios_bit_blt(hdc, x, y, width, height, NULL, 0, 0, rop);
}

/*
 * Brush functions
 */

HBRUSH ios_create_solid_brush(COLORREF color)
{
    TRACE("ios_create_solid_brush(0x%08x)", color);
    
    /* Store color in brush handle (simplified) */
    HBRUSH brush = (HBRUSH)(uintptr_t)(color | 0x80000000);
    return brush;
}

/*
 * Pen functions
 */

HPEN ios_create_pen(int style, int width, COLORREF color)
{
    TRACE("ios_create_pen(style=%d, width=%d, color=0x%08x)", style, width, color);
    
    /* Store pen parameters in handle (simplified) */
    uint64_t pen_data = ((uint64_t)style << 48) | ((uint64_t)width << 32) | color;
    HPEN pen = (HPEN)(uintptr_t)(pen_data | 0x8000000000000000ULL);
    return pen;
}

/*
 * Bitmap functions
 */

HBITMAP ios_create_bitmap(int width, int height, UINT planes, UINT bpp, const void *bits)
{
    TRACE("ios_create_bitmap(%d, %d, %d, %d)", width, height, planes, bpp);
    
    if (planes != 1) {
        ERR("Only 1 plane supported, got %d", planes);
        return NULL;
    }
    
    WINE_SURFACE *surface = ios_surface_create(width, height, bpp, bits);
    return (HBITMAP)surface;
}

HBITMAP ios_create_compatible_bitmap(HDC hdc, int width, int height)
{
    TRACE("ios_create_compatible_bitmap(%p, %d, %d)", hdc, width, height);
    return (HBITMAP)ios_surface_create(width, height, 32, NULL);
}

HBITMAP ios_create_dib_section(HDC hdc, int width, int height, int bpp, 
                                void **bits, HANDLE section, DWORD offset)
{
    TRACE("ios_create_dib_section(%p, %d, %d, %d)", hdc, width, height, bpp);
    
    WINE_SURFACE *surface = ios_surface_create(width, height, bpp, NULL);
    if (surface && bits) {
        *bits = surface->data;
    }
    
    return (HBITMAP)surface;
}

int ios_get_dibits(HDC hdc, HBITMAP bitmap, UINT start_scan, UINT scan_lines,
                   void *bits, void *info, UINT usage)
{
    if (!bitmap) return 0;
    
    WINE_SURFACE *surface = (WINE_SURFACE *)bitmap;
    if (!surface->data) return 0;
    
    /* Simplified implementation */
    int copy_height = min_int(scan_lines, surface->height - start_scan);
    if (copy_height <= 0) return 0;
    
    char *src = (char *)surface->data + start_scan * surface->stride;
    char *dst = (char *)bits;
    
    memcpy(dst, src, copy_height * surface->stride);
    
    return copy_height;
}

/*
 * Clipping functions
 */

int ios_select_clip_rgn(HDC hdc, HRGN rgn)
{
    if (!hdc) return ERROR;
    WINE_DC *dc = (WINE_DC *)hdc;
    
    dc->clip_region = rgn;
    return SIMPLEREGION;
}

int ios_get_clip_rgn(HDC hdc, HRGN rgn)
{
    if (!hdc) return ERROR;
    WINE_DC *dc = (WINE_DC *)hdc;
    
    if (!dc->clip_region) return NULLREGION;
    return SIMPLEREGION;
}

int ios_exclude_clip_rect(HDC hdc, int left, int top, int right, int bottom)
{
    if (!hdc) return ERROR;
    return SIMPLEREGION;
}

int ios_intersect_clip_rect(HDC hdc, int left, int top, int right, int bottom)
{
    if (!hdc) return ERROR;
    return SIMPLEREGION;
}

/*
 * Color adjustment
 */

BOOL ios_set_color_adjustment(HDC hdc, const void *ca)
{
    if (!hdc) return FALSE;
    return TRUE;
}

/*
 * Coordinate functions
 */

BOOL ios_dp_to_lp(HDC hdc, POINT *points, int count)
{
    if (!hdc || !points) return FALSE;
    /* In MM_TEXT mode, DP == LP */
    return TRUE;
}

BOOL ios_lp_to_dp(HDC hdc, POINT *points, int count)
{
    if (!hdc || !points) return FALSE;
    /* In MM_TEXT mode, DP == LP */
    return TRUE;
}

BOOL ios_set_viewport_org_ex(HDC hdc, int x, int y, POINT *old_point)
{
    if (!hdc) return FALSE;
    WINE_DC *dc = (WINE_DC *)hdc;
    
    if (old_point) {
        old_point->x = dc->viewport_org.x;
        old_point->y = dc->viewport_org.y;
    }
    
    dc->viewport_org.x = x;
    dc->viewport_org.y = y;
    
    return TRUE;
}

BOOL ios_get_viewport_org_ex(HDC hdc, POINT *point)
{
    if (!hdc) return FALSE;
    WINE_DC *dc = (WINE_DC *)hdc;
    
    if (point) {
        point->x = dc->viewport_org.x;
        point->y = dc->viewport_org.y;
    }
    
    return TRUE;
}

/*
 * Path functions
 */

BOOL ios_begin_path(HDC hdc)
{
    if (!hdc) return FALSE;
    return TRUE;
}

BOOL ios_end_path(HDC hdc)
{
    if (!hdc) return FALSE;
    return TRUE;
}

BOOL ios_close_figure(HDC hdc)
{
    if (!hdc) return FALSE;
    return TRUE;
}

/*
 * Graphics mode
 */

int ios_set_graphics_mode(HDC hdc, int mode)
{
    if (!hdc) return GM_ERROR;
    WINE_DC *dc = (WINE_DC *)hdc;
    int old = dc->map_mode;
    dc->map_mode = mode;
    return old;
}

int ios_get_graphics_mode(HDC hdc)
{
    if (!hdc) return GM_ERROR;
    WINE_DC *dc = (WINE_DC *)hdc;
    return dc->map_mode;
}

/*
 * Palette functions
 */

HPALETTE ios_create_palette(const void *logpalette)
{
    return (HPALETTE)0x10000000;
}

HPALETTE ios_select_palette(HDC hdc, HPALETTE palette, BOOL bkgnd)
{
    if (!hdc) return NULL;
    WINE_DC *dc = (WINE_DC *)hdc;
    
    HPALETTE old = dc->palette;
    dc->palette = palette;
    
    return old;
}

UINT ios_realize_palette(HDC hdc)
{
    if (!hdc) return GDI_ERROR;
    return 0;
}

/*
 * Font functions
 */

HFONT ios_create_font(const void *logfont)
{
    TRACE("ios_create_font()");
    return (HFONT)0x20000000;
}

HFONT ios_create_font_indirect(const void *logfont)
{
    TRACE("ios_create_font_indirect()");
    return (HFONT)0x20000000;
}

/*
 * Region functions
 */

HRGN ios_create_rect_rgn(int left, int top, int right, int bottom)
{
    TRACE("ios_create_rect_rgn(%d, %d, %d, %d)", left, top, right, bottom);
    
    HRGN rgn = malloc(sizeof(RECT));
    if (rgn) {
        RECT *rect = (RECT *)rgn;
        rect->left = left;
        rect->top = top;
        rect->right = right;
        rect->bottom = bottom;
    }
    
    return rgn;
}

HRGN ios_create_elliptic_rgn(int left, int top, int right, int bottom)
{
    return ios_create_rect_rgn(left, top, right, bottom);
}

HRGN ios_create_polygon_rgn(const POINT *points, int count, int mode)
{
    if (!points || count <= 0) return NULL;
    
    /* Store polygon points in region (simplified) */
    HRGN rgn = malloc(sizeof(POINT) * count + sizeof(int));
    if (rgn) {
        POINT *pts = (POINT *)(rgn + sizeof(int));
        memcpy(pts, points, sizeof(POINT) * count);
        *(int *)rgn = count;
    }
    
    return rgn;
}

int ios_combine_rgn(HRGN dest, HRGN src1, HRGN src2, int mode)
{
    return SIMPLEREGION;
}

/*
 * Metal-specific functions
 */

#if HAVE_METAL

id<MTLDevice> ios_metal_get_device(void)
{
    if (!g_metal_device) {
        g_metal_device = MTLCreateSystemDefaultDevice();
    }
    return g_metal_device;
}

id<MTLCommandQueue> ios_metal_get_command_queue(void)
{
    if (!g_metal_queue && g_metal_device) {
        g_metal_queue = [g_metal_device newCommandQueue];
    }
    return g_metal_queue;
}

void ios_metal_present(CAMetalLayer *layer)
{
    if (!layer || !g_metal_queue) return;
    
    id<CAMetalDrawable> drawable = layer.nextDrawable;
    if (!drawable) return;
    
    id<MTLCommandBuffer> cmd = [g_metal_queue commandBuffer];
    [cmd presentDrawable:drawable];
    [cmd commit];
}

#endif

/*
 * Initialization
 */

int ios_graphics_init(void)
{
    TRACE("ios_graphics_init()");
    
#if HAVE_METAL
    g_metal_device = MTLCreateSystemDefaultDevice();
    if (g_metal_device) {
        g_metal_queue = [g_metal_device newCommandQueue];
        TRACE("Metal initialized: %s", [[g_metal_device name] UTF8String]);
    } else {
        WARN("Metal not available");
    }
#else
    WARN("Metal not available (header not found)");
#endif
    
    return 0;
}

void ios_graphics_cleanup(void)
{
    TRACE("ios_graphics_cleanup()");
    
    /* Cleanup surfaces */
    pthread_mutex_lock(&g_surface_mutex);
    for (int i = 0; i < g_surface_count; i++) {
        if (g_surface_cache[i]) {
            ios_surface_destroy(g_surface_cache[i]);
            g_surface_cache[i] = NULL;
        }
    }
    g_surface_count = 0;
    pthread_mutex_unlock(&g_surface_mutex);
    
#if HAVE_METAL
    if (g_metal_queue) {
        [g_metal_queue release];
        g_metal_queue = nil;
    }
    if (g_metal_device) {
        [g_metal_device release];
        g_metal_device = nil;
    }
#endif
}
