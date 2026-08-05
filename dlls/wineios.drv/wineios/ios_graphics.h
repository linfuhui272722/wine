/*
 * iOS Metal Graphics Backend
 *
 * Copyright 2024 Wine Project
 *
 * Implements GDI/GDI+ functions using Apple's Metal framework.
 * This allows Wine to render Windows graphics on iOS devices.
 *
 * Note: This is a pure C header. The actual Metal implementation
 * is in the Objective-C files (WineViewController.m).
 */

#ifndef __WINE_IOS_GRAPHICS_H
#define __WINE_IOS_GRAPHICS_H

#include <stdint.h>
#include <stdbool.h>

/* Define opaque types for Metal objects - actual definitions in ObjC */
typedef struct __IOS_MTL_Device *IOS_MTL_Device;
typedef struct __IOS_MTL_CommandQueue *IOS_MTL_CommandQueue;
typedef struct __IOS_MTL_Texture *IOS_MTL_Texture;
typedef struct __IOS_MTL_Buffer *IOS_MTL_Buffer;
typedef struct __IOS_CA_MetalLayer *IOS_CA_MetalLayer;

/* Wine-compatible Windows types */
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef void *HANDLE;
typedef const uint16_t *WCHAR;
typedef uint8_t BYTE;
typedef void *LPVOID;
typedef const void *LPCVOID;

/* BLENDFUNCTION for AlphaBlend */
typedef struct {
    uint8_t BlendOp;
    uint8_t BlendFlags;
    uint8_t SourceConstantAlpha;
    uint8_t AlphaFormat;
} BLENDFUNCTION;

/* GDI constants */
#define ALTERNATE 1
#define WINDING 2
#define BLACKONWHITE 1
#define WHITEONBLACK 2
#define COLORONCOLOR 3
#define HALFTONE 4
#define MAXSTRETCHBLTMODE 4

#define AD_COUNTERCLOCKWISE 1
#define AD_CLOCKWISE 2

#define ANSI_CHARSET 0
#define DEFAULT_CHARSET 1
#define SYMBOL_CHARSET 2
#define SHIFTJIS_CHARSET 128
#define HANGUL_CHARSET 129
#define GB2312_CHARSET 134
#define CHINESEBIG5_CHARSET 136
#define OEM_CHARSET 255

#define ERROR 0
#define NULLREGION 1
#define SIMPLEREGION 2
#define COMPLEXREGION 3

#define GM_ERROR 0
#define GM_COMPATIBLE 1
#define GM_ADVANCED 2

#define GDI_ERROR ((INT)-1)

#define CLR_INVALID 0xFFFFFFFF

/* Metal feature detection */
#define HAVE_METAL 1

/* Wine-compatible types */
typedef void *HDC;
typedef void *HBITMAP;
typedef void *HPEN;
typedef void *HBRUSH;
typedef void *HFONT;
typedef void *HRGN;
typedef void *HPALETTE;
typedef void *HICON;
typedef void *HCURSOR;

typedef struct {
    int x, y;
} POINT;

typedef struct {
    int cx, cy;
} SIZE;

typedef struct {
    int left, top, right, bottom;
} RECT;

typedef uint32_t COLORREF;
typedef uint32_t DWORD;
/* BOOL is defined by objc/objc.h when compiling ObjC */
#ifndef __OBJC__
typedef int BOOL;
#endif
typedef int INT;
typedef unsigned int UINT;
typedef float FLOAT;
typedef void *PVOID;

/* Pixel formats */
typedef enum {
    DIB_PAL_COLORS = 1,
    DIB_RGB_COLORS = 2
} DIB_PALETTE_TYPE;

/* Raster operation codes */
typedef enum {
    SRCCOPY = 0x00CC0020,
    SRCPAINT = 0x00EE0086,
    SRCAND = 0x008800C6,
    SRCINVERT = 0x00660046,
    SRCERASE = 0x00440328,
    NOTSRCCOPY = 0x00330088,
    NOTSRCERASE = 0x001100A6,
    MERGECOPY = 0x00C000CA,
    MERGEPAINT = 0x00BB0226,
    PATCOPY = 0x00F00021,
    PATPAINT = 0x00FB0A09,
    PATINVERT = 0x005A0049,
    DSTINVERT = 0x00550009,
    BLACKNESS = 0x00000042,
    WHITENESS = 0x00FF0062
} RASTER_OPERATION;

