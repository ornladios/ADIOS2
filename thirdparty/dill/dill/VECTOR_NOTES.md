# DILL_Q: vector support

Design notes for the NEON/SSE vector extension (branch `neon-vector-ops`).
Motivating client: fused derived-expression loops in ADIOS2 (element-wise
float/double kernels), where dill's scalar output loses to compiler-vectorized
library code.

## Type

One new atomic type, `DILL_Q`: an opaque vector value living in an FP/vector
register. Lane interpretation (float vs double) is carried by the *operation*,
not the type, mirroring how the hardware works. `DILL_Q` is appended to the
type enum after `DILL_EC` and before `DILL_ERR`, which keeps every existing
type index stable.

### Width is a per-target property, not part of the API

`DILL_Q`'s width is **whatever the target's `type_size[DILL_Q]` says** — 16
today on both arm64 and x86_64, but a backend may widen to 32 (AVX) or 64
(AVX-512) without touching anything outside its own file. That works because
every width-sensitive path in dill already routes through that table:

- spill slots come from `dill_local(c, DILL_Q)` -> `type_size[DILL_Q]`
- spill/reload go through `j->storei/loadi(DILL_Q)`, which each backend maps
  to a full-width move
- the 4-bit type field in `iclass_loadstore`'s `insn_code` is width-agnostic
  (`DILL_Q` = 14 fits regardless)

Clients must therefore **never assume a lane count**. Two queries expose the
geometry, both looking through to the emit-time table the way
`dill_has_vector_ops` does:

- `dill_vector_bytes(s)` — bytes per vector, 0 if unsupported
- `dill_vector_lanes(s, DILL_F | DILL_D)` — lanes, i.e. the vector loop stride

`vtests/vec.c` is written entirely against these (buffer sizes, loop strides,
`n & ~(lanes-1)` masks, expected-value loops), so it is the regression test for
the abstraction as much as for the ops. Verified by temporarily setting
x86_64's table entry to 32: the test reported "32 bytes: 8 float lanes, 4
double lanes" with no source change and failed cleanly starting at lane 4 —
i.e. only the emitters were still 128-bit. Nothing else in dill assumes 16.

Widening a backend is thus a self-contained change: emitters + the table
entry, with the tests and clients following automatically.

Constraint checked: `iclass_loadstore` packs the type into 4 bits of
`insn_code` (`& 0xf`, 0x10 = store, 0x20 = bswap). `DILL_Q` = 14 still fits.

## Operations

Data movement rides the existing fixed jump-table entries, keyed by type:

- `dill_ldq/ldqi/stq/stqi` — full-width load/store, lane-agnostic (base.ops
  `&loadstore` gains letter `q`)
- `dill_movq` — full-register move (`&mov` gains `q`)
- no `dill_setq` (no vector immediates; use scalar set + splat), no bswap

New arith families appended at the END of the a3/a2 index spaces (arch .ops
files fill slots by name, so unimplementing arches just leave NULLs):

- a3: `vadd vsub vmul vdiv` x `f d`  (operands all DILL_Q)
- a3: `vfma` x `f d` — ACCUMULATING: `dest += src1*src2` (NEON FMLA, fused
  single-rounding, matches C fma()/fmaf()).  Dill's only read-modify-write
  op: liveness and the optimizers treat dest as also-used (see
  `is_accum_insn` in virtual.c — insn_uses/build_bb_body add the dest use;
  const-prop, mov back-prop, and same-except-dest elimination are barred).
- a2: `vneg vsqrt` x `f d`           (operands all DILL_Q)
- a2: `vsplat` x `f d`               (dest DILL_Q, src scalar DILL_F/D; DUP)
- a2: `sqrt` x `f d`                 (SCALAR sqrt — hardware has it, spares
                                      the libm call TryFuse pays today)

Capability query: `dill_has_vector_ops(dill_stream s)` — checks the *native*
target's jump table (in virtual mode `s->j` is the always-filled virtual
table, so the query must look through to the emit-time table). Clients fall
back to scalar loops when false.

## Virtual mode

- virtual.ops: record-only entries for the new families (reuse
  dill_varith3/varith2 recorders); NO vm emulation cases initially — the
  libffi/vm path reports the ops unsupported.
