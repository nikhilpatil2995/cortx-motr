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


/**
   @page dtm0br-dld DLD of DTM0 basic recovery

   - @ref DTM0BR-ovw
   - @ref DTM0BR-def
   - @ref DTM0BR-req
   - @ref DTM0BR-depends
   - @ref DTM0BR-highlights
   - @subpage DTM0BR-fspec "Functional Specification"
   - @ref DTM0BR-lspec
      - @ref DTM0BR-lspec-comps
      - @ref DTM0BR-lspec-rem-fom
      - @ref DTM0BR-lspec-loc-fom
      - @ref DTM0BR-lspec-evc-fom
      - @ref DTM0BR-lspec-state
      - @ref DTM0BR-lspec-thread
      - @ref DTM0BR-lspec-numa
   - @ref DTM0BR-conformance
   - @ref DTM0BR-usecases
   - @ref DTM0BR-ref
   - @ref DTM0BR-impl-plan
--->>>
Max: [* defect *] ut and st sections are missing.
IvanA: Agree. I will add it once we are more-or-less confident in use-cases and
       requirements.
<<<---
--->>>
Max: [* style *] usecases section should go to .h file, along with the rest of
     the sections in .h in DLD template.
<<<---


   <hr>
   @section DTM0BR-ovw Overview
   <i>All specifications must start with an Overview section that
   briefly describes the document and provides any additional
   instructions or hints on how to best read the specification.</i>

   The document describes the way how DTM0 service is supposed to restore
   consistency of the data replicated across a certain kind of Motr-based
   cluster. The term "basic" implies that consistency is resored only
   for a limited number of use-cases. In the rest of the document,
   the term "basic" is omitted for simplicity (there are no other kinds of
   DTM0-based recovery at the moment).

--->>>
Max: [* defect *] This is an obvious definition, which has very small value.
     Please make an overview which would bring as much useful information to the
     reader as it can in just a few lines.
IvanA: [fixed] I tried to stick to the point, and avoid any "new" terms in
       the paragraph.
<<<---


   <hr>
   @section DTM0BR-def Definitions
   <i>Mandatory.
   The DLD shall provide definitions of the terms and concepts
   introduced by the design, as well as the relevant terms used by the
   specification but described elsewhere.  References to the
   M0 Glossary and the component's HLD are permitted and encouraged.
   Agreed upon terminology should be incorporated in the glossary.</i>

--->>>
Max: [* question *] Are those all the terms that are used here? If no, please
     add the rest of the terms. Please also consider adding terms that are being
     used, but which are not in DTM0 HLD.
IvanA: [fixed] No, it seems like they are not used. I removed this part.
       I'll populate the "new terms" first, and if there are any redundant
       information, I'll add references to the HLD.
<<<---

   Terms and definitions used in the document:
   - <b>User service</b> is a Motr service that is capable of replicating
     of its data ("user data") across the cluster. The document is focused on
     CAS (see @ref cas-dld) but this term could be applied to any Motr service
     that supports CRDT[2] and has behavior similar to CAS.
     Note, this document does not differentiate actual "clients" and "servers".
     For example, the Motr client (including DIX) is also considered to be a
     "User service".
   - <b>DTM0 service</b> is a Motr-service that helps a user service restore
     consistency of its replicated data.
   - <b>Recoverable process</b> is Motr process that has exactly one user
     service and exactly one DTM0 service in it. Note that in UT environment,
     a single OS process may have several user/DTM0 services, thus it may
     have multiple recoverable services.
--->>>
IvanA: [* question *] @Max, "Recoverable process" does not sound good to me.
       What do you think about "DTM0 process"? or "DTM0-backed process"?
       I just want to avoid confusion with confd and any other Motr process that
       does not have a DTM0 service in it. Any ideas on that?
<<<---
   - <b>Recovery procedures</b> is a broad term used to point at DTM0 services'
     reactions to HA notifications about states of DTM0 services. In other
     words, it references to actions performed by DTM0 services that
     help to restore consistency of replicated data.
   - <b>Recovery machine</b> is a state machine running with a DTM0 service
     that performs recovery procedures. Each recoverable process
     has a single instance of recovery machine.
   - <b>Recovery FOM</b> is a long-lived FOM responsible for reaction to
      HA-provided state changes of a recoverable process. The FOM knows the
      id (FID) of this process (<b>Recovery FOM id</b>). If the recovery
      FOM id matches with the id of the process it is running on, then
      this FOM is called "local". Otherwise, it is called "remote".
      If a Motr cluster has N recoverable processes in the configuration then
      each recoverable process has N recovery FOMs (one per remote counterpart
      plus one local FOM).
   - <b>Recovery FOM role</b> is a sub-set of states of a recovery FOM. Each
      recovery FOM may have one of the following roles: remote recovery,
      local recovery, eviction. The term "role" is often omitted.
   - <b>Recovery FOM reincarnation</b> is a transition between the roles
      of a recovery FOM. Reincarnation happens as a reaction to HA notifications
      about state changes of the corresponding process.
   - <b>Remote recovery role</b> defines a sub-set of states of a recovery FOM
      that performs recovery of a remote process (by sending REDO messages).
   - <b>Local recovery role </b> defines a sub-set of states of a recovery FOM
      that is responsible for starting and stopping of recovery procedures on
      the local recoverable process.
   - <b>Eviction role</b> defines a sub-set of states of a recovery FOM that is
      restores consistency of up to N-1 (N is number of recoverable processes
      in the configuration) recoverable processes after one of the recoverable
      processes of the cluster experienced a permanent failure (see HLD[1]).
   - <b>W-request</b> is a FOP sent from one user service to another that
      causes modifications of persistent storage. Such a request always
      contains DTM0-related meta-data (transaction descriptor). W-requests
      are used to replicate data across the cluster. They are stored in DTM0
      log, and replayed by DTM0 service in form of REDO messages. In case of
      CAS, W-requests are PUT and DEL operations
   - <b>R-request</b> is a user service FOP that does not modify persistent
      storage. R-requires are allowed to be sent only to the processes that
      have ONLINE state. In case of CAS, R-requests are GET and NEXT operations.
--->>>
Max: [* defect *] The terms are not defined. Please define them.
IvanA: I populated the list. I'll update the rest of the document
       in a separate round of fixes.
<<<---

   <hr>
   @section DTM0BR-req Requirements
   <i>Mandatory.
   The DLD shall state the requirements that it attempts to meet.</i>

   Recovery machine shall meet the requirements defined in the DTM0 HLD[1].
--->>>
Max: [* question *] All of them? If no, please put a list here. If so, please
     put the list anyway.
<<<---
   Additionally, it has the following list of low-level requirements:
--->>>
Max: [* question *] Why do we have those requirements? Please provide a way to
     track them to the top-level requirements or requriements for other DTM0
     subcomponents.
<<<---

   - @b R.DTM0BR.HA-Aligned State transitions of the recovery SM shall be
   aligned with the state transitions of Motr processes delivered by
   the HA subsystem to the recovery machine.
   - @b R.DTM0BR.Basic Recovery machine shall support only a subset of
   possible use-cases. The subset is defined in @ref DTM0BR-usecases.
--->>>
Max: [* defect *] Usecases are not defined.
<<<---
   - @b R.DTM0BR.No-Batching Recovery machine shall replay logs in
   a synchronous manner one-by-one.
--->>>
Max: [* question *] One-by-one log or one-by-one log record? In the latter case:
     what about records from logs on different nodes?
<<<---
--->>>
Max: [* question *] How those logs or records are ordered (to be able to process
     them one-by-one)?
<<<---
--->>>
Max: [* question *] What about performance: is it possible for this design to
     meet performance requirements? In any case please clearly state if it is
     and why.
<<<---

   <hr>
   @section DTM0BR-depends Dependencies
   <i>Mandatory. Identify other components on which this specification
   depends.</i>

   The basic recovery machine depends on the following components:
   - Hare. This component is supposed to own an ordered log of all HA events
     in the system. Note, this dependency is optional for a certain set
     of use-cases where events are delivered in the right way even without
     beign backed up by a log.
--->>>
Max: [* defect *] This is not true at the moment. Please specify if it's
     possible to implement this design without this being true or what is the
     plan to deal with this.
IvanA: [fixed] I added a note that this thing is optional for some cases.
<<<---
   - DTM0 service. The service provides access to DTM0 log and DTM0 RPC link.
   - DTM0 log. The log is used to fill in REDO messages.
   - DTM0 RPC link. The link is used as transport for REDO messages.
   - Motr HA/conf. HA states of Motr conf objects provide the information
   about state transitions of local and remote Motr processes.
   - Motr conf. The conf module provides the list of remote DTM0 services.

   <hr>
   @section DTM0BR-highlights Design Highlights
   <i>Mandatory. This section briefly summarizes the key design
   decisions that are important for understanding the functional and
   logical specifications, and enumerates topics that need special
   attention.</i>

   Each DTM0 service has a recovery machine. Each recovery machine contains
   N recovery FOMs (one per counterpart + one local). Recovery FOMs start
   listen to various events (HA events, DTM0 log events, RPC replies) and
   act accordingly (sending out M0_CONF_HA_PROCESS_* events and REDO messages).

   @verbatim

   +-------------------------------------------+
   | Recoverable process, process fid = 1      |
   | +---------------------------------------+ |
   | | DTM0 Service                          | |
   | |                                       | |
   | | +-----------------------------------+ | |
   | | | Recovery machine                  | | |
   | | |                                   | | |
   | | | +--------+ +--------+ +--------+  | | |
   | | | | R_FOM1 | | R_FOM2 | | R_FOM3 |  | | |
   | | | | FID=1  | | FID=2  | | FID=3  |  | | |
   | | | +--------+ +--------+ +--------+  | | |
   | | |                                   | | |
   | | +-----------------------------------+ | |
   | +---------------------------------------+ |
   +-------------------------------------------+

       Recovery machines of one of the processes of a 3-process cluster.
     The other two processes have the same set of FOMs. FID=N means that
     the id of this recovery FOM is N. The first one (R_FOM1) is the local
     recovery FOM of this DTM0 service. The other two (R_FOM2, R_FOM3) are
     remote recovery FOMs that would send REDOs to the processes 2 and 3.

   @endverbatim

   Each recovery FOM may await on external events:

   @verbatim

   +-----------------------------------------------------+
   |  Recovery FOM inputs                                |
   |                                                     |
   |                      Pollling                       |
   |   ----------------------------------------------    |
   |   |  HA queue |  | DTM0 log |  | RPC replies   |    |
   |   |           |  | watcher  |  |               |    |
   |   |    /|\    |  |          |  |     /|\       |    |
   |   |     |     |  |   /|\    |  |      |        |    |
   |   |     |     |  |    |     |  |      |        |    |
   |   |  conf obj |  | DTM0 log |  | DTM0 RPC link |    |
   +-----------------------------------------------------+

   @endverbatim

   For example, when a recovery FOM performs recovery of a remote
   process, it may await on HA queue (to halt sending of REDOs), on
   DTM0 log watcher (to learn if there are new entries in the log),
   or on DTM0 RPC link (to learn if next REDO message can be sent).

   Whenever a recovery FOM receives an event from the HA queue, it gets
   "reincarnated" to serve its particular role in recovery procedures
   (see "Recovery FOM reincarnation" definition).
   Incarnation of a recovery FOM is just a sub-state-machine of the FOM.
   For example, here is the full state machine of R_FOM2:

   @verbatim

   INIT -> AWAIT_HEQ (HA event queue)

   AWAIT_HEQ -> DECIDE_WHAT_TO_DO
   DECIDE_WHAT_TO_DO -> AWAIT_HEQ
   DECIDE_WHAT_TO_DO -> RESET
   DECIDE_WHAT_TO_DO -> FINAL

   RESET -> SEND_REDO

   SEND_REDO -> AWAIT_HEQ_OR_LOG
   SEND_REDO -> AWAIT_HEQ_OR_RPC

   AWAIT_HEQ_OR_LOG -> SEND_REDO
   AWAIT_HEQ_OR_LOG -> HEQ

   AWAIT_HEQ_OR_RPC -> SEND_REDO
   AWAIT_HEQ_OR_RPC -> HEQ

   INIT -------------->  AWAIT_HEQ
                         |     /|\
			\|/     |
                        DECIDE_WHAT_TO_DO --------------> FINAL
                         |    /|\
			 |     |               (remote recovery
                         |     |                 sub-states)
   //====================|=====|===================================//
   //  RESET <-----------+     +---< HEQ      (Transitions that    //
   //    |                                     may happen as       //
   //    |                                     a reaction to       //
   //    |    +------------------------+       RECOVERING HA event)//
   //   \|/  \|/                       |                           //
   //  SEND_REDO ---------------> AWAIT_ON_HEQ_OR_RPC ---> HEQ     //
   //      |  /|\                                                  //
   //     \|/  |                                                   //
   //     AWAIT_ON_HEQ_OR_LOG -----> HEQ                           //
   //==============================================================//

                        ...   ...
			 |    |             (eviction sub-states)
   //====================|====|====================================//
   //   RESET <----------+    +----< HEQ     (Transitions that     //
   //    |                                    may happen as a      //
   //    |    +------------------+            a reaction to FAILED //
   //   \|/  \|/                 |            HA event)            //
   //   SEND_REDO -----> AWAIT_ON_HEQ_OR_RPC ----> HEQ             //
   //==============================================================//
   @endverbatim

   The first frame highlighted with "===//" shows the sub-state machine of
   a recovery FOM that are used to recover a participant that entered
   RECOVERING HA state. If the same participant enters FAILED state
   (permanent failure) then another set of sub-states would be involved
   in recovery procedures (the second frame).

   On the diagram above, the transition DECIDE_WHAT_TO_DO -> RESET
   marks the point where recovery FOM enters a new incarnation,
   the transtion HEQ -> DECIDE_WHAT_TO_DO marks the end of the live of this
   incaration. The set of sub-states is called "role".
   For example, if a recovery FOM is somewhere inside the first frame
   (remote recovery) then we say that it has "remote recovery role" and it
   performs "remote recovery". Correspondingly, the second frame describes
   "eviction role" or just "eviction".

   The notion of incarnation is used to emphasise that a recovery FOM must
   always "clean up" its volatile state before processing next HA event.
   For example, it may await on a pending RPC reply or it may have to reset
   DTM0 log iterator.

   The local recovery FOM (R_FOM1 in the example), is just a volatile
   state that captures information about certain points in the log and sends
   out ::M0_CONF_HA_PROCESS_RECOVERED when it believes that the local process
   has been fully recovered (see the recovery stop condition below).
   The local recovery FOM cannot change its role (it cannot recover itself
   from transient or permanent failures).


   <hr>
   @section DTM0BR-lspec Logical Specification
   <i>Mandatory.  This section describes the internal design of the component,
   explaining how the functional specification is met.  Sub-components and
   diagrams of their interaction should go into this section.  The section has
   mandatory subsections created using the Doxygen @@subsection command.  The
   designer should feel free to use additional sub-sectioning if needed, though
   if there is significant additional sub-sectioning, provide a table of
   contents here.</i>

   - @ref DTM0BR-lspec-comps
   - @ref DTM0BR-lspec-rem-fom
   - @ref DTM0BR-lspec-loc-fom
   - @ref DTM0BR-lspec-evc-fom
   - @ref DTM0BR-lspec-state
   - @ref DTM0BR-lspec-thread
   - @ref DTM0BR-lspec-numa

   @subsection DLD-lspec-comps Component Overview
   <i>Mandatory.
   This section describes the internal logical decomposition.
   A diagram of the interaction between internal components and
   between external consumers and the internal components is useful.</i>

   The following diagram shows connections between recovery machine and
   the other components of the system:

   @dot
	digraph {
		rankdir = LR;
		bgcolor="lightblue"
		node[shape=box];
		label = "DTM0 Basic recovery machine components and dependencies";
		subgraph cluster_recovery_sm {
			label = "Recovery SM";
			bgcolor="lightyellow"
			node[shape=box];
			rfom[label="Recovery FOM"];
			E1[shape=none  label="&#8942;" fontsize=30]
			rfom_another[label="Another Recovery FOM"];
		}

		subgraph cluster_ha_conf {
			node[shape=box];
			label ="HA and Conf";
			conf_obj[label="Conf object"];
			ha[label="Motr HA"];
		}

		drlink[label="DTM0 RPC link"];
		dlog[label="DTM0 log"];

		conf_obj -> rfom[label="HA decisions"];
		rfom -> ha [label="Process events"];
		rfom -> drlink [label="REDO msgs"];
		dlog -> rfom[label="records"];
	}
   @enddot
--->>>
Max: [* defect *] Communication with other recovery foms in the same process and
     in different processes are missing. Please add.
<<<---

   The following sequence diagram represents an example of linerisation of
   HA decisions processing:
   @msc
   ha,rfom, remote;
   ha -> rfom [label="RECOVERING"];
   rfom -> remote [label="REDO(1)"];
   ha -> rfom [label="TRANSIENT"];
   remote -> rfom [label="REDO(1).reply(-ETIMEOUT)"];
   ha -> rfom [label="RECOVERING"];
   rfom -> rfom [label="Cancel recovery"];
   rfom -> remote [label="REDO(1)"];
   remote -> rfom [label="REDO(1).reply(ok)"];
   @endmsc

   In this example the recovery machine receives a sequence of events
   (TRANSIENT, RECOVERING, TRANSIENT), and reacts on them:
   - Starts replay the log by sending REDO(1).
   - Awaits on the reply.
   - Cancels recovery.
   - Re-starts replay by sending REDO(1) again.
   - Get the reply.
--->>>
Max: [* defect *] One example is not enough to describe the algorithm. Please
     explain how it's done exactly. Maybe not in this section, but somewhere in DLD.
<<<---

   @subsection DTM0BR-lspec-rem-fom Remote recovery FOM
   <i>Such sections briefly describes the purpose and design of each
   sub-component.</i>
--->>>
Max: [* defect *] State machine is there, but what the fom actually does is
     missing. Please add.
<<<---

   When a recovery FOM detects a state transition to RECOVERING of a remote
   participant, it transits to the sub-SM called "Remote recovery FOM".
   In other words, it re-incarnates as "Remote recovery FOM" as soon as
   the previous incarnation is ready to reach its "terminal" state.

   Remote recovery FOM acts as a reactor on the following kinds of events:
   - Process state transitions (from HA subsystem);
   - DTM0 log getting new records;
   - RPC replies (from DTM0 RPC link component).

   Remote recovery FOM has the following states and transitions:

   @verbatim
   REMOTE_RECOVERY(initial) -> WAITING_ON_HA_OR_RPC
   REMOTE_RECOVERY(initial) -> WAITING_ON_HA_OR_LOG
   WAITING_ON_HA_OR_RPC     -> WAITING_ON_HA_OR_LOG
   WAITING_ON_HA_OR_LOG     -> WAITING_ON_HA_OR_RPC
   WAITING_ON_HA_OR_RPC     -> NEXT_HA_STATE (terminal)
   WAITING_ON_HA_OR_LOG     -> NEXT_HA_STATE (terminal)
   WAITING_ON_HA_OR_RPC     -> SHUTDOWN (terminal)
   WAITING_ON_HA_OR_LOG     -> SHUTDOWN (terminal)

       Where:
       REMOTE_RECOVERY is a local initial state (local to the sub-SM);
       NEXT_HA_STATE is a local terminal state;
       SHUTDOWN is a global terminal state.
   @endverbatim

   @subsection DTM0BR-lspec-loc-fom Local recovery FOM
   <i>Such sections briefly describes the purpose and design of each
   sub-component.</i>

   Local recovery is used to ensure fairness of overall recovery procedure.
--->>>
Max: [* defect *] Definition of "fairness" in this context is missing. Please
     add.
<<<---
--->>>
Max: [* defect *] Definition of the "fairness" is "ensured" is missing. Please
     add.
<<<---
   Whenever a participant learns that it got all missed log records, it sends
   M0_CONF_HA_PROCESS_RECOVERED process event to the HA subsystem.

   The point where this event has to be sent is called "recovery stop
   condition". The local participant (i.e., the one that is beign in
--->>>
Max: [* typo *] being
<<<---
   the RECOVERING state) uses the information about new incoming W-requests,
   the information about the most recent (txid-wise) log entries on the other
   participants to make a decision whether recovery shall be stopped or not.

   TODO: describe the details on when and how it should stop.
--->>>
Max: [* defect *] You're right, the details are missing.
<<<---


   @subsection DTM0BR-lspec-evc-fom Eviction FOM
   <i>Such sections briefly describes the purpose and design of each
   sub-component.</i>
--->>>
Max: [* defect *] The purpose of the eviction fom is not described.
<<<---

   A recovery FOM re-incarnates as eviction FOM (i.e., enters the initial
--->>>
Max: [* defect *] A definition of "re-incarnation" is missing.
<<<---
   of the corresponding sub-SM) when the HA subsystem notifies about
--->>>
Max: [* defect *] A definition of sub-SM is missing.
<<<---
   permanent failure (FAILED) on the corresponding participant.

   The local DTM0 log shall be scanned for any record where the FAILED
--->>>
Max: [* doc *] Please add a reference to the component which does the scanning.
<<<---
--->>>
Max: [* doc *] Please explain how to scanning is done:
     - sequentially?
     - from least recent to most recent?
     - one fom/thread/whatever or multiple? How the work is distributed?
     - locks also have to be described somewhere in DLD.
<<<---
   participant participated. Such a record shall be replayed to the
   other participants that are capable of receiving REDO messages.
--->>>
Max: [* defect *] What to do with the message if a participant is in TRANSIENT
     state is not described.
<<<---

   When the log has been replayed completely, the eviction FOM notifies
--->>>
Max: [* defect *] Criteria of "log has been replayed completely" is missing.
     Please add.
<<<---
   the HA subsystem about completion and leaves the sub-SM.
--->>>
Max: [* question *] How? Please also describe what is expected from HA.
<<<---

   TODO: GC of FAILED processes and handling of clients restart.

   @subsection DTM0BR-lspec-state State Specification
   <i>Mandatory.
   This section describes any formal state models used by the component,
   whether externally exposed or purely internal.</i>

   The states of a recovery FOM is defined as a collection of the sub-SM
   states, and a few global states.

   TODO: state diagram for overall recovery FOM.
--->>>
Max: [* doc *] Also distributed state diagram.
<<<---

   @subsection DTM0BR-lspec-thread Threading and Concurrency Model
   <i>Mandatory.
   This section describes the threading and concurrency model.
   It describes the various asynchronous threads of operation, identifies
   the critical sections and synchronization primitives used
   (such as semaphores, locks, mutexes and condition variables).</i>

   Recovery machine revolves around the following kinds of threads:
--->>>
Max: [* defect *] Definition of "revolves" is missing.
<<<---
   - Locality threads (Recovery FOMs, DTM0 log updates).
   - RPC machine thread (RPC replies).

   Each recovery FOM is implemented using Motr coroutines library.
   Within the coroutine ::m0_be_op and/or ::m0_co_op is used to await
--->>>
Max: [* question *] RE: "the coroutine": which one?
<<<---
   on external events.
--->>>
Max: [* suggestion *] A list of such events would help to understand why be/co
     ops are needed.
<<<---

   DTM0 log is locked using a mutex (TBD: the log mutex or a separate mutex?)
--->>>
Max: [* question *] Entire DTM0 log is locked using a mutex? If no, please add
     unambiguous sentence. Example: "<pointer to a watcher ot whatever> is
     protected with a mutex" or "a separate mutex protects concurrent access to
     <a field>".
<<<---
   whenever recovery machine sets up or cleans up its watcher.
--->>>
Max: [* defect *] The watcher description/workflow/etc. is missing. Please add.
<<<---

   Interraction with RPC machine is wrapped by Motr RPC library and DTM0
--->>>
Max: [* typo *] s/Interraction/Interaction/
<<<---
   RPC link component. It helps to unify the type of completion objects
   across the FOM code.

   TODO: describe be/co op poll/select.

   @subsection DTM0BR-lspec-numa NUMA optimizations
   <i>Mandatory for components with programmatic interfaces.
   This section describes if optimal behavior can be supported by
   associating the utilizing thread to a single processor.</i>

   There is no so much shared resources outside of DTM0 log and
   the localities. Scaling and batching is outside of the scope
--->>>
Max: [* defect *] The number of localities is equal to the number of CPU cores
in our default configuration, so whatever uses more than one locality has to
have some kind of synchronisation.
<<<---
--->>>
Max: [* defect *] It's not clear what "scaling" and "batching" mean in this
     context. Please explain and/or add a reference where they are explained.
<<<---
   of this document.
--->>>
Max: [* defect *] FOM locality assignment is missing.
<<<---

   <hr>
   @section DTM0BR-conformance Conformance
   <i>Mandatory.
   This section cites each requirement in the @ref DTM0BR-req section,
   and explains briefly how the DLD meets the requirement.</i>
--->>>
Max: [* defect *] Top-level requirements from HLD are missing.
<<<---

   - @b I.DTM0BR.HA-Aligned Recovery machine provides an event queue that
   is beign consumed orderly by the corresponding FOM.
--->>>
Max: [* defect *] The word "queue" is present in this document only here. Please
     describe the event queue and events somewhere in this DLD.
<<<---
--->>>
Max: [* defect *] It's not clear how "HA subsystem" from the requirement is met
     by the implementation.
<<<---
   - @b I.DTM0BR.Basic Recovery machine supports only basic log replay defined
   by the remote recovery FOM and its counterpart.
--->>>
Max: [* defect *] "basic" term is not defined. Please define.
<<<---
   - @b I.DTM0BR.No-Batching Recovery machine achieves it by awaiting on
   RPC replies for every REDO message sent.
--->>>
Max: [* defect *] Maybe I should wait for I.* for performance and availability
     requirements, but as of now it's not clear how DTM recovery would catch up
     with the rest of the nodes creating new DTM0 transactions in parallel with
     DTM recovery.
<<<---

   <hr>
   @section DTM0BR-usecases
   <i>Mandatory. This section describes use-cases for recovery machine.
   </i>

   TODO: Add use-cases.
--->>>
Max: [* defect *] Right, they are missing.
<<<---


   <hr>
   @section DLD-O Analysis
   <i>This section estimates the performance of the component, in terms of
   resource (memory, processor, locks, messages, etc.) consumption,
   ideally described in big-O notation.</i>
--->>>
Max: [* doc *] Please fill this section with references to the requirements.
<<<---

   <hr>
   @section DLD-ref References
   <i>Mandatory. Provide references to other documents and components that
   are cited or used in the design.
   In particular a link to the HLD for the DLD should be provided.</i>

   - [1] <a href="https://github.com/Seagate/cortx-motr/blob/documentation/doc/dev/dtm/dtm-hld.org">
   DTM0 HLD</a>
   - [2] <a href="https://en.wikipedia.org/wiki/Conflict-free_replicated_data_type">

   <hr>
   @section DLD-impl-plan Implementation Plan
   <i>Mandatory.  Describe the steps that should be taken to implement this
   design.</i>

   - Develop pseudocode-based description of recovery acivities.
   - Identify and add missing concurrency primitives (select/poll for be op).
   - Use the pseudocode and new primitives to implement a skeleton of the FOMs.
   - Incorporate RECOVERING and TRANSIENT states with their
   new meaning as soon as they are available in the upstream code.
   - Populate the list of use-cases based on available tools.
   - Populate the list of system tests.
   - Improve the stop condition based on the use-cases and tests.
   - Add implementation of eviction FOM.
--->>>
Max: [* defect *] No mention of tests being done. Please clearly state when and
     how the tests are going to be done or where and when it would be possible
     to find this information.
<<<---
 */


