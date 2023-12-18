/* Common BPF helpers to be used by all BPF programs loaded by Android */

#include <linux/bpf.h>
#include <stdbool.h>
#include <stdint.h>

#include "bpf_map_def.h"

/******************************************************************************
 * WARNING: CHANGES TO THIS FILE OUTSIDE OF AOSP/MASTER ARE LIKELY TO BREAK   *
 * DEVICE COMPATIBILITY WITH MAINLINE MODULES SHIPPING EBPF CODE.             *
 *                                                                            *
 * THIS WILL LIKELY RESULT IN BRICKED DEVICES AT SOME ARBITRARY FUTURE TIME   *
 *                                                                            *
 * THAT GOES ESPECIALLY FOR THE 'SEC' 'LICENSE' AND 'CRITICAL' MACRO DEFINES  *
 *                                                                            *
 * We strongly suggest that if you need changes to bpfloader functionality    *
 * you get your changes reviewed and accepted into aosp/master.               *
 *                                                                            *
 ******************************************************************************/

/* place things in different elf sections */
#define SEC(NAME) __attribute__((section(NAME), used))

/* Must be present in every program, example usage:
 *   LICENSE("GPL"); or LICENSE("Apache 2.0");
 *
 * We also take this opportunity to embed a bunch of other useful values in
 * the resulting .o (This is to enable some limited forward compatibility
 * with mainline module shipped ebpf programs)
 *
 * The bpfloader_{min/max}_ver defines the [min, max) range of bpfloader
 * versions that should load this .o file (bpfloaders outside of this range
 * will simply ignore/skip this *entire* .o)
 * The [inclusive,exclusive) matches what we do for kernel ver dependencies.
 *
 * The size_of_bpf_{map,prog}_def allow the bpfloader to load programs where
 * these structures have been extended with additional fields (they will of
 * course simply be ignored then).
 *
 * If missing, bpfloader_{min/max}_ver default to 0/0x10000 ie. [v0.0, v1.0),
 * while size_of_bpf_{map/prog}_def default to 32/20 which are the v0.0 sizes.
 */
#define LICENSE(NAME)                                                                       \
    unsigned int _bpfloader_min_ver SEC("bpfloader_min_ver") = DEFAULT_BPFLOADER_MIN_VER;   \
    unsigned int _bpfloader_max_ver SEC("bpfloader_max_ver") = DEFAULT_BPFLOADER_MAX_VER;   \
    size_t _size_of_bpf_map_def SEC("size_of_bpf_map_def") = sizeof(struct bpf_map_def);    \
    size_t _size_of_bpf_prog_def SEC("size_of_bpf_prog_def") = sizeof(struct bpf_prog_def); \
    char _license[] SEC("license") = (NAME)

/* flag the resulting bpf .o file as critical to system functionality,
 * loading all kernel version appropriate programs in it must succeed
 * for bpfloader success
 */
#define CRITICAL(REASON) char _critical[] SEC("critical") = (REASON)

/*
 * Helper functions called from eBPF programs written in C. These are
 * implemented in the kernel sources.
 */

#define KVER_NONE 0
#define KVER(a, b, c) (((a) << 24) + ((b) << 16) + (c))
#define KVER_INF 0xFFFFFFFFu

/* generic functions */

/*
 * Type-unsafe bpf map functions - avoid if possible.
 *
 * Using these it is possible to pass in keys/values of the wrong type/size,
 * or, for 'bpf_map_lookup_elem_unsafe' receive into a pointer to the wrong type.
 * You will not get a compile time failure, and for certain types of errors you
 * might not even get a failure from the kernel's ebpf verifier during program load,
 * instead stuff might just not work right at runtime.
 *
 * Instead please use:
 *   DEFINE_BPF_MAP(foo_map, TYPE, KeyType, ValueType, num_entries)
 * where TYPE can be something like HASH or ARRAY, and num_entries is an integer.
 *
 * This defines the map (hence this should not be used in a header file included
 * from multiple locations) and provides type safe accessors:
 *   ValueType * bpf_foo_map_lookup_elem(const KeyType *)
 *   int bpf_foo_map_update_elem(const KeyType *, const ValueType *, flags)
 *   int bpf_foo_map_delete_elem(const KeyType *)
 *
 * This will make sure that if you change the type of a map you'll get compile
 * errors at any spots you forget to update with the new type.
 *
 * Note: these all take pointers to const map because from the C/eBPF point of view
 * the map struct is really just a readonly map definition of the in kernel object.
 * Runtime modification of the map defining struct is meaningless, since
 * the contents is only ever used during bpf program loading & map creation
 * by the bpf loader, and not by the eBPF program itself.
 */
