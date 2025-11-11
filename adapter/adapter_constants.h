/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Distributed under BSD 3-Clause license.                                   *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Illinois Institute of Technology.                        *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of Hermes. The full Hermes copyright notice, including  *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the top directory. If you do not  *
 * have access to the file, you may request a copy from help@hdfgroup.org.   *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef WRP_CTE_ADAPTER_ADAPTER_CONSTANTS_H_
#define WRP_CTE_ADAPTER_ADAPTER_CONSTANTS_H_

// Macro for function declarations (backward compatibility)
#ifndef WRP_CTE_DECL
#define WRP_CTE_DECL(F) F
#endif

#include "mapper/abstract_mapper.h"

namespace wrp::cae {

static inline const MapperType kMapperType = MapperType::kBalancedMapper;

}

#endif  // WRP_CTE_ADAPTER_ADAPTER_CONSTANTS_H_