- Register allocation: `DILL_Q` joins `DILL_F/DILL_D` in the fpregs class
  (all `case DILL_F:` switches in virtual.c). Spill slots come from
  `dill_local(c, DILL_Q)`, sized from the type tables; spill/reload go
  through `j->storei/loadi(DILL_Q)` = full-width q/xmm moves.
- `do_global_assign` gives `DILL_Q` a cross-block register only when the
  backend sets `jmp_table.vector_global_regs` (x86_64 does; arm64 does not yet
  — see the hazard section). With it off, a loop-carried vector accumulator
  spills and reloads **every iteration** even with registers idle.
- `do_global_assign` used to `break` out of its whole candidate loop when one
  candidate could not get a register. Register exhaustion is per *class*, so a
  starved integer pool (four pointer parameters is enough) silently denied
  every float/vector candidate a register too. Now `continue`s. This is what
  actually unblocked the accumulator; the DILL_Q eligibility change alone did
  nothing.

### Narrow-save hazards (the subtle rule, and it bites on both arches)

The recurring failure mode is a `DILL_Q` value crossing a call through a
64-bit-wide save. It shows up twice, for different reasons:

**arm64, from the ABI.** AAPCS64 preserves only the LOW 64 bits of v8-v15.
arm64's `var_f` pool is exactly v8-v15, so a `DILL_Q` assigned there would
silently lose its top lanes at any `dill_scall`. Rule: `DILL_Q` vregs allocate
from the tmp (caller-saved) FP pool only. arm64 therefore leaves
`vector_global_regs` at 0 — giving it cross-block vector registers needs this
worked through first.

**x86_64, from dill's own code — FIXED.** The original note claimed "x86_64
SysV: all xmm caller-saved, no issue", which is true of the ABI but missed that
`x86_64_calli` saves live xmm registers *itself* via
`x86_64_save_restore_op(..., DILL_D, i)` — an 8-byte `movsd`. Any `DILL_Q`
live in a register across a call lost everything above 64 bits.

`x86_64_calli` now saves/restores the **full 128 bits** (`MOVUPD`) of every
mustsave xmm, into `smi->vec_save_base`, a dedicated 16-byte-spaced area. It
deliberately does not reuse `save_base`'s float region: that region doubles as
the incoming-parameter/vararg save area, whose 8-byte spacing is pinned by
`args[i].offset` and the SysV vararg ABI. Saving a scalar double at full width
is harmless, and the call path cannot know which xmm holds which.

This is demonstrably load-bearing, not theoretical. With `vector_global_regs`
on but the 8-byte save restored, `vtests/vec.c`'s `spill_and_call` fails
immediately ("elem 2 got 191.25, expected 205") and a two-live-vector probe
loses lanes 2 and 3. The two changes are genuinely one change.

The Windows path was already correct — `save_restore_op` uses `MOVUPS` at
16-byte spacing there — so it is left alone.

Still outstanding: `x86_64_callr` (a direct `dill_callr` through a register,
as opposed to the `calli` path `dill_scall` uses) saves no xmm registers at
all. Pre-existing, and not reached by the vector tests.

## arm64 encoders (first target)

All fixed 32-bit words, fields OR'd in, same style as existing emitters:

- `FADD/FSUB/FMUL/FDIV Vd.<T>, Vn.<T>, Vm.<T>` (T = 4S/2D)
- `FMLA Vd.<T>, Vn.<T>, Vm.<T>` (accumulating)
- `FNEG/FSQRT Vd.<T>, Vn.<T>`
- `DUP Vd.4S, Vn.S[0]` / `DUP Vd.2D, Vn.D[0]` (splat)
- `FSQRT Sd,Sn / Dd,Dn` (scalar)
- `LDR/STR Qt, [Xn, #imm]` (+ unscaled/reg-offset fallbacks, unaligned OK)
- mov: `ORR Vd.16B, Vn.16B, Vn.16B`

Disassembly verification comes free via the binutils disassembler hookup.

## x86_64 encoders (second target)

All the DILL_Q arithmetic is **VEX.128** encoded (three-byte C4 form
unconditionally; the two-byte C5 form would only ever apply if every register
were XMM0-XMM7, and the allocator hands out XMM8-XMM15). Loads, stores and
`movq` stay legacy SSE — they are already non-destructive, so VEX would buy
nothing there and would mean restructuring the shared `ploadi/pload/pstorei/
pstore` prefix path that all types go through.

