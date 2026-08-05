/*
 * iOS PE Loader Implementation
 *
 * Copyright 2024 Wine Project
 *
 * Simplified PE loader for loading Windows executables on iOS.
 * This implementation supports basic PE loading for demonstration.
 */

#include "ios_pe.h"
#include "ios_syscalls.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#define TRACE_PE 1

#if TRACE_PE
#define TRACE(fmt, ...) fprintf(stderr, "[ios-pe] " fmt "\n", ##__VA_ARGS__)
#else
#define TRACE(fmt, ...) do {} while(0)
#endif

#define WARN(fmt, ...) fprintf(stderr, "[ios-pe:warn] " fmt "\n", ##__VA_ARGS__)
#define ERR(fmt, ...) fprintf(stderr, "[ios-pe:err] " fmt "\n", ##__VA_ARGS__)

/* Maximum number of sections */
#define MAX_SECTIONS 16

/* Maximum number of DLLs */
#define MAX_DLLS 256

/* Loaded DLLs cache */
static struct {
    char path[256];
    PE_DLL *dll;
} g_dll_cache[MAX_DLLS];
static int g_num_dlls = 0;
static pthread_mutex_t g_dll_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * PE Loading functions
 */

PE_IMAGE *ios_pe_load_from_memory(const void *data, size_t size)
{
    TRACE("ios_pe_load_from_memory(data=%p, size=%zu)", data, size);
    
    if (!data || size < sizeof(PE_DOS_HEADER) + sizeof(PE_FILE_HEADER)) {
        ERR("Invalid PE data");
        return NULL;
    }
    
    const uint8_t *bytes = (const uint8_t *)data;
    
    /* Check DOS header */
    const PE_DOS_HEADER *dos_hdr = (const PE_DOS_HEADER *)data;
    if (dos_hdr->signature[0] != 'M' || dos_hdr->signature[1] != 'Z') {
        ERR("Invalid DOS signature");
        return NULL;
    }
    
    /* Get PE header offset */
    uint32_t pe_offset = dos_hdr->pe_offset;
    if (pe_offset >= size || pe_offset < sizeof(PE_DOS_HEADER)) {
        ERR("Invalid PE offset: %u", pe_offset);
        return NULL;
    }
    
    /* Check PE signature */
    const PE_FILE_HEADER *file_hdr = (const PE_FILE_HEADER *)(bytes + pe_offset);
    if (file_hdr->signature[0] != 'P' || file_hdr->signature[1] != 'E' ||
        file_hdr->signature[2] != 0 || file_hdr->signature[3] != 0) {
        ERR("Invalid PE signature");
        return NULL;
    }
    
    TRACE("Machine: 0x%04x, Sections: %d", file_hdr->machine, file_hdr->num_sections);
    
    /* Check supported architectures */
    if (file_hdr->machine != PE_MACHINE_ARM64 && 
        file_hdr->machine != PE_MACHINE_ARM &&
        file_hdr->machine != PE_MACHINE_I386) {
        ERR("Unsupported machine type: 0x%04x", file_hdr->machine);
        return NULL;
    }
    
    /* Get optional header */
    const PE_OPTIONAL_HEADER *opt_hdr = (const PE_OPTIONAL_HEADER *)(file_hdr + 1);
    
    /* Validate optional header */
    if (file_hdr->opt_hdr_size < sizeof(PE_OPTIONAL_HEADER)) {
        ERR("Optional header too small");
        return NULL;
    }
    
    /* Allocate PE image structure */
    PE_IMAGE *image = calloc(1, sizeof(PE_IMAGE));
    if (!image) {
        ERR("Failed to allocate PE_IMAGE");
        return NULL;
    }
    
    image->refcount = 1;
    image->machine = file_hdr->machine;
    image->subsystem = opt_hdr->subsystem;
    image->entry_point = opt_hdr->entry_point;
    image->code_base = opt_hdr->code_base;
    image->data_base = opt_hdr->data_base;
    image->image_size = opt_hdr->image_size;
    
    /* Get section headers */
    const uint8_t *section_hdrs = (const uint8_t *)(opt_hdr) + file_hdr->opt_hdr_size;
    image->num_sections = file_hdr->num_sections;
    
    if (image->num_sections > MAX_SECTIONS) {
        WARN("Too many sections (%d), limiting to %d", image->num_sections, MAX_SECTIONS);
        image->num_sections = MAX_SECTIONS;
    }
    
    /* Allocate section data */
    image->section_data = calloc(image->num_sections, sizeof(void *));
    image->section_rva = calloc(image->num_sections, sizeof(uint32_t));
    image->section_size = calloc(image->num_sections, sizeof(uint32_t));
    
    /* Copy section headers */
    for (int i = 0; i < image->num_sections; i++) {
        const PE_SECTION_HEADER *sec = (const PE_SECTION_HEADER *)section_hdrs + i;
        memcpy(&image->section_rva[i], &sec->virtual_address, sizeof(uint32_t));
        memcpy(&image->section_size[i], &sec->virtual_size, sizeof(uint32_t));
        
        TRACE("Section %d: RVA=0x%08x, VSize=%d, RawSize=%d, RawOffset=%d",
              i, sec->virtual_address, sec->virtual_size, sec->raw_size, sec->raw_offset);
    }
    
    /* Get data directories */
    const PE_DATA_DIRECTORY *dirs = (const PE_DATA_DIRECTORY *)((const uint8_t *)opt_hdr + 
                                                                 sizeof(PE_OPTIONAL_HEADER));
    image->num_directories = (file_hdr->opt_hdr_size - sizeof(PE_OPTIONAL_HEADER)) / sizeof(PE_DATA_DIRECTORY);
    if (image->num_directories > 16) image->num_directories = 16;
    image->directories = (PE_DATA_DIRECTORY *)dirs;
    
    TRACE("Image loaded: entry=0x%08x, image_size=%d, subsystem=%d",
          image->entry_point, image->image_size, image->subsystem);
    
    return image;
}