#define M0_TRACE_SUBSYSTEM M0_TRACE_SUBSYS_DTM0
#include "lib/trace.h"
#include "dtm0/recovery.h"

/*
 * This the draft of the DLD. It is left here for refence while the DLD
 * is being formalised. It will be removed before formal review.
 *
 *
 * DTM0 basic recovery DLD
 *
 * Definition
 * ----------
 *
 *   DTM0 basic recovery defines a set of states and transitions between them
 * for each participant of the cluster considering only a limited set of
 * usecases.
 *
 *
 * Assumptions (limitations)
 * ---------------------------
 *
 *  A1: HA (Cortx-Hare or Cortx-Hare+Cortx-HA) provides EOS for each event.
 *  A2: HA provides total ordering for all events in the system.
 *  *A3: DTM0 handles HA events one-by-one without filtering.
 *  A4: DTM0 user relies on EXECUTED and STABLE events to ensure proper ordering
 *      of DTXes.
 *  A5: There may be a point on the timeline where data is not fully replicated
 *      across the cluster.
 *  A6: RPC connections to a FAILED/TRANSIENT participant are terminated as soon
 *      as HA sends the corresponding event.
 *  A7: HA epochs are not exposed to DTM0 even if HA replies on them.
 *
 *
 * HA states and HA events
 * -----------------------
 *
 *   Every participant (PA) has the following set of HA states:
 *     TRANSIENT
 *     ONLINE
 *     RECOVERING
 *     FAILED
 * States are assigned by HA: it delivers an HA event to every participant
 * whenever it decided that one of the participants has changed its state.
 *   Motr-HA subsystem provides the "receiver-side" guarantee to comply with A1.
 * It has its own log of events that is being updated whenever an HA event
 * has been fully handled.
 *
 *
 * State transitions (HA side)
 * ---------------------------
 *
 * PROCESS_STARTING: TRANSIENT -> RECOVERING
 *   HA sends "Recovering" when the corresponding
 *   participant sends PROCESS_STARTED event to HA.
 *
 * PROCESS_STARTED: RECOVERING -> ONLINE
 *   When the process finished recovering from a failure, it sends
 *   PROCESS_STARTED to HA. HA notifies everyone that this process is back
 *   ONLINE.
 *
 * <PROCESS_T_FAILED>: ONLINE -> TRANSIENT
 *   When HA detects a transient failure, it notifies everyone about it.
 *
 * <PROCESS_FAILED>: [ONLINE, RECOVERING, TRANSIENT] -> FAILED
 *   Whenever HA detects a permanent failure, it starts eviction of the
 *   corresponding participant from the cluster.
 *
 *
 * HA Events (Motr side)
 * ---------------------
 *
 *   PROCESS_STARTING is sent when Motr process is ready to start recovering
 * procedures, and it is ready to serve WRITE-like requests.
 *   PROCESS_STARTED is sent when Motr process finished its recovery procedures,
 * and it is ready to serve READ-like requests.
 *
 *
 * State transitions (DTM0 side)
 * -----------------------------
 *
 *  TRANSIENT -> RECOVERING: DTM0 service launches a recovery FOM. There are
 *    two types of recovery FOMs: local recovery FOM and remote recovery FOM.
 *    The type is based on the participant that has changed its state.
 *  RECOVERING -> ONLINE: DTM0 service stops/finalises recovery FOMs for the
 *    participant that has changed its state.
 *  ONLINE -> TRANSIENT: DTM0 service terminates DTM0 RPC links established
 *    to the participant that has changed its state (see A6).
 *  [ONLINE, RECOVERING, TRANSIENT] -> FAILED: DTM0 service launches a recovery
 *    FOM to handle eviction (remote eviction FOM).
 *
 *
 * DTM0 recovery FOMs
 * ------------------
 *
 *   Local recovery FOM. A local recovery FOM is supposed to handle recovery
 * of the process where the fom has been launched. The FOM sends
 * PROCESS_STARTING event to HA to indicate its readiness to receive REDOs.
 * When the recovery stop condition is satisfied, the FOM causes
 * PROCESS_STARTED event.
 *   Remote recovery FOM. This FOM uses the local DTM0 log to replay DTM0
 * transactions to a remote participant. The FOM launched and stopped based
 * on the HA events.
 *   Remote eviction FOM. The FOM replays every log record to every participant
 * (where this log record is not persistent) if the FAILED participant belongs
 * the list of participants of the corresponding transaction descriptor
 * (or if it is the originator of the transaction). Note, eviction happens
 * without any state transitions of the other participants.
 *
 * DTM0 recovery stop condition
 * ----------------------------
 *   The stop condition is used to stop recovery FOMs and propagate state
 * transitions to HA. Recovery stops when a participant "catches up" with
 * the new cluster state. Since we allow WRITE-like operations to be performed
 * at almost any time (the exceptions are TRANSIENT and FAILED states of
 * of the target participants), the stop condition relies on the information
 * about the difference between the DTM0 log state when recovery started (1)
 * and the point where recovery ended (2). The second point is defined using
 * so-called "recovery stop condition". Each recovery FOM has its own condition
 * described in the next sections.
 *   Note, HA epochs would let us define a quite precise boundaries for the
 * stop. However, as it is stated in A7, we cannot rely on them. Because of
 * that, we simply store the first WRITE-like operation on the participant
 * experiencing RECOVERING, so that this participant can make a decision
 * whether recovery has to be stopped or not.
 *
 *
 * Stop condition for local FOM
 * ----------------------------
 *
 *   The local recovery FOM is created before the local process sends
 * PROCESS_STARTED event the HA. Then, it stores (in volatile memory) the first
 * WRITE-like operation that arrived after the FOM was created. Since clients
 * cannot send WRITE-like operations to servers in TRANSIENT or FAILED state,
 * and leaving of such a state happens only a local recovery FOM is created,
 * it helps to avoid a race window, as it is shown below:
 *
 * @verbatim
 *   P1.HA     | P1.TRANSIENT        | P1.RECOVERING
 *   P1.Motr   | <down> | <starting> |
 *   P1.DTM    | <down>                          | <started recovery FOM>
 *   C1.HA         | P1.TRANSIENT          | P1.RECOVERING
 *   C1.Motr   | <ongoing IO to other PAs>  | <sends KVS PUT to P1>
 *             -------------------------------------------------> real time
 *                                   1    2 3    4
 *
 *   (1) HA on P1 learns about the state transition.
 *   (2) HA on P2 learns about the state transition.
 *   (3) Client sends KVS PUT (let's say it gets delivered almost instantly).
 *   (4) A recovery FOM was launched on P1. It missed the KVS PUT.
 * @endverbatim
 *
 *   The stop condition for a local recovery FOM can be broken down to the
 * following sub-conditions applied to every ONLINE-or-TRANSIENT participant
 * of the cluster:
 *   1. Caught-up: participant sends a log record that has the same TX ID as
 *   the first operation recorded by the local recovery FOM.
 *   2. End of log: participant send "end-of-the-log" message.
 * In other words, a participant leaves RECOVERING state when it got REDOs
 * from all the others until the point where new IO was started. Note, there
 * are a lot of corner cases related to interleaving and clock synchronisation
 * but they are not covered here.
 *
 *
 * Stop condition for remote FOM
 * -----------------------------
 *
 *   The next question is "when a remote process should stop sending REDOs?".
 * This question defines the stop condition for a remote recovery FOM. Since
 * there is a race window between the point where a remote process gets
 * RECOVERING event and the point where one of the clients sends a "new"
 * WRITE-like operations, remote participants are not able to use the
 * "until first WRITE" condition. Instead, they rely on HA state transitions.
 * When a participant leaves RECOVERING state, all remote recovery FOMs should
 * be stopped.
 *   When a remote recovery FOM reaches the end of the local DTM0 log, it
 * goes to sleep. It gets awaken when a new record appears in the log. As it
 * was mentioned above, this kind of FOM can be stopped only when HA notifies
 * about state transition of the participant that is being recovered.
 *
 * Stop condition for eviction FOM
 * -------------------------------
 *
 *   The final question is how to stop eviction of a FAILED participant. The
 * stop condition here is that all the records where the participant
 * participated should be replayed to the other participants.
 *   An eviction FOM stops after all the needed log records were replayed.
 *
 *
 * REDO message
 * ------------
 *
 *   REDO messages contain full updates (as opposite to PERSISTENT messages
 * that carry only partial updates). They are sent one-by-one by a
 * recover FOM (remote/eviction). A REDO message is populated using
 * a DTM0 log record. User-specific service provides necessary callbacks
 * to transmute the message if it is required.
 *   A log record can be removed only by the local log pruner daemon.
 * DTM0 log locks or the locality lock can be used to avoid such kind of
 * conflicts (up to the point where a shot-lived lock can be taken to make a
 * copy of a log record).
 *   The REDO message is just a wrapper around user-specific WRITE-like
 * operation (for example, CAS op). Aside from user request, it may contain
 * the most recent information about the states of the transaction on the
 * other participants and information that helps to identify the sender
 * DTM0 service:
 * @verbatim
 *   REDO message request {
 *      struct pa_state latest_states[];
 *      uint64_t        source;
 *      struct buf      original_request;
 *   }
 *   REDO message reply {
 *      struct buf      reply_to_original_request;
 *   }
 * @endverbatim
 *   The receiver should send back a packed reply or an empty reply. The reply
 * serves two purposes:
 *   - Establishes back-pressure. As it was described above, a remote recovery
 *   or eviction FOM sends packets one-by-one awaiting replies each time. This
 *   could be a subject to some batching policy in future.
 *   - Helps to detect failures. For example, if one of the participants
 *   reports an error (ENOMEM or ENOENT) then it may mean that HA should
 *   put the participant into TRANSIENT state (by rebooting the OS process)
 *   or even mark it as failed (for example, if a certain index does
 *   not exist there).
 *
 *
 * REDO FOM
 * --------
 *
 *   Whenever a REDO message is received, DTM0 service schedules a REDO FOM.
 * The FOM creates a user-specific FOP (for example, CAS FOP), then it creates
 * an FOM for this FOP (let's call it uFOM). DTM0 service uses fom_interpose
 * API to execute the uFOM. Upon uFOM completion, the REDO FOM sends back
 * an RPC reply with REDO message reply FOP.
 *   Additional information carried by a REDO fop may be applied separately
 * to the DTM0 log.
 *
 *
 * Recovery and HA EOS
 * -------------------
 *
 *   HA EOS is needed to replay the events sent to the local participant
 * in order to "catch up" with the most recent state of the cluster.
 * If some the events were "consumed" (committed) by DTM0 then
 * they should not be replayed.
 *   Here is a list of allowed state transitions for a participant:
 *     TRANSIENT  -> RECOVERING (1)
 *     TRANSIENT  -> FAILED     (2)
 *     RECOVERING -> ONLINE     (3)
 *     RECOVERING -> FAILED     (4)
 *     RECOVERING -> TRANSIENT  (5)
 *     ONLINE     -> TRANSIENT  (6)
 *     ONLINE     -> FAILED     (7)
 *     FAILED     -> EVICTED    (*)
 *   Note that replay if the local HA log may be required only if
 * the participant enters TRANSIENT state: all other transitions happen
 * either without losing of the volatile state of the participant or
 * with losing of its permanent state (which means its volatile state
 * cannot be recovered at all).
 *   When a participant enters TRANSIENT (5 and 6), its previous transitions
 * (1, 3, 4, 5) get "consumed".
 *   When a participant leaves RECOVERING (3, 4, 5), its previous transition
 * (1) gets "consumed".
 *   When a participant enters FAILED, its previous transitions (1, 3, 5, 6)
 *  get "consumed".
 *   When a participant leaves FAILED (*), its previous transition (7) gets
 * "consumed". Note, this transition is not well-defined yet. If the transition
 * (7) causes every participant enter RECOVERING (i.e, ONLINE -> RECOVERING
 * is allowed) then this is not required, so that any transition to FAILED
 * gets "consumed" as soon as it gets delivered.
 *
 *
 * Originators' reaction to TRANSIENT failures
 * -------------------------------------
 *
 *  When one of the participants with persistent state enters TRANSIENT,
 * an originator (a participant without persistent state) must force
 * the DTXes that contain this TRANSIENT participant to progress without
 * waiting on a reply ("EXECUTED") or on a DTM0 PERSISTENT message.
 *  Originator's DTM0 service participates in recovery process in the
 * same way as the other types of participants.
 *
 *
 * Errata and comments
 * -------------------
 *
 *  1. PROCESS_RECOVERED is be used as an indicator of completeness.
 *     The existing states (PROCESS_START{ING,ED}) keep their original
 *     meaning.
 *  2. REDO may contain extra information about the last seen TXD or about
 *     the point where the process received the corresponding * -> RECOVERING
 *     state transition event.
 *  3. "REDO messages are sent one-by-one" is only a starting point. They
 *     can be one-way messages (with DTM0 log-based re-sends) but it has
 *     to be added at the top of basic recovery.
 *  4. WRITE-like opeartion means actual PUT/DEL requrests for the very first
 *     version of DTM0 (it should support only DIX).
 *  5. Eviction is not fully covered by this document.
 */

