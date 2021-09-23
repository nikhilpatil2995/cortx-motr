/* -*- C -*- */
/*
 * Copyright (c) 2021 Seagate Technology LLC and/or its Affiliates
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com.
 *
 */


#pragma once

#ifndef __MOTR_DTM0_RECOVERY_H__
#define __MOTR_DTM0_RECOVERY_H__

#include "lib/tlist.h" /* m0_tl */

/* TODO: figure good literals and it move to the satchel */
#define M0_DTM0_RMACH_MAGIC 0x3AB0DBED
#define M0_DTM0_RMACH_HEAD_MAGIC 0xB0EB0D11

/* imports */
struct m0_dtm0_service;

/* exports */
struct m0_dtm0_recovery_machine {
	struct m0_dtm0_service *rm_svc;
	struct m0_tl            rm_rfoms;
};

M0_INTERNAL int
m0_dtm0_recovery_machine_init(struct m0_dtm0_recovery_machine *recovery,
			      struct m0_dtm0_service  *svc);

M0_INTERNAL void
m0_dtm0_recovery_machine_start(struct m0_dtm0_recovery_machine *recovery);

M0_INTERNAL void
m0_dtm0_recovery_machine_stop(struct m0_dtm0_recovery_machine *recovery);

M0_INTERNAL void
m0_dtm0_recovery_machine_fini(struct m0_dtm0_recovery_machine *recovery);

#endif /* __MOTR_DTM0_RECOVERY_H__ */

/*
 *  Local variables:
 *  c-indentation-style: "K&R"
 *  c-basic-offset: 8
 *  tab-width: 8
 *  fill-column: 80
 *  scroll-step: 1
 *  End:
 */
/*
 * vim: tabstop=8 shiftwidth=8 noexpandtab textwidth=80 nowrap
 */