PE_IMAGE *ios_pe_load_from_file(const char *path)
{
    TRACE("ios_pe_load_from_file(%s)", path);
    
    /* Open file */
    int fd = ios_open(path, O_RDONLY, 0);
    if (fd < 0) {
        ERR("Failed to open %s: %s", path, strerror(errno));
        return NULL;
    }
    
    /* Get file size */
    off_t size = ios_file_size(fd);
    if (size < 0) {
        ERR("Failed to get file size");
        ios_close(fd);
        return NULL;
    }
    
    /* Read file into memory */
    void *data = malloc(size);
    if (!data) {
        ERR("Failed to allocate memory for PE file");
        ios_close(fd);
        return NULL;
    }
    
    ssize_t read_size = ios_read(fd, data, size);
    ios_close(fd);
    
    if (read_size != size) {
        ERR("Failed to read PE file: read %zd of %ld", read_size, (long)size);
        free(data);
        return NULL;
    }
    
    /* Load from memory */
    PE_IMAGE *image = ios_pe_load_from_memory(data, size);
    if (!image) {
        free(data);
        return NULL;
    }
    
    /* Allocate image memory */
    uint64_t image_base = 0x100000000ULL;  /* 4GB base for 64-bit */
    image->base_address = ios_mmap((void *)image_base, image->image_size,
                                    IOS_PROT_RWX, IOS_MAP_PRIVATE | IOS_MAP_ANONYMOUS, -1, 0);
    
    if (!image->base_address) {
        /* Try without fixed address */
        image->base_address = ios_mmap(NULL, image->image_size,
                                       IOS_PROT_RWX, IOS_MAP_PRIVATE | IOS_MAP_ANONYMOUS, -1, 0);
    }
    
    if (!image->base_address) {
        ERR("Failed to allocate memory for PE image");
        ios_pe_unload(image);
        free(data);
        return NULL;
    }
    
    TRACE("Image allocated at %p", image->base_address);
    
    /* Note: Full PE loading would copy sections and apply relocations here */
    /* This is a simplified implementation */
    
    return image;
}

void ios_pe_unload(PE_IMAGE *image)
{
    if (!image) return;
    
    TRACE("ios_pe_unload(%p)", image);
    
    image->refcount--;
    if (image->refcount > 0) return;
    
    /* Unmap image memory */
    if (image->base_address) {
        ios_munmap(image->base_address, image->image_size);
    }
    
    /* Free section arrays */
    if (image->section_data) free(image->section_data);
    if (image->section_rva) free(image->section_rva);
    if (image->section_size) free(image->section_size);
    
    free(image);
}

void *ios_pe_get_entry_point(PE_IMAGE *image)
{
    if (!image) return NULL;
    
    if (!image->base_address) return NULL;
    
    return (uint8_t *)image->base_address + image->entry_point;
}

/*
 * PE Execution functions
 */

int ios_pe_execute(PE_IMAGE *image, int argc, char **argv, char **envp)
{
    TRACE("ios_pe_execute(%p, argc=%d)", image, argc);
    
    if (!image) return -1;
    
    void *entry = ios_pe_get_entry_point(image);
    if (!entry) {
        ERR("No entry point");
        return -1;
    }
    
    /* For ARM64, entry would be called as a function pointer */
    /* This is architecture-specific and would need proper calling conventions */
    
    TRACE("Would execute at %p", entry);
    
    return 0;
}

int ios_pe_create_process(const char *path, int argc, char **argv, char **envp)
{
    TRACE("ios_pe_create_process(%s, argc=%d)", path, argc);
    
    PE_IMAGE *image = ios_pe_load_from_file(path);
    if (!image) {
        ERR("Failed to load PE file: %s", path);
        return -1;
    }
    
    int result = ios_pe_execute(image, argc, argv, envp);
    
    ios_pe_unload(image);
    
    return result;
}

/*
 * DLL Loading functions
 */