static void* (*bpf_map_lookup_elem_unsafe)(const struct bpf_map_def* map,
                                           const void* key) = (void*)BPF_FUNC_map_lookup_elem;
static int (*bpf_map_update_elem_unsafe)(const struct bpf_map_def* map, const void* key,
                                         const void* value, unsigned long long flags) = (void*)
        BPF_FUNC_map_update_elem;
static int (*bpf_map_delete_elem_unsafe)(const struct bpf_map_def* map,
                                         const void* key) = (void*)BPF_FUNC_map_delete_elem;

/* type safe macro to declare a map and related accessor functions */
#define DEFINE_BPF_MAP_UGM(the_map, TYPE, TypeOfKey, TypeOfValue, num_entries, usr, grp, md)     \
    const struct bpf_map_def SEC("maps") the_map = {                                             \
            .type = BPF_MAP_TYPE_##TYPE,                                                         \
            .key_size = sizeof(TypeOfKey),                                                       \
            .value_size = sizeof(TypeOfValue),                                                   \
            .max_entries = (num_entries),                                                        \
            .map_flags = 0,                                                                      \
            .uid = (usr),                                                                        \
            .gid = (grp),                                                                        \
            .mode = (md),                                                                        \
            .bpfloader_min_ver = DEFAULT_BPFLOADER_MIN_VER,                                      \
            .bpfloader_max_ver = DEFAULT_BPFLOADER_MAX_VER,                                      \
            .min_kver = KVER_NONE,                                                               \
            .max_kver = KVER_INF,                                                                \
    };                                                                                           \
                                                                                                 \
    static inline __always_inline __unused TypeOfValue* bpf_##the_map##_lookup_elem(             \
            const TypeOfKey* k) {                                                                \
        return bpf_map_lookup_elem_unsafe(&the_map, k);                                          \
    };                                                                                           \
                                                                                                 \
    static inline __always_inline __unused int bpf_##the_map##_update_elem(                      \
            const TypeOfKey* k, const TypeOfValue* v, unsigned long long flags) {                \
        return bpf_map_update_elem_unsafe(&the_map, k, v, flags);                                \
    };                                                                                           \
                                                                                                 \
    static inline __always_inline __unused int bpf_##the_map##_delete_elem(const TypeOfKey* k) { \
        return bpf_map_delete_elem_unsafe(&the_map, k);                                          \
    };

#define DEFINE_BPF_MAP(the_map, TYPE, TypeOfKey, TypeOfValue, num_entries) \
    DEFINE_BPF_MAP_UGM(the_map, TYPE, TypeOfKey, TypeOfValue, num_entries, AID_ROOT, AID_ROOT, 0600)

#define DEFINE_BPF_MAP_GWO(the_map, TYPE, TypeOfKey, TypeOfValue, num_entries, gid) \
    DEFINE_BPF_MAP_UGM(the_map, TYPE, TypeOfKey, TypeOfValue, num_entries, AID_ROOT, gid, 0620)

#define DEFINE_BPF_MAP_GRO(the_map, TYPE, TypeOfKey, TypeOfValue, num_entries, gid) \
    DEFINE_BPF_MAP_UGM(the_map, TYPE, TypeOfKey, TypeOfValue, num_entries, AID_ROOT, gid, 0640)

#define DEFINE_BPF_MAP_GRW(the_map, TYPE, TypeOfKey, TypeOfValue, num_entries, gid) \
    DEFINE_BPF_MAP_UGM(the_map, TYPE, TypeOfKey, TypeOfValue, num_entries, AID_ROOT, gid, 0660)

