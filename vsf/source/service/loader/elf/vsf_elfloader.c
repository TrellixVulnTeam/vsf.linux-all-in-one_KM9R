/*****************************************************************************
 *   Copyright(C)2009-2022 by VSF Team                                       *
 *                                                                           *
 *  Licensed under the Apache License, Version 2.0 (the "License");          *
 *  you may not use this file except in compliance with the License.         *
 *  You may obtain a copy of the License at                                  *
 *                                                                           *
 *     http://www.apache.org/licenses/LICENSE-2.0                            *
 *                                                                           *
 *  Unless required by applicable law or agreed to in writing, software      *
 *  distributed under the License is distributed on an "AS IS" BASIS,        *
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
 *  See the License for the specific language governing permissions and      *
 *  limitations under the License.                                           *
 *                                                                           *
 ****************************************************************************/

/*============================ INCLUDES ======================================*/
#include "service/vsf_service_cfg.h"

#if VSF_USE_LOADER == ENABLED && VSF_LOADER_USE_ELF == ENABLED

#define __VSF_ELFLOADER_CLASS_IMPLEMENT
#define __VSF_LOADER_CLASS_INHERIT__
#include "../vsf_loader.h"
#include "./elf.h"

#if VSF_ELFLOADER_CFG_DEBUG == ENABLED
#   include "service/vsf_service.h"
#endif

/*============================ MACROS ========================================*/

#ifndef VSF_ELFLOADER_CFG_MAX_SYM_LEN
#   define VSF_ELFLOADER_CFG_MAX_SYM_LEN        (32 + 1)
#endif

#define VSF_ELFLOADER_LOADABLE_SECNUM           7

/*============================ MACROFIED FUNCTIONS ===========================*/

#ifdef VSF_ELFLOADER_CFG_DEBUG
#   define vsf_elfloader_trace(...)             vsf_trace(__VA_ARGS__)
#   define vsf_elfloader_trace_buffer(...)      vsf_trace_buffer(__VA_ARGS__)
#else
#   define vsf_elfloader_trace(...)
#   define vsf_elfloader_trace_buffer(...)
#endif

/*============================ TYPES =========================================*/

typedef struct vsf_elfloader_section_info_t {
    int index;
    Elf32_Off offset;
    Elf32_Word size;
    Elf32_Addr addr;
    void *buffer;
} vsf_elfloader_section_info_t;

typedef struct vsf_elfloader_info_t {
    void *initarr;
    Elf32_Word initarr_sz;

    struct {
        Elf32_Shdr header;
        int loader_index;               // index in __vsf_elfloader_section_loaders
        void **buffer;                  // buffer to load
        uint32_t align;                 // alignment of the buffer
        vsf_loader_mem_attr_t attr;     // attribute of the buffer
        bool is_to_load_section;
    } sinfo;
    vsf_elfloader_section_info_t sinfos[VSF_ELFLOADER_LOADABLE_SECNUM];
} vsf_elfloader_info_t;

typedef struct vsf_elfloader_section_loader_t {
    char *name;
    vsf_err_t (*load)(vsf_elfloader_t *elfloader, vsf_loader_target_t *target, vsf_elfloader_info_t *linfo);
} vsf_elfloader_section_loader_t;

/*============================ PROTOTYPES ====================================*/

static vsf_err_t __vsf_elfloader_text_loader(vsf_elfloader_t *elfloader, vsf_loader_target_t *target, vsf_elfloader_info_t *linfo);
static vsf_err_t __vsf_elfloader_bss_loader(vsf_elfloader_t *elfloader, vsf_loader_target_t *target, vsf_elfloader_info_t *linfo);
static vsf_err_t __vsf_elfloader_data_loader(vsf_elfloader_t *elfloader, vsf_loader_target_t *target, vsf_elfloader_info_t *linfo);
static vsf_err_t __vsf_elfloader_rodata_loader(vsf_elfloader_t *elfloader, vsf_loader_target_t *target, vsf_elfloader_info_t *linfo);
static vsf_err_t __vsf_elfloader_noinit_loader(vsf_elfloader_t *elfloader, vsf_loader_target_t *target, vsf_elfloader_info_t *linfo);
static vsf_err_t __vsf_elfloader_dynsym_loader(vsf_elfloader_t *elfloader, vsf_loader_target_t *target, vsf_elfloader_info_t *linfo);
static vsf_err_t __vsf_elfloader_dynstr_loader(vsf_elfloader_t *elfloader, vsf_loader_target_t *target, vsf_elfloader_info_t *linfo);
static vsf_err_t __vsf_elfloader_plt_loader(vsf_elfloader_t *elfloader, vsf_loader_target_t *target, vsf_elfloader_info_t *linfo);
static vsf_err_t __vsf_elfloader_initarr_loader(vsf_elfloader_t *elfloader, vsf_loader_target_t *target, vsf_elfloader_info_t *linfo);
static vsf_err_t __vsf_elfloader_finiarr_loader(vsf_elfloader_t *elfloader, vsf_loader_target_t *target, vsf_elfloader_info_t *linfo);
static vsf_err_t __vsf_elfloader_got_loader(vsf_elfloader_t *elfloader, vsf_loader_target_t *target, vsf_elfloader_info_t *linfo);