/* start of recovery machine pseudo-code */
#if 1
/* ------------------------8< recovery.h ------------------------------------ */
/* imports */
struct m0_dtm0_service;
struct m0_fid;
enum m0_ha_obj_state;

/* exports */
struct m0_dtm0_recovery {
	struct m0_dtm0_service *re_svc;
	struct m0_tl            re_foms;
};

M0_INTERNAL void m0_dtm0_recovery_domain_init(void);
M0_INTERNAL void m0_dtm0_recovery_domain_fini(void);

M0_INTERNAL void m0_dtm0_recovery_init(struct m0_dtm0_recovery *recovery,
				       struct m0_dtm0_service  *dtms);
M0_INTERNAL void m0_dtm0_recovery_fini(struct m0_dtm0_recovery *recovery);

/* ------------------------8< recovery.c ------------------------------------ */

#define M0_DTM0_RMACH_MAGIC 0x3AB0DBED
#define M0_DTM0_PMACH_HEAD_MAGIC 0xB0EB0D11

struct m0_co_fom {
	struct m0_fom        cf_base;
	struct m0_co_context cf_coctx;
	struct m0_co_op      cf_await;
};

struct m0_dtm0_log_cursor {
	struct m0_dtm0_log     *c_log;
	struct m0_dtm0_tx_desc  c_txd;
	struct m0_buf           c_payload;
};

