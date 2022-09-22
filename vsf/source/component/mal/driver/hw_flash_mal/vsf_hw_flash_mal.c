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

#include "../../vsf_mal_cfg.h"

#if VSF_USE_MAL == ENABLED && VSF_MAL_USE_HW_FLASH_MAL == ENABLED && VSF_HAL_USE_FLASH == ENABLED

#define __VSF_MAL_CLASS_INHERIT__
#define __VSF_HW_FLASH_MAL_CLASS_IMPLEMENT

#include "../../vsf_mal.h"
#include "./vsf_hw_flash_mal.h"

/*============================ MACROS ========================================*/
/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/
/*============================ PROTOTYPES ====================================*/

static uint_fast32_t __vk_hw_flash_mal_blksz(vk_mal_t *mal, uint_fast64_t addr, uint_fast32_t size, vsf_mal_op_t op);
dcl_vsf_peda_methods(static, __vk_hw_flash_mal_init)
dcl_vsf_peda_methods(static, __vk_hw_flash_mal_fini)
dcl_vsf_peda_methods(static, __vk_hw_flash_mal_read)
dcl_vsf_peda_methods(static, __vk_hw_flash_mal_write)

/*============================ GLOBAL VARIABLES ==============================*/

#if     __IS_COMPILER_GCC__
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wcast-function-type"
#endif

const vk_mal_drv_t vk_hw_flash_mal_drv = {
    .blksz          = __vk_hw_flash_mal_blksz,
    .init           = (vsf_peda_evthandler_t)vsf_peda_func(__vk_hw_flash_mal_init),
    .fini           = (vsf_peda_evthandler_t)vsf_peda_func(__vk_hw_flash_mal_fini),
    .read           = (vsf_peda_evthandler_t)vsf_peda_func(__vk_hw_flash_mal_read),
    .write          = (vsf_peda_evthandler_t)vsf_peda_func(__vk_hw_flash_mal_write),
};

#if     __IS_COMPILER_GCC__
#   pragma GCC diagnostic pop
#endif

/*============================ LOCAL VARIABLES ===============================*/
/*============================ IMPLEMENTATION ================================*/

static uint_fast32_t __vk_hw_flash_mal_blksz(vk_mal_t *mal, uint_fast64_t addr,
                uint_fast32_t size, vsf_mal_op_t op)
{
    return 0;
}

#if     __IS_COMPILER_GCC__
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wcast-align"
#elif   __IS_COMPILER_LLVM__ || __IS_COMPILER_ARM_COMPILER_6__
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wcast-align"
#endif

__vsf_component_peda_ifs_entry(__vk_hw_flash_mal_init, vk_mal_init)
{
    vsf_peda_begin();
    vk_hw_flash_mal_t *pthis = (vk_hw_flash_mal_t *)&vsf_this;
    VSF_MAL_ASSERT(pthis != NULL);
    vsf_eda_return(VSF_ERR_NONE);
    vsf_peda_end();
}

__vsf_component_peda_ifs_entry(__vk_hw_flash_mal_fini, vk_mal_fini)
{
    vsf_peda_begin();
    vk_hw_flash_mal_t *pthis = (vk_hw_flash_mal_t *)&vsf_this;
    VSF_UNUSED_PARAM(pthis);
    VSF_MAL_ASSERT(pthis != NULL);
    vsf_eda_return(VSF_ERR_NONE);
    vsf_peda_end();
}

__vsf_component_peda_ifs_entry(__vk_hw_flash_mal_read, vk_mal_read)
{
    vsf_peda_begin();
    vk_hw_flash_mal_t *pthis = (vk_hw_flash_mal_t *)&vsf_this;
    uint_fast64_t addr;
    uint_fast32_t size;

    VSF_MAL_ASSERT(pthis != NULL);
    addr = vsf_local.addr;
    size = vsf_local.size;

    vsf_eda_return(size);
    vsf_peda_end();
}

__vsf_component_peda_ifs_entry(__vk_hw_flash_mal_write, vk_mal_write)
{
    vsf_peda_begin();
    vk_hw_flash_mal_t *pthis = (vk_hw_flash_mal_t *)&vsf_this;
    uint_fast64_t addr;
    uint_fast32_t size;

    VSF_MAL_ASSERT(pthis != NULL);
    addr = vsf_local.addr;
    size = vsf_local.size;

    vsf_eda_return(size);
    vsf_peda_end();
}

#if     __IS_COMPILER_GCC__
#   pragma GCC diagnostic pop
#elif   __IS_COMPILER_LLVM__ || __IS_COMPILER_ARM_COMPILER_6__
#   pragma clang diagnostic pop
#endif

#endif