- `VADDP{S,D}/VSUBP{S,D}/VMULP{S,D}/VDIVP{S,D}` (0F 58/5c/59/5e), `data2`
  selecting pp = none (single) or 0x66 (double)
- `VSQRTP{S,D}` (0F 51), two-operand, so VEX.vvvv = 1111
- `VFMADD231P{S,D}` (0F38 B8). Note this group alone takes its lane type from
  **VEX.W**, not pp — it is 0F38/66 for both.
- `vneg`: `VPCMPEQD` / `VPSLLD 31` (`VPSLLQ 63`) / `VXORP{S,D}` — a true
  sign-bit flip, not `0 - x`, so signed zeroes match NEON's FNEG. `VPSLLD`
  with an immediate has an odd layout worth knowing: the DESTINATION goes in
  VEX.vvvv, the source in r/m, and the ModRM reg field is a fixed /6.
- `vsplat`: `VSHUFPS dest,src,src,0` / `VUNPCKLPD dest,src,src` — one
  instruction each, where the legacy encoding needed a MOVAPD first
- scalar `sqrt`: `SQRTSS/SQRTSD` (`f3/f2 0f 51`), legacy SSE, needs no AVX
- load/store: `MOVUPD` (`66 0f 10` / `66 0f 11`), reusing the existing
  `float_op` prefix path verbatim since 0x66 is just another prefix byte
  there. Unaligned, so no 16-byte frame alignment requirement.
- `movq`: `MOVAPD` reg-reg (`66 0f 28`), sharing the DILL_F/DILL_D case

Mixing VEX.128 with the surrounding legacy-SSE scalar code is safe and needs
no `vzeroupper`: VEX.128 zeroes bits 255:128, so it never leaves the dirty
upper YMM state that causes AVX-SSE transition penalties. **This is the
property that would be lost at VEX.256.**

### What the VEX conversion actually bought (measured)

Honest accounting, because the motivation was partly wrong:

- **Throughput: no measurable change.** Magnitude kernel ~50 -> ~48 GB/s
  (within run-to-run spread), dot ~16.7 -> ~16.8. The instructions it removes
  are register-register `MOVAPD`s, which modern x86 handles in the renamer at
  zero execution cost, and these kernels are memory/sqrt bound rather than
  front-end bound.
- **Code size: marginal.** Dot loop 204 -> 200 bytes, mixed-op sequence
  144 -> 138. Each VEX instruction is a byte *longer* than the legacy form it
  replaces; the win comes only from deleting whole instructions.
- **Emitter simplicity: this is the real gain.** Three-operand forms mean the
  arithmetic ops have no operand-aliasing analysis, no src1 copy and no
  scratch register at all. `x86_64_vfarith` went from "stash src2 in XMM0 if
  it aliases dest, copy src1 into dest, then operate" to a single emit call.
- **One latent fragility removed.** The legacy path depended on XMM0 never
  being handed out by the register allocator — true (verified through
  `get_tentative_assign` -> `dill_raw_getreg` -> `reg_alloc`, which only draws
  from `avail`, and XMM0-7 are in `tmp_f.members` but never in `init_avail`)
  but not enforced anywhere. Only `vneg`'s dest == src case still relies on it.

### Capability gating

`x86_64_vector_init` retracts jump-table entries after a CPUID probe, in two
independent steps, because there are two independent requirements:

- **no AVX** -> every DILL_Q entry is NULLed. This is not optional: the
  emitters are VEX-encoded, so without AVX they would produce #UD. (Before the
  VEX conversion the SSE2 ops would have run fine on such a CPU, so this
  gating had to be added along with it.)
- **no FMA3** -> only `vfma` is NULLed. We drop it rather than emit an unfused
  mul+add, which would disagree numerically with arm64's FMLA.

Both bits also require the OS to have enabled XMM+YMM state (OSXSAVE plus
XCR0[2:1] == 11b), since both are VEX-encoded. Either retraction makes
`dill_has_vector_ops()` false — it checks vaddf/vaddd/vfmaf/vsplatf — so
clients fall back to scalar. Leaving an entry at 0 is the same "this arch does
not implement it" convention the .ops files already use. The SCALAR sqrt is
legacy-encoded and deliberately survives both retractions. Cross-generating
x86_64 code from a non-x86_64 host reports no features, since the CPU that
will run the code cannot be probed.

## Status (2026-08-20)