/* Binary raster operations */
typedef enum {
    R2_BLACK = 1,
    R2_NOTMERGEPEN = 2,
    R2_MASKNOTPEN = 3,
    R2_NOTCOPYPEN = 4,
    R2_MASKPENNOT = 5,
    R2_INVERT = 6,
    R2_XORPEN = 7,
    R2_NOTMASKPEN = 8,
    R2_MASKPEN = 9,
    R2_NOTXORPEN = 10,
    R2_NOP = 11,
    R2_MERGENOTPEN = 12,
    R2_COPYPEN = 13,
    R2_MERGEPENNOT = 14,
    R2_MERGEPEN = 15,
    R2_WHITE = 16
} BINARY_ROP;

/* Ternary raster operations */
#define SRCCPY     0x00CC0020L
#define SRCPAINT   0x00EE0086L
#define SRCAND     0x008800C6L
#define SRCINVERT  0x00660046L
#define SRCERASE   0x00440328L
#define NOTSRCCOPY 0x00330088L
#define NOTSRCERASE 0x001100A6L
#define MERGECOPY  0x00C000CAL
#define MERGEPAINT 0x00BB0226L
#define PATCOPY    0x00F00021L
#define PATPAINT   0x00FB0A09L
#define PATINVERT  0x005A0049L
#define DSTINVERT  0x00550009L
#define BLACKNESS  0x00000042L
#define WHITENESS  0x00FF0062L

/* Graphics modes (for reference - used as integers) */
#define GM_COMPATIBLE_VAL 1
#define GM_ADVANCED_VAL 2

/* Background modes */
typedef enum {
    TRANSPARENT = 1,
    OPAQUE = 2
} BG_MODE;

/* Mapping modes */
typedef enum {
    MM_TEXT = 1,
    MM_LOMETRIC = 2,
    MM_HIMETRIC = 3,
    MM_LOENGLISH = 4,
    MM_HIENGLISH = 5,
    MM_TWIPS = 6,
    MM_ISOTROPIC = 7,
    MM_ANISOTROPIC = 8
} MM;

/* Stock objects */
#define WHITE_BRUSH     0
#define LTGRAY_BRUSH    1
#define GRAY_BRUSH      2
#define DKGRAY_BRUSH    3
#define BLACK_BRUSH     4
#define NULL_BRUSH      5
#define HOLLOW_BRUSH    5
#define WHITE_PEN       6
#define BLACK_PEN       7
#define NULL_PEN        8
#define OEM_FIXED_FONT  10
#define ANSI_FIXED_FONT 11
#define ANSI_VAR_FONT   12
#define SYSTEM_FONT     13
#define DEFAULT_PALETTE 15

/* Common colors */
#define CLR_INVALID     0xFFFFFFFF
#define RGB(r,g,b)      ((uint32_t)(((uint8_t)(r)|((uint16_t)((uint8_t)(g))<<8))|(((uint32_t)(uint8_t)(b))<<16)))
#define GetRValue(rgb)  ((uint8_t)(rgb))
#define GetGValue(rgb)  ((uint8_t)(((uint16_t)(rgb)) >> 8))
#define GetBValue(rgb)  ((uint8_t)((rgb)>>16))

/* Surface types */
typedef enum {
    SURFACE_TYPE_DIB,      /* Device-independent bitmap */
    SURFACE_TYPE_DC,       /* Device context */
    SURFACE_TYPE_METADC,    /* Metafile DC */
    SURFACE_TYPE_OTHER      /* Other type */
} SURFACE_TYPE;

/* Wine surface structure */
typedef struct wine_surface {
    SURFACE_TYPE type;
    int width;
    int height;
    int stride;
    void *data;           /* Pixel data */
    bool dirty;            /* Needs redraw */
    
    /* Metal resources for GPU rendering (opaque handles) */
    IOS_MTL_Texture texture;
    IOS_MTL_Buffer buffer;
    
    /* Palette (if applicable) */
    HPALETTE palette;
    
    /* Reference count */
    int refcount;
} WINE_SURFACE;

