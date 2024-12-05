#ifndef __BIMODAL_H__
#define __BIMODAL_H__

#ifdef __cplusplus
extern "C"{
#endif

#include "bp/bp.h"

/*************Interface to Scarab***************/
void bp_bimodal_init();
void bp_bimodal_timestamp(Op* op);
uns8 bp_bimodal_pred(Op*);
void bp_bimodal_spec_update(Op* op);
void bp_bimodal_update(Op* op);
void bp_bimodal_retire(Op* op);
void bp_bimodal_recover(Recovery_Info*);
uns8 bp_bimodal_full(uns proc_id);

#ifdef __cplusplus
}

#endif

#endif