**arm64/macOS**: base.ops/virtual.ops plumbing, RA integration, NEON encoders.
19/19 ctest, ASAN-clean, 120x soak clean. pregen-source regenerated (and now
includes dill_arm64.c, previously missing).

**x86_64/Linux**: VEX.128 encoders for all DILL_Q ops, AVX+FMA3 capability
gating, `dill_vector_bytes`/`dill_vector_lanes` width queries. 19/19 ctest.
Every emitted encoding was checked byte-for-byte against `as`/`objdump`
output, including the aliasing cases (`dest == src1`, `dest == src2`, fully
aliased) and both `vneg` mask paths.

`vtests/vec.c` covers binops, unops, splat, scalar sqrt, a fused-magnitude
loop with scalar remainder, vfma straight-line plus a loop-carried
dot-product accumulator, and a 20-vector spill across a call — all written
against the lane-count queries, none of it assuming a width.

Two test weaknesses found and fixed while porting: the vfma check used
operands where `a*b` happened to be exact (cross term `(i+1)(i+2)` is always
even), so **0 of 8 lanes** actually distinguished fused from unfused and a
mul+add would have passed; and there was no signed-zero check pinning `vneg`
to true sign-flip semantics. Both now hold on every lane.


**Register allocation (2026-08-20)**: `DILL_Q` can now hold a cross-block
register on backends that opt in via `jmp_table.vector_global_regs`, and
`do_global_assign` no longer aborts on per-class register exhaustion. Measured
on the dot/FMA kernel: **16.8 -> 34.7 GB/s, 2.07x**, taking dill from 43% to
~90% of a clean 128-bit intrinsics loop. The magnitude kernel is unchanged
(49.8 GB/s) because it has no loop-carried vector — its remaining gap is
integer-side, item 1 below. 25 consecutive clean ctest runs.

CAVEAT: the `do_global_assign` break->continue fix is architecture-neutral and
changes integer/float allocation on **every** backend, not just x86_64. It was
only exercised on x86_64 here. It should be safe on arm64 (F/D values are
<= 64 bits, so AAPCS64's low-64 preservation of v8-v15 is sufficient), but that
is reasoning, not measurement.

NOT built or run on Windows/MSVC. The `_MSC_VER` arms of the CPUID probe are
untested.

Gotcha worth remembering: `dill_scall*`'s arg string drives its OWN vararg
push (register numbers passed as varargs after the string). Manually calling
dill_push_init/push_arg* before dill_scall* and omitting the varargs is UB
that corrupts the stream (cost a day of heisenbug chasing in the test).
ADIOS's TryFuse WIP makes exactly this mistake with its libm wrapper calls;
fix it when porting.

## Side-by-side against gcc (matched 128-bit width, 2026-08-21)

Throughput comparisons alone hid *what* gcc does differently, so here are the
instruction streams. gcc built `-O3 -mavx2 -mfma -mprefer-vector-width=128` so
width is not a confound. (At -O2 gcc's cheap cost model declines to vectorize
these at all and emits scalar code.)

**magnitude** — gcc 11 instructions/iteration, dill 16:

    gcc                                  dill
    vmovups (%rsi,%rax,1),%xmm2          mov    r15,[rbp-0x310]   <- counter reload
    vmovups (%rdi,%rax,1),%xmm3          mov    r11,r15           <- recompute
    vmovups (%r9,%rax,1),%xmm4           shl    r11,0x2              address
    vmulps  %xmm2,%xmm2,%xmm0            movupd xmm8,[r11+rbx*1]
    vfmadd231ps %xmm3,%xmm3,%xmm0        movupd xmm9,[r11+r12*1]
    vfmadd231ps %xmm4,%xmm4,%xmm0        movupd xmm10,[r11+r13*1]
    vsqrtps %xmm0,%xmm0                  vmulps xmm8,xmm8,xmm8
    vmovups %xmm0,(%rdx,%rax,1)          vmulps xmm9,xmm9,xmm9
    add     $0x10,%rax                   vmulps xmm10,xmm10,xmm10
    cmp     %rcx,%rax                    vaddps xmm8,xmm8,xmm9
    jne                                  vaddps xmm8,xmm8,xmm10
                                         vsqrtps xmm8,xmm8
                                         movupd [r11+r14*1],xmm8
                                         add    r15,0x4
                                         mov    [rbp-0x310],r15   <- counter spill
                                         jmp

**dot** — gcc 6 instructions/iteration, dill 9. Note gcc folds one load into
the FMA as a memory operand, which dill has no op form for:

    vmovups (%rsi),%xmm1                 mov    r15,[rbp-0x308]
    add     $0x10,%rsi                   mov    r11,r15
    vfmadd231ps (%rdi),%xmm1,%xmm0       shl    r11,0x2
    add     $0x10,%rdi                   movupd xmm9,[r11+rbx*1]
    cmp     %rax,%rsi                    movupd xmm10,[r11+r12*1]
    jne                                  vfmadd231ps xmm8,xmm9,xmm10
                                         add    r15,0x4
                                         mov    [rbp-0x308],r15
                                         jmp

Three structural gaps: (a) the spilled loop counter, (b) no induction-variable
strength reduction — dill recomputes `i << 2` every iteration where gcc walks a
byte offset, (c) no memory-operand forms, so every load is its own instruction.
No unrolling either.

The two `vmulps`+`vaddps` pairs in dill's magnitude are NOT a missing-FMA gap —
`dill_vfmaf` exists and works. What dill lacks is gcc's automatic
*contraction* of `a*a + b*b` into an FMA, which C enables by default via
`-ffp-contract=fast`. That is deliberate here: contraction changes the numerics
(one rounding instead of two), so dill emits exactly the ops the client asked
for and leaves fusion to an explicit `dill_vfmaf` call. Reproducibility across
backends is worth more to this client than two instructions.

### Which of those actually costs anything

Only (a). Two experiments:

- Rewriting the magnitude kernel in gcc's contracted shape (`dill_vmulf` then
  two `dill_vfmaf`, 2 fewer instructions per iteration) changed **nothing** —
  46-49 GB/s against a 43-53 baseline. The loop is not instruction-throughput
  bound, so the contraction question is moot for performance anyway.