static void m0_dtm0_log_cursor_init(void);
static bool m0_dtm0_log_cursor_has_next(void);
static void m0_dtm0_log_cursor_next(void);
static void m0_dtm0_log_cursor_get(void);

enum {
	/*
	 * Number of HA event that could be submitted by the HA subsystem
	 * at once; where:
	 *   "HA event" means state tranistion of a single DTM0 process;
	 *   "at once" means the maximal duration of a tick of any FOM
	 *     running on the same locality as the recovery FOM that
	 *     handles HA events.
	 * XXX: The number was chosen randomly. Update the previous sentence
	 *      if you want to change the number.
	 */
	HA_EVENT_MAX_NR = 32,
};

struct recovery_task {
	struct m0_sm              rt_sm;
	struct m0_sm_group        rt_sm_group;

	struct m0_tlink           rt_linkage;
	struct m0_fid             rt_target;

	struct m0_fom             rt_fom;

	struct m0_dtm0_log_cursor rt_log_cursor;
#if 0
	struct m0_co_fom          rt_cf;
	struct m0_dtm0_tid        rt_first_seen;
	struct m0_dtm0_tid        rt_last_seen;
	struct m0_dtm0_log_cursor rt_cursor;
#endif
	struct m0_clink            rt_ha_clink;

	/* HA event queue populated by the clink and consumed by the FOM. */
	struct m0_be_queue         rt_heq;
};