/* Wine DC structure */
typedef struct wine_dc {
    /* Surface being drawn to */
    WINE_SURFACE *surface;
    
    /* Clipping region */
    HRGN clip_region;
    
    /* Current positions */
    POINT cur_pos;
    POINT brush_origin;
    
    /* Current objects */
    HPEN pen;
    HBRUSH brush;
    HBITMAP bitmap;
    HFONT font;
    HPALETTE palette;
    
    /* Object selection flags */
    BOOL own_pen;
    BOOL own_brush;
    BOOL own_bitmap;
    BOOL own_font;
    
    /* Drawing modes */
    BINARY_ROP rop2;
    BG_MODE bg_mode;
    MM map_mode;
    
    /* Miter limit */
    FLOAT miter_limit;
    
    /* Viewport and window extents */
    SIZE viewport_ext;
    POINT viewport_org;
    SIZE window_ext;
    POINT window_org;
    
    /* Arc directions */
    INT arc_direction;
    
    /* Charset */
    UINT charset;
    
    /* Layout (for BiDi) */
    DWORD layout;
    
    /* DC attributes */
    COLORREF text_color;
    COLORREF bg_color;
    INT poly_fill_mode;
    INT stretch_blt_mode;
    
    /* Reference count */
    int refcount;
} WINE_DC;

/*
 * Surface functions
 */

/* Create a new surface */
WINE_SURFACE *ios_surface_create(int width, int height, int bpp, const void *data);

/* Create a surface from a DIB section */
WINE_SURFACE *ios_surface_create_dib(int width, int height, int bpp, void **bits);

/* Create a surface from raw image data (BGRA format) */
WINE_SURFACE *ios_surface_create_from_image(int width, int height, const void *data);

/* Destroy a surface */
void ios_surface_destroy(WINE_SURFACE *surface);

/* Get surface dimensions */
void ios_surface_get_size(WINE_SURFACE *surface, int *width, int *height);

/* Get surface pixel data */
void *ios_surface_get_data(WINE_SURFACE *surface);

/* Get surface stride */
int ios_surface_get_stride(WINE_SURFACE *surface);

/* Lock surface for direct access */
BOOL ios_surface_lock(WINE_SURFACE *surface);

/* Unlock surface */
void ios_surface_unlock(WINE_SURFACE *surface);

/* Mark surface as dirty */
void ios_surface_mark_dirty(WINE_SURFACE *surface);

/* Get surface data pointer */
void *ios_surface_get_pixels(WINE_SURFACE *surface);

/*
 * Device Context functions
 */

/* Create a memory DC */
HDC ios_create_dc(void);

/* Create a compatible DC */
HDC ios_create_compatible_dc(HDC hdc);

/* Delete a DC */
BOOL ios_delete_dc(HDC hdc);

/* Get DC surface */
WINE_SURFACE *ios_dc_get_surface(HDC hdc);

/* Select a surface into DC */
HBITMAP ios_select_surface(HDC hdc, WINE_SURFACE *surface);

/*
 * Drawing functions
 */

/* Set pixel */
COLORREF ios_set_pixel(HDC hdc, int x, int y, COLORREF color);

/* Get pixel */
COLORREF ios_get_pixel(HDC hdc, int x, int y);

/* Move to position */
BOOL ios_move_to(HDC hdc, int x, int y, POINT *old_point);

/* Line to */
BOOL ios_line_to(HDC hdc, int x, int y);

/* Polyline */
BOOL ios_polyline(HDC hdc, const POINT *points, int count);

/* Polyline to */
BOOL ios_polyline_to(HDC hdc, const POINT *points, int count);

/* PolyBezier */
BOOL ios_poly_bezier(HDC hdc, const POINT *points, int count);

/* Rectangle */
BOOL ios_rectangle(HDC hdc, int left, int top, int right, int bottom);