/*============================ LOCAL VARIABLES ===============================*/

static const vsf_elfloader_section_loader_t __vsf_elfloader_section_loaders[] = {
    // loadable sections
    {".bss",            __vsf_elfloader_bss_loader},
    {".data",           __vsf_elfloader_data_loader},
    {".noinit",         __vsf_elfloader_noinit_loader},
    {".got",            __vsf_elfloader_got_loader},

    {".text",           __vsf_elfloader_text_loader},
    {".plt",            __vsf_elfloader_plt_loader},
    {".rodata",         __vsf_elfloader_rodata_loader},

    // non-loadable sections
    {".dynsym",         __vsf_elfloader_dynsym_loader},
    {".dynstr",         __vsf_elfloader_dynstr_loader},
    {".init_array",     __vsf_elfloader_initarr_loader},
    {".fini_array",     __vsf_elfloader_finiarr_loader},
};

/*============================ GLOBAL VARIABLES ==============================*/
/*============================ IMPLEMENTATION ================================*/

static vsf_elfloader_section_info_t * __vsf_elfloader_get_section_by_index(vsf_elfloader_info_t *linfo, int idx)
{
    vsf_elfloader_section_info_t *sinfo = linfo->sinfos;
    for (int i = 0; i < dimof(linfo->sinfos); i++, sinfo++) {
        if (sinfo->index == idx) {
            return sinfo;
        }
    }
    return NULL;
}

static vsf_elfloader_section_info_t * __vsf_elfloader_get_section_by_addr(vsf_elfloader_info_t *linfo, Elf32_Addr addr)
{
    vsf_elfloader_section_info_t *sinfo = linfo->sinfos;
    for (int i = 0; i < dimof(linfo->sinfos); i++, sinfo++) {
        if ((sinfo->addr <= addr) && (sinfo->addr + sinfo->size > addr)) {
            return sinfo;
        }
    }
    return NULL;
}

WEAK(vsf_elfloader_relocate_sym)
vsf_err_t vsf_elfloader_relocate_sym(Elf32_Addr tgtaddr, int type, Elf32_Addr tgtvalue)
{
    switch (type) {
    case R_ARM_JUMP_SLOT:
    case R_ARM_RELATIVE:
        *(uint32_t *)tgtaddr = tgtvalue;
        return VSF_ERR_NONE;
    }
    return VSF_ERR_FAIL;
}