struct ha_event {
	enum m0_ha_obj_state state he_state;
	struct m0_sm_ast           he_ast;
};

M0_TL_DESCR_DEFINE(rtask, "recovery_task",
		   static, struct recovery_task, rt_linkage,
		   rt_magic, M0_DTM0_RMACH_MAGIC, M0_DTM0_RMACH_HEAD_MAGIC);
M0_TL_DEFINE(rtask, static, struct recovery_task);


static const m0_fid *fom_target(struct m0_fom *);

M0_INTERNAL void m0_dtm0_recovery_init(struct m0_dtm0_recovery *r,
				       struct m0_dtm0_service  *svc);
M0_INTERNAL void m0_dtm0_recovery_fini(struct m0_dtm0_recovery *r);

struct m0_reqh *m0_dtm0_recovery_reqh(struct m0_dtm0_recovery *r);

static const struct m0_fid *local_fid(const struct m0_dtm0_recovery *r);
static struct m0_fom *local_fom_alloc(struct m0_dtm0_recovery *r);
static struct m0_fom *remote_fom_alloc(struct m0_dtm0_recovery *r,
				       const struct m0_fid     *target);

#define F M0_CO_FRAME_DATA

static struct recovery_task *fom2rt(struct m0_fom *fom)
{
	struct recovery_task *rt = M0_AMB(rt, fom, rt_fom);
	return rt;
}

