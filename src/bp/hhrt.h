#ifndef __HHRT_H__
#define __HHRT_H__

#ifdef __cplusplus
extern "C"{
#endif

#include "bp/bp.h"

/*************Interface to Scarab***************/
void bp_hhrt_init();
void bp_hhrt_timestamp(Op* op);
uns8 bp_hhrt_pred(Op*);
void bp_hhrt_spec_update(Op* op);
void bp_hhrt_update(Op* op);
void bp_hhrt_retire(Op* op);
void bp_hhrt_recover(Recovery_Info*);
uns8 bp_hhrt_full(uns proc_id);

#ifdef __cplusplus
}

#endif

#endif