static vsf_err_t __vsf_elfloader_relocate(vsf_elfloader_t *elfloader, vsf_loader_target_t *target,
        vsf_elfloader_info_t *linfo)
{
    Elf32_Word relnum = linfo->sinfo.header.sh_size / linfo->sinfo.header.sh_entsize;
    Elf32_Off offset = linfo->sinfo.header.sh_offset;
    Elf32_Rel rel;
    Elf32_Word relsym;
    Elf32_Word reltype;
    Elf32_Sym sym;
    Elf32_Addr tgtvalue;

    char symname[VSF_ELFLOADER_CFG_MAX_SYM_LEN];
    vsf_elfloader_section_info_t *tgt_sinfo;
    vsf_elfloader_section_info_t *sym_sinfo;

    if (linfo->sinfo.header.sh_entsize != sizeof(rel)) {
        return VSF_ERR_FAIL;
    }

    for (int i = 0; i < relnum; i++, offset += sizeof(rel)) {
        if (vsf_loader_read(target, offset, &rel, sizeof(rel)) != sizeof(rel)) {
            return VSF_ERR_FAIL;
        }

        relsym = ELF32_R_SYM(rel.r_info);
        reltype = ELF32_R_TYPE(rel.r_info);
        tgt_sinfo = __vsf_elfloader_get_section_by_addr(linfo, rel.r_offset);
        if (NULL == tgt_sinfo) {
            vsf_elfloader_trace(VSF_TRACE_ERROR, "fail to get rel target" VSF_TRACE_CFG_LINEEND);
            return VSF_ERR_FAIL;
        }

        if (0 == relsym) {
            if (tgt_sinfo->buffer == elfloader->got) {
                tgtvalue = *(Elf32_Addr *)((Elf32_Addr)tgt_sinfo->buffer + rel.r_offset - tgt_sinfo->addr);
                sym_sinfo = __vsf_elfloader_get_section_by_addr(linfo, tgtvalue);
                if (NULL == sym_sinfo) {
                    vsf_elfloader_trace(VSF_TRACE_ERROR, "fail to get rel value" VSF_TRACE_CFG_LINEEND);
                    return VSF_ERR_FAIL;
                }

                if (sym_sinfo->buffer != NULL) {
                    tgtvalue = (Elf32_Addr)sym_sinfo->buffer + tgtvalue - sym_sinfo->addr;
                } else if (target->is_xip) {
                    tgtvalue = (Elf32_Addr)target->object + tgtvalue;
                } else {
                    vsf_elfloader_trace(VSF_TRACE_ERROR, "fail to get rel value" VSF_TRACE_CFG_LINEEND);
                    return VSF_ERR_FAIL;
                }
            } else {
                vsf_elfloader_trace(VSF_TRACE_ERROR, "unknown rel" VSF_TRACE_CFG_LINEEND);
                return VSF_ERR_FAIL;
            }
        } else {
            if (vsf_loader_read(target, elfloader->symtbl_off + relsym * sizeof(sym), &sym, sizeof(sym)) != sizeof(sym)) {
                return VSF_ERR_FAIL;
            }
            if (!vsf_loader_read(target, elfloader->symstrtbl_off + sym.st_name, symname, sizeof(symname))) {
                return VSF_ERR_FAIL;
            }

            vsf_elfloader_trace(VSF_TRACE_DEBUG, "relocate %s" VSF_TRACE_CFG_LINEEND, symname);

            if (SHN_UNDEF == sym.st_shndx) {
                tgtvalue = (Elf32_Addr)vsf_loader_link((vsf_loader_t *)elfloader, symname);
                if (0 == tgtvalue) {
                    vsf_elfloader_trace(VSF_TRACE_ERROR, "fail to get %s" VSF_TRACE_CFG_LINEEND, symname);
                    return VSF_ERR_FAIL;
                }
            } else {
                sym_sinfo = __vsf_elfloader_get_section_by_index(linfo, sym.st_shndx);
                if (NULL == sym_sinfo) {
                    vsf_elfloader_trace(VSF_TRACE_ERROR, "fail to get section of %s" VSF_TRACE_CFG_LINEEND, symname);
                    return VSF_ERR_FAIL;
                }
                if (sym_sinfo->buffer != NULL) {
                    tgtvalue = (Elf32_Addr)sym_sinfo->buffer + sym.st_value;
                } else if (target->is_xip) {
                    tgtvalue = (Elf32_Addr)target->object + sym.st_value;
                } else {
                    vsf_elfloader_trace(VSF_TRACE_ERROR, "fail to get rel value" VSF_TRACE_CFG_LINEEND);
                    return VSF_ERR_FAIL;
                }
            }
        }

        if (VSF_ERR_NONE != vsf_elfloader_relocate_sym((Elf32_Addr)tgt_sinfo->buffer + rel.r_offset - tgt_sinfo->addr, reltype, tgtvalue)) {
            vsf_elfloader_trace(VSF_TRACE_ERROR, "fail to link rel" VSF_TRACE_CFG_LINEEND);
        }
    }
    return VSF_ERR_NONE;
}

static vsf_err_t __vsf_elfloader_text_loader(vsf_elfloader_t *elfloader, vsf_loader_target_t *target,
        vsf_elfloader_info_t *linfo)
{
    linfo->sinfo.is_to_load_section = !target->is_xip;
    linfo->sinfo.buffer = &elfloader->text;
    return VSF_ERR_NONE;
}