static struct m0_co_context *CO(struct m0_fom *fom)
{
	return &fom2rt(fom)->rt_context;
}

M0_INTERNAL void m0_be_dtm0_log_on_new_record(struct m0_be_dtm0_log *log,
					      struct m0_clink       *clink);

static void log_add_clink_cb(struct m0_clink *clink);

struct m0_be_op_poll {
	struct m0_be_op  bop_super;
	struct m0_be_op *bop_set;
	uint64_t         bop_set_nr;
	uint64_t         bop_done_mask;
};

M0_INTERNAL int m0_be_op_await(struct m0_be_op *op,
			       struct m0_fom   *fom,
			       int              next_state)
{
	enum m0_fom_phase_outcome ret = M0_FSO_AGAIN;

	m0_be_op_lock(op);
	M0_PRE(M0_IN(op->bo_sm.sm_state, (M0_BOS_ACTIVE, M0_BOS_DONE)));

	if (op->bo_sm.sm_state == M0_BOS_ACTIVE) {
		ret = M0_FSO_WAIT;
		m0_fom_wait_on(fom, &op->bo_sm.sm_chan, &fom->fo_cb);
	}
	m0_be_op_unlock(op);
	return ret;
}

static void m0_be_op_poll_cb(struct m0_be_op *op, void *param)
{
	int                i;
	struct m0_be_poll *poll = param;

	for (i = 0; i < poll->bop_set_nr; ++i)
		if (op == poll->bop_set + i)
			break;

	M0_ASSERT(i < poll->bop_set_nr);

	m0_be_op_lock(&poll->super);
	poll->bop_done_mask |= M0_BITS(i);
	if (!m0_be_op_is_done(&poll->super))
		m0_sm_state_set(&poll->super.bo_sm);
	m0_be_op_unlock(&poll->super);
}