- Temporarily dropping `MIN_ONDEMAND_RESERVE` to 1 so the counter gets a
  register (vreg 101 -> R15) moved magnitude **49.8 -> 64.2 GB/s, +29%**, and
  dot 34.7 -> 36.7, +6%.

That asymmetry explains why the spill fix doubled dot but did nothing for
magnitude: dot is bound by the ~4-cycle FMA accumulator dependency chain, which
*hides* the counter's store-to-load forward. Magnitude has no such chain, so
the recurrence is exposed and sets the loop time.

`MIN_ONDEMAND_RESERVE = 3` was NOT lowered — three is principled (an `arith3`
can need three distinct registers at once) and 19 passing tests do not prove
the on-demand allocator won't thrash. The proper fix is item 1 below.

## Appendix: dill's x86_64 integer register model

Not really a vector topic, but the vector work ran into it and the analysis is
worth keeping. It is why the magnitude kernel stalls at ~50% of a clean
128-bit loop while the dot kernel reaches ~90%.

### Where all 16 GPRs go

| register | role | allocatable |
|----------|------|-------------|
| RSP | stack pointer | no, architectural |
| RBP | frame pointer (`_frame_reg`, `dill_local_pointer`) | no, architectural |
| RAX | `_temp_reg`, `machine_strr_tmp_reg`, return value, SysV vararg SSE-count (`setl(s, EAX, float_arg_count)` in every `callr`) | no; in `tmp_i.members`, never `init_avail` |
| RDI, RSI, R8, R9 | SysV outgoing argument registers | no |
| RDX | argument register **and** RDX:RAX for MUL/DIV | no |
| RCX | argument register **and** CL for variable shifts | no |
| R10 | **nothing at all** — its only appearance in the backend is a save-area offset | no, for no reason |
| RBX, R12-R15 | callee-saved | yes: `var_i`, 5 |
| R11 | caller-saved temp, **and** the call-target register in `calli` | yes: `tmp_i`, 1 |

Six allocatable. For contrast arm64 has seventeen (10 `var_i` + 7 `tmp_i`), so
everything below is an x86_64-specific problem: with four pointer parameters
arm64 still has six callee-saved registers spare.

### The binding constraint is the on-demand reserve, not the register count

