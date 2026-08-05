/*
 * iOS PE (Portable Executable) Loader
 *
 * Copyright 2024 Wine Project
 *
 * Loads and executes Windows PE (.exe, .dll) files on iOS.
 * This is a simplified loader for demonstration purposes.
 */

#ifndef __WINE_IOS_PE_H
#define __WINE_IOS_PE_H

#include <stdint.h>
#include <stdbool.h>

/* PE Header structures */
typedef struct {
    uint8_t  signature[2];       /* "MZ" */
    uint16_t last_size_mod;       /* Last size mod 512 */
    uint16_t num_blocks;          /* Number of blocks */
    uint16_t num_relocs;          /* Number of relocations */
    uint16_t hdr_size;            /* Header size in paragraphs */
    uint16_t min_extra_paras;     /* Min extra paragraphs */
    uint16_t max_extra_paras;     /* Max extra paragraphs */
    uint16_t ss;                  /* Stack segment */
    uint16_t sp;                  /* Stack pointer */
    uint16_t checksum;            /* Checksum */
    uint16_t ip;                  /* Instruction pointer */
    uint16_t cs;                  /* Code segment */
    uint16_t reloc_offset;        /* Relocation table offset */
    uint16_t overlay_num;         /* Overlay number */
    uint8_t  reserved[32];       /* Reserved */
    uint16_t oem_id;             /* OEM identifier */
    uint16_t oem_info;           /* OEM info */
    uint8_t  reserved2[20];      /* Reserved */
    uint32_t pe_offset;          /* Offset to PE header */
} PE_DOS_HEADER;

typedef struct {
    uint8_t  signature[4];        /* "PE\0\0" */
    uint16_t machine;             /* Machine type */
    uint16_t num_sections;        /* Number of sections */
    uint32_t time_date;          /* Time/date stamp */
    uint32_t sym_table_offset;   /* Symbol table offset */
    uint32_t num_symbols;        /* Number of symbols */
    uint16_t opt_hdr_size;       /* Optional header size */
    uint16_t characteristics;    /* Characteristics */
} PE_FILE_HEADER;

typedef struct {
    uint16_t magic;               /* Magic number (0x10b or 0x20b) */
    uint8_t  linker_version;      /* Linker version */
    uint32_t code_size;          /* Size of code */
    uint32_t data_size;          /* Size of initialized data */
    uint32_t bss_size;           /* Size of uninitialized data */
    uint32_t entry_point;        /* Entry point RVA */
    uint32_t code_base;          /* Base of code */
    uint32_t data_base;          /* Base of data */
    uint32_t image_base;         /* Image base */
    uint32_t section_align;      /* Section alignment */
    uint32_t file_align;         /* File alignment */
    uint16_t os_version;         /* OS version */
    uint16_t image_version;      /* Image version */
    uint16_t subsystem_version;   /* Subsystem version */
    uint32_t win32_version;      /* Win32 version */
    uint32_t image_size;         /* Size of image */
    uint32_t headers_size;       /* Size of headers */
    uint32_t checksum;           /* Checksum */
    uint16_t subsystem;          /* Subsystem */
    uint16_t dll_characteristics; /* DLL characteristics */
    uint32_t stack_reserve;      /* Stack reserve */
    uint32_t stack_commit;       /* Stack commit */
    uint32_t heap_reserve;       /* Heap reserve */
    uint32_t heap_commit;        /* Heap commit */
    uint32_t loader_flags;       /* Loader flags */
    uint32_t num_rva_sizes;      /* Number of RVA sizes */
} PE_OPTIONAL_HEADER;

typedef struct {
    uint8_t  name[8];             /* Section name */
    uint32_t virtual_size;       /* Virtual size */
    uint32_t virtual_address;     /* Virtual address */
    uint32_t raw_size;           /* Size of raw data */
    uint32_t raw_offset;         /* Pointer to raw data */
    uint32_t relocs_offset;      /* Relocations offset */
    uint16_t num_relocs;         /* Number of relocations */
    uint16_t line_nums;          /* Line numbers */
    uint32_t charact;            /* Characteristics */
} PE_SECTION_HEADER;

