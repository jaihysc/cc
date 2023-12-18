#include "il2analysis.h"

ErrorCode il2analysis_tossa(Cfg* cfg) {
	return cfg_compute_block_use_def(cfg);
}