/* RoundRect */
BOOL ios_round_rect(HDC hdc, int left, int top, int right, int bottom,
                    int ellipsis_width, int ellipsis_height);

/* Ellipse */
BOOL ios_ellipse(HDC hdc, int left, int top, int right, int bottom);

/* Arc */
BOOL ios_arc(HDC hdc, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4);

/* Chord */
BOOL ios_chord(HDC hdc, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4);

/* Pie */
BOOL ios_pie(HDC hdc, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4);

/* FloodFill */
BOOL ios_flood_fill(HDC hdc, int x, int y, COLORREF color);

/* ExtFloodFill */
BOOL ios_ext_flood_fill(HDC hdc, int x, int y, COLORREF color, UINT fill_type);

/* FillRect */
int ios_fill_rect(HDC hdc, const RECT *rect, HBRUSH brush);

/* FrameRect */
int ios_frame_rect(HDC hdc, const RECT *rect, HBRUSH brush);

/* InvertRect */
BOOL ios_invert_rect(HDC hdc, const RECT *rect);

/* Draw edge */
BOOL ios_draw_edge(HDC hdc, RECT *rect, UINT edge, UINT flags);

/*
 * Bitmap functions
 */

/* CreateBitmap */
HBITMAP ios_create_bitmap(int width, int height, UINT planes, UINT bpp, const void *bits);

/* CreateCompatibleBitmap */
HBITMAP ios_create_compatible_bitmap(HDC hdc, int width, int height);

/* CreateDIBSection */
HBITMAP ios_create_dib_section(HDC hdc, int width, int height, int bpp, 
                                void **bits, HANDLE section, DWORD offset);

/* GetDIBits */
int ios_get_dibits(HDC hdc, HBITMAP bitmap, UINT start_scan, UINT scan_lines,
                   void *bits, void *info, UINT usage);

/* SetDIBits */
int ios_set_dibits(HDC hdc, HBITMAP bitmap, UINT start_scan, UINT scan_lines,
                   const void *bits, const void *info, UINT usage);

/* StretchDIBits */
int ios_stretch_dibits(HDC hdc, int dst_x, int dst_y, int dst_width, int dst_height,
                       int src_x, int src_y, int src_width, int src_height,
                       const void *bits, const void *info, UINT usage, DWORD rop);

/*
 * Blt functions
 */

/* BitBlt */
BOOL ios_bit_blt(HDC hdc_dst, int x_dst, int y_dst, int width, int height,
                 HDC hdc_src, int x_src, int y_src, DWORD rop);

/* StretchBlt */
BOOL ios_stretch_blt(HDC hdc_dst, int x_dst, int y_dst, int width, int height,
                     HDC hdc_src, int x_src, int y_src, int src_width, int src_height,
                     DWORD rop);

/* PatBlt */
BOOL ios_pat_blt(HDC hdc, int x, int y, int width, int height, DWORD rop);

/* MaskBlt */
BOOL ios_mask_blt(HDC hdc, int x, int y, int width, int height,
                  HDC hdc_src, int x_src, int y_src,
                  HBITMAP mask, int x_mask, int y_mask,
                  DWORD rop);

/* TransparentBlt */
BOOL ios_transparent_blt(HDC hdc_dst, int x_dst, int y_dst, int width, int height,
                          HDC hdc_src, int x_src, int y_src, int src_width, int src_height,
                          UINT transparent_color);

/*
 * Brush functions
 */

/* CreateSolidBrush */
HBRUSH ios_create_solid_brush(COLORREF color);

/* CreateHatchBrush */
HBRUSH ios_create_hatch_brush(int style, COLORREF color);

/* CreatePatternBrush */
HBRUSH ios_create_pattern_brush(HBITMAP bitmap);

/* CreateDIBPatternBrush */
HBRUSH ios_create_dib_pattern_brush(const void *info, UINT usage);

/* GetBrushOrgEx */
BOOL ios_get_brush_org_ex(HDC hdc, POINT *point);

/* SetBrushOrgEx */
BOOL ios_set_brush_org_ex(HDC hdc, int x, int y, POINT *old_point);

/*
 * Pen functions
 */