typedef struct {
    uint32_t rva;                /* Relative virtual address */
    uint32_t size;               /* Size */
} PE_DATA_DIRECTORY;

#define PE_MACHINE_ARM     0x01C0  /* ARM */
#define PE_MACHINE_ARM64   0xAA64  /* ARM64 */
#define PE_MACHINE_I386    0x014C  /* x86 */
#define PE_MACHINE_X64     0x8664  /* x64 */

#define PE_SUBSYSTEM_WINDOWS_CUI  3  /* Console */
#define PE_SUBSYSTEM_WINDOWS_GUI  2  /* GUI */

#define PE_CHAR_EXECUTABLE  0x0002
#define PE_CHAR_DLL         0x2000

/* PE loaded image */
typedef struct {
    void *base_address;           /* Base address of loaded image */
    uint32_t image_size;          /* Size of image */
    uint32_t entry_point;         /* Entry point RVA */
    uint32_t code_base;           /* Code base */
    uint32_t data_base;           /* Data base */
    void *entry_function;         /* Pointer to entry function */
    
    /* Sections */
    int num_sections;
    void **section_data;
    uint32_t *section_rva;
    uint32_t *section_size;
    
    /* Data directories */
    PE_DATA_DIRECTORY *directories;
    int num_directories;
    
    /* Subsystem */
    uint16_t subsystem;
    
    /* Architecture */
    uint16_t machine;
    
    /* TLS callback support */
    void **tls_callbacks;
    int num_tls_callbacks;
    
    /* Reference count */
    int refcount;
} PE_IMAGE;

/* DLL export/import */
typedef struct {
    const char *name;
    void *address;
    uint16_t ordinal;
} PE_EXPORT_ENTRY;

typedef struct {
    PE_IMAGE *image;
    int num_exports;
    PE_EXPORT_ENTRY *exports;
} PE_DLL;

/*
 * PE Loading functions
 */

/* Load PE image from file */
PE_IMAGE *ios_pe_load_from_file(const char *path);

/* Load PE image from memory */
PE_IMAGE *ios_pe_load_from_memory(const void *data, size_t size);

/* Unload PE image */
void ios_pe_unload(PE_IMAGE *image);

/* Get entry point */
void *ios_pe_get_entry_point(PE_IMAGE *image);

/* Get procedure address */
void *ios_pe_get_proc_address(PE_IMAGE *image, const char *name_or_ordinal);

/*
 * PE Execution functions
 */

/* Execute loaded PE image */
int ios_pe_execute(PE_IMAGE *image, int argc, char **argv, char **envp);

/* Create process from PE file */
int ios_pe_create_process(const char *path, int argc, char **argv, char **envp);

/*
 * DLL Loading functions
 */

/* Load DLL */
PE_DLL *ios_dll_load(const char *path);

/* Free DLL */
void ios_dll_free(PE_DLL *dll);

/* Get DLL procedure */
void *ios_dll_get_proc(PE_DLL *dll, const char *name);

/*
 * PE Image information
 */

/* Check if file is PE */
bool ios_pe_is_valid(const void *data, size_t size);

/* Get subsystem type */
const char *ios_pe_get_subsystem_name(uint16_t subsystem);

/* Get machine type name */
const char *ios_pe_get_machine_name(uint16_t machine);

/*
 * Address translation
 */

/* RVA to file offset */
uint32_t ios_pe_rva_to_offset(PE_IMAGE *image, uint32_t rva);

/* File offset to RVA */
uint32_t ios_pe_offset_to_rva(PE_IMAGE *image, uint32_t offset);

/* Translate pointer using RVA */
void *ios_pe_rva_to_pointer(PE_IMAGE *image, uint32_t rva);

/*
 * PE Relocation
 */

/* Apply relocations */
int ios_pe_apply_relocations(PE_IMAGE *image, uint64_t load_address);

/*
 * PE Initialization
 */

int ios_pe_init(void);

void ios_pe_cleanup(void);

#endif /* __WINE_IOS_PE_H */