static void m0_be_op_poll_init(struct m0_be_op_poll *poll,
			       struct m0_be_op      *opset,
			       uint64_t              opset_nr,
			       uint64_t              mask)
{
	int              i;
	struct m0_be_op *super = &poll->bop_super;

	poll->bop_set_nr = opset_nr;
	poll->bop_set = opset;
	poll->bop_done_mask = 0;

	m0_be_op_init(super);

	m0_be_op_lock(super);

#define forall_forward \
	for (i = 0; i < opset_nr; ++i) \
		if (M0_BITS(i) & mask)

#define forall_backward \
	for (i = opset_nr; i > 0; --i) \
			if (M0_BITS(i) & mask)

	forall_forward {
		m0_be_op_lock(opset + i);
	}

	forall_forward {
		if (m0_be_op_is_done(opset + i)) {
			poll->bop_done_mask |= M0_BITS(i);
			m0_sm_state_set(&super->bo_sm);
			break;
		}
		else
			m0_be_op_callback_set(opset + i, m0_be_op_poll_cb, poll,
					      M0_BOS_DONE);
	}

	if (m0_be_op_is_done(super))
		forall_forward {
			m0_be_op_callback_clear(opset + i, M0_BOS_DONE);
		}

	forall_backward {
		m0_be_op_unlock(opset + i - 1);
	}

	m0_be_op_unlock(super);
#undef forall_forward
#undef forall_backward
}

static void m0_be_op_poll_fini(struct m0_be_op_poll *poll)
{
	struct m0_be_op *super = &poll->super;
	m0_be_op_fini(super);
	M0_SET0(poll);
}


static void m0_be_op_poll_co(struct m0_fom   *fom,
			     struct m0_be_op *opset,
			     uint64_t         opset_nr,
			     uint64_t         opin_mask,
			     uint64_t        *opout_mask)
{
	M0_CO_REENTER(CO(fom),
		      struct m0_be_op_poll bop;
		      );

	m0_be_op_poll_init(&F(bop), opset, opset_nr, opin_mask);

	M0_CO_YIELD_RC(m0_be_op_await(&F(bop).super));

	*opout_mask = F(bop).bop_done_mask;

	m0_be_op_poll_fini(&F(bop));
}



static void remote_recovery(struct m0_fom *fom, enum m0_ha_obj_state *next)
{
	struct recovery_task *rt = fom2rt(fom);
	int                   i;

	enum { OP_LOG, OP_HEQ, OP_RPC, OP_NR };
	static const int OP_ALL = M0_BITS(OP_LOG, OP_HEQ, OP_RPC);

	M0_CO_REENTER(CO(fom),
		      struct m0_buf   record;
		      struct m0_be_op ops[OP_NR];
		      uint64_t        next;
		      uint64_t        who;
		      int             rc;
		      );
	F(rc) = 0;
	F(next) = 0;

	for (i = 0; i < OP_NR; ++i)
		m0_be_op_init(&F(ops[i]));

	log_iter_init(rt);

	M0_BE_QUEUE_GET(&F(ops[OP_HEQ]), &rt->rt_heq, &F(next), &got);

	while (1) {
		while (log_iter_next(rt, &F(record))) {
			rc = redo_post(fom, &F(record), &F(ops[OP_RPC]),
				       &F(rc));
			/*
			 * We should not continue operation if we are not
			 * even able to send out a DTM0 REDO FOP.
			 */
			/*
			 * XXX: We may use be_queue_peek() instead.
			 */
			M0_ASSERT(rc == 0);
			M0_CO_FUN(CO(fom),
				  m0_be_op_poll_co(F(ops), ARRAY_SIZE(F(ops)),
						   M0_BITS(OP_RPC, OP_HEQ),
						   &F(who)));
			if ((M0_BITS(OP_HEQ) & F(who)) != 0) {
				M0_CO_YIELD_RC(m0_be_op_await(fom,
						    &F(ops[OP_RPC])));
				/* TODO: finalise ops */
				goto out;
			}
			M0_ASSERT((M0_BITS(OP_RPC) & who) != 0);
			if (F(rc) != 0) {
				/* TODO: finalise rpc op */
				break;
			}
			/* TODO: reinit rpc op */
		}

		log_watcher_set(rt, &F(ops[OP_LOG]));

		M0_CO_FUN(CO(fom),
			  m0_be_op_poll_co(F(ops), ARRAY_SIZE(F(ops)),
					   M0_BITS(OP_LOG, OP_HEQ), &F(who)));
		if ((M0_BITS(OP_HEQ) & F(who)) != 0) {
			log_watcher_set(rt, NULL);
			/* TODO: finalise ops */
			break;
		}
		M0_ASSERT((M0_BITS(OP_LOG) & who) != 0);
		/* TODO: reinit log op */
	}

out:
	if (F(got)) {
		M0_ASSERT(F(next) < M0_NC_NR);
		*next = F(next);
	} else
		*next = M0_NC_UNKNOWN;
}

static void local_recovery(struct m0_fom *fom, enum m0_ha_obj_state *next)
{
	struct recovery_task *rt = fom2rt(fom);
	int                   i;

	M0_CO_REENTER(CO(fom),
		      struct m0_dtm0_tid latest[SRV][CLNT];
		      struct m0_dtm0_tid earliest_local[CLNT];
		     );

	earliest_local = m0_dtm0_log_latest_tids(log, online_clients);

	process_event_post(PROCESS_STARTED);

	while (1) {
		for (i = 0; i < CLNT; ++i) {
			if (is_none(latest[SELF][i]))
				cas_watcher_arm(new_io_watcher);
		}
		dtms_redo_watcher_arm(redo_watcher);
		recovery_timer_arm(no_io_timer, NO_IO_TIMEOUT_SEC);
		M0_BE_QUEUE_GET(heq, heq_op, next);
		M0_CO_YIELD(m0_be_op_poll(redo_watcher,
					  new_io_watcher,
					  no_io_timer,
					  heq_op));
		if (triggered(heq_op)) {
			/*
			 * Stop condition: HA forced the local process
			 * to stop recovery procedures.
			 */
			if (*next == ONLINE)
				goto forced_stop;
		}
		if (triggered(new_io_watcher)) {
			latest[SELF] = latest(latest[SELF],
					      new_io_watcher->tids);
		}
		if (triggered(redo_watcher)) {
			latest[redo_watcher->where] = redo_watcher->tids;
		}

		/*
		 * Stop condition when there is no visible IO:
		 *   If the local DTM0 service detects no IO for
		 * the given period NO_IO_TIMEOUT_SEC then it leaves
		 * RECOVERING state.
		 */
		if (triggered(no_io_timeout)) {
			if ((forall(i, CLNT, is_none(latest[LOCAL][i])) &&
			     forall(i, CLNT,
				    forall(j, SRV excluding LOCAL,
					   is_none(latest[i][j]))))) {
				goto stop;
			} else
				recovery_timer_reset(no_io_timer);
		}

		/*
		 * Stop condition in presense of ongoing IO:
		 *   If every client (originator) produced IO reqests
		 * (DTM0 Execute message) that are:
		 *   (1) later than the latest entry in the local log.
		 *   (2) visible on all ONLINE/RECOVERING servers
		 *      (participants with persistent state).
		 * then recovery shall stop when all the previous records in
		 * the local log become stable.
		 */
		if (forall(i, CLNT, earliest_local[i] < latest[LOCAL][i]) &&
		    (forall(i, SRV excluding LOCAL,
			    forall(j, CLNT,
				   latest[i][j] < latest[SELF][i])))) {
			M0_CO_FUN(log_await_until_stable(latest[LOCAL]));
			goto stop;
		}
	}

stop:
	process_event_post(PROCESS_RECOVERED);
	M0_BE_QUEUE_GET(heq, *next, got);
	M0_CO_YIELD(m0_be_op_await(heq));
	if (!got)
		*next = M0_NC_UNKNOWN;
forced_stop:
	return;
}

