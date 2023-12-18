/* Analysis algorithms on IL2 */
#ifndef IL2ANALYSIS_H
#define IL2ANALYSIS_H

#include "cfg.h"

/* Converts IL2 in CFG to Static Single Assignment form */
ErrorCode il2analysis_tossa(Cfg* cfg);

#endif