`do_global_assign` will only hand a vreg a cross-block register if it can also
reserve `MIN_ONDEMAND_RESERVE` (3) more of the same class. That number is not a
fudge factor. `select_reg` obtains pregs through `get_tentative_assign` ->
`dill_raw_getreg(DILL_VAR)`, which returns only registers still in `avail`;
globally-assigned pregs were taken and never returned, so `select_reg`
structurally **cannot** evict them. The reserve therefore has to cover the
worst-case simultaneous on-demand demand, which for an `arith3` with three
distinct spilled operands is exactly three. It cannot be lowered.

So with four pointer parameters consuming four of five `var_i` registers, one
is free, the probe needs three more, and **no integer vreg gets a register at
all** — including the loop counter, which then spills. Measured cost: dropping
the reserve to 1 (unsafe, do not ship) moved magnitude 49.8 -> 64.2 GB/s, +29%.

### Why the obvious fixes don't work

**"The argument registers are dead between calls, use them as temporaries."**
They are not dead. `internal_push` writes `arg_regs[]` directly
(`x86_64_pmov(..., arg.out_reg, ...)`), and the pushes sit INSIDE the block
that ends just before the call (`iclass_call` -> `end_bb`, `bb->end = i - 1`).
`spill_current_pregs` runs only at `j == bb->end`, i.e. after the pushes. A
live vreg in RDI is clobbered by an earlier push before anything is spilled.

**"So spill at the first push instead of at the call."** This is the right
mechanism, and it would work — but not the way it first appears. The argument
registers would not end up holding the loop counter; `dill_raw_getreg` drains
`var_i` first, so the counter still lands in R15. What they would do is make
the reserve *satisfiable*, which is what blocks global assignment today.

Its cost is real, though: moving the boundary makes the push source operands
`live_at_end` of the earlier block, so they are spilled and immediately
reloaded — a memory round-trip per call argument, where today sources that die
at the push are never spilled at all. Call-heavy code (much of FFS/ADIOS) could
easily lose more than the loops gain. It also needs the argument registers
marked unavailable for the duration of the push block, with reloads staged
through R10/R11, or a reload lands in RSI and push #2 clobbers it. And it is
generic RA work requiring per-backend argument-register knowledge, paid for by
every backend including arm64, which has no problem to fix.

**"Add R10."** Free and safe (see below), but it only reaches three free
registers where four are needed. Necessary, not sufficient.

**"Free RBP too."** A sixth callee-saved register would satisfy the reserve
with no generic RA change: `var_i` 2 free + `tmp_i` 2 = 4. But omitting the
frame pointer means RSP-relative local addressing, and dill moves RSP mid-block
for stack arguments (`dill_subli(s, ESP, ESP, call_stack_space)`), so every
local offset would need tracking through the call sequence. That is exactly the
case frame pointers exist for.

### A latent hazard to know about before touching any of this