static void remote_eviction(struct m0_fom *fom, enum m0_ha_obj_state *next)
{
	(void) fom;
	(void) next;
	/* Not implemented yet. */
}

static void idle(struct m0_fom *fom, enum m0_ha_obj_state *next)
{
	M0_CO_REENTER(CO(fom));
	M0_BE_QUEUE_GET(heq, heq_op, next, got);
	M0_CO_YIELD(m0_be_op_await(heq_op));
	if (!got)
		*next = M0_NC_UNKNOWN;
}


static void recovery_fom_launch(struct m0_fom *fom, enum m0_ha_obj_state state)
{
	struct recovery_task *rt = fom2rt(fom);

	M0_CO_REENTER(CO(fom);
		      enum m0_ha_obj_state next;
		      );

	F(next) = state;

	while (F(next) != M0_NC_UNKNOWN) {
		switch (F(next)) {
		case M0_NC_RECOVERING:
			if (is_local(rt))
				M0_CO_FUN(CO(fom), local_recovery(fom,
								  &F(next)));
			else
				M0_CO_FUN(CO(fom), remote_recovery(fom,
								   &F(next)));
			break;
		case M0_NC_FAILED:
			M0_ASSERT(!is_local(rt));
			M0_CO_FUN(CO(fom), remote_eviction(fom, &F(next)));
			break;
		default:
			M0_CO_FUN(CO(fom), idle(fom, &F(next)));
		}
	}
}

static void recovery_task_fom_coro(struct m0_fom *fom)
{
	struct recovery_task *rt = fom2rt(fom);
	enum m0_ha_obj_state state;

	M0_CO_REENTER(CO(fom),
		      struct m0_be_op op;
		      bool            got;
		      uint64_t        state;);

	F(enough) = false;
	m0_be_op_init(&F(op));
	M0_BE_QUEUE_GET(&F(op), &rt->rt_heq, &F(state), &got);
	M0_CO_YIELD(m0_be_op_await(&F(op)));
	if (!&F(got))
		return;
	m0_be_op_fini(&F(op));

	M0_ASSERT(F(state) < M0_NC_NR);
	state = F(state);

	M0_CO_FUN(CO(fom), recovery_fom_launch(fom, state));
}

static int recovery_task_fom_tick(struct m0_fom *fom)
{
	int rc;
	M0_CO_START(CO(fom));
	recovery_task_fom_coro(CO(fom));
	rc = M0_CO_END(CO(fom));
	M0_POST(M0_IN(rc, (0, M0_FSO_AGAIN, M0_FSO_WAIT)));
	return rc ?: M0_FSO_WAIT;
}

static void recovery_task_fini(struct recovery_task *fom)
{
	m0_clink_del_lock(&process->dop_ha_link);
	m0_clink_fini(&process->dop_ha_link);
	rtask_tlink_init(&rt->rt_linkage);
	m0_sm_group_init(&rt->rt_sm_group);
	m0_sm_init(rt->rt_sm_group, recovery_sm_conf, &rt->sm_group);
}

static bool recovery_task_clink_cb(struct m0_clink *clink)
{
	struct recovery_task *rt        = M0_AMB(rt, clink, rt_ha_clink);
	struct m0_conf_obj   *proc_conf = container_of(clink->cl_chan,
						       struct m0_conf_obj,
						       co_ha_chan);
	uint64_t event = proc_conf->co_ha_state;
	/*
	 * Assumption: the queue never gets full.
	 * XXX: We could panic in this case. Right now, the HA FOM should get
	 *      stuck in the tick. Prorably, panic is a better solution
	 *      here.
	 */
	M0_BE_OP_SYNC(op, M0_BE_QUEUE_PUT(&rt->rt_heq, op, &event));
	return false;
}

static void recovery_task_init(struct recovery_task    *rt,
			       struct m0_dtm0_recovery *recovery,
			       struct m0_conf_process  *proc_conf,
			       const struct m0_fid     *target)
{
	rt->rt_target   = target;
	rt->rt_recovery = recovery;

	rtask_tlink_init(&rt->rt_linkage);

	m0_sm_group_init(&rt->rt_sm_group);
	m0_sm_init(rt->rt_sm_group, recovery_sm_conf, &rt->sm_group);

	m0_clink_init(rt->rt_ha_clink, recovery_task_clink_cb);
	m0_clink_add_lock(&proc_conf->pc_obj.co_ha_chan, &rf->rf_ha_clink);

	m0_fom_init(&rt->rt_fom, &recovery_fom_type,
		    &recovery_fom_ops, NULL, NULL,
		    m0_dtm0_recovery_reqh(recovery));

	m0_fom_queue(&rt->rt_fom);
}

static int recovery_task_add(struct m0_dtm0_recovery *recovery,
			    struct m0_conf_process  *proc_conf,
			    const struct m0_fid     *target)
{
	struct recovery_task *rt;

	if (M0_ALLOC_PTR(rt) != NULL) {
		recovery_task_init(rt);
		rtask_tlist_add_tail(&recovery->re_tasks);
		return 0;
	} else
		return M0_ERR(-ENOMEM);
}

static void unpopulate(struct m0_dtm0_recovery *recovery)
{
	(void) recovery;
	M0_ASSERT_INFO(false, "TODO");
}

static int populate(struct m0_dtm0_recovery *recovery)
{
	struct m0_confc        *confc = m0_reqh2confc(service->rs_reqh);
	struct m0_conf_root    *root;
	struct m0_conf_diter    it;
	struct m0_conf_process *proc_conf;
	struct m0_conf_service *svc_conf;
	struct m0_fid          *svc_fid;
	int                     rc;

	M0_ENTRY("recovery=%p", recovery);

	rc = m0_confc_root_open(confc, &root) ?:
		m0_conf_diter_init(&it, confc,
				   &root->rt_obj,
				   M0_CONF_ROOT_NODES_FID,
				   M0_CONF_NODE_PROCESSES_FID);
	if (rc != 0)
		goto out;

	while ((rc = m0_conf_diter_next_sync(&it, conf_obj_is_process)) > 0) {
		obj = m0_conf_diter_result(&it);
		proc_conf = M0_CONF_CAST(obj, m0_conf_process);
		rc = m0_conf_process2service_get(confc,
						 &proc_conf->pc_obj.co_id,
						 M0_CST_DTM0, &svc_fid);
		if (rc == 0) {
			rc = recovery_task_add(recovery, proc_conf, svc_fid);
			if (rc != 0) {
				unpopulate(recovery);
				break;
			}
		}
	}

	m0_conf_diter_fini(&it);

out:
	if (root != NULL)
		m0_confc_close(&root->rt_obj);
	return M0_RC(0);
}

M0_INTERNAL int m0_dtm0_recovery_init(struct m0_dtm0_recovery *recovery,
				      struct m0_dtm0_service  *dtms)
{
	recovery->re_svc = dtms;
	rmach_tlist_init(&recovery->re_foms);
	return populate(recovery);
}

M0_INTERNAL void m0_dtm0_recovery_fini(struct m0_dtm0_recovery *recovery)
{
	rmach_tlist_fini(&recovery->re_foms);
	recovery->re_svc = NULL;
	unpopulate(recovery);
}

#endif /* end of recovery machine pseudo-code */
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