static int (*bpf_probe_read)(void* dst, int size, void* unsafe_ptr) = (void*) BPF_FUNC_probe_read;
static int (*bpf_probe_read_str)(void* dst, int size, void* unsafe_ptr) = (void*) BPF_FUNC_probe_read_str;
static unsigned long long (*bpf_ktime_get_ns)(void) = (void*) BPF_FUNC_ktime_get_ns;
static unsigned long long (*bpf_ktime_get_boot_ns)(void) = (void*)BPF_FUNC_ktime_get_boot_ns;
static int (*bpf_trace_printk)(const char* fmt, int fmt_size, ...) = (void*) BPF_FUNC_trace_printk;
static unsigned long long (*bpf_get_current_pid_tgid)(void) = (void*) BPF_FUNC_get_current_pid_tgid;
static unsigned long long (*bpf_get_current_uid_gid)(void) = (void*) BPF_FUNC_get_current_uid_gid;
static unsigned long long (*bpf_get_smp_processor_id)(void) = (void*) BPF_FUNC_get_smp_processor_id;

#define DEFINE_BPF_PROG_KVER_RANGE_OPT(SECTION_NAME, prog_uid, prog_gid, the_prog, min_kv, max_kv, \
                                       opt)                                                        \
    const struct bpf_prog_def SEC("progs") the_prog##_def = {                                      \
            .uid = (prog_uid),                                                                     \
            .gid = (prog_gid),                                                                     \
            .min_kver = (min_kv),                                                                  \
            .max_kver = (max_kv),                                                                  \
            .optional = (opt),                                                                     \
            .bpfloader_min_ver = DEFAULT_BPFLOADER_MIN_VER,                                        \
            .bpfloader_max_ver = DEFAULT_BPFLOADER_MAX_VER,                                        \
    };                                                                                             \
    SEC(SECTION_NAME)                                                                              \
    int the_prog

// Programs (here used in the sense of functions/sections) marked optional are allowed to fail
// to load (for example due to missing kernel patches).
// The bpfloader will just ignore these failures and continue processing the next section.
//
// A non-optional program (function/section) failing to load causes a failure and aborts
// processing of the entire .o, if the .o is additionally marked critical, this will result
// in the entire bpfloader process terminating with a failure and not setting the bpf.progs_loaded
// system property.  This in turn results in waitForProgsLoaded() never finishing.
//
// ie. a non-optional program in a critical .o is mandatory for kernels matching the min/max kver.

// programs requiring a kernel version >= min_kv && < max_kv
#define DEFINE_BPF_PROG_KVER_RANGE(SECTION_NAME, prog_uid, prog_gid, the_prog, min_kv, max_kv) \
    DEFINE_BPF_PROG_KVER_RANGE_OPT(SECTION_NAME, prog_uid, prog_gid, the_prog, min_kv, max_kv, \
                                   false)
#define DEFINE_OPTIONAL_BPF_PROG_KVER_RANGE(SECTION_NAME, prog_uid, prog_gid, the_prog, min_kv, \
                                            max_kv)                                             \
    DEFINE_BPF_PROG_KVER_RANGE_OPT(SECTION_NAME, prog_uid, prog_gid, the_prog, min_kv, max_kv, true)

// programs requiring a kernel version >= min_kv
#define DEFINE_BPF_PROG_KVER(SECTION_NAME, prog_uid, prog_gid, the_prog, min_kv)                 \
    DEFINE_BPF_PROG_KVER_RANGE_OPT(SECTION_NAME, prog_uid, prog_gid, the_prog, min_kv, KVER_INF, \
                                   false)
#define DEFINE_OPTIONAL_BPF_PROG_KVER(SECTION_NAME, prog_uid, prog_gid, the_prog, min_kv)        \
    DEFINE_BPF_PROG_KVER_RANGE_OPT(SECTION_NAME, prog_uid, prog_gid, the_prog, min_kv, KVER_INF, \
                                   true)

// programs with no kernel version requirements
#define DEFINE_BPF_PROG(SECTION_NAME, prog_uid, prog_gid, the_prog) \
    DEFINE_BPF_PROG_KVER_RANGE_OPT(SECTION_NAME, prog_uid, prog_gid, the_prog, 0, KVER_INF, false)
#define DEFINE_OPTIONAL_BPF_PROG(SECTION_NAME, prog_uid, prog_gid, the_prog) \
    DEFINE_BPF_PROG_KVER_RANGE_OPT(SECTION_NAME, prog_uid, prog_gid, the_prog, 0, KVER_INF, true)