/* CreatePen */
HPEN ios_create_pen(int style, int width, COLORREF color);

/* CreatePenIndirect */
HPEN ios_create_pen_indirect(const void *logpen);

/* GetPolyFillMode */
int ios_get_poly_fill_mode(HDC hdc);

/* SetPolyFillMode */
int ios_set_poly_fill_mode(HDC hdc, int mode);

/*
 * Font functions
 */

/* CreateFont */
HFONT ios_create_font(const void *logfont);

/* CreateFontIndirect */
HFONT ios_create_font_indirect(const void *logfont);

/* GetFontData */
DWORD ios_get_font_data(HDC hdc, DWORD table, DWORD offset, void *buf, DWORD count);

/* GetGlyphIndices */
DWORD ios_get_glyph_indices(HDC hdc, const WCHAR *str, int count, WORD *indices, DWORD flags);

/* GetTextMetrics */
BOOL ios_get_text_metrics(HDC hdc, void *metrics);

/*
 * Region functions
 */

/* CreateRectRgn */
HRGN ios_create_rect_rgn(int left, int top, int right, int bottom);

/* CreateEllipticRgn */
HRGN ios_create_elliptic_rgn(int left, int top, int right, int bottom);

/* CreatePolygonRgn */
HRGN ios_create_polygon_rgn(const POINT *points, int count, int mode);

/* CombineRgn */
int ios_combine_rgn(HRGN dest, HRGN src1, HRGN src2, int mode);

/* FillRgn */
BOOL ios_fill_rgn(HDC hdc, HRGN rgn, HBRUSH brush);

/* FrameRgn */
BOOL ios_frame_rgn(HDC hdc, HRGN rgn, HBRUSH brush, int width, int height);

/* InvertRgn */
BOOL ios_invert_rgn(HDC hdc, HRGN rgn);

/* PaintRgn */
BOOL ios_paint_rgn(HDC hdc, HRGN rgn);

/*
 * Clipping functions
 */

/* SelectClipRgn */
int ios_select_clip_rgn(HDC hdc, HRGN rgn);

/* GetClipRgn */
int ios_get_clip_rgn(HDC hdc, HRGN rgn);

/* SetMetaRgn */
int ios_set_meta_rgn(HDC hdc);

/* ExcludeClipRect */
int ios_exclude_clip_rect(HDC hdc, int left, int top, int right, int bottom);

/* IntersectClipRect */
int ios_intersect_clip_rect(HDC hdc, int left, int top, int right, int bottom);

/* OffsetClipRgn */
int ios_offset_clip_rgn(HDC hdc, int x, int y);

/*
 * Path functions
 */

/* BeginPath */
BOOL ios_begin_path(HDC hdc);

/* EndPath */
BOOL ios_end_path(HDC hdc);

/* AbortPath */
BOOL ios_abort_path(HDC hdc);

/* CloseFigure */
BOOL ios_close_figure(HDC hdc);

/* FillPath */
BOOL ios_fill_path(HDC hdc);

/* StrokePath */
BOOL ios_stroke_path(HDC hdc);

/* StrokeAndFillPath */
BOOL ios_stroke_and_fill_path(HDC hdc);

/* FillAndStrokePath */
BOOL ios_fill_and_stroke_path(HDC hdc);

/* WidenPath */
BOOL ios_widen_path(HDC hdc);

/* FlattenPath */
BOOL ios_flatten_path(HDC hdc);

/* GetPath */
int ios_get_path(HDC hdc, POINT *points, BYTE *types, int size);

/*
 * Palette functions
 */

/* CreatePalette */
HPALETTE ios_create_palette(const void *logpalette);

/* SelectPalette */
HPALETTE ios_select_palette(HDC hdc, HPALETTE palette, BOOL bkgnd);

/* RealizePalette */
UINT ios_realize_palette(HDC hdc);

/* UpdateColors */
BOOL ios_update_colors(HDC hdc);

/*
 * Color adjustment
 */

/* SetColorAdjustment */
BOOL ios_set_color_adjustment(HDC hdc, const void *ca);

/* GetColorAdjustment */
BOOL ios_get_color_adjustment(HDC hdc, void *ca);