PE_DLL *ios_dll_load(const char *path)
{
    TRACE("ios_dll_load(%s)", path);
    
    pthread_mutex_lock(&g_dll_mutex);
    
    /* Check cache */
    for (int i = 0; i < g_num_dlls; i++) {
        if (strcmp(g_dll_cache[i].path, path) == 0) {
            pthread_mutex_unlock(&g_dll_mutex);
            g_dll_cache[i].dll->refcount++;
            return g_dll_cache[i].dll;
        }
    }
    
    pthread_mutex_unlock(&g_dll_mutex);
    
    /* Load DLL file */
    int fd = ios_open(path, O_RDONLY, 0);
    if (fd < 0) {
        ERR("Failed to open DLL: %s", path);
        return NULL;
    }
    
    off_t size = ios_file_size(fd);
    void *data = malloc(size);
    if (!data) {
        ios_close(fd);
        return NULL;
    }
    
    ios_read(fd, data, size);
    ios_close(fd);
    
    PE_IMAGE *image = ios_pe_load_from_memory(data, size);
    free(data);
    
    if (!image) {
        return NULL;
    }
    
    PE_DLL *dll = calloc(1, sizeof(PE_DLL));
    if (!dll) {
        ios_pe_unload(image);
        return NULL;
    }
    
    dll->image = image;
    dll->refcount = 1;
    
    /* Add to cache */
    pthread_mutex_lock(&g_dll_mutex);
    if (g_num_dlls < MAX_DLLS) {
        strncpy(g_dll_cache[g_num_dlls].path, path, sizeof(g_dll_cache[0].path) - 1);
        g_dll_cache[g_num_dlls].dll = dll;
        g_num_dlls++;
    }
    pthread_mutex_unlock(&g_dll_mutex);
    
    return dll;
}

void ios_dll_free(PE_DLL *dll)
{
    if (!dll) return;
    
    dll->refcount--;
    if (dll->refcount > 0) return;
    
    if (dll->exports) free(dll->exports);
    ios_pe_unload(dll->image);
    free(dll);
}

void *ios_dll_get_proc(PE_DLL *dll, const char *name)
{
    if (!dll || !name) return NULL;
    
    /* Would look up export table here */
    TRACE("ios_dll_get_proc(%s) - export lookup not implemented", name);
    
    return NULL;
}

/*
 * PE Image information
 */

bool ios_pe_is_valid(const void *data, size_t size)
{
    if (!data || size < sizeof(PE_DOS_HEADER)) return false;
    
    const PE_DOS_HEADER *dos_hdr = (const PE_DOS_HEADER *)data;
    if (dos_hdr->signature[0] != 'M' || dos_hdr->signature[1] != 'Z') return false;
    
    uint32_t pe_offset = dos_hdr->pe_offset;
    if (pe_offset >= size) return false;
    
    const uint8_t *bytes = (const uint8_t *)data;
    const PE_FILE_HEADER *file_hdr = (const PE_FILE_HEADER *)(bytes + pe_offset);
    
    if (file_hdr->signature[0] != 'P' || file_hdr->signature[1] != 'E') return false;
    
    return true;
}

const char *ios_pe_get_subsystem_name(uint16_t subsystem)
{
    switch (subsystem) {
        case PE_SUBSYSTEM_WINDOWS_CUI: return "Console";
        case PE_SUBSYSTEM_WINDOWS_GUI: return "GUI";
        default: return "Unknown";
    }
}

const char *ios_pe_get_machine_name(uint16_t machine)
{
    switch (machine) {
        case PE_MACHINE_ARM:    return "ARM";
        case PE_MACHINE_ARM64:  return "ARM64";
        case PE_MACHINE_I386:   return "x86";
        case PE_MACHINE_X64:    return "x64";
        default: return "Unknown";
    }
}

/*
 * Address translation
 */

uint32_t ios_pe_rva_to_offset(PE_IMAGE *image, uint32_t rva)
{
    if (!image) return 0;
    
    for (int i = 0; i < image->num_sections; i++) {
        uint32_t sec_rva = image->section_rva[i];
        uint32_t sec_size = image->section_size[i];
        
        if (rva >= sec_rva && rva < sec_rva + sec_size) {
            /* Would calculate actual file offset based on section raw data */
            return rva - sec_rva;  /* Simplified */
        }
    }
    
    return 0;
}

uint32_t ios_pe_offset_to_rva(PE_IMAGE *image, uint32_t offset)
{
    if (!image) return 0;
    
    /* Simplified implementation */
    return offset;
}

void *ios_pe_rva_to_pointer(PE_IMAGE *image, uint32_t rva)
{
    if (!image || !image->base_address) return NULL;
    
    return (uint8_t *)image->base_address + rva;
}

/*
 * PE Initialization
 */

int ios_pe_init(void)
{
    TRACE("ios_pe_init()");
    return 0;
}

void ios_pe_cleanup(void)
{
    TRACE("ios_pe_cleanup()");
    
    pthread_mutex_lock(&g_dll_mutex);
    for (int i = 0; i < g_num_dlls; i++) {
        if (g_dll_cache[i].dll) {
            ios_dll_free(g_dll_cache[i].dll);
            g_dll_cache[i].dll = NULL;
        }
    }
    g_num_dlls = 0;
    pthread_mutex_unlock(&g_dll_mutex);
}