`spill_current_pregs` deliberately **skips** globally-assigned vregs ("value
stays in register across blocks"). That is safe only if the register survives
calls. For FP it does: `x86_64_calli` saves mustsave `tmp_f`, at full vector
width since 2026-08-20. But **`calli` saves no integer temporaries at all** —
the loops are XMM-only. A globally-assigned integer vreg in a caller-saved
register would be silently destroyed by any call.

It is unreachable today only by arithmetic accident: `dill_raw_getreg(DILL_VAR)`
drains `var_i` before `tmp_i`, and the reserve needs four free of the class,
but `tmp_i` has exactly one member. Adding argument registers to `tmp_i`, or
lowering the reserve, breaks that. The reserve=1 experiment passed all 19 tests
only because it happened to land on R15.

**Prerequisite for any work here: give `calli` an integer save loop first, in
its own commit, with its own tests.**

### The one cheap, safe improvement

R10 is caller-saved, is not an argument register, and is untouched by every
emitter. Adding it to `tmp_i.init_avail` takes the pool from 6 to 7 and, more
usefully, takes `tmp_i` from one register to two — today the sole integer temp
is R11, which `calli` also clobbers as the call target, so any block needing two
simultaneous integer temps must spill. It stays clear of the hazard above:
with `tmp_i` at two members the reserve still cannot be met from it, so R10/R11
remain ineligible for global assignment. `save_restore_op` already has a slot
for R10 if `calli` ever grows an integer save loop.

Not done here — it is unrelated to the vector work and belongs in its own
commit.

## Known-suboptimal, in rough priority order

1. **Integer register pressure (x86_64 only)** — now the binding constraint
   for kernels without a loop-carried vector. Four pointer parameters consume
   four of five `var_i` registers, and `MIN_ONDEMAND_RESERVE = 3` then blocks
   all integer global assignment, so the loop counter spills and puts a
   store-to-load forward in the loop-carried dependency. Worth ~29% on the
   magnitude kernel, which is otherwise optimal. This is a dill register-model
   problem, not a vector one, and none of the obvious fixes are safe — see the
   appendix below before touching it. arm64 does not have this problem.
2. DONE (2026-08-24): integer multiply by a power-of-two immediate is now
   strength-reduced to a shift (x1 to a mov) at emit time in
   `new_emit_insns`, so `dill_mululi(off, i, 4)` costs one `lsl`/`shl` on
   every backend instead of x86_64's 8-instruction `MUL` sequence.  Guarded
   by operand-type width; non-powers and the `DILL_OLD_REGS` legacy path
   keep the real multiply.  Clients need no change.
3. arm64 has not been given `vector_global_regs`, so it still spills
   loop-carried vectors every iteration. Needs the AAPCS64 v8-v15 low-64-bits
   problem worked through first (allocate DILL_Q from the tmp pool only, and
   widen arm64's call save the way x86_64's now is).
4. Width still 16 on both arches — deliberate for now, see below.

DONE: the redundant `movapd` on aliased operands, the commutative-swap gap,
and the XMM0 scratch for arithmetic all disappeared with the VEX.128
conversion. The loop-carried vector spill and the truncating call save are
fixed on x86_64 (worth 2.07x on the FMA kernel).

## Width: what 128 vs 256 is actually worth (measured, 2026-08-20)

Measured with intrinsics, identical loop structure, only the width differing,
on an i5-13600K (Raptor Lake, no AVX-512), P-core pinned. This isolates width
properly; an earlier comparison against gcc's 256-bit output did NOT, because
it conflated width with gcc's much better loop code.

| kernel | level | 128-bit | 256-bit | width gain |
|--------|-------|---------|---------|------------|
| magnitude (3 loads, store, sqrt) | L1 | 98.8 | 98.9 | 1.00x |
| | L2 | 99.0 | 91.7 | 0.93x |
| | DRAM | 21.1 | 20.3 | 0.96x |
| dot (fma accumulate) | L1 | 38.8 | 94.3 | **2.43x** |
| | L2 | 38.8 | 76.5 | **1.97x** |
| | DRAM | 28.1 | 18.4 | 0.66x (noisy, 40 reps) |

So width is entirely kernel-shaped:

- **sqrt/divide-heavy or memory-bound: nothing.** `VSQRTPS ymm` costs about
  double `VSQRTPS xmm` on the divider, so doubling lanes doubles the cost.
- **FMA/mul/add-dense and cache-resident: ~2x.** Real and large.

`vzeroupper` is NOT the obstacle it was made out to be earlier in these notes:
the 256-bit kernels above include it and still win 2.4x. It remains a genuine
requirement (VEX.256 dirties the upper YMM state, unlike VEX.128), just not a
dealbreaker. The AVX2 requirement for register-source `VBROADCASTSS` and for
256-bit `VPCMPEQD`/`VPSLLD` in the vneg mask does still stand.

### Why 128 is nevertheless the right thing to fix LAST

Against a clean *128-bit* intrinsics loop — same width, so this isolates loop
quality rather than width:

| kernel | dill vector | clean 128-bit | dill is at |
|--------|-------------|---------------|------------|
| magnitude (L2) | 50.5 | 99.0 | 51% |
| dot (L2) | 16.8 | 38.8 | 43% |

dill leaves ~2x on the table **at its current width**, from the loop-carried
accumulator spilling every iteration (item 1 below). Widening before fixing
that would capture little of the 2x, because the loop would still be waiting
on a reload. Fix the spill first: it is ~2x, it helps both kernel shapes, and
it is a precondition for width paying off at all. Then widen, for FMA-dense
clients specifically.

Because the width mechanism is already parameterized (see Type, above), that
second step stays cheap: emitters plus one table entry.

## Explicitly out of scope (for now)

- horizontal reductions (sum/min/max/dot). The dot-product test leaves N
  partial sums for the caller to fold; any real reduction kernel wants
  `FADDV`/`ADDV` on arm64 and `haddps`/shuffle-add on x86. Obvious next op.
- gathers, masked ops, integer lanes, vector byteswap, vector calls (SLEEF),
  vm-mode emulation