static vsf_err_t __vsf_elfloader_bss_loader(vsf_elfloader_t *elfloader, vsf_loader_target_t *target,
        vsf_elfloader_info_t *linfo)
{
    linfo->sinfo.buffer = &elfloader->bss;
    return VSF_ERR_NONE;
}

static vsf_err_t __vsf_elfloader_data_loader(vsf_elfloader_t *elfloader, vsf_loader_target_t *target,
        vsf_elfloader_info_t *linfo)
{
    linfo->sinfo.buffer = &elfloader->data;
    return VSF_ERR_NONE;
}

static vsf_err_t __vsf_elfloader_rodata_loader(vsf_elfloader_t *elfloader, vsf_loader_target_t *target,
        vsf_elfloader_info_t *linfo)
{
    linfo->sinfo.is_to_load_section = !target->is_xip;
    linfo->sinfo.buffer = &elfloader->rodata;
    return VSF_ERR_NONE;
}

static vsf_err_t __vsf_elfloader_noinit_loader(vsf_elfloader_t *elfloader, vsf_loader_target_t *target,
        vsf_elfloader_info_t *linfo)
{
    linfo->sinfo.buffer = &elfloader->noinit;
    return VSF_ERR_NONE;
}

static vsf_err_t __vsf_elfloader_dynsym_loader(vsf_elfloader_t *elfloader, vsf_loader_target_t *target,
        vsf_elfloader_info_t *linfo)
{
    elfloader->symtbl_off = linfo->sinfo.header.sh_offset;
    elfloader->symtbl_sz = linfo->sinfo.header.sh_size;
    return VSF_ERR_NONE;
}

static vsf_err_t __vsf_elfloader_dynstr_loader(vsf_elfloader_t *elfloader, vsf_loader_target_t *target,
        vsf_elfloader_info_t *linfo)
{
    elfloader->symstrtbl_off = linfo->sinfo.header.sh_offset;
    elfloader->symstrtbl_sz = linfo->sinfo.header.sh_size;
    return VSF_ERR_NONE;
}

static vsf_err_t __vsf_elfloader_plt_loader(vsf_elfloader_t *elfloader, vsf_loader_target_t *target,
        vsf_elfloader_info_t *linfo)
{
    linfo->sinfo.is_to_load_section = !target->is_xip;
    linfo->sinfo.buffer = &elfloader->plt;
    return VSF_ERR_NONE;
}

static vsf_err_t __vsf_elfloader_initarr_loader(vsf_elfloader_t *elfloader, vsf_loader_target_t *target,
        vsf_elfloader_info_t *linfo)
{
    linfo->sinfo.buffer = &linfo->initarr;
    linfo->initarr_sz = linfo->sinfo.header.sh_size;
    return VSF_ERR_NONE;
}

static vsf_err_t __vsf_elfloader_finiarr_loader(vsf_elfloader_t *elfloader, vsf_loader_target_t *target,
        vsf_elfloader_info_t *linfo)
{
    linfo->sinfo.buffer = &elfloader->finiarr;
    return VSF_ERR_NONE;
}

static vsf_err_t __vsf_elfloader_got_loader(vsf_elfloader_t *elfloader, vsf_loader_target_t *target,
        vsf_elfloader_info_t *linfo)
{
    linfo->sinfo.buffer = &elfloader->got;
    return VSF_ERR_NONE;
}

void vsf_elfloader_cleanup(vsf_elfloader_t *elfloader)
{
    if (elfloader->finiarr_off != 0) {
        // TODO: call fini
        VSF_SERVICE_ASSERT(false);
    }

    elfloader->target = NULL;
    if (elfloader->plt != NULL) {
        vsf_loader_free(elfloader, VSF_LOADER_MEM_X, elfloader->plt);
        elfloader->plt = NULL;
    }
    if (elfloader->got != NULL) {
        vsf_loader_free(elfloader, VSF_LOADER_MEM_RW, elfloader->got);
        elfloader->got = NULL;
    }
    if (elfloader->text != NULL) {
        vsf_loader_free(elfloader, VSF_LOADER_MEM_X, elfloader->text);
        elfloader->text = NULL;
    }
    if (elfloader->finiarr != NULL) {
        vsf_loader_free(elfloader, VSF_LOADER_MEM_RW, elfloader->finiarr);
        elfloader->finiarr = NULL;
    }
    if (elfloader->bss != NULL) {
        vsf_loader_free(elfloader, VSF_LOADER_MEM_RW, elfloader->bss);
        elfloader->bss = NULL;
    }
    if (elfloader->data != NULL) {
        vsf_loader_free(elfloader, VSF_LOADER_MEM_RW, elfloader->data);
        elfloader->data = NULL;
    }
    if (elfloader->rodata != NULL) {
        vsf_loader_free(elfloader, VSF_LOADER_MEM_RW, elfloader->rodata);
        elfloader->rodata = NULL;
    }
    if (elfloader->noinit != NULL) {
        vsf_loader_free(elfloader, VSF_LOADER_MEM_RW, elfloader->noinit);
        elfloader->noinit = NULL;
    }
}