/*
 * ICM (Image Color Management)
 */

/* SetICMMode */
int ios_set_icm_mode(HDC hdc, int mode);

/* CheckColorsInGamut */
BOOL ios_check_colors_in_gamut(HDC hdc, const COLORREF *colors, void *results, DWORD count);

/*
 * Graphics mode
 */

/* SetGraphicsMode */
int ios_set_graphics_mode(HDC hdc, int mode);

/* GetGraphicsMode */
int ios_get_graphics_mode(HDC hdc);

/*
 * Coordinate functions
 */

/* DPtoLP */
BOOL ios_dp_to_lp(HDC hdc, POINT *points, int count);

/* LPtoDP */
BOOL ios_lp_to_dp(HDC hdc, POINT *points, int count);

/* SetViewportOrgEx */
BOOL ios_set_viewport_org_ex(HDC hdc, int x, int y, POINT *old_point);

/* GetViewportOrgEx */
BOOL ios_get_viewport_org_ex(HDC hdc, POINT *point);

/* SetViewportExtEx */
BOOL ios_set_viewport_ext_ex(HDC hdc, int x, int y, SIZE *old_size);

/* GetViewportExtEx */
BOOL ios_get_viewport_ext_ex(HDC hdc, SIZE *size);

/* SetWindowOrgEx */
BOOL ios_set_window_org_ex(HDC hdc, int x, int y, POINT *old_point);

/* GetWindowOrgEx */
BOOL ios_get_window_org_ex(HDC hdc, POINT *point);

/* SetWindowExtEx */
BOOL ios_set_window_ext_ex(HDC hdc, int x, int y, SIZE *old_size);

/* GetWindowExtEx */
BOOL ios_get_window_ext_ex(HDC hdc, SIZE *size);

/* OffsetViewportOrgEx */
BOOL ios_offset_viewport_org_ex(HDC hdc, int x, int y, POINT *old_point);

/* OffsetWindowOrgEx */
BOOL ios_offset_window_org_ex(HDC hdc, int x, int y, POINT *old_point);

/* ScaleViewportExtEx */
BOOL ios_scale_viewport_ext_ex(HDC hdc, int x_num, int x_den, int y_num, int y_den, SIZE *old_size);

/* ScaleWindowExtEx */
BOOL ios_scale_window_ext_ex(HDC hdc, int x_num, int x_den, int y_num, int y_den, SIZE *old_size);

/*
 * Metafile functions
 */

/* PlayMetaFile */
BOOL ios_play_meta_file(HDC hdc, HANDLE hmf, const RECT *bounds);

/*
 * Alpha blend
 */
#if HAVE_METAL
/* AlphaBlend */
BOOL ios_alpha_blend(HDC hdc_dst, int x_dst, int y_dst, int width, int height,
                     HDC hdc_src, int x_src, int y_src, int src_width, int src_height,
                     BLENDFUNCTION blend_function);
#endif

/*
 * Gradient fill
 */

/* GradientFill triangle */
BOOL ios_gradient_fill_tri(HDC hdc, const void *vert_array, DWORD vert_count,
                           void *elements, DWORD element_count, DWORD mode);

/* GradientFill rectangle */
BOOL ios_gradient_fill_rect(HDC hdc, const void *vert_array, DWORD vert_count,
                           void *elements, DWORD element_count, DWORD mode);

/*
 * Initialization
 */

/* Initialize graphics subsystem */
int ios_graphics_init(void);

/* Cleanup graphics subsystem */
void ios_graphics_cleanup(void);

/*
 * Metal-specific functions (implemented in ObjC)
 */

/* Get Metal device */
IOS_MTL_Device ios_metal_get_device(void);

/* Get Metal command queue */
IOS_MTL_CommandQueue ios_metal_get_command_queue(void);

/* Present drawable */
void ios_metal_present(IOS_CA_MetalLayer *layer);

/*
 * Render to UIView
 */

/* Render surface to UIView */
void ios_render_to_view(void *ui_view, WINE_SURFACE *surface);

#endif /* __WINE_IOS_GRAPHICS_H */