void * vsf_elfloader_load(vsf_elfloader_t *elfloader, vsf_loader_target_t *target)
{
    VSF_SERVICE_ASSERT((elfloader != NULL) && (target != NULL));
    VSF_SERVICE_ASSERT(NULL == elfloader->target);
    VSF_SERVICE_ASSERT(target->fn_read != NULL);

    Elf32_Hdr elf_hdr;
    vsf_elfloader_info_t linfo = { 0 };
    if (vsf_loader_read(target, 0, &elf_hdr, sizeof(elf_hdr)) != sizeof(elf_hdr)) {
        vsf_elfloader_trace(VSF_TRACE_ERROR, "fail to read elf header" VSF_TRACE_CFG_LINEEND);
        return NULL;
    }

    const char Elf32_Magic[4] = {ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3};
    if (    memcmp(&elf_hdr.e_ident, Elf32_Magic, sizeof(Elf32_Magic))
        ||  (elf_hdr.e_machine != elfloader->machine)
        ||  (elf_hdr.e_version != EV_CURRENT)
        ||  (elf_hdr.e_shentsize != sizeof(Elf32_Shdr))) {
        vsf_elfloader_trace(VSF_TRACE_ERROR, "invalid elf header" VSF_TRACE_CFG_LINEEND);
        return NULL;
    }

    if (vsf_loader_read(target, elf_hdr.e_shoff + elf_hdr.e_shstrndx * elf_hdr.e_shentsize,
            &linfo.sinfo.header, sizeof(linfo.sinfo.header)) != sizeof(linfo.sinfo.header)) {
        vsf_elfloader_trace(VSF_TRACE_ERROR, "fail to read string section" VSF_TRACE_CFG_LINEEND);
        return NULL;
    }
    elfloader->strtbl_off = linfo.sinfo.header.sh_offset;
    elfloader->strtbl_sz = linfo.sinfo.header.sh_size;

    char section_name[VSF_ELFLOADER_CFG_MAX_SYM_LEN];
    vsf_elfloader_section_loader_t *sloader;
    for (int i = 1; i < elf_hdr.e_shnum; i++) {
        memset(&linfo.sinfo, 0, sizeof(linfo.sinfo));
        if (vsf_loader_read(target, elf_hdr.e_shoff + i * elf_hdr.e_shentsize,
                &linfo.sinfo.header, sizeof(linfo.sinfo.header)) != sizeof(linfo.sinfo.header)) {
            return NULL;
        }

        if (!vsf_loader_read(target, elfloader->strtbl_off + linfo.sinfo.header.sh_name, section_name, sizeof(section_name))) {
            return NULL;
        }
        if (!strncmp(section_name, ".rel.", 5)) {
            continue;
        }

        vsf_elfloader_trace(VSF_TRACE_DEBUG, "loading section %s" VSF_TRACE_CFG_LINEEND, section_name);
        linfo.sinfo.is_to_load_section = linfo.sinfo.header.sh_flags & SHF_ALLOC;
        if (linfo.sinfo.header.sh_flags & SHF_EXECINSTR) {
            linfo.sinfo.attr = VSF_LOADER_MEM_X;
        } else if (linfo.sinfo.header.sh_flags & SHF_WRITE) {
            linfo.sinfo.attr = VSF_LOADER_MEM_RW;
        } else {
            linfo.sinfo.attr = VSF_LOADER_MEM_R;
        }

        sloader = NULL;
        for (linfo.sinfo.loader_index = 0; linfo.sinfo.loader_index < dimof(__vsf_elfloader_section_loaders); linfo.sinfo.loader_index++) {
            if (!strcmp(__vsf_elfloader_section_loaders[linfo.sinfo.loader_index].name, section_name)) {
                sloader = (vsf_elfloader_section_loader_t *)&__vsf_elfloader_section_loaders[linfo.sinfo.loader_index];
                break;
            }
        }

        if (sloader != NULL) {
            if (linfo.sinfo.loader_index < dimof(linfo.sinfos)) {
                linfo.sinfos[linfo.sinfo.loader_index].index = i;
                linfo.sinfos[linfo.sinfo.loader_index].offset = linfo.sinfo.header.sh_offset;
                linfo.sinfos[linfo.sinfo.loader_index].size = linfo.sinfo.header.sh_size;
                linfo.sinfos[linfo.sinfo.loader_index].addr = linfo.sinfo.header.sh_addr;
            }
            if ((sloader->load != NULL) && (VSF_ERR_NONE != sloader->load(elfloader, target, &linfo))) {
                vsf_elfloader_trace(VSF_TRACE_ERROR, "fail to load section %s" VSF_TRACE_CFG_LINEEND, section_name);
                goto cleanup_and_fail;
            }
        }

        if (linfo.sinfo.is_to_load_section && (linfo.sinfo.buffer != NULL) && (linfo.sinfo.header.sh_size > 0)) {
            *linfo.sinfo.buffer = vsf_loader_malloc(elfloader, linfo.sinfo.attr, linfo.sinfo.header.sh_size, linfo.sinfo.header.sh_addralign);
            if (NULL == *linfo.sinfo.buffer) {
                vsf_elfloader_trace(VSF_TRACE_ERROR, "fail to allocate section buffer for %s" VSF_TRACE_CFG_LINEEND, section_name);
                goto cleanup_and_fail;
            }
            if (linfo.sinfo.header.sh_type != SHT_NOBITS) {
                if (vsf_loader_read(target, linfo.sinfo.header.sh_offset, *linfo.sinfo.buffer, linfo.sinfo.header.sh_size) != linfo.sinfo.header.sh_size) {
                    vsf_elfloader_trace(VSF_TRACE_ERROR, "fail to read section %s" VSF_TRACE_CFG_LINEEND, section_name);
                    goto cleanup_and_fail;
                }
            }

            if ((sloader != NULL) && (linfo.sinfo.loader_index < dimof(linfo.sinfos))) {
                linfo.sinfos[linfo.sinfo.loader_index].buffer = *linfo.sinfo.buffer;
            }
        }
    }

    for (int i = 1; i < elf_hdr.e_shnum; i++) {
        memset(&linfo.sinfo, 0, sizeof(linfo.sinfo));
        if (vsf_loader_read(target, elf_hdr.e_shoff + i * elf_hdr.e_shentsize,
                &linfo.sinfo.header, sizeof(linfo.sinfo.header)) != sizeof(linfo.sinfo.header)) {
            return NULL;
        }

        if (!vsf_loader_read(target, elfloader->strtbl_off + linfo.sinfo.header.sh_name, section_name, sizeof(section_name))) {
            return NULL;
        }
        if (strncmp(section_name, ".rel.", 5)) {
            continue;
        }

        vsf_elfloader_trace(VSF_TRACE_DEBUG, "relcating section %s" VSF_TRACE_CFG_LINEEND, section_name);
        if (VSF_ERR_NONE != __vsf_elfloader_relocate(elfloader, target, &linfo)) {
            vsf_elfloader_trace(VSF_TRACE_ERROR, "fail to relocate section %s" VSF_TRACE_CFG_LINEEND, section_name);
            goto cleanup_and_fail;
        }
    }

    if (linfo.initarr != 0) {
        // TODO: call init
        VSF_SERVICE_ASSERT(false);

        vsf_loader_free(elfloader, VSF_LOADER_MEM_RW, linfo.initarr);
    }

    elfloader->target = target;
    if (target->is_xip) {
        return (void *)(target->object + elf_hdr.e_entry);
    } else {
        return (void *)((uintptr_t)elfloader->text + elf_hdr.e_entry);
    }

cleanup_and_fail:
    vsf_elfloader_cleanup(elfloader);
    return NULL;
}

#endif      // VSF_USE_LOADER && VSF_LOADER_USE_ELF
