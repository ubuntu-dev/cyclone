#include <setjmp.h>
/* This is a C header file to be used by the output of the Cyclone to
   C translator.  The corresponding definitions are in file lib/runtime_cyc.c */
#ifndef _CYC_INCLUDE_H_
#define _CYC_INCLUDE_H_

/***********************************************************************/
/* Runtime Stack routines (runtime_stack.c).                           */
/***********************************************************************/

/* Need one of these per thread (we don't have threads)
   The runtime maintains a stack that contains either _handler_cons
   structs or _RegionHandle structs.  The tag is 0 for a handler_cons
   and 1 for a region handle.  */
struct _RuntimeStack {
  int tag; /* 0 for an exception handler, 1 for a region handle */
  struct _RuntimeStack *next;
  void (*cleanup)(struct _RuntimeStack *frame);
};

/***********************************************************************/
/* Low-level representations etc.                                      */
/***********************************************************************/

#ifndef offsetof
/* should be size_t, but int is fine. */
#define offsetof(t,n) ((int)(&(((t *)0)->n)))
#endif

/* Tagged arrays */
struct _dyneither_ptr {
  unsigned char *curr; 
  unsigned char *base; 
  unsigned char *last_plus_one; 
};  

/* Discriminated Unions */
struct _xtunion_struct { char *tag; };

/* Regions */
struct _RegionPage
#ifdef CYC_REGION_PROFILE
{ unsigned total_bytes;
  unsigned free_bytes;
  /* MWH: wish we didn't have to include the stuff below ... */
  struct _RegionPage *next;
  char data[1];
}
#endif
; // abstract -- defined in runtime_memory.c

struct _RegionHandle {
  struct _RuntimeStack s;
  struct _RegionPage *curr;
  char               *offset;
  char               *last_plus_one;
  struct _DynRegionHandle *sub_regions;
#ifdef CYC_REGION_PROFILE
  const char         *name;
#else
  unsigned used_bytes;
  unsigned wasted_bytes;
#endif
};

struct _DynRegionFrame {
  struct _RuntimeStack s;
  struct _DynRegionHandle *x;
};

// A dynamic region is just a region handle.  We have the
// wrapper struct for type abstraction reasons.
struct Cyc_Core_DynamicRegion {
  struct _RegionHandle h;
};

extern struct _RegionHandle _new_region(const char *);
extern void * _region_malloc(struct _RegionHandle *, unsigned);
extern void * _region_calloc(struct _RegionHandle *, unsigned t, unsigned n);
extern void   _free_region(struct _RegionHandle *);
extern struct _RegionHandle *_open_dynregion(struct _DynRegionFrame *f,
                                             struct _DynRegionHandle *h);
extern void   _pop_dynregion();

/* Exceptions */
struct _handler_cons {
  struct _RuntimeStack s;
  jmp_buf handler;
};
extern void _push_handler(struct _handler_cons *);
extern void _push_region(struct _RegionHandle *);
extern void _npop_handler(int);
extern void _pop_handler();
extern void _pop_region();

#ifndef _throw
extern void* _throw_null_fn(const char *filename, unsigned lineno);
extern void* _throw_arraybounds_fn(const char *filename, unsigned lineno);
extern void* _throw_badalloc_fn(const char *filename, unsigned lineno);
extern void* _throw_match_fn(const char *filename, unsigned lineno);
extern void* _throw_fn(void* e, const char *filename, unsigned lineno);
extern void* _rethrow(void* e);
#define _throw_null() (_throw_null_fn(__FILE__,__LINE__))
#define _throw_arraybounds() (_throw_arraybounds_fn(__FILE__,__LINE__))
#define _throw_badalloc() (_throw_badalloc_fn(__FILE__,__LINE__))
#define _throw_match() (_throw_match_fn(__FILE__,__LINE__))
#define _throw(e) (_throw_fn((e),__FILE__,__LINE__))
#endif

extern struct _xtunion_struct *_exn_thrown;

/* Built-in Exceptions */
struct Cyc_Null_Exception_exn_struct { char *tag; };
struct Cyc_Array_bounds_exn_struct { char *tag; };
struct Cyc_Match_Exception_exn_struct { char *tag; };
struct Cyc_Bad_alloc_exn_struct { char *tag; };
extern char Cyc_Null_Exception[];
extern char Cyc_Array_bounds[];
extern char Cyc_Match_Exception[];
extern char Cyc_Bad_alloc[];

/* Built-in Run-time Checks and company */
#ifdef CYC_ANSI_OUTPUT
#define _INLINE  
#else
#define _INLINE inline
#endif

#ifdef NO_CYC_NULL_CHECKS
#define _check_null(ptr) (ptr)
#else
#define _check_null(ptr) \
  ({ void*_cks_null = (void*)(ptr); \
     if (!_cks_null) _throw_null(); \
     _cks_null; })
#endif

#ifdef NO_CYC_BOUNDS_CHECKS
#define _check_known_subscript_null(ptr,bound,elt_sz,index)\
   ((char *)ptr) + (elt_sz)*(index))
#define _check_known_subscript_notnull(bound,index) (index)
#define _check_known_subscript_notnullX(bound,index)\
   ((char *)ptr) + (elt_sz)*(index))

#define _zero_arr_plus_char_fn(orig_x,orig_sz,orig_i,f,l) ((orig_x)+(orig_i))
#define _zero_arr_plus_short_fn(orig_x,orig_sz,orig_i,f,l) ((orig_x)+(orig_i))
#define _zero_arr_plus_int_fn(orig_x,orig_sz,orig_i,f,l) ((orig_x)+(orig_i))
#define _zero_arr_plus_float_fn(orig_x,orig_sz,orig_i,f,l) ((orig_x)+(orig_i))
#define _zero_arr_plus_double_fn(orig_x,orig_sz,orig_i,f,l) ((orig_x)+(orig_i))
#define _zero_arr_plus_longdouble_fn(orig_x,orig_sz,orig_i,f,l) ((orig_x)+(orig_i))
#define _zero_arr_plus_voidstar_fn(orig_x,orig_sz,orig_i,f,l) ((orig_x)+(orig_i))
#else
#define _check_known_subscript_null(ptr,bound,elt_sz,index) ({ \
  char*_cks_ptr = (char*)(ptr); \
  unsigned _cks_bound = (bound); \
  unsigned _cks_elt_sz = (elt_sz); \
  unsigned _cks_index = (index); \
  if (!_cks_ptr) _throw_null(); \
  if (_cks_index >= _cks_bound) _throw_arraybounds(); \
  (_cks_ptr) + _cks_elt_sz*_cks_index; })
#define _check_known_subscript_notnull(ptr,bound,elt_sz,index) ({ \
  char*_cks_ptr = (char*)(ptr); \
  unsigned _cks_bound = (bound); \
  unsigned _cks_elt_sz = (elt_sz); \
  unsigned _cks_index = (index); \
  if (_cks_index >= _cks_bound) _throw_arraybounds(); \
  (_cks_ptr) + _cks_elt_sz*_cks_index; })

/* Add i to zero-terminated pointer x.  Checks for x being null and
   ensures that x[0..i-1] are not 0. */
char * _zero_arr_plus_char_fn(char *orig_x, unsigned int orig_sz, int orig_i,const char *filename, unsigned lineno);
short * _zero_arr_plus_short_fn(short *orig_x, unsigned int orig_sz, int orig_i,const char *filename, unsigned lineno);
int * _zero_arr_plus_int_fn(int *orig_x, unsigned int orig_sz, int orig_i, const char *filename, unsigned lineno);
float * _zero_arr_plus_float_fn(float *orig_x, unsigned int orig_sz, int orig_i,const char *filename, unsigned lineno);
double * _zero_arr_plus_double_fn(double *orig_x, unsigned int orig_sz, int orig_i,const char *filename, unsigned lineno);
long double * _zero_arr_plus_longdouble_fn(long double *orig_x, unsigned int orig_sz, int orig_i, const char *filename, unsigned lineno);
void * _zero_arr_plus_voidstar_fn(void **orig_x, unsigned int orig_sz, int orig_i,const char *filename,unsigned lineno);
#endif

#define _zero_arr_plus_char(x,s,i) \
  (_zero_arr_plus_char_fn(x,s,i,__FILE__,__LINE__))
#define _zero_arr_plus_short(x,s,i) \
  (_zero_arr_plus_short_fn(x,s,i,__FILE__,__LINE__))
#define _zero_arr_plus_int(x,s,i) \
  (_zero_arr_plus_int_fn(x,s,i,__FILE__,__LINE__))
#define _zero_arr_plus_float(x,s,i) \
  (_zero_arr_plus_float_fn(x,s,i,__FILE__,__LINE__))
#define _zero_arr_plus_double(x,s,i) \
  (_zero_arr_plus_double_fn(x,s,i,__FILE__,__LINE__))
#define _zero_arr_plus_longdouble(x,s,i) \
  (_zero_arr_plus_longdouble_fn(x,s,i,__FILE__,__LINE__))
#define _zero_arr_plus_voidstar(x,s,i) \
  (_zero_arr_plus_voidstar_fn(x,s,i,__FILE__,__LINE__))

/* Calculates the number of elements in a zero-terminated, thin array.
   If non-null, the array is guaranteed to have orig_offset elements. */
int _get_zero_arr_size_char(const char *orig_x, unsigned int orig_offset);
int _get_zero_arr_size_short(const short *orig_x, unsigned int orig_offset);
int _get_zero_arr_size_int(const int *orig_x, unsigned int orig_offset);
int _get_zero_arr_size_float(const float *orig_x, unsigned int orig_offset);
int _get_zero_arr_size_double(const double *orig_x, unsigned int orig_offset);
int _get_zero_arr_size_longdouble(const long double *orig_x, unsigned int orig_offset);
int _get_zero_arr_size_voidstar(const void **orig_x, unsigned int orig_offset);

/* Does in-place addition of a zero-terminated pointer (x += e and ++x).  
   Note that this expands to call _zero_arr_plus_<type>_fn. */
char * _zero_arr_inplace_plus_char_fn(char **x, int orig_i,const char *filename,unsigned lineno);
short * _zero_arr_inplace_plus_short_fn(short **x, int orig_i,const char *filename,unsigned lineno);
int * _zero_arr_inplace_plus_int(int **x, int orig_i,const char *filename,unsigned lineno);
float * _zero_arr_inplace_plus_float_fn(float **x, int orig_i,const char *filename,unsigned lineno);
double * _zero_arr_inplace_plus_double_fn(double **x, int orig_i,const char *filename,unsigned lineno);
long double * _zero_arr_inplace_plus_longdouble_fn(long double **x, int orig_i,const char *filename,unsigned lineno);
void * _zero_arr_inplace_plus_voidstar_fn(void ***x, int orig_i,const char *filename,unsigned lineno);
#define _zero_arr_inplace_plus_char(x,i) \
  _zero_arr_inplace_plus_char_fn((char **)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_short(x,i) \
  _zero_arr_inplace_plus_short_fn((short **)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_int(x,i) \
  _zero_arr_inplace_plus_int_fn((int **)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_float(x,i) \
  _zero_arr_inplace_plus_float_fn((float **)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_double(x,i) \
  _zero_arr_inplace_plus_double_fn((double **)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_longdouble(x,i) \
  _zero_arr_inplace_plus_longdouble_fn((long double **)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_voidstar(x,i) \
  _zero_arr_inplace_plus_voidstar_fn((void ***)(x),i,__FILE__,__LINE__)

/* Does in-place increment of a zero-terminated pointer (e.g., x++). */
char * _zero_arr_inplace_plus_post_char_fn(char **x, int orig_i,const char *filename,unsigned lineno);
short * _zero_arr_inplace_plus_post_short_fn(short **x, int orig_i,const char *filename,unsigned lineno);
int * _zero_arr_inplace_plus_post_int_fn(int **x, int orig_i,const char *filename, unsigned lineno);
float * _zero_arr_inplace_plus_post_float_fn(float **x, int orig_i,const char *filename, unsigned lineno);
double * _zero_arr_inplace_plus_post_double_fn(double **x, int orig_i,const char *filename,unsigned lineno);
long double * _zero_arr_inplace_plus_post_longdouble_fn(long double **x, int orig_i,const char *filename,unsigned lineno);
void ** _zero_arr_inplace_plus_post_voidstar_fn(void ***x, int orig_i,const char *filename,unsigned lineno);
#define _zero_arr_inplace_plus_post_char(x,i) \
  _zero_arr_inplace_plus_post_char_fn((char **)(x),(i),__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_short(x,i) \
  _zero_arr_inplace_plus_post_short_fn((short **)(x),(i),__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_int(x,i) \
  _zero_arr_inplace_plus_post_int_fn((int **)(x),(i),__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_float(x,i) \
  _zero_arr_inplace_plus_post_float_fn((float **)(x),(i),__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_double(x,i) \
  _zero_arr_inplace_plus_post_double_fn((double **)(x),(i),__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_longdouble(x,i) \
  _zero_arr_inplace_plus_post_longdouble_fn((long double **)(x),(i),__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_voidstar(x,i) \
  _zero_arr_inplace_plus_post_voidstar_fn((void***)(x),(i),__FILE__,__LINE__)

/* functions for dealing with dynamically sized pointers */
#ifdef NO_CYC_BOUNDS_CHECKS
#define _check_dyneither_subscript(arr,elt_sz,index) ({ \
  struct _dyneither_ptr _cus_arr = (arr); \
  unsigned _cus_elt_sz = (elt_sz); \
  unsigned _cus_index = (index); \
  unsigned char *_cus_ans = _cus_arr.curr + _cus_elt_sz * _cus_index; \
  _cus_ans; })
#else
#define _check_dyneither_subscript(arr,elt_sz,index) ({ \
  struct _dyneither_ptr _cus_arr = (arr); \
  unsigned _cus_elt_sz = (elt_sz); \
  unsigned _cus_index = (index); \
  unsigned char *_cus_ans = _cus_arr.curr + _cus_elt_sz * _cus_index; \
  /* JGM: not needed! if (!_cus_arr.base) _throw_null();*/ \
  if (_cus_ans < _cus_arr.base || _cus_ans >= _cus_arr.last_plus_one) \
    _throw_arraybounds(); \
  _cus_ans; })
#endif

#define _tag_dyneither(tcurr,elt_sz,num_elts) ({ \
  struct _dyneither_ptr _tag_arr_ans; \
  _tag_arr_ans.base = _tag_arr_ans.curr = (void*)(tcurr); \
  /* JGM: if we're tagging NULL, ignore num_elts */ \
  _tag_arr_ans.last_plus_one = _tag_arr_ans.base ? (_tag_arr_ans.base + (elt_sz) * (num_elts)) : 0; \
  _tag_arr_ans; })

#ifdef NO_CYC_BOUNDS_CHECKS
#define _untag_dyneither_ptr(arr,elt_sz,num_elts) ((arr).curr)
#else
#define _untag_dyneither_ptr(arr,elt_sz,num_elts) ({ \
  struct _dyneither_ptr _arr = (arr); \
  unsigned char *_curr = _arr.curr; \
  if ((_curr < _arr.base || _curr + (elt_sz) * (num_elts) > _arr.last_plus_one) &&\
      _curr != (unsigned char *)0) \
    _throw_arraybounds(); \
  _curr; })
#endif

#define _get_dyneither_size(arr,elt_sz) \
  ({struct _dyneither_ptr _get_arr_size_temp = (arr); \
    unsigned char *_get_arr_size_curr=_get_arr_size_temp.curr; \
    unsigned char *_get_arr_size_last=_get_arr_size_temp.last_plus_one; \
    (_get_arr_size_curr < _get_arr_size_temp.base || \
     _get_arr_size_curr >= _get_arr_size_last) ? 0 : \
    ((_get_arr_size_last - _get_arr_size_curr) / (elt_sz));})

#define _dyneither_ptr_plus(arr,elt_sz,change) ({ \
  struct _dyneither_ptr _ans = (arr); \
  _ans.curr += ((int)(elt_sz))*(change); \
  _ans; })

#define _dyneither_ptr_inplace_plus(arr_ptr,elt_sz,change) ({ \
  struct _dyneither_ptr * _arr_ptr = (arr_ptr); \
  _arr_ptr->curr += ((int)(elt_sz))*(change); \
  *_arr_ptr; })

#define _dyneither_ptr_inplace_plus_post(arr_ptr,elt_sz,change) ({ \
  struct _dyneither_ptr * _arr_ptr = (arr_ptr); \
  struct _dyneither_ptr _ans = *_arr_ptr; \
  _arr_ptr->curr += ((int)(elt_sz))*(change); \
  _ans; })

/* This is not a macro since initialization order matters.  Defined in
   runtime_zeroterm.c. */
extern struct _dyneither_ptr _dyneither_ptr_decrease_size(struct _dyneither_ptr x,
  unsigned int sz,
  unsigned int numelts);

/* Allocation */
extern void* GC_malloc(int);
extern void* GC_malloc_atomic(int);
extern void* GC_calloc(unsigned,unsigned);
extern void* GC_calloc_atomic(unsigned,unsigned);
/* bound the allocation size to be less than MAX_ALLOC_SIZE,
   which is defined in runtime_memory.c
*/
extern void* _bounded_GC_malloc(int,const char *file,int lineno);
extern void* _bounded_GC_malloc_atomic(int,const char *file,int lineno);
extern void* _bounded_GC_calloc(unsigned n, unsigned s,
                                const char *file,int lineno);
extern void* _bounded_GC_calloc_atomic(unsigned n, unsigned s,
                                       const char *file,
                                       int lineno);
/* FIX?  Not sure if we want to pass filename and lineno in here... */
#ifndef CYC_REGION_PROFILE
#define _cycalloc(n) _bounded_GC_malloc(n,__FILE__,__LINE__)
#define _cycalloc_atomic(n) _bounded_GC_malloc_atomic(n,__FILE__,__LINE__)
#define _cyccalloc(n,s) _bounded_GC_calloc(n,s,__FILE__,__LINE__)
#define _cyccalloc_atomic(n,s) _bounded_GC_calloc_atomic(n,s,__FILE__,__LINE__)
#endif

#define MAX_MALLOC_SIZE (1 << 28)
static _INLINE unsigned int _check_times(unsigned x, unsigned y) {
  unsigned long long whole_ans = 
    ((unsigned long long) x)*((unsigned long long)y);
  unsigned word_ans = (unsigned)whole_ans;
  if(word_ans < whole_ans || word_ans > MAX_MALLOC_SIZE)
    _throw_badalloc();
  return word_ans;
}

#define _CYC_MAX_REGION_CONST 2
#define _CYC_MIN_ALIGNMENT (sizeof(double))

#ifdef CYC_REGION_PROFILE
extern int rgn_total_bytes;
#endif

static _INLINE void *_fast_region_malloc(struct _RegionHandle *r, unsigned orig_s) {  
  if (r > (struct _RegionHandle *)_CYC_MAX_REGION_CONST && r->curr != 0) { 
#ifdef CYC_NOALIGN
    unsigned s =  orig_s;
#else
    unsigned s =  (orig_s + _CYC_MIN_ALIGNMENT - 1) & (~(_CYC_MIN_ALIGNMENT -1)); 
#endif
    char *result; 
    result = r->offset; 
    if (s <= (r->last_plus_one - result)) {
      r->offset = result + s; 
#ifdef CYC_REGION_PROFILE
    r->curr->free_bytes = r->curr->free_bytes - s;
    rgn_total_bytes += s;
#endif
      return result;
    }
  } 
  return _region_malloc(r,orig_s); 
}

#if defined(CYC_REGION_PROFILE) 
extern void* _profile_GC_malloc(int,const char *file,const char *func,
                                int lineno);
extern void* _profile_GC_malloc_atomic(int,const char *file,
                                       const char *func,int lineno);
extern void* _profile_GC_calloc(unsigned n, unsigned s,
                                const char *file, const char *func, int lineno);
extern void* _profile_GC_calloc_atomic(unsigned n, unsigned s,
                                       const char *file, const char *func,
                                       int lineno);
extern void* _profile_region_malloc(struct _RegionHandle *, unsigned,
                                    const char *file,
                                    const char *func,
                                    int lineno);
extern void* _profile_region_calloc(struct _RegionHandle *, unsigned,
                                    unsigned,
                                    const char *file,
                                    const char *func,
                                    int lineno);
extern struct _RegionHandle _profile_new_region(const char *rgn_name,
						const char *file,
						const char *func,
                                                int lineno);
extern void _profile_free_region(struct _RegionHandle *,
				 const char *file,
                                 const char *func,
                                 int lineno);
#  if !defined(RUNTIME_CYC)
#define _new_region(n) _profile_new_region(n,__FILE__,__FUNCTION__,__LINE__)
#define _free_region(r) _profile_free_region(r,__FILE__,__FUNCTION__,__LINE__)
#define _region_malloc(rh,n) _profile_region_malloc(rh,n,__FILE__,__FUNCTION__,__LINE__)
#define _region_calloc(rh,n,t) _profile_region_calloc(rh,n,t,__FILE__,__FUNCTION__,__LINE__)
#  endif
#define _cycalloc(n) _profile_GC_malloc(n,__FILE__,__FUNCTION__,__LINE__)
#define _cycalloc_atomic(n) _profile_GC_malloc_atomic(n,__FILE__,__FUNCTION__,__LINE__)
#define _cyccalloc(n,s) _profile_GC_calloc(n,s,__FILE__,__FUNCTION__,__LINE__)
#define _cyccalloc_atomic(n,s) _profile_GC_calloc_atomic(n,s,__FILE__,__FUNCTION__,__LINE__)
#endif
#endif
 struct Cyc_Core_Opt{void*v;};struct _tuple0{void*f1;void*f2;};
# 108 "core.h"
void*Cyc_Core_fst(struct _tuple0*);
# 119
int Cyc_Core_intcmp(int,int);extern char Cyc_Core_Invalid_argument[17U];struct Cyc_Core_Invalid_argument_exn_struct{char*tag;struct _dyneither_ptr f1;};extern char Cyc_Core_Failure[8U];struct Cyc_Core_Failure_exn_struct{char*tag;struct _dyneither_ptr f1;};extern char Cyc_Core_Impossible[11U];struct Cyc_Core_Impossible_exn_struct{char*tag;struct _dyneither_ptr f1;};extern char Cyc_Core_Not_found[10U];struct Cyc_Core_Not_found_exn_struct{char*tag;};extern char Cyc_Core_Unreachable[12U];struct Cyc_Core_Unreachable_exn_struct{char*tag;struct _dyneither_ptr f1;};
# 165
extern struct _RegionHandle*Cyc_Core_heap_region;
# 168
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc___cycFILE;
# 53 "cycboot.h"
extern struct Cyc___cycFILE*Cyc_stderr;struct Cyc_String_pa_PrintArg_struct{int tag;struct _dyneither_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 73
struct _dyneither_ptr Cyc_aprintf(struct _dyneither_ptr,struct _dyneither_ptr);
# 88
int Cyc_fflush(struct Cyc___cycFILE*);
# 100
int Cyc_fprintf(struct Cyc___cycFILE*,struct _dyneither_ptr,struct _dyneither_ptr);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned int*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _dyneither_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _dyneither_ptr f1;};
# 157 "cycboot.h"
int Cyc_printf(struct _dyneither_ptr,struct _dyneither_ptr);extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _dyneither_ptr f1;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 54 "list.h"
struct Cyc_List_List*Cyc_List_list(struct _dyneither_ptr);
# 61
int Cyc_List_length(struct Cyc_List_List*x);
# 70
struct Cyc_List_List*Cyc_List_copy(struct Cyc_List_List*x);
# 76
struct Cyc_List_List*Cyc_List_map(void*(*f)(void*),struct Cyc_List_List*x);
# 83
struct Cyc_List_List*Cyc_List_map_c(void*(*f)(void*,void*),void*env,struct Cyc_List_List*x);
# 86
struct Cyc_List_List*Cyc_List_rmap_c(struct _RegionHandle*,void*(*f)(void*,void*),void*env,struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 94
struct Cyc_List_List*Cyc_List_map2(void*(*f)(void*,void*),struct Cyc_List_List*x,struct Cyc_List_List*y);
# 133
void Cyc_List_iter(void(*f)(void*),struct Cyc_List_List*x);
# 135
void Cyc_List_iter_c(void(*f)(void*,void*),void*env,struct Cyc_List_List*x);
# 161
struct Cyc_List_List*Cyc_List_revappend(struct Cyc_List_List*x,struct Cyc_List_List*y);
# 178
struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*x);
# 190
struct Cyc_List_List*Cyc_List_rappend(struct _RegionHandle*,struct Cyc_List_List*x,struct Cyc_List_List*y);
# 195
struct Cyc_List_List*Cyc_List_imp_append(struct Cyc_List_List*x,struct Cyc_List_List*y);
# 205
struct Cyc_List_List*Cyc_List_rflatten(struct _RegionHandle*,struct Cyc_List_List*x);extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};
# 242
void*Cyc_List_nth(struct Cyc_List_List*x,int n);
# 261
int Cyc_List_exists_c(int(*pred)(void*,void*),void*env,struct Cyc_List_List*x);
# 270
struct Cyc_List_List*Cyc_List_zip(struct Cyc_List_List*x,struct Cyc_List_List*y);
# 276
struct Cyc_List_List*Cyc_List_rzip(struct _RegionHandle*r1,struct _RegionHandle*r2,struct Cyc_List_List*x,struct Cyc_List_List*y);struct _tuple1{struct Cyc_List_List*f1;struct Cyc_List_List*f2;};
# 303
struct _tuple1 Cyc_List_rsplit(struct _RegionHandle*r1,struct _RegionHandle*r2,struct Cyc_List_List*x);
# 322
int Cyc_List_mem(int(*compare)(void*,void*),struct Cyc_List_List*l,void*x);
# 336
void*Cyc_List_assoc_cmp(int(*cmp)(void*,void*),struct Cyc_List_List*l,void*x);
# 383
int Cyc_List_list_cmp(int(*cmp)(void*,void*),struct Cyc_List_List*l1,struct Cyc_List_List*l2);struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;
# 37 "position.h"
struct _dyneither_ptr Cyc_Position_string_of_segment(unsigned int);struct Cyc_Position_Error;
# 47
extern int Cyc_Position_num_errors;
extern int Cyc_Position_max_errors;struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};
# 96 "absyn.h"
union Cyc_Absyn_Nmspace Cyc_Absyn_Loc_n;
union Cyc_Absyn_Nmspace Cyc_Absyn_Rel_n(struct Cyc_List_List*);
# 99
union Cyc_Absyn_Nmspace Cyc_Absyn_Abs_n(struct Cyc_List_List*ns,int C_scope);struct _tuple2{union Cyc_Absyn_Nmspace f1;struct _dyneither_ptr*f2;};
# 159
enum Cyc_Absyn_Scope{Cyc_Absyn_Static  = 0U,Cyc_Absyn_Abstract  = 1U,Cyc_Absyn_Public  = 2U,Cyc_Absyn_Extern  = 3U,Cyc_Absyn_ExternC  = 4U,Cyc_Absyn_Register  = 5U};struct Cyc_Absyn_Tqual{int print_const: 1;int q_volatile: 1;int q_restrict: 1;int real_const: 1;unsigned int loc;};
# 180
enum Cyc_Absyn_Size_of{Cyc_Absyn_Char_sz  = 0U,Cyc_Absyn_Short_sz  = 1U,Cyc_Absyn_Int_sz  = 2U,Cyc_Absyn_Long_sz  = 3U,Cyc_Absyn_LongLong_sz  = 4U};
# 185
enum Cyc_Absyn_AliasQual{Cyc_Absyn_Aliasable  = 0U,Cyc_Absyn_Unique  = 1U,Cyc_Absyn_Top  = 2U};
# 191
enum Cyc_Absyn_KindQual{Cyc_Absyn_AnyKind  = 0U,Cyc_Absyn_MemKind  = 1U,Cyc_Absyn_BoxKind  = 2U,Cyc_Absyn_RgnKind  = 3U,Cyc_Absyn_EffKind  = 4U,Cyc_Absyn_IntKind  = 5U,Cyc_Absyn_BoolKind  = 6U,Cyc_Absyn_PtrBndKind  = 7U};struct Cyc_Absyn_Kind{enum Cyc_Absyn_KindQual kind;enum Cyc_Absyn_AliasQual aliasqual;};
# 213
enum Cyc_Absyn_Sign{Cyc_Absyn_Signed  = 0U,Cyc_Absyn_Unsigned  = 1U,Cyc_Absyn_None  = 2U};
# 215
enum Cyc_Absyn_AggrKind{Cyc_Absyn_StructA  = 0U,Cyc_Absyn_UnionA  = 1U};struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct{int tag;struct Cyc_Absyn_Kind*f1;};struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;};struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_Tvar{struct _dyneither_ptr*name;int identity;void*kind;};struct Cyc_Absyn_PtrLoc{unsigned int ptr_loc;unsigned int rgn_loc;unsigned int zt_loc;};struct Cyc_Absyn_PtrAtts{void*rgn;void*nullable;void*bounds;void*zero_term;struct Cyc_Absyn_PtrLoc*ptrloc;};struct Cyc_Absyn_PtrInfo{void*elt_type;struct Cyc_Absyn_Tqual elt_tq;struct Cyc_Absyn_PtrAtts ptr_atts;};struct Cyc_Absyn_VarargInfo{struct _dyneither_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;int inject;};struct Cyc_Absyn_FnInfo{struct Cyc_List_List*tvars;void*effect;struct Cyc_Absyn_Tqual ret_tqual;void*ret_type;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*cyc_varargs;struct Cyc_List_List*rgn_po;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;struct Cyc_List_List*requires_relns;struct Cyc_Absyn_Exp*ensures_clause;struct Cyc_List_List*ensures_relns;};struct Cyc_Absyn_UnknownDatatypeInfo{struct _tuple2*name;int is_extensible;};struct _union_DatatypeInfo_UnknownDatatype{int tag;struct Cyc_Absyn_UnknownDatatypeInfo val;};struct _union_DatatypeInfo_KnownDatatype{int tag;struct Cyc_Absyn_Datatypedecl**val;};union Cyc_Absyn_DatatypeInfo{struct _union_DatatypeInfo_UnknownDatatype UnknownDatatype;struct _union_DatatypeInfo_KnownDatatype KnownDatatype;};
# 302
union Cyc_Absyn_DatatypeInfo Cyc_Absyn_UnknownDatatype(struct Cyc_Absyn_UnknownDatatypeInfo);
union Cyc_Absyn_DatatypeInfo Cyc_Absyn_KnownDatatype(struct Cyc_Absyn_Datatypedecl**);struct Cyc_Absyn_UnknownDatatypeFieldInfo{struct _tuple2*datatype_name;struct _tuple2*field_name;int is_extensible;};struct _union_DatatypeFieldInfo_UnknownDatatypefield{int tag;struct Cyc_Absyn_UnknownDatatypeFieldInfo val;};struct _tuple3{struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;};struct _union_DatatypeFieldInfo_KnownDatatypefield{int tag;struct _tuple3 val;};union Cyc_Absyn_DatatypeFieldInfo{struct _union_DatatypeFieldInfo_UnknownDatatypefield UnknownDatatypefield;struct _union_DatatypeFieldInfo_KnownDatatypefield KnownDatatypefield;};
# 317
union Cyc_Absyn_DatatypeFieldInfo Cyc_Absyn_KnownDatatypefield(struct Cyc_Absyn_Datatypedecl*,struct Cyc_Absyn_Datatypefield*);struct _tuple4{enum Cyc_Absyn_AggrKind f1;struct _tuple2*f2;struct Cyc_Core_Opt*f3;};struct _union_AggrInfo_UnknownAggr{int tag;struct _tuple4 val;};struct _union_AggrInfo_KnownAggr{int tag;struct Cyc_Absyn_Aggrdecl**val;};union Cyc_Absyn_AggrInfo{struct _union_AggrInfo_UnknownAggr UnknownAggr;struct _union_AggrInfo_KnownAggr KnownAggr;};
# 324
union Cyc_Absyn_AggrInfo Cyc_Absyn_UnknownAggr(enum Cyc_Absyn_AggrKind,struct _tuple2*,struct Cyc_Core_Opt*);
union Cyc_Absyn_AggrInfo Cyc_Absyn_KnownAggr(struct Cyc_Absyn_Aggrdecl**);struct Cyc_Absyn_ArrayInfo{void*elt_type;struct Cyc_Absyn_Tqual tq;struct Cyc_Absyn_Exp*num_elts;void*zero_term;unsigned int zt_loc;};struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_TypeDecl{void*r;unsigned int loc;};struct Cyc_Absyn_VoidCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_IntCon_Absyn_TyCon_struct{int tag;enum Cyc_Absyn_Sign f1;enum Cyc_Absyn_Size_of f2;};struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct{int tag;int f1;};struct Cyc_Absyn_RgnHandleCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TagCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_HeapCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_UniqueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RefCntCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_AccessCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RgnsCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TrueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FalseCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_ThinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FatCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct{int tag;struct _tuple2*f1;struct Cyc_Absyn_Enumdecl*f2;};struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct{int tag;struct _dyneither_ptr f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeInfo f1;};struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeFieldInfo f1;};struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_AggrInfo f1;};struct Cyc_Absyn_AppType_Absyn_Type_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Evar_Absyn_Type_struct{int tag;struct Cyc_Core_Opt*f1;void*f2;int f3;struct Cyc_Core_Opt*f4;};struct Cyc_Absyn_VarType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Tvar*f1;};struct Cyc_Absyn_PointerType_Absyn_Type_struct{int tag;struct Cyc_Absyn_PtrInfo f1;};struct Cyc_Absyn_ArrayType_Absyn_Type_struct{int tag;struct Cyc_Absyn_ArrayInfo f1;};struct Cyc_Absyn_FnType_Absyn_Type_struct{int tag;struct Cyc_Absyn_FnInfo f1;};struct Cyc_Absyn_TupleType_Absyn_Type_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct{int tag;enum Cyc_Absyn_AggrKind f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_TypedefType_Absyn_Type_struct{int tag;struct _tuple2*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_Typedefdecl*f3;void*f4;};struct Cyc_Absyn_ValueofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct{int tag;struct Cyc_Absyn_TypeDecl*f1;void**f2;};struct Cyc_Absyn_TypeofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;unsigned int f2;};struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;int f2;struct Cyc_Absyn_VarargInfo*f3;void*f4;struct Cyc_List_List*f5;struct Cyc_Absyn_Exp*f6;struct Cyc_Absyn_Exp*f7;};
# 427 "absyn.h"
enum Cyc_Absyn_Format_Type{Cyc_Absyn_Printf_ft  = 0U,Cyc_Absyn_Scanf_ft  = 1U};struct Cyc_Absyn_Regparm_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Stdcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Cdecl_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Fastcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Noreturn_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Const_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Packed_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Section_att_Absyn_Attribute_struct{int tag;struct _dyneither_ptr f1;};struct Cyc_Absyn_Nocommon_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Shared_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Unused_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Weak_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllimport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllexport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_instrument_function_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Constructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Destructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_check_memory_usage_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Format_att_Absyn_Attribute_struct{int tag;enum Cyc_Absyn_Format_Type f1;int f2;int f3;};struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Noliveunique_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Consume_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Pure_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Mode_att_Absyn_Attribute_struct{int tag;struct _dyneither_ptr f1;};struct Cyc_Absyn_Alias_att_Absyn_Attribute_struct{int tag;struct _dyneither_ptr f1;};struct Cyc_Absyn_Always_inline_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct{int tag;void*f1;unsigned int f2;};struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;unsigned int f3;};struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_PtrAtts f1;struct Cyc_Absyn_Tqual f2;};struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct{int tag;void*f1;};struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_List_List*f1;unsigned int f2;int f3;};struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct{int tag;unsigned int f1;struct Cyc_List_List*f2;};struct _union_Cnst_Null_c{int tag;int val;};struct _tuple5{enum Cyc_Absyn_Sign f1;char f2;};struct _union_Cnst_Char_c{int tag;struct _tuple5 val;};struct _union_Cnst_Wchar_c{int tag;struct _dyneither_ptr val;};struct _tuple6{enum Cyc_Absyn_Sign f1;short f2;};struct _union_Cnst_Short_c{int tag;struct _tuple6 val;};struct _tuple7{enum Cyc_Absyn_Sign f1;int f2;};struct _union_Cnst_Int_c{int tag;struct _tuple7 val;};struct _tuple8{enum Cyc_Absyn_Sign f1;long long f2;};struct _union_Cnst_LongLong_c{int tag;struct _tuple8 val;};struct _tuple9{struct _dyneither_ptr f1;int f2;};struct _union_Cnst_Float_c{int tag;struct _tuple9 val;};struct _union_Cnst_String_c{int tag;struct _dyneither_ptr val;};struct _union_Cnst_Wstring_c{int tag;struct _dyneither_ptr val;};union Cyc_Absyn_Cnst{struct _union_Cnst_Null_c Null_c;struct _union_Cnst_Char_c Char_c;struct _union_Cnst_Wchar_c Wchar_c;struct _union_Cnst_Short_c Short_c;struct _union_Cnst_Int_c Int_c;struct _union_Cnst_LongLong_c LongLong_c;struct _union_Cnst_Float_c Float_c;struct _union_Cnst_String_c String_c;struct _union_Cnst_Wstring_c Wstring_c;};
# 506
extern union Cyc_Absyn_Cnst Cyc_Absyn_Null_c;
# 517
enum Cyc_Absyn_Primop{Cyc_Absyn_Plus  = 0U,Cyc_Absyn_Times  = 1U,Cyc_Absyn_Minus  = 2U,Cyc_Absyn_Div  = 3U,Cyc_Absyn_Mod  = 4U,Cyc_Absyn_Eq  = 5U,Cyc_Absyn_Neq  = 6U,Cyc_Absyn_Gt  = 7U,Cyc_Absyn_Lt  = 8U,Cyc_Absyn_Gte  = 9U,Cyc_Absyn_Lte  = 10U,Cyc_Absyn_Not  = 11U,Cyc_Absyn_Bitnot  = 12U,Cyc_Absyn_Bitand  = 13U,Cyc_Absyn_Bitor  = 14U,Cyc_Absyn_Bitxor  = 15U,Cyc_Absyn_Bitlshift  = 16U,Cyc_Absyn_Bitlrshift  = 17U,Cyc_Absyn_Bitarshift  = 18U,Cyc_Absyn_Numelts  = 19U};
# 524
enum Cyc_Absyn_Incrementor{Cyc_Absyn_PreInc  = 0U,Cyc_Absyn_PostInc  = 1U,Cyc_Absyn_PreDec  = 2U,Cyc_Absyn_PostDec  = 3U};struct Cyc_Absyn_VarargCallInfo{int num_varargs;struct Cyc_List_List*injectors;struct Cyc_Absyn_VarargInfo*vai;};struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct{int tag;struct _dyneither_ptr*f1;};struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct{int tag;unsigned int f1;};
# 542
enum Cyc_Absyn_Coercion{Cyc_Absyn_Unknown_coercion  = 0U,Cyc_Absyn_No_coercion  = 1U,Cyc_Absyn_Null_to_NonNull  = 2U,Cyc_Absyn_Other_coercion  = 3U};struct Cyc_Absyn_MallocInfo{int is_calloc;struct Cyc_Absyn_Exp*rgn;void**elt_type;struct Cyc_Absyn_Exp*num_elts;int fat_result;int inline_call;};struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct{int tag;union Cyc_Absyn_Cnst f1;};struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct{int tag;struct _dyneither_ptr f1;};struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct{int tag;enum Cyc_Absyn_Primop f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;enum Cyc_Absyn_Incrementor f2;};struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_VarargCallInfo*f3;int f4;};struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;};struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Exp*f2;int f3;enum Cyc_Absyn_Coercion f4;};struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _dyneither_ptr*f2;int f3;int f4;};struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _dyneither_ptr*f2;int f3;int f4;};struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct _tuple10{struct _dyneither_ptr*f1;struct Cyc_Absyn_Tqual f2;void*f3;};struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct{int tag;struct _tuple10*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;int f4;};struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;int f3;};struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct{int tag;struct _tuple2*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*f4;};struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_MallocInfo f1;};struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _dyneither_ptr*f2;};struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct{int tag;int f1;struct _dyneither_ptr f2;};struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Exp{void*topt;void*r;unsigned int loc;void*annot;};struct Cyc_Absyn_Skip_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;struct Cyc_Absyn_Stmt*f3;};struct _tuple11{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct{int tag;struct _tuple11 f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Break_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Continue_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct{int tag;struct _dyneither_ptr*f1;};struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _tuple11 f2;struct _tuple11 f3;struct Cyc_Absyn_Stmt*f4;};struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Switch_clause**f2;};struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct{int tag;struct _dyneither_ptr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct _tuple11 f2;};struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Stmt{void*r;unsigned int loc;void*annot;};struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct{int tag;struct Cyc_List_List*f1;int f2;};struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Pat*f1;};struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct{int tag;union Cyc_Absyn_AggrInfo*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Null_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct{int tag;enum Cyc_Absyn_Sign f1;int f2;};struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct{int tag;char f1;};struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct{int tag;struct _dyneither_ptr f1;int f2;};struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_UnknownId_p_Absyn_Raw_pat_struct{int tag;struct _tuple2*f1;};struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct{int tag;struct _tuple2*f1;struct Cyc_List_List*f2;int f3;};struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Pat{void*r;void*topt;unsigned int loc;};struct Cyc_Absyn_Switch_clause{struct Cyc_Absyn_Pat*pattern;struct Cyc_Core_Opt*pat_vars;struct Cyc_Absyn_Exp*where_clause;struct Cyc_Absyn_Stmt*body;unsigned int loc;};struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct{int tag;struct _tuple2*f1;};struct Cyc_Absyn_Global_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Funname_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Param_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Local_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Pat_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Vardecl{enum Cyc_Absyn_Scope sc;struct _tuple2*name;unsigned int varloc;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*initializer;void*rgn;struct Cyc_List_List*attributes;int escapes;};struct Cyc_Absyn_Fndecl{enum Cyc_Absyn_Scope sc;int is_inline;struct _tuple2*name;struct Cyc_Absyn_Stmt*body;struct Cyc_Absyn_FnInfo i;void*cached_type;struct Cyc_Core_Opt*param_vardecls;struct Cyc_Absyn_Vardecl*fn_vardecl;};struct Cyc_Absyn_Aggrfield{struct _dyneither_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*width;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;};struct Cyc_Absyn_AggrdeclImpl{struct Cyc_List_List*exist_vars;struct Cyc_List_List*rgn_po;struct Cyc_List_List*fields;int tagged;};struct Cyc_Absyn_Aggrdecl{enum Cyc_Absyn_AggrKind kind;enum Cyc_Absyn_Scope sc;struct _tuple2*name;struct Cyc_List_List*tvs;struct Cyc_Absyn_AggrdeclImpl*impl;struct Cyc_List_List*attributes;int expected_mem_kind;};struct Cyc_Absyn_Datatypefield{struct _tuple2*name;struct Cyc_List_List*typs;unsigned int loc;enum Cyc_Absyn_Scope sc;};struct Cyc_Absyn_Datatypedecl{enum Cyc_Absyn_Scope sc;struct _tuple2*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*fields;int is_extensible;};struct Cyc_Absyn_Enumfield{struct _tuple2*name;struct Cyc_Absyn_Exp*tag;unsigned int loc;};struct Cyc_Absyn_Enumdecl{enum Cyc_Absyn_Scope sc;struct _tuple2*name;struct Cyc_Core_Opt*fields;};struct Cyc_Absyn_Typedefdecl{struct _tuple2*name;struct Cyc_Absyn_Tqual tq;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*kind;void*defn;struct Cyc_List_List*atts;int extern_c;};struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Pat*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;void*f4;};struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Typedefdecl*f1;};struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct{int tag;struct _dyneither_ptr*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct{int tag;struct _tuple2*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Porton_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Portoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempeston_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempestoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Decl{void*r;unsigned int loc;};struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_FieldName_Absyn_Designator_struct{int tag;struct _dyneither_ptr*f1;};extern char Cyc_Absyn_EmptyAnnot[11U];struct Cyc_Absyn_EmptyAnnot_Absyn_AbsynAnnot_struct{char*tag;};
# 890 "absyn.h"
int Cyc_Absyn_qvar_cmp(struct _tuple2*,struct _tuple2*);
# 892
int Cyc_Absyn_tvar_cmp(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*);
# 900
struct Cyc_Absyn_Tqual Cyc_Absyn_empty_tqual(unsigned int);
# 903
void*Cyc_Absyn_compress_kb(void*);
# 909
int Cyc_Absyn_type2bool(int def,void*);
# 914
void*Cyc_Absyn_new_evar(struct Cyc_Core_Opt*k,struct Cyc_Core_Opt*tenv);
# 919
extern void*Cyc_Absyn_uint_type;extern void*Cyc_Absyn_ulong_type;extern void*Cyc_Absyn_ulonglong_type;
# 921
extern void*Cyc_Absyn_sint_type;extern void*Cyc_Absyn_slong_type;extern void*Cyc_Absyn_slonglong_type;
# 926
extern void*Cyc_Absyn_heap_rgn_type;extern void*Cyc_Absyn_unique_rgn_type;extern void*Cyc_Absyn_refcnt_rgn_type;
# 928
extern void*Cyc_Absyn_empty_effect;
# 930
extern void*Cyc_Absyn_true_type;extern void*Cyc_Absyn_false_type;
# 932
extern void*Cyc_Absyn_void_type;void*Cyc_Absyn_var_type(struct Cyc_Absyn_Tvar*);void*Cyc_Absyn_access_eff(void*);void*Cyc_Absyn_join_eff(struct Cyc_List_List*);void*Cyc_Absyn_regionsof_eff(void*);void*Cyc_Absyn_enum_type(struct _tuple2*n,struct Cyc_Absyn_Enumdecl*d);
# 951
extern struct _tuple2*Cyc_Absyn_datatype_print_arg_qvar;
extern struct _tuple2*Cyc_Absyn_datatype_scanf_arg_qvar;
# 957
extern void*Cyc_Absyn_fat_bound_type;
# 959
void*Cyc_Absyn_thin_bounds_exp(struct Cyc_Absyn_Exp*);
# 961
void*Cyc_Absyn_bounds_one();
# 963
void*Cyc_Absyn_pointer_type(struct Cyc_Absyn_PtrInfo);
# 968
void*Cyc_Absyn_atb_type(void*t,void*rgn,struct Cyc_Absyn_Tqual tq,void*b,void*zero_term);
# 990
void*Cyc_Absyn_datatype_type(union Cyc_Absyn_DatatypeInfo,struct Cyc_List_List*args);
# 992
void*Cyc_Absyn_aggr_type(union Cyc_Absyn_AggrInfo,struct Cyc_List_List*args);
# 995
struct Cyc_Absyn_Exp*Cyc_Absyn_new_exp(void*,unsigned int);
struct Cyc_Absyn_Exp*Cyc_Absyn_New_exp(struct Cyc_Absyn_Exp*rgn_handle,struct Cyc_Absyn_Exp*,unsigned int);
struct Cyc_Absyn_Exp*Cyc_Absyn_copy_exp(struct Cyc_Absyn_Exp*);
struct Cyc_Absyn_Exp*Cyc_Absyn_const_exp(union Cyc_Absyn_Cnst,unsigned int);
# 1005
struct Cyc_Absyn_Exp*Cyc_Absyn_uint_exp(unsigned int,unsigned int);
# 1012
struct Cyc_Absyn_Exp*Cyc_Absyn_varb_exp(void*,unsigned int);
# 1014
struct Cyc_Absyn_Exp*Cyc_Absyn_pragma_exp(struct _dyneither_ptr s,unsigned int loc);
struct Cyc_Absyn_Exp*Cyc_Absyn_primop_exp(enum Cyc_Absyn_Primop,struct Cyc_List_List*es,unsigned int);
# 1018
struct Cyc_Absyn_Exp*Cyc_Absyn_swap_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned int);
# 1028
struct Cyc_Absyn_Exp*Cyc_Absyn_assignop_exp(struct Cyc_Absyn_Exp*,struct Cyc_Core_Opt*,struct Cyc_Absyn_Exp*,unsigned int);
# 1030
struct Cyc_Absyn_Exp*Cyc_Absyn_increment_exp(struct Cyc_Absyn_Exp*,enum Cyc_Absyn_Incrementor,unsigned int);
# 1035
struct Cyc_Absyn_Exp*Cyc_Absyn_conditional_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned int);
struct Cyc_Absyn_Exp*Cyc_Absyn_and_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned int);
struct Cyc_Absyn_Exp*Cyc_Absyn_or_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned int);
struct Cyc_Absyn_Exp*Cyc_Absyn_seq_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned int);
# 1041
struct Cyc_Absyn_Exp*Cyc_Absyn_throw_exp(struct Cyc_Absyn_Exp*,unsigned int);
struct Cyc_Absyn_Exp*Cyc_Absyn_rethrow_exp(struct Cyc_Absyn_Exp*,unsigned int);
struct Cyc_Absyn_Exp*Cyc_Absyn_noinstantiate_exp(struct Cyc_Absyn_Exp*,unsigned int);
struct Cyc_Absyn_Exp*Cyc_Absyn_instantiate_exp(struct Cyc_Absyn_Exp*,struct Cyc_List_List*,unsigned int);
struct Cyc_Absyn_Exp*Cyc_Absyn_cast_exp(void*,struct Cyc_Absyn_Exp*,int user_cast,enum Cyc_Absyn_Coercion,unsigned int);
struct Cyc_Absyn_Exp*Cyc_Absyn_address_exp(struct Cyc_Absyn_Exp*,unsigned int);
struct Cyc_Absyn_Exp*Cyc_Absyn_sizeoftype_exp(void*t,unsigned int);
struct Cyc_Absyn_Exp*Cyc_Absyn_sizeofexp_exp(struct Cyc_Absyn_Exp*e,unsigned int);
struct Cyc_Absyn_Exp*Cyc_Absyn_offsetof_exp(void*,struct Cyc_List_List*,unsigned int);
struct Cyc_Absyn_Exp*Cyc_Absyn_deref_exp(struct Cyc_Absyn_Exp*,unsigned int);
# 1053
struct Cyc_Absyn_Exp*Cyc_Absyn_subscript_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned int);
struct Cyc_Absyn_Exp*Cyc_Absyn_tuple_exp(struct Cyc_List_List*,unsigned int);
# 1059
struct Cyc_Absyn_Exp*Cyc_Absyn_valueof_exp(void*,unsigned int);
struct Cyc_Absyn_Exp*Cyc_Absyn_asm_exp(int volatile_kw,struct _dyneither_ptr body,unsigned int);
struct Cyc_Absyn_Exp*Cyc_Absyn_extension_exp(struct Cyc_Absyn_Exp*,unsigned int);
# 1100
struct Cyc_Absyn_Decl*Cyc_Absyn_alias_decl(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Vardecl*,struct Cyc_Absyn_Exp*,unsigned int);
struct Cyc_Absyn_Vardecl*Cyc_Absyn_new_vardecl(unsigned int varloc,struct _tuple2*x,void*t,struct Cyc_Absyn_Exp*init);
# 1147
struct Cyc_Absyn_Aggrfield*Cyc_Absyn_lookup_field(struct Cyc_List_List*,struct _dyneither_ptr*);
# 1149
struct Cyc_Absyn_Aggrfield*Cyc_Absyn_lookup_decl_field(struct Cyc_Absyn_Aggrdecl*,struct _dyneither_ptr*);struct _tuple12{struct Cyc_Absyn_Tqual f1;void*f2;};
# 1151
struct _tuple12*Cyc_Absyn_lookup_tuple_field(struct Cyc_List_List*,int);
# 1153
struct _dyneither_ptr Cyc_Absyn_attribute2string(void*);
# 1155
int Cyc_Absyn_fntype_att(void*);
# 1161
struct Cyc_Absyn_Aggrdecl*Cyc_Absyn_get_known_aggrdecl(union Cyc_Absyn_AggrInfo);struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};
# 53 "absynpp.h"
void Cyc_Absynpp_set_params(struct Cyc_Absynpp_Params*fs);
# 55
extern struct Cyc_Absynpp_Params Cyc_Absynpp_tc_params_r;
# 62
struct _dyneither_ptr Cyc_Absynpp_typ2string(void*);
# 64
struct _dyneither_ptr Cyc_Absynpp_kind2string(struct Cyc_Absyn_Kind*);
struct _dyneither_ptr Cyc_Absynpp_kindbound2string(void*);
# 67
struct _dyneither_ptr Cyc_Absynpp_exp2string(struct Cyc_Absyn_Exp*);
# 69
struct _dyneither_ptr Cyc_Absynpp_qvar2string(struct _tuple2*);
# 74
struct _dyneither_ptr Cyc_Absynpp_tvar2string(struct Cyc_Absyn_Tvar*);
# 38 "string.h"
unsigned long Cyc_strlen(struct _dyneither_ptr s);
# 49 "string.h"
int Cyc_strcmp(struct _dyneither_ptr s1,struct _dyneither_ptr s2);
int Cyc_strptrcmp(struct _dyneither_ptr*s1,struct _dyneither_ptr*s2);
# 62
struct _dyneither_ptr Cyc_strconcat(struct _dyneither_ptr,struct _dyneither_ptr);
# 26 "warn.h"
void Cyc_Warn_vwarn(unsigned int loc,struct _dyneither_ptr fmt,struct _dyneither_ptr ap);
# 28
void Cyc_Warn_warn(unsigned int loc,struct _dyneither_ptr fmt,struct _dyneither_ptr ap);
# 30
void Cyc_Warn_flush_warnings();
# 32
void Cyc_Warn_verr(unsigned int loc,struct _dyneither_ptr fmt,struct _dyneither_ptr ap);
# 34
void Cyc_Warn_err(unsigned int loc,struct _dyneither_ptr fmt,struct _dyneither_ptr ap);
# 37
void*Cyc_Warn_vimpos(struct _dyneither_ptr fmt,struct _dyneither_ptr ap);
# 39
void*Cyc_Warn_impos(struct _dyneither_ptr fmt,struct _dyneither_ptr ap);
# 42
void*Cyc_Warn_vimpos_loc(unsigned int loc,struct _dyneither_ptr fmt,struct _dyneither_ptr ap);
# 45
void*Cyc_Warn_impos_loc(unsigned int loc,struct _dyneither_ptr fmt,struct _dyneither_ptr ap);struct _tuple13{unsigned int f1;int f2;};
# 28 "evexp.h"
struct _tuple13 Cyc_Evexp_eval_const_uint_exp(struct Cyc_Absyn_Exp*e);
# 41 "evexp.h"
int Cyc_Evexp_same_const_exp(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2);
int Cyc_Evexp_lte_const_exp(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2);
# 45
int Cyc_Evexp_const_exp_cmp(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2);struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};
# 37 "iter.h"
int Cyc_Iter_next(struct Cyc_Iter_Iter,void*);struct Cyc_Set_Set;extern char Cyc_Set_Absent[7U];struct Cyc_Set_Absent_exn_struct{char*tag;};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};struct Cyc_RgnOrder_RgnPO;
# 32 "rgnorder.h"
struct Cyc_RgnOrder_RgnPO*Cyc_RgnOrder_initial_fn_po(struct Cyc_List_List*tvs,struct Cyc_List_List*po,void*effect,struct Cyc_Absyn_Tvar*fst_rgn,unsigned int);
# 39
struct Cyc_RgnOrder_RgnPO*Cyc_RgnOrder_add_outlives_constraint(struct Cyc_RgnOrder_RgnPO*,void*eff,void*rgn,unsigned int);
struct Cyc_RgnOrder_RgnPO*Cyc_RgnOrder_add_youngest(struct Cyc_RgnOrder_RgnPO*,struct Cyc_Absyn_Tvar*rgn,int opened);
struct Cyc_RgnOrder_RgnPO*Cyc_RgnOrder_add_unordered(struct Cyc_RgnOrder_RgnPO*,struct Cyc_Absyn_Tvar*rgn);
int Cyc_RgnOrder_effect_outlives(struct Cyc_RgnOrder_RgnPO*,void*eff,void*rgn);
int Cyc_RgnOrder_satisfies_constraints(struct Cyc_RgnOrder_RgnPO*,struct Cyc_List_List*constraints,void*default_bound,int do_pin);
# 45
int Cyc_RgnOrder_eff_outlives_eff(struct Cyc_RgnOrder_RgnPO*,void*eff1,void*eff2);
# 48
void Cyc_RgnOrder_print_region_po(struct Cyc_RgnOrder_RgnPO*po);extern char Cyc_Tcenv_Env_error[10U];struct Cyc_Tcenv_Env_error_exn_struct{char*tag;};struct Cyc_Tcenv_Genv{struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;};struct Cyc_Tcenv_Fenv;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Tcenv_Genv*ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof: 1;int in_extern_c_include: 1;int in_tempest: 1;int tempest_generalize: 1;};
# 72 "tcenv.h"
struct Cyc_Tcenv_Fenv*Cyc_Tcenv_bogus_fenv(void*ret_type,struct Cyc_List_List*args);
# 77
struct Cyc_Absyn_Aggrdecl**Cyc_Tcenv_lookup_aggrdecl(struct Cyc_Tcenv_Tenv*,unsigned int,struct _tuple2*);
struct Cyc_Absyn_Datatypedecl**Cyc_Tcenv_lookup_datatypedecl(struct Cyc_Tcenv_Tenv*,unsigned int,struct _tuple2*);
# 80
struct Cyc_Absyn_Enumdecl**Cyc_Tcenv_lookup_enumdecl(struct Cyc_Tcenv_Tenv*,unsigned int,struct _tuple2*);
struct Cyc_Absyn_Typedefdecl*Cyc_Tcenv_lookup_typedefdecl(struct Cyc_Tcenv_Tenv*,unsigned int,struct _tuple2*);
# 83
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_allow_valueof(struct Cyc_Tcenv_Tenv*);
# 89
enum Cyc_Tcenv_NewStatus{Cyc_Tcenv_NoneNew  = 0U,Cyc_Tcenv_InNew  = 1U,Cyc_Tcenv_InNewAggr  = 2U};
# 99
struct Cyc_List_List*Cyc_Tcenv_lookup_type_vars(struct Cyc_Tcenv_Tenv*);
struct Cyc_Core_Opt*Cyc_Tcenv_lookup_opt_type_vars(struct Cyc_Tcenv_Tenv*);
# 146
int Cyc_Tcenv_region_outlives(struct Cyc_Tcenv_Tenv*,void*r1,void*r2);
# 31 "tcutil.h"
void*Cyc_Tcutil_impos(struct _dyneither_ptr,struct _dyneither_ptr);
# 33
void Cyc_Tcutil_terr(unsigned int,struct _dyneither_ptr,struct _dyneither_ptr);
# 35
void Cyc_Tcutil_warn(unsigned int,struct _dyneither_ptr,struct _dyneither_ptr);
# 40
int Cyc_Tcutil_is_void_type(void*);
int Cyc_Tcutil_is_char_type(void*);
int Cyc_Tcutil_is_any_int_type(void*);
int Cyc_Tcutil_is_any_float_type(void*);
int Cyc_Tcutil_is_integral_type(void*);
int Cyc_Tcutil_is_arithmetic_type(void*);
int Cyc_Tcutil_is_signed_type(void*);
int Cyc_Tcutil_is_function_type(void*t);
int Cyc_Tcutil_is_pointer_type(void*t);
int Cyc_Tcutil_is_array_type(void*t);
int Cyc_Tcutil_is_boxed(void*t);
# 53
int Cyc_Tcutil_is_dyneither_ptr(void*t);
int Cyc_Tcutil_is_zeroterm_pointer_type(void*t);
int Cyc_Tcutil_is_nullable_pointer_type(void*t);
int Cyc_Tcutil_is_bound_one(void*b);
# 58
int Cyc_Tcutil_is_tagged_pointer_type(void*);
# 61
int Cyc_Tcutil_is_bits_only_type(void*t);
# 63
int Cyc_Tcutil_is_noreturn_fn_type(void*);
# 68
void*Cyc_Tcutil_pointer_elt_type(void*t);
# 70
void*Cyc_Tcutil_pointer_region(void*t);
# 73
int Cyc_Tcutil_rgn_of_pointer(void*t,void**rgn);
# 76
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_bounds_exp(void*def,void*b);
# 79
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_type_bound(void*t);
# 81
int Cyc_Tcutil_is_tagged_pointer_type_elt(void*t,void**elt_dest);
# 83
int Cyc_Tcutil_is_zero_pointer_type_elt(void*t,void**elt_type_dest);
# 85
int Cyc_Tcutil_is_zero_ptr_type(void*t,void**ptr_type,int*is_dyneither,void**elt_type);
# 89
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_bounds_exp(void*def,void*b);
# 92
int Cyc_Tcutil_is_integral(struct Cyc_Absyn_Exp*);
int Cyc_Tcutil_is_numeric(struct Cyc_Absyn_Exp*);
int Cyc_Tcutil_is_zero(struct Cyc_Absyn_Exp*e);
# 99
void*Cyc_Tcutil_copy_type(void*t);
# 102
struct Cyc_Absyn_Exp*Cyc_Tcutil_deep_copy_exp(int preserve_types,struct Cyc_Absyn_Exp*);
# 105
int Cyc_Tcutil_kind_leq(struct Cyc_Absyn_Kind*k1,struct Cyc_Absyn_Kind*k2);
# 109
struct Cyc_Absyn_Kind*Cyc_Tcutil_tvar_kind(struct Cyc_Absyn_Tvar*t,struct Cyc_Absyn_Kind*def);
struct Cyc_Absyn_Kind*Cyc_Tcutil_type_kind(void*t);
int Cyc_Tcutil_kind_eq(struct Cyc_Absyn_Kind*k1,struct Cyc_Absyn_Kind*k2);
void*Cyc_Tcutil_compress(void*t);
void Cyc_Tcutil_unchecked_cast(struct Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Exp*,void*,enum Cyc_Absyn_Coercion);
int Cyc_Tcutil_coerce_arg(struct Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Exp*,void*,int*alias_coercion);
int Cyc_Tcutil_coerce_assign(struct Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Exp*,void*);
int Cyc_Tcutil_coerce_to_bool(struct Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Exp*);
int Cyc_Tcutil_coerce_list(struct Cyc_Tcenv_Tenv*,void*,struct Cyc_List_List*);
int Cyc_Tcutil_coerce_uint_type(struct Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Exp*);
int Cyc_Tcutil_coerce_sint_type(struct Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Exp*);
# 122
int Cyc_Tcutil_silent_castable(struct Cyc_Tcenv_Tenv*,unsigned int,void*,void*);
# 124
enum Cyc_Absyn_Coercion Cyc_Tcutil_castable(struct Cyc_Tcenv_Tenv*,unsigned int,void*,void*);
# 126
int Cyc_Tcutil_subtype(struct Cyc_Tcenv_Tenv*te,struct Cyc_List_List*assume,void*t1,void*t2);struct _tuple14{struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Exp*f2;};
# 130
struct _tuple14 Cyc_Tcutil_insert_alias(struct Cyc_Absyn_Exp*e,void*e_typ);
# 132
extern int Cyc_Tcutil_warn_alias_coerce;
# 135
extern int Cyc_Tcutil_warn_region_coerce;
# 139
extern struct Cyc_Absyn_Kind Cyc_Tcutil_rk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_ak;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_bk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_mk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_ek;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_ik;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_boolk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_ptrbk;
# 148
extern struct Cyc_Absyn_Kind Cyc_Tcutil_trk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_tak;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_tbk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_tmk;
# 153
extern struct Cyc_Absyn_Kind Cyc_Tcutil_urk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_uak;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_ubk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_umk;
# 158
extern struct Cyc_Core_Opt Cyc_Tcutil_rko;
extern struct Cyc_Core_Opt Cyc_Tcutil_ako;
extern struct Cyc_Core_Opt Cyc_Tcutil_bko;
extern struct Cyc_Core_Opt Cyc_Tcutil_mko;
extern struct Cyc_Core_Opt Cyc_Tcutil_iko;
extern struct Cyc_Core_Opt Cyc_Tcutil_eko;
extern struct Cyc_Core_Opt Cyc_Tcutil_boolko;
extern struct Cyc_Core_Opt Cyc_Tcutil_ptrbko;
# 167
extern struct Cyc_Core_Opt Cyc_Tcutil_trko;
extern struct Cyc_Core_Opt Cyc_Tcutil_tako;
extern struct Cyc_Core_Opt Cyc_Tcutil_tbko;
extern struct Cyc_Core_Opt Cyc_Tcutil_tmko;
# 172
extern struct Cyc_Core_Opt Cyc_Tcutil_urko;
extern struct Cyc_Core_Opt Cyc_Tcutil_uako;
extern struct Cyc_Core_Opt Cyc_Tcutil_ubko;
extern struct Cyc_Core_Opt Cyc_Tcutil_umko;
# 177
struct Cyc_Core_Opt*Cyc_Tcutil_kind_to_opt(struct Cyc_Absyn_Kind*k);
void*Cyc_Tcutil_kind_to_bound(struct Cyc_Absyn_Kind*k);
int Cyc_Tcutil_unify_kindbound(void*,void*);struct _tuple15{struct Cyc_Absyn_Tvar*f1;void*f2;};
# 181
struct _tuple15 Cyc_Tcutil_swap_kind(void*t,void*kb);
# 186
int Cyc_Tcutil_zero_to_null(struct Cyc_Tcenv_Tenv*,void*t,struct Cyc_Absyn_Exp*e);
# 188
void*Cyc_Tcutil_max_arithmetic_type(void*,void*);
# 192
void Cyc_Tcutil_explain_failure();
# 194
int Cyc_Tcutil_unify(void*,void*);
# 196
int Cyc_Tcutil_typecmp(void*,void*);
int Cyc_Tcutil_aggrfield_cmp(struct Cyc_Absyn_Aggrfield*,struct Cyc_Absyn_Aggrfield*);
# 199
void*Cyc_Tcutil_substitute(struct Cyc_List_List*,void*);
# 201
void*Cyc_Tcutil_rsubstitute(struct _RegionHandle*,struct Cyc_List_List*,void*);
struct Cyc_List_List*Cyc_Tcutil_rsubst_rgnpo(struct _RegionHandle*,struct Cyc_List_List*,struct Cyc_List_List*);
# 207
struct Cyc_Absyn_Exp*Cyc_Tcutil_rsubsexp(struct _RegionHandle*r,struct Cyc_List_List*,struct Cyc_Absyn_Exp*);
# 210
int Cyc_Tcutil_subset_effect(int may_constrain_evars,void*e1,void*e2);
# 214
int Cyc_Tcutil_region_in_effect(int constrain,void*r,void*e);
# 216
void*Cyc_Tcutil_fndecl2type(struct Cyc_Absyn_Fndecl*);
# 220
struct _tuple15*Cyc_Tcutil_make_inst_var(struct Cyc_List_List*,struct Cyc_Absyn_Tvar*);struct _tuple16{struct Cyc_List_List*f1;struct _RegionHandle*f2;};
struct _tuple15*Cyc_Tcutil_r_make_inst_var(struct _tuple16*,struct Cyc_Absyn_Tvar*);
# 226
void Cyc_Tcutil_check_bitfield(unsigned int loc,struct Cyc_Tcenv_Tenv*te,void*field_typ,struct Cyc_Absyn_Exp*width,struct _dyneither_ptr*fn);
# 253 "tcutil.h"
void Cyc_Tcutil_check_valid_toplevel_type(unsigned int,struct Cyc_Tcenv_Tenv*,void*);
# 255
void Cyc_Tcutil_check_fndecl_valid_type(unsigned int,struct Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Fndecl*);
# 263
void Cyc_Tcutil_check_type(unsigned int,struct Cyc_Tcenv_Tenv*,struct Cyc_List_List*bound_tvars,struct Cyc_Absyn_Kind*k,int allow_evars,int allow_abs_aggr,void*);
# 266
void Cyc_Tcutil_check_unique_vars(struct Cyc_List_List*vs,unsigned int loc,struct _dyneither_ptr err_msg);
void Cyc_Tcutil_check_unique_tvars(unsigned int,struct Cyc_List_List*);
# 273
void Cyc_Tcutil_check_nonzero_bound(unsigned int,void*);
# 275
void Cyc_Tcutil_check_bound(unsigned int,unsigned int i,void*,int do_warn);
# 277
int Cyc_Tcutil_equal_tqual(struct Cyc_Absyn_Tqual tq1,struct Cyc_Absyn_Tqual tq2);
# 279
struct Cyc_List_List*Cyc_Tcutil_resolve_aggregate_designators(struct _RegionHandle*rgn,unsigned int loc,struct Cyc_List_List*des,enum Cyc_Absyn_AggrKind,struct Cyc_List_List*fields);
# 287
int Cyc_Tcutil_is_zero_ptr_deref(struct Cyc_Absyn_Exp*e1,void**ptr_type,int*is_dyneither,void**elt_type);
# 292
int Cyc_Tcutil_is_noalias_region(void*r,int must_be_unique);
# 295
int Cyc_Tcutil_is_noalias_pointer(void*t,int must_be_unique);
# 300
int Cyc_Tcutil_is_noalias_path(struct Cyc_Absyn_Exp*e);
# 305
int Cyc_Tcutil_is_noalias_pointer_or_aggr(void*t);struct _tuple17{int f1;void*f2;};
# 309
struct _tuple17 Cyc_Tcutil_addressof_props(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*e);
# 312
void*Cyc_Tcutil_normalize_effect(void*e);
# 315
struct Cyc_Absyn_Tvar*Cyc_Tcutil_new_tvar(void*k);
# 317
int Cyc_Tcutil_new_tvar_id();
# 319
void Cyc_Tcutil_add_tvar_identity(struct Cyc_Absyn_Tvar*);
void Cyc_Tcutil_add_tvar_identities(struct Cyc_List_List*);
# 322
int Cyc_Tcutil_is_temp_tvar(struct Cyc_Absyn_Tvar*);
# 324
void Cyc_Tcutil_rewrite_temp_tvar(struct Cyc_Absyn_Tvar*);
# 327
int Cyc_Tcutil_same_atts(struct Cyc_List_List*,struct Cyc_List_List*);
# 330
int Cyc_Tcutil_is_const_exp(struct Cyc_Absyn_Exp*e);
# 333
void*Cyc_Tcutil_snd_tqt(struct _tuple12*);
# 337
int Cyc_Tcutil_extract_const_from_typedef(unsigned int,int declared_const,void*);
# 341
struct Cyc_List_List*Cyc_Tcutil_transfer_fn_type_atts(void*t,struct Cyc_List_List*atts);
# 344
void Cyc_Tcutil_check_no_qual(unsigned int loc,void*t);
# 348
struct Cyc_Absyn_Vardecl*Cyc_Tcutil_nonesc_vardecl(void*b);
# 351
struct Cyc_List_List*Cyc_Tcutil_filter_nulls(struct Cyc_List_List*l);
# 355
void*Cyc_Tcutil_promote_array(void*t,void*rgn,int convert_tag);
# 358
int Cyc_Tcutil_zeroable_type(void*t);
# 362
int Cyc_Tcutil_force_type2bool(int desired,void*t);
# 365
void*Cyc_Tcutil_any_bool(struct Cyc_Tcenv_Tenv**te);
# 367
void*Cyc_Tcutil_any_bounds(struct Cyc_Tcenv_Tenv**te);
# 28 "tcexp.h"
void*Cyc_Tcexp_tcExp(struct Cyc_Tcenv_Tenv*,void**,struct Cyc_Absyn_Exp*);
# 40 "tc.h"
void Cyc_Tc_tcAggrdecl(struct Cyc_Tcenv_Tenv*,unsigned int,struct Cyc_Absyn_Aggrdecl*);
void Cyc_Tc_tcDatatypedecl(struct Cyc_Tcenv_Tenv*,unsigned int,struct Cyc_Absyn_Datatypedecl*);
void Cyc_Tc_tcEnumdecl(struct Cyc_Tcenv_Tenv*,unsigned int,struct Cyc_Absyn_Enumdecl*);
# 24 "cyclone.h"
extern int Cyc_Cyclone_tovc_r;
# 26
enum Cyc_Cyclone_C_Compilers{Cyc_Cyclone_Gcc_c  = 0U,Cyc_Cyclone_Vc_c  = 1U};struct _union_RelnOp_RConst{int tag;unsigned int val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned int val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned int val;};struct _union_RelnOp_RReturn{int tag;unsigned int val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 41 "relations-ap.h"
union Cyc_Relations_RelnOp Cyc_Relations_RParam(unsigned int);union Cyc_Relations_RelnOp Cyc_Relations_RParamNumelts(unsigned int);union Cyc_Relations_RelnOp Cyc_Relations_RReturn();
# 50
enum Cyc_Relations_Relation{Cyc_Relations_Req  = 0U,Cyc_Relations_Rneq  = 1U,Cyc_Relations_Rlte  = 2U,Cyc_Relations_Rlt  = 3U};struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};
# 70
struct Cyc_Relations_Reln*Cyc_Relations_negate(struct _RegionHandle*,struct Cyc_Relations_Reln*);
# 84
struct Cyc_List_List*Cyc_Relations_exp2relns(struct _RegionHandle*r,struct Cyc_Absyn_Exp*e);
# 110
struct Cyc_List_List*Cyc_Relations_copy_relns(struct _RegionHandle*,struct Cyc_List_List*);
# 129
int Cyc_Relations_consistent_relations(struct Cyc_List_List*rlns);
# 42 "tcutil.cyc"
int Cyc_Tcutil_is_void_type(void*t){
void*_tmp0=Cyc_Tcutil_compress(t);void*_tmp1=_tmp0;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1)->tag == 0U){if(((struct Cyc_Absyn_VoidCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1)->f1)->tag == 0U){_LL1: _LL2:
 return 1;}else{goto _LL3;}}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 48
int Cyc_Tcutil_is_array_type(void*t){
void*_tmp2=Cyc_Tcutil_compress(t);void*_tmp3=_tmp2;if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp3)->tag == 4U){_LL1: _LL2:
 return 1;}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 54
int Cyc_Tcutil_is_heap_rgn_type(void*t){
void*_tmp4=Cyc_Tcutil_compress(t);void*_tmp5=_tmp4;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5)->tag == 0U){if(((struct Cyc_Absyn_HeapCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5)->f1)->tag == 5U){_LL1: _LL2:
 return 1;}else{goto _LL3;}}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 60
int Cyc_Tcutil_is_pointer_type(void*t){
void*_tmp6=Cyc_Tcutil_compress(t);void*_tmp7=_tmp6;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp7)->tag == 3U){_LL1: _LL2:
 return 1;}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 67
int Cyc_Tcutil_is_char_type(void*t){
void*_tmp8=Cyc_Tcutil_compress(t);void*_tmp9=_tmp8;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp9)->tag == 0U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp9)->f1)->tag == 1U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp9)->f1)->f2 == Cyc_Absyn_Char_sz){_LL1: _LL2:
 return 1;}else{goto _LL3;}}else{goto _LL3;}}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 74
int Cyc_Tcutil_is_any_int_type(void*t){
void*_tmpA=Cyc_Tcutil_compress(t);void*_tmpB=_tmpA;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpB)->tag == 0U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpB)->f1)->tag == 1U){_LL1: _LL2:
 return 1;}else{goto _LL3;}}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 81
int Cyc_Tcutil_is_any_float_type(void*t){
void*_tmpC=Cyc_Tcutil_compress(t);void*_tmpD=_tmpC;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpD)->tag == 0U){if(((struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpD)->f1)->tag == 2U){_LL1: _LL2:
 return 1;}else{goto _LL3;}}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 88
int Cyc_Tcutil_is_integral_type(void*t){
void*_tmpE=Cyc_Tcutil_compress(t);void*_tmpF=_tmpE;void*_tmp11;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpF)->tag == 0U){_LL1: _tmp11=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpF)->f1;_LL2: {
# 91
void*_tmp10=_tmp11;switch(*((int*)_tmp10)){case 1U: _LL6: _LL7:
 goto _LL9;case 4U: _LL8: _LL9:
 goto _LLB;case 15U: _LLA: _LLB:
 goto _LLD;case 16U: _LLC: _LLD:
 return 1;default: _LLE: _LLF:
 return 0;}_LL5:;}}else{_LL3: _LL4:
# 98
 return 0;}_LL0:;}
# 101
int Cyc_Tcutil_is_signed_type(void*t){
void*_tmp12=Cyc_Tcutil_compress(t);void*_tmp13=_tmp12;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp13)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp13)->f1)){case 1U: if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp13)->f1)->f1 == Cyc_Absyn_Signed){_LL1: _LL2:
 return 1;}else{goto _LL5;}case 2U: _LL3: _LL4:
 return 1;default: goto _LL5;}else{_LL5: _LL6:
 return 0;}_LL0:;}
# 108
int Cyc_Tcutil_is_arithmetic_type(void*t){
return Cyc_Tcutil_is_integral_type(t) || Cyc_Tcutil_is_any_float_type(t);}
# 111
int Cyc_Tcutil_is_strict_arithmetic_type(void*t){
return Cyc_Tcutil_is_any_int_type(t) || Cyc_Tcutil_is_any_float_type(t);}
# 114
int Cyc_Tcutil_is_function_type(void*t){
void*_tmp14=Cyc_Tcutil_compress(t);void*_tmp15=_tmp14;if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp15)->tag == 5U){_LL1: _LL2:
 return 1;}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 121
int Cyc_Tcutil_is_boxed(void*t){
return(int)(Cyc_Tcutil_type_kind(t))->kind == (int)Cyc_Absyn_BoxKind;}
# 129
int Cyc_Tcutil_is_integral(struct Cyc_Absyn_Exp*e){
void*_tmp16=Cyc_Tcutil_compress((void*)_check_null(e->topt));void*_tmp17=_tmp16;void*_tmp19;switch(*((int*)_tmp17)){case 0U: _LL1: _tmp19=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp17)->f1;_LL2: {
# 132
void*_tmp18=_tmp19;switch(*((int*)_tmp18)){case 1U: _LL8: _LL9:
 goto _LLB;case 15U: _LLA: _LLB:
 goto _LLD;case 16U: _LLC: _LLD:
 goto _LLF;case 4U: _LLE: _LLF:
 return 1;default: _LL10: _LL11:
 return 0;}_LL7:;}case 1U: _LL3: _LL4:
# 139
 return Cyc_Tcutil_unify((void*)_check_null(e->topt),Cyc_Absyn_sint_type);default: _LL5: _LL6:
 return 0;}_LL0:;}
# 145
int Cyc_Tcutil_is_zeroterm_pointer_type(void*t){
void*_tmp1A=Cyc_Tcutil_compress(t);void*_tmp1B=_tmp1A;void*_tmp1C;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1B)->tag == 3U){_LL1: _tmp1C=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1B)->f1).ptr_atts).zero_term;_LL2:
# 148
 return Cyc_Tcutil_force_type2bool(0,_tmp1C);}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 154
int Cyc_Tcutil_is_nullable_pointer_type(void*t){
void*_tmp1D=Cyc_Tcutil_compress(t);void*_tmp1E=_tmp1D;void*_tmp1F;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1E)->tag == 3U){_LL1: _tmp1F=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1E)->f1).ptr_atts).nullable;_LL2:
# 157
 return Cyc_Tcutil_force_type2bool(0,_tmp1F);}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 163
int Cyc_Tcutil_is_dyneither_ptr(void*t){
void*_tmp20=Cyc_Tcutil_compress(t);void*_tmp21=_tmp20;void*_tmp22;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp21)->tag == 3U){_LL1: _tmp22=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp21)->f1).ptr_atts).bounds;_LL2:
# 166
 return Cyc_Tcutil_unify(Cyc_Absyn_fat_bound_type,_tmp22);}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 173
int Cyc_Tcutil_is_tagged_pointer_type_elt(void*t,void**elt_type_dest){
void*_tmp23=Cyc_Tcutil_compress(t);void*_tmp24=_tmp23;void*_tmp26;void*_tmp25;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp24)->tag == 3U){_LL1: _tmp26=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp24)->f1).elt_type;_tmp25=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp24)->f1).ptr_atts).bounds;_LL2:
# 176
 if(Cyc_Tcutil_unify(_tmp25,Cyc_Absyn_fat_bound_type)){
*elt_type_dest=_tmp26;
return 1;}else{
# 180
return 0;}}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 187
int Cyc_Tcutil_is_zero_pointer_type_elt(void*t,void**elt_type_dest){
void*_tmp27=Cyc_Tcutil_compress(t);void*_tmp28=_tmp27;void*_tmp2A;void*_tmp29;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp28)->tag == 3U){_LL1: _tmp2A=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp28)->f1).elt_type;_tmp29=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp28)->f1).ptr_atts).zero_term;_LL2:
# 190
*elt_type_dest=_tmp2A;
return Cyc_Absyn_type2bool(0,_tmp29);}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 199
int Cyc_Tcutil_is_zero_ptr_type(void*t,void**ptr_type,int*is_dyneither,void**elt_type){
# 201
void*_tmp2B=Cyc_Tcutil_compress(t);void*_tmp2C=_tmp2B;void*_tmp35;struct Cyc_Absyn_Tqual _tmp34;struct Cyc_Absyn_Exp*_tmp33;void*_tmp32;void*_tmp31;void*_tmp30;void*_tmp2F;switch(*((int*)_tmp2C)){case 3U: _LL1: _tmp31=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2C)->f1).elt_type;_tmp30=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2C)->f1).ptr_atts).bounds;_tmp2F=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2C)->f1).ptr_atts).zero_term;_LL2:
# 203
 if(Cyc_Absyn_type2bool(0,_tmp2F)){
*ptr_type=t;
*elt_type=_tmp31;
{void*_tmp2D=Cyc_Tcutil_compress(_tmp30);void*_tmp2E=_tmp2D;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2E)->tag == 0U){if(((struct Cyc_Absyn_FatCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2E)->f1)->tag == 14U){_LL8: _LL9:
*is_dyneither=1;goto _LL7;}else{goto _LLA;}}else{_LLA: _LLB:
*is_dyneither=0;goto _LL7;}_LL7:;}
# 210
return 1;}else{
return 0;}case 4U: _LL3: _tmp35=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp2C)->f1).elt_type;_tmp34=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp2C)->f1).tq;_tmp33=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp2C)->f1).num_elts;_tmp32=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp2C)->f1).zero_term;_LL4:
# 213
 if(Cyc_Absyn_type2bool(0,_tmp32)){
*elt_type=_tmp35;
*is_dyneither=0;
({void*_tmpA11=Cyc_Tcutil_promote_array(t,Cyc_Absyn_heap_rgn_type,0);*ptr_type=_tmpA11;});
return 1;}else{
return 0;}default: _LL5: _LL6:
 return 0;}_LL0:;}
# 226
int Cyc_Tcutil_is_tagged_pointer_type(void*t){
void*ignore=Cyc_Absyn_void_type;
return Cyc_Tcutil_is_tagged_pointer_type_elt(t,& ignore);}
# 232
int Cyc_Tcutil_is_bound_one(void*b){
struct Cyc_Absyn_Exp*_tmp36=({void*_tmpA12=Cyc_Absyn_bounds_one();Cyc_Tcutil_get_bounds_exp(_tmpA12,b);});
if(_tmp36 == 0)return 0;{
struct Cyc_Absyn_Exp*_tmp37=_tmp36;
struct _tuple13 _tmp38=Cyc_Evexp_eval_const_uint_exp(_tmp37);struct _tuple13 _tmp39=_tmp38;unsigned int _tmp3B;int _tmp3A;_LL1: _tmp3B=_tmp39.f1;_tmp3A=_tmp39.f2;_LL2:;
return _tmp3A  && _tmp3B == (unsigned int)1;};}
# 241
int Cyc_Tcutil_is_bits_only_type(void*t){
void*_tmp3C=Cyc_Tcutil_compress(t);void*_tmp3D=_tmp3C;struct Cyc_List_List*_tmp49;struct Cyc_List_List*_tmp48;void*_tmp47;void*_tmp46;void*_tmp45;struct Cyc_List_List*_tmp44;switch(*((int*)_tmp3D)){case 0U: _LL1: _tmp45=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3D)->f1;_tmp44=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3D)->f2;_LL2: {
# 244
void*_tmp3E=_tmp45;struct Cyc_Absyn_Aggrdecl*_tmp43;switch(*((int*)_tmp3E)){case 0U: _LLC: _LLD:
 goto _LLF;case 1U: _LLE: _LLF:
 goto _LL11;case 2U: _LL10: _LL11:
 return 1;case 15U: _LL12: _LL13:
 goto _LL15;case 16U: _LL14: _LL15:
 return 0;case 20U: if(((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_tmp3E)->f1).UnknownAggr).tag == 1){_LL16: _LL17:
 return 0;}else{_LL18: _tmp43=*((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_tmp3E)->f1).KnownAggr).val;_LL19:
# 252
 if(_tmp43->impl == 0)
return 0;{
int okay=1;
{struct Cyc_List_List*fs=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(_tmp43->impl))->fields;for(0;fs != 0;fs=fs->tl){
if(!Cyc_Tcutil_is_bits_only_type(((struct Cyc_Absyn_Aggrfield*)fs->hd)->type)){okay=0;break;}}}
if(okay)return 1;{
struct _RegionHandle _tmp3F=_new_region("rgn");struct _RegionHandle*rgn=& _tmp3F;_push_region(rgn);
{struct Cyc_List_List*_tmp40=((struct Cyc_List_List*(*)(struct _RegionHandle*r1,struct _RegionHandle*r2,struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_rzip)(rgn,rgn,_tmp43->tvs,_tmp44);
{struct Cyc_List_List*fs=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(_tmp43->impl))->fields;for(0;fs != 0;fs=fs->tl){
if(!Cyc_Tcutil_is_bits_only_type(Cyc_Tcutil_rsubstitute(rgn,_tmp40,((struct Cyc_Absyn_Aggrfield*)fs->hd)->type))){int _tmp41=0;_npop_handler(0U);return _tmp41;}}}{
int _tmp42=1;_npop_handler(0U);return _tmp42;};}
# 259
;_pop_region(rgn);};};}default: _LL1A: _LL1B:
# 263
 return 0;}_LLB:;}case 4U: _LL3: _tmp47=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp3D)->f1).elt_type;_tmp46=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp3D)->f1).zero_term;_LL4:
# 268
 return !Cyc_Absyn_type2bool(0,_tmp46) && Cyc_Tcutil_is_bits_only_type(_tmp47);case 6U: _LL5: _tmp48=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp3D)->f1;_LL6:
# 270
 for(0;_tmp48 != 0;_tmp48=_tmp48->tl){
if(!Cyc_Tcutil_is_bits_only_type((*((struct _tuple12*)_tmp48->hd)).f2))return 0;}
return 1;case 7U: _LL7: _tmp49=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp3D)->f2;_LL8:
# 274
 for(0;_tmp49 != 0;_tmp49=_tmp49->tl){
if(!Cyc_Tcutil_is_bits_only_type(((struct Cyc_Absyn_Aggrfield*)_tmp49->hd)->type))return 0;}
return 1;default: _LL9: _LLA:
 return 0;}_LL0:;}
# 283
int Cyc_Tcutil_is_noreturn_fn_type(void*t){
{void*_tmp4A=Cyc_Tcutil_compress(t);void*_tmp4B=_tmp4A;struct Cyc_List_List*_tmp4F;void*_tmp4E;switch(*((int*)_tmp4B)){case 3U: _LL1: _tmp4E=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4B)->f1).elt_type;_LL2:
 return Cyc_Tcutil_is_noreturn_fn_type(_tmp4E);case 5U: _LL3: _tmp4F=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp4B)->f1).attributes;_LL4:
# 287
 for(0;_tmp4F != 0;_tmp4F=_tmp4F->tl){
void*_tmp4C=(void*)_tmp4F->hd;void*_tmp4D=_tmp4C;if(((struct Cyc_Absyn_Noreturn_att_Absyn_Attribute_struct*)_tmp4D)->tag == 4U){_LL8: _LL9:
 return 1;}else{_LLA: _LLB:
 continue;}_LL7:;}
# 292
goto _LL0;default: _LL5: _LL6:
 goto _LL0;}_LL0:;}
# 295
return 0;}char Cyc_Tcutil_Unify[6U]="Unify";struct Cyc_Tcutil_Unify_exn_struct{char*tag;};
# 299
struct Cyc_Tcutil_Unify_exn_struct Cyc_Tcutil_Unify_val={Cyc_Tcutil_Unify};
# 301
void Cyc_Tcutil_unify_it(void*t1,void*t2);
# 305
int Cyc_Tcutil_warn_region_coerce=0;
# 308
static void*Cyc_Tcutil_t1_failure=0;
static int Cyc_Tcutil_tq1_const=0;
static void*Cyc_Tcutil_t2_failure=0;
static int Cyc_Tcutil_tq2_const=0;
# 313
struct _dyneither_ptr Cyc_Tcutil_failure_reason={(void*)0,(void*)0,(void*)(0 + 0)};
# 317
void Cyc_Tcutil_explain_failure(){
if(Cyc_Position_num_errors >= Cyc_Position_max_errors)return;
Cyc_fflush(Cyc_stderr);
# 322
if(({struct _dyneither_ptr _tmpA13=({const char*_tmp50="(qualifiers don't match)";_tag_dyneither(_tmp50,sizeof(char),25U);});Cyc_strcmp(_tmpA13,(struct _dyneither_ptr)Cyc_Tcutil_failure_reason);})== 0){
({struct Cyc_String_pa_PrintArg_struct _tmp53=({struct Cyc_String_pa_PrintArg_struct _tmp97D;_tmp97D.tag=0U,_tmp97D.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Tcutil_failure_reason);_tmp97D;});void*_tmp51[1U];_tmp51[0]=& _tmp53;({struct Cyc___cycFILE*_tmpA15=Cyc_stderr;struct _dyneither_ptr _tmpA14=({const char*_tmp52="  %s\n";_tag_dyneither(_tmp52,sizeof(char),6U);});Cyc_fprintf(_tmpA15,_tmpA14,_tag_dyneither(_tmp51,sizeof(void*),1U));});});
return;}
# 327
if(({struct _dyneither_ptr _tmpA16=({const char*_tmp54="(function effects do not match)";_tag_dyneither(_tmp54,sizeof(char),32U);});Cyc_strcmp(_tmpA16,(struct _dyneither_ptr)Cyc_Tcutil_failure_reason);})== 0){
struct Cyc_Absynpp_Params _tmp55=Cyc_Absynpp_tc_params_r;
_tmp55.print_all_effects=1;
Cyc_Absynpp_set_params(& _tmp55);}{
# 332
void*_tmp56=Cyc_Tcutil_t1_failure;
void*_tmp57=Cyc_Tcutil_t2_failure;
struct _dyneither_ptr s1=(unsigned int)_tmp56?Cyc_Absynpp_typ2string(_tmp56):({const char*_tmp72="<?>";_tag_dyneither(_tmp72,sizeof(char),4U);});
struct _dyneither_ptr s2=(unsigned int)_tmp57?Cyc_Absynpp_typ2string(_tmp57):({const char*_tmp71="<?>";_tag_dyneither(_tmp71,sizeof(char),4U);});
int pos=2;
({struct Cyc_String_pa_PrintArg_struct _tmp5A=({struct Cyc_String_pa_PrintArg_struct _tmp97E;_tmp97E.tag=0U,_tmp97E.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)s1);_tmp97E;});void*_tmp58[1U];_tmp58[0]=& _tmp5A;({struct Cyc___cycFILE*_tmpA18=Cyc_stderr;struct _dyneither_ptr _tmpA17=({const char*_tmp59="  %s";_tag_dyneither(_tmp59,sizeof(char),5U);});Cyc_fprintf(_tmpA18,_tmpA17,_tag_dyneither(_tmp58,sizeof(void*),1U));});});
pos +=_get_dyneither_size(s1,sizeof(char));
if(pos + 5 >= 80){
({void*_tmp5B=0U;({struct Cyc___cycFILE*_tmpA1A=Cyc_stderr;struct _dyneither_ptr _tmpA19=({const char*_tmp5C="\n\t";_tag_dyneither(_tmp5C,sizeof(char),3U);});Cyc_fprintf(_tmpA1A,_tmpA19,_tag_dyneither(_tmp5B,sizeof(void*),0U));});});
pos=8;}else{
# 343
({void*_tmp5D=0U;({struct Cyc___cycFILE*_tmpA1C=Cyc_stderr;struct _dyneither_ptr _tmpA1B=({const char*_tmp5E=" ";_tag_dyneither(_tmp5E,sizeof(char),2U);});Cyc_fprintf(_tmpA1C,_tmpA1B,_tag_dyneither(_tmp5D,sizeof(void*),0U));});});
++ pos;}
# 346
({void*_tmp5F=0U;({struct Cyc___cycFILE*_tmpA1E=Cyc_stderr;struct _dyneither_ptr _tmpA1D=({const char*_tmp60="and ";_tag_dyneither(_tmp60,sizeof(char),5U);});Cyc_fprintf(_tmpA1E,_tmpA1D,_tag_dyneither(_tmp5F,sizeof(void*),0U));});});
pos +=4;
if((unsigned int)pos + _get_dyneither_size(s2,sizeof(char))>= (unsigned int)80){
({void*_tmp61=0U;({struct Cyc___cycFILE*_tmpA20=Cyc_stderr;struct _dyneither_ptr _tmpA1F=({const char*_tmp62="\n\t";_tag_dyneither(_tmp62,sizeof(char),3U);});Cyc_fprintf(_tmpA20,_tmpA1F,_tag_dyneither(_tmp61,sizeof(void*),0U));});});
pos=8;}
# 352
({struct Cyc_String_pa_PrintArg_struct _tmp65=({struct Cyc_String_pa_PrintArg_struct _tmp97F;_tmp97F.tag=0U,_tmp97F.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)s2);_tmp97F;});void*_tmp63[1U];_tmp63[0]=& _tmp65;({struct Cyc___cycFILE*_tmpA22=Cyc_stderr;struct _dyneither_ptr _tmpA21=({const char*_tmp64="%s ";_tag_dyneither(_tmp64,sizeof(char),4U);});Cyc_fprintf(_tmpA22,_tmpA21,_tag_dyneither(_tmp63,sizeof(void*),1U));});});
pos +=_get_dyneither_size(s2,sizeof(char))+ (unsigned int)1;
if(pos + 17 >= 80){
({void*_tmp66=0U;({struct Cyc___cycFILE*_tmpA24=Cyc_stderr;struct _dyneither_ptr _tmpA23=({const char*_tmp67="\n\t";_tag_dyneither(_tmp67,sizeof(char),3U);});Cyc_fprintf(_tmpA24,_tmpA23,_tag_dyneither(_tmp66,sizeof(void*),0U));});});
pos=8;}
# 358
({void*_tmp68=0U;({struct Cyc___cycFILE*_tmpA26=Cyc_stderr;struct _dyneither_ptr _tmpA25=({const char*_tmp69="are not compatible. ";_tag_dyneither(_tmp69,sizeof(char),21U);});Cyc_fprintf(_tmpA26,_tmpA25,_tag_dyneither(_tmp68,sizeof(void*),0U));});});
pos +=17;
if(({char*_tmpA27=(char*)Cyc_Tcutil_failure_reason.curr;_tmpA27 != (char*)(_tag_dyneither(0,0,0)).curr;})){
if(({unsigned long _tmpA28=(unsigned long)pos;_tmpA28 + Cyc_strlen((struct _dyneither_ptr)Cyc_Tcutil_failure_reason);})>= (unsigned long)80)
({void*_tmp6A=0U;({struct Cyc___cycFILE*_tmpA2A=Cyc_stderr;struct _dyneither_ptr _tmpA29=({const char*_tmp6B="\n\t";_tag_dyneither(_tmp6B,sizeof(char),3U);});Cyc_fprintf(_tmpA2A,_tmpA29,_tag_dyneither(_tmp6A,sizeof(void*),0U));});});
# 364
({struct Cyc_String_pa_PrintArg_struct _tmp6E=({struct Cyc_String_pa_PrintArg_struct _tmp980;_tmp980.tag=0U,_tmp980.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Tcutil_failure_reason);_tmp980;});void*_tmp6C[1U];_tmp6C[0]=& _tmp6E;({struct Cyc___cycFILE*_tmpA2C=Cyc_stderr;struct _dyneither_ptr _tmpA2B=({const char*_tmp6D="%s";_tag_dyneither(_tmp6D,sizeof(char),3U);});Cyc_fprintf(_tmpA2C,_tmpA2B,_tag_dyneither(_tmp6C,sizeof(void*),1U));});});}
# 366
({void*_tmp6F=0U;({struct Cyc___cycFILE*_tmpA2E=Cyc_stderr;struct _dyneither_ptr _tmpA2D=({const char*_tmp70="\n";_tag_dyneither(_tmp70,sizeof(char),2U);});Cyc_fprintf(_tmpA2E,_tmpA2D,_tag_dyneither(_tmp6F,sizeof(void*),0U));});});
Cyc_fflush(Cyc_stderr);};}
# 370
void Cyc_Tcutil_terr(unsigned int loc,struct _dyneither_ptr fmt,struct _dyneither_ptr ap){
# 372
Cyc_Warn_verr(loc,fmt,ap);}
# 374
void*Cyc_Tcutil_impos(struct _dyneither_ptr fmt,struct _dyneither_ptr ap){
# 377
Cyc_Warn_vimpos(fmt,ap);}
# 379
void Cyc_Tcutil_warn(unsigned int sg,struct _dyneither_ptr fmt,struct _dyneither_ptr ap){
# 381
Cyc_Warn_vwarn(sg,fmt,ap);}
# 384
int Cyc_Tcutil_is_numeric(struct Cyc_Absyn_Exp*e){
if(Cyc_Tcutil_is_integral(e))
return 1;{
void*_tmp73=Cyc_Tcutil_compress((void*)_check_null(e->topt));void*_tmp74=_tmp73;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp74)->tag == 0U){if(((struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp74)->f1)->tag == 2U){_LL1: _LL2:
 return 1;}else{goto _LL3;}}else{_LL3: _LL4:
 return 0;}_LL0:;};}
# 394
static int Cyc_Tcutil_fast_tvar_cmp(struct Cyc_Absyn_Tvar*tv1,struct Cyc_Absyn_Tvar*tv2){
return tv1->identity - tv2->identity;}
# 399
void*Cyc_Tcutil_compress(void*t){
void*_tmp75=t;void*_tmp7E;struct Cyc_Absyn_Exp*_tmp7D;struct Cyc_Absyn_Exp*_tmp7C;void**_tmp7B;void**_tmp7A;switch(*((int*)_tmp75)){case 1U: if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp75)->f2 == 0){_LL1: _LL2:
 goto _LL4;}else{_LL7: _tmp7A=(void**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp75)->f2;_LL8: {
# 410
void*ta=(void*)_check_null(*_tmp7A);
void*t2=Cyc_Tcutil_compress(ta);
if(t2 != ta)
*_tmp7A=t2;
return t2;}}case 8U: if(((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp75)->f4 == 0){_LL3: _LL4:
# 402
 return t;}else{_LL5: _tmp7B=(void**)&((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp75)->f4;_LL6: {
# 404
void*ta=(void*)_check_null(*_tmp7B);
void*t2=Cyc_Tcutil_compress(ta);
if(t2 != ta)
*_tmp7B=t2;
return t2;}}case 9U: _LL9: _tmp7C=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp75)->f1;_LLA:
# 416
 Cyc_Evexp_eval_const_uint_exp(_tmp7C);{
void*_tmp76=_tmp7C->r;void*_tmp77=_tmp76;void*_tmp78;if(((struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*)_tmp77)->tag == 39U){_LL12: _tmp78=(void*)((struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*)_tmp77)->f1;_LL13:
 return Cyc_Tcutil_compress(_tmp78);}else{_LL14: _LL15:
 return t;}_LL11:;};case 11U: _LLB: _tmp7D=((struct Cyc_Absyn_TypeofType_Absyn_Type_struct*)_tmp75)->f1;_LLC: {
# 422
void*_tmp79=_tmp7D->topt;
if(_tmp79 != 0)return _tmp79;else{
return t;}}case 10U: if(((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp75)->f2 != 0){_LLD: _tmp7E=*((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp75)->f2;_LLE:
# 426
 return Cyc_Tcutil_compress(_tmp7E);}else{goto _LLF;}default: _LLF: _LL10:
 return t;}_LL0:;}
# 435
void*Cyc_Tcutil_copy_type(void*t);
static struct Cyc_List_List*Cyc_Tcutil_copy_types(struct Cyc_List_List*ts){
return((struct Cyc_List_List*(*)(void*(*f)(void*),struct Cyc_List_List*x))Cyc_List_map)(Cyc_Tcutil_copy_type,ts);}
# 439
static void*Cyc_Tcutil_copy_kindbound(void*kb){
void*_tmp7F=Cyc_Absyn_compress_kb(kb);void*_tmp80=_tmp7F;struct Cyc_Absyn_Kind*_tmp83;switch(*((int*)_tmp80)){case 1U: _LL1: _LL2:
 return(void*)({struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*_tmp81=_cycalloc(sizeof(*_tmp81));_tmp81->tag=1U,_tmp81->f1=0;_tmp81;});case 2U: _LL3: _tmp83=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp80)->f2;_LL4:
 return(void*)({struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_tmp82=_cycalloc(sizeof(*_tmp82));_tmp82->tag=2U,_tmp82->f1=0,_tmp82->f2=_tmp83;_tmp82;});default: _LL5: _LL6:
 return kb;}_LL0:;}
# 446
static struct Cyc_Absyn_Tvar*Cyc_Tcutil_copy_tvar(struct Cyc_Absyn_Tvar*tv){
# 448
return({struct Cyc_Absyn_Tvar*_tmp84=_cycalloc(sizeof(*_tmp84));_tmp84->name=tv->name,_tmp84->identity=- 1,({void*_tmpA2F=Cyc_Tcutil_copy_kindbound(tv->kind);_tmp84->kind=_tmpA2F;});_tmp84;});}
# 450
static struct _tuple10*Cyc_Tcutil_copy_arg(struct _tuple10*arg){
# 452
struct _tuple10*_tmp85=arg;struct _dyneither_ptr*_tmp89;struct Cyc_Absyn_Tqual _tmp88;void*_tmp87;_LL1: _tmp89=_tmp85->f1;_tmp88=_tmp85->f2;_tmp87=_tmp85->f3;_LL2:;
return({struct _tuple10*_tmp86=_cycalloc(sizeof(*_tmp86));_tmp86->f1=_tmp89,_tmp86->f2=_tmp88,({void*_tmpA30=Cyc_Tcutil_copy_type(_tmp87);_tmp86->f3=_tmpA30;});_tmp86;});}
# 455
static struct _tuple12*Cyc_Tcutil_copy_tqt(struct _tuple12*arg){
struct _tuple12*_tmp8A=arg;struct Cyc_Absyn_Tqual _tmp8D;void*_tmp8C;_LL1: _tmp8D=_tmp8A->f1;_tmp8C=_tmp8A->f2;_LL2:;
return({struct _tuple12*_tmp8B=_cycalloc(sizeof(*_tmp8B));_tmp8B->f1=_tmp8D,({void*_tmpA31=Cyc_Tcutil_copy_type(_tmp8C);_tmp8B->f2=_tmpA31;});_tmp8B;});}
# 459
struct Cyc_Absyn_Exp*Cyc_Tcutil_deep_copy_exp_opt(int preserve_types,struct Cyc_Absyn_Exp*);
# 461
static struct Cyc_Absyn_Aggrfield*Cyc_Tcutil_copy_field(struct Cyc_Absyn_Aggrfield*f){
return({struct Cyc_Absyn_Aggrfield*_tmp8E=_cycalloc(sizeof(*_tmp8E));_tmp8E->name=f->name,_tmp8E->tq=f->tq,({void*_tmpA33=Cyc_Tcutil_copy_type(f->type);_tmp8E->type=_tmpA33;}),_tmp8E->width=f->width,_tmp8E->attributes=f->attributes,({
struct Cyc_Absyn_Exp*_tmpA32=Cyc_Tcutil_deep_copy_exp_opt(1,f->requires_clause);_tmp8E->requires_clause=_tmpA32;});_tmp8E;});}
# 465
static struct _tuple0*Cyc_Tcutil_copy_rgncmp(struct _tuple0*x){
struct _tuple0*_tmp8F=x;void*_tmp92;void*_tmp91;_LL1: _tmp92=_tmp8F->f1;_tmp91=_tmp8F->f2;_LL2:;
return({struct _tuple0*_tmp90=_cycalloc(sizeof(*_tmp90));({void*_tmpA35=Cyc_Tcutil_copy_type(_tmp92);_tmp90->f1=_tmpA35;}),({void*_tmpA34=Cyc_Tcutil_copy_type(_tmp91);_tmp90->f2=_tmpA34;});_tmp90;});}
# 469
static struct Cyc_Absyn_Enumfield*Cyc_Tcutil_copy_enumfield(struct Cyc_Absyn_Enumfield*f){
return({struct Cyc_Absyn_Enumfield*_tmp93=_cycalloc(sizeof(*_tmp93));_tmp93->name=f->name,_tmp93->tag=f->tag,_tmp93->loc=f->loc;_tmp93;});}
# 472
static void*Cyc_Tcutil_tvar2type(struct Cyc_Absyn_Tvar*t){
return Cyc_Absyn_var_type(Cyc_Tcutil_copy_tvar(t));}
# 476
void*Cyc_Tcutil_copy_type(void*t){
void*_tmp94=Cyc_Tcutil_compress(t);void*_tmp95=_tmp94;struct Cyc_Absyn_Datatypedecl*_tmpD9;struct Cyc_Absyn_Enumdecl*_tmpD8;struct Cyc_Absyn_Aggrdecl*_tmpD7;struct _tuple2*_tmpD6;struct Cyc_List_List*_tmpD5;struct Cyc_Absyn_Typedefdecl*_tmpD4;struct Cyc_Absyn_Exp*_tmpD3;struct Cyc_Absyn_Exp*_tmpD2;enum Cyc_Absyn_AggrKind _tmpD1;struct Cyc_List_List*_tmpD0;struct Cyc_List_List*_tmpCF;struct Cyc_List_List*_tmpCE;void*_tmpCD;struct Cyc_Absyn_Tqual _tmpCC;void*_tmpCB;struct Cyc_List_List*_tmpCA;int _tmpC9;struct Cyc_Absyn_VarargInfo*_tmpC8;struct Cyc_List_List*_tmpC7;struct Cyc_List_List*_tmpC6;struct Cyc_Absyn_Exp*_tmpC5;struct Cyc_List_List*_tmpC4;struct Cyc_Absyn_Exp*_tmpC3;struct Cyc_List_List*_tmpC2;void*_tmpC1;struct Cyc_Absyn_Tqual _tmpC0;struct Cyc_Absyn_Exp*_tmpBF;void*_tmpBE;unsigned int _tmpBD;void*_tmpBC;struct Cyc_Absyn_Tqual _tmpBB;void*_tmpBA;void*_tmpB9;void*_tmpB8;void*_tmpB7;struct Cyc_Absyn_PtrLoc*_tmpB6;struct Cyc_Absyn_Tvar*_tmpB5;void*_tmpB4;struct Cyc_List_List*_tmpB3;void*_tmpB2;switch(*((int*)_tmp95)){case 0U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp95)->f2 == 0){_LL1: _tmpB2=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp95)->f1;_LL2:
 return t;}else{_LL3: _tmpB4=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp95)->f1;_tmpB3=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp95)->f2;_LL4:
 return(void*)({struct Cyc_Absyn_AppType_Absyn_Type_struct*_tmp96=_cycalloc(sizeof(*_tmp96));_tmp96->tag=0U,_tmp96->f1=_tmpB4,({struct Cyc_List_List*_tmpA36=Cyc_Tcutil_copy_types(_tmpB3);_tmp96->f2=_tmpA36;});_tmp96;});}case 1U: _LL5: _LL6:
 return t;case 2U: _LL7: _tmpB5=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp95)->f1;_LL8:
 return Cyc_Absyn_var_type(Cyc_Tcutil_copy_tvar(_tmpB5));case 3U: _LL9: _tmpBC=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp95)->f1).elt_type;_tmpBB=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp95)->f1).elt_tq;_tmpBA=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp95)->f1).ptr_atts).rgn;_tmpB9=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp95)->f1).ptr_atts).nullable;_tmpB8=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp95)->f1).ptr_atts).bounds;_tmpB7=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp95)->f1).ptr_atts).zero_term;_tmpB6=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp95)->f1).ptr_atts).ptrloc;_LLA: {
# 483
void*_tmp97=Cyc_Tcutil_copy_type(_tmpBC);
void*_tmp98=Cyc_Tcutil_copy_type(_tmpBA);
void*_tmp99=Cyc_Tcutil_copy_type(_tmpB9);
struct Cyc_Absyn_Tqual _tmp9A=_tmpBB;
# 488
void*_tmp9B=Cyc_Tcutil_copy_type(_tmpB8);
void*_tmp9C=Cyc_Tcutil_copy_type(_tmpB7);
return(void*)({struct Cyc_Absyn_PointerType_Absyn_Type_struct*_tmp9D=_cycalloc(sizeof(*_tmp9D));_tmp9D->tag=3U,(_tmp9D->f1).elt_type=_tmp97,(_tmp9D->f1).elt_tq=_tmp9A,((_tmp9D->f1).ptr_atts).rgn=_tmp98,((_tmp9D->f1).ptr_atts).nullable=_tmp99,((_tmp9D->f1).ptr_atts).bounds=_tmp9B,((_tmp9D->f1).ptr_atts).zero_term=_tmp9C,((_tmp9D->f1).ptr_atts).ptrloc=_tmpB6;_tmp9D;});}case 4U: _LLB: _tmpC1=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp95)->f1).elt_type;_tmpC0=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp95)->f1).tq;_tmpBF=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp95)->f1).num_elts;_tmpBE=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp95)->f1).zero_term;_tmpBD=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp95)->f1).zt_loc;_LLC: {
# 492
struct Cyc_Absyn_Exp*eopt2=Cyc_Tcutil_deep_copy_exp_opt(1,_tmpBF);
return(void*)({struct Cyc_Absyn_ArrayType_Absyn_Type_struct*_tmp9E=_cycalloc(sizeof(*_tmp9E));_tmp9E->tag=4U,({void*_tmpA38=Cyc_Tcutil_copy_type(_tmpC1);(_tmp9E->f1).elt_type=_tmpA38;}),(_tmp9E->f1).tq=_tmpC0,(_tmp9E->f1).num_elts=eopt2,({
void*_tmpA37=Cyc_Tcutil_copy_type(_tmpBE);(_tmp9E->f1).zero_term=_tmpA37;}),(_tmp9E->f1).zt_loc=_tmpBD;_tmp9E;});}case 5U: _LLD: _tmpCE=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp95)->f1).tvars;_tmpCD=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp95)->f1).effect;_tmpCC=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp95)->f1).ret_tqual;_tmpCB=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp95)->f1).ret_type;_tmpCA=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp95)->f1).args;_tmpC9=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp95)->f1).c_varargs;_tmpC8=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp95)->f1).cyc_varargs;_tmpC7=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp95)->f1).rgn_po;_tmpC6=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp95)->f1).attributes;_tmpC5=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp95)->f1).requires_clause;_tmpC4=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp95)->f1).requires_relns;_tmpC3=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp95)->f1).ensures_clause;_tmpC2=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp95)->f1).ensures_relns;_LLE: {
# 496
struct Cyc_List_List*_tmp9F=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x))Cyc_List_map)(Cyc_Tcutil_copy_tvar,_tmpCE);
void*effopt2=_tmpCD == 0?0: Cyc_Tcutil_copy_type(_tmpCD);
void*_tmpA0=Cyc_Tcutil_copy_type(_tmpCB);
struct Cyc_List_List*_tmpA1=((struct Cyc_List_List*(*)(struct _tuple10*(*f)(struct _tuple10*),struct Cyc_List_List*x))Cyc_List_map)(Cyc_Tcutil_copy_arg,_tmpCA);
int _tmpA2=_tmpC9;
struct Cyc_Absyn_VarargInfo*cyc_varargs2=0;
if(_tmpC8 != 0){
struct Cyc_Absyn_VarargInfo*cv=_tmpC8;
cyc_varargs2=({struct Cyc_Absyn_VarargInfo*_tmpA3=_cycalloc(sizeof(*_tmpA3));_tmpA3->name=cv->name,_tmpA3->tq=cv->tq,({void*_tmpA39=Cyc_Tcutil_copy_type(cv->type);_tmpA3->type=_tmpA39;}),_tmpA3->inject=cv->inject;_tmpA3;});}{
# 507
struct Cyc_List_List*_tmpA4=((struct Cyc_List_List*(*)(struct _tuple0*(*f)(struct _tuple0*),struct Cyc_List_List*x))Cyc_List_map)(Cyc_Tcutil_copy_rgncmp,_tmpC7);
struct Cyc_List_List*_tmpA5=_tmpC6;
struct Cyc_Absyn_Exp*_tmpA6=Cyc_Tcutil_deep_copy_exp_opt(1,_tmpC5);
struct Cyc_List_List*_tmpA7=Cyc_Relations_copy_relns(Cyc_Core_heap_region,_tmpC4);
struct Cyc_Absyn_Exp*_tmpA8=Cyc_Tcutil_deep_copy_exp_opt(1,_tmpC3);
struct Cyc_List_List*_tmpA9=Cyc_Relations_copy_relns(Cyc_Core_heap_region,_tmpC2);
return(void*)({struct Cyc_Absyn_FnType_Absyn_Type_struct*_tmpAA=_cycalloc(sizeof(*_tmpAA));_tmpAA->tag=5U,(_tmpAA->f1).tvars=_tmp9F,(_tmpAA->f1).effect=effopt2,(_tmpAA->f1).ret_tqual=_tmpCC,(_tmpAA->f1).ret_type=_tmpA0,(_tmpAA->f1).args=_tmpA1,(_tmpAA->f1).c_varargs=_tmpA2,(_tmpAA->f1).cyc_varargs=cyc_varargs2,(_tmpAA->f1).rgn_po=_tmpA4,(_tmpAA->f1).attributes=_tmpA5,(_tmpAA->f1).requires_clause=_tmpA6,(_tmpAA->f1).requires_relns=_tmpA7,(_tmpAA->f1).ensures_clause=_tmpA8,(_tmpAA->f1).ensures_relns=_tmpA9;_tmpAA;});};}case 6U: _LLF: _tmpCF=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp95)->f1;_LL10:
# 516
 return(void*)({struct Cyc_Absyn_TupleType_Absyn_Type_struct*_tmpAB=_cycalloc(sizeof(*_tmpAB));_tmpAB->tag=6U,({struct Cyc_List_List*_tmpA3A=((struct Cyc_List_List*(*)(struct _tuple12*(*f)(struct _tuple12*),struct Cyc_List_List*x))Cyc_List_map)(Cyc_Tcutil_copy_tqt,_tmpCF);_tmpAB->f1=_tmpA3A;});_tmpAB;});case 7U: _LL11: _tmpD1=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp95)->f1;_tmpD0=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp95)->f2;_LL12:
 return(void*)({struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*_tmpAC=_cycalloc(sizeof(*_tmpAC));_tmpAC->tag=7U,_tmpAC->f1=_tmpD1,({struct Cyc_List_List*_tmpA3B=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Aggrfield*(*f)(struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*x))Cyc_List_map)(Cyc_Tcutil_copy_field,_tmpD0);_tmpAC->f2=_tmpA3B;});_tmpAC;});case 9U: _LL13: _tmpD2=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp95)->f1;_LL14:
# 520
 return(void*)({struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_tmpAD=_cycalloc(sizeof(*_tmpAD));_tmpAD->tag=9U,_tmpAD->f1=_tmpD2;_tmpAD;});case 11U: _LL15: _tmpD3=((struct Cyc_Absyn_TypeofType_Absyn_Type_struct*)_tmp95)->f1;_LL16:
# 523
 return(void*)({struct Cyc_Absyn_TypeofType_Absyn_Type_struct*_tmpAE=_cycalloc(sizeof(*_tmpAE));_tmpAE->tag=11U,_tmpAE->f1=_tmpD3;_tmpAE;});case 8U: _LL17: _tmpD6=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp95)->f1;_tmpD5=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp95)->f2;_tmpD4=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp95)->f3;_LL18:
# 525
 return(void*)({struct Cyc_Absyn_TypedefType_Absyn_Type_struct*_tmpAF=_cycalloc(sizeof(*_tmpAF));_tmpAF->tag=8U,_tmpAF->f1=_tmpD6,({struct Cyc_List_List*_tmpA3C=Cyc_Tcutil_copy_types(_tmpD5);_tmpAF->f2=_tmpA3C;}),_tmpAF->f3=_tmpD4,_tmpAF->f4=0;_tmpAF;});default: switch(*((int*)((struct Cyc_Absyn_TypeDecl*)((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp95)->f1)->r)){case 0U: _LL19: _tmpD7=((struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct*)(((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp95)->f1)->r)->f1;_LL1A: {
# 528
struct Cyc_List_List*_tmpB0=((struct Cyc_List_List*(*)(void*(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x))Cyc_List_map)(Cyc_Tcutil_tvar2type,_tmpD7->tvs);
return({union Cyc_Absyn_AggrInfo _tmpA3D=Cyc_Absyn_UnknownAggr(_tmpD7->kind,_tmpD7->name,0);Cyc_Absyn_aggr_type(_tmpA3D,_tmpB0);});}case 1U: _LL1B: _tmpD8=((struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct*)(((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp95)->f1)->r)->f1;_LL1C:
# 531
 return Cyc_Absyn_enum_type(_tmpD8->name,0);default: _LL1D: _tmpD9=((struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct*)(((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp95)->f1)->r)->f1;_LL1E: {
# 533
struct Cyc_List_List*_tmpB1=((struct Cyc_List_List*(*)(void*(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x))Cyc_List_map)(Cyc_Tcutil_tvar2type,_tmpD9->tvs);
return({union Cyc_Absyn_DatatypeInfo _tmpA3E=Cyc_Absyn_UnknownDatatype(({struct Cyc_Absyn_UnknownDatatypeInfo _tmp981;_tmp981.name=_tmpD9->name,_tmp981.is_extensible=0;_tmp981;}));Cyc_Absyn_datatype_type(_tmpA3E,_tmpB1);});}}}_LL0:;}
# 548 "tcutil.cyc"
static void*Cyc_Tcutil_copy_designator(int preserve_types,void*d){
void*_tmpDA=d;struct _dyneither_ptr*_tmpDD;struct Cyc_Absyn_Exp*_tmpDC;if(((struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct*)_tmpDA)->tag == 0U){_LL1: _tmpDC=((struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct*)_tmpDA)->f1;_LL2:
 return(void*)({struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct*_tmpDB=_cycalloc(sizeof(*_tmpDB));_tmpDB->tag=0U,({struct Cyc_Absyn_Exp*_tmpA3F=Cyc_Tcutil_deep_copy_exp(preserve_types,_tmpDC);_tmpDB->f1=_tmpA3F;});_tmpDB;});}else{_LL3: _tmpDD=((struct Cyc_Absyn_FieldName_Absyn_Designator_struct*)_tmpDA)->f1;_LL4:
 return d;}_LL0:;}struct _tuple18{struct Cyc_List_List*f1;struct Cyc_Absyn_Exp*f2;};
# 554
static struct _tuple18*Cyc_Tcutil_copy_eds(int preserve_types,struct _tuple18*e){
# 556
return({struct _tuple18*_tmpDE=_cycalloc(sizeof(*_tmpDE));({struct Cyc_List_List*_tmpA41=((struct Cyc_List_List*(*)(void*(*f)(int,void*),int env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Tcutil_copy_designator,preserve_types,(e[0]).f1);_tmpDE->f1=_tmpA41;}),({struct Cyc_Absyn_Exp*_tmpA40=Cyc_Tcutil_deep_copy_exp(preserve_types,(e[0]).f2);_tmpDE->f2=_tmpA40;});_tmpDE;});}
# 559
struct Cyc_Absyn_Exp*Cyc_Tcutil_deep_copy_exp(int preserve_types,struct Cyc_Absyn_Exp*e){
struct Cyc_Absyn_Exp*new_e;
int _tmpDF=preserve_types;
{void*_tmpE0=e->r;void*_tmpE1=_tmpE0;int _tmp15E;struct _dyneither_ptr _tmp15D;void*_tmp15C;struct Cyc_Absyn_Exp*_tmp15B;struct _dyneither_ptr*_tmp15A;struct Cyc_Core_Opt*_tmp159;struct Cyc_List_List*_tmp158;struct Cyc_Absyn_Exp*_tmp157;struct Cyc_Absyn_Exp*_tmp156;int _tmp155;struct Cyc_Absyn_Exp*_tmp154;void**_tmp153;struct Cyc_Absyn_Exp*_tmp152;int _tmp151;int _tmp150;void*_tmp14F;struct Cyc_Absyn_Enumfield*_tmp14E;struct Cyc_Absyn_Enumdecl*_tmp14D;struct Cyc_Absyn_Enumfield*_tmp14C;struct Cyc_List_List*_tmp14B;struct Cyc_Absyn_Datatypedecl*_tmp14A;struct Cyc_Absyn_Datatypefield*_tmp149;void*_tmp148;struct Cyc_List_List*_tmp147;struct _tuple2*_tmp146;struct Cyc_List_List*_tmp145;struct Cyc_List_List*_tmp144;struct Cyc_Absyn_Aggrdecl*_tmp143;struct Cyc_Absyn_Exp*_tmp142;void*_tmp141;int _tmp140;struct Cyc_Absyn_Vardecl*_tmp13F;struct Cyc_Absyn_Exp*_tmp13E;struct Cyc_Absyn_Exp*_tmp13D;int _tmp13C;struct Cyc_List_List*_tmp13B;struct _dyneither_ptr*_tmp13A;struct Cyc_Absyn_Tqual _tmp139;void*_tmp138;struct Cyc_List_List*_tmp137;struct Cyc_List_List*_tmp136;struct Cyc_Absyn_Exp*_tmp135;struct Cyc_Absyn_Exp*_tmp134;struct Cyc_Absyn_Exp*_tmp133;struct _dyneither_ptr*_tmp132;int _tmp131;int _tmp130;struct Cyc_Absyn_Exp*_tmp12F;struct _dyneither_ptr*_tmp12E;int _tmp12D;int _tmp12C;struct Cyc_Absyn_Exp*_tmp12B;struct Cyc_Absyn_Exp*_tmp12A;void*_tmp129;struct Cyc_List_List*_tmp128;struct Cyc_Absyn_Exp*_tmp127;void*_tmp126;struct Cyc_Absyn_Exp*_tmp125;struct Cyc_Absyn_Exp*_tmp124;struct Cyc_Absyn_Exp*_tmp123;void*_tmp122;struct Cyc_Absyn_Exp*_tmp121;int _tmp120;enum Cyc_Absyn_Coercion _tmp11F;struct Cyc_Absyn_Exp*_tmp11E;struct Cyc_List_List*_tmp11D;struct Cyc_Absyn_Exp*_tmp11C;struct Cyc_Absyn_Exp*_tmp11B;int _tmp11A;struct Cyc_Absyn_Exp*_tmp119;struct Cyc_List_List*_tmp118;struct Cyc_Absyn_VarargCallInfo*_tmp117;int _tmp116;struct Cyc_Absyn_Exp*_tmp115;struct Cyc_Absyn_Exp*_tmp114;struct Cyc_Absyn_Exp*_tmp113;struct Cyc_Absyn_Exp*_tmp112;struct Cyc_Absyn_Exp*_tmp111;struct Cyc_Absyn_Exp*_tmp110;struct Cyc_Absyn_Exp*_tmp10F;struct Cyc_Absyn_Exp*_tmp10E;struct Cyc_Absyn_Exp*_tmp10D;struct Cyc_Absyn_Exp*_tmp10C;enum Cyc_Absyn_Incrementor _tmp10B;struct Cyc_Absyn_Exp*_tmp10A;struct Cyc_Core_Opt*_tmp109;struct Cyc_Absyn_Exp*_tmp108;enum Cyc_Absyn_Primop _tmp107;struct Cyc_List_List*_tmp106;struct _dyneither_ptr _tmp105;void*_tmp104;union Cyc_Absyn_Cnst _tmp103;switch(*((int*)_tmpE1)){case 0U: _LL1: _tmp103=((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_LL2:
 new_e=Cyc_Absyn_const_exp(_tmp103,e->loc);goto _LL0;case 1U: _LL3: _tmp104=(void*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_LL4:
 new_e=Cyc_Absyn_varb_exp(_tmp104,e->loc);goto _LL0;case 2U: _LL5: _tmp105=((struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_LL6:
 new_e=Cyc_Absyn_pragma_exp(_tmp105,e->loc);goto _LL0;case 3U: _LL7: _tmp107=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp106=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_LL8:
 new_e=({enum Cyc_Absyn_Primop _tmpA43=_tmp107;struct Cyc_List_List*_tmpA42=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*f)(int,struct Cyc_Absyn_Exp*),int env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Tcutil_deep_copy_exp,_tmpDF,_tmp106);Cyc_Absyn_primop_exp(_tmpA43,_tmpA42,e->loc);});goto _LL0;case 4U: _LL9: _tmp10A=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp109=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_tmp108=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmpE1)->f3;_LLA:
# 568
 new_e=({struct Cyc_Absyn_Exp*_tmpA46=Cyc_Tcutil_deep_copy_exp(_tmpDF,_tmp10A);struct Cyc_Core_Opt*_tmpA45=(unsigned int)_tmp109?({struct Cyc_Core_Opt*_tmpE2=_cycalloc(sizeof(*_tmpE2));_tmpE2->v=(void*)_tmp109->v;_tmpE2;}): 0;struct Cyc_Absyn_Exp*_tmpA44=Cyc_Tcutil_deep_copy_exp(_tmpDF,_tmp108);Cyc_Absyn_assignop_exp(_tmpA46,_tmpA45,_tmpA44,e->loc);});
goto _LL0;case 5U: _LLB: _tmp10C=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp10B=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_LLC:
 new_e=({struct Cyc_Absyn_Exp*_tmpA48=Cyc_Tcutil_deep_copy_exp(_tmpDF,_tmp10C);enum Cyc_Absyn_Incrementor _tmpA47=_tmp10B;Cyc_Absyn_increment_exp(_tmpA48,_tmpA47,e->loc);});goto _LL0;case 6U: _LLD: _tmp10F=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp10E=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_tmp10D=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmpE1)->f3;_LLE:
# 572
 new_e=({struct Cyc_Absyn_Exp*_tmpA4B=Cyc_Tcutil_deep_copy_exp(_tmpDF,_tmp10F);struct Cyc_Absyn_Exp*_tmpA4A=Cyc_Tcutil_deep_copy_exp(_tmpDF,_tmp10E);struct Cyc_Absyn_Exp*_tmpA49=Cyc_Tcutil_deep_copy_exp(_tmpDF,_tmp10D);Cyc_Absyn_conditional_exp(_tmpA4B,_tmpA4A,_tmpA49,e->loc);});goto _LL0;case 7U: _LLF: _tmp111=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp110=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_LL10:
 new_e=({struct Cyc_Absyn_Exp*_tmpA4D=Cyc_Tcutil_deep_copy_exp(_tmpDF,_tmp111);struct Cyc_Absyn_Exp*_tmpA4C=Cyc_Tcutil_deep_copy_exp(_tmpDF,_tmp110);Cyc_Absyn_and_exp(_tmpA4D,_tmpA4C,e->loc);});goto _LL0;case 8U: _LL11: _tmp113=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp112=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_LL12:
 new_e=({struct Cyc_Absyn_Exp*_tmpA4F=Cyc_Tcutil_deep_copy_exp(_tmpDF,_tmp113);struct Cyc_Absyn_Exp*_tmpA4E=Cyc_Tcutil_deep_copy_exp(_tmpDF,_tmp112);Cyc_Absyn_or_exp(_tmpA4F,_tmpA4E,e->loc);});goto _LL0;case 9U: _LL13: _tmp115=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp114=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_LL14:
 new_e=({struct Cyc_Absyn_Exp*_tmpA51=Cyc_Tcutil_deep_copy_exp(_tmpDF,_tmp115);struct Cyc_Absyn_Exp*_tmpA50=Cyc_Tcutil_deep_copy_exp(_tmpDF,_tmp114);Cyc_Absyn_seq_exp(_tmpA51,_tmpA50,e->loc);});goto _LL0;case 10U: _LL15: _tmp119=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp118=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_tmp117=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmpE1)->f3;_tmp116=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmpE1)->f4;_LL16:
# 577
{struct Cyc_Absyn_VarargCallInfo*_tmpE3=_tmp117;int _tmpEF;struct Cyc_List_List*_tmpEE;struct Cyc_Absyn_VarargInfo*_tmpED;if(_tmpE3 != 0){_LL56: _tmpEF=_tmpE3->num_varargs;_tmpEE=_tmpE3->injectors;_tmpED=_tmpE3->vai;_LL57: {
# 579
struct Cyc_Absyn_VarargInfo*_tmpE4=_tmpED;struct _dyneither_ptr*_tmpEB;struct Cyc_Absyn_Tqual _tmpEA;void*_tmpE9;int _tmpE8;_LL5B: _tmpEB=_tmpE4->name;_tmpEA=_tmpE4->tq;_tmpE9=_tmpE4->type;_tmpE8=_tmpE4->inject;_LL5C:;
new_e=({void*_tmpA57=(void*)({struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*_tmpE7=_cycalloc(sizeof(*_tmpE7));_tmpE7->tag=10U,({
struct Cyc_Absyn_Exp*_tmpA56=Cyc_Tcutil_deep_copy_exp(_tmpDF,_tmp119);_tmpE7->f1=_tmpA56;}),({struct Cyc_List_List*_tmpA55=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*f)(int,struct Cyc_Absyn_Exp*),int env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Tcutil_deep_copy_exp,_tmpDF,_tmp118);_tmpE7->f2=_tmpA55;}),({
struct Cyc_Absyn_VarargCallInfo*_tmpA54=({struct Cyc_Absyn_VarargCallInfo*_tmpE6=_cycalloc(sizeof(*_tmpE6));_tmpE6->num_varargs=_tmpEF,_tmpE6->injectors=_tmpEE,({
struct Cyc_Absyn_VarargInfo*_tmpA53=({struct Cyc_Absyn_VarargInfo*_tmpE5=_cycalloc(sizeof(*_tmpE5));_tmpE5->name=_tmpEB,_tmpE5->tq=_tmpEA,({void*_tmpA52=Cyc_Tcutil_copy_type(_tmpE9);_tmpE5->type=_tmpA52;}),_tmpE5->inject=_tmpE8;_tmpE5;});_tmpE6->vai=_tmpA53;});_tmpE6;});
# 582
_tmpE7->f3=_tmpA54;}),_tmpE7->f4=_tmp116;_tmpE7;});
# 580
Cyc_Absyn_new_exp(_tmpA57,e->loc);});
# 585
goto _LL55;}}else{_LL58: _LL59:
# 587
 new_e=({void*_tmpA5A=(void*)({struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*_tmpEC=_cycalloc(sizeof(*_tmpEC));_tmpEC->tag=10U,({struct Cyc_Absyn_Exp*_tmpA59=Cyc_Tcutil_deep_copy_exp(_tmpDF,_tmp119);_tmpEC->f1=_tmpA59;}),({struct Cyc_List_List*_tmpA58=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*f)(int,struct Cyc_Absyn_Exp*),int env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Tcutil_deep_copy_exp,_tmpDF,_tmp118);_tmpEC->f2=_tmpA58;}),_tmpEC->f3=_tmp117,_tmpEC->f4=_tmp116;_tmpEC;});Cyc_Absyn_new_exp(_tmpA5A,e->loc);});
goto _LL55;}_LL55:;}
# 590
goto _LL0;case 11U: _LL17: _tmp11B=((struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp11A=((struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_LL18:
# 592
 new_e=_tmp11A?({struct Cyc_Absyn_Exp*_tmpA5C=Cyc_Tcutil_deep_copy_exp(_tmpDF,_tmp11B);Cyc_Absyn_rethrow_exp(_tmpA5C,e->loc);}):({struct Cyc_Absyn_Exp*_tmpA5B=Cyc_Tcutil_deep_copy_exp(_tmpDF,_tmp11B);Cyc_Absyn_throw_exp(_tmpA5B,e->loc);});
goto _LL0;case 12U: _LL19: _tmp11C=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_LL1A:
 new_e=({struct Cyc_Absyn_Exp*_tmpA5D=Cyc_Tcutil_deep_copy_exp(_tmpDF,_tmp11C);Cyc_Absyn_noinstantiate_exp(_tmpA5D,e->loc);});
goto _LL0;case 13U: _LL1B: _tmp11E=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp11D=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_LL1C:
# 597
 new_e=({struct Cyc_Absyn_Exp*_tmpA5F=Cyc_Tcutil_deep_copy_exp(_tmpDF,_tmp11E);struct Cyc_List_List*_tmpA5E=((struct Cyc_List_List*(*)(void*(*f)(void*),struct Cyc_List_List*x))Cyc_List_map)(Cyc_Tcutil_copy_type,_tmp11D);Cyc_Absyn_instantiate_exp(_tmpA5F,_tmpA5E,e->loc);});
goto _LL0;case 14U: _LL1D: _tmp122=(void*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp121=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_tmp120=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmpE1)->f3;_tmp11F=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmpE1)->f4;_LL1E:
# 600
 new_e=({void*_tmpA63=Cyc_Tcutil_copy_type(_tmp122);struct Cyc_Absyn_Exp*_tmpA62=Cyc_Tcutil_deep_copy_exp(_tmpDF,_tmp121);int _tmpA61=_tmp120;enum Cyc_Absyn_Coercion _tmpA60=_tmp11F;Cyc_Absyn_cast_exp(_tmpA63,_tmpA62,_tmpA61,_tmpA60,e->loc);});goto _LL0;case 15U: _LL1F: _tmp123=((struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_LL20:
 new_e=({struct Cyc_Absyn_Exp*_tmpA64=Cyc_Tcutil_deep_copy_exp(_tmpDF,_tmp123);Cyc_Absyn_address_exp(_tmpA64,e->loc);});goto _LL0;case 16U: _LL21: _tmp125=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp124=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_LL22: {
# 603
struct Cyc_Absyn_Exp*eo1=_tmp125;if(_tmp125 != 0)eo1=Cyc_Tcutil_deep_copy_exp(_tmpDF,_tmp125);
new_e=({struct Cyc_Absyn_Exp*_tmpA66=eo1;struct Cyc_Absyn_Exp*_tmpA65=Cyc_Tcutil_deep_copy_exp(_tmpDF,_tmp124);Cyc_Absyn_New_exp(_tmpA66,_tmpA65,e->loc);});
goto _LL0;}case 17U: _LL23: _tmp126=(void*)((struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_LL24:
 new_e=({void*_tmpA67=Cyc_Tcutil_copy_type(_tmp126);Cyc_Absyn_sizeoftype_exp(_tmpA67,e->loc);});
goto _LL0;case 18U: _LL25: _tmp127=((struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_LL26:
 new_e=({struct Cyc_Absyn_Exp*_tmpA68=Cyc_Tcutil_deep_copy_exp(_tmpDF,_tmp127);Cyc_Absyn_sizeofexp_exp(_tmpA68,e->loc);});goto _LL0;case 19U: _LL27: _tmp129=(void*)((struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp128=((struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_LL28:
# 610
 new_e=({void*_tmpA6A=Cyc_Tcutil_copy_type(_tmp129);struct Cyc_List_List*_tmpA69=_tmp128;Cyc_Absyn_offsetof_exp(_tmpA6A,_tmpA69,e->loc);});goto _LL0;case 20U: _LL29: _tmp12A=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_LL2A:
 new_e=({struct Cyc_Absyn_Exp*_tmpA6B=Cyc_Tcutil_deep_copy_exp(_tmpDF,_tmp12A);Cyc_Absyn_deref_exp(_tmpA6B,e->loc);});goto _LL0;case 41U: _LL2B: _tmp12B=((struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_LL2C:
 new_e=({struct Cyc_Absyn_Exp*_tmpA6C=Cyc_Tcutil_deep_copy_exp(_tmpDF,_tmp12B);Cyc_Absyn_extension_exp(_tmpA6C,e->loc);});goto _LL0;case 21U: _LL2D: _tmp12F=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp12E=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_tmp12D=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmpE1)->f3;_tmp12C=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmpE1)->f4;_LL2E:
# 614
 new_e=({void*_tmpA6E=(void*)({struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*_tmpF0=_cycalloc(sizeof(*_tmpF0));_tmpF0->tag=21U,({struct Cyc_Absyn_Exp*_tmpA6D=Cyc_Tcutil_deep_copy_exp(_tmpDF,_tmp12F);_tmpF0->f1=_tmpA6D;}),_tmpF0->f2=_tmp12E,_tmpF0->f3=_tmp12D,_tmpF0->f4=_tmp12C;_tmpF0;});Cyc_Absyn_new_exp(_tmpA6E,e->loc);});goto _LL0;case 22U: _LL2F: _tmp133=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp132=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_tmp131=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmpE1)->f3;_tmp130=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmpE1)->f4;_LL30:
# 616
 new_e=({void*_tmpA70=(void*)({struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*_tmpF1=_cycalloc(sizeof(*_tmpF1));_tmpF1->tag=22U,({struct Cyc_Absyn_Exp*_tmpA6F=Cyc_Tcutil_deep_copy_exp(_tmpDF,_tmp133);_tmpF1->f1=_tmpA6F;}),_tmpF1->f2=_tmp132,_tmpF1->f3=_tmp131,_tmpF1->f4=_tmp130;_tmpF1;});Cyc_Absyn_new_exp(_tmpA70,e->loc);});goto _LL0;case 23U: _LL31: _tmp135=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp134=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_LL32:
 new_e=({struct Cyc_Absyn_Exp*_tmpA72=Cyc_Tcutil_deep_copy_exp(_tmpDF,_tmp135);struct Cyc_Absyn_Exp*_tmpA71=Cyc_Tcutil_deep_copy_exp(_tmpDF,_tmp134);Cyc_Absyn_subscript_exp(_tmpA72,_tmpA71,e->loc);});
goto _LL0;case 24U: _LL33: _tmp136=((struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_LL34:
 new_e=({struct Cyc_List_List*_tmpA73=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*f)(int,struct Cyc_Absyn_Exp*),int env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Tcutil_deep_copy_exp,_tmpDF,_tmp136);Cyc_Absyn_tuple_exp(_tmpA73,e->loc);});goto _LL0;case 25U: _LL35: _tmp13A=(((struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*)_tmpE1)->f1)->f1;_tmp139=(((struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*)_tmpE1)->f1)->f2;_tmp138=(((struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*)_tmpE1)->f1)->f3;_tmp137=((struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_LL36: {
# 621
struct _dyneither_ptr*vopt1=_tmp13A;
if(_tmp13A != 0)vopt1=_tmp13A;
new_e=({void*_tmpA77=(void*)({struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*_tmpF3=_cycalloc(sizeof(*_tmpF3));_tmpF3->tag=25U,({struct _tuple10*_tmpA76=({struct _tuple10*_tmpF2=_cycalloc(sizeof(*_tmpF2));_tmpF2->f1=vopt1,_tmpF2->f2=_tmp139,({void*_tmpA75=Cyc_Tcutil_copy_type(_tmp138);_tmpF2->f3=_tmpA75;});_tmpF2;});_tmpF3->f1=_tmpA76;}),({
struct Cyc_List_List*_tmpA74=((struct Cyc_List_List*(*)(struct _tuple18*(*f)(int,struct _tuple18*),int env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Tcutil_copy_eds,_tmpDF,_tmp137);_tmpF3->f2=_tmpA74;});_tmpF3;});
# 623
Cyc_Absyn_new_exp(_tmpA77,e->loc);});
# 625
goto _LL0;}case 26U: _LL37: _tmp13B=((struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_LL38:
 new_e=({void*_tmpA79=(void*)({struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*_tmpF4=_cycalloc(sizeof(*_tmpF4));_tmpF4->tag=26U,({struct Cyc_List_List*_tmpA78=((struct Cyc_List_List*(*)(struct _tuple18*(*f)(int,struct _tuple18*),int env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Tcutil_copy_eds,_tmpDF,_tmp13B);_tmpF4->f1=_tmpA78;});_tmpF4;});Cyc_Absyn_new_exp(_tmpA79,e->loc);});
goto _LL0;case 27U: _LL39: _tmp13F=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp13E=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_tmp13D=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmpE1)->f3;_tmp13C=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmpE1)->f4;_LL3A:
# 629
 new_e=({void*_tmpA7C=(void*)({struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*_tmpF5=_cycalloc(sizeof(*_tmpF5));_tmpF5->tag=27U,_tmpF5->f1=_tmp13F,({struct Cyc_Absyn_Exp*_tmpA7B=Cyc_Tcutil_deep_copy_exp(_tmpDF,_tmp13E);_tmpF5->f2=_tmpA7B;}),({struct Cyc_Absyn_Exp*_tmpA7A=Cyc_Tcutil_deep_copy_exp(_tmpDF,_tmp13D);_tmpF5->f3=_tmpA7A;}),_tmpF5->f4=_tmp13C;_tmpF5;});Cyc_Absyn_new_exp(_tmpA7C,e->loc);});
goto _LL0;case 28U: _LL3B: _tmp142=((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp141=(void*)((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_tmp140=((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmpE1)->f3;_LL3C:
# 632
 new_e=({void*_tmpA7F=(void*)({struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*_tmpF6=_cycalloc(sizeof(*_tmpF6));_tmpF6->tag=28U,({struct Cyc_Absyn_Exp*_tmpA7E=Cyc_Tcutil_deep_copy_exp(_tmpDF,_tmp142);_tmpF6->f1=_tmpA7E;}),({void*_tmpA7D=Cyc_Tcutil_copy_type(_tmp141);_tmpF6->f2=_tmpA7D;}),_tmpF6->f3=_tmp140;_tmpF6;});Cyc_Absyn_new_exp(_tmpA7F,_tmp142->loc);});
# 634
goto _LL0;case 29U: _LL3D: _tmp146=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp145=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_tmp144=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmpE1)->f3;_tmp143=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmpE1)->f4;_LL3E:
# 636
 new_e=({void*_tmpA82=(void*)({struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*_tmpF7=_cycalloc(sizeof(*_tmpF7));_tmpF7->tag=29U,_tmpF7->f1=_tmp146,({struct Cyc_List_List*_tmpA81=((struct Cyc_List_List*(*)(void*(*f)(void*),struct Cyc_List_List*x))Cyc_List_map)(Cyc_Tcutil_copy_type,_tmp145);_tmpF7->f2=_tmpA81;}),({struct Cyc_List_List*_tmpA80=((struct Cyc_List_List*(*)(struct _tuple18*(*f)(int,struct _tuple18*),int env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Tcutil_copy_eds,_tmpDF,_tmp144);_tmpF7->f3=_tmpA80;}),_tmpF7->f4=_tmp143;_tmpF7;});Cyc_Absyn_new_exp(_tmpA82,e->loc);});
# 638
goto _LL0;case 30U: _LL3F: _tmp148=(void*)((struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp147=((struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_LL40:
# 640
 new_e=({void*_tmpA85=(void*)({struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*_tmpF8=_cycalloc(sizeof(*_tmpF8));_tmpF8->tag=30U,({void*_tmpA84=Cyc_Tcutil_copy_type(_tmp148);_tmpF8->f1=_tmpA84;}),({struct Cyc_List_List*_tmpA83=((struct Cyc_List_List*(*)(struct _tuple18*(*f)(int,struct _tuple18*),int env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Tcutil_copy_eds,_tmpDF,_tmp147);_tmpF8->f2=_tmpA83;});_tmpF8;});Cyc_Absyn_new_exp(_tmpA85,e->loc);});
goto _LL0;case 31U: _LL41: _tmp14B=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp14A=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_tmp149=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmpE1)->f3;_LL42:
# 643
 new_e=({void*_tmpA87=(void*)({struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*_tmpF9=_cycalloc(sizeof(*_tmpF9));_tmpF9->tag=31U,({struct Cyc_List_List*_tmpA86=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*f)(int,struct Cyc_Absyn_Exp*),int env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Tcutil_deep_copy_exp,_tmpDF,_tmp14B);_tmpF9->f1=_tmpA86;}),_tmpF9->f2=_tmp14A,_tmpF9->f3=_tmp149;_tmpF9;});Cyc_Absyn_new_exp(_tmpA87,e->loc);});
goto _LL0;case 32U: _LL43: _tmp14D=((struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp14C=((struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_LL44:
 new_e=e;goto _LL0;case 33U: _LL45: _tmp14F=(void*)((struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp14E=((struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_LL46:
# 647
 new_e=({void*_tmpA89=(void*)({struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct*_tmpFA=_cycalloc(sizeof(*_tmpFA));_tmpFA->tag=33U,({void*_tmpA88=Cyc_Tcutil_copy_type(_tmp14F);_tmpFA->f1=_tmpA88;}),_tmpFA->f2=_tmp14E;_tmpFA;});Cyc_Absyn_new_exp(_tmpA89,e->loc);});
goto _LL0;case 34U: _LL47: _tmp155=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmpE1)->f1).is_calloc;_tmp154=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmpE1)->f1).rgn;_tmp153=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmpE1)->f1).elt_type;_tmp152=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmpE1)->f1).num_elts;_tmp151=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmpE1)->f1).fat_result;_tmp150=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmpE1)->f1).inline_call;_LL48: {
# 650
struct Cyc_Absyn_Exp*_tmpFB=Cyc_Absyn_copy_exp(e);
struct Cyc_Absyn_Exp*r1=_tmp154;if(_tmp154 != 0)r1=Cyc_Tcutil_deep_copy_exp(_tmpDF,_tmp154);{
void**t1=_tmp153;if(_tmp153 != 0)t1=({void**_tmpFC=_cycalloc(sizeof(*_tmpFC));({void*_tmpA8A=Cyc_Tcutil_copy_type(*_tmp153);*_tmpFC=_tmpA8A;});_tmpFC;});
({void*_tmpA8B=(void*)({struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*_tmpFD=_cycalloc(sizeof(*_tmpFD));_tmpFD->tag=34U,(_tmpFD->f1).is_calloc=_tmp155,(_tmpFD->f1).rgn=r1,(_tmpFD->f1).elt_type=t1,(_tmpFD->f1).num_elts=_tmp152,(_tmpFD->f1).fat_result=_tmp151,(_tmpFD->f1).inline_call=_tmp150;_tmpFD;});_tmpFB->r=_tmpA8B;});
new_e=_tmpFB;
goto _LL0;};}case 35U: _LL49: _tmp157=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp156=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_LL4A:
 new_e=({struct Cyc_Absyn_Exp*_tmpA8D=Cyc_Tcutil_deep_copy_exp(_tmpDF,_tmp157);struct Cyc_Absyn_Exp*_tmpA8C=Cyc_Tcutil_deep_copy_exp(_tmpDF,_tmp156);Cyc_Absyn_swap_exp(_tmpA8D,_tmpA8C,e->loc);});goto _LL0;case 36U: _LL4B: _tmp159=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp158=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_LL4C: {
# 658
struct Cyc_Core_Opt*nopt1=_tmp159;
if(_tmp159 != 0)nopt1=({struct Cyc_Core_Opt*_tmpFE=_cycalloc(sizeof(*_tmpFE));_tmpFE->v=(struct _tuple2*)_tmp159->v;_tmpFE;});
new_e=({void*_tmpA8F=(void*)({struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*_tmpFF=_cycalloc(sizeof(*_tmpFF));_tmpFF->tag=36U,_tmpFF->f1=nopt1,({struct Cyc_List_List*_tmpA8E=((struct Cyc_List_List*(*)(struct _tuple18*(*f)(int,struct _tuple18*),int env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Tcutil_copy_eds,_tmpDF,_tmp158);_tmpFF->f2=_tmpA8E;});_tmpFF;});Cyc_Absyn_new_exp(_tmpA8F,e->loc);});
goto _LL0;}case 37U: _LL4D: _LL4E:
# 663
(int)_throw((void*)({struct Cyc_Core_Failure_exn_struct*_tmp101=_cycalloc(sizeof(*_tmp101));_tmp101->tag=Cyc_Core_Failure,({struct _dyneither_ptr _tmpA90=({const char*_tmp100="deep_copy: statement expressions unsupported";_tag_dyneither(_tmp100,sizeof(char),45U);});_tmp101->f1=_tmpA90;});_tmp101;}));case 38U: _LL4F: _tmp15B=((struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp15A=((struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_LL50:
 new_e=({void*_tmpA92=(void*)({struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*_tmp102=_cycalloc(sizeof(*_tmp102));_tmp102->tag=38U,({struct Cyc_Absyn_Exp*_tmpA91=Cyc_Tcutil_deep_copy_exp(_tmpDF,_tmp15B);_tmp102->f1=_tmpA91;}),_tmp102->f2=_tmp15A;_tmp102;});Cyc_Absyn_new_exp(_tmpA92,e->loc);});
goto _LL0;case 39U: _LL51: _tmp15C=(void*)((struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_LL52:
 new_e=({void*_tmpA93=Cyc_Tcutil_copy_type(_tmp15C);Cyc_Absyn_valueof_exp(_tmpA93,e->loc);});
goto _LL0;default: _LL53: _tmp15E=((struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp15D=((struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_LL54:
 new_e=Cyc_Absyn_asm_exp(_tmp15E,_tmp15D,e->loc);goto _LL0;}_LL0:;}
# 671
if(preserve_types){
new_e->topt=e->topt;
new_e->annot=e->annot;}
# 675
return new_e;}
# 678
struct Cyc_Absyn_Exp*Cyc_Tcutil_deep_copy_exp_opt(int preserve_types,struct Cyc_Absyn_Exp*e){
if(e == 0)return 0;else{
return Cyc_Tcutil_deep_copy_exp(preserve_types,e);}}struct _tuple19{enum Cyc_Absyn_KindQual f1;enum Cyc_Absyn_KindQual f2;};struct _tuple20{enum Cyc_Absyn_AliasQual f1;enum Cyc_Absyn_AliasQual f2;};
# 691 "tcutil.cyc"
int Cyc_Tcutil_kind_leq(struct Cyc_Absyn_Kind*ka1,struct Cyc_Absyn_Kind*ka2){
struct Cyc_Absyn_Kind*_tmp15F=ka1;enum Cyc_Absyn_KindQual _tmp168;enum Cyc_Absyn_AliasQual _tmp167;_LL1: _tmp168=_tmp15F->kind;_tmp167=_tmp15F->aliasqual;_LL2:;{
struct Cyc_Absyn_Kind*_tmp160=ka2;enum Cyc_Absyn_KindQual _tmp166;enum Cyc_Absyn_AliasQual _tmp165;_LL4: _tmp166=_tmp160->kind;_tmp165=_tmp160->aliasqual;_LL5:;
# 695
if((int)_tmp168 != (int)_tmp166){
struct _tuple19 _tmp161=({struct _tuple19 _tmp982;_tmp982.f1=_tmp168,_tmp982.f2=_tmp166;_tmp982;});struct _tuple19 _tmp162=_tmp161;switch(_tmp162.f1){case Cyc_Absyn_BoxKind: switch(_tmp162.f2){case Cyc_Absyn_MemKind: _LL7: _LL8:
 goto _LLA;case Cyc_Absyn_AnyKind: _LL9: _LLA:
 goto _LLC;default: goto _LLD;}case Cyc_Absyn_MemKind: if(_tmp162.f2 == Cyc_Absyn_AnyKind){_LLB: _LLC:
 goto _LL6;}else{goto _LLD;}default: _LLD: _LLE:
 return 0;}_LL6:;}
# 704
if((int)_tmp167 != (int)_tmp165){
struct _tuple20 _tmp163=({struct _tuple20 _tmp983;_tmp983.f1=_tmp167,_tmp983.f2=_tmp165;_tmp983;});struct _tuple20 _tmp164=_tmp163;switch(_tmp164.f1){case Cyc_Absyn_Aliasable: if(_tmp164.f2 == Cyc_Absyn_Top){_LL10: _LL11:
 goto _LL13;}else{goto _LL14;}case Cyc_Absyn_Unique: if(_tmp164.f2 == Cyc_Absyn_Top){_LL12: _LL13:
 return 1;}else{goto _LL14;}default: _LL14: _LL15:
 return 0;}_LLF:;}
# 711
return 1;};}
# 714
struct Cyc_Absyn_Kind*Cyc_Tcutil_tvar_kind(struct Cyc_Absyn_Tvar*tv,struct Cyc_Absyn_Kind*def){
void*_tmp169=Cyc_Absyn_compress_kb(tv->kind);void*_tmp16A=_tmp169;struct Cyc_Absyn_Kind*_tmp16D;struct Cyc_Absyn_Kind*_tmp16C;switch(*((int*)_tmp16A)){case 0U: _LL1: _tmp16C=((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp16A)->f1;_LL2:
 return _tmp16C;case 2U: _LL3: _tmp16D=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp16A)->f2;_LL4:
 return _tmp16D;default: _LL5: _LL6:
# 719
({void*_tmpA94=(void*)({struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_tmp16B=_cycalloc(sizeof(*_tmp16B));_tmp16B->tag=2U,_tmp16B->f1=0,_tmp16B->f2=def;_tmp16B;});tv->kind=_tmpA94;});
return def;}_LL0:;}
# 724
int Cyc_Tcutil_unify_kindbound(void*kb1,void*kb2){
struct _tuple0 _tmp16E=({struct _tuple0 _tmp984;({void*_tmpA96=Cyc_Absyn_compress_kb(kb1);_tmp984.f1=_tmpA96;}),({void*_tmpA95=Cyc_Absyn_compress_kb(kb2);_tmp984.f2=_tmpA95;});_tmp984;});struct _tuple0 _tmp16F=_tmp16E;struct Cyc_Core_Opt**_tmp184;void*_tmp183;void*_tmp182;struct Cyc_Core_Opt**_tmp181;struct Cyc_Core_Opt**_tmp180;struct Cyc_Absyn_Kind*_tmp17F;struct Cyc_Core_Opt**_tmp17E;struct Cyc_Absyn_Kind*_tmp17D;struct Cyc_Core_Opt**_tmp17C;struct Cyc_Absyn_Kind*_tmp17B;struct Cyc_Absyn_Kind*_tmp17A;struct Cyc_Absyn_Kind*_tmp179;struct Cyc_Core_Opt**_tmp178;struct Cyc_Absyn_Kind*_tmp177;struct Cyc_Absyn_Kind*_tmp176;struct Cyc_Absyn_Kind*_tmp175;switch(*((int*)_tmp16F.f1)){case 0U: switch(*((int*)_tmp16F.f2)){case 0U: _LL1: _tmp176=((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp16F.f1)->f1;_tmp175=((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp16F.f2)->f1;_LL2:
 return _tmp176 == _tmp175;case 2U: _LL5: _tmp179=((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp16F.f1)->f1;_tmp178=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp16F.f2)->f1;_tmp177=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp16F.f2)->f2;_LL6:
# 733
 if(Cyc_Tcutil_kind_leq(_tmp179,_tmp177)){
({struct Cyc_Core_Opt*_tmpA97=({struct Cyc_Core_Opt*_tmp171=_cycalloc(sizeof(*_tmp171));_tmp171->v=kb1;_tmp171;});*_tmp178=_tmpA97;});
return 1;}else{
return 0;}default: goto _LLB;}case 2U: switch(*((int*)_tmp16F.f2)){case 0U: _LL3: _tmp17C=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp16F.f1)->f1;_tmp17B=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp16F.f1)->f2;_tmp17A=((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp16F.f2)->f1;_LL4:
# 728
 if(Cyc_Tcutil_kind_leq(_tmp17A,_tmp17B)){
({struct Cyc_Core_Opt*_tmpA98=({struct Cyc_Core_Opt*_tmp170=_cycalloc(sizeof(*_tmp170));_tmp170->v=kb2;_tmp170;});*_tmp17C=_tmpA98;});
return 1;}else{
return 0;}case 2U: _LL7: _tmp180=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp16F.f1)->f1;_tmp17F=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp16F.f1)->f2;_tmp17E=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp16F.f2)->f1;_tmp17D=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp16F.f2)->f2;_LL8:
# 738
 if(Cyc_Tcutil_kind_leq(_tmp17F,_tmp17D)){
({struct Cyc_Core_Opt*_tmpA99=({struct Cyc_Core_Opt*_tmp172=_cycalloc(sizeof(*_tmp172));_tmp172->v=kb1;_tmp172;});*_tmp17E=_tmpA99;});
return 1;}else{
if(Cyc_Tcutil_kind_leq(_tmp17D,_tmp17F)){
({struct Cyc_Core_Opt*_tmpA9A=({struct Cyc_Core_Opt*_tmp173=_cycalloc(sizeof(*_tmp173));_tmp173->v=kb2;_tmp173;});*_tmp180=_tmpA9A;});
return 1;}else{
return 0;}}default: _LLB: _tmp182=_tmp16F.f1;_tmp181=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmp16F.f2)->f1;_LLC:
# 747
({struct Cyc_Core_Opt*_tmpA9B=({struct Cyc_Core_Opt*_tmp174=_cycalloc(sizeof(*_tmp174));_tmp174->v=_tmp182;_tmp174;});*_tmp181=_tmpA9B;});
return 1;}default: _LL9: _tmp184=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmp16F.f1)->f1;_tmp183=_tmp16F.f2;_LLA:
# 745
 _tmp182=_tmp183;_tmp181=_tmp184;goto _LLC;}_LL0:;}
# 752
struct _tuple15 Cyc_Tcutil_swap_kind(void*t,void*kb){
void*_tmp185=Cyc_Tcutil_compress(t);void*_tmp186=_tmp185;struct Cyc_Absyn_Tvar*_tmp18B;if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp186)->tag == 2U){_LL1: _tmp18B=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp186)->f1;_LL2: {
# 755
void*_tmp187=_tmp18B->kind;
_tmp18B->kind=kb;
return({struct _tuple15 _tmp985;_tmp985.f1=_tmp18B,_tmp985.f2=_tmp187;_tmp985;});}}else{_LL3: _LL4:
# 759
({struct Cyc_String_pa_PrintArg_struct _tmp18A=({struct Cyc_String_pa_PrintArg_struct _tmp986;_tmp986.tag=0U,({struct _dyneither_ptr _tmpA9C=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(t));_tmp986.f1=_tmpA9C;});_tmp986;});void*_tmp188[1U];_tmp188[0]=& _tmp18A;({struct _dyneither_ptr _tmpA9D=({const char*_tmp189="swap_kind: cannot update the kind of %s";_tag_dyneither(_tmp189,sizeof(char),40U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpA9D,_tag_dyneither(_tmp188,sizeof(void*),1U));});});}_LL0:;}
# 765
static struct Cyc_Absyn_Kind*Cyc_Tcutil_field_kind(void*field_type,struct Cyc_List_List*ts,struct Cyc_List_List*tvs){
# 767
struct Cyc_Absyn_Kind*k=Cyc_Tcutil_type_kind(field_type);
if(ts != 0  && (k == & Cyc_Tcutil_ak  || k == & Cyc_Tcutil_tak)){
# 771
struct _RegionHandle _tmp18C=_new_region("temp");struct _RegionHandle*temp=& _tmp18C;_push_region(temp);
{struct Cyc_List_List*_tmp18D=0;
# 774
for(0;tvs != 0;(tvs=tvs->tl,ts=ts->tl)){
struct Cyc_Absyn_Tvar*_tmp18E=(struct Cyc_Absyn_Tvar*)tvs->hd;
void*_tmp18F=(void*)((struct Cyc_List_List*)_check_null(ts))->hd;
struct Cyc_Absyn_Kind*_tmp190=Cyc_Tcutil_tvar_kind(_tmp18E,& Cyc_Tcutil_bk);struct Cyc_Absyn_Kind*_tmp191=_tmp190;switch(((struct Cyc_Absyn_Kind*)_tmp191)->kind){case Cyc_Absyn_IntKind: _LL1: _LL2:
 goto _LL4;case Cyc_Absyn_AnyKind: _LL3: _LL4:
# 780
 _tmp18D=({struct Cyc_List_List*_tmp193=_region_malloc(temp,sizeof(*_tmp193));({struct _tuple15*_tmpA9E=({struct _tuple15*_tmp192=_region_malloc(temp,sizeof(*_tmp192));_tmp192->f1=_tmp18E,_tmp192->f2=_tmp18F;_tmp192;});_tmp193->hd=_tmpA9E;}),_tmp193->tl=_tmp18D;_tmp193;});goto _LL0;default: _LL5: _LL6:
 goto _LL0;}_LL0:;}
# 784
if(_tmp18D != 0){
field_type=({struct _RegionHandle*_tmpAA0=temp;struct Cyc_List_List*_tmpA9F=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(_tmp18D);Cyc_Tcutil_rsubstitute(_tmpAA0,_tmpA9F,field_type);});
k=Cyc_Tcutil_type_kind(field_type);}}
# 772
;_pop_region(temp);}
# 789
return k;}
# 796
struct Cyc_Absyn_Kind*Cyc_Tcutil_type_kind(void*t){
# 798
void*_tmp194=Cyc_Tcutil_compress(t);void*_tmp195=_tmp194;struct Cyc_Absyn_Typedefdecl*_tmp1B3;struct Cyc_Absyn_Exp*_tmp1B2;struct Cyc_Absyn_PtrInfo _tmp1B1;void*_tmp1B0;struct Cyc_List_List*_tmp1AF;struct Cyc_Absyn_Tvar*_tmp1AE;struct Cyc_Core_Opt*_tmp1AD;switch(*((int*)_tmp195)){case 1U: _LL1: _tmp1AD=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp195)->f1;_LL2:
 return(struct Cyc_Absyn_Kind*)((struct Cyc_Core_Opt*)_check_null(_tmp1AD))->v;case 2U: _LL3: _tmp1AE=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp195)->f1;_LL4:
 return Cyc_Tcutil_tvar_kind(_tmp1AE,& Cyc_Tcutil_bk);case 0U: _LL5: _tmp1B0=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp195)->f1;_tmp1AF=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp195)->f2;_LL6: {
# 802
void*_tmp196=_tmp1B0;enum Cyc_Absyn_AggrKind _tmp1A3;struct Cyc_List_List*_tmp1A2;struct Cyc_Absyn_AggrdeclImpl*_tmp1A1;int _tmp1A0;struct Cyc_Absyn_Kind*_tmp19F;enum Cyc_Absyn_Size_of _tmp19E;switch(*((int*)_tmp196)){case 0U: _LL1E: _LL1F:
 return& Cyc_Tcutil_mk;case 1U: _LL20: _tmp19E=((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp196)->f2;_LL21:
 return((int)_tmp19E == (int)2U  || (int)_tmp19E == (int)3U)?& Cyc_Tcutil_bk:& Cyc_Tcutil_mk;case 2U: _LL22: _LL23:
 return& Cyc_Tcutil_mk;case 15U: _LL24: _LL25:
 goto _LL27;case 16U: _LL26: _LL27:
 goto _LL29;case 3U: _LL28: _LL29:
 return& Cyc_Tcutil_bk;case 6U: _LL2A: _LL2B:
 return& Cyc_Tcutil_urk;case 5U: _LL2C: _LL2D:
 return& Cyc_Tcutil_rk;case 7U: _LL2E: _LL2F:
 return& Cyc_Tcutil_trk;case 17U: _LL30: _tmp19F=((struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct*)_tmp196)->f2;_LL31:
 return _tmp19F;case 4U: _LL32: _LL33:
 return& Cyc_Tcutil_bk;case 8U: _LL34: _LL35:
 goto _LL37;case 9U: _LL36: _LL37:
 goto _LL39;case 10U: _LL38: _LL39:
 return& Cyc_Tcutil_ek;case 12U: _LL3A: _LL3B:
 goto _LL3D;case 11U: _LL3C: _LL3D:
 return& Cyc_Tcutil_boolk;case 13U: _LL3E: _LL3F:
 goto _LL41;case 14U: _LL40: _LL41:
 return& Cyc_Tcutil_ptrbk;case 18U: _LL42: _LL43:
 return& Cyc_Tcutil_ak;case 19U: if(((((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)_tmp196)->f1).KnownDatatypefield).tag == 2){_LL44: _LL45:
# 823
 return& Cyc_Tcutil_mk;}else{_LL46: _LL47:
# 825
({void*_tmp197=0U;({struct _dyneither_ptr _tmpAA1=({const char*_tmp198="type_kind: Unresolved DatatypeFieldType";_tag_dyneither(_tmp198,sizeof(char),40U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpAA1,_tag_dyneither(_tmp197,sizeof(void*),0U));});});}default: if(((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_tmp196)->f1).UnknownAggr).tag == 1){_LL48: _LL49:
# 829
 return& Cyc_Tcutil_ak;}else{_LL4A: _tmp1A3=(*((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_tmp196)->f1).KnownAggr).val)->kind;_tmp1A2=(*((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_tmp196)->f1).KnownAggr).val)->tvs;_tmp1A1=(*((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_tmp196)->f1).KnownAggr).val)->impl;_tmp1A0=(*((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_tmp196)->f1).KnownAggr).val)->expected_mem_kind;_LL4B:
# 831
 if(_tmp1A1 == 0){
if(_tmp1A0)
return& Cyc_Tcutil_mk;else{
# 835
return& Cyc_Tcutil_ak;}}{
# 837
struct Cyc_List_List*_tmp199=_tmp1A1->fields;
if(_tmp199 == 0)return& Cyc_Tcutil_mk;
# 840
if((int)_tmp1A3 == (int)0U){
for(0;((struct Cyc_List_List*)_check_null(_tmp199))->tl != 0;_tmp199=_tmp199->tl){;}{
void*_tmp19A=((struct Cyc_Absyn_Aggrfield*)_tmp199->hd)->type;
struct Cyc_Absyn_Kind*_tmp19B=Cyc_Tcutil_field_kind(_tmp19A,_tmp1AF,_tmp1A2);
if(_tmp19B == & Cyc_Tcutil_ak  || _tmp19B == & Cyc_Tcutil_tak)return _tmp19B;};}else{
# 848
for(0;_tmp199 != 0;_tmp199=_tmp199->tl){
void*_tmp19C=((struct Cyc_Absyn_Aggrfield*)_tmp199->hd)->type;
struct Cyc_Absyn_Kind*_tmp19D=Cyc_Tcutil_field_kind(_tmp19C,_tmp1AF,_tmp1A2);
if(_tmp19D == & Cyc_Tcutil_ak  || _tmp19D == & Cyc_Tcutil_tak)return _tmp19D;}}
# 854
return& Cyc_Tcutil_mk;};}}_LL1D:;}case 5U: _LL7: _LL8:
# 856
 return& Cyc_Tcutil_ak;case 7U: _LL9: _LLA:
 return& Cyc_Tcutil_mk;case 3U: _LLB: _tmp1B1=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp195)->f1;_LLC: {
# 859
void*_tmp1A4=Cyc_Tcutil_compress((_tmp1B1.ptr_atts).bounds);void*_tmp1A5=_tmp1A4;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1A5)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1A5)->f1)){case 13U: _LL4D: _LL4E: {
# 861
enum Cyc_Absyn_AliasQual _tmp1A6=(Cyc_Tcutil_type_kind((_tmp1B1.ptr_atts).rgn))->aliasqual;enum Cyc_Absyn_AliasQual _tmp1A7=_tmp1A6;switch(_tmp1A7){case Cyc_Absyn_Aliasable: _LL54: _LL55:
 return& Cyc_Tcutil_bk;case Cyc_Absyn_Unique: _LL56: _LL57:
 return& Cyc_Tcutil_ubk;default: _LL58: _LL59:
 return& Cyc_Tcutil_tbk;}_LL53:;}case 14U: _LL4F: _LL50:
# 867
 goto _LL52;default: goto _LL51;}else{_LL51: _LL52: {
# 869
enum Cyc_Absyn_AliasQual _tmp1A8=(Cyc_Tcutil_type_kind((_tmp1B1.ptr_atts).rgn))->aliasqual;enum Cyc_Absyn_AliasQual _tmp1A9=_tmp1A8;switch(_tmp1A9){case Cyc_Absyn_Aliasable: _LL5B: _LL5C:
 return& Cyc_Tcutil_mk;case Cyc_Absyn_Unique: _LL5D: _LL5E:
 return& Cyc_Tcutil_umk;default: _LL5F: _LL60:
 return& Cyc_Tcutil_tmk;}_LL5A:;}}_LL4C:;}case 9U: _LLD: _LLE:
# 875
 return& Cyc_Tcutil_ik;case 11U: _LLF: _LL10:
# 879
 return& Cyc_Tcutil_ak;case 4U: _LL11: _tmp1B2=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp195)->f1).num_elts;_LL12:
# 881
 if(_tmp1B2 == 0  || Cyc_Tcutil_is_const_exp(_tmp1B2))return& Cyc_Tcutil_mk;
return& Cyc_Tcutil_ak;case 6U: _LL13: _LL14:
 return& Cyc_Tcutil_mk;case 8U: _LL15: _tmp1B3=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp195)->f3;_LL16:
# 885
 if(_tmp1B3 == 0  || _tmp1B3->kind == 0)
({struct Cyc_String_pa_PrintArg_struct _tmp1AC=({struct Cyc_String_pa_PrintArg_struct _tmp987;_tmp987.tag=0U,({struct _dyneither_ptr _tmpAA2=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(t));_tmp987.f1=_tmpAA2;});_tmp987;});void*_tmp1AA[1U];_tmp1AA[0]=& _tmp1AC;({struct _dyneither_ptr _tmpAA3=({const char*_tmp1AB="type_kind: typedef found: %s";_tag_dyneither(_tmp1AB,sizeof(char),29U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpAA3,_tag_dyneither(_tmp1AA,sizeof(void*),1U));});});
return(struct Cyc_Absyn_Kind*)((struct Cyc_Core_Opt*)_check_null(_tmp1B3->kind))->v;default: switch(*((int*)((struct Cyc_Absyn_TypeDecl*)((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp195)->f1)->r)){case 0U: _LL17: _LL18:
 return& Cyc_Tcutil_ak;case 1U: _LL19: _LL1A:
 return& Cyc_Tcutil_bk;default: _LL1B: _LL1C:
 return& Cyc_Tcutil_ak;}}_LL0:;}
# 894
int Cyc_Tcutil_kind_eq(struct Cyc_Absyn_Kind*k1,struct Cyc_Absyn_Kind*k2){
return k1 == k2  || (int)k1->kind == (int)k2->kind  && (int)k1->aliasqual == (int)k2->aliasqual;}
# 899
int Cyc_Tcutil_unify(void*t1,void*t2){
struct _handler_cons _tmp1B4;_push_handler(& _tmp1B4);{int _tmp1B6=0;if(setjmp(_tmp1B4.handler))_tmp1B6=1;if(!_tmp1B6){
Cyc_Tcutil_unify_it(t1,t2);{
int _tmp1B7=1;_npop_handler(0U);return _tmp1B7;};
# 901
;_pop_handler();}else{void*_tmp1B5=(void*)_exn_thrown;void*_tmp1B8=_tmp1B5;void*_tmp1B9;if(((struct Cyc_Tcutil_Unify_exn_struct*)_tmp1B8)->tag == Cyc_Tcutil_Unify){_LL1: _LL2:
# 903
 return 0;}else{_LL3: _tmp1B9=_tmp1B8;_LL4:(int)_rethrow(_tmp1B9);}_LL0:;}};}
# 908
static void Cyc_Tcutil_occurslist(void*evar,struct _RegionHandle*r,struct Cyc_List_List*env,struct Cyc_List_List*ts);
static void Cyc_Tcutil_occurs(void*evar,struct _RegionHandle*r,struct Cyc_List_List*env,void*t){
t=Cyc_Tcutil_compress(t);{
void*_tmp1BA=t;struct Cyc_List_List*_tmp1DA;struct Cyc_List_List*_tmp1D9;struct Cyc_List_List*_tmp1D8;struct Cyc_List_List*_tmp1D7;struct Cyc_List_List*_tmp1D6;void*_tmp1D5;struct Cyc_Absyn_Tqual _tmp1D4;void*_tmp1D3;struct Cyc_List_List*_tmp1D2;int _tmp1D1;struct Cyc_Absyn_VarargInfo*_tmp1D0;struct Cyc_List_List*_tmp1CF;struct Cyc_List_List*_tmp1CE;struct Cyc_Absyn_Exp*_tmp1CD;struct Cyc_List_List*_tmp1CC;struct Cyc_Absyn_Exp*_tmp1CB;struct Cyc_List_List*_tmp1CA;void*_tmp1C9;void*_tmp1C8;struct Cyc_Absyn_PtrInfo _tmp1C7;void*_tmp1C6;struct Cyc_Core_Opt**_tmp1C5;struct Cyc_Absyn_Tvar*_tmp1C4;switch(*((int*)_tmp1BA)){case 2U: _LL1: _tmp1C4=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp1BA)->f1;_LL2:
# 913
 if(!((int(*)(int(*compare)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct Cyc_Absyn_Tvar*x))Cyc_List_mem)(Cyc_Tcutil_fast_tvar_cmp,env,_tmp1C4)){
Cyc_Tcutil_failure_reason=({const char*_tmp1BB="(type variable would escape scope)";_tag_dyneither(_tmp1BB,sizeof(char),35U);});
(int)_throw((void*)& Cyc_Tcutil_Unify_val);}
# 917
goto _LL0;case 1U: _LL3: _tmp1C6=(void*)((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp1BA)->f2;_tmp1C5=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp1BA)->f4;_LL4:
# 919
 if(t == evar){
Cyc_Tcutil_failure_reason=({const char*_tmp1BC="(occurs check)";_tag_dyneither(_tmp1BC,sizeof(char),15U);});
(int)_throw((void*)& Cyc_Tcutil_Unify_val);}else{
# 923
if(_tmp1C6 != 0)Cyc_Tcutil_occurs(evar,r,env,_tmp1C6);else{
# 926
int problem=0;
{struct Cyc_List_List*s=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(*_tmp1C5))->v;for(0;s != 0;s=s->tl){
if(!((int(*)(int(*compare)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct Cyc_Absyn_Tvar*x))Cyc_List_mem)(Cyc_Tcutil_fast_tvar_cmp,env,(struct Cyc_Absyn_Tvar*)s->hd)){
problem=1;break;}}}
# 933
if(problem){
struct Cyc_List_List*_tmp1BD=0;
{struct Cyc_List_List*s=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(*_tmp1C5))->v;for(0;s != 0;s=s->tl){
if(((int(*)(int(*compare)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct Cyc_Absyn_Tvar*x))Cyc_List_mem)(Cyc_Tcutil_fast_tvar_cmp,env,(struct Cyc_Absyn_Tvar*)s->hd))
_tmp1BD=({struct Cyc_List_List*_tmp1BE=_cycalloc(sizeof(*_tmp1BE));_tmp1BE->hd=(struct Cyc_Absyn_Tvar*)s->hd,_tmp1BE->tl=_tmp1BD;_tmp1BE;});}}
# 939
({struct Cyc_Core_Opt*_tmpAA4=({struct Cyc_Core_Opt*_tmp1BF=_cycalloc(sizeof(*_tmp1BF));_tmp1BF->v=_tmp1BD;_tmp1BF;});*_tmp1C5=_tmpAA4;});}}}
# 942
goto _LL0;case 3U: _LL5: _tmp1C7=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1BA)->f1;_LL6:
# 944
 Cyc_Tcutil_occurs(evar,r,env,_tmp1C7.elt_type);
Cyc_Tcutil_occurs(evar,r,env,(_tmp1C7.ptr_atts).rgn);
Cyc_Tcutil_occurs(evar,r,env,(_tmp1C7.ptr_atts).nullable);
Cyc_Tcutil_occurs(evar,r,env,(_tmp1C7.ptr_atts).bounds);
Cyc_Tcutil_occurs(evar,r,env,(_tmp1C7.ptr_atts).zero_term);
goto _LL0;case 4U: _LL7: _tmp1C9=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp1BA)->f1).elt_type;_tmp1C8=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp1BA)->f1).zero_term;_LL8:
# 952
 Cyc_Tcutil_occurs(evar,r,env,_tmp1C9);
Cyc_Tcutil_occurs(evar,r,env,_tmp1C8);
goto _LL0;case 5U: _LL9: _tmp1D6=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1BA)->f1).tvars;_tmp1D5=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1BA)->f1).effect;_tmp1D4=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1BA)->f1).ret_tqual;_tmp1D3=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1BA)->f1).ret_type;_tmp1D2=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1BA)->f1).args;_tmp1D1=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1BA)->f1).c_varargs;_tmp1D0=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1BA)->f1).cyc_varargs;_tmp1CF=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1BA)->f1).rgn_po;_tmp1CE=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1BA)->f1).attributes;_tmp1CD=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1BA)->f1).requires_clause;_tmp1CC=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1BA)->f1).requires_relns;_tmp1CB=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1BA)->f1).ensures_clause;_tmp1CA=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1BA)->f1).ensures_relns;_LLA:
# 957
 env=((struct Cyc_List_List*(*)(struct _RegionHandle*,struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_rappend)(r,_tmp1D6,env);
if(_tmp1D5 != 0)Cyc_Tcutil_occurs(evar,r,env,_tmp1D5);
Cyc_Tcutil_occurs(evar,r,env,_tmp1D3);
for(0;_tmp1D2 != 0;_tmp1D2=_tmp1D2->tl){
Cyc_Tcutil_occurs(evar,r,env,(*((struct _tuple10*)_tmp1D2->hd)).f3);}
if(_tmp1D0 != 0)
Cyc_Tcutil_occurs(evar,r,env,_tmp1D0->type);
for(0;_tmp1CF != 0;_tmp1CF=_tmp1CF->tl){
struct _tuple0*_tmp1C0=(struct _tuple0*)_tmp1CF->hd;struct _tuple0*_tmp1C1=_tmp1C0;void*_tmp1C3;void*_tmp1C2;_LL16: _tmp1C3=_tmp1C1->f1;_tmp1C2=_tmp1C1->f2;_LL17:;
Cyc_Tcutil_occurs(evar,r,env,_tmp1C3);
Cyc_Tcutil_occurs(evar,r,env,_tmp1C2);}
# 969
goto _LL0;case 6U: _LLB: _tmp1D7=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp1BA)->f1;_LLC:
# 971
 for(0;_tmp1D7 != 0;_tmp1D7=_tmp1D7->tl){
Cyc_Tcutil_occurs(evar,r,env,(*((struct _tuple12*)_tmp1D7->hd)).f2);}
goto _LL0;case 7U: _LLD: _tmp1D8=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp1BA)->f2;_LLE:
# 976
 for(0;_tmp1D8 != 0;_tmp1D8=_tmp1D8->tl){
Cyc_Tcutil_occurs(evar,r,env,((struct Cyc_Absyn_Aggrfield*)_tmp1D8->hd)->type);}
goto _LL0;case 8U: _LLF: _tmp1D9=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp1BA)->f2;_LL10:
 _tmp1DA=_tmp1D9;goto _LL12;case 0U: _LL11: _tmp1DA=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1BA)->f2;_LL12:
# 981
 Cyc_Tcutil_occurslist(evar,r,env,_tmp1DA);goto _LL0;default: _LL13: _LL14:
# 984
 goto _LL0;}_LL0:;};}
# 987
static void Cyc_Tcutil_occurslist(void*evar,struct _RegionHandle*r,struct Cyc_List_List*env,struct Cyc_List_List*ts){
# 989
for(0;ts != 0;ts=ts->tl){
Cyc_Tcutil_occurs(evar,r,env,(void*)ts->hd);}}
# 994
static void Cyc_Tcutil_unify_list(struct Cyc_List_List*t1,struct Cyc_List_List*t2){
for(0;t1 != 0  && t2 != 0;(t1=t1->tl,t2=t2->tl)){
Cyc_Tcutil_unify_it((void*)t1->hd,(void*)t2->hd);}
if(t1 != 0  || t2 != 0)
(int)_throw((void*)& Cyc_Tcutil_Unify_val);}
# 1002
static void Cyc_Tcutil_unify_tqual(struct Cyc_Absyn_Tqual tq1,void*t1,struct Cyc_Absyn_Tqual tq2,void*t2){
if(tq1.print_const  && !tq1.real_const)
({void*_tmp1DB=0U;({struct _dyneither_ptr _tmpAA5=({const char*_tmp1DC="tq1 real_const not set.";_tag_dyneither(_tmp1DC,sizeof(char),24U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpAA5,_tag_dyneither(_tmp1DB,sizeof(void*),0U));});});
if(tq2.print_const  && !tq2.real_const)
({void*_tmp1DD=0U;({struct _dyneither_ptr _tmpAA6=({const char*_tmp1DE="tq2 real_const not set.";_tag_dyneither(_tmp1DE,sizeof(char),24U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpAA6,_tag_dyneither(_tmp1DD,sizeof(void*),0U));});});
# 1008
if((tq1.real_const != tq2.real_const  || tq1.q_volatile != tq2.q_volatile) || tq1.q_restrict != tq2.q_restrict){
# 1011
Cyc_Tcutil_t1_failure=t1;
Cyc_Tcutil_t2_failure=t2;
Cyc_Tcutil_tq1_const=tq1.real_const;
Cyc_Tcutil_tq2_const=tq2.real_const;
Cyc_Tcutil_failure_reason=({const char*_tmp1DF="(qualifiers don't match)";_tag_dyneither(_tmp1DF,sizeof(char),25U);});
(int)_throw((void*)& Cyc_Tcutil_Unify_val);}
# 1019
Cyc_Tcutil_tq1_const=0;
Cyc_Tcutil_tq2_const=0;}
# 1023
int Cyc_Tcutil_equal_tqual(struct Cyc_Absyn_Tqual tq1,struct Cyc_Absyn_Tqual tq2){
return(tq1.real_const == tq2.real_const  && tq1.q_volatile == tq2.q_volatile) && tq1.q_restrict == tq2.q_restrict;}
# 1029
static void Cyc_Tcutil_unify_cmp_exp(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2){
struct _tuple0 _tmp1E0=({struct _tuple0 _tmp988;_tmp988.f1=e1->r,_tmp988.f2=e2->r;_tmp988;});struct _tuple0 _tmp1E1=_tmp1E0;void*_tmp1EB;void*_tmp1EA;struct Cyc_Absyn_Exp*_tmp1E9;struct Cyc_Absyn_Exp*_tmp1E8;if(((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp1E1.f1)->tag == 14U){_LL1: _tmp1E8=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp1E1.f1)->f2;_LL2:
# 1034
 Cyc_Tcutil_unify_cmp_exp(_tmp1E8,e2);return;}else{if(((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp1E1.f2)->tag == 14U){_LL3: _tmp1E9=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp1E1.f2)->f2;_LL4:
 Cyc_Tcutil_unify_cmp_exp(e1,_tmp1E9);return;}else{if(((struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*)_tmp1E1.f1)->tag == 39U){_LL5: _tmp1EA=(void*)((struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*)_tmp1E1.f1)->f1;_LL6: {
# 1037
void*_tmp1E2=Cyc_Tcutil_compress(_tmp1EA);void*_tmp1E3=_tmp1E2;if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp1E3)->tag == 1U){_LLC: _LLD:
({void*_tmpAA7=_tmp1EA;Cyc_Tcutil_unify_it(_tmpAA7,(void*)({struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_tmp1E4=_cycalloc(sizeof(*_tmp1E4));_tmp1E4->tag=9U,_tmp1E4->f1=e2;_tmp1E4;}));});return;}else{_LLE: _LLF:
(int)_throw((void*)& Cyc_Tcutil_Unify_val);}_LLB:;}}else{if(((struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*)_tmp1E1.f2)->tag == 39U){_LL7: _tmp1EB=(void*)((struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*)_tmp1E1.f2)->f1;_LL8: {
# 1042
void*_tmp1E5=Cyc_Tcutil_compress(_tmp1EB);void*_tmp1E6=_tmp1E5;if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp1E6)->tag == 1U){_LL11: _LL12:
({void*_tmpAA8=_tmp1EB;Cyc_Tcutil_unify_it(_tmpAA8,(void*)({struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_tmp1E7=_cycalloc(sizeof(*_tmp1E7));_tmp1E7->tag=9U,_tmp1E7->f1=e1;_tmp1E7;}));});return;}else{_LL13: _LL14:
(int)_throw((void*)& Cyc_Tcutil_Unify_val);}_LL10:;}}else{_LL9: _LLA:
# 1046
(int)_throw((void*)& Cyc_Tcutil_Unify_val);}}}}_LL0:;}
# 1050
static int Cyc_Tcutil_attribute_case_number(void*att){
void*_tmp1EC=att;switch(*((int*)_tmp1EC)){case 0U: _LL1: _LL2:
 return 0;case 1U: _LL3: _LL4:
 return 1;case 2U: _LL5: _LL6:
 return 2;case 3U: _LL7: _LL8:
 return 3;case 4U: _LL9: _LLA:
 return 4;case 5U: _LLB: _LLC:
 return 5;case 6U: _LLD: _LLE:
 return 6;case 7U: _LLF: _LL10:
 return 7;case 8U: _LL11: _LL12:
 return 8;case 9U: _LL13: _LL14:
 return 9;case 10U: _LL15: _LL16:
 return 10;case 11U: _LL17: _LL18:
 return 11;case 12U: _LL19: _LL1A:
 return 12;case 13U: _LL1B: _LL1C:
 return 13;case 14U: _LL1D: _LL1E:
 return 14;case 15U: _LL1F: _LL20:
 return 15;case 16U: _LL21: _LL22:
 return 16;case 17U: _LL23: _LL24:
 return 17;case 18U: _LL25: _LL26:
 return 18;case 19U: _LL27: _LL28:
 return 19;case 20U: _LL29: _LL2A:
 return 20;default: _LL2B: _LL2C:
 return 21;}_LL0:;}
# 1077
static int Cyc_Tcutil_attribute_cmp(void*att1,void*att2){
struct _tuple0 _tmp1ED=({struct _tuple0 _tmp989;_tmp989.f1=att1,_tmp989.f2=att2;_tmp989;});struct _tuple0 _tmp1EE=_tmp1ED;enum Cyc_Absyn_Format_Type _tmp1FE;int _tmp1FD;int _tmp1FC;enum Cyc_Absyn_Format_Type _tmp1FB;int _tmp1FA;int _tmp1F9;struct _dyneither_ptr _tmp1F8;struct _dyneither_ptr _tmp1F7;struct Cyc_Absyn_Exp*_tmp1F6;struct Cyc_Absyn_Exp*_tmp1F5;int _tmp1F4;int _tmp1F3;int _tmp1F2;int _tmp1F1;switch(*((int*)_tmp1EE.f1)){case 0U: if(((struct Cyc_Absyn_Regparm_att_Absyn_Attribute_struct*)_tmp1EE.f2)->tag == 0U){_LL1: _tmp1F2=((struct Cyc_Absyn_Regparm_att_Absyn_Attribute_struct*)_tmp1EE.f1)->f1;_tmp1F1=((struct Cyc_Absyn_Regparm_att_Absyn_Attribute_struct*)_tmp1EE.f2)->f1;_LL2:
 _tmp1F4=_tmp1F2;_tmp1F3=_tmp1F1;goto _LL4;}else{goto _LLB;}case 20U: if(((struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct*)_tmp1EE.f2)->tag == 20U){_LL3: _tmp1F4=((struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct*)_tmp1EE.f1)->f1;_tmp1F3=((struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct*)_tmp1EE.f2)->f1;_LL4:
# 1081
 return Cyc_Core_intcmp(_tmp1F4,_tmp1F3);}else{goto _LLB;}case 6U: if(((struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct*)_tmp1EE.f2)->tag == 6U){_LL5: _tmp1F6=((struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct*)_tmp1EE.f1)->f1;_tmp1F5=((struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct*)_tmp1EE.f2)->f1;_LL6:
# 1083
 if(_tmp1F6 == _tmp1F5)return 0;
if(_tmp1F6 == 0)return - 1;
if(_tmp1F5 == 0)return 1;
return Cyc_Evexp_const_exp_cmp(_tmp1F6,_tmp1F5);}else{goto _LLB;}case 8U: if(((struct Cyc_Absyn_Section_att_Absyn_Attribute_struct*)_tmp1EE.f2)->tag == 8U){_LL7: _tmp1F8=((struct Cyc_Absyn_Section_att_Absyn_Attribute_struct*)_tmp1EE.f1)->f1;_tmp1F7=((struct Cyc_Absyn_Section_att_Absyn_Attribute_struct*)_tmp1EE.f2)->f1;_LL8:
 return Cyc_strcmp((struct _dyneither_ptr)_tmp1F8,(struct _dyneither_ptr)_tmp1F7);}else{goto _LLB;}case 19U: if(((struct Cyc_Absyn_Format_att_Absyn_Attribute_struct*)_tmp1EE.f2)->tag == 19U){_LL9: _tmp1FE=((struct Cyc_Absyn_Format_att_Absyn_Attribute_struct*)_tmp1EE.f1)->f1;_tmp1FD=((struct Cyc_Absyn_Format_att_Absyn_Attribute_struct*)_tmp1EE.f1)->f2;_tmp1FC=((struct Cyc_Absyn_Format_att_Absyn_Attribute_struct*)_tmp1EE.f1)->f3;_tmp1FB=((struct Cyc_Absyn_Format_att_Absyn_Attribute_struct*)_tmp1EE.f2)->f1;_tmp1FA=((struct Cyc_Absyn_Format_att_Absyn_Attribute_struct*)_tmp1EE.f2)->f2;_tmp1F9=((struct Cyc_Absyn_Format_att_Absyn_Attribute_struct*)_tmp1EE.f2)->f3;_LLA: {
# 1089
int _tmp1EF=Cyc_Core_intcmp((int)((unsigned int)_tmp1FE),(int)((unsigned int)_tmp1FB));
if(_tmp1EF != 0)return _tmp1EF;{
int _tmp1F0=Cyc_Core_intcmp(_tmp1FD,_tmp1FA);
if(_tmp1F0 != 0)return _tmp1F0;
return Cyc_Core_intcmp(_tmp1FC,_tmp1F9);};}}else{goto _LLB;}default: _LLB: _LLC:
# 1095
 return({int _tmpAA9=Cyc_Tcutil_attribute_case_number(att1);Cyc_Core_intcmp(_tmpAA9,Cyc_Tcutil_attribute_case_number(att2));});}_LL0:;}
# 1099
static int Cyc_Tcutil_equal_att(void*a1,void*a2){
return Cyc_Tcutil_attribute_cmp(a1,a2)== 0;}
# 1103
int Cyc_Tcutil_same_atts(struct Cyc_List_List*a1,struct Cyc_List_List*a2){
{struct Cyc_List_List*a=a1;for(0;a != 0;a=a->tl){
if(!((int(*)(int(*pred)(void*,void*),void*env,struct Cyc_List_List*x))Cyc_List_exists_c)(Cyc_Tcutil_equal_att,(void*)a->hd,a2))return 0;}}
{struct Cyc_List_List*a=a2;for(0;a != 0;a=a->tl){
if(!((int(*)(int(*pred)(void*,void*),void*env,struct Cyc_List_List*x))Cyc_List_exists_c)(Cyc_Tcutil_equal_att,(void*)a->hd,a1))return 0;}}
return 1;}
# 1112
static void*Cyc_Tcutil_rgns_of(void*t);
# 1114
static void*Cyc_Tcutil_rgns_of_field(struct Cyc_Absyn_Aggrfield*af){
return Cyc_Tcutil_rgns_of(af->type);}
# 1118
static struct _tuple15*Cyc_Tcutil_region_free_subst(struct Cyc_Absyn_Tvar*tv){
void*t;
{struct Cyc_Absyn_Kind*_tmp1FF=Cyc_Tcutil_tvar_kind(tv,& Cyc_Tcutil_bk);struct Cyc_Absyn_Kind*_tmp200=_tmp1FF;switch(((struct Cyc_Absyn_Kind*)_tmp200)->kind){case Cyc_Absyn_RgnKind: switch(((struct Cyc_Absyn_Kind*)_tmp200)->aliasqual){case Cyc_Absyn_Unique: _LL1: _LL2:
 t=Cyc_Absyn_unique_rgn_type;goto _LL0;case Cyc_Absyn_Aliasable: _LL3: _LL4:
 t=Cyc_Absyn_heap_rgn_type;goto _LL0;default: goto _LLD;}case Cyc_Absyn_EffKind: _LL5: _LL6:
 t=Cyc_Absyn_empty_effect;goto _LL0;case Cyc_Absyn_IntKind: _LL7: _LL8:
 t=(void*)({struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_tmp201=_cycalloc(sizeof(*_tmp201));_tmp201->tag=9U,({struct Cyc_Absyn_Exp*_tmpAAA=Cyc_Absyn_uint_exp(0U,0U);_tmp201->f1=_tmpAAA;});_tmp201;});goto _LL0;case Cyc_Absyn_BoolKind: _LL9: _LLA:
 t=Cyc_Absyn_true_type;goto _LL0;case Cyc_Absyn_PtrBndKind: _LLB: _LLC:
 t=Cyc_Absyn_fat_bound_type;goto _LL0;default: _LLD: _LLE:
 t=Cyc_Absyn_sint_type;goto _LL0;}_LL0:;}
# 1129
return({struct _tuple15*_tmp202=_cycalloc(sizeof(*_tmp202));_tmp202->f1=tv,_tmp202->f2=t;_tmp202;});}
# 1136
static void*Cyc_Tcutil_rgns_of(void*t){
void*_tmp203=Cyc_Tcutil_compress(t);void*_tmp204=_tmp203;struct Cyc_List_List*_tmp21A;struct Cyc_List_List*_tmp219;struct Cyc_List_List*_tmp218;void*_tmp217;struct Cyc_Absyn_Tqual _tmp216;void*_tmp215;struct Cyc_List_List*_tmp214;struct Cyc_Absyn_VarargInfo*_tmp213;struct Cyc_List_List*_tmp212;struct Cyc_List_List*_tmp211;void*_tmp210;void*_tmp20F;void*_tmp20E;struct Cyc_List_List*_tmp20D;switch(*((int*)_tmp204)){case 0U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp204)->f2 == 0){_LL1: _LL2:
 return Cyc_Absyn_empty_effect;}else{if(((struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp204)->f1)->tag == 9U){_LL3: _LL4:
 return t;}else{_LL5: _tmp20D=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp204)->f2;_LL6: {
# 1141
struct Cyc_List_List*new_ts=((struct Cyc_List_List*(*)(void*(*f)(void*),struct Cyc_List_List*x))Cyc_List_map)(Cyc_Tcutil_rgns_of,_tmp20D);
return Cyc_Tcutil_normalize_effect(Cyc_Absyn_join_eff(new_ts));}}}case 1U: _LL7: _LL8:
 goto _LLA;case 2U: _LL9: _LLA: {
# 1145
struct Cyc_Absyn_Kind*_tmp205=Cyc_Tcutil_type_kind(t);struct Cyc_Absyn_Kind*_tmp206=_tmp205;switch(((struct Cyc_Absyn_Kind*)_tmp206)->kind){case Cyc_Absyn_RgnKind: _LL1E: _LL1F:
 return Cyc_Absyn_access_eff(t);case Cyc_Absyn_EffKind: _LL20: _LL21:
 return t;case Cyc_Absyn_IntKind: _LL22: _LL23:
 return Cyc_Absyn_empty_effect;default: _LL24: _LL25:
 return Cyc_Absyn_regionsof_eff(t);}_LL1D:;}case 3U: _LLB: _tmp20F=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp204)->f1).elt_type;_tmp20E=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp204)->f1).ptr_atts).rgn;_LLC:
# 1153
 return Cyc_Tcutil_normalize_effect(Cyc_Absyn_join_eff(({void*_tmp207[2U];({void*_tmpAAC=Cyc_Absyn_access_eff(_tmp20E);_tmp207[0]=_tmpAAC;}),({void*_tmpAAB=Cyc_Tcutil_rgns_of(_tmp20F);_tmp207[1]=_tmpAAB;});((struct Cyc_List_List*(*)(struct _dyneither_ptr))Cyc_List_list)(_tag_dyneither(_tmp207,sizeof(void*),2U));})));case 4U: _LLD: _tmp210=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp204)->f1).elt_type;_LLE:
# 1155
 return Cyc_Tcutil_normalize_effect(Cyc_Tcutil_rgns_of(_tmp210));case 7U: _LLF: _tmp211=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp204)->f2;_LL10:
# 1157
 return Cyc_Tcutil_normalize_effect(Cyc_Absyn_join_eff(((struct Cyc_List_List*(*)(void*(*f)(struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*x))Cyc_List_map)(Cyc_Tcutil_rgns_of_field,_tmp211)));case 5U: _LL11: _tmp218=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp204)->f1).tvars;_tmp217=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp204)->f1).effect;_tmp216=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp204)->f1).ret_tqual;_tmp215=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp204)->f1).ret_type;_tmp214=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp204)->f1).args;_tmp213=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp204)->f1).cyc_varargs;_tmp212=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp204)->f1).rgn_po;_LL12: {
# 1166
void*_tmp208=({struct Cyc_List_List*_tmpAAD=((struct Cyc_List_List*(*)(struct _tuple15*(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x))Cyc_List_map)(Cyc_Tcutil_region_free_subst,_tmp218);Cyc_Tcutil_substitute(_tmpAAD,(void*)_check_null(_tmp217));});
return Cyc_Tcutil_normalize_effect(_tmp208);}case 6U: _LL13: _tmp219=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp204)->f1;_LL14: {
# 1169
struct Cyc_List_List*_tmp209=0;
for(0;_tmp219 != 0;_tmp219=_tmp219->tl){
_tmp209=({struct Cyc_List_List*_tmp20A=_cycalloc(sizeof(*_tmp20A));_tmp20A->hd=(*((struct _tuple12*)_tmp219->hd)).f2,_tmp20A->tl=_tmp209;_tmp20A;});}
_tmp21A=_tmp209;goto _LL16;}case 8U: _LL15: _tmp21A=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp204)->f2;_LL16:
# 1174
 return Cyc_Tcutil_normalize_effect(Cyc_Absyn_join_eff(((struct Cyc_List_List*(*)(void*(*f)(void*),struct Cyc_List_List*x))Cyc_List_map)(Cyc_Tcutil_rgns_of,_tmp21A)));case 10U: _LL17: _LL18:
({void*_tmp20B=0U;({struct _dyneither_ptr _tmpAAE=({const char*_tmp20C="typedecl in rgns_of";_tag_dyneither(_tmp20C,sizeof(char),20U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpAAE,_tag_dyneither(_tmp20B,sizeof(void*),0U));});});case 9U: _LL19: _LL1A:
 goto _LL1C;default: _LL1B: _LL1C:
 return Cyc_Absyn_empty_effect;}_LL0:;}
# 1184
void*Cyc_Tcutil_normalize_effect(void*e){
e=Cyc_Tcutil_compress(e);{
void*_tmp21B=e;void*_tmp227;struct Cyc_List_List**_tmp226;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp21B)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp21B)->f1)){case 9U: _LL1: _tmp226=(struct Cyc_List_List**)&((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp21B)->f2;_LL2: {
# 1188
int redo_join=0;
{struct Cyc_List_List*effs=*_tmp226;for(0;effs != 0;effs=effs->tl){
void*_tmp21C=(void*)effs->hd;
({void*_tmpAAF=(void*)Cyc_Tcutil_compress(Cyc_Tcutil_normalize_effect(_tmp21C));effs->hd=_tmpAAF;});{
void*_tmp21D=(void*)effs->hd;void*_tmp21E=_tmp21D;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp21E)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp21E)->f1)){case 9U: _LL8: _LL9:
 goto _LLB;case 8U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp21E)->f2 != 0){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)((struct Cyc_List_List*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp21E)->f2)->hd)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)((struct Cyc_List_List*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp21E)->f2)->hd)->f1)){case 5U: if(((struct Cyc_List_List*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp21E)->f2)->tl == 0){_LLA: _LLB:
 goto _LLD;}else{goto _LL10;}case 7U: if(((struct Cyc_List_List*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp21E)->f2)->tl == 0){_LLC: _LLD:
 goto _LLF;}else{goto _LL10;}case 6U: if(((struct Cyc_List_List*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp21E)->f2)->tl == 0){_LLE: _LLF:
# 1197
 redo_join=1;goto _LL7;}else{goto _LL10;}default: goto _LL10;}else{goto _LL10;}}else{goto _LL10;}default: goto _LL10;}else{_LL10: _LL11:
 goto _LL7;}_LL7:;};}}
# 1201
if(!redo_join)return e;{
struct Cyc_List_List*effects=0;
{struct Cyc_List_List*effs=*_tmp226;for(0;effs != 0;effs=effs->tl){
void*_tmp21F=Cyc_Tcutil_compress((void*)effs->hd);void*_tmp220=_tmp21F;void*_tmp223;struct Cyc_List_List*_tmp222;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp220)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp220)->f1)){case 9U: _LL13: _tmp222=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp220)->f2;_LL14:
# 1206
 effects=((struct Cyc_List_List*(*)(struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_revappend)(_tmp222,effects);
goto _LL12;case 8U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp220)->f2 != 0){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)((struct Cyc_List_List*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp220)->f2)->hd)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)((struct Cyc_List_List*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp220)->f2)->hd)->f1)){case 5U: if(((struct Cyc_List_List*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp220)->f2)->tl == 0){_LL15: _LL16:
 goto _LL18;}else{goto _LL1B;}case 7U: if(((struct Cyc_List_List*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp220)->f2)->tl == 0){_LL17: _LL18:
 goto _LL1A;}else{goto _LL1B;}case 6U: if(((struct Cyc_List_List*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp220)->f2)->tl == 0){_LL19: _LL1A:
 goto _LL12;}else{goto _LL1B;}default: goto _LL1B;}else{goto _LL1B;}}else{goto _LL1B;}default: goto _LL1B;}else{_LL1B: _tmp223=_tmp220;_LL1C:
 effects=({struct Cyc_List_List*_tmp221=_cycalloc(sizeof(*_tmp221));_tmp221->hd=_tmp223,_tmp221->tl=effects;_tmp221;});goto _LL12;}_LL12:;}}
# 1214
({struct Cyc_List_List*_tmpAB0=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(effects);*_tmp226=_tmpAB0;});
return e;};}case 10U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp21B)->f2 != 0){_LL3: _tmp227=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp21B)->f2)->hd;_LL4: {
# 1217
void*_tmp224=Cyc_Tcutil_compress(_tmp227);void*_tmp225=_tmp224;switch(*((int*)_tmp225)){case 1U: _LL1E: _LL1F:
 goto _LL21;case 2U: _LL20: _LL21:
 return e;default: _LL22: _LL23:
 return Cyc_Tcutil_rgns_of(_tmp227);}_LL1D:;}}else{goto _LL5;}default: goto _LL5;}else{_LL5: _LL6:
# 1222
 return e;}_LL0:;};}
# 1227
static void*Cyc_Tcutil_dummy_fntype(void*eff){
struct Cyc_Absyn_FnType_Absyn_Type_struct*_tmp228=({struct Cyc_Absyn_FnType_Absyn_Type_struct*_tmp229=_cycalloc(sizeof(*_tmp229));_tmp229->tag=5U,(_tmp229->f1).tvars=0,(_tmp229->f1).effect=eff,({
struct Cyc_Absyn_Tqual _tmpAB1=Cyc_Absyn_empty_tqual(0U);(_tmp229->f1).ret_tqual=_tmpAB1;}),(_tmp229->f1).ret_type=Cyc_Absyn_void_type,(_tmp229->f1).args=0,(_tmp229->f1).c_varargs=0,(_tmp229->f1).cyc_varargs=0,(_tmp229->f1).rgn_po=0,(_tmp229->f1).attributes=0,(_tmp229->f1).requires_clause=0,(_tmp229->f1).requires_relns=0,(_tmp229->f1).ensures_clause=0,(_tmp229->f1).ensures_relns=0;_tmp229;});
# 1237
return({void*_tmpAB5=(void*)_tmp228;void*_tmpAB4=Cyc_Absyn_heap_rgn_type;struct Cyc_Absyn_Tqual _tmpAB3=Cyc_Absyn_empty_tqual(0U);void*_tmpAB2=Cyc_Absyn_bounds_one();Cyc_Absyn_atb_type(_tmpAB5,_tmpAB4,_tmpAB3,_tmpAB2,Cyc_Absyn_false_type);});}
# 1244
int Cyc_Tcutil_region_in_effect(int constrain,void*r,void*e){
r=Cyc_Tcutil_compress(r);
if((r == Cyc_Absyn_heap_rgn_type  || r == Cyc_Absyn_unique_rgn_type) || r == Cyc_Absyn_refcnt_rgn_type)
return 1;{
void*_tmp22A=Cyc_Tcutil_compress(e);void*_tmp22B=_tmp22A;struct Cyc_Core_Opt*_tmp246;void**_tmp245;struct Cyc_Core_Opt*_tmp244;void*_tmp243;struct Cyc_List_List*_tmp242;void*_tmp241;switch(*((int*)_tmp22B)){case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp22B)->f1)){case 8U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp22B)->f2 != 0){_LL1: _tmp241=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp22B)->f2)->hd;_LL2:
# 1251
 if(constrain)return Cyc_Tcutil_unify(r,_tmp241);
_tmp241=Cyc_Tcutil_compress(_tmp241);
if(r == _tmp241)return 1;{
struct _tuple0 _tmp22C=({struct _tuple0 _tmp98A;_tmp98A.f1=r,_tmp98A.f2=_tmp241;_tmp98A;});struct _tuple0 _tmp22D=_tmp22C;struct Cyc_Absyn_Tvar*_tmp22F;struct Cyc_Absyn_Tvar*_tmp22E;if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp22D.f1)->tag == 2U){if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp22D.f2)->tag == 2U){_LLC: _tmp22F=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp22D.f1)->f1;_tmp22E=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp22D.f2)->f1;_LLD:
 return Cyc_Absyn_tvar_cmp(_tmp22F,_tmp22E)== 0;}else{goto _LLE;}}else{_LLE: _LLF:
 return 0;}_LLB:;};}else{goto _LL9;}case 9U: _LL3: _tmp242=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp22B)->f2;_LL4:
# 1259
 for(0;_tmp242 != 0;_tmp242=_tmp242->tl){
if(Cyc_Tcutil_region_in_effect(constrain,r,(void*)_tmp242->hd))return 1;}
return 0;case 10U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp22B)->f2 != 0){_LL5: _tmp243=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp22B)->f2)->hd;_LL6: {
# 1263
void*_tmp230=Cyc_Tcutil_rgns_of(_tmp243);void*_tmp231=_tmp230;void*_tmp23B;void*_tmp23A;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp231)->tag == 0U){if(((struct Cyc_Absyn_RgnsCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp231)->f1)->tag == 10U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp231)->f2 != 0){_LL11: _tmp23A=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp231)->f2)->hd;_LL12:
# 1265
 if(!constrain)return 0;{
void*_tmp232=Cyc_Tcutil_compress(_tmp23A);void*_tmp233=_tmp232;struct Cyc_Core_Opt*_tmp239;void**_tmp238;struct Cyc_Core_Opt*_tmp237;if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp233)->tag == 1U){_LL16: _tmp239=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp233)->f1;_tmp238=(void**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp233)->f2;_tmp237=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp233)->f4;_LL17: {
# 1270
void*_tmp234=Cyc_Absyn_new_evar(& Cyc_Tcutil_eko,_tmp237);
# 1273
Cyc_Tcutil_occurs(_tmp234,Cyc_Core_heap_region,(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(_tmp237))->v,r);{
void*_tmp235=Cyc_Tcutil_dummy_fntype(Cyc_Absyn_join_eff(({void*_tmp236[2U];_tmp236[0]=_tmp234,({void*_tmpAB6=Cyc_Absyn_access_eff(r);_tmp236[1]=_tmpAB6;});((struct Cyc_List_List*(*)(struct _dyneither_ptr))Cyc_List_list)(_tag_dyneither(_tmp236,sizeof(void*),2U));})));
*_tmp238=_tmp235;
return 1;};}}else{_LL18: _LL19:
 return 0;}_LL15:;};}else{goto _LL13;}}else{goto _LL13;}}else{_LL13: _tmp23B=_tmp231;_LL14:
# 1279
 return Cyc_Tcutil_region_in_effect(constrain,r,_tmp23B);}_LL10:;}}else{goto _LL9;}default: goto _LL9;}case 1U: _LL7: _tmp246=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp22B)->f1;_tmp245=(void**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp22B)->f2;_tmp244=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp22B)->f4;_LL8:
# 1282
 if(_tmp246 == 0  || (int)((struct Cyc_Absyn_Kind*)_tmp246->v)->kind != (int)Cyc_Absyn_EffKind)
({void*_tmp23C=0U;({struct _dyneither_ptr _tmpAB7=({const char*_tmp23D="effect evar has wrong kind";_tag_dyneither(_tmp23D,sizeof(char),27U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpAB7,_tag_dyneither(_tmp23C,sizeof(void*),0U));});});
if(!constrain)return 0;{
# 1287
void*_tmp23E=Cyc_Absyn_new_evar(& Cyc_Tcutil_eko,_tmp244);
# 1290
Cyc_Tcutil_occurs(_tmp23E,Cyc_Core_heap_region,(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(_tmp244))->v,r);{
void*_tmp23F=Cyc_Absyn_join_eff(({void*_tmp240[2U];_tmp240[0]=_tmp23E,({void*_tmpAB8=Cyc_Absyn_access_eff(r);_tmp240[1]=_tmpAB8;});((struct Cyc_List_List*(*)(struct _dyneither_ptr))Cyc_List_list)(_tag_dyneither(_tmp240,sizeof(void*),2U));}));
*_tmp245=_tmp23F;
return 1;};};default: _LL9: _LLA:
 return 0;}_LL0:;};}
# 1301
static int Cyc_Tcutil_type_in_effect(int may_constrain_evars,void*t,void*e){
t=Cyc_Tcutil_compress(t);{
void*_tmp247=Cyc_Tcutil_normalize_effect(Cyc_Tcutil_compress(e));void*_tmp248=_tmp247;struct Cyc_Core_Opt*_tmp25A;void**_tmp259;struct Cyc_Core_Opt*_tmp258;void*_tmp257;struct Cyc_List_List*_tmp256;switch(*((int*)_tmp248)){case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp248)->f1)){case 8U: _LL1: _LL2:
 return 0;case 9U: _LL3: _tmp256=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp248)->f2;_LL4:
# 1306
 for(0;_tmp256 != 0;_tmp256=_tmp256->tl){
if(Cyc_Tcutil_type_in_effect(may_constrain_evars,t,(void*)_tmp256->hd))
return 1;}
return 0;case 10U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp248)->f2 != 0){_LL5: _tmp257=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp248)->f2)->hd;_LL6:
# 1311
 _tmp257=Cyc_Tcutil_compress(_tmp257);
if(t == _tmp257)return 1;
if(may_constrain_evars)return Cyc_Tcutil_unify(t,_tmp257);{
void*_tmp249=Cyc_Tcutil_rgns_of(t);void*_tmp24A=_tmp249;void*_tmp250;void*_tmp24F;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp24A)->tag == 0U){if(((struct Cyc_Absyn_RgnsCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp24A)->f1)->tag == 10U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp24A)->f2 != 0){_LLC: _tmp24F=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp24A)->f2)->hd;_LLD: {
# 1316
struct _tuple0 _tmp24B=({struct _tuple0 _tmp98B;({void*_tmpAB9=Cyc_Tcutil_compress(_tmp24F);_tmp98B.f1=_tmpAB9;}),_tmp98B.f2=_tmp257;_tmp98B;});struct _tuple0 _tmp24C=_tmp24B;struct Cyc_Absyn_Tvar*_tmp24E;struct Cyc_Absyn_Tvar*_tmp24D;if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp24C.f1)->tag == 2U){if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp24C.f2)->tag == 2U){_LL11: _tmp24E=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp24C.f1)->f1;_tmp24D=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp24C.f2)->f1;_LL12:
 return Cyc_Tcutil_unify(t,_tmp257);}else{goto _LL13;}}else{_LL13: _LL14:
 return _tmp24F == _tmp257;}_LL10:;}}else{goto _LLE;}}else{goto _LLE;}}else{_LLE: _tmp250=_tmp24A;_LLF:
# 1320
 return Cyc_Tcutil_type_in_effect(may_constrain_evars,t,_tmp250);}_LLB:;};}else{goto _LL9;}default: goto _LL9;}case 1U: _LL7: _tmp25A=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp248)->f1;_tmp259=(void**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp248)->f2;_tmp258=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp248)->f4;_LL8:
# 1323
 if(_tmp25A == 0  || (int)((struct Cyc_Absyn_Kind*)_tmp25A->v)->kind != (int)Cyc_Absyn_EffKind)
({void*_tmp251=0U;({struct _dyneither_ptr _tmpABA=({const char*_tmp252="effect evar has wrong kind";_tag_dyneither(_tmp252,sizeof(char),27U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpABA,_tag_dyneither(_tmp251,sizeof(void*),0U));});});
if(!may_constrain_evars)return 0;{
# 1328
void*_tmp253=Cyc_Absyn_new_evar(& Cyc_Tcutil_eko,_tmp258);
# 1331
Cyc_Tcutil_occurs(_tmp253,Cyc_Core_heap_region,(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(_tmp258))->v,t);{
void*_tmp254=Cyc_Absyn_join_eff(({void*_tmp255[2U];_tmp255[0]=_tmp253,({void*_tmpABB=Cyc_Absyn_regionsof_eff(t);_tmp255[1]=_tmpABB;});((struct Cyc_List_List*(*)(struct _dyneither_ptr))Cyc_List_list)(_tag_dyneither(_tmp255,sizeof(void*),2U));}));
*_tmp259=_tmp254;
return 1;};};default: _LL9: _LLA:
 return 0;}_LL0:;};}
# 1342
static int Cyc_Tcutil_variable_in_effect(int may_constrain_evars,struct Cyc_Absyn_Tvar*v,void*e){
e=Cyc_Tcutil_compress(e);{
void*_tmp25B=e;struct Cyc_Core_Opt*_tmp272;void**_tmp271;struct Cyc_Core_Opt*_tmp270;void*_tmp26F;struct Cyc_List_List*_tmp26E;struct Cyc_Absyn_Tvar*_tmp26D;switch(*((int*)_tmp25B)){case 2U: _LL1: _tmp26D=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp25B)->f1;_LL2:
 return Cyc_Absyn_tvar_cmp(v,_tmp26D)== 0;case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp25B)->f1)){case 9U: _LL3: _tmp26E=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp25B)->f2;_LL4:
# 1347
 for(0;_tmp26E != 0;_tmp26E=_tmp26E->tl){
if(Cyc_Tcutil_variable_in_effect(may_constrain_evars,v,(void*)_tmp26E->hd))
return 1;}
return 0;case 10U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp25B)->f2 != 0){_LL5: _tmp26F=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp25B)->f2)->hd;_LL6: {
# 1352
void*_tmp25C=Cyc_Tcutil_rgns_of(_tmp26F);void*_tmp25D=_tmp25C;void*_tmp267;void*_tmp266;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp25D)->tag == 0U){if(((struct Cyc_Absyn_RgnsCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp25D)->f1)->tag == 10U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp25D)->f2 != 0){_LLC: _tmp266=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp25D)->f2)->hd;_LLD:
# 1354
 if(!may_constrain_evars)return 0;{
void*_tmp25E=Cyc_Tcutil_compress(_tmp266);void*_tmp25F=_tmp25E;struct Cyc_Core_Opt*_tmp265;void**_tmp264;struct Cyc_Core_Opt*_tmp263;if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp25F)->tag == 1U){_LL11: _tmp265=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp25F)->f1;_tmp264=(void**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp25F)->f2;_tmp263=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp25F)->f4;_LL12: {
# 1360
void*_tmp260=Cyc_Absyn_new_evar(& Cyc_Tcutil_eko,_tmp263);
# 1362
if(!((int(*)(int(*compare)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct Cyc_Absyn_Tvar*x))Cyc_List_mem)(Cyc_Tcutil_fast_tvar_cmp,(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(_tmp263))->v,v))return 0;{
void*_tmp261=Cyc_Tcutil_dummy_fntype(Cyc_Absyn_join_eff(({void*_tmp262[2U];_tmp262[0]=_tmp260,({void*_tmpABC=Cyc_Absyn_var_type(v);_tmp262[1]=_tmpABC;});((struct Cyc_List_List*(*)(struct _dyneither_ptr))Cyc_List_list)(_tag_dyneither(_tmp262,sizeof(void*),2U));})));
*_tmp264=_tmp261;
return 1;};}}else{_LL13: _LL14:
 return 0;}_LL10:;};}else{goto _LLE;}}else{goto _LLE;}}else{_LLE: _tmp267=_tmp25D;_LLF:
# 1368
 return Cyc_Tcutil_variable_in_effect(may_constrain_evars,v,_tmp267);}_LLB:;}}else{goto _LL9;}default: goto _LL9;}case 1U: _LL7: _tmp272=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp25B)->f1;_tmp271=(void**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp25B)->f2;_tmp270=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp25B)->f4;_LL8:
# 1371
 if(_tmp272 == 0  || (int)((struct Cyc_Absyn_Kind*)_tmp272->v)->kind != (int)Cyc_Absyn_EffKind)
({void*_tmp268=0U;({struct _dyneither_ptr _tmpABD=({const char*_tmp269="effect evar has wrong kind";_tag_dyneither(_tmp269,sizeof(char),27U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpABD,_tag_dyneither(_tmp268,sizeof(void*),0U));});});{
# 1375
void*_tmp26A=Cyc_Absyn_new_evar(& Cyc_Tcutil_eko,_tmp270);
# 1377
if(!((int(*)(int(*compare)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct Cyc_Absyn_Tvar*x))Cyc_List_mem)(Cyc_Tcutil_fast_tvar_cmp,(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(_tmp270))->v,v))
return 0;{
void*_tmp26B=Cyc_Absyn_join_eff(({void*_tmp26C[2U];_tmp26C[0]=_tmp26A,({void*_tmpABE=Cyc_Absyn_var_type(v);_tmp26C[1]=_tmpABE;});((struct Cyc_List_List*(*)(struct _dyneither_ptr))Cyc_List_list)(_tag_dyneither(_tmp26C,sizeof(void*),2U));}));
*_tmp271=_tmp26B;
return 1;};};default: _LL9: _LLA:
 return 0;}_LL0:;};}
# 1387
static int Cyc_Tcutil_evar_in_effect(void*evar,void*e){
e=Cyc_Tcutil_compress(e);{
void*_tmp273=e;void*_tmp279;struct Cyc_List_List*_tmp278;switch(*((int*)_tmp273)){case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp273)->f1)){case 9U: _LL1: _tmp278=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp273)->f2;_LL2:
# 1391
 for(0;_tmp278 != 0;_tmp278=_tmp278->tl){
if(Cyc_Tcutil_evar_in_effect(evar,(void*)_tmp278->hd))
return 1;}
return 0;case 10U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp273)->f2 != 0){_LL3: _tmp279=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp273)->f2)->hd;_LL4: {
# 1396
void*_tmp274=Cyc_Tcutil_rgns_of(_tmp279);void*_tmp275=_tmp274;void*_tmp277;void*_tmp276;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp275)->tag == 0U){if(((struct Cyc_Absyn_RgnsCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp275)->f1)->tag == 10U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp275)->f2 != 0){_LLA: _tmp276=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp275)->f2)->hd;_LLB:
 return 0;}else{goto _LLC;}}else{goto _LLC;}}else{_LLC: _tmp277=_tmp275;_LLD:
 return Cyc_Tcutil_evar_in_effect(evar,_tmp277);}_LL9:;}}else{goto _LL7;}default: goto _LL7;}case 1U: _LL5: _LL6:
# 1400
 return evar == e;default: _LL7: _LL8:
 return 0;}_LL0:;};}
# 1414 "tcutil.cyc"
int Cyc_Tcutil_subset_effect(int may_constrain_evars,void*e1,void*e2){
# 1419
void*_tmp27A=Cyc_Tcutil_compress(e1);void*_tmp27B=_tmp27A;void**_tmp288;struct Cyc_Core_Opt*_tmp287;struct Cyc_Absyn_Tvar*_tmp286;void*_tmp285;void*_tmp284;struct Cyc_List_List*_tmp283;switch(*((int*)_tmp27B)){case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp27B)->f1)){case 9U: _LL1: _tmp283=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp27B)->f2;_LL2:
# 1421
 for(0;_tmp283 != 0;_tmp283=_tmp283->tl){
if(!Cyc_Tcutil_subset_effect(may_constrain_evars,(void*)_tmp283->hd,e2))
return 0;}
return 1;case 8U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp27B)->f2 != 0){_LL3: _tmp284=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp27B)->f2)->hd;_LL4:
# 1432
 return Cyc_Tcutil_region_in_effect(may_constrain_evars,_tmp284,e2) || 
may_constrain_evars  && Cyc_Tcutil_unify(_tmp284,Cyc_Absyn_heap_rgn_type);}else{goto _LLB;}case 10U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp27B)->f2 != 0){_LL7: _tmp285=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp27B)->f2)->hd;_LL8: {
# 1436
void*_tmp27C=Cyc_Tcutil_rgns_of(_tmp285);void*_tmp27D=_tmp27C;void*_tmp27F;void*_tmp27E;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp27D)->tag == 0U){if(((struct Cyc_Absyn_RgnsCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp27D)->f1)->tag == 10U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp27D)->f2 != 0){_LLE: _tmp27E=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp27D)->f2)->hd;_LLF:
# 1441
 return Cyc_Tcutil_type_in_effect(may_constrain_evars,_tmp27E,e2) || 
may_constrain_evars  && Cyc_Tcutil_unify(_tmp27E,Cyc_Absyn_sint_type);}else{goto _LL10;}}else{goto _LL10;}}else{_LL10: _tmp27F=_tmp27D;_LL11:
 return Cyc_Tcutil_subset_effect(may_constrain_evars,_tmp27F,e2);}_LLD:;}}else{goto _LLB;}default: goto _LLB;}case 2U: _LL5: _tmp286=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp27B)->f1;_LL6:
# 1434
 return Cyc_Tcutil_variable_in_effect(may_constrain_evars,_tmp286,e2);case 1U: _LL9: _tmp288=(void**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp27B)->f2;_tmp287=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp27B)->f4;_LLA:
# 1446
 if(!Cyc_Tcutil_evar_in_effect(e1,e2)){
# 1450
*_tmp288=Cyc_Absyn_empty_effect;
# 1453
return 1;}else{
# 1455
return 0;}default: _LLB: _LLC:
({struct Cyc_String_pa_PrintArg_struct _tmp282=({struct Cyc_String_pa_PrintArg_struct _tmp98C;_tmp98C.tag=0U,({struct _dyneither_ptr _tmpABF=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(e1));_tmp98C.f1=_tmpABF;});_tmp98C;});void*_tmp280[1U];_tmp280[0]=& _tmp282;({struct _dyneither_ptr _tmpAC0=({const char*_tmp281="subset_effect: bad effect: %s";_tag_dyneither(_tmp281,sizeof(char),30U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpAC0,_tag_dyneither(_tmp280,sizeof(void*),1U));});});}_LL0:;}
# 1471 "tcutil.cyc"
static int Cyc_Tcutil_unify_effect(void*e1,void*e2){
e1=Cyc_Tcutil_normalize_effect(e1);
e2=Cyc_Tcutil_normalize_effect(e2);
if(Cyc_Tcutil_subset_effect(0,e1,e2) && Cyc_Tcutil_subset_effect(0,e2,e1))
return 1;
if(Cyc_Tcutil_subset_effect(1,e1,e2) && Cyc_Tcutil_subset_effect(1,e2,e1))
return 1;
return 0;}
# 1487
static int Cyc_Tcutil_sub_rgnpo(struct Cyc_List_List*rpo1,struct Cyc_List_List*rpo2){
# 1489
{struct Cyc_List_List*r1=rpo1;for(0;r1 != 0;r1=r1->tl){
struct _tuple0*_tmp289=(struct _tuple0*)r1->hd;struct _tuple0*_tmp28A=_tmp289;void*_tmp290;void*_tmp28F;_LL1: _tmp290=_tmp28A->f1;_tmp28F=_tmp28A->f2;_LL2:;{
int found=_tmp290 == Cyc_Absyn_heap_rgn_type;
{struct Cyc_List_List*r2=rpo2;for(0;r2 != 0  && !found;r2=r2->tl){
struct _tuple0*_tmp28B=(struct _tuple0*)r2->hd;struct _tuple0*_tmp28C=_tmp28B;void*_tmp28E;void*_tmp28D;_LL4: _tmp28E=_tmp28C->f1;_tmp28D=_tmp28C->f2;_LL5:;
if(Cyc_Tcutil_unify(_tmp290,_tmp28E) && Cyc_Tcutil_unify(_tmp28F,_tmp28D)){
found=1;
break;}}}
# 1499
if(!found)return 0;};}}
# 1501
return 1;}
# 1508
static int Cyc_Tcutil_check_logical_implication(struct Cyc_List_List*r1,struct Cyc_List_List*r2){
for(0;r2 != 0;r2=r2->tl){
struct Cyc_Relations_Reln*_tmp291=Cyc_Relations_negate(Cyc_Core_heap_region,(struct Cyc_Relations_Reln*)r2->hd);
struct Cyc_List_List*_tmp292=({struct Cyc_List_List*_tmp293=_cycalloc(sizeof(*_tmp293));_tmp293->hd=_tmp291,_tmp293->tl=r1;_tmp293;});
if(Cyc_Relations_consistent_relations(_tmp292))return 0;}
# 1514
return 1;}
# 1519
static int Cyc_Tcutil_check_logical_equivalence(struct Cyc_List_List*r1,struct Cyc_List_List*r2){
if(r1 == r2)return 1;
return Cyc_Tcutil_check_logical_implication(r1,r2) && Cyc_Tcutil_check_logical_implication(r2,r1);}
# 1525
static int Cyc_Tcutil_same_rgn_po(struct Cyc_List_List*rpo1,struct Cyc_List_List*rpo2){
# 1527
return Cyc_Tcutil_sub_rgnpo(rpo1,rpo2) && Cyc_Tcutil_sub_rgnpo(rpo2,rpo1);}
# 1530
int Cyc_Tcutil_tycon2int(void*t){
void*_tmp294=t;switch(*((int*)_tmp294)){case 0U: _LL1: _LL2:
 return 0;case 1U: switch(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp294)->f1){case Cyc_Absyn_Unsigned: switch(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp294)->f2){case Cyc_Absyn_Char_sz: _LL3: _LL4:
 return 1;case Cyc_Absyn_Short_sz: _LL9: _LLA:
# 1536
 return 4;case Cyc_Absyn_Int_sz: _LLF: _LL10:
# 1539
 return 7;case Cyc_Absyn_Long_sz: _LL15: _LL16:
# 1542
 return 7;default: _LL1B: _LL1C:
# 1545
 return 13;}case Cyc_Absyn_Signed: switch(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp294)->f2){case Cyc_Absyn_Char_sz: _LL5: _LL6:
# 1534
 return 2;case Cyc_Absyn_Short_sz: _LLB: _LLC:
# 1537
 return 5;case Cyc_Absyn_Int_sz: _LL11: _LL12:
# 1540
 return 8;case Cyc_Absyn_Long_sz: _LL17: _LL18:
# 1543
 return 8;default: _LL1D: _LL1E:
# 1546
 return 14;}default: switch(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp294)->f2){case Cyc_Absyn_Char_sz: _LL7: _LL8:
# 1535
 return 3;case Cyc_Absyn_Short_sz: _LLD: _LLE:
# 1538
 return 6;case Cyc_Absyn_Int_sz: _LL13: _LL14:
# 1541
 return 9;case Cyc_Absyn_Long_sz: _LL19: _LL1A:
# 1544
 return 9;default: _LL1F: _LL20:
# 1547
 return 15;}}case 2U: switch(((struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)_tmp294)->f1){case 0U: _LL21: _LL22:
 return 16;case 1U: _LL23: _LL24:
 return 17;default: _LL25: _LL26:
 return 18;}case 3U: _LL27: _LL28:
 return 19;case 4U: _LL29: _LL2A:
 return 20;case 5U: _LL2B: _LL2C:
 return 21;case 6U: _LL2D: _LL2E:
 return 22;case 7U: _LL2F: _LL30:
 return 23;case 8U: _LL31: _LL32:
 return 24;case 9U: _LL33: _LL34:
 return 25;case 10U: _LL35: _LL36:
 return 26;case 11U: _LL37: _LL38:
 return 27;case 12U: _LL39: _LL3A:
 return 28;case 14U: _LL3B: _LL3C:
 return 29;case 13U: _LL3D: _LL3E:
 return 30;case 15U: _LL3F: _LL40:
 return 31;case 16U: _LL41: _LL42:
 return 32;case 17U: _LL43: _LL44:
 return 33;case 18U: _LL45: _LL46:
 return 34;case 19U: _LL47: _LL48:
 return 35;default: _LL49: _LL4A:
 return 36;}_LL0:;}
# 1572
static int Cyc_Tcutil_enumfield_cmp(struct Cyc_Absyn_Enumfield*e1,struct Cyc_Absyn_Enumfield*e2);
# 1574
struct _tuple2*Cyc_Tcutil_get_datatype_qvar(union Cyc_Absyn_DatatypeInfo i){
union Cyc_Absyn_DatatypeInfo _tmp295=i;struct _tuple2*_tmp297;struct Cyc_Absyn_Datatypedecl*_tmp296;if((_tmp295.KnownDatatype).tag == 2){_LL1: _tmp296=*(_tmp295.KnownDatatype).val;_LL2:
 return _tmp296->name;}else{_LL3: _tmp297=((_tmp295.UnknownDatatype).val).name;_LL4:
 return _tmp297;}_LL0:;}struct _tuple21{struct _tuple2*f1;struct _tuple2*f2;};
# 1581
struct _tuple21 Cyc_Tcutil_get_datatype_field_qvars(union Cyc_Absyn_DatatypeFieldInfo i){
union Cyc_Absyn_DatatypeFieldInfo _tmp298=i;struct _tuple2*_tmp29C;struct _tuple2*_tmp29B;struct Cyc_Absyn_Datatypedecl*_tmp29A;struct Cyc_Absyn_Datatypefield*_tmp299;if((_tmp298.KnownDatatypefield).tag == 2){_LL1: _tmp29A=((_tmp298.KnownDatatypefield).val).f1;_tmp299=((_tmp298.KnownDatatypefield).val).f2;_LL2:
# 1584
 return({struct _tuple21 _tmp98D;_tmp98D.f1=_tmp29A->name,_tmp98D.f2=_tmp299->name;_tmp98D;});}else{_LL3: _tmp29C=((_tmp298.UnknownDatatypefield).val).datatype_name;_tmp29B=((_tmp298.UnknownDatatypefield).val).field_name;_LL4:
# 1586
 return({struct _tuple21 _tmp98E;_tmp98E.f1=_tmp29C,_tmp98E.f2=_tmp29B;_tmp98E;});}_LL0:;}struct _tuple22{enum Cyc_Absyn_AggrKind f1;struct _tuple2*f2;};
# 1590
struct _tuple22 Cyc_Tcutil_get_aggr_kind_and_qvar(union Cyc_Absyn_AggrInfo i){
union Cyc_Absyn_AggrInfo _tmp29D=i;struct Cyc_Absyn_Aggrdecl*_tmp2A0;enum Cyc_Absyn_AggrKind _tmp29F;struct _tuple2*_tmp29E;if((_tmp29D.UnknownAggr).tag == 1){_LL1: _tmp29F=((_tmp29D.UnknownAggr).val).f1;_tmp29E=((_tmp29D.UnknownAggr).val).f2;_LL2:
 return({struct _tuple22 _tmp98F;_tmp98F.f1=_tmp29F,_tmp98F.f2=_tmp29E;_tmp98F;});}else{_LL3: _tmp2A0=*(_tmp29D.KnownAggr).val;_LL4:
 return({struct _tuple22 _tmp990;_tmp990.f1=_tmp2A0->kind,_tmp990.f2=_tmp2A0->name;_tmp990;});}_LL0:;}
# 1597
int Cyc_Tcutil_tycon_cmp(void*t1,void*t2){
if(t1 == t2)return 0;{
int i1=Cyc_Tcutil_tycon2int(t1);
int i2=Cyc_Tcutil_tycon2int(t2);
if(i1 != i2)return i1 - i2;{
# 1603
struct _tuple0 _tmp2A1=({struct _tuple0 _tmp991;_tmp991.f1=t1,_tmp991.f2=t2;_tmp991;});struct _tuple0 _tmp2A2=_tmp2A1;union Cyc_Absyn_AggrInfo _tmp2C0;union Cyc_Absyn_AggrInfo _tmp2BF;union Cyc_Absyn_DatatypeFieldInfo _tmp2BE;union Cyc_Absyn_DatatypeFieldInfo _tmp2BD;union Cyc_Absyn_DatatypeInfo _tmp2BC;union Cyc_Absyn_DatatypeInfo _tmp2BB;struct Cyc_List_List*_tmp2BA;struct Cyc_List_List*_tmp2B9;struct _dyneither_ptr _tmp2B8;struct _dyneither_ptr _tmp2B7;struct _tuple2*_tmp2B6;struct _tuple2*_tmp2B5;switch(*((int*)_tmp2A2.f1)){case 15U: if(((struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*)_tmp2A2.f2)->tag == 15U){_LL1: _tmp2B6=((struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*)_tmp2A2.f1)->f1;_tmp2B5=((struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*)_tmp2A2.f2)->f1;_LL2:
 return Cyc_Absyn_qvar_cmp(_tmp2B6,_tmp2B5);}else{goto _LLD;}case 17U: if(((struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct*)_tmp2A2.f2)->tag == 17U){_LL3: _tmp2B8=((struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct*)_tmp2A2.f1)->f1;_tmp2B7=((struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct*)_tmp2A2.f2)->f1;_LL4:
 return Cyc_strcmp((struct _dyneither_ptr)_tmp2B8,(struct _dyneither_ptr)_tmp2B7);}else{goto _LLD;}case 16U: if(((struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct*)_tmp2A2.f2)->tag == 16U){_LL5: _tmp2BA=((struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct*)_tmp2A2.f1)->f1;_tmp2B9=((struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct*)_tmp2A2.f2)->f1;_LL6:
# 1607
 return((int(*)(int(*cmp)(struct Cyc_Absyn_Enumfield*,struct Cyc_Absyn_Enumfield*),struct Cyc_List_List*l1,struct Cyc_List_List*l2))Cyc_List_list_cmp)(Cyc_Tcutil_enumfield_cmp,_tmp2BA,_tmp2B9);}else{goto _LLD;}case 18U: if(((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)_tmp2A2.f2)->tag == 18U){_LL7: _tmp2BC=((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)_tmp2A2.f1)->f1;_tmp2BB=((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)_tmp2A2.f2)->f1;_LL8: {
# 1609
struct _tuple2*q1=Cyc_Tcutil_get_datatype_qvar(_tmp2BC);
struct _tuple2*q2=Cyc_Tcutil_get_datatype_qvar(_tmp2BB);
return Cyc_Absyn_qvar_cmp(q1,q2);}}else{goto _LLD;}case 19U: if(((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)_tmp2A2.f2)->tag == 19U){_LL9: _tmp2BE=((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)_tmp2A2.f1)->f1;_tmp2BD=((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)_tmp2A2.f2)->f1;_LLA: {
# 1613
struct _tuple21 _tmp2A3=Cyc_Tcutil_get_datatype_field_qvars(_tmp2BE);struct _tuple21 _tmp2A4=_tmp2A3;struct _tuple2*_tmp2AB;struct _tuple2*_tmp2AA;_LL10: _tmp2AB=_tmp2A4.f1;_tmp2AA=_tmp2A4.f2;_LL11:;{
struct _tuple21 _tmp2A5=Cyc_Tcutil_get_datatype_field_qvars(_tmp2BD);struct _tuple21 _tmp2A6=_tmp2A5;struct _tuple2*_tmp2A9;struct _tuple2*_tmp2A8;_LL13: _tmp2A9=_tmp2A6.f1;_tmp2A8=_tmp2A6.f2;_LL14:;{
int _tmp2A7=Cyc_Absyn_qvar_cmp(_tmp2AB,_tmp2A9);
if(_tmp2A7 != 0)return _tmp2A7;
return Cyc_Absyn_qvar_cmp(_tmp2AA,_tmp2A8);};};}}else{goto _LLD;}case 20U: if(((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_tmp2A2.f2)->tag == 20U){_LLB: _tmp2C0=((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_tmp2A2.f1)->f1;_tmp2BF=((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_tmp2A2.f2)->f1;_LLC: {
# 1619
struct _tuple22 _tmp2AC=Cyc_Tcutil_get_aggr_kind_and_qvar(_tmp2C0);struct _tuple22 _tmp2AD=_tmp2AC;enum Cyc_Absyn_AggrKind _tmp2B4;struct _tuple2*_tmp2B3;_LL16: _tmp2B4=_tmp2AD.f1;_tmp2B3=_tmp2AD.f2;_LL17:;{
struct _tuple22 _tmp2AE=Cyc_Tcutil_get_aggr_kind_and_qvar(_tmp2BF);struct _tuple22 _tmp2AF=_tmp2AE;enum Cyc_Absyn_AggrKind _tmp2B2;struct _tuple2*_tmp2B1;_LL19: _tmp2B2=_tmp2AF.f1;_tmp2B1=_tmp2AF.f2;_LL1A:;{
int _tmp2B0=Cyc_Absyn_qvar_cmp(_tmp2B3,_tmp2B1);
if(_tmp2B0 != 0)return _tmp2B0;
return(int)_tmp2B4 - (int)_tmp2B2;};};}}else{goto _LLD;}default: _LLD: _LLE:
 return 0;}_LL0:;};};}struct _tuple23{struct Cyc_Absyn_VarargInfo*f1;struct Cyc_Absyn_VarargInfo*f2;};
# 1629
void Cyc_Tcutil_unify_it(void*t1,void*t2){
Cyc_Tcutil_t1_failure=t1;
Cyc_Tcutil_t2_failure=t2;
Cyc_Tcutil_failure_reason=_tag_dyneither(0,0,0);
t1=Cyc_Tcutil_compress(t1);
t2=Cyc_Tcutil_compress(t2);
if(t1 == t2)return;
{void*_tmp2C1=t1;struct Cyc_Core_Opt*_tmp2CF;void**_tmp2CE;struct Cyc_Core_Opt*_tmp2CD;if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp2C1)->tag == 1U){_LL1: _tmp2CF=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp2C1)->f1;_tmp2CE=(void**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp2C1)->f2;_tmp2CD=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp2C1)->f4;_LL2:
# 1640
 Cyc_Tcutil_occurs(t1,Cyc_Core_heap_region,(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(_tmp2CD))->v,t2);{
struct Cyc_Absyn_Kind*_tmp2C2=Cyc_Tcutil_type_kind(t2);
# 1645
if(Cyc_Tcutil_kind_leq(_tmp2C2,(struct Cyc_Absyn_Kind*)((struct Cyc_Core_Opt*)_check_null(_tmp2CF))->v)){
*_tmp2CE=t2;
return;}else{
# 1649
{void*_tmp2C3=t2;struct Cyc_Absyn_PtrInfo _tmp2CB;void**_tmp2CA;struct Cyc_Core_Opt*_tmp2C9;switch(*((int*)_tmp2C3)){case 1U: _LL6: _tmp2CA=(void**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp2C3)->f2;_tmp2C9=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp2C3)->f4;_LL7: {
# 1652
struct Cyc_List_List*_tmp2C4=(struct Cyc_List_List*)_tmp2CD->v;
{struct Cyc_List_List*s2=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(_tmp2C9))->v;for(0;s2 != 0;s2=s2->tl){
if(!((int(*)(int(*compare)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct Cyc_Absyn_Tvar*x))Cyc_List_mem)(Cyc_Tcutil_fast_tvar_cmp,_tmp2C4,(struct Cyc_Absyn_Tvar*)s2->hd)){
Cyc_Tcutil_failure_reason=({const char*_tmp2C5="(type variable would escape scope)";_tag_dyneither(_tmp2C5,sizeof(char),35U);});
(int)_throw((void*)& Cyc_Tcutil_Unify_val);}}}
# 1659
if(Cyc_Tcutil_kind_leq((struct Cyc_Absyn_Kind*)_tmp2CF->v,_tmp2C2)){
*_tmp2CA=t1;return;}
# 1662
Cyc_Tcutil_failure_reason=({const char*_tmp2C6="(kinds are incompatible)";_tag_dyneither(_tmp2C6,sizeof(char),25U);});
goto _LL5;}case 3U: _LL8: _tmp2CB=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2C3)->f1;if((int)((struct Cyc_Absyn_Kind*)_tmp2CF->v)->kind == (int)Cyc_Absyn_BoxKind){_LL9: {
# 1668
void*_tmp2C7=Cyc_Tcutil_compress((_tmp2CB.ptr_atts).bounds);
{void*_tmp2C8=_tmp2C7;if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp2C8)->tag == 1U){_LLD: _LLE:
# 1671
({void*_tmpAC1=_tmp2C7;Cyc_Tcutil_unify(_tmpAC1,Cyc_Absyn_bounds_one());});
*_tmp2CE=t2;
return;}else{_LLF: _LL10:
 goto _LLC;}_LLC:;}
# 1676
goto _LL5;}}else{goto _LLA;}default: _LLA: _LLB:
 goto _LL5;}_LL5:;}
# 1679
Cyc_Tcutil_failure_reason=({const char*_tmp2CC="(kinds are incompatible)";_tag_dyneither(_tmp2CC,sizeof(char),25U);});
(int)_throw((void*)& Cyc_Tcutil_Unify_val);}};}else{_LL3: _LL4:
# 1682
 goto _LL0;}_LL0:;}
# 1687
{struct _tuple0 _tmp2D0=({struct _tuple0 _tmp997;_tmp997.f1=t2,_tmp997.f2=t1;_tmp997;});struct _tuple0 _tmp2D1=_tmp2D0;struct Cyc_List_List*_tmp35F;struct Cyc_Absyn_Typedefdecl*_tmp35E;struct Cyc_List_List*_tmp35D;struct Cyc_Absyn_Typedefdecl*_tmp35C;enum Cyc_Absyn_AggrKind _tmp35B;struct Cyc_List_List*_tmp35A;enum Cyc_Absyn_AggrKind _tmp359;struct Cyc_List_List*_tmp358;struct Cyc_List_List*_tmp357;struct Cyc_List_List*_tmp356;struct Cyc_List_List*_tmp355;void*_tmp354;struct Cyc_Absyn_Tqual _tmp353;void*_tmp352;struct Cyc_List_List*_tmp351;int _tmp350;struct Cyc_Absyn_VarargInfo*_tmp34F;struct Cyc_List_List*_tmp34E;struct Cyc_List_List*_tmp34D;struct Cyc_Absyn_Exp*_tmp34C;struct Cyc_List_List*_tmp34B;struct Cyc_Absyn_Exp*_tmp34A;struct Cyc_List_List*_tmp349;struct Cyc_List_List*_tmp348;void*_tmp347;struct Cyc_Absyn_Tqual _tmp346;void*_tmp345;struct Cyc_List_List*_tmp344;int _tmp343;struct Cyc_Absyn_VarargInfo*_tmp342;struct Cyc_List_List*_tmp341;struct Cyc_List_List*_tmp340;struct Cyc_Absyn_Exp*_tmp33F;struct Cyc_List_List*_tmp33E;struct Cyc_Absyn_Exp*_tmp33D;struct Cyc_List_List*_tmp33C;void*_tmp33B;struct Cyc_Absyn_Tqual _tmp33A;struct Cyc_Absyn_Exp*_tmp339;void*_tmp338;void*_tmp337;struct Cyc_Absyn_Tqual _tmp336;struct Cyc_Absyn_Exp*_tmp335;void*_tmp334;struct Cyc_Absyn_Exp*_tmp333;struct Cyc_Absyn_Exp*_tmp332;void*_tmp331;struct Cyc_Absyn_Tqual _tmp330;void*_tmp32F;void*_tmp32E;void*_tmp32D;void*_tmp32C;void*_tmp32B;struct Cyc_Absyn_Tqual _tmp32A;void*_tmp329;void*_tmp328;void*_tmp327;void*_tmp326;struct Cyc_Absyn_Tvar*_tmp325;struct Cyc_Absyn_Tvar*_tmp324;void*_tmp323;struct Cyc_List_List*_tmp322;void*_tmp321;struct Cyc_List_List*_tmp320;switch(*((int*)_tmp2D1.f1)){case 1U: _LL12: _LL13:
# 1690
 Cyc_Tcutil_unify_it(t2,t1);
return;case 0U: if(((struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2D1.f1)->f1)->tag == 9U){_LL14: _LL15:
# 1693
 goto _LL17;}else{if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2D1.f2)->tag == 0U){if(((struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2D1.f2)->f1)->tag == 9U)goto _LL16;else{if(((struct Cyc_Absyn_AccessCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2D1.f1)->f1)->tag == 8U)goto _LL18;else{if(((struct Cyc_Absyn_AccessCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2D1.f2)->f1)->tag == 8U)goto _LL1A;else{if(((struct Cyc_Absyn_RgnsCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2D1.f1)->f1)->tag == 10U)goto _LL1C;else{if(((struct Cyc_Absyn_RgnsCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2D1.f2)->f1)->tag == 10U)goto _LL1E;else{_LL20: _tmp323=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2D1.f1)->f1;_tmp322=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2D1.f1)->f2;_tmp321=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2D1.f2)->f1;_tmp320=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2D1.f2)->f2;_LL21:
# 1704
 if(Cyc_Tcutil_tycon_cmp(_tmp323,_tmp321)== 0){
Cyc_Tcutil_unify_list(_tmp322,_tmp320);
return;}else{
# 1708
Cyc_Tcutil_failure_reason=({const char*_tmp2D3="(different type constructors)";_tag_dyneither(_tmp2D3,sizeof(char),30U);});}
goto _LL11;}}}}}}else{switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2D1.f1)->f1)){case 8U: _LL18: _LL19:
# 1695
 goto _LL1B;case 10U: _LL1C: _LL1D:
# 1697
 goto _LL1F;default: goto _LL32;}}}default: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2D1.f2)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2D1.f2)->f1)){case 9U: _LL16: _LL17:
# 1694
 goto _LL19;case 8U: _LL1A: _LL1B:
# 1696
 goto _LL1D;case 10U: _LL1E: _LL1F:
# 1699
 if(Cyc_Tcutil_unify_effect(t1,t2))return;
Cyc_Tcutil_failure_reason=({const char*_tmp2D2="(effects don't unify)";_tag_dyneither(_tmp2D2,sizeof(char),22U);});
goto _LL11;default: switch(*((int*)_tmp2D1.f1)){case 2U: goto _LL32;case 3U: goto _LL32;case 9U: goto _LL32;case 4U: goto _LL32;case 5U: goto _LL32;case 6U: goto _LL32;case 7U: goto _LL32;case 8U: goto _LL32;default: goto _LL32;}}else{switch(*((int*)_tmp2D1.f1)){case 2U: if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp2D1.f2)->tag == 2U){_LL22: _tmp325=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp2D1.f1)->f1;_tmp324=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp2D1.f2)->f1;_LL23: {
# 1712
struct _dyneither_ptr*_tmp2D4=_tmp325->name;
struct _dyneither_ptr*_tmp2D5=_tmp324->name;
# 1715
int _tmp2D6=_tmp325->identity;
int _tmp2D7=_tmp324->identity;
if(_tmp2D7 == _tmp2D6)return;
Cyc_Tcutil_failure_reason=({const char*_tmp2D8="(variable types are not the same)";_tag_dyneither(_tmp2D8,sizeof(char),34U);});
goto _LL11;}}else{goto _LL32;}case 3U: if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2D1.f2)->tag == 3U){_LL24: _tmp331=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2D1.f1)->f1).elt_type;_tmp330=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2D1.f1)->f1).elt_tq;_tmp32F=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2D1.f1)->f1).ptr_atts).rgn;_tmp32E=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2D1.f1)->f1).ptr_atts).nullable;_tmp32D=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2D1.f1)->f1).ptr_atts).bounds;_tmp32C=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2D1.f1)->f1).ptr_atts).zero_term;_tmp32B=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2D1.f2)->f1).elt_type;_tmp32A=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2D1.f2)->f1).elt_tq;_tmp329=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2D1.f2)->f1).ptr_atts).rgn;_tmp328=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2D1.f2)->f1).ptr_atts).nullable;_tmp327=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2D1.f2)->f1).ptr_atts).bounds;_tmp326=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2D1.f2)->f1).ptr_atts).zero_term;_LL25:
# 1723
 Cyc_Tcutil_unify_it(_tmp32B,_tmp331);
Cyc_Tcutil_unify_it(_tmp32F,_tmp329);
Cyc_Tcutil_t1_failure=t1;
Cyc_Tcutil_t2_failure=t2;{
struct _dyneither_ptr _tmp2D9=Cyc_Tcutil_failure_reason;
Cyc_Tcutil_failure_reason=({const char*_tmp2DA="(not both zero terminated)";_tag_dyneither(_tmp2DA,sizeof(char),27U);});
Cyc_Tcutil_unify_it(_tmp326,_tmp32C);
Cyc_Tcutil_unify_tqual(_tmp32A,_tmp32B,_tmp330,_tmp331);
Cyc_Tcutil_failure_reason=({const char*_tmp2DB="(different pointer bounds)";_tag_dyneither(_tmp2DB,sizeof(char),27U);});
Cyc_Tcutil_unify_it(_tmp327,_tmp32D);{
# 1734
void*_tmp2DC=Cyc_Tcutil_compress(_tmp327);void*_tmp2DD=_tmp2DC;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2DD)->tag == 0U){if(((struct Cyc_Absyn_FatCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2DD)->f1)->tag == 14U){_LL35: _LL36:
# 1736
 Cyc_Tcutil_failure_reason=_tmp2D9;
return;}else{goto _LL37;}}else{_LL37: _LL38:
# 1739
 Cyc_Tcutil_failure_reason=({const char*_tmp2DE="(incompatible pointer types)";_tag_dyneither(_tmp2DE,sizeof(char),29U);});
Cyc_Tcutil_unify_it(_tmp328,_tmp32E);
return;}_LL34:;};};}else{goto _LL32;}case 9U: if(((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp2D1.f2)->tag == 9U){_LL26: _tmp333=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp2D1.f1)->f1;_tmp332=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp2D1.f2)->f1;_LL27:
# 1745
 if(!Cyc_Evexp_same_const_exp(_tmp333,_tmp332)){
Cyc_Tcutil_failure_reason=({const char*_tmp2DF="(cannot prove expressions are the same)";_tag_dyneither(_tmp2DF,sizeof(char),40U);});
goto _LL11;}
# 1749
return;}else{goto _LL32;}case 4U: if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp2D1.f2)->tag == 4U){_LL28: _tmp33B=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp2D1.f1)->f1).elt_type;_tmp33A=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp2D1.f1)->f1).tq;_tmp339=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp2D1.f1)->f1).num_elts;_tmp338=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp2D1.f1)->f1).zero_term;_tmp337=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp2D1.f2)->f1).elt_type;_tmp336=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp2D1.f2)->f1).tq;_tmp335=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp2D1.f2)->f1).num_elts;_tmp334=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp2D1.f2)->f1).zero_term;_LL29:
# 1753
 Cyc_Tcutil_unify_it(_tmp337,_tmp33B);
Cyc_Tcutil_unify_tqual(_tmp336,_tmp337,_tmp33A,_tmp33B);
Cyc_Tcutil_t1_failure=t1;
Cyc_Tcutil_t2_failure=t2;
Cyc_Tcutil_failure_reason=({const char*_tmp2E0="(not both zero terminated)";_tag_dyneither(_tmp2E0,sizeof(char),27U);});
Cyc_Tcutil_unify_it(_tmp338,_tmp334);
if(_tmp339 == _tmp335)return;
if(_tmp339 == 0  || _tmp335 == 0)goto _LL11;
if(Cyc_Evexp_same_const_exp(_tmp339,_tmp335))
return;
Cyc_Tcutil_failure_reason=({const char*_tmp2E1="(different array sizes)";_tag_dyneither(_tmp2E1,sizeof(char),24U);});
goto _LL11;}else{goto _LL32;}case 5U: if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp2D1.f2)->tag == 5U){_LL2A: _tmp355=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp2D1.f1)->f1).tvars;_tmp354=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp2D1.f1)->f1).effect;_tmp353=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp2D1.f1)->f1).ret_tqual;_tmp352=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp2D1.f1)->f1).ret_type;_tmp351=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp2D1.f1)->f1).args;_tmp350=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp2D1.f1)->f1).c_varargs;_tmp34F=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp2D1.f1)->f1).cyc_varargs;_tmp34E=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp2D1.f1)->f1).rgn_po;_tmp34D=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp2D1.f1)->f1).attributes;_tmp34C=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp2D1.f1)->f1).requires_clause;_tmp34B=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp2D1.f1)->f1).requires_relns;_tmp34A=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp2D1.f1)->f1).ensures_clause;_tmp349=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp2D1.f1)->f1).ensures_relns;_tmp348=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp2D1.f2)->f1).tvars;_tmp347=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp2D1.f2)->f1).effect;_tmp346=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp2D1.f2)->f1).ret_tqual;_tmp345=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp2D1.f2)->f1).ret_type;_tmp344=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp2D1.f2)->f1).args;_tmp343=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp2D1.f2)->f1).c_varargs;_tmp342=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp2D1.f2)->f1).cyc_varargs;_tmp341=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp2D1.f2)->f1).rgn_po;_tmp340=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp2D1.f2)->f1).attributes;_tmp33F=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp2D1.f2)->f1).requires_clause;_tmp33E=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp2D1.f2)->f1).requires_relns;_tmp33D=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp2D1.f2)->f1).ensures_clause;_tmp33C=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp2D1.f2)->f1).ensures_relns;_LL2B: {
# 1768
int done=0;
{struct _RegionHandle _tmp2E2=_new_region("rgn");struct _RegionHandle*rgn=& _tmp2E2;_push_region(rgn);
{struct Cyc_List_List*inst=0;
while(_tmp348 != 0){
if(_tmp355 == 0){
Cyc_Tcutil_failure_reason=({const char*_tmp2E3="(second function type has too few type variables)";_tag_dyneither(_tmp2E3,sizeof(char),50U);});
(int)_throw((void*)& Cyc_Tcutil_Unify_val);}{
# 1776
void*_tmp2E4=((struct Cyc_Absyn_Tvar*)_tmp348->hd)->kind;
void*_tmp2E5=((struct Cyc_Absyn_Tvar*)_tmp355->hd)->kind;
if(!Cyc_Tcutil_unify_kindbound(_tmp2E4,_tmp2E5)){
Cyc_Tcutil_failure_reason=(struct _dyneither_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp2E8=({struct Cyc_String_pa_PrintArg_struct _tmp994;_tmp994.tag=0U,({
struct _dyneither_ptr _tmpAC2=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_tvar2string((struct Cyc_Absyn_Tvar*)_tmp348->hd));_tmp994.f1=_tmpAC2;});_tmp994;});struct Cyc_String_pa_PrintArg_struct _tmp2E9=({struct Cyc_String_pa_PrintArg_struct _tmp993;_tmp993.tag=0U,({
struct _dyneither_ptr _tmpAC3=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_kind2string(Cyc_Tcutil_tvar_kind((struct Cyc_Absyn_Tvar*)_tmp348->hd,& Cyc_Tcutil_bk)));_tmp993.f1=_tmpAC3;});_tmp993;});struct Cyc_String_pa_PrintArg_struct _tmp2EA=({struct Cyc_String_pa_PrintArg_struct _tmp992;_tmp992.tag=0U,({
struct _dyneither_ptr _tmpAC4=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_kind2string(Cyc_Tcutil_tvar_kind((struct Cyc_Absyn_Tvar*)_tmp355->hd,& Cyc_Tcutil_bk)));_tmp992.f1=_tmpAC4;});_tmp992;});void*_tmp2E6[3U];_tmp2E6[0]=& _tmp2E8,_tmp2E6[1]=& _tmp2E9,_tmp2E6[2]=& _tmp2EA;({struct _dyneither_ptr _tmpAC5=({const char*_tmp2E7="(type var %s has different kinds %s and %s)";_tag_dyneither(_tmp2E7,sizeof(char),44U);});Cyc_aprintf(_tmpAC5,_tag_dyneither(_tmp2E6,sizeof(void*),3U));});});
(int)_throw((void*)& Cyc_Tcutil_Unify_val);}
# 1785
inst=({struct Cyc_List_List*_tmp2EC=_region_malloc(rgn,sizeof(*_tmp2EC));({struct _tuple15*_tmpAC7=({struct _tuple15*_tmp2EB=_region_malloc(rgn,sizeof(*_tmp2EB));_tmp2EB->f1=(struct Cyc_Absyn_Tvar*)_tmp355->hd,({void*_tmpAC6=Cyc_Absyn_var_type((struct Cyc_Absyn_Tvar*)_tmp348->hd);_tmp2EB->f2=_tmpAC6;});_tmp2EB;});_tmp2EC->hd=_tmpAC7;}),_tmp2EC->tl=inst;_tmp2EC;});
_tmp348=_tmp348->tl;
_tmp355=_tmp355->tl;};}
# 1789
if(_tmp355 != 0){
Cyc_Tcutil_failure_reason=({const char*_tmp2ED="(second function type has too many type variables)";_tag_dyneither(_tmp2ED,sizeof(char),51U);});
_npop_handler(0U);goto _LL11;}
# 1793
if(inst != 0){
({void*_tmpACA=(void*)({struct Cyc_Absyn_FnType_Absyn_Type_struct*_tmp2EE=_cycalloc(sizeof(*_tmp2EE));_tmp2EE->tag=5U,(_tmp2EE->f1).tvars=0,(_tmp2EE->f1).effect=_tmp347,(_tmp2EE->f1).ret_tqual=_tmp346,(_tmp2EE->f1).ret_type=_tmp345,(_tmp2EE->f1).args=_tmp344,(_tmp2EE->f1).c_varargs=_tmp343,(_tmp2EE->f1).cyc_varargs=_tmp342,(_tmp2EE->f1).rgn_po=_tmp341,(_tmp2EE->f1).attributes=_tmp340,(_tmp2EE->f1).requires_clause=_tmp34C,(_tmp2EE->f1).requires_relns=_tmp34B,(_tmp2EE->f1).ensures_clause=_tmp34A,(_tmp2EE->f1).ensures_relns=_tmp349;_tmp2EE;});Cyc_Tcutil_unify_it(_tmpACA,({
# 1797
struct _RegionHandle*_tmpAC9=rgn;struct Cyc_List_List*_tmpAC8=inst;Cyc_Tcutil_rsubstitute(_tmpAC9,_tmpAC8,(void*)({struct Cyc_Absyn_FnType_Absyn_Type_struct*_tmp2EF=_cycalloc(sizeof(*_tmp2EF));
_tmp2EF->tag=5U,(_tmp2EF->f1).tvars=0,(_tmp2EF->f1).effect=_tmp354,(_tmp2EF->f1).ret_tqual=_tmp353,(_tmp2EF->f1).ret_type=_tmp352,(_tmp2EF->f1).args=_tmp351,(_tmp2EF->f1).c_varargs=_tmp350,(_tmp2EF->f1).cyc_varargs=_tmp34F,(_tmp2EF->f1).rgn_po=_tmp34E,(_tmp2EF->f1).attributes=_tmp34D,(_tmp2EF->f1).requires_clause=_tmp33F,(_tmp2EF->f1).requires_relns=_tmp33E,(_tmp2EF->f1).ensures_clause=_tmp33D,(_tmp2EF->f1).ensures_relns=_tmp33C;_tmp2EF;}));}));});
# 1802
done=1;}}
# 1770
;_pop_region(rgn);}
# 1805
if(done)
return;
Cyc_Tcutil_unify_it(_tmp345,_tmp352);
Cyc_Tcutil_unify_tqual(_tmp346,_tmp345,_tmp353,_tmp352);
for(0;_tmp344 != 0  && _tmp351 != 0;(_tmp344=_tmp344->tl,_tmp351=_tmp351->tl)){
struct _tuple10 _tmp2F0=*((struct _tuple10*)_tmp344->hd);struct _tuple10 _tmp2F1=_tmp2F0;struct Cyc_Absyn_Tqual _tmp2F7;void*_tmp2F6;_LL3A: _tmp2F7=_tmp2F1.f2;_tmp2F6=_tmp2F1.f3;_LL3B:;{
struct _tuple10 _tmp2F2=*((struct _tuple10*)_tmp351->hd);struct _tuple10 _tmp2F3=_tmp2F2;struct Cyc_Absyn_Tqual _tmp2F5;void*_tmp2F4;_LL3D: _tmp2F5=_tmp2F3.f2;_tmp2F4=_tmp2F3.f3;_LL3E:;
Cyc_Tcutil_unify_it(_tmp2F6,_tmp2F4);
Cyc_Tcutil_unify_tqual(_tmp2F7,_tmp2F6,_tmp2F5,_tmp2F4);};}
# 1815
Cyc_Tcutil_t1_failure=t1;
Cyc_Tcutil_t2_failure=t2;
if(_tmp344 != 0  || _tmp351 != 0){
Cyc_Tcutil_failure_reason=({const char*_tmp2F8="(function types have different number of arguments)";_tag_dyneither(_tmp2F8,sizeof(char),52U);});
goto _LL11;}
# 1821
if(_tmp343 != _tmp350){
Cyc_Tcutil_failure_reason=({const char*_tmp2F9="(only one function type takes C varargs)";_tag_dyneither(_tmp2F9,sizeof(char),41U);});
goto _LL11;}{
# 1826
int bad_cyc_vararg=0;
{struct _tuple23 _tmp2FA=({struct _tuple23 _tmp995;_tmp995.f1=_tmp342,_tmp995.f2=_tmp34F;_tmp995;});struct _tuple23 _tmp2FB=_tmp2FA;struct _dyneither_ptr*_tmp305;struct Cyc_Absyn_Tqual _tmp304;void*_tmp303;int _tmp302;struct _dyneither_ptr*_tmp301;struct Cyc_Absyn_Tqual _tmp300;void*_tmp2FF;int _tmp2FE;if(_tmp2FB.f1 == 0){if(_tmp2FB.f2 == 0){_LL40: _LL41:
 goto _LL3F;}else{_LL42: _LL43:
 goto _LL45;}}else{if(_tmp2FB.f2 == 0){_LL44: _LL45:
# 1831
 bad_cyc_vararg=1;
Cyc_Tcutil_failure_reason=({const char*_tmp2FC="(only one function type takes varargs)";_tag_dyneither(_tmp2FC,sizeof(char),39U);});
goto _LL3F;}else{_LL46: _tmp305=(_tmp2FB.f1)->name;_tmp304=(_tmp2FB.f1)->tq;_tmp303=(_tmp2FB.f1)->type;_tmp302=(_tmp2FB.f1)->inject;_tmp301=(_tmp2FB.f2)->name;_tmp300=(_tmp2FB.f2)->tq;_tmp2FF=(_tmp2FB.f2)->type;_tmp2FE=(_tmp2FB.f2)->inject;_LL47:
# 1835
 Cyc_Tcutil_unify_it(_tmp303,_tmp2FF);
Cyc_Tcutil_unify_tqual(_tmp304,_tmp303,_tmp300,_tmp2FF);
if(_tmp302 != _tmp2FE){
bad_cyc_vararg=1;
Cyc_Tcutil_failure_reason=({const char*_tmp2FD="(only one function type injects varargs)";_tag_dyneither(_tmp2FD,sizeof(char),41U);});}
# 1841
goto _LL3F;}}_LL3F:;}
# 1843
if(bad_cyc_vararg)goto _LL11;{
# 1846
int bad_effect=0;
{struct _tuple0 _tmp306=({struct _tuple0 _tmp996;_tmp996.f1=_tmp347,_tmp996.f2=_tmp354;_tmp996;});struct _tuple0 _tmp307=_tmp306;if(_tmp307.f1 == 0){if(_tmp307.f2 == 0){_LL49: _LL4A:
 goto _LL48;}else{_LL4B: _LL4C:
 goto _LL4E;}}else{if(_tmp307.f2 == 0){_LL4D: _LL4E:
 bad_effect=1;goto _LL48;}else{_LL4F: _LL50:
 bad_effect=!({void*_tmpACB=(void*)_check_null(_tmp347);Cyc_Tcutil_unify_effect(_tmpACB,(void*)_check_null(_tmp354));});goto _LL48;}}_LL48:;}
# 1853
Cyc_Tcutil_t1_failure=t1;
Cyc_Tcutil_t2_failure=t2;
if(bad_effect){
Cyc_Tcutil_failure_reason=({const char*_tmp308="(function effects do not match)";_tag_dyneither(_tmp308,sizeof(char),32U);});
goto _LL11;}
# 1859
if(!Cyc_Tcutil_same_atts(_tmp34D,_tmp340)){
Cyc_Tcutil_failure_reason=({const char*_tmp309="(function types have different attributes)";_tag_dyneither(_tmp309,sizeof(char),43U);});
goto _LL11;}
# 1863
if(!Cyc_Tcutil_same_rgn_po(_tmp34E,_tmp341)){
Cyc_Tcutil_failure_reason=({const char*_tmp30A="(function types have different region lifetime orderings)";_tag_dyneither(_tmp30A,sizeof(char),58U);});
goto _LL11;}
# 1867
if(!Cyc_Tcutil_check_logical_equivalence(_tmp34B,_tmp33E)){
Cyc_Tcutil_failure_reason=({const char*_tmp30B="(@requires clauses not equivalent)";_tag_dyneither(_tmp30B,sizeof(char),35U);});
goto _LL11;}
# 1871
if(!Cyc_Tcutil_check_logical_equivalence(_tmp349,_tmp33C)){
Cyc_Tcutil_failure_reason=({const char*_tmp30C="(@ensures clauses not equivalent)";_tag_dyneither(_tmp30C,sizeof(char),34U);});
goto _LL11;}
# 1875
return;};};}}else{goto _LL32;}case 6U: if(((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp2D1.f2)->tag == 6U){_LL2C: _tmp357=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp2D1.f1)->f1;_tmp356=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp2D1.f2)->f1;_LL2D:
# 1878
 for(0;_tmp356 != 0  && _tmp357 != 0;(_tmp356=_tmp356->tl,_tmp357=_tmp357->tl)){
struct _tuple12 _tmp30D=*((struct _tuple12*)_tmp356->hd);struct _tuple12 _tmp30E=_tmp30D;struct Cyc_Absyn_Tqual _tmp314;void*_tmp313;_LL52: _tmp314=_tmp30E.f1;_tmp313=_tmp30E.f2;_LL53:;{
struct _tuple12 _tmp30F=*((struct _tuple12*)_tmp357->hd);struct _tuple12 _tmp310=_tmp30F;struct Cyc_Absyn_Tqual _tmp312;void*_tmp311;_LL55: _tmp312=_tmp310.f1;_tmp311=_tmp310.f2;_LL56:;
Cyc_Tcutil_unify_it(_tmp313,_tmp311);
Cyc_Tcutil_unify_tqual(_tmp314,_tmp313,_tmp312,_tmp311);};}
# 1884
if(_tmp356 == 0  && _tmp357 == 0)return;
Cyc_Tcutil_t1_failure=t1;
Cyc_Tcutil_t2_failure=t2;
Cyc_Tcutil_failure_reason=({const char*_tmp315="(tuple types have different numbers of components)";_tag_dyneither(_tmp315,sizeof(char),51U);});
goto _LL11;}else{goto _LL32;}case 7U: if(((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp2D1.f2)->tag == 7U){_LL2E: _tmp35B=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp2D1.f1)->f1;_tmp35A=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp2D1.f1)->f2;_tmp359=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp2D1.f2)->f1;_tmp358=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp2D1.f2)->f2;_LL2F:
# 1891
 if((int)_tmp359 != (int)_tmp35B){Cyc_Tcutil_failure_reason=({const char*_tmp316="(struct and union type)";_tag_dyneither(_tmp316,sizeof(char),24U);});goto _LL11;}
for(0;_tmp358 != 0  && _tmp35A != 0;(_tmp358=_tmp358->tl,_tmp35A=_tmp35A->tl)){
struct Cyc_Absyn_Aggrfield*_tmp317=(struct Cyc_Absyn_Aggrfield*)_tmp358->hd;
struct Cyc_Absyn_Aggrfield*_tmp318=(struct Cyc_Absyn_Aggrfield*)_tmp35A->hd;
if(Cyc_strptrcmp(_tmp317->name,_tmp318->name)!= 0){
Cyc_Tcutil_failure_reason=({const char*_tmp319="(different member names)";_tag_dyneither(_tmp319,sizeof(char),25U);});
(int)_throw((void*)& Cyc_Tcutil_Unify_val);}
# 1899
Cyc_Tcutil_unify_it(_tmp317->type,_tmp318->type);
Cyc_Tcutil_unify_tqual(_tmp317->tq,_tmp317->type,_tmp318->tq,_tmp318->type);
if(!Cyc_Tcutil_same_atts(_tmp317->attributes,_tmp318->attributes)){
Cyc_Tcutil_t1_failure=t1;
Cyc_Tcutil_t2_failure=t2;
Cyc_Tcutil_failure_reason=({const char*_tmp31A="(different attributes on member)";_tag_dyneither(_tmp31A,sizeof(char),33U);});
(int)_throw((void*)& Cyc_Tcutil_Unify_val);}
# 1907
if((_tmp317->width != 0  && _tmp318->width == 0  || 
_tmp318->width != 0  && _tmp317->width == 0) || 
(_tmp317->width != 0  && _tmp318->width != 0) && !({
struct Cyc_Absyn_Exp*_tmpACC=(struct Cyc_Absyn_Exp*)_check_null(_tmp317->width);Cyc_Evexp_same_const_exp(_tmpACC,(struct Cyc_Absyn_Exp*)_check_null(_tmp318->width));})){
Cyc_Tcutil_t1_failure=t1;
Cyc_Tcutil_t2_failure=t2;
Cyc_Tcutil_failure_reason=({const char*_tmp31B="(different bitfield widths on member)";_tag_dyneither(_tmp31B,sizeof(char),38U);});
(int)_throw((void*)& Cyc_Tcutil_Unify_val);}
# 1916
if((_tmp317->requires_clause != 0  && _tmp318->requires_clause == 0  || 
_tmp317->requires_clause == 0  && _tmp318->requires_clause != 0) || 
(_tmp317->requires_clause == 0  && _tmp318->requires_clause != 0) && !({
struct Cyc_Absyn_Exp*_tmpACD=(struct Cyc_Absyn_Exp*)_check_null(_tmp317->requires_clause);Cyc_Evexp_same_const_exp(_tmpACD,(struct Cyc_Absyn_Exp*)_check_null(_tmp318->requires_clause));})){
# 1921
Cyc_Tcutil_t1_failure=t1;
Cyc_Tcutil_t2_failure=t2;
Cyc_Tcutil_failure_reason=({const char*_tmp31C="(different @requires clauses on member)";_tag_dyneither(_tmp31C,sizeof(char),40U);});
(int)_throw((void*)& Cyc_Tcutil_Unify_val);}}
# 1927
if(_tmp358 == 0  && _tmp35A == 0)return;
Cyc_Tcutil_t1_failure=t1;
Cyc_Tcutil_t2_failure=t2;
Cyc_Tcutil_failure_reason=({const char*_tmp31D="(different number of members)";_tag_dyneither(_tmp31D,sizeof(char),30U);});
goto _LL11;}else{goto _LL32;}case 8U: if(((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp2D1.f2)->tag == 8U){_LL30: _tmp35F=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp2D1.f1)->f2;_tmp35E=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp2D1.f1)->f3;_tmp35D=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp2D1.f2)->f2;_tmp35C=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp2D1.f2)->f3;_LL31:
# 1933
 if(_tmp35E != _tmp35C){
Cyc_Tcutil_failure_reason=({const char*_tmp31E="(different abstract typedefs)";_tag_dyneither(_tmp31E,sizeof(char),30U);});
goto _LL11;}
# 1937
Cyc_Tcutil_failure_reason=({const char*_tmp31F="(type parameters to typedef differ)";_tag_dyneither(_tmp31F,sizeof(char),36U);});
Cyc_Tcutil_unify_list(_tmp35F,_tmp35D);
return;}else{goto _LL32;}default: _LL32: _LL33:
 goto _LL11;}}}_LL11:;}
# 1942
(int)_throw((void*)& Cyc_Tcutil_Unify_val);}
# 1945
int Cyc_Tcutil_star_cmp(int(*cmp)(void*,void*),void*a1,void*a2){
if(a1 == a2)return 0;
if(a1 == 0  && a2 != 0)return - 1;
if(a1 != 0  && a2 == 0)return 1;
return({int(*_tmpACF)(void*,void*)=cmp;void*_tmpACE=(void*)_check_null(a1);_tmpACF(_tmpACE,(void*)_check_null(a2));});}
# 1952
static int Cyc_Tcutil_tqual_cmp(struct Cyc_Absyn_Tqual tq1,struct Cyc_Absyn_Tqual tq2){
int _tmp360=(tq1.real_const + (tq1.q_volatile << 1))+ (tq1.q_restrict << 2);
int _tmp361=(tq2.real_const + (tq2.q_volatile << 1))+ (tq2.q_restrict << 2);
return Cyc_Core_intcmp(_tmp360,_tmp361);}
# 1958
static int Cyc_Tcutil_tqual_type_cmp(struct _tuple12*tqt1,struct _tuple12*tqt2){
struct _tuple12*_tmp362=tqt1;struct Cyc_Absyn_Tqual _tmp368;void*_tmp367;_LL1: _tmp368=_tmp362->f1;_tmp367=_tmp362->f2;_LL2:;{
struct _tuple12*_tmp363=tqt2;struct Cyc_Absyn_Tqual _tmp366;void*_tmp365;_LL4: _tmp366=_tmp363->f1;_tmp365=_tmp363->f2;_LL5:;{
int _tmp364=Cyc_Tcutil_tqual_cmp(_tmp368,_tmp366);
if(_tmp364 != 0)return _tmp364;
return Cyc_Tcutil_typecmp(_tmp367,_tmp365);};};}
# 1966
int Cyc_Tcutil_aggrfield_cmp(struct Cyc_Absyn_Aggrfield*f1,struct Cyc_Absyn_Aggrfield*f2){
int _tmp369=Cyc_strptrcmp(f1->name,f2->name);
if(_tmp369 != 0)return _tmp369;{
int _tmp36A=Cyc_Tcutil_tqual_cmp(f1->tq,f2->tq);
if(_tmp36A != 0)return _tmp36A;{
int _tmp36B=Cyc_Tcutil_typecmp(f1->type,f2->type);
if(_tmp36B != 0)return _tmp36B;{
int _tmp36C=((int(*)(int(*cmp)(void*,void*),struct Cyc_List_List*l1,struct Cyc_List_List*l2))Cyc_List_list_cmp)(Cyc_Tcutil_attribute_cmp,f1->attributes,f2->attributes);
if(_tmp36C != 0)return _tmp36C;
_tmp36C=((int(*)(int(*cmp)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*a1,struct Cyc_Absyn_Exp*a2))Cyc_Tcutil_star_cmp)(Cyc_Evexp_const_exp_cmp,f1->width,f2->width);
if(_tmp36C != 0)return _tmp36C;
return((int(*)(int(*cmp)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*a1,struct Cyc_Absyn_Exp*a2))Cyc_Tcutil_star_cmp)(Cyc_Evexp_const_exp_cmp,f1->requires_clause,f2->requires_clause);};};};}
# 1980
static int Cyc_Tcutil_enumfield_cmp(struct Cyc_Absyn_Enumfield*e1,struct Cyc_Absyn_Enumfield*e2){
int _tmp36D=Cyc_Absyn_qvar_cmp(e1->name,e2->name);
if(_tmp36D != 0)return _tmp36D;
return((int(*)(int(*cmp)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*a1,struct Cyc_Absyn_Exp*a2))Cyc_Tcutil_star_cmp)(Cyc_Evexp_const_exp_cmp,e1->tag,e2->tag);}
# 1986
static int Cyc_Tcutil_type_case_number(void*t){
void*_tmp36E=t;void*_tmp36F;switch(*((int*)_tmp36E)){case 1U: _LL1: _LL2:
 return 1;case 2U: _LL3: _LL4:
 return 2;case 3U: _LL5: _LL6:
 return 3;case 4U: _LL7: _LL8:
 return 4;case 5U: _LL9: _LLA:
 return 5;case 6U: _LLB: _LLC:
 return 6;case 7U: _LLD: _LLE:
 return 7;case 8U: _LLF: _LL10:
 return 8;case 9U: _LL11: _LL12:
 return 9;case 10U: _LL13: _LL14:
 return 10;case 11U: _LL15: _LL16:
 return 11;default: _LL17: _tmp36F=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp36E)->f1;_LL18:
 return 12 + Cyc_Tcutil_tycon2int(_tmp36F);}_LL0:;}
# 2005
int Cyc_Tcutil_typecmp(void*t1,void*t2){
t1=Cyc_Tcutil_compress(t1);
t2=Cyc_Tcutil_compress(t2);
if(t1 == t2)return 0;{
int _tmp370=({int _tmpAD0=Cyc_Tcutil_type_case_number(t1);Cyc_Core_intcmp(_tmpAD0,Cyc_Tcutil_type_case_number(t2));});
if(_tmp370 != 0)
return _tmp370;{
# 2014
struct _tuple0 _tmp371=({struct _tuple0 _tmp998;_tmp998.f1=t2,_tmp998.f2=t1;_tmp998;});struct _tuple0 _tmp372=_tmp371;struct Cyc_Absyn_Exp*_tmp3BE;struct Cyc_Absyn_Exp*_tmp3BD;struct Cyc_Absyn_Exp*_tmp3BC;struct Cyc_Absyn_Exp*_tmp3BB;enum Cyc_Absyn_AggrKind _tmp3BA;struct Cyc_List_List*_tmp3B9;enum Cyc_Absyn_AggrKind _tmp3B8;struct Cyc_List_List*_tmp3B7;struct Cyc_List_List*_tmp3B6;struct Cyc_List_List*_tmp3B5;struct Cyc_Absyn_FnInfo _tmp3B4;struct Cyc_Absyn_FnInfo _tmp3B3;void*_tmp3B2;struct Cyc_Absyn_Tqual _tmp3B1;struct Cyc_Absyn_Exp*_tmp3B0;void*_tmp3AF;void*_tmp3AE;struct Cyc_Absyn_Tqual _tmp3AD;struct Cyc_Absyn_Exp*_tmp3AC;void*_tmp3AB;void*_tmp3AA;struct Cyc_Absyn_Tqual _tmp3A9;void*_tmp3A8;void*_tmp3A7;void*_tmp3A6;void*_tmp3A5;void*_tmp3A4;struct Cyc_Absyn_Tqual _tmp3A3;void*_tmp3A2;void*_tmp3A1;void*_tmp3A0;void*_tmp39F;struct Cyc_Absyn_Tvar*_tmp39E;struct Cyc_Absyn_Tvar*_tmp39D;void*_tmp39C;struct Cyc_List_List*_tmp39B;void*_tmp39A;struct Cyc_List_List*_tmp399;switch(*((int*)_tmp372.f1)){case 0U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp372.f2)->tag == 0U){_LL1: _tmp39C=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp372.f1)->f1;_tmp39B=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp372.f1)->f2;_tmp39A=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp372.f2)->f1;_tmp399=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp372.f2)->f2;_LL2: {
# 2016
int _tmp373=Cyc_Tcutil_tycon_cmp(_tmp39C,_tmp39A);
if(_tmp373 != 0)return _tmp373;
return((int(*)(int(*cmp)(void*,void*),struct Cyc_List_List*l1,struct Cyc_List_List*l2))Cyc_List_list_cmp)(Cyc_Tcutil_typecmp,_tmp39B,_tmp399);}}else{goto _LL15;}case 1U: if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp372.f2)->tag == 1U){_LL3: _LL4:
# 2020
({void*_tmp374=0U;({struct _dyneither_ptr _tmpAD1=({const char*_tmp375="typecmp: can only compare closed types";_tag_dyneither(_tmp375,sizeof(char),39U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpAD1,_tag_dyneither(_tmp374,sizeof(void*),0U));});});}else{goto _LL15;}case 2U: if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp372.f2)->tag == 2U){_LL5: _tmp39E=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp372.f1)->f1;_tmp39D=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp372.f2)->f1;_LL6:
# 2024
 return Cyc_Core_intcmp(_tmp39D->identity,_tmp39E->identity);}else{goto _LL15;}case 3U: if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp372.f2)->tag == 3U){_LL7: _tmp3AA=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp372.f1)->f1).elt_type;_tmp3A9=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp372.f1)->f1).elt_tq;_tmp3A8=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp372.f1)->f1).ptr_atts).rgn;_tmp3A7=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp372.f1)->f1).ptr_atts).nullable;_tmp3A6=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp372.f1)->f1).ptr_atts).bounds;_tmp3A5=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp372.f1)->f1).ptr_atts).zero_term;_tmp3A4=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp372.f2)->f1).elt_type;_tmp3A3=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp372.f2)->f1).elt_tq;_tmp3A2=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp372.f2)->f1).ptr_atts).rgn;_tmp3A1=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp372.f2)->f1).ptr_atts).nullable;_tmp3A0=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp372.f2)->f1).ptr_atts).bounds;_tmp39F=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp372.f2)->f1).ptr_atts).zero_term;_LL8: {
# 2028
int _tmp376=Cyc_Tcutil_typecmp(_tmp3A4,_tmp3AA);
if(_tmp376 != 0)return _tmp376;{
int _tmp377=Cyc_Tcutil_typecmp(_tmp3A2,_tmp3A8);
if(_tmp377 != 0)return _tmp377;{
int _tmp378=Cyc_Tcutil_tqual_cmp(_tmp3A3,_tmp3A9);
if(_tmp378 != 0)return _tmp378;{
int _tmp379=Cyc_Tcutil_typecmp(_tmp3A0,_tmp3A6);
if(_tmp379 != 0)return _tmp379;{
int _tmp37A=Cyc_Tcutil_typecmp(_tmp39F,_tmp3A5);
if(_tmp37A != 0)return _tmp37A;{
int _tmp37B=Cyc_Tcutil_typecmp(_tmp3A0,_tmp3A6);
if(_tmp37B != 0)return _tmp37B;
return Cyc_Tcutil_typecmp(_tmp3A1,_tmp3A7);};};};};};}}else{goto _LL15;}case 4U: if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp372.f2)->tag == 4U){_LL9: _tmp3B2=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp372.f1)->f1).elt_type;_tmp3B1=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp372.f1)->f1).tq;_tmp3B0=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp372.f1)->f1).num_elts;_tmp3AF=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp372.f1)->f1).zero_term;_tmp3AE=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp372.f2)->f1).elt_type;_tmp3AD=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp372.f2)->f1).tq;_tmp3AC=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp372.f2)->f1).num_elts;_tmp3AB=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp372.f2)->f1).zero_term;_LLA: {
# 2044
int _tmp37C=Cyc_Tcutil_tqual_cmp(_tmp3AD,_tmp3B1);
if(_tmp37C != 0)return _tmp37C;{
int _tmp37D=Cyc_Tcutil_typecmp(_tmp3AE,_tmp3B2);
if(_tmp37D != 0)return _tmp37D;{
int _tmp37E=Cyc_Tcutil_typecmp(_tmp3AF,_tmp3AB);
if(_tmp37E != 0)return _tmp37E;
if(_tmp3B0 == _tmp3AC)return 0;
if(_tmp3B0 == 0  || _tmp3AC == 0)
({void*_tmp37F=0U;({struct _dyneither_ptr _tmpAD2=({const char*_tmp380="missing expression in array index";_tag_dyneither(_tmp380,sizeof(char),34U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpAD2,_tag_dyneither(_tmp37F,sizeof(void*),0U));});});
# 2054
return((int(*)(int(*cmp)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*a1,struct Cyc_Absyn_Exp*a2))Cyc_Tcutil_star_cmp)(Cyc_Evexp_const_exp_cmp,_tmp3B0,_tmp3AC);};};}}else{goto _LL15;}case 5U: if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp372.f2)->tag == 5U){_LLB: _tmp3B4=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp372.f1)->f1;_tmp3B3=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp372.f2)->f1;_LLC:
# 2057
 if(Cyc_Tcutil_unify(t1,t2))return 0;{
int r=Cyc_Tcutil_typecmp(_tmp3B4.ret_type,_tmp3B3.ret_type);
if(r != 0)return r;
r=Cyc_Tcutil_tqual_cmp(_tmp3B4.ret_tqual,_tmp3B3.ret_tqual);
if(r != 0)return r;{
struct Cyc_List_List*_tmp381=_tmp3B4.args;
struct Cyc_List_List*_tmp382=_tmp3B3.args;
for(0;_tmp381 != 0  && _tmp382 != 0;(_tmp381=_tmp381->tl,_tmp382=_tmp382->tl)){
struct _tuple10 _tmp383=*((struct _tuple10*)_tmp381->hd);struct _tuple10 _tmp384=_tmp383;struct Cyc_Absyn_Tqual _tmp38A;void*_tmp389;_LL18: _tmp38A=_tmp384.f2;_tmp389=_tmp384.f3;_LL19:;{
struct _tuple10 _tmp385=*((struct _tuple10*)_tmp382->hd);struct _tuple10 _tmp386=_tmp385;struct Cyc_Absyn_Tqual _tmp388;void*_tmp387;_LL1B: _tmp388=_tmp386.f2;_tmp387=_tmp386.f3;_LL1C:;
r=Cyc_Tcutil_tqual_cmp(_tmp38A,_tmp388);
if(r != 0)return r;
r=Cyc_Tcutil_typecmp(_tmp389,_tmp387);
if(r != 0)return r;};}
# 2072
if(_tmp381 != 0)return 1;
if(_tmp382 != 0)return - 1;
if(_tmp3B4.c_varargs  && !_tmp3B3.c_varargs)return 1;
if(!_tmp3B4.c_varargs  && _tmp3B3.c_varargs)return - 1;
if(_tmp3B4.cyc_varargs != 0 & _tmp3B3.cyc_varargs == 0)return 1;
if(_tmp3B4.cyc_varargs == 0 & _tmp3B3.cyc_varargs != 0)return - 1;
if(_tmp3B4.cyc_varargs != 0 & _tmp3B3.cyc_varargs != 0){
r=({struct Cyc_Absyn_Tqual _tmpAD3=((struct Cyc_Absyn_VarargInfo*)_check_null(_tmp3B4.cyc_varargs))->tq;Cyc_Tcutil_tqual_cmp(_tmpAD3,((struct Cyc_Absyn_VarargInfo*)_check_null(_tmp3B3.cyc_varargs))->tq);});
if(r != 0)return r;
r=Cyc_Tcutil_typecmp((_tmp3B4.cyc_varargs)->type,(_tmp3B3.cyc_varargs)->type);
if(r != 0)return r;
if((_tmp3B4.cyc_varargs)->inject  && !(_tmp3B3.cyc_varargs)->inject)return 1;
if(!(_tmp3B4.cyc_varargs)->inject  && (_tmp3B3.cyc_varargs)->inject)return - 1;}
# 2086
r=Cyc_Tcutil_star_cmp(Cyc_Tcutil_typecmp,_tmp3B4.effect,_tmp3B3.effect);
if(r != 0)return r;{
struct Cyc_List_List*_tmp38B=_tmp3B4.rgn_po;
struct Cyc_List_List*_tmp38C=_tmp3B3.rgn_po;
for(0;_tmp38B != 0  && _tmp38C != 0;(_tmp38B=_tmp38B->tl,_tmp38C=_tmp38C->tl)){
struct _tuple0 _tmp38D=*((struct _tuple0*)_tmp38B->hd);struct _tuple0 _tmp38E=_tmp38D;void*_tmp394;void*_tmp393;_LL1E: _tmp394=_tmp38E.f1;_tmp393=_tmp38E.f2;_LL1F:;{
struct _tuple0 _tmp38F=*((struct _tuple0*)_tmp38C->hd);struct _tuple0 _tmp390=_tmp38F;void*_tmp392;void*_tmp391;_LL21: _tmp392=_tmp390.f1;_tmp391=_tmp390.f2;_LL22:;
r=Cyc_Tcutil_typecmp(_tmp394,_tmp392);if(r != 0)return r;
r=Cyc_Tcutil_typecmp(_tmp393,_tmp391);if(r != 0)return r;};}
# 2096
if(_tmp38B != 0)return 1;
if(_tmp38C != 0)return - 1;
r=((int(*)(int(*cmp)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*a1,struct Cyc_Absyn_Exp*a2))Cyc_Tcutil_star_cmp)(Cyc_Evexp_const_exp_cmp,_tmp3B4.requires_clause,_tmp3B3.requires_clause);
if(r != 0)return r;
r=((int(*)(int(*cmp)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*a1,struct Cyc_Absyn_Exp*a2))Cyc_Tcutil_star_cmp)(Cyc_Evexp_const_exp_cmp,_tmp3B4.ensures_clause,_tmp3B3.ensures_clause);
if(r != 0)return r;
# 2104
({void*_tmp395=0U;({struct _dyneither_ptr _tmpAD4=({const char*_tmp396="typecmp: function type comparison should never get here!";_tag_dyneither(_tmp396,sizeof(char),57U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpAD4,_tag_dyneither(_tmp395,sizeof(void*),0U));});});};};};}else{goto _LL15;}case 6U: if(((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp372.f2)->tag == 6U){_LLD: _tmp3B6=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp372.f1)->f1;_tmp3B5=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp372.f2)->f1;_LLE:
# 2107
 return((int(*)(int(*cmp)(struct _tuple12*,struct _tuple12*),struct Cyc_List_List*l1,struct Cyc_List_List*l2))Cyc_List_list_cmp)(Cyc_Tcutil_tqual_type_cmp,_tmp3B5,_tmp3B6);}else{goto _LL15;}case 7U: if(((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp372.f2)->tag == 7U){_LLF: _tmp3BA=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp372.f1)->f1;_tmp3B9=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp372.f1)->f2;_tmp3B8=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp372.f2)->f1;_tmp3B7=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp372.f2)->f2;_LL10:
# 2110
 if((int)_tmp3B8 != (int)_tmp3BA){
if((int)_tmp3B8 == (int)0U)return - 1;else{
return 1;}}
return((int(*)(int(*cmp)(struct Cyc_Absyn_Aggrfield*,struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*l1,struct Cyc_List_List*l2))Cyc_List_list_cmp)(Cyc_Tcutil_aggrfield_cmp,_tmp3B7,_tmp3B9);}else{goto _LL15;}case 9U: if(((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp372.f2)->tag == 9U){_LL11: _tmp3BC=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp372.f1)->f1;_tmp3BB=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp372.f2)->f1;_LL12:
# 2115
 _tmp3BE=_tmp3BC;_tmp3BD=_tmp3BB;goto _LL14;}else{goto _LL15;}case 11U: if(((struct Cyc_Absyn_TypeofType_Absyn_Type_struct*)_tmp372.f2)->tag == 11U){_LL13: _tmp3BE=((struct Cyc_Absyn_TypeofType_Absyn_Type_struct*)_tmp372.f1)->f1;_tmp3BD=((struct Cyc_Absyn_TypeofType_Absyn_Type_struct*)_tmp372.f2)->f1;_LL14:
# 2117
 return Cyc_Evexp_const_exp_cmp(_tmp3BE,_tmp3BD);}else{goto _LL15;}default: _LL15: _LL16:
({void*_tmp397=0U;({struct _dyneither_ptr _tmpAD5=({const char*_tmp398="Unmatched case in typecmp";_tag_dyneither(_tmp398,sizeof(char),26U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpAD5,_tag_dyneither(_tmp397,sizeof(void*),0U));});});}_LL0:;};};}
# 2124
int Cyc_Tcutil_will_lose_precision(void*t1,void*t2){
t1=Cyc_Tcutil_compress(t1);
t2=Cyc_Tcutil_compress(t2);{
struct _tuple0 _tmp3BF=({struct _tuple0 _tmp99A;_tmp99A.f1=t1,_tmp99A.f2=t2;_tmp99A;});struct _tuple0 _tmp3C0=_tmp3BF;void*_tmp3C6;void*_tmp3C5;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3C0.f1)->tag == 0U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3C0.f2)->tag == 0U){_LL1: _tmp3C6=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3C0.f1)->f1;_tmp3C5=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3C0.f2)->f1;_LL2: {
# 2129
struct _tuple0 _tmp3C1=({struct _tuple0 _tmp999;_tmp999.f1=_tmp3C6,_tmp999.f2=_tmp3C5;_tmp999;});struct _tuple0 _tmp3C2=_tmp3C1;int _tmp3C4;int _tmp3C3;switch(*((int*)_tmp3C2.f1)){case 2U: switch(*((int*)_tmp3C2.f2)){case 2U: _LL6: _tmp3C4=((struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)_tmp3C2.f1)->f1;_tmp3C3=((struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)_tmp3C2.f2)->f1;_LL7:
# 2131
 return _tmp3C3 < _tmp3C4;case 1U: _LL8: _LL9:
 goto _LLB;case 4U: _LLA: _LLB:
 return 1;default: goto _LL26;}case 1U: switch(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp3C2.f1)->f2){case Cyc_Absyn_LongLong_sz: if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp3C2.f2)->tag == 1U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp3C2.f2)->f2 == Cyc_Absyn_LongLong_sz){_LLC: _LLD:
 return 0;}else{goto _LLE;}}else{_LLE: _LLF:
 return 1;}case Cyc_Absyn_Long_sz: switch(*((int*)_tmp3C2.f2)){case 1U: switch(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp3C2.f2)->f2){case Cyc_Absyn_Int_sz: _LL10: _LL11:
# 2138
 goto _LL13;case Cyc_Absyn_Short_sz: _LL18: _LL19:
# 2143
 goto _LL1B;case Cyc_Absyn_Char_sz: _LL1E: _LL1F:
# 2146
 goto _LL21;default: goto _LL26;}case 2U: if(((struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)_tmp3C2.f2)->f1 == 0){_LL14: _LL15:
# 2141
 goto _LL17;}else{goto _LL26;}default: goto _LL26;}case Cyc_Absyn_Int_sz: switch(*((int*)_tmp3C2.f2)){case 1U: switch(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp3C2.f2)->f2){case Cyc_Absyn_Long_sz: _LL12: _LL13:
# 2139
 return 0;case Cyc_Absyn_Short_sz: _LL1A: _LL1B:
# 2144
 goto _LL1D;case Cyc_Absyn_Char_sz: _LL20: _LL21:
# 2147
 goto _LL23;default: goto _LL26;}case 2U: if(((struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)_tmp3C2.f2)->f1 == 0){_LL16: _LL17:
# 2142
 goto _LL19;}else{goto _LL26;}default: goto _LL26;}case Cyc_Absyn_Short_sz: if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp3C2.f2)->tag == 1U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp3C2.f2)->f2 == Cyc_Absyn_Char_sz){_LL22: _LL23:
# 2148
 goto _LL25;}else{goto _LL26;}}else{goto _LL26;}default: goto _LL26;}case 4U: if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp3C2.f2)->tag == 1U)switch(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp3C2.f2)->f2){case Cyc_Absyn_Short_sz: _LL1C: _LL1D:
# 2145
 goto _LL1F;case Cyc_Absyn_Char_sz: _LL24: _LL25:
# 2149
 return 1;default: goto _LL26;}else{goto _LL26;}default: _LL26: _LL27:
# 2151
 return 0;}_LL5:;}}else{goto _LL3;}}else{_LL3: _LL4:
# 2153
 return 0;}_LL0:;};}
# 2159
int Cyc_Tcutil_coerce_list(struct Cyc_Tcenv_Tenv*te,void*t,struct Cyc_List_List*es){
# 2162
struct Cyc_Core_Opt*max_arith_type=0;
{struct Cyc_List_List*el=es;for(0;el != 0;el=el->tl){
void*t1=Cyc_Tcutil_compress((void*)_check_null(((struct Cyc_Absyn_Exp*)el->hd)->topt));
if(Cyc_Tcutil_is_arithmetic_type(t1)){
if(max_arith_type == 0  || 
Cyc_Tcutil_will_lose_precision(t1,(void*)max_arith_type->v))
max_arith_type=({struct Cyc_Core_Opt*_tmp3C7=_cycalloc(sizeof(*_tmp3C7));_tmp3C7->v=t1;_tmp3C7;});}}}
# 2171
if(max_arith_type != 0){
if(!Cyc_Tcutil_unify(t,(void*)max_arith_type->v))
return 0;}
# 2175
{struct Cyc_List_List*el=es;for(0;el != 0;el=el->tl){
if(!Cyc_Tcutil_coerce_assign(te,(struct Cyc_Absyn_Exp*)el->hd,t)){
({struct Cyc_String_pa_PrintArg_struct _tmp3CA=({struct Cyc_String_pa_PrintArg_struct _tmp99C;_tmp99C.tag=0U,({
struct _dyneither_ptr _tmpAD6=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(t));_tmp99C.f1=_tmpAD6;});_tmp99C;});struct Cyc_String_pa_PrintArg_struct _tmp3CB=({struct Cyc_String_pa_PrintArg_struct _tmp99B;_tmp99B.tag=0U,({struct _dyneither_ptr _tmpAD7=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string((void*)_check_null(((struct Cyc_Absyn_Exp*)el->hd)->topt)));_tmp99B.f1=_tmpAD7;});_tmp99B;});void*_tmp3C8[2U];_tmp3C8[0]=& _tmp3CA,_tmp3C8[1]=& _tmp3CB;({unsigned int _tmpAD9=((struct Cyc_Absyn_Exp*)el->hd)->loc;struct _dyneither_ptr _tmpAD8=({const char*_tmp3C9="type mismatch: expecting %s but found %s";_tag_dyneither(_tmp3C9,sizeof(char),41U);});Cyc_Tcutil_terr(_tmpAD9,_tmpAD8,_tag_dyneither(_tmp3C8,sizeof(void*),2U));});});
return 0;}}}
# 2181
return 1;}
# 2186
int Cyc_Tcutil_coerce_to_bool(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*e){
if(!Cyc_Tcutil_coerce_sint_type(te,e)){
void*_tmp3CC=Cyc_Tcutil_compress((void*)_check_null(e->topt));void*_tmp3CD=_tmp3CC;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp3CD)->tag == 3U){_LL1: _LL2:
 Cyc_Tcutil_unchecked_cast(te,e,Cyc_Absyn_uint_type,Cyc_Absyn_Other_coercion);goto _LL0;}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 2192
return 1;}
# 2196
int Cyc_Tcutil_coerce_uint_type(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*e){
if(Cyc_Tcutil_unify((void*)_check_null(e->topt),Cyc_Absyn_uint_type))
return 1;
# 2200
if(Cyc_Tcutil_is_integral_type((void*)_check_null(e->topt))){
if(Cyc_Tcutil_will_lose_precision((void*)_check_null(e->topt),Cyc_Absyn_uint_type))
({void*_tmp3CE=0U;({unsigned int _tmpADB=e->loc;struct _dyneither_ptr _tmpADA=({const char*_tmp3CF="integral size mismatch; conversion supplied";_tag_dyneither(_tmp3CF,sizeof(char),44U);});Cyc_Tcutil_warn(_tmpADB,_tmpADA,_tag_dyneither(_tmp3CE,sizeof(void*),0U));});});
Cyc_Tcutil_unchecked_cast(te,e,Cyc_Absyn_uint_type,Cyc_Absyn_No_coercion);
return 1;}
# 2206
return 0;}
# 2210
int Cyc_Tcutil_coerce_sint_type(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*e){
if(Cyc_Tcutil_unify((void*)_check_null(e->topt),Cyc_Absyn_sint_type))
return 1;
# 2214
if(Cyc_Tcutil_is_integral_type((void*)_check_null(e->topt))){
if(Cyc_Tcutil_will_lose_precision((void*)_check_null(e->topt),Cyc_Absyn_sint_type))
({void*_tmp3D0=0U;({unsigned int _tmpADD=e->loc;struct _dyneither_ptr _tmpADC=({const char*_tmp3D1="integral size mismatch; conversion supplied";_tag_dyneither(_tmp3D1,sizeof(char),44U);});Cyc_Tcutil_warn(_tmpADD,_tmpADC,_tag_dyneither(_tmp3D0,sizeof(void*),0U));});});
Cyc_Tcutil_unchecked_cast(te,e,Cyc_Absyn_sint_type,Cyc_Absyn_No_coercion);
return 1;}
# 2220
return 0;}
# 2225
int Cyc_Tcutil_force_type2bool(int desired,void*t){
Cyc_Tcutil_unify(desired?Cyc_Absyn_true_type: Cyc_Absyn_false_type,t);
return Cyc_Absyn_type2bool(desired,t);}
# 2231
void*Cyc_Tcutil_force_bounds_one(void*t){
({void*_tmpADE=t;Cyc_Tcutil_unify(_tmpADE,Cyc_Absyn_bounds_one());});
return Cyc_Tcutil_compress(t);}
# 2236
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_thin_bound(struct Cyc_List_List*ts){
void*_tmp3D2=Cyc_Tcutil_compress((void*)((struct Cyc_List_List*)_check_null(ts))->hd);
void*_tmp3D3=_tmp3D2;struct Cyc_Absyn_Exp*_tmp3D5;if(((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp3D3)->tag == 9U){_LL1: _tmp3D5=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp3D3)->f1;_LL2:
 return _tmp3D5;}else{_LL3: _LL4: {
# 2241
struct Cyc_Absyn_Exp*_tmp3D4=Cyc_Absyn_valueof_exp(_tmp3D2,0U);
_tmp3D4->topt=Cyc_Absyn_uint_type;
return _tmp3D4;}}_LL0:;}
# 2250
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_bounds_exp(void*def,void*b){
Cyc_Tcutil_unify(def,b);{
void*_tmp3D6=Cyc_Tcutil_compress(b);void*_tmp3D7=_tmp3D6;struct Cyc_List_List*_tmp3DB;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3D7)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3D7)->f1)){case 14U: _LL1: _LL2:
 return 0;case 13U: _LL3: _tmp3DB=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3D7)->f2;_LL4:
 return Cyc_Tcutil_get_thin_bound(_tmp3DB);default: goto _LL5;}else{_LL5: _LL6:
({struct Cyc_String_pa_PrintArg_struct _tmp3DA=({struct Cyc_String_pa_PrintArg_struct _tmp99D;_tmp99D.tag=0U,({struct _dyneither_ptr _tmpADF=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(b));_tmp99D.f1=_tmpADF;});_tmp99D;});void*_tmp3D8[1U];_tmp3D8[0]=& _tmp3DA;({struct _dyneither_ptr _tmpAE0=({const char*_tmp3D9="get_bounds_exp: %s";_tag_dyneither(_tmp3D9,sizeof(char),19U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpAE0,_tag_dyneither(_tmp3D8,sizeof(void*),1U));});});}_LL0:;};}
# 2259
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_ptr_bounds_exp(void*def,void*t){
void*_tmp3DC=Cyc_Tcutil_compress(t);void*_tmp3DD=_tmp3DC;void*_tmp3E1;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp3DD)->tag == 3U){_LL1: _tmp3E1=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp3DD)->f1).ptr_atts).bounds;_LL2:
# 2262
 return Cyc_Tcutil_get_bounds_exp(def,_tmp3E1);}else{_LL3: _LL4:
({struct Cyc_String_pa_PrintArg_struct _tmp3E0=({struct Cyc_String_pa_PrintArg_struct _tmp99E;_tmp99E.tag=0U,({struct _dyneither_ptr _tmpAE1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(t));_tmp99E.f1=_tmpAE1;});_tmp99E;});void*_tmp3DE[1U];_tmp3DE[0]=& _tmp3E0;({struct _dyneither_ptr _tmpAE2=({const char*_tmp3DF="get_ptr_bounds_exp not pointer: %s";_tag_dyneither(_tmp3DF,sizeof(char),35U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpAE2,_tag_dyneither(_tmp3DE,sizeof(void*),1U));});});}_LL0:;}
# 2268
void*Cyc_Tcutil_any_bool(struct Cyc_Tcenv_Tenv**tep){
if(tep != 0)
return Cyc_Absyn_new_evar(& Cyc_Tcutil_boolko,({struct Cyc_Core_Opt*_tmp3E2=_cycalloc(sizeof(*_tmp3E2));({struct Cyc_List_List*_tmpAE3=Cyc_Tcenv_lookup_type_vars(*tep);_tmp3E2->v=_tmpAE3;});_tmp3E2;}));else{
# 2272
return Cyc_Absyn_new_evar(& Cyc_Tcutil_boolko,({struct Cyc_Core_Opt*_tmp3E3=_cycalloc(sizeof(*_tmp3E3));_tmp3E3->v=0;_tmp3E3;}));}}
# 2275
void*Cyc_Tcutil_any_bounds(struct Cyc_Tcenv_Tenv**tep){
if(tep != 0)
return Cyc_Absyn_new_evar(& Cyc_Tcutil_ptrbko,({struct Cyc_Core_Opt*_tmp3E4=_cycalloc(sizeof(*_tmp3E4));({struct Cyc_List_List*_tmpAE4=Cyc_Tcenv_lookup_type_vars(*tep);_tmp3E4->v=_tmpAE4;});_tmp3E4;}));else{
# 2279
return Cyc_Absyn_new_evar(& Cyc_Tcutil_ptrbko,({struct Cyc_Core_Opt*_tmp3E5=_cycalloc(sizeof(*_tmp3E5));_tmp3E5->v=0;_tmp3E5;}));}}
# 2283
static int Cyc_Tcutil_ptrsubtype(struct Cyc_Tcenv_Tenv*te,struct Cyc_List_List*assume,void*t1,void*t2);struct _tuple24{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};
# 2291
int Cyc_Tcutil_silent_castable(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void*t1,void*t2){
# 2293
t1=Cyc_Tcutil_compress(t1);
t2=Cyc_Tcutil_compress(t2);{
struct _tuple0 _tmp3E6=({struct _tuple0 _tmp9A2;_tmp9A2.f1=t1,_tmp9A2.f2=t2;_tmp9A2;});struct _tuple0 _tmp3E7=_tmp3E6;void*_tmp3FC;struct Cyc_Absyn_Tqual _tmp3FB;struct Cyc_Absyn_Exp*_tmp3FA;void*_tmp3F9;void*_tmp3F8;struct Cyc_Absyn_Tqual _tmp3F7;struct Cyc_Absyn_Exp*_tmp3F6;void*_tmp3F5;struct Cyc_Absyn_PtrInfo _tmp3F4;struct Cyc_Absyn_PtrInfo _tmp3F3;switch(*((int*)_tmp3E7.f1)){case 3U: if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp3E7.f2)->tag == 3U){_LL1: _tmp3F4=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp3E7.f1)->f1;_tmp3F3=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp3E7.f2)->f1;_LL2: {
# 2297
int okay=1;
# 2299
if(!Cyc_Tcutil_unify((_tmp3F4.ptr_atts).nullable,(_tmp3F3.ptr_atts).nullable))
# 2301
okay=!Cyc_Tcutil_force_type2bool(0,(_tmp3F4.ptr_atts).nullable);
# 2303
if(!Cyc_Tcutil_unify((_tmp3F4.ptr_atts).bounds,(_tmp3F3.ptr_atts).bounds)){
struct _tuple24 _tmp3E8=({struct _tuple24 _tmp99F;({struct Cyc_Absyn_Exp*_tmpAE8=({void*_tmpAE7=Cyc_Absyn_bounds_one();Cyc_Tcutil_get_bounds_exp(_tmpAE7,(_tmp3F4.ptr_atts).bounds);});_tmp99F.f1=_tmpAE8;}),({
struct Cyc_Absyn_Exp*_tmpAE6=({void*_tmpAE5=Cyc_Absyn_bounds_one();Cyc_Tcutil_get_bounds_exp(_tmpAE5,(_tmp3F3.ptr_atts).bounds);});_tmp99F.f2=_tmpAE6;});_tmp99F;});
# 2304
struct _tuple24 _tmp3E9=_tmp3E8;struct Cyc_Absyn_Exp*_tmp3ED;struct Cyc_Absyn_Exp*_tmp3EC;if(_tmp3E9.f2 == 0){_LLA: _LLB:
# 2307
 okay=1;goto _LL9;}else{if(_tmp3E9.f1 == 0){_LLC: _LLD:
# 2310
 if(Cyc_Tcutil_force_type2bool(0,(_tmp3F4.ptr_atts).zero_term) && ({
void*_tmpAE9=Cyc_Absyn_bounds_one();Cyc_Tcutil_unify(_tmpAE9,(_tmp3F3.ptr_atts).bounds);}))
goto _LL9;
okay=0;
goto _LL9;}else{_LLE: _tmp3ED=_tmp3E9.f1;_tmp3EC=_tmp3E9.f2;_LLF:
# 2316
 okay=okay  && ({struct Cyc_Absyn_Exp*_tmpAEA=(struct Cyc_Absyn_Exp*)_check_null(_tmp3EC);Cyc_Evexp_lte_const_exp(_tmpAEA,(struct Cyc_Absyn_Exp*)_check_null(_tmp3ED));});
# 2320
if(!Cyc_Tcutil_force_type2bool(0,(_tmp3F3.ptr_atts).zero_term))
({void*_tmp3EA=0U;({unsigned int _tmpAEC=loc;struct _dyneither_ptr _tmpAEB=({const char*_tmp3EB="implicit cast to shorter array";_tag_dyneither(_tmp3EB,sizeof(char),31U);});Cyc_Tcutil_warn(_tmpAEC,_tmpAEB,_tag_dyneither(_tmp3EA,sizeof(void*),0U));});});
goto _LL9;}}_LL9:;}
# 2327
okay=okay  && (!(_tmp3F4.elt_tq).real_const  || (_tmp3F3.elt_tq).real_const);
# 2330
if(!Cyc_Tcutil_unify((_tmp3F4.ptr_atts).rgn,(_tmp3F3.ptr_atts).rgn)){
if(Cyc_Tcenv_region_outlives(te,(_tmp3F4.ptr_atts).rgn,(_tmp3F3.ptr_atts).rgn)){
if(Cyc_Tcutil_warn_region_coerce)
({struct Cyc_String_pa_PrintArg_struct _tmp3F0=({struct Cyc_String_pa_PrintArg_struct _tmp9A1;_tmp9A1.tag=0U,({
struct _dyneither_ptr _tmpAED=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string((_tmp3F4.ptr_atts).rgn));_tmp9A1.f1=_tmpAED;});_tmp9A1;});struct Cyc_String_pa_PrintArg_struct _tmp3F1=({struct Cyc_String_pa_PrintArg_struct _tmp9A0;_tmp9A0.tag=0U,({
struct _dyneither_ptr _tmpAEE=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string((_tmp3F3.ptr_atts).rgn));_tmp9A0.f1=_tmpAEE;});_tmp9A0;});void*_tmp3EE[2U];_tmp3EE[0]=& _tmp3F0,_tmp3EE[1]=& _tmp3F1;({unsigned int _tmpAF0=loc;struct _dyneither_ptr _tmpAEF=({const char*_tmp3EF="implicit cast from region %s to region %s";_tag_dyneither(_tmp3EF,sizeof(char),42U);});Cyc_Tcutil_warn(_tmpAF0,_tmpAEF,_tag_dyneither(_tmp3EE,sizeof(void*),2U));});});}else{
okay=0;}}
# 2340
okay=okay  && (Cyc_Tcutil_unify((_tmp3F4.ptr_atts).zero_term,(_tmp3F3.ptr_atts).zero_term) || 
# 2342
Cyc_Tcutil_force_type2bool(1,(_tmp3F4.ptr_atts).zero_term) && (_tmp3F3.elt_tq).real_const);{
# 2350
int _tmp3F2=
({void*_tmpAF1=Cyc_Absyn_bounds_one();Cyc_Tcutil_unify(_tmpAF1,(_tmp3F3.ptr_atts).bounds);}) && !
Cyc_Tcutil_force_type2bool(0,(_tmp3F3.ptr_atts).zero_term);
# 2356
okay=okay  && (Cyc_Tcutil_unify(_tmp3F4.elt_type,_tmp3F3.elt_type) || 
(_tmp3F2  && ((_tmp3F3.elt_tq).real_const  || Cyc_Tcutil_kind_leq(& Cyc_Tcutil_ak,Cyc_Tcutil_type_kind(_tmp3F3.elt_type)))) && Cyc_Tcutil_ptrsubtype(te,0,_tmp3F4.elt_type,_tmp3F3.elt_type));
# 2359
return okay;};}}else{goto _LL7;}case 4U: if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp3E7.f2)->tag == 4U){_LL3: _tmp3FC=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp3E7.f1)->f1).elt_type;_tmp3FB=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp3E7.f1)->f1).tq;_tmp3FA=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp3E7.f1)->f1).num_elts;_tmp3F9=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp3E7.f1)->f1).zero_term;_tmp3F8=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp3E7.f2)->f1).elt_type;_tmp3F7=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp3E7.f2)->f1).tq;_tmp3F6=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp3E7.f2)->f1).num_elts;_tmp3F5=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp3E7.f2)->f1).zero_term;_LL4: {
# 2363
int okay;
# 2366
okay=Cyc_Tcutil_unify(_tmp3F9,_tmp3F5) && (
(_tmp3FA != 0  && _tmp3F6 != 0) && Cyc_Evexp_same_const_exp(_tmp3FA,_tmp3F6));
# 2369
return(okay  && Cyc_Tcutil_unify(_tmp3FC,_tmp3F8)) && (!_tmp3FB.real_const  || _tmp3F7.real_const);}}else{goto _LL7;}case 0U: if(((struct Cyc_Absyn_TagCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3E7.f1)->f1)->tag == 4U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3E7.f2)->tag == 0U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3E7.f2)->f1)->tag == 1U){_LL5: _LL6:
# 2371
 return 0;}else{goto _LL7;}}else{goto _LL7;}}else{goto _LL7;}default: _LL7: _LL8:
# 2373
 return Cyc_Tcutil_unify(t1,t2);}_LL0:;};}
# 2377
void*Cyc_Tcutil_pointer_elt_type(void*t){
void*_tmp3FD=Cyc_Tcutil_compress(t);void*_tmp3FE=_tmp3FD;void*_tmp401;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp3FE)->tag == 3U){_LL1: _tmp401=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp3FE)->f1).elt_type;_LL2:
 return _tmp401;}else{_LL3: _LL4:
({void*_tmp3FF=0U;({struct _dyneither_ptr _tmpAF2=({const char*_tmp400="pointer_elt_type";_tag_dyneither(_tmp400,sizeof(char),17U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpAF2,_tag_dyneither(_tmp3FF,sizeof(void*),0U));});});}_LL0:;}
# 2383
void*Cyc_Tcutil_pointer_region(void*t){
void*_tmp402=Cyc_Tcutil_compress(t);void*_tmp403=_tmp402;struct Cyc_Absyn_PtrAtts*_tmp406;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp403)->tag == 3U){_LL1: _tmp406=(struct Cyc_Absyn_PtrAtts*)&(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp403)->f1).ptr_atts;_LL2:
 return _tmp406->rgn;}else{_LL3: _LL4:
({void*_tmp404=0U;({struct _dyneither_ptr _tmpAF3=({const char*_tmp405="pointer_elt_type";_tag_dyneither(_tmp405,sizeof(char),17U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpAF3,_tag_dyneither(_tmp404,sizeof(void*),0U));});});}_LL0:;}
# 2390
int Cyc_Tcutil_rgn_of_pointer(void*t,void**rgn){
void*_tmp407=Cyc_Tcutil_compress(t);void*_tmp408=_tmp407;void*_tmp409;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp408)->tag == 3U){_LL1: _tmp409=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp408)->f1).ptr_atts).rgn;_LL2:
# 2393
*rgn=_tmp409;
return 1;}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 2402
static int Cyc_Tcutil_admits_zero(void*t){
void*_tmp40A=Cyc_Tcutil_compress(t);void*_tmp40B=_tmp40A;void*_tmp40D;void*_tmp40C;switch(*((int*)_tmp40B)){case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp40B)->f1)){case 1U: _LL1: _LL2:
 goto _LL4;case 2U: _LL3: _LL4:
 return 1;default: goto _LL7;}case 3U: _LL5: _tmp40D=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp40B)->f1).ptr_atts).nullable;_tmp40C=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp40B)->f1).ptr_atts).bounds;_LL6:
# 2410
 return !Cyc_Tcutil_unify(Cyc_Absyn_fat_bound_type,_tmp40C) && Cyc_Tcutil_force_type2bool(0,_tmp40D);default: _LL7: _LL8:
 return 0;}_LL0:;}
# 2416
int Cyc_Tcutil_is_zero(struct Cyc_Absyn_Exp*e){
void*_tmp40E=e->r;void*_tmp40F=_tmp40E;void*_tmp413;struct Cyc_Absyn_Exp*_tmp412;struct _dyneither_ptr _tmp411;switch(*((int*)_tmp40F)){case 0U: switch(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp40F)->f1).Wchar_c).tag){case 5U: if((((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp40F)->f1).Int_c).val).f2 == 0){_LL1: _LL2:
 goto _LL4;}else{goto _LLF;}case 2U: if((((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp40F)->f1).Char_c).val).f2 == 0){_LL3: _LL4:
 goto _LL6;}else{goto _LLF;}case 4U: if((((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp40F)->f1).Short_c).val).f2 == 0){_LL5: _LL6:
 goto _LL8;}else{goto _LLF;}case 6U: if((((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp40F)->f1).LongLong_c).val).f2 == 0){_LL7: _LL8:
 goto _LLA;}else{goto _LLF;}case 3U: _LLB: _tmp411=((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp40F)->f1).Wchar_c).val;_LLC: {
# 2425
unsigned long _tmp410=Cyc_strlen((struct _dyneither_ptr)_tmp411);
int i=0;
if(_tmp410 >= (unsigned long)2  && (int)*((const char*)_check_dyneither_subscript(_tmp411,sizeof(char),0))== (int)'\\'){
if((int)*((const char*)_check_dyneither_subscript(_tmp411,sizeof(char),1))== (int)'0')i=2;else{
if(((int)*((const char*)_check_dyneither_subscript(_tmp411,sizeof(char),1))== (int)'x'  && _tmp410 >= (unsigned long)3) && (int)*((const char*)_check_dyneither_subscript(_tmp411,sizeof(char),2))== (int)'0')i=3;else{
return 0;}}
for(0;(unsigned long)i < _tmp410;++ i){
if((int)*((const char*)_check_dyneither_subscript(_tmp411,sizeof(char),i))!= (int)'0')return 0;}
return 1;}else{
# 2435
return 0;}}default: goto _LLF;}case 2U: _LL9: _LLA:
# 2423
 return 1;case 14U: _LLD: _tmp413=(void*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp40F)->f1;_tmp412=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp40F)->f2;_LLE:
# 2436
 return Cyc_Tcutil_is_zero(_tmp412) && Cyc_Tcutil_admits_zero(_tmp413);default: _LLF: _LL10:
 return 0;}_LL0:;}
# 2441
struct Cyc_Absyn_Kind Cyc_Tcutil_rk={Cyc_Absyn_RgnKind,Cyc_Absyn_Aliasable};
struct Cyc_Absyn_Kind Cyc_Tcutil_ak={Cyc_Absyn_AnyKind,Cyc_Absyn_Aliasable};
struct Cyc_Absyn_Kind Cyc_Tcutil_bk={Cyc_Absyn_BoxKind,Cyc_Absyn_Aliasable};
struct Cyc_Absyn_Kind Cyc_Tcutil_mk={Cyc_Absyn_MemKind,Cyc_Absyn_Aliasable};
struct Cyc_Absyn_Kind Cyc_Tcutil_ik={Cyc_Absyn_IntKind,Cyc_Absyn_Aliasable};
struct Cyc_Absyn_Kind Cyc_Tcutil_ek={Cyc_Absyn_EffKind,Cyc_Absyn_Aliasable};
struct Cyc_Absyn_Kind Cyc_Tcutil_boolk={Cyc_Absyn_BoolKind,Cyc_Absyn_Aliasable};
struct Cyc_Absyn_Kind Cyc_Tcutil_ptrbk={Cyc_Absyn_PtrBndKind,Cyc_Absyn_Aliasable};
# 2450
struct Cyc_Absyn_Kind Cyc_Tcutil_trk={Cyc_Absyn_RgnKind,Cyc_Absyn_Top};
struct Cyc_Absyn_Kind Cyc_Tcutil_tak={Cyc_Absyn_AnyKind,Cyc_Absyn_Top};
struct Cyc_Absyn_Kind Cyc_Tcutil_tbk={Cyc_Absyn_BoxKind,Cyc_Absyn_Top};
struct Cyc_Absyn_Kind Cyc_Tcutil_tmk={Cyc_Absyn_MemKind,Cyc_Absyn_Top};
# 2455
struct Cyc_Absyn_Kind Cyc_Tcutil_urk={Cyc_Absyn_RgnKind,Cyc_Absyn_Unique};
struct Cyc_Absyn_Kind Cyc_Tcutil_uak={Cyc_Absyn_AnyKind,Cyc_Absyn_Unique};
struct Cyc_Absyn_Kind Cyc_Tcutil_ubk={Cyc_Absyn_BoxKind,Cyc_Absyn_Unique};
struct Cyc_Absyn_Kind Cyc_Tcutil_umk={Cyc_Absyn_MemKind,Cyc_Absyn_Unique};
# 2460
struct Cyc_Core_Opt Cyc_Tcutil_rko={(void*)& Cyc_Tcutil_rk};
struct Cyc_Core_Opt Cyc_Tcutil_ako={(void*)& Cyc_Tcutil_ak};
struct Cyc_Core_Opt Cyc_Tcutil_bko={(void*)& Cyc_Tcutil_bk};
struct Cyc_Core_Opt Cyc_Tcutil_mko={(void*)& Cyc_Tcutil_mk};
struct Cyc_Core_Opt Cyc_Tcutil_iko={(void*)& Cyc_Tcutil_ik};
struct Cyc_Core_Opt Cyc_Tcutil_eko={(void*)& Cyc_Tcutil_ek};
struct Cyc_Core_Opt Cyc_Tcutil_boolko={(void*)& Cyc_Tcutil_boolk};
struct Cyc_Core_Opt Cyc_Tcutil_ptrbko={(void*)& Cyc_Tcutil_ptrbk};
# 2469
struct Cyc_Core_Opt Cyc_Tcutil_trko={(void*)& Cyc_Tcutil_trk};
struct Cyc_Core_Opt Cyc_Tcutil_tako={(void*)& Cyc_Tcutil_tak};
struct Cyc_Core_Opt Cyc_Tcutil_tbko={(void*)& Cyc_Tcutil_tbk};
struct Cyc_Core_Opt Cyc_Tcutil_tmko={(void*)& Cyc_Tcutil_tmk};
# 2474
struct Cyc_Core_Opt Cyc_Tcutil_urko={(void*)& Cyc_Tcutil_urk};
struct Cyc_Core_Opt Cyc_Tcutil_uako={(void*)& Cyc_Tcutil_uak};
struct Cyc_Core_Opt Cyc_Tcutil_ubko={(void*)& Cyc_Tcutil_ubk};
struct Cyc_Core_Opt Cyc_Tcutil_umko={(void*)& Cyc_Tcutil_umk};
# 2479
struct Cyc_Core_Opt*Cyc_Tcutil_kind_to_opt(struct Cyc_Absyn_Kind*ka){
struct Cyc_Absyn_Kind*_tmp414=ka;enum Cyc_Absyn_KindQual _tmp41D;enum Cyc_Absyn_AliasQual _tmp41C;_LL1: _tmp41D=_tmp414->kind;_tmp41C=_tmp414->aliasqual;_LL2:;
{enum Cyc_Absyn_AliasQual _tmp415=_tmp41C;switch(_tmp415){case Cyc_Absyn_Aliasable: _LL4: _LL5: {
# 2483
enum Cyc_Absyn_KindQual _tmp416=_tmp41D;switch(_tmp416){case Cyc_Absyn_AnyKind: _LLB: _LLC:
 return& Cyc_Tcutil_ako;case Cyc_Absyn_MemKind: _LLD: _LLE:
 return& Cyc_Tcutil_mko;case Cyc_Absyn_BoxKind: _LLF: _LL10:
 return& Cyc_Tcutil_bko;case Cyc_Absyn_RgnKind: _LL11: _LL12:
 return& Cyc_Tcutil_rko;case Cyc_Absyn_EffKind: _LL13: _LL14:
 return& Cyc_Tcutil_eko;case Cyc_Absyn_IntKind: _LL15: _LL16:
 return& Cyc_Tcutil_iko;case Cyc_Absyn_BoolKind: _LL17: _LL18:
 return& Cyc_Tcutil_bko;default: _LL19: _LL1A:
 return& Cyc_Tcutil_ptrbko;}_LLA:;}case Cyc_Absyn_Unique: _LL6: _LL7:
# 2494
{enum Cyc_Absyn_KindQual _tmp417=_tmp41D;switch(_tmp417){case Cyc_Absyn_AnyKind: _LL1C: _LL1D:
 return& Cyc_Tcutil_uako;case Cyc_Absyn_MemKind: _LL1E: _LL1F:
 return& Cyc_Tcutil_umko;case Cyc_Absyn_BoxKind: _LL20: _LL21:
 return& Cyc_Tcutil_ubko;case Cyc_Absyn_RgnKind: _LL22: _LL23:
 return& Cyc_Tcutil_urko;default: _LL24: _LL25:
 goto _LL1B;}_LL1B:;}
# 2501
goto _LL3;default: _LL8: _LL9:
# 2503
{enum Cyc_Absyn_KindQual _tmp418=_tmp41D;switch(_tmp418){case Cyc_Absyn_AnyKind: _LL27: _LL28:
 return& Cyc_Tcutil_tako;case Cyc_Absyn_MemKind: _LL29: _LL2A:
 return& Cyc_Tcutil_tmko;case Cyc_Absyn_BoxKind: _LL2B: _LL2C:
 return& Cyc_Tcutil_tbko;case Cyc_Absyn_RgnKind: _LL2D: _LL2E:
 return& Cyc_Tcutil_trko;default: _LL2F: _LL30:
 goto _LL26;}_LL26:;}
# 2510
goto _LL3;}_LL3:;}
# 2512
({struct Cyc_String_pa_PrintArg_struct _tmp41B=({struct Cyc_String_pa_PrintArg_struct _tmp9A3;_tmp9A3.tag=0U,({struct _dyneither_ptr _tmpAF4=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_kind2string(ka));_tmp9A3.f1=_tmpAF4;});_tmp9A3;});void*_tmp419[1U];_tmp419[0]=& _tmp41B;({struct _dyneither_ptr _tmpAF5=({const char*_tmp41A="kind_to_opt: bad kind %s\n";_tag_dyneither(_tmp41A,sizeof(char),26U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpAF5,_tag_dyneither(_tmp419,sizeof(void*),1U));});});}
# 2515
void*Cyc_Tcutil_kind_to_bound(struct Cyc_Absyn_Kind*k){
return(void*)({struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*_tmp41E=_cycalloc(sizeof(*_tmp41E));_tmp41E->tag=0U,_tmp41E->f1=k;_tmp41E;});}
# 2518
struct Cyc_Core_Opt*Cyc_Tcutil_kind_to_bound_opt(struct Cyc_Absyn_Kind*k){
return({struct Cyc_Core_Opt*_tmp41F=_cycalloc(sizeof(*_tmp41F));({void*_tmpAF6=Cyc_Tcutil_kind_to_bound(k);_tmp41F->v=_tmpAF6;});_tmp41F;});}
# 2524
int Cyc_Tcutil_zero_to_null(struct Cyc_Tcenv_Tenv*te,void*t2,struct Cyc_Absyn_Exp*e1){
if(Cyc_Tcutil_is_pointer_type(t2) && Cyc_Tcutil_is_zero(e1)){
({void*_tmpAF7=(void*)({struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*_tmp420=_cycalloc(sizeof(*_tmp420));_tmp420->tag=0U,_tmp420->f1=Cyc_Absyn_Null_c;_tmp420;});e1->r=_tmpAF7;});{
struct Cyc_Core_Opt*_tmp421=Cyc_Tcenv_lookup_opt_type_vars(te);
void*_tmp422=Cyc_Absyn_pointer_type(({struct Cyc_Absyn_PtrInfo _tmp9A6;({void*_tmpAFC=Cyc_Absyn_new_evar(& Cyc_Tcutil_ako,_tmp421);_tmp9A6.elt_type=_tmpAFC;}),({
struct Cyc_Absyn_Tqual _tmpAFB=Cyc_Absyn_empty_tqual(0U);_tmp9A6.elt_tq=_tmpAFB;}),
({void*_tmpAFA=Cyc_Absyn_new_evar(& Cyc_Tcutil_trko,_tmp421);(_tmp9A6.ptr_atts).rgn=_tmpAFA;}),(_tmp9A6.ptr_atts).nullable=Cyc_Absyn_true_type,({
# 2532
void*_tmpAF9=Cyc_Absyn_new_evar(& Cyc_Tcutil_ptrbko,_tmp421);(_tmp9A6.ptr_atts).bounds=_tmpAF9;}),({
void*_tmpAF8=Cyc_Absyn_new_evar(& Cyc_Tcutil_boolko,_tmp421);(_tmp9A6.ptr_atts).zero_term=_tmpAF8;}),(_tmp9A6.ptr_atts).ptrloc=0;_tmp9A6;}));
e1->topt=_tmp422;{
int bogus=0;
int retv=Cyc_Tcutil_coerce_arg(te,e1,t2,& bogus);
if(bogus != 0)
({struct Cyc_String_pa_PrintArg_struct _tmp425=({struct Cyc_String_pa_PrintArg_struct _tmp9A5;_tmp9A5.tag=0U,({
struct _dyneither_ptr _tmpAFD=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_exp2string(e1));_tmp9A5.f1=_tmpAFD;});_tmp9A5;});struct Cyc_String_pa_PrintArg_struct _tmp426=({struct Cyc_String_pa_PrintArg_struct _tmp9A4;_tmp9A4.tag=0U,({struct _dyneither_ptr _tmpAFE=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Position_string_of_segment(e1->loc));_tmp9A4.f1=_tmpAFE;});_tmp9A4;});void*_tmp423[2U];_tmp423[0]=& _tmp425,_tmp423[1]=& _tmp426;({struct _dyneither_ptr _tmpAFF=({const char*_tmp424="zero_to_null resulted in an alias coercion on %s at %s\n";_tag_dyneither(_tmp424,sizeof(char),56U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpAFF,_tag_dyneither(_tmp423,sizeof(void*),2U));});});
return retv;};};}
# 2542
return 0;}
# 2545
int Cyc_Tcutil_warn_alias_coerce=0;
# 2551
struct _tuple14 Cyc_Tcutil_insert_alias(struct Cyc_Absyn_Exp*e,void*e_type){
static struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct rgn_kb={0U,& Cyc_Tcutil_rk};
# 2555
static int counter=0;
struct _tuple2*v=({struct _tuple2*_tmp43A=_cycalloc(sizeof(*_tmp43A));_tmp43A->f1=Cyc_Absyn_Loc_n,({struct _dyneither_ptr*_tmpB02=({struct _dyneither_ptr*_tmp439=_cycalloc(sizeof(*_tmp439));({struct _dyneither_ptr _tmpB01=(struct _dyneither_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp438=({struct Cyc_Int_pa_PrintArg_struct _tmp9A8;_tmp9A8.tag=1U,_tmp9A8.f1=(unsigned long)counter ++;_tmp9A8;});void*_tmp436[1U];_tmp436[0]=& _tmp438;({struct _dyneither_ptr _tmpB00=({const char*_tmp437="__aliasvar%d";_tag_dyneither(_tmp437,sizeof(char),13U);});Cyc_aprintf(_tmpB00,_tag_dyneither(_tmp436,sizeof(void*),1U));});});*_tmp439=_tmpB01;});_tmp439;});_tmp43A->f2=_tmpB02;});_tmp43A;});
struct Cyc_Absyn_Vardecl*vd=Cyc_Absyn_new_vardecl(0U,v,e_type,e);
struct Cyc_Absyn_Exp*ve=({void*_tmpB03=(void*)({struct Cyc_Absyn_Local_b_Absyn_Binding_struct*_tmp435=_cycalloc(sizeof(*_tmp435));_tmp435->tag=4U,_tmp435->f1=vd;_tmp435;});Cyc_Absyn_varb_exp(_tmpB03,e->loc);});
# 2564
struct Cyc_Absyn_Tvar*tv=Cyc_Tcutil_new_tvar((void*)& rgn_kb);
# 2566
{void*_tmp427=Cyc_Tcutil_compress(e_type);void*_tmp428=_tmp427;void*_tmp434;struct Cyc_Absyn_Tqual _tmp433;void*_tmp432;void*_tmp431;void*_tmp430;void*_tmp42F;struct Cyc_Absyn_PtrLoc*_tmp42E;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp428)->tag == 3U){_LL1: _tmp434=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp428)->f1).elt_type;_tmp433=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp428)->f1).elt_tq;_tmp432=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp428)->f1).ptr_atts).rgn;_tmp431=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp428)->f1).ptr_atts).nullable;_tmp430=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp428)->f1).ptr_atts).bounds;_tmp42F=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp428)->f1).ptr_atts).zero_term;_tmp42E=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp428)->f1).ptr_atts).ptrloc;_LL2:
# 2568
{void*_tmp429=Cyc_Tcutil_compress(_tmp432);void*_tmp42A=_tmp429;void**_tmp42D;struct Cyc_Core_Opt*_tmp42C;if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp42A)->tag == 1U){_LL6: _tmp42D=(void**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp42A)->f2;_tmp42C=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp42A)->f4;_LL7: {
# 2570
void*_tmp42B=Cyc_Absyn_var_type(tv);
*_tmp42D=_tmp42B;
goto _LL5;}}else{_LL8: _LL9:
 goto _LL5;}_LL5:;}
# 2575
goto _LL0;}else{_LL3: _LL4:
 goto _LL0;}_LL0:;}
# 2579
e->topt=0;
vd->initializer=0;{
# 2583
struct Cyc_Absyn_Decl*d=Cyc_Absyn_alias_decl(tv,vd,e,e->loc);
# 2585
return({struct _tuple14 _tmp9A7;_tmp9A7.f1=d,_tmp9A7.f2=ve;_tmp9A7;});};}
# 2590
static int Cyc_Tcutil_can_insert_alias(struct Cyc_Absyn_Exp*e,void*e_type,void*wants_type,unsigned int loc){
# 2593
if((Cyc_Tcutil_is_noalias_path(e) && 
Cyc_Tcutil_is_noalias_pointer(e_type,0)) && 
Cyc_Tcutil_is_pointer_type(e_type)){
# 2598
void*_tmp43B=Cyc_Tcutil_compress(wants_type);void*_tmp43C=_tmp43B;void*_tmp43E;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp43C)->tag == 3U){_LL1: _tmp43E=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp43C)->f1).ptr_atts).rgn;_LL2:
# 2600
 if(Cyc_Tcutil_is_heap_rgn_type(_tmp43E))return 0;{
struct Cyc_Absyn_Kind*_tmp43D=Cyc_Tcutil_type_kind(_tmp43E);
return(int)_tmp43D->kind == (int)Cyc_Absyn_RgnKind  && (int)_tmp43D->aliasqual == (int)Cyc_Absyn_Aliasable;};}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 2606
return 0;}
# 2610
int Cyc_Tcutil_coerce_arg(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*e,void*t2,int*alias_coercion){
void*t1=Cyc_Tcutil_compress((void*)_check_null(e->topt));
enum Cyc_Absyn_Coercion c;
int do_alias_coercion=0;
# 2615
if(Cyc_Tcutil_unify(t1,t2))return 1;
# 2617
if(Cyc_Tcutil_is_arithmetic_type(t2) && Cyc_Tcutil_is_arithmetic_type(t1)){
# 2619
if(Cyc_Tcutil_will_lose_precision(t1,t2))
({struct Cyc_String_pa_PrintArg_struct _tmp441=({struct Cyc_String_pa_PrintArg_struct _tmp9AA;_tmp9AA.tag=0U,({
struct _dyneither_ptr _tmpB04=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(t1));_tmp9AA.f1=_tmpB04;});_tmp9AA;});struct Cyc_String_pa_PrintArg_struct _tmp442=({struct Cyc_String_pa_PrintArg_struct _tmp9A9;_tmp9A9.tag=0U,({struct _dyneither_ptr _tmpB05=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(t2));_tmp9A9.f1=_tmpB05;});_tmp9A9;});void*_tmp43F[2U];_tmp43F[0]=& _tmp441,_tmp43F[1]=& _tmp442;({unsigned int _tmpB07=e->loc;struct _dyneither_ptr _tmpB06=({const char*_tmp440="integral size mismatch; %s -> %s conversion supplied";_tag_dyneither(_tmp440,sizeof(char),53U);});Cyc_Tcutil_warn(_tmpB07,_tmpB06,_tag_dyneither(_tmp43F,sizeof(void*),2U));});});
Cyc_Tcutil_unchecked_cast(te,e,t2,Cyc_Absyn_No_coercion);
return 1;}else{
# 2627
if(Cyc_Tcutil_can_insert_alias(e,t1,t2,e->loc)){
if(Cyc_Tcutil_warn_alias_coerce)
({struct Cyc_String_pa_PrintArg_struct _tmp445=({struct Cyc_String_pa_PrintArg_struct _tmp9AD;_tmp9AD.tag=0U,({
struct _dyneither_ptr _tmpB08=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_exp2string(e));_tmp9AD.f1=_tmpB08;});_tmp9AD;});struct Cyc_String_pa_PrintArg_struct _tmp446=({struct Cyc_String_pa_PrintArg_struct _tmp9AC;_tmp9AC.tag=0U,({struct _dyneither_ptr _tmpB09=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(t1));_tmp9AC.f1=_tmpB09;});_tmp9AC;});struct Cyc_String_pa_PrintArg_struct _tmp447=({struct Cyc_String_pa_PrintArg_struct _tmp9AB;_tmp9AB.tag=0U,({struct _dyneither_ptr _tmpB0A=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(t2));_tmp9AB.f1=_tmpB0A;});_tmp9AB;});void*_tmp443[3U];_tmp443[0]=& _tmp445,_tmp443[1]=& _tmp446,_tmp443[2]=& _tmp447;({unsigned int _tmpB0C=e->loc;struct _dyneither_ptr _tmpB0B=({const char*_tmp444="implicit alias coercion for %s:%s to %s";_tag_dyneither(_tmp444,sizeof(char),40U);});Cyc_Tcutil_warn(_tmpB0C,_tmpB0B,_tag_dyneither(_tmp443,sizeof(void*),3U));});});
*alias_coercion=1;}
# 2634
if(Cyc_Tcutil_silent_castable(te,e->loc,t1,t2)){
Cyc_Tcutil_unchecked_cast(te,e,t2,Cyc_Absyn_Other_coercion);
return 1;}else{
if(Cyc_Tcutil_zero_to_null(te,t2,e))
return 1;else{
if((int)(c=Cyc_Tcutil_castable(te,e->loc,t1,t2))!= (int)Cyc_Absyn_Unknown_coercion){
# 2641
if((int)c != (int)1U)Cyc_Tcutil_unchecked_cast(te,e,t2,c);
if((int)c != (int)2U)
({struct Cyc_String_pa_PrintArg_struct _tmp44A=({struct Cyc_String_pa_PrintArg_struct _tmp9AF;_tmp9AF.tag=0U,({
struct _dyneither_ptr _tmpB0D=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(t1));_tmp9AF.f1=_tmpB0D;});_tmp9AF;});struct Cyc_String_pa_PrintArg_struct _tmp44B=({struct Cyc_String_pa_PrintArg_struct _tmp9AE;_tmp9AE.tag=0U,({struct _dyneither_ptr _tmpB0E=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(t2));_tmp9AE.f1=_tmpB0E;});_tmp9AE;});void*_tmp448[2U];_tmp448[0]=& _tmp44A,_tmp448[1]=& _tmp44B;({unsigned int _tmpB10=e->loc;struct _dyneither_ptr _tmpB0F=({const char*_tmp449="implicit cast from %s to %s";_tag_dyneither(_tmp449,sizeof(char),28U);});Cyc_Tcutil_warn(_tmpB10,_tmpB0F,_tag_dyneither(_tmp448,sizeof(void*),2U));});});
return 1;}else{
# 2647
return 0;}}}}}
# 2654
int Cyc_Tcutil_coerce_assign(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*e,void*t){
# 2657
int bogus=0;
return Cyc_Tcutil_coerce_arg(te,e,t,& bogus);}
# 2671 "tcutil.cyc"
static struct Cyc_List_List*Cyc_Tcutil_flatten_type(struct _RegionHandle*r,int flatten,struct Cyc_Tcenv_Tenv*te,void*t1);struct _tuple25{struct Cyc_List_List*f1;struct _RegionHandle*f2;struct Cyc_Tcenv_Tenv*f3;int f4;};
# 2675
static struct Cyc_List_List*Cyc_Tcutil_flatten_type_f(struct _tuple25*env,struct Cyc_Absyn_Aggrfield*x){
# 2678
struct _tuple25 _tmp44C=*env;struct _tuple25 _tmp44D=_tmp44C;struct Cyc_List_List*_tmp455;struct _RegionHandle*_tmp454;struct Cyc_Tcenv_Tenv*_tmp453;int _tmp452;_LL1: _tmp455=_tmp44D.f1;_tmp454=_tmp44D.f2;_tmp453=_tmp44D.f3;_tmp452=_tmp44D.f4;_LL2:;{
# 2680
void*_tmp44E=_tmp455 == 0?x->type: Cyc_Tcutil_rsubstitute(_tmp454,_tmp455,x->type);
struct Cyc_List_List*_tmp44F=Cyc_Tcutil_flatten_type(_tmp454,_tmp452,_tmp453,_tmp44E);
if(((int(*)(struct Cyc_List_List*x))Cyc_List_length)(_tmp44F)== 1)
return({struct Cyc_List_List*_tmp451=_region_malloc(_tmp454,sizeof(*_tmp451));({struct _tuple12*_tmpB11=({struct _tuple12*_tmp450=_region_malloc(_tmp454,sizeof(*_tmp450));_tmp450->f1=x->tq,_tmp450->f2=_tmp44E;_tmp450;});_tmp451->hd=_tmpB11;}),_tmp451->tl=0;_tmp451;});else{
return _tmp44F;}};}struct _tuple26{struct _RegionHandle*f1;struct Cyc_Tcenv_Tenv*f2;int f3;};
# 2686
static struct Cyc_List_List*Cyc_Tcutil_rcopy_tqt(struct _tuple26*env,struct _tuple12*x){
# 2688
struct _tuple26 _tmp456=*env;struct _tuple26 _tmp457=_tmp456;struct _RegionHandle*_tmp461;struct Cyc_Tcenv_Tenv*_tmp460;int _tmp45F;_LL1: _tmp461=_tmp457.f1;_tmp460=_tmp457.f2;_tmp45F=_tmp457.f3;_LL2:;{
struct _tuple12 _tmp458=*x;struct _tuple12 _tmp459=_tmp458;struct Cyc_Absyn_Tqual _tmp45E;void*_tmp45D;_LL4: _tmp45E=_tmp459.f1;_tmp45D=_tmp459.f2;_LL5:;{
struct Cyc_List_List*_tmp45A=Cyc_Tcutil_flatten_type(_tmp461,_tmp45F,_tmp460,_tmp45D);
if(((int(*)(struct Cyc_List_List*x))Cyc_List_length)(_tmp45A)== 1)
return({struct Cyc_List_List*_tmp45C=_region_malloc(_tmp461,sizeof(*_tmp45C));({struct _tuple12*_tmpB12=({struct _tuple12*_tmp45B=_region_malloc(_tmp461,sizeof(*_tmp45B));_tmp45B->f1=_tmp45E,_tmp45B->f2=_tmp45D;_tmp45B;});_tmp45C->hd=_tmpB12;}),_tmp45C->tl=0;_tmp45C;});else{
return _tmp45A;}};};}
# 2695
static struct Cyc_List_List*Cyc_Tcutil_flatten_type(struct _RegionHandle*r,int flatten,struct Cyc_Tcenv_Tenv*te,void*t1){
# 2699
if(flatten){
t1=Cyc_Tcutil_compress(t1);{
void*_tmp462=t1;struct Cyc_List_List*_tmp481;struct Cyc_List_List*_tmp480;struct Cyc_Absyn_Aggrdecl*_tmp47F;struct Cyc_List_List*_tmp47E;switch(*((int*)_tmp462)){case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp462)->f1)){case 0U: _LL1: _LL2:
 return 0;case 20U: if(((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp462)->f1)->f1).KnownAggr).tag == 2){_LL5: _tmp47F=*((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp462)->f1)->f1).KnownAggr).val;_tmp47E=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp462)->f2;_LL6:
# 2717
 if((((int)_tmp47F->kind == (int)Cyc_Absyn_UnionA  || _tmp47F->impl == 0) || ((struct Cyc_Absyn_AggrdeclImpl*)_check_null(_tmp47F->impl))->exist_vars != 0) || ((struct Cyc_Absyn_AggrdeclImpl*)_check_null(_tmp47F->impl))->rgn_po != 0)
# 2719
return({struct Cyc_List_List*_tmp46D=_region_malloc(r,sizeof(*_tmp46D));({struct _tuple12*_tmpB14=({struct _tuple12*_tmp46C=_region_malloc(r,sizeof(*_tmp46C));({struct Cyc_Absyn_Tqual _tmpB13=Cyc_Absyn_empty_tqual(0U);_tmp46C->f1=_tmpB13;}),_tmp46C->f2=t1;_tmp46C;});_tmp46D->hd=_tmpB14;}),_tmp46D->tl=0;_tmp46D;});{
struct Cyc_List_List*_tmp46E=((struct Cyc_List_List*(*)(struct _RegionHandle*r1,struct _RegionHandle*r2,struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_rzip)(r,r,_tmp47F->tvs,_tmp47E);
struct _tuple25 env=({struct _tuple25 _tmp9B0;_tmp9B0.f1=_tmp46E,_tmp9B0.f2=r,_tmp9B0.f3=te,_tmp9B0.f4=flatten;_tmp9B0;});
struct Cyc_List_List*_tmp46F=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(_tmp47F->impl))->fields;struct Cyc_List_List*_tmp470=_tmp46F;struct Cyc_Absyn_Aggrfield*_tmp476;struct Cyc_List_List*_tmp475;if(_tmp470 == 0){_LL11: _LL12:
 return 0;}else{_LL13: _tmp476=(struct Cyc_Absyn_Aggrfield*)_tmp470->hd;_tmp475=_tmp470->tl;_LL14: {
# 2725
struct Cyc_List_List*_tmp471=Cyc_Tcutil_flatten_type_f(& env,_tmp476);
env.f4=0;{
struct Cyc_List_List*_tmp472=((struct Cyc_List_List*(*)(struct _RegionHandle*,struct Cyc_List_List*(*f)(struct _tuple25*,struct Cyc_Absyn_Aggrfield*),struct _tuple25*env,struct Cyc_List_List*x))Cyc_List_rmap_c)(r,Cyc_Tcutil_flatten_type_f,& env,_tmp475);
struct Cyc_List_List*_tmp473=({struct Cyc_List_List*_tmp474=_region_malloc(r,sizeof(*_tmp474));_tmp474->hd=_tmp471,_tmp474->tl=_tmp472;_tmp474;});
return((struct Cyc_List_List*(*)(struct _RegionHandle*,struct Cyc_List_List*x))Cyc_List_rflatten)(r,_tmp473);};}}_LL10:;};}else{goto _LL9;}default: goto _LL9;}case 6U: _LL3: _tmp480=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp462)->f1;_LL4: {
# 2704
struct _tuple26 _tmp463=({struct _tuple26 _tmp9B1;_tmp9B1.f1=r,_tmp9B1.f2=te,_tmp9B1.f3=flatten;_tmp9B1;});
# 2706
struct Cyc_List_List*_tmp464=_tmp480;struct _tuple12*_tmp46B;struct Cyc_List_List*_tmp46A;if(_tmp464 == 0){_LLC: _LLD:
 return 0;}else{_LLE: _tmp46B=(struct _tuple12*)_tmp464->hd;_tmp46A=_tmp464->tl;_LLF: {
# 2709
struct Cyc_List_List*_tmp465=Cyc_Tcutil_rcopy_tqt(& _tmp463,_tmp46B);
_tmp463.f3=0;{
struct Cyc_List_List*_tmp466=((struct Cyc_List_List*(*)(struct _RegionHandle*,struct Cyc_List_List*(*f)(struct _tuple26*,struct _tuple12*),struct _tuple26*env,struct Cyc_List_List*x))Cyc_List_rmap_c)(r,Cyc_Tcutil_rcopy_tqt,& _tmp463,_tmp480);
struct Cyc_List_List*_tmp467=({struct Cyc_List_List*_tmp469=_region_malloc(r,sizeof(*_tmp469));_tmp469->hd=_tmp465,_tmp469->tl=_tmp466;_tmp469;});
return({struct _RegionHandle*_tmpB15=r;((struct Cyc_List_List*(*)(struct _RegionHandle*,struct Cyc_List_List*x))Cyc_List_rflatten)(_tmpB15,({struct Cyc_List_List*_tmp468=_region_malloc(r,sizeof(*_tmp468));_tmp468->hd=_tmp465,_tmp468->tl=_tmp466;_tmp468;}));});};}}_LLB:;}case 7U: if(((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp462)->f1 == Cyc_Absyn_StructA){_LL7: _tmp481=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp462)->f2;_LL8: {
# 2732
struct _tuple25 env=({struct _tuple25 _tmp9B2;_tmp9B2.f1=0,_tmp9B2.f2=r,_tmp9B2.f3=te,_tmp9B2.f4=flatten;_tmp9B2;});
struct Cyc_List_List*_tmp477=_tmp481;struct Cyc_Absyn_Aggrfield*_tmp47D;struct Cyc_List_List*_tmp47C;if(_tmp477 == 0){_LL16: _LL17:
 return 0;}else{_LL18: _tmp47D=(struct Cyc_Absyn_Aggrfield*)_tmp477->hd;_tmp47C=_tmp477->tl;_LL19: {
# 2736
struct Cyc_List_List*_tmp478=Cyc_Tcutil_flatten_type_f(& env,_tmp47D);
env.f4=0;{
struct Cyc_List_List*_tmp479=((struct Cyc_List_List*(*)(struct _RegionHandle*,struct Cyc_List_List*(*f)(struct _tuple25*,struct Cyc_Absyn_Aggrfield*),struct _tuple25*env,struct Cyc_List_List*x))Cyc_List_rmap_c)(r,Cyc_Tcutil_flatten_type_f,& env,_tmp47C);
struct Cyc_List_List*_tmp47A=({struct Cyc_List_List*_tmp47B=_region_malloc(r,sizeof(*_tmp47B));_tmp47B->hd=_tmp478,_tmp47B->tl=_tmp479;_tmp47B;});
return((struct Cyc_List_List*(*)(struct _RegionHandle*,struct Cyc_List_List*x))Cyc_List_rflatten)(r,_tmp47A);};}}_LL15:;}}else{goto _LL9;}default: _LL9: _LLA:
# 2742
 goto _LL0;}_LL0:;};}
# 2745
return({struct Cyc_List_List*_tmp483=_region_malloc(r,sizeof(*_tmp483));({struct _tuple12*_tmpB17=({struct _tuple12*_tmp482=_region_malloc(r,sizeof(*_tmp482));({struct Cyc_Absyn_Tqual _tmpB16=Cyc_Absyn_empty_tqual(0U);_tmp482->f1=_tmpB16;}),_tmp482->f2=t1;_tmp482;});_tmp483->hd=_tmpB17;}),_tmp483->tl=0;_tmp483;});}
# 2749
static int Cyc_Tcutil_sub_attributes(struct Cyc_List_List*a1,struct Cyc_List_List*a2){
{struct Cyc_List_List*t=a1;for(0;t != 0;t=t->tl){
void*_tmp484=(void*)t->hd;void*_tmp485=_tmp484;switch(*((int*)_tmp485)){case 23U: _LL1: _LL2:
 goto _LL4;case 4U: _LL3: _LL4:
 goto _LL6;case 20U: _LL5: _LL6:
# 2755
 continue;default: _LL7: _LL8:
# 2757
 if(!((int(*)(int(*pred)(void*,void*),void*env,struct Cyc_List_List*x))Cyc_List_exists_c)(Cyc_Tcutil_equal_att,(void*)t->hd,a2))return 0;}_LL0:;}}
# 2760
for(0;a2 != 0;a2=a2->tl){
if(!((int(*)(int(*pred)(void*,void*),void*env,struct Cyc_List_List*x))Cyc_List_exists_c)(Cyc_Tcutil_equal_att,(void*)a2->hd,a1))return 0;}
# 2763
return 1;}
# 2766
static int Cyc_Tcutil_isomorphic(void*t1,void*t2){
struct _tuple0 _tmp486=({struct _tuple0 _tmp9B3;({void*_tmpB19=Cyc_Tcutil_compress(t1);_tmp9B3.f1=_tmpB19;}),({void*_tmpB18=Cyc_Tcutil_compress(t2);_tmp9B3.f2=_tmpB18;});_tmp9B3;});struct _tuple0 _tmp487=_tmp486;enum Cyc_Absyn_Size_of _tmp489;enum Cyc_Absyn_Size_of _tmp488;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp487.f1)->tag == 0U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp487.f1)->f1)->tag == 1U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp487.f2)->tag == 0U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp487.f2)->f1)->tag == 1U){_LL1: _tmp489=((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp487.f1)->f1)->f2;_tmp488=((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp487.f2)->f1)->f2;_LL2:
# 2769
 return((int)_tmp489 == (int)_tmp488  || (int)_tmp489 == (int)2U  && (int)_tmp488 == (int)3U) || 
(int)_tmp489 == (int)3U  && (int)_tmp488 == (int)Cyc_Absyn_Int_sz;}else{goto _LL3;}}else{goto _LL3;}}else{goto _LL3;}}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 2777
int Cyc_Tcutil_subtype(struct Cyc_Tcenv_Tenv*te,struct Cyc_List_List*assume,void*t1,void*t2){
# 2780
if(Cyc_Tcutil_unify(t1,t2))return 1;
{struct Cyc_List_List*a=assume;for(0;a != 0;a=a->tl){
if(Cyc_Tcutil_unify(t1,(*((struct _tuple0*)a->hd)).f1) && Cyc_Tcutil_unify(t2,(*((struct _tuple0*)a->hd)).f2))
return 1;}}
t1=Cyc_Tcutil_compress(t1);
t2=Cyc_Tcutil_compress(t2);{
struct _tuple0 _tmp48A=({struct _tuple0 _tmp9B4;_tmp9B4.f1=t1,_tmp9B4.f2=t2;_tmp9B4;});struct _tuple0 _tmp48B=_tmp48A;struct Cyc_Absyn_FnInfo _tmp4B5;struct Cyc_Absyn_FnInfo _tmp4B4;struct Cyc_Absyn_Datatypedecl*_tmp4B3;struct Cyc_Absyn_Datatypefield*_tmp4B2;struct Cyc_List_List*_tmp4B1;struct Cyc_Absyn_Datatypedecl*_tmp4B0;struct Cyc_List_List*_tmp4AF;void*_tmp4AE;struct Cyc_Absyn_Tqual _tmp4AD;void*_tmp4AC;void*_tmp4AB;void*_tmp4AA;void*_tmp4A9;void*_tmp4A8;struct Cyc_Absyn_Tqual _tmp4A7;void*_tmp4A6;void*_tmp4A5;void*_tmp4A4;void*_tmp4A3;switch(*((int*)_tmp48B.f1)){case 3U: if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp48B.f2)->tag == 3U){_LL1: _tmp4AE=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp48B.f1)->f1).elt_type;_tmp4AD=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp48B.f1)->f1).elt_tq;_tmp4AC=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp48B.f1)->f1).ptr_atts).rgn;_tmp4AB=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp48B.f1)->f1).ptr_atts).nullable;_tmp4AA=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp48B.f1)->f1).ptr_atts).bounds;_tmp4A9=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp48B.f1)->f1).ptr_atts).zero_term;_tmp4A8=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp48B.f2)->f1).elt_type;_tmp4A7=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp48B.f2)->f1).elt_tq;_tmp4A6=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp48B.f2)->f1).ptr_atts).rgn;_tmp4A5=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp48B.f2)->f1).ptr_atts).nullable;_tmp4A4=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp48B.f2)->f1).ptr_atts).bounds;_tmp4A3=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp48B.f2)->f1).ptr_atts).zero_term;_LL2:
# 2792
 if(_tmp4AD.real_const  && !_tmp4A7.real_const)
return 0;
# 2795
if((!Cyc_Tcutil_unify(_tmp4AB,_tmp4A5) && 
Cyc_Absyn_type2bool(0,_tmp4AB)) && !Cyc_Absyn_type2bool(0,_tmp4A5))
return 0;
# 2799
if((Cyc_Tcutil_unify(_tmp4A9,_tmp4A3) && !
Cyc_Absyn_type2bool(0,_tmp4A9)) && Cyc_Absyn_type2bool(0,_tmp4A3))
return 0;
# 2803
if((!Cyc_Tcutil_unify(_tmp4AC,_tmp4A6) && !Cyc_Tcenv_region_outlives(te,_tmp4AC,_tmp4A6)) && !
Cyc_Tcutil_subtype(te,assume,_tmp4AC,_tmp4A6))
return 0;
# 2807
if(!Cyc_Tcutil_unify(_tmp4AA,_tmp4A4)){
struct Cyc_Absyn_Exp*_tmp48C=({void*_tmpB1A=Cyc_Absyn_bounds_one();Cyc_Tcutil_get_bounds_exp(_tmpB1A,_tmp4AA);});
struct Cyc_Absyn_Exp*_tmp48D=({void*_tmpB1B=Cyc_Absyn_bounds_one();Cyc_Tcutil_get_bounds_exp(_tmpB1B,_tmp4A4);});
if(_tmp48C != _tmp48D){
if((_tmp48C == 0  || _tmp48D == 0) || !Cyc_Evexp_lte_const_exp(_tmp48D,_tmp48D))
return 0;}}
# 2817
if(!_tmp4A7.real_const  && _tmp4AD.real_const){
if(!Cyc_Tcutil_kind_leq(& Cyc_Tcutil_ak,Cyc_Tcutil_type_kind(_tmp4A8)))
return 0;}{
# 2823
int _tmp48E=
({void*_tmpB1C=_tmp4A4;Cyc_Tcutil_unify(_tmpB1C,Cyc_Absyn_bounds_one());}) && !Cyc_Tcutil_force_type2bool(0,_tmp4A3);
# 2828
return(_tmp48E  && ({struct Cyc_Tcenv_Tenv*_tmpB20=te;struct Cyc_List_List*_tmpB1F=({struct Cyc_List_List*_tmp490=_cycalloc(sizeof(*_tmp490));({struct _tuple0*_tmpB1D=({struct _tuple0*_tmp48F=_cycalloc(sizeof(*_tmp48F));_tmp48F->f1=t1,_tmp48F->f2=t2;_tmp48F;});_tmp490->hd=_tmpB1D;}),_tmp490->tl=assume;_tmp490;});void*_tmpB1E=_tmp4AE;Cyc_Tcutil_ptrsubtype(_tmpB20,_tmpB1F,_tmpB1E,_tmp4A8);}) || Cyc_Tcutil_unify(_tmp4AE,_tmp4A8)) || Cyc_Tcutil_isomorphic(_tmp4AE,_tmp4A8);};}else{goto _LL7;}case 0U: if(((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp48B.f1)->f1)->tag == 19U){if(((((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp48B.f1)->f1)->f1).KnownDatatypefield).tag == 2){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp48B.f2)->tag == 0U){if(((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp48B.f2)->f1)->tag == 18U){if(((((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp48B.f2)->f1)->f1).KnownDatatype).tag == 2){_LL3: _tmp4B3=(((((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp48B.f1)->f1)->f1).KnownDatatypefield).val).f1;_tmp4B2=(((((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp48B.f1)->f1)->f1).KnownDatatypefield).val).f2;_tmp4B1=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp48B.f1)->f2;_tmp4B0=*((((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp48B.f2)->f1)->f1).KnownDatatype).val;_tmp4AF=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp48B.f2)->f2;_LL4:
# 2835
 if(_tmp4B3 != _tmp4B0  && Cyc_Absyn_qvar_cmp(_tmp4B3->name,_tmp4B0->name)!= 0)return 0;
# 2837
if(({int _tmpB21=((int(*)(struct Cyc_List_List*x))Cyc_List_length)(_tmp4B1);_tmpB21 != ((int(*)(struct Cyc_List_List*x))Cyc_List_length)(_tmp4AF);}))return 0;
for(0;_tmp4B1 != 0;(_tmp4B1=_tmp4B1->tl,_tmp4AF=_tmp4AF->tl)){
if(!Cyc_Tcutil_unify((void*)_tmp4B1->hd,(void*)((struct Cyc_List_List*)_check_null(_tmp4AF))->hd))return 0;}
return 1;}else{goto _LL7;}}else{goto _LL7;}}else{goto _LL7;}}else{goto _LL7;}}else{goto _LL7;}case 5U: if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp48B.f2)->tag == 5U){_LL5: _tmp4B5=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp48B.f1)->f1;_tmp4B4=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp48B.f2)->f1;_LL6:
# 2844
 if(_tmp4B5.tvars != 0  || _tmp4B4.tvars != 0){
struct Cyc_List_List*_tmp491=_tmp4B5.tvars;
struct Cyc_List_List*_tmp492=_tmp4B4.tvars;
if(({int _tmpB22=((int(*)(struct Cyc_List_List*x))Cyc_List_length)(_tmp491);_tmpB22 != ((int(*)(struct Cyc_List_List*x))Cyc_List_length)(_tmp492);}))return 0;{
struct Cyc_List_List*inst=0;
while(_tmp491 != 0){
if(!Cyc_Tcutil_unify_kindbound(((struct Cyc_Absyn_Tvar*)_tmp491->hd)->kind,((struct Cyc_Absyn_Tvar*)((struct Cyc_List_List*)_check_null(_tmp492))->hd)->kind))return 0;
inst=({struct Cyc_List_List*_tmp494=_cycalloc(sizeof(*_tmp494));({struct _tuple15*_tmpB24=({struct _tuple15*_tmp493=_cycalloc(sizeof(*_tmp493));_tmp493->f1=(struct Cyc_Absyn_Tvar*)_tmp492->hd,({void*_tmpB23=Cyc_Absyn_var_type((struct Cyc_Absyn_Tvar*)_tmp491->hd);_tmp493->f2=_tmpB23;});_tmp493;});_tmp494->hd=_tmpB24;}),_tmp494->tl=inst;_tmp494;});
_tmp491=_tmp491->tl;
_tmp492=_tmp492->tl;}
# 2855
if(inst != 0){
_tmp4B5.tvars=0;
_tmp4B4.tvars=0;
return({struct Cyc_Tcenv_Tenv*_tmpB27=te;struct Cyc_List_List*_tmpB26=assume;void*_tmpB25=(void*)({struct Cyc_Absyn_FnType_Absyn_Type_struct*_tmp495=_cycalloc(sizeof(*_tmp495));_tmp495->tag=5U,_tmp495->f1=_tmp4B5;_tmp495;});Cyc_Tcutil_subtype(_tmpB27,_tmpB26,_tmpB25,(void*)({struct Cyc_Absyn_FnType_Absyn_Type_struct*_tmp496=_cycalloc(sizeof(*_tmp496));_tmp496->tag=5U,_tmp496->f1=_tmp4B4;_tmp496;}));});}};}
# 2862
if(!Cyc_Tcutil_subtype(te,assume,_tmp4B5.ret_type,_tmp4B4.ret_type))return 0;{
struct Cyc_List_List*_tmp497=_tmp4B5.args;
struct Cyc_List_List*_tmp498=_tmp4B4.args;
# 2867
if(({int _tmpB28=((int(*)(struct Cyc_List_List*x))Cyc_List_length)(_tmp497);_tmpB28 != ((int(*)(struct Cyc_List_List*x))Cyc_List_length)(_tmp498);}))return 0;
# 2869
for(0;_tmp497 != 0;(_tmp497=_tmp497->tl,_tmp498=_tmp498->tl)){
struct _tuple10 _tmp499=*((struct _tuple10*)_tmp497->hd);struct _tuple10 _tmp49A=_tmp499;struct Cyc_Absyn_Tqual _tmp4A0;void*_tmp49F;_LLA: _tmp4A0=_tmp49A.f2;_tmp49F=_tmp49A.f3;_LLB:;{
struct _tuple10 _tmp49B=*((struct _tuple10*)((struct Cyc_List_List*)_check_null(_tmp498))->hd);struct _tuple10 _tmp49C=_tmp49B;struct Cyc_Absyn_Tqual _tmp49E;void*_tmp49D;_LLD: _tmp49E=_tmp49C.f2;_tmp49D=_tmp49C.f3;_LLE:;
# 2873
if(_tmp49E.real_const  && !_tmp4A0.real_const  || !Cyc_Tcutil_subtype(te,assume,_tmp49D,_tmp49F))
return 0;};}
# 2877
if(_tmp4B5.c_varargs != _tmp4B4.c_varargs)return 0;
if(_tmp4B5.cyc_varargs != 0  && _tmp4B4.cyc_varargs != 0){
struct Cyc_Absyn_VarargInfo _tmp4A1=*_tmp4B5.cyc_varargs;
struct Cyc_Absyn_VarargInfo _tmp4A2=*_tmp4B4.cyc_varargs;
# 2882
if((_tmp4A2.tq).real_const  && !(_tmp4A1.tq).real_const  || !
Cyc_Tcutil_subtype(te,assume,_tmp4A2.type,_tmp4A1.type))
return 0;}else{
if(_tmp4B5.cyc_varargs != 0  || _tmp4B4.cyc_varargs != 0)return 0;}
# 2887
if(!({void*_tmpB29=(void*)_check_null(_tmp4B5.effect);Cyc_Tcutil_subset_effect(1,_tmpB29,(void*)_check_null(_tmp4B4.effect));}))return 0;
# 2889
if(!Cyc_Tcutil_sub_rgnpo(_tmp4B5.rgn_po,_tmp4B4.rgn_po))return 0;
# 2891
if(!Cyc_Tcutil_sub_attributes(_tmp4B5.attributes,_tmp4B4.attributes))return 0;
# 2893
if(!Cyc_Tcutil_check_logical_implication(_tmp4B4.requires_relns,_tmp4B5.requires_relns))
return 0;
# 2896
if(!Cyc_Tcutil_check_logical_implication(_tmp4B5.ensures_relns,_tmp4B4.ensures_relns))
return 0;
# 2899
return 1;};}else{goto _LL7;}default: _LL7: _LL8:
 return 0;}_LL0:;};}
# 2911 "tcutil.cyc"
static int Cyc_Tcutil_ptrsubtype(struct Cyc_Tcenv_Tenv*te,struct Cyc_List_List*assume,void*t1,void*t2){
# 2913
struct Cyc_List_List*tqs1=Cyc_Tcutil_flatten_type(Cyc_Core_heap_region,1,te,t1);
struct Cyc_List_List*tqs2=Cyc_Tcutil_flatten_type(Cyc_Core_heap_region,1,te,t2);
for(0;tqs2 != 0;(tqs2=tqs2->tl,tqs1=tqs1->tl)){
if(tqs1 == 0)return 0;{
struct _tuple12*_tmp4B6=(struct _tuple12*)tqs1->hd;struct _tuple12*_tmp4B7=_tmp4B6;struct Cyc_Absyn_Tqual _tmp4BD;void*_tmp4BC;_LL1: _tmp4BD=_tmp4B7->f1;_tmp4BC=_tmp4B7->f2;_LL2:;{
struct _tuple12*_tmp4B8=(struct _tuple12*)tqs2->hd;struct _tuple12*_tmp4B9=_tmp4B8;struct Cyc_Absyn_Tqual _tmp4BB;void*_tmp4BA;_LL4: _tmp4BB=_tmp4B9->f1;_tmp4BA=_tmp4B9->f2;_LL5:;
# 2920
if(_tmp4BD.real_const  && !_tmp4BB.real_const)return 0;
# 2922
if((_tmp4BB.real_const  || Cyc_Tcutil_kind_leq(& Cyc_Tcutil_ak,Cyc_Tcutil_type_kind(_tmp4BA))) && 
Cyc_Tcutil_subtype(te,assume,_tmp4BC,_tmp4BA))
# 2925
continue;
# 2927
if(Cyc_Tcutil_unify(_tmp4BC,_tmp4BA))
# 2929
continue;
# 2931
if(Cyc_Tcutil_isomorphic(_tmp4BC,_tmp4BA))
# 2933
continue;
# 2936
return 0;};};}
# 2938
return 1;}
# 2943
enum Cyc_Absyn_Coercion Cyc_Tcutil_castable(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void*t1,void*t2){
if(Cyc_Tcutil_unify(t1,t2))
return Cyc_Absyn_No_coercion;
t1=Cyc_Tcutil_compress(t1);
t2=Cyc_Tcutil_compress(t2);
# 2949
{void*_tmp4BE=t2;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4BE)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4BE)->f1)){case 0U: _LL1: _LL2:
 return Cyc_Absyn_No_coercion;case 1U: switch(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4BE)->f1)->f2){case Cyc_Absyn_Int_sz: _LL3: _LL4:
# 2952
 goto _LL6;case Cyc_Absyn_Long_sz: _LL5: _LL6:
# 2954
 if((int)(Cyc_Tcutil_type_kind(t1))->kind == (int)Cyc_Absyn_BoxKind)return Cyc_Absyn_Other_coercion;
goto _LL0;default: goto _LL7;}default: goto _LL7;}else{_LL7: _LL8:
 goto _LL0;}_LL0:;}{
# 2958
void*_tmp4BF=t1;void*_tmp4E4;struct Cyc_Absyn_Enumdecl*_tmp4E3;void*_tmp4E2;struct Cyc_Absyn_Tqual _tmp4E1;struct Cyc_Absyn_Exp*_tmp4E0;void*_tmp4DF;void*_tmp4DE;struct Cyc_Absyn_Tqual _tmp4DD;void*_tmp4DC;void*_tmp4DB;void*_tmp4DA;void*_tmp4D9;switch(*((int*)_tmp4BF)){case 3U: _LLA: _tmp4DE=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4BF)->f1).elt_type;_tmp4DD=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4BF)->f1).elt_tq;_tmp4DC=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4BF)->f1).ptr_atts).rgn;_tmp4DB=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4BF)->f1).ptr_atts).nullable;_tmp4DA=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4BF)->f1).ptr_atts).bounds;_tmp4D9=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4BF)->f1).ptr_atts).zero_term;_LLB:
# 2967
{void*_tmp4C0=t2;void*_tmp4CF;struct Cyc_Absyn_Tqual _tmp4CE;void*_tmp4CD;void*_tmp4CC;void*_tmp4CB;void*_tmp4CA;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4C0)->tag == 3U){_LL19: _tmp4CF=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4C0)->f1).elt_type;_tmp4CE=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4C0)->f1).elt_tq;_tmp4CD=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4C0)->f1).ptr_atts).rgn;_tmp4CC=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4C0)->f1).ptr_atts).nullable;_tmp4CB=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4C0)->f1).ptr_atts).bounds;_tmp4CA=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4C0)->f1).ptr_atts).zero_term;_LL1A: {
# 2971
enum Cyc_Absyn_Coercion coercion=3U;
struct Cyc_List_List*_tmp4C1=({struct Cyc_List_List*_tmp4C9=_cycalloc(sizeof(*_tmp4C9));({struct _tuple0*_tmpB2A=({struct _tuple0*_tmp4C8=_cycalloc(sizeof(*_tmp4C8));_tmp4C8->f1=t1,_tmp4C8->f2=t2;_tmp4C8;});_tmp4C9->hd=_tmpB2A;}),_tmp4C9->tl=0;_tmp4C9;});
int _tmp4C2=_tmp4CE.real_const  || !_tmp4DD.real_const;
# 2985 "tcutil.cyc"
int _tmp4C3=
({void*_tmpB2B=_tmp4CB;Cyc_Tcutil_unify(_tmpB2B,Cyc_Absyn_bounds_one());}) && !Cyc_Tcutil_force_type2bool(0,_tmp4CA);
# 2988
int _tmp4C4=_tmp4C2  && (
((_tmp4C3  && Cyc_Tcutil_ptrsubtype(te,_tmp4C1,_tmp4DE,_tmp4CF) || 
Cyc_Tcutil_unify(_tmp4DE,_tmp4CF)) || Cyc_Tcutil_isomorphic(_tmp4DE,_tmp4CF)) || Cyc_Tcutil_unify(_tmp4CF,Cyc_Absyn_void_type));
# 2992
Cyc_Tcutil_t1_failure=t1;
Cyc_Tcutil_t2_failure=t2;{
int zeroterm_ok=Cyc_Tcutil_unify(_tmp4D9,_tmp4CA) || !Cyc_Absyn_type2bool(0,_tmp4CA);
# 2996
int _tmp4C5=_tmp4C4?0:((Cyc_Tcutil_is_bits_only_type(_tmp4DE) && Cyc_Tcutil_is_char_type(_tmp4CF)) && !
Cyc_Tcutil_force_type2bool(0,_tmp4CA)) && (
_tmp4CE.real_const  || !_tmp4DD.real_const);
int bounds_ok=Cyc_Tcutil_unify(_tmp4DA,_tmp4CB);
if(!bounds_ok  && !_tmp4C5){
struct Cyc_Absyn_Exp*_tmp4C6=({void*_tmpB2C=Cyc_Absyn_bounds_one();Cyc_Tcutil_get_bounds_exp(_tmpB2C,_tmp4DA);});
struct Cyc_Absyn_Exp*_tmp4C7=({void*_tmpB2D=Cyc_Absyn_bounds_one();Cyc_Tcutil_get_bounds_exp(_tmpB2D,_tmp4CB);});
if((_tmp4C6 != 0  && _tmp4C7 != 0) && Cyc_Evexp_lte_const_exp(_tmp4C7,_tmp4C6))
bounds_ok=1;else{
if(_tmp4C6 == 0  || _tmp4C7 == 0)
bounds_ok=1;}}{
# 3008
int t1_nullable=Cyc_Tcutil_force_type2bool(0,_tmp4DB);
int t2_nullable=Cyc_Tcutil_force_type2bool(0,_tmp4CC);
if(t1_nullable  && !t2_nullable)
coercion=2U;
# 3015
if(((bounds_ok  && zeroterm_ok) && (_tmp4C4  || _tmp4C5)) && (
Cyc_Tcutil_unify(_tmp4DC,_tmp4CD) || Cyc_Tcenv_region_outlives(te,_tmp4DC,_tmp4CD)))
return coercion;else{
return Cyc_Absyn_Unknown_coercion;}};};}}else{_LL1B: _LL1C:
 goto _LL18;}_LL18:;}
# 3021
return Cyc_Absyn_Unknown_coercion;case 4U: _LLC: _tmp4E2=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp4BF)->f1).elt_type;_tmp4E1=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp4BF)->f1).tq;_tmp4E0=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp4BF)->f1).num_elts;_tmp4DF=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp4BF)->f1).zero_term;_LLD:
# 3023
{void*_tmp4D0=t2;void*_tmp4D4;struct Cyc_Absyn_Tqual _tmp4D3;struct Cyc_Absyn_Exp*_tmp4D2;void*_tmp4D1;if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp4D0)->tag == 4U){_LL1E: _tmp4D4=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp4D0)->f1).elt_type;_tmp4D3=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp4D0)->f1).tq;_tmp4D2=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp4D0)->f1).num_elts;_tmp4D1=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp4D0)->f1).zero_term;_LL1F: {
# 3025
int okay;
okay=
(((_tmp4E0 != 0  && _tmp4D2 != 0) && Cyc_Tcutil_unify(_tmp4DF,_tmp4D1)) && 
Cyc_Evexp_lte_const_exp(_tmp4D2,_tmp4E0)) && 
Cyc_Evexp_lte_const_exp(_tmp4E0,_tmp4D2);
return
# 3032
(okay  && Cyc_Tcutil_unify(_tmp4E2,_tmp4D4)) && (!_tmp4E1.real_const  || _tmp4D3.real_const)?Cyc_Absyn_No_coercion: Cyc_Absyn_Unknown_coercion;}}else{_LL20: _LL21:
# 3034
 return Cyc_Absyn_Unknown_coercion;}_LL1D:;}
# 3036
return Cyc_Absyn_Unknown_coercion;case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4BF)->f1)){case 15U: _LLE: _tmp4E3=((struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4BF)->f1)->f2;_LLF:
# 3040
{void*_tmp4D5=t2;struct Cyc_Absyn_Enumdecl*_tmp4D6;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4D5)->tag == 0U){if(((struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4D5)->f1)->tag == 15U){_LL23: _tmp4D6=((struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4D5)->f1)->f2;_LL24:
# 3042
 if((((struct Cyc_Absyn_Enumdecl*)_check_null(_tmp4E3))->fields != 0  && ((struct Cyc_Absyn_Enumdecl*)_check_null(_tmp4D6))->fields != 0) && ({
int _tmpB2E=((int(*)(struct Cyc_List_List*x))Cyc_List_length)((struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(_tmp4E3->fields))->v);_tmpB2E >= ((int(*)(struct Cyc_List_List*x))Cyc_List_length)((struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(_tmp4D6->fields))->v);}))
return Cyc_Absyn_Other_coercion;
goto _LL22;}else{goto _LL25;}}else{_LL25: _LL26:
 goto _LL22;}_LL22:;}
# 3048
goto _LL11;case 1U: _LL10: _LL11:
 goto _LL13;case 2U: _LL12: _LL13:
# 3051
 return Cyc_Tcutil_is_strict_arithmetic_type(t2)?Cyc_Absyn_Other_coercion: Cyc_Absyn_Unknown_coercion;case 3U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4BF)->f2 != 0){_LL14: _tmp4E4=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4BF)->f2)->hd;_LL15:
# 3054
{void*_tmp4D7=t2;void*_tmp4D8;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4D7)->tag == 0U){if(((struct Cyc_Absyn_RgnHandleCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4D7)->f1)->tag == 3U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4D7)->f2 != 0){_LL28: _tmp4D8=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4D7)->f2)->hd;_LL29:
# 3056
 if(Cyc_Tcenv_region_outlives(te,_tmp4E4,_tmp4D8))return Cyc_Absyn_No_coercion;
goto _LL27;}else{goto _LL2A;}}else{goto _LL2A;}}else{_LL2A: _LL2B:
 goto _LL27;}_LL27:;}
# 3060
return Cyc_Absyn_Unknown_coercion;}else{goto _LL16;}default: goto _LL16;}default: _LL16: _LL17:
 return Cyc_Absyn_Unknown_coercion;}_LL9:;};}
# 3066
void Cyc_Tcutil_unchecked_cast(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*e,void*t,enum Cyc_Absyn_Coercion c){
if(!Cyc_Tcutil_unify((void*)_check_null(e->topt),t)){
struct Cyc_Absyn_Exp*_tmp4E5=Cyc_Absyn_copy_exp(e);
({void*_tmpB2F=(void*)({struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*_tmp4E6=_cycalloc(sizeof(*_tmp4E6));_tmp4E6->tag=14U,_tmp4E6->f1=t,_tmp4E6->f2=_tmp4E5,_tmp4E6->f3=0,_tmp4E6->f4=c;_tmp4E6;});e->r=_tmpB2F;});
e->topt=t;}}
# 3074
void*Cyc_Tcutil_max_arithmetic_type(void*t1,void*t2){
{struct _tuple0 _tmp4E7=({struct _tuple0 _tmp9B6;({void*_tmpB31=Cyc_Tcutil_compress(t1);_tmp9B6.f1=_tmpB31;}),({void*_tmpB30=Cyc_Tcutil_compress(t2);_tmp9B6.f2=_tmpB30;});_tmp9B6;});struct _tuple0 _tmp4E8=_tmp4E7;void*_tmp4EE;void*_tmp4ED;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4E8.f1)->tag == 0U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4E8.f2)->tag == 0U){_LL1: _tmp4EE=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4E8.f1)->f1;_tmp4ED=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4E8.f2)->f1;_LL2:
# 3077
{struct _tuple0 _tmp4E9=({struct _tuple0 _tmp9B5;_tmp9B5.f1=_tmp4EE,_tmp9B5.f2=_tmp4ED;_tmp9B5;});struct _tuple0 _tmp4EA=_tmp4E9;int _tmp4EC;int _tmp4EB;if(((struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)_tmp4EA.f1)->tag == 2U){if(((struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)_tmp4EA.f2)->tag == 2U){_LL6: _tmp4EC=((struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)_tmp4EA.f1)->f1;_tmp4EB=((struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)_tmp4EA.f2)->f1;_LL7:
# 3079
 if(_tmp4EC != 0  && _tmp4EC != 1)return t1;else{
if(_tmp4EB != 0  && _tmp4EB != 1)return t2;else{
if(_tmp4EC >= _tmp4EB)return t1;else{
return t2;}}}}else{_LL8: _LL9:
 return t1;}}else{if(((struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)_tmp4EA.f2)->tag == 2U){_LLA: _LLB:
 return t2;}else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp4EA.f1)->tag == 1U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp4EA.f1)->f1 == Cyc_Absyn_Unsigned){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp4EA.f1)->f2 == Cyc_Absyn_LongLong_sz){_LLC: _LLD:
 goto _LLF;}else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp4EA.f2)->tag == 1U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp4EA.f2)->f1 == Cyc_Absyn_Unsigned){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp4EA.f2)->f2 == Cyc_Absyn_LongLong_sz)goto _LLE;else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp4EA.f1)->f2 == Cyc_Absyn_Long_sz)goto _LL14;else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp4EA.f2)->f2 == Cyc_Absyn_Long_sz)goto _LL16;else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp4EA.f1)->f2 == Cyc_Absyn_Int_sz)goto _LL1C;else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp4EA.f2)->f2 == Cyc_Absyn_Int_sz)goto _LL1E;else{goto _LL24;}}}}}}else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp4EA.f2)->f2 == Cyc_Absyn_LongLong_sz)goto _LL12;else{switch(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp4EA.f1)->f2){case Cyc_Absyn_Long_sz: goto _LL14;case Cyc_Absyn_Int_sz: goto _LL1C;default: if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp4EA.f2)->f2 == Cyc_Absyn_Long_sz)goto _LL22;else{goto _LL24;}}}}}else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp4EA.f1)->f2 == Cyc_Absyn_Long_sz){_LL14: _LL15:
# 3089
 goto _LL17;}else{if(((struct Cyc_Absyn_TagCon_Absyn_TyCon_struct*)_tmp4EA.f2)->tag == 4U)goto _LL1A;else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp4EA.f1)->f2 == Cyc_Absyn_Int_sz){_LL1C: _LL1D:
# 3094
 goto _LL1F;}else{goto _LL24;}}}}}}else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp4EA.f2)->tag == 1U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp4EA.f2)->f1 == Cyc_Absyn_Unsigned){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp4EA.f2)->f2 == Cyc_Absyn_LongLong_sz)goto _LLE;else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp4EA.f1)->f2 == Cyc_Absyn_LongLong_sz)goto _LL10;else{switch(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp4EA.f2)->f2){case Cyc_Absyn_Long_sz: goto _LL16;case Cyc_Absyn_Int_sz: goto _LL1E;default: if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp4EA.f1)->f2 == Cyc_Absyn_Long_sz)goto _LL20;else{goto _LL24;}}}}}else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp4EA.f1)->f2 == Cyc_Absyn_LongLong_sz)goto _LL10;else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp4EA.f2)->f2 == Cyc_Absyn_LongLong_sz)goto _LL12;else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp4EA.f1)->f2 == Cyc_Absyn_Long_sz)goto _LL20;else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp4EA.f2)->f2 == Cyc_Absyn_Long_sz)goto _LL22;else{goto _LL24;}}}}}}else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp4EA.f1)->f2 == Cyc_Absyn_LongLong_sz){_LL10: _LL11:
# 3087
 goto _LL13;}else{if(((struct Cyc_Absyn_TagCon_Absyn_TyCon_struct*)_tmp4EA.f2)->tag == 4U)goto _LL1A;else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp4EA.f1)->f2 == Cyc_Absyn_Long_sz){_LL20: _LL21:
# 3096
 goto _LL23;}else{goto _LL24;}}}}}}else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp4EA.f2)->tag == 1U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp4EA.f2)->f1 == Cyc_Absyn_Unsigned)switch(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp4EA.f2)->f2){case Cyc_Absyn_LongLong_sz: _LLE: _LLF:
# 3086
 return Cyc_Absyn_ulonglong_type;case Cyc_Absyn_Long_sz: _LL16: _LL17:
# 3090
 return Cyc_Absyn_ulong_type;default: if(((struct Cyc_Absyn_TagCon_Absyn_TyCon_struct*)_tmp4EA.f1)->tag == 4U)goto _LL18;else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp4EA.f2)->f2 == Cyc_Absyn_Int_sz){_LL1E: _LL1F:
# 3095
 return Cyc_Absyn_uint_type;}else{goto _LL24;}}}else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp4EA.f2)->f2 == Cyc_Absyn_LongLong_sz){_LL12: _LL13:
# 3088
 return Cyc_Absyn_slonglong_type;}else{if(((struct Cyc_Absyn_TagCon_Absyn_TyCon_struct*)_tmp4EA.f1)->tag == 4U)goto _LL18;else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp4EA.f2)->f2 == Cyc_Absyn_Long_sz){_LL22: _LL23:
# 3097
 return Cyc_Absyn_slong_type;}else{goto _LL24;}}}}}else{if(((struct Cyc_Absyn_TagCon_Absyn_TyCon_struct*)_tmp4EA.f1)->tag == 4U){_LL18: _LL19:
# 3092
 goto _LL1B;}else{if(((struct Cyc_Absyn_TagCon_Absyn_TyCon_struct*)_tmp4EA.f2)->tag == 4U){_LL1A: _LL1B:
 goto _LL1D;}else{_LL24: _LL25:
# 3098
 goto _LL5;}}}}}}_LL5:;}
# 3100
goto _LL0;}else{goto _LL3;}}else{_LL3: _LL4:
 goto _LL0;}_LL0:;}
# 3103
return Cyc_Absyn_sint_type;}
# 3113 "tcutil.cyc"
static int Cyc_Tcutil_constrain_kinds(void*c1,void*c2){
c1=Cyc_Absyn_compress_kb(c1);
c2=Cyc_Absyn_compress_kb(c2);
if(c1 == c2)return 1;{
struct _tuple0 _tmp4EF=({struct _tuple0 _tmp9B7;_tmp9B7.f1=c1,_tmp9B7.f2=c2;_tmp9B7;});struct _tuple0 _tmp4F0=_tmp4EF;struct Cyc_Core_Opt**_tmp504;struct Cyc_Absyn_Kind*_tmp503;struct Cyc_Core_Opt**_tmp502;struct Cyc_Absyn_Kind*_tmp501;struct Cyc_Core_Opt**_tmp500;struct Cyc_Absyn_Kind*_tmp4FF;struct Cyc_Absyn_Kind*_tmp4FE;struct Cyc_Core_Opt**_tmp4FD;struct Cyc_Core_Opt**_tmp4FC;struct Cyc_Absyn_Kind*_tmp4FB;struct Cyc_Core_Opt**_tmp4FA;struct Cyc_Absyn_Kind*_tmp4F9;struct Cyc_Absyn_Kind*_tmp4F8;struct Cyc_Absyn_Kind*_tmp4F7;if(((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp4F0.f1)->tag == 0U)switch(*((int*)_tmp4F0.f2)){case 0U: _LL1: _tmp4F8=((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp4F0.f1)->f1;_tmp4F7=((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp4F0.f2)->f1;_LL2:
 return _tmp4F8 == _tmp4F7;case 1U: goto _LL3;default: _LL9: _tmp4FB=((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp4F0.f1)->f1;_tmp4FA=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp4F0.f2)->f1;_tmp4F9=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp4F0.f2)->f2;_LLA:
# 3126
 if(Cyc_Tcutil_kind_leq(_tmp4FB,_tmp4F9)){
({struct Cyc_Core_Opt*_tmpB32=({struct Cyc_Core_Opt*_tmp4F4=_cycalloc(sizeof(*_tmp4F4));_tmp4F4->v=c1;_tmp4F4;});*_tmp4FA=_tmpB32;});return 1;}else{
return 0;}}else{if(((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmp4F0.f2)->tag == 1U){_LL3: _tmp4FC=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmp4F0.f2)->f1;_LL4:
# 3119
({struct Cyc_Core_Opt*_tmpB33=({struct Cyc_Core_Opt*_tmp4F1=_cycalloc(sizeof(*_tmp4F1));_tmp4F1->v=c1;_tmp4F1;});*_tmp4FC=_tmpB33;});return 1;}else{if(((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmp4F0.f1)->tag == 1U){_LL5: _tmp4FD=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmp4F0.f1)->f1;_LL6:
({struct Cyc_Core_Opt*_tmpB34=({struct Cyc_Core_Opt*_tmp4F2=_cycalloc(sizeof(*_tmp4F2));_tmp4F2->v=c2;_tmp4F2;});*_tmp4FD=_tmpB34;});return 1;}else{if(((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp4F0.f2)->tag == 0U){_LL7: _tmp500=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp4F0.f1)->f1;_tmp4FF=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp4F0.f1)->f2;_tmp4FE=((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp4F0.f2)->f1;_LL8:
# 3122
 if(Cyc_Tcutil_kind_leq(_tmp4FE,_tmp4FF)){
({struct Cyc_Core_Opt*_tmpB35=({struct Cyc_Core_Opt*_tmp4F3=_cycalloc(sizeof(*_tmp4F3));_tmp4F3->v=c2;_tmp4F3;});*_tmp500=_tmpB35;});return 1;}else{
return 0;}}else{_LLB: _tmp504=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp4F0.f1)->f1;_tmp503=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp4F0.f1)->f2;_tmp502=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp4F0.f2)->f1;_tmp501=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp4F0.f2)->f2;_LLC:
# 3130
 if(Cyc_Tcutil_kind_leq(_tmp503,_tmp501)){
({struct Cyc_Core_Opt*_tmpB36=({struct Cyc_Core_Opt*_tmp4F5=_cycalloc(sizeof(*_tmp4F5));_tmp4F5->v=c1;_tmp4F5;});*_tmp502=_tmpB36;});return 1;}else{
if(Cyc_Tcutil_kind_leq(_tmp501,_tmp503)){
({struct Cyc_Core_Opt*_tmpB37=({struct Cyc_Core_Opt*_tmp4F6=_cycalloc(sizeof(*_tmp4F6));_tmp4F6->v=c2;_tmp4F6;});*_tmp504=_tmpB37;});return 1;}else{
return 0;}}}}}}_LL0:;};}
# 3139
static int Cyc_Tcutil_tvar_id_counter=0;
int Cyc_Tcutil_new_tvar_id(){
return Cyc_Tcutil_tvar_id_counter ++;}
# 3144
static int Cyc_Tcutil_tvar_counter=0;
struct Cyc_Absyn_Tvar*Cyc_Tcutil_new_tvar(void*k){
int i=Cyc_Tcutil_tvar_counter ++;
struct _dyneither_ptr s=(struct _dyneither_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp50A=({struct Cyc_Int_pa_PrintArg_struct _tmp9B8;_tmp9B8.tag=1U,_tmp9B8.f1=(unsigned long)i;_tmp9B8;});void*_tmp508[1U];_tmp508[0]=& _tmp50A;({struct _dyneither_ptr _tmpB38=({const char*_tmp509="#%d";_tag_dyneither(_tmp509,sizeof(char),4U);});Cyc_aprintf(_tmpB38,_tag_dyneither(_tmp508,sizeof(void*),1U));});});
return({struct Cyc_Absyn_Tvar*_tmp507=_cycalloc(sizeof(*_tmp507));({struct _dyneither_ptr*_tmpB39=({unsigned int _tmp506=1;struct _dyneither_ptr*_tmp505=_cycalloc(_check_times(_tmp506,sizeof(struct _dyneither_ptr)));_tmp505[0]=s;_tmp505;});_tmp507->name=_tmpB39;}),_tmp507->identity=- 1,_tmp507->kind=k;_tmp507;});}
# 3151
int Cyc_Tcutil_is_temp_tvar(struct Cyc_Absyn_Tvar*t){
struct _dyneither_ptr _tmp50B=*t->name;
return(int)*((const char*)_check_dyneither_subscript(_tmp50B,sizeof(char),0))== (int)'#';}
# 3156
void Cyc_Tcutil_rewrite_temp_tvar(struct Cyc_Absyn_Tvar*t){
({struct Cyc_String_pa_PrintArg_struct _tmp50E=({struct Cyc_String_pa_PrintArg_struct _tmp9B9;_tmp9B9.tag=0U,_tmp9B9.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*t->name);_tmp9B9;});void*_tmp50C[1U];_tmp50C[0]=& _tmp50E;({struct _dyneither_ptr _tmpB3A=({const char*_tmp50D="%s";_tag_dyneither(_tmp50D,sizeof(char),3U);});Cyc_printf(_tmpB3A,_tag_dyneither(_tmp50C,sizeof(void*),1U));});});
if(!Cyc_Tcutil_is_temp_tvar(t))return;{
struct _dyneither_ptr _tmp50F=({struct _dyneither_ptr _tmpB3B=({const char*_tmp515="`";_tag_dyneither(_tmp515,sizeof(char),2U);});Cyc_strconcat(_tmpB3B,(struct _dyneither_ptr)*t->name);});
({struct _dyneither_ptr _tmp510=_dyneither_ptr_plus(_tmp50F,sizeof(char),1);char _tmp511=*((char*)_check_dyneither_subscript(_tmp510,sizeof(char),0U));char _tmp512='t';if(_get_dyneither_size(_tmp510,sizeof(char))== 1U  && (_tmp511 == 0  && _tmp512 != 0))_throw_arraybounds();*((char*)_tmp510.curr)=_tmp512;});
({struct _dyneither_ptr*_tmpB3C=({unsigned int _tmp514=1;struct _dyneither_ptr*_tmp513=_cycalloc(_check_times(_tmp514,sizeof(struct _dyneither_ptr)));_tmp513[0]=(struct _dyneither_ptr)_tmp50F;_tmp513;});t->name=_tmpB3C;});};}
# 3165
void*Cyc_Tcutil_fndecl2type(struct Cyc_Absyn_Fndecl*fd){
if(fd->cached_type == 0){
# 3172
struct Cyc_List_List*_tmp516=0;
{struct Cyc_List_List*atts=(fd->i).attributes;for(0;atts != 0;atts=atts->tl){
if(Cyc_Absyn_fntype_att((void*)atts->hd))
_tmp516=({struct Cyc_List_List*_tmp517=_cycalloc(sizeof(*_tmp517));_tmp517->hd=(void*)atts->hd,_tmp517->tl=_tmp516;_tmp517;});}}{
struct Cyc_Absyn_FnInfo _tmp518=fd->i;
_tmp518.attributes=_tmp516;
return(void*)({struct Cyc_Absyn_FnType_Absyn_Type_struct*_tmp519=_cycalloc(sizeof(*_tmp519));_tmp519->tag=5U,_tmp519->f1=_tmp518;_tmp519;});};}
# 3180
return(void*)_check_null(fd->cached_type);}
# 3186
static void Cyc_Tcutil_replace_rop(struct Cyc_List_List*args,union Cyc_Relations_RelnOp*rop){
# 3188
union Cyc_Relations_RelnOp _tmp51A=*rop;union Cyc_Relations_RelnOp _tmp51B=_tmp51A;struct Cyc_Absyn_Vardecl*_tmp52E;struct Cyc_Absyn_Vardecl*_tmp52D;switch((_tmp51B.RNumelts).tag){case 2U: _LL1: _tmp52D=(_tmp51B.RVar).val;_LL2: {
# 3190
struct _tuple2 _tmp51C=*_tmp52D->name;struct _tuple2 _tmp51D=_tmp51C;union Cyc_Absyn_Nmspace _tmp524;struct _dyneither_ptr*_tmp523;_LL8: _tmp524=_tmp51D.f1;_tmp523=_tmp51D.f2;_LL9:;
if(!((int)((_tmp524.Loc_n).tag == 4)))goto _LL0;
if(({struct _dyneither_ptr _tmpB3D=(struct _dyneither_ptr)*_tmp523;Cyc_strcmp(_tmpB3D,({const char*_tmp51E="return_value";_tag_dyneither(_tmp51E,sizeof(char),13U);}));})== 0){
({union Cyc_Relations_RelnOp _tmpB3E=Cyc_Relations_RReturn();*rop=_tmpB3E;});
goto _LL0;}{
# 3196
unsigned int c=0U;
{struct Cyc_List_List*_tmp51F=args;for(0;_tmp51F != 0;(_tmp51F=_tmp51F->tl,c ++)){
struct _tuple10*_tmp520=(struct _tuple10*)_tmp51F->hd;struct _tuple10*_tmp521=_tmp520;struct _dyneither_ptr*_tmp522;_LLB: _tmp522=_tmp521->f1;_LLC:;
if(_tmp522 != 0){
if(Cyc_strcmp((struct _dyneither_ptr)*_tmp523,(struct _dyneither_ptr)*_tmp522)== 0){
({union Cyc_Relations_RelnOp _tmpB3F=Cyc_Relations_RParam(c);*rop=_tmpB3F;});
break;}}}}
# 3206
goto _LL0;};}case 3U: _LL3: _tmp52E=(_tmp51B.RNumelts).val;_LL4: {
# 3208
struct _tuple2 _tmp525=*_tmp52E->name;struct _tuple2 _tmp526=_tmp525;union Cyc_Absyn_Nmspace _tmp52C;struct _dyneither_ptr*_tmp52B;_LLE: _tmp52C=_tmp526.f1;_tmp52B=_tmp526.f2;_LLF:;
if(!((int)((_tmp52C.Loc_n).tag == 4)))goto _LL0;{
unsigned int c=0U;
{struct Cyc_List_List*_tmp527=args;for(0;_tmp527 != 0;(_tmp527=_tmp527->tl,c ++)){
struct _tuple10*_tmp528=(struct _tuple10*)_tmp527->hd;struct _tuple10*_tmp529=_tmp528;struct _dyneither_ptr*_tmp52A;_LL11: _tmp52A=_tmp529->f1;_LL12:;
if(_tmp52A != 0){
if(Cyc_strcmp((struct _dyneither_ptr)*_tmp52B,(struct _dyneither_ptr)*_tmp52A)== 0){
({union Cyc_Relations_RelnOp _tmpB40=Cyc_Relations_RParamNumelts(c);*rop=_tmpB40;});
break;}}}}
# 3220
goto _LL0;};}default: _LL5: _LL6:
 goto _LL0;}_LL0:;}
# 3225
static void Cyc_Tcutil_replace_rops(struct Cyc_List_List*args,struct Cyc_Relations_Reln*r){
# 3227
Cyc_Tcutil_replace_rop(args,& r->rop1);
Cyc_Tcutil_replace_rop(args,& r->rop2);}
# 3231
static struct Cyc_List_List*Cyc_Tcutil_extract_relns(struct Cyc_List_List*args,struct Cyc_Absyn_Exp*e){
# 3233
if(e == 0)return 0;{
struct Cyc_List_List*_tmp52F=Cyc_Relations_exp2relns(Cyc_Core_heap_region,e);
((void(*)(void(*f)(struct Cyc_List_List*,struct Cyc_Relations_Reln*),struct Cyc_List_List*env,struct Cyc_List_List*x))Cyc_List_iter_c)(Cyc_Tcutil_replace_rops,args,_tmp52F);
return _tmp52F;};}
# 3240
static struct _dyneither_ptr*Cyc_Tcutil_fst_fdarg(struct _tuple10*t){return(struct _dyneither_ptr*)_check_null((*t).f1);}
void*Cyc_Tcutil_snd_tqt(struct _tuple12*t){return(*t).f2;}
static struct _tuple12*Cyc_Tcutil_map2_tq(struct _tuple12*pr,void*t){
struct _tuple12*_tmp530=pr;struct Cyc_Absyn_Tqual _tmp533;void*_tmp532;_LL1: _tmp533=_tmp530->f1;_tmp532=_tmp530->f2;_LL2:;
if(_tmp532 == t)return pr;else{
return({struct _tuple12*_tmp531=_cycalloc(sizeof(*_tmp531));_tmp531->f1=_tmp533,_tmp531->f2=t;_tmp531;});}}struct _tuple27{struct _dyneither_ptr*f1;struct Cyc_Absyn_Tqual f2;};struct _tuple28{struct _tuple27*f1;void*f2;};
# 3247
static struct _tuple28*Cyc_Tcutil_substitute_f1(struct _RegionHandle*rgn,struct _tuple10*y){
# 3249
return({struct _tuple28*_tmp535=_region_malloc(rgn,sizeof(*_tmp535));({struct _tuple27*_tmpB41=({struct _tuple27*_tmp534=_region_malloc(rgn,sizeof(*_tmp534));_tmp534->f1=(*y).f1,_tmp534->f2=(*y).f2;_tmp534;});_tmp535->f1=_tmpB41;}),_tmp535->f2=(*y).f3;_tmp535;});}
# 3251
static struct _tuple10*Cyc_Tcutil_substitute_f2(struct _tuple10*orig_arg,void*t){
# 3253
struct _tuple10 _tmp536=*orig_arg;struct _tuple10 _tmp537=_tmp536;struct _dyneither_ptr*_tmp53B;struct Cyc_Absyn_Tqual _tmp53A;void*_tmp539;_LL1: _tmp53B=_tmp537.f1;_tmp53A=_tmp537.f2;_tmp539=_tmp537.f3;_LL2:;
if(t == _tmp539)return orig_arg;
return({struct _tuple10*_tmp538=_cycalloc(sizeof(*_tmp538));_tmp538->f1=_tmp53B,_tmp538->f2=_tmp53A,_tmp538->f3=t;_tmp538;});}
# 3257
static void*Cyc_Tcutil_field_type(struct Cyc_Absyn_Aggrfield*f){
return f->type;}
# 3260
static struct Cyc_List_List*Cyc_Tcutil_substs(struct _RegionHandle*rgn,struct Cyc_List_List*inst,struct Cyc_List_List*ts);
# 3265
static struct Cyc_Absyn_Exp*Cyc_Tcutil_copye(struct Cyc_Absyn_Exp*old,void*r){
# 3267
return({struct Cyc_Absyn_Exp*_tmp53C=_cycalloc(sizeof(*_tmp53C));_tmp53C->topt=old->topt,_tmp53C->r=r,_tmp53C->loc=old->loc,_tmp53C->annot=old->annot;_tmp53C;});}
# 3272
struct Cyc_Absyn_Exp*Cyc_Tcutil_rsubsexp(struct _RegionHandle*r,struct Cyc_List_List*inst,struct Cyc_Absyn_Exp*e){
void*_tmp53D=e->r;void*_tmp53E=_tmp53D;void*_tmp57E;void*_tmp57D;struct Cyc_List_List*_tmp57C;struct Cyc_Absyn_Exp*_tmp57B;struct Cyc_Absyn_Exp*_tmp57A;void*_tmp579;void*_tmp578;struct Cyc_Absyn_Exp*_tmp577;int _tmp576;enum Cyc_Absyn_Coercion _tmp575;struct Cyc_Absyn_Exp*_tmp574;struct Cyc_Absyn_Exp*_tmp573;struct Cyc_Absyn_Exp*_tmp572;struct Cyc_Absyn_Exp*_tmp571;struct Cyc_Absyn_Exp*_tmp570;struct Cyc_Absyn_Exp*_tmp56F;struct Cyc_Absyn_Exp*_tmp56E;struct Cyc_Absyn_Exp*_tmp56D;struct Cyc_Absyn_Exp*_tmp56C;enum Cyc_Absyn_Primop _tmp56B;struct Cyc_List_List*_tmp56A;switch(*((int*)_tmp53E)){case 0U: _LL1: _LL2:
 goto _LL4;case 32U: _LL3: _LL4:
 goto _LL6;case 33U: _LL5: _LL6:
 goto _LL8;case 2U: _LL7: _LL8:
 goto _LLA;case 1U: _LL9: _LLA:
 return e;case 3U: _LLB: _tmp56B=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp53E)->f1;_tmp56A=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp53E)->f2;_LLC:
# 3281
 if(((int(*)(struct Cyc_List_List*x))Cyc_List_length)(_tmp56A)== 1){
struct Cyc_Absyn_Exp*_tmp53F=(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(_tmp56A))->hd;
struct Cyc_Absyn_Exp*_tmp540=Cyc_Tcutil_rsubsexp(r,inst,_tmp53F);
if(_tmp540 == _tmp53F)return e;
return({struct Cyc_Absyn_Exp*_tmpB43=e;Cyc_Tcutil_copye(_tmpB43,(void*)({struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*_tmp542=_cycalloc(sizeof(*_tmp542));_tmp542->tag=3U,_tmp542->f1=_tmp56B,({struct Cyc_List_List*_tmpB42=({struct Cyc_Absyn_Exp*_tmp541[1U];_tmp541[0]=_tmp540;((struct Cyc_List_List*(*)(struct _dyneither_ptr))Cyc_List_list)(_tag_dyneither(_tmp541,sizeof(struct Cyc_Absyn_Exp*),1U));});_tmp542->f2=_tmpB42;});_tmp542;}));});}else{
if(((int(*)(struct Cyc_List_List*x))Cyc_List_length)(_tmp56A)== 2){
struct Cyc_Absyn_Exp*_tmp543=(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(_tmp56A))->hd;
struct Cyc_Absyn_Exp*_tmp544=(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(_tmp56A->tl))->hd;
struct Cyc_Absyn_Exp*_tmp545=Cyc_Tcutil_rsubsexp(r,inst,_tmp543);
struct Cyc_Absyn_Exp*_tmp546=Cyc_Tcutil_rsubsexp(r,inst,_tmp544);
if(_tmp545 == _tmp543  && _tmp546 == _tmp544)return e;
return({struct Cyc_Absyn_Exp*_tmpB45=e;Cyc_Tcutil_copye(_tmpB45,(void*)({struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*_tmp548=_cycalloc(sizeof(*_tmp548));_tmp548->tag=3U,_tmp548->f1=_tmp56B,({struct Cyc_List_List*_tmpB44=({struct Cyc_Absyn_Exp*_tmp547[2U];_tmp547[0]=_tmp545,_tmp547[1]=_tmp546;((struct Cyc_List_List*(*)(struct _dyneither_ptr))Cyc_List_list)(_tag_dyneither(_tmp547,sizeof(struct Cyc_Absyn_Exp*),2U));});_tmp548->f2=_tmpB44;});_tmp548;}));});}else{
return({void*_tmp549=0U;({struct _dyneither_ptr _tmpB46=({const char*_tmp54A="primop does not have 1 or 2 args!";_tag_dyneither(_tmp54A,sizeof(char),34U);});((struct Cyc_Absyn_Exp*(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpB46,_tag_dyneither(_tmp549,sizeof(void*),0U));});});}}case 6U: _LLD: _tmp56E=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp53E)->f1;_tmp56D=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp53E)->f2;_tmp56C=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp53E)->f3;_LLE: {
# 3295
struct Cyc_Absyn_Exp*_tmp54B=Cyc_Tcutil_rsubsexp(r,inst,_tmp56E);
struct Cyc_Absyn_Exp*_tmp54C=Cyc_Tcutil_rsubsexp(r,inst,_tmp56D);
struct Cyc_Absyn_Exp*_tmp54D=Cyc_Tcutil_rsubsexp(r,inst,_tmp56C);
if((_tmp54B == _tmp56E  && _tmp54C == _tmp56D) && _tmp54D == _tmp56C)return e;
return({struct Cyc_Absyn_Exp*_tmpB47=e;Cyc_Tcutil_copye(_tmpB47,(void*)({struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*_tmp54E=_cycalloc(sizeof(*_tmp54E));_tmp54E->tag=6U,_tmp54E->f1=_tmp54B,_tmp54E->f2=_tmp54C,_tmp54E->f3=_tmp54D;_tmp54E;}));});}case 7U: _LLF: _tmp570=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp53E)->f1;_tmp56F=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp53E)->f2;_LL10: {
# 3301
struct Cyc_Absyn_Exp*_tmp54F=Cyc_Tcutil_rsubsexp(r,inst,_tmp570);
struct Cyc_Absyn_Exp*_tmp550=Cyc_Tcutil_rsubsexp(r,inst,_tmp56F);
if(_tmp54F == _tmp570  && _tmp550 == _tmp56F)return e;
return({struct Cyc_Absyn_Exp*_tmpB48=e;Cyc_Tcutil_copye(_tmpB48,(void*)({struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*_tmp551=_cycalloc(sizeof(*_tmp551));_tmp551->tag=7U,_tmp551->f1=_tmp54F,_tmp551->f2=_tmp550;_tmp551;}));});}case 8U: _LL11: _tmp572=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp53E)->f1;_tmp571=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp53E)->f2;_LL12: {
# 3306
struct Cyc_Absyn_Exp*_tmp552=Cyc_Tcutil_rsubsexp(r,inst,_tmp572);
struct Cyc_Absyn_Exp*_tmp553=Cyc_Tcutil_rsubsexp(r,inst,_tmp571);
if(_tmp552 == _tmp572  && _tmp553 == _tmp571)return e;
return({struct Cyc_Absyn_Exp*_tmpB49=e;Cyc_Tcutil_copye(_tmpB49,(void*)({struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*_tmp554=_cycalloc(sizeof(*_tmp554));_tmp554->tag=8U,_tmp554->f1=_tmp552,_tmp554->f2=_tmp553;_tmp554;}));});}case 9U: _LL13: _tmp574=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp53E)->f1;_tmp573=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp53E)->f2;_LL14: {
# 3311
struct Cyc_Absyn_Exp*_tmp555=Cyc_Tcutil_rsubsexp(r,inst,_tmp574);
struct Cyc_Absyn_Exp*_tmp556=Cyc_Tcutil_rsubsexp(r,inst,_tmp573);
if(_tmp555 == _tmp574  && _tmp556 == _tmp573)return e;
return({struct Cyc_Absyn_Exp*_tmpB4A=e;Cyc_Tcutil_copye(_tmpB4A,(void*)({struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*_tmp557=_cycalloc(sizeof(*_tmp557));_tmp557->tag=9U,_tmp557->f1=_tmp555,_tmp557->f2=_tmp556;_tmp557;}));});}case 14U: _LL15: _tmp578=(void*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp53E)->f1;_tmp577=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp53E)->f2;_tmp576=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp53E)->f3;_tmp575=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp53E)->f4;_LL16: {
# 3316
struct Cyc_Absyn_Exp*_tmp558=Cyc_Tcutil_rsubsexp(r,inst,_tmp577);
void*_tmp559=Cyc_Tcutil_rsubstitute(r,inst,_tmp578);
if(_tmp558 == _tmp577  && _tmp559 == _tmp578)return e;
return({struct Cyc_Absyn_Exp*_tmpB4B=e;Cyc_Tcutil_copye(_tmpB4B,(void*)({struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*_tmp55A=_cycalloc(sizeof(*_tmp55A));_tmp55A->tag=14U,_tmp55A->f1=_tmp559,_tmp55A->f2=_tmp558,_tmp55A->f3=_tmp576,_tmp55A->f4=_tmp575;_tmp55A;}));});}case 17U: _LL17: _tmp579=(void*)((struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*)_tmp53E)->f1;_LL18: {
# 3321
void*_tmp55B=Cyc_Tcutil_rsubstitute(r,inst,_tmp579);
if(_tmp55B == _tmp579)return e;
return({struct Cyc_Absyn_Exp*_tmpB4C=e;Cyc_Tcutil_copye(_tmpB4C,(void*)({struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*_tmp55C=_cycalloc(sizeof(*_tmp55C));_tmp55C->tag=17U,_tmp55C->f1=_tmp55B;_tmp55C;}));});}case 18U: _LL19: _tmp57A=((struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_tmp53E)->f1;_LL1A: {
# 3325
struct Cyc_Absyn_Exp*_tmp55D=Cyc_Tcutil_rsubsexp(r,inst,_tmp57A);
if(_tmp55D == _tmp57A)return e;
return({struct Cyc_Absyn_Exp*_tmpB4D=e;Cyc_Tcutil_copye(_tmpB4D,(void*)({struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*_tmp55E=_cycalloc(sizeof(*_tmp55E));_tmp55E->tag=18U,_tmp55E->f1=_tmp55D;_tmp55E;}));});}case 41U: _LL1B: _tmp57B=((struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*)_tmp53E)->f1;_LL1C: {
# 3329
struct Cyc_Absyn_Exp*_tmp55F=Cyc_Tcutil_rsubsexp(r,inst,_tmp57B);
if(_tmp55F == _tmp57B)return e;
return({struct Cyc_Absyn_Exp*_tmpB4E=e;Cyc_Tcutil_copye(_tmpB4E,(void*)({struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*_tmp560=_cycalloc(sizeof(*_tmp560));_tmp560->tag=41U,_tmp560->f1=_tmp55F;_tmp560;}));});}case 19U: _LL1D: _tmp57D=(void*)((struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_tmp53E)->f1;_tmp57C=((struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_tmp53E)->f2;_LL1E: {
# 3333
void*_tmp561=Cyc_Tcutil_rsubstitute(r,inst,_tmp57D);
if(_tmp561 == _tmp57D)return e;
return({struct Cyc_Absyn_Exp*_tmpB4F=e;Cyc_Tcutil_copye(_tmpB4F,(void*)({struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*_tmp562=_cycalloc(sizeof(*_tmp562));_tmp562->tag=19U,_tmp562->f1=_tmp561,_tmp562->f2=_tmp57C;_tmp562;}));});}case 39U: _LL1F: _tmp57E=(void*)((struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*)_tmp53E)->f1;_LL20: {
# 3337
void*_tmp563=Cyc_Tcutil_rsubstitute(r,inst,_tmp57E);
if(_tmp563 == _tmp57E)return e;{
# 3340
void*_tmp564=Cyc_Tcutil_compress(_tmp563);void*_tmp565=_tmp564;struct Cyc_Absyn_Exp*_tmp567;if(((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp565)->tag == 9U){_LL24: _tmp567=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp565)->f1;_LL25:
 return _tmp567;}else{_LL26: _LL27:
# 3343
 return({struct Cyc_Absyn_Exp*_tmpB50=e;Cyc_Tcutil_copye(_tmpB50,(void*)({struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*_tmp566=_cycalloc(sizeof(*_tmp566));_tmp566->tag=39U,_tmp566->f1=_tmp563;_tmp566;}));});}_LL23:;};}default: _LL21: _LL22:
# 3346
 return({void*_tmp568=0U;({struct _dyneither_ptr _tmpB51=({const char*_tmp569="non-type-level-expression in Tcutil::rsubsexp";_tag_dyneither(_tmp569,sizeof(char),46U);});((struct Cyc_Absyn_Exp*(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpB51,_tag_dyneither(_tmp568,sizeof(void*),0U));});});}_LL0:;}
# 3350
static struct Cyc_Absyn_Exp*Cyc_Tcutil_rsubs_exp_opt(struct _RegionHandle*r,struct Cyc_List_List*inst,struct Cyc_Absyn_Exp*e){
# 3353
if(e == 0)return 0;else{
return Cyc_Tcutil_rsubsexp(r,inst,e);}}
# 3357
static struct Cyc_Absyn_Aggrfield*Cyc_Tcutil_subst_aggrfield(struct _RegionHandle*r,struct Cyc_List_List*inst,struct Cyc_Absyn_Aggrfield*f){
# 3361
void*_tmp57F=f->type;
struct Cyc_Absyn_Exp*_tmp580=f->requires_clause;
void*_tmp581=Cyc_Tcutil_rsubstitute(r,inst,_tmp57F);
struct Cyc_Absyn_Exp*_tmp582=Cyc_Tcutil_rsubs_exp_opt(r,inst,_tmp580);
if(_tmp57F == _tmp581  && _tmp580 == _tmp582)return f;else{
return({struct Cyc_Absyn_Aggrfield*_tmp583=_cycalloc(sizeof(*_tmp583));_tmp583->name=f->name,_tmp583->tq=f->tq,_tmp583->type=_tmp581,_tmp583->width=f->width,_tmp583->attributes=f->attributes,_tmp583->requires_clause=_tmp582;_tmp583;});}}
# 3371
static struct Cyc_List_List*Cyc_Tcutil_subst_aggrfields(struct _RegionHandle*r,struct Cyc_List_List*inst,struct Cyc_List_List*fs){
# 3374
if(fs == 0)return 0;{
struct Cyc_Absyn_Aggrfield*_tmp584=(struct Cyc_Absyn_Aggrfield*)fs->hd;
struct Cyc_List_List*_tmp585=fs->tl;
struct Cyc_Absyn_Aggrfield*_tmp586=Cyc_Tcutil_subst_aggrfield(r,inst,_tmp584);
struct Cyc_List_List*_tmp587=Cyc_Tcutil_subst_aggrfields(r,inst,_tmp585);
if(_tmp586 == _tmp584  && _tmp587 == _tmp585)return fs;
return({struct Cyc_List_List*_tmp588=_cycalloc(sizeof(*_tmp588));_tmp588->hd=_tmp586,_tmp588->tl=_tmp587;_tmp588;});};}
# 3383
struct Cyc_List_List*Cyc_Tcutil_rsubst_rgnpo(struct _RegionHandle*rgn,struct Cyc_List_List*inst,struct Cyc_List_List*rgn_po){
# 3386
struct _tuple1 _tmp589=((struct _tuple1(*)(struct _RegionHandle*r1,struct _RegionHandle*r2,struct Cyc_List_List*x))Cyc_List_rsplit)(rgn,rgn,rgn_po);struct _tuple1 _tmp58A=_tmp589;struct Cyc_List_List*_tmp58E;struct Cyc_List_List*_tmp58D;_LL1: _tmp58E=_tmp58A.f1;_tmp58D=_tmp58A.f2;_LL2:;{
struct Cyc_List_List*_tmp58B=Cyc_Tcutil_substs(rgn,inst,_tmp58E);
struct Cyc_List_List*_tmp58C=Cyc_Tcutil_substs(rgn,inst,_tmp58D);
if(_tmp58B == _tmp58E  && _tmp58C == _tmp58D)
return rgn_po;else{
# 3392
return((struct Cyc_List_List*(*)(struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_zip)(_tmp58B,_tmp58C);}};}
# 3395
void*Cyc_Tcutil_rsubstitute(struct _RegionHandle*rgn,struct Cyc_List_List*inst,void*t){
# 3398
void*_tmp58F=Cyc_Tcutil_compress(t);void*_tmp590=_tmp58F;struct Cyc_Absyn_Exp*_tmp5E8;struct Cyc_Absyn_Exp*_tmp5E7;void*_tmp5E6;struct Cyc_List_List*_tmp5E5;void*_tmp5E4;void*_tmp5E3;enum Cyc_Absyn_AggrKind _tmp5E2;struct Cyc_List_List*_tmp5E1;struct Cyc_List_List*_tmp5E0;struct Cyc_List_List*_tmp5DF;void*_tmp5DE;struct Cyc_Absyn_Tqual _tmp5DD;void*_tmp5DC;struct Cyc_List_List*_tmp5DB;int _tmp5DA;struct Cyc_Absyn_VarargInfo*_tmp5D9;struct Cyc_List_List*_tmp5D8;struct Cyc_List_List*_tmp5D7;struct Cyc_Absyn_Exp*_tmp5D6;struct Cyc_Absyn_Exp*_tmp5D5;void*_tmp5D4;struct Cyc_Absyn_Tqual _tmp5D3;void*_tmp5D2;void*_tmp5D1;void*_tmp5D0;void*_tmp5CF;void*_tmp5CE;struct Cyc_Absyn_Tqual _tmp5CD;struct Cyc_Absyn_Exp*_tmp5CC;void*_tmp5CB;unsigned int _tmp5CA;struct _tuple2*_tmp5C9;struct Cyc_List_List*_tmp5C8;struct Cyc_Absyn_Typedefdecl*_tmp5C7;void*_tmp5C6;struct Cyc_Absyn_Tvar*_tmp5C5;switch(*((int*)_tmp590)){case 2U: _LL1: _tmp5C5=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp590)->f1;_LL2: {
# 3401
struct _handler_cons _tmp591;_push_handler(& _tmp591);{int _tmp593=0;if(setjmp(_tmp591.handler))_tmp593=1;if(!_tmp593){{void*_tmp594=((void*(*)(int(*cmp)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct Cyc_Absyn_Tvar*x))Cyc_List_assoc_cmp)(Cyc_Absyn_tvar_cmp,inst,_tmp5C5);_npop_handler(0U);return _tmp594;};_pop_handler();}else{void*_tmp592=(void*)_exn_thrown;void*_tmp595=_tmp592;void*_tmp596;if(((struct Cyc_Core_Not_found_exn_struct*)_tmp595)->tag == Cyc_Core_Not_found){_LL1C: _LL1D:
 return t;}else{_LL1E: _tmp596=_tmp595;_LL1F:(int)_rethrow(_tmp596);}_LL1B:;}};}case 8U: _LL3: _tmp5C9=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp590)->f1;_tmp5C8=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp590)->f2;_tmp5C7=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp590)->f3;_tmp5C6=(void*)((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp590)->f4;_LL4: {
# 3404
struct Cyc_List_List*_tmp597=Cyc_Tcutil_substs(rgn,inst,_tmp5C8);
return _tmp597 == _tmp5C8?t:(void*)({struct Cyc_Absyn_TypedefType_Absyn_Type_struct*_tmp598=_cycalloc(sizeof(*_tmp598));_tmp598->tag=8U,_tmp598->f1=_tmp5C9,_tmp598->f2=_tmp597,_tmp598->f3=_tmp5C7,_tmp598->f4=_tmp5C6;_tmp598;});}case 4U: _LL5: _tmp5CE=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp590)->f1).elt_type;_tmp5CD=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp590)->f1).tq;_tmp5CC=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp590)->f1).num_elts;_tmp5CB=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp590)->f1).zero_term;_tmp5CA=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp590)->f1).zt_loc;_LL6: {
# 3407
void*_tmp599=Cyc_Tcutil_rsubstitute(rgn,inst,_tmp5CE);
struct Cyc_Absyn_Exp*_tmp59A=_tmp5CC == 0?0: Cyc_Tcutil_rsubsexp(rgn,inst,_tmp5CC);
void*_tmp59B=Cyc_Tcutil_rsubstitute(rgn,inst,_tmp5CB);
return(_tmp599 == _tmp5CE  && _tmp59A == _tmp5CC) && _tmp59B == _tmp5CB?t:(void*)({struct Cyc_Absyn_ArrayType_Absyn_Type_struct*_tmp59C=_cycalloc(sizeof(*_tmp59C));
_tmp59C->tag=4U,(_tmp59C->f1).elt_type=_tmp599,(_tmp59C->f1).tq=_tmp5CD,(_tmp59C->f1).num_elts=_tmp59A,(_tmp59C->f1).zero_term=_tmp59B,(_tmp59C->f1).zt_loc=_tmp5CA;_tmp59C;});}case 3U: _LL7: _tmp5D4=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp590)->f1).elt_type;_tmp5D3=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp590)->f1).elt_tq;_tmp5D2=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp590)->f1).ptr_atts).rgn;_tmp5D1=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp590)->f1).ptr_atts).nullable;_tmp5D0=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp590)->f1).ptr_atts).bounds;_tmp5CF=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp590)->f1).ptr_atts).zero_term;_LL8: {
# 3413
void*_tmp59D=Cyc_Tcutil_rsubstitute(rgn,inst,_tmp5D4);
void*_tmp59E=Cyc_Tcutil_rsubstitute(rgn,inst,_tmp5D2);
void*_tmp59F=Cyc_Tcutil_rsubstitute(rgn,inst,_tmp5D0);
void*_tmp5A0=Cyc_Tcutil_rsubstitute(rgn,inst,_tmp5CF);
if(((_tmp59D == _tmp5D4  && _tmp59E == _tmp5D2) && _tmp59F == _tmp5D0) && _tmp5A0 == _tmp5CF)
return t;
return Cyc_Absyn_pointer_type(({struct Cyc_Absyn_PtrInfo _tmp9BA;_tmp9BA.elt_type=_tmp59D,_tmp9BA.elt_tq=_tmp5D3,(_tmp9BA.ptr_atts).rgn=_tmp59E,(_tmp9BA.ptr_atts).nullable=_tmp5D1,(_tmp9BA.ptr_atts).bounds=_tmp59F,(_tmp9BA.ptr_atts).zero_term=_tmp5A0,(_tmp9BA.ptr_atts).ptrloc=0;_tmp9BA;}));}case 5U: _LL9: _tmp5DF=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp590)->f1).tvars;_tmp5DE=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp590)->f1).effect;_tmp5DD=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp590)->f1).ret_tqual;_tmp5DC=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp590)->f1).ret_type;_tmp5DB=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp590)->f1).args;_tmp5DA=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp590)->f1).c_varargs;_tmp5D9=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp590)->f1).cyc_varargs;_tmp5D8=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp590)->f1).rgn_po;_tmp5D7=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp590)->f1).attributes;_tmp5D6=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp590)->f1).requires_clause;_tmp5D5=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp590)->f1).ensures_clause;_LLA:
# 3423
{struct Cyc_List_List*_tmp5A1=_tmp5DF;for(0;_tmp5A1 != 0;_tmp5A1=_tmp5A1->tl){
inst=({struct Cyc_List_List*_tmp5A3=_region_malloc(rgn,sizeof(*_tmp5A3));({struct _tuple15*_tmpB53=({struct _tuple15*_tmp5A2=_region_malloc(rgn,sizeof(*_tmp5A2));_tmp5A2->f1=(struct Cyc_Absyn_Tvar*)_tmp5A1->hd,({void*_tmpB52=Cyc_Absyn_var_type((struct Cyc_Absyn_Tvar*)_tmp5A1->hd);_tmp5A2->f2=_tmpB52;});_tmp5A2;});_tmp5A3->hd=_tmpB53;}),_tmp5A3->tl=inst;_tmp5A3;});}}{
struct _tuple1 _tmp5A4=({struct _RegionHandle*_tmpB55=rgn;struct _RegionHandle*_tmpB54=rgn;((struct _tuple1(*)(struct _RegionHandle*r1,struct _RegionHandle*r2,struct Cyc_List_List*x))Cyc_List_rsplit)(_tmpB55,_tmpB54,
((struct Cyc_List_List*(*)(struct _RegionHandle*,struct _tuple28*(*f)(struct _RegionHandle*,struct _tuple10*),struct _RegionHandle*env,struct Cyc_List_List*x))Cyc_List_rmap_c)(rgn,Cyc_Tcutil_substitute_f1,rgn,_tmp5DB));});
# 3425
struct _tuple1 _tmp5A5=_tmp5A4;struct Cyc_List_List*_tmp5B4;struct Cyc_List_List*_tmp5B3;_LL21: _tmp5B4=_tmp5A5.f1;_tmp5B3=_tmp5A5.f2;_LL22:;{
# 3427
struct Cyc_List_List*_tmp5A6=_tmp5DB;
struct Cyc_List_List*_tmp5A7=Cyc_Tcutil_substs(rgn,inst,_tmp5B3);
if(_tmp5A7 != _tmp5B3)
_tmp5A6=((struct Cyc_List_List*(*)(struct _tuple10*(*f)(struct _tuple10*,void*),struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_map2)(Cyc_Tcutil_substitute_f2,_tmp5DB,_tmp5A7);{
void*eff2;
if(_tmp5DE == 0)
eff2=0;else{
# 3435
void*new_eff=Cyc_Tcutil_rsubstitute(rgn,inst,_tmp5DE);
if(new_eff == _tmp5DE)
eff2=_tmp5DE;else{
# 3439
eff2=new_eff;}}{
# 3441
struct Cyc_Absyn_VarargInfo*cyc_varargs2;
if(_tmp5D9 == 0)
cyc_varargs2=0;else{
# 3445
struct Cyc_Absyn_VarargInfo _tmp5A8=*_tmp5D9;struct Cyc_Absyn_VarargInfo _tmp5A9=_tmp5A8;struct _dyneither_ptr*_tmp5AF;struct Cyc_Absyn_Tqual _tmp5AE;void*_tmp5AD;int _tmp5AC;_LL24: _tmp5AF=_tmp5A9.name;_tmp5AE=_tmp5A9.tq;_tmp5AD=_tmp5A9.type;_tmp5AC=_tmp5A9.inject;_LL25:;{
void*_tmp5AA=Cyc_Tcutil_rsubstitute(rgn,inst,_tmp5AD);
if(_tmp5AA == _tmp5AD)cyc_varargs2=_tmp5D9;else{
# 3449
cyc_varargs2=({struct Cyc_Absyn_VarargInfo*_tmp5AB=_cycalloc(sizeof(*_tmp5AB));_tmp5AB->name=_tmp5AF,_tmp5AB->tq=_tmp5AE,_tmp5AB->type=_tmp5AA,_tmp5AB->inject=_tmp5AC;_tmp5AB;});}};}{
# 3451
struct Cyc_List_List*rgn_po2=Cyc_Tcutil_rsubst_rgnpo(rgn,inst,_tmp5D8);
struct Cyc_Absyn_Exp*req2=Cyc_Tcutil_rsubs_exp_opt(rgn,inst,_tmp5D6);
struct Cyc_Absyn_Exp*ens2=Cyc_Tcutil_rsubs_exp_opt(rgn,inst,_tmp5D5);
struct Cyc_List_List*_tmp5B0=Cyc_Tcutil_extract_relns(_tmp5A6,req2);
struct Cyc_List_List*_tmp5B1=Cyc_Tcutil_extract_relns(_tmp5A6,ens2);
return(void*)({struct Cyc_Absyn_FnType_Absyn_Type_struct*_tmp5B2=_cycalloc(sizeof(*_tmp5B2));
_tmp5B2->tag=5U,(_tmp5B2->f1).tvars=_tmp5DF,(_tmp5B2->f1).effect=eff2,(_tmp5B2->f1).ret_tqual=_tmp5DD,({void*_tmpB56=Cyc_Tcutil_rsubstitute(rgn,inst,_tmp5DC);(_tmp5B2->f1).ret_type=_tmpB56;}),(_tmp5B2->f1).args=_tmp5A6,(_tmp5B2->f1).c_varargs=_tmp5DA,(_tmp5B2->f1).cyc_varargs=cyc_varargs2,(_tmp5B2->f1).rgn_po=rgn_po2,(_tmp5B2->f1).attributes=_tmp5D7,(_tmp5B2->f1).requires_clause=req2,(_tmp5B2->f1).requires_relns=_tmp5B0,(_tmp5B2->f1).ensures_clause=ens2,(_tmp5B2->f1).ensures_relns=_tmp5B1;_tmp5B2;});};};};};};case 6U: _LLB: _tmp5E0=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp590)->f1;_LLC: {
# 3461
struct Cyc_List_List*ts2=0;
int change=0;
{struct Cyc_List_List*_tmp5B5=_tmp5E0;for(0;_tmp5B5 != 0;_tmp5B5=_tmp5B5->tl){
void*_tmp5B6=(*((struct _tuple12*)_tmp5B5->hd)).f2;
void*_tmp5B7=Cyc_Tcutil_rsubstitute(rgn,inst,_tmp5B6);
if(_tmp5B6 != _tmp5B7)
change=1;
# 3469
ts2=({struct Cyc_List_List*_tmp5B8=_region_malloc(rgn,sizeof(*_tmp5B8));_tmp5B8->hd=_tmp5B7,_tmp5B8->tl=ts2;_tmp5B8;});}}
# 3471
if(!change)
return t;{
struct Cyc_List_List*_tmp5B9=({struct Cyc_List_List*_tmpB57=_tmp5E0;((struct Cyc_List_List*(*)(struct _tuple12*(*f)(struct _tuple12*,void*),struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_map2)(Cyc_Tcutil_map2_tq,_tmpB57,((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(ts2));});
return(void*)({struct Cyc_Absyn_TupleType_Absyn_Type_struct*_tmp5BA=_cycalloc(sizeof(*_tmp5BA));_tmp5BA->tag=6U,_tmp5BA->f1=_tmp5B9;_tmp5BA;});};}case 7U: _LLD: _tmp5E2=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp590)->f1;_tmp5E1=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp590)->f2;_LLE: {
# 3476
struct Cyc_List_List*_tmp5BB=Cyc_Tcutil_subst_aggrfields(rgn,inst,_tmp5E1);
if(_tmp5E1 == _tmp5BB)return t;
return(void*)({struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*_tmp5BC=_cycalloc(sizeof(*_tmp5BC));_tmp5BC->tag=7U,_tmp5BC->f1=_tmp5E2,_tmp5BC->f2=_tmp5BB;_tmp5BC;});}case 1U: _LLF: _tmp5E3=(void*)((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp590)->f2;_LL10:
# 3480
 if(_tmp5E3 != 0)return Cyc_Tcutil_rsubstitute(rgn,inst,_tmp5E3);else{
return t;}case 0U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp590)->f2 == 0){_LL11: _tmp5E4=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp590)->f1;_LL12:
 return t;}else{_LL13: _tmp5E6=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp590)->f1;_tmp5E5=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp590)->f2;_LL14: {
# 3484
struct Cyc_List_List*_tmp5BD=Cyc_Tcutil_substs(rgn,inst,_tmp5E5);
if(_tmp5E5 == _tmp5BD)return t;else{
return(void*)({struct Cyc_Absyn_AppType_Absyn_Type_struct*_tmp5BE=_cycalloc(sizeof(*_tmp5BE));_tmp5BE->tag=0U,_tmp5BE->f1=_tmp5E6,_tmp5BE->f2=_tmp5BD;_tmp5BE;});}}}case 9U: _LL15: _tmp5E7=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp590)->f1;_LL16: {
# 3488
struct Cyc_Absyn_Exp*_tmp5BF=Cyc_Tcutil_rsubsexp(rgn,inst,_tmp5E7);
return _tmp5BF == _tmp5E7?t:(void*)({struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_tmp5C0=_cycalloc(sizeof(*_tmp5C0));_tmp5C0->tag=9U,_tmp5C0->f1=_tmp5BF;_tmp5C0;});}case 11U: _LL17: _tmp5E8=((struct Cyc_Absyn_TypeofType_Absyn_Type_struct*)_tmp590)->f1;_LL18: {
# 3491
struct Cyc_Absyn_Exp*_tmp5C1=Cyc_Tcutil_rsubsexp(rgn,inst,_tmp5E8);
return _tmp5C1 == _tmp5E8?t:(void*)({struct Cyc_Absyn_TypeofType_Absyn_Type_struct*_tmp5C2=_cycalloc(sizeof(*_tmp5C2));_tmp5C2->tag=11U,_tmp5C2->f1=_tmp5C1;_tmp5C2;});}default: _LL19: _LL1A:
({void*_tmp5C3=0U;({struct _dyneither_ptr _tmpB58=({const char*_tmp5C4="found typedecltype in rsubs";_tag_dyneither(_tmp5C4,sizeof(char),28U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpB58,_tag_dyneither(_tmp5C3,sizeof(void*),0U));});});}_LL0:;}
# 3497
static struct Cyc_List_List*Cyc_Tcutil_substs(struct _RegionHandle*rgn,struct Cyc_List_List*inst,struct Cyc_List_List*ts){
# 3500
if(ts == 0)
return 0;{
void*_tmp5E9=(void*)ts->hd;
struct Cyc_List_List*_tmp5EA=ts->tl;
void*_tmp5EB=Cyc_Tcutil_rsubstitute(rgn,inst,_tmp5E9);
struct Cyc_List_List*_tmp5EC=Cyc_Tcutil_substs(rgn,inst,_tmp5EA);
if(_tmp5E9 == _tmp5EB  && _tmp5EA == _tmp5EC)
return ts;
return({struct Cyc_List_List*_tmp5ED=_cycalloc(sizeof(*_tmp5ED));_tmp5ED->hd=_tmp5EB,_tmp5ED->tl=_tmp5EC;_tmp5ED;});};}
# 3511
extern void*Cyc_Tcutil_substitute(struct Cyc_List_List*inst,void*t){
if(inst != 0)
return Cyc_Tcutil_rsubstitute(Cyc_Core_heap_region,inst,t);else{
return t;}}
# 3518
struct _tuple15*Cyc_Tcutil_make_inst_var(struct Cyc_List_List*s,struct Cyc_Absyn_Tvar*tv){
struct Cyc_Core_Opt*_tmp5EE=Cyc_Tcutil_kind_to_opt(Cyc_Tcutil_tvar_kind(tv,& Cyc_Tcutil_bk));
return({struct _tuple15*_tmp5F0=_cycalloc(sizeof(*_tmp5F0));_tmp5F0->f1=tv,({void*_tmpB5A=({struct Cyc_Core_Opt*_tmpB59=_tmp5EE;Cyc_Absyn_new_evar(_tmpB59,({struct Cyc_Core_Opt*_tmp5EF=_cycalloc(sizeof(*_tmp5EF));_tmp5EF->v=s;_tmp5EF;}));});_tmp5F0->f2=_tmpB5A;});_tmp5F0;});}
# 3523
struct _tuple15*Cyc_Tcutil_r_make_inst_var(struct _tuple16*env,struct Cyc_Absyn_Tvar*tv){
# 3525
struct _tuple16*_tmp5F1=env;struct Cyc_List_List*_tmp5F6;struct _RegionHandle*_tmp5F5;_LL1: _tmp5F6=_tmp5F1->f1;_tmp5F5=_tmp5F1->f2;_LL2:;{
struct Cyc_Core_Opt*_tmp5F2=Cyc_Tcutil_kind_to_opt(Cyc_Tcutil_tvar_kind(tv,& Cyc_Tcutil_bk));
return({struct _tuple15*_tmp5F4=_region_malloc(_tmp5F5,sizeof(*_tmp5F4));_tmp5F4->f1=tv,({void*_tmpB5C=({struct Cyc_Core_Opt*_tmpB5B=_tmp5F2;Cyc_Absyn_new_evar(_tmpB5B,({struct Cyc_Core_Opt*_tmp5F3=_cycalloc(sizeof(*_tmp5F3));_tmp5F3->v=_tmp5F6;_tmp5F3;}));});_tmp5F4->f2=_tmpB5C;});_tmp5F4;});};}
# 3535
static struct Cyc_List_List*Cyc_Tcutil_add_free_tvar(unsigned int loc,struct Cyc_List_List*tvs,struct Cyc_Absyn_Tvar*tv){
# 3539
{struct Cyc_List_List*tvs2=tvs;for(0;tvs2 != 0;tvs2=tvs2->tl){
if(Cyc_strptrcmp(((struct Cyc_Absyn_Tvar*)tvs2->hd)->name,tv->name)== 0){
void*k1=((struct Cyc_Absyn_Tvar*)tvs2->hd)->kind;
void*k2=tv->kind;
if(!Cyc_Tcutil_constrain_kinds(k1,k2))
({struct Cyc_String_pa_PrintArg_struct _tmp5F9=({struct Cyc_String_pa_PrintArg_struct _tmp9BD;_tmp9BD.tag=0U,_tmp9BD.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*tv->name);_tmp9BD;});struct Cyc_String_pa_PrintArg_struct _tmp5FA=({struct Cyc_String_pa_PrintArg_struct _tmp9BC;_tmp9BC.tag=0U,({
struct _dyneither_ptr _tmpB5D=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_kindbound2string(k1));_tmp9BC.f1=_tmpB5D;});_tmp9BC;});struct Cyc_String_pa_PrintArg_struct _tmp5FB=({struct Cyc_String_pa_PrintArg_struct _tmp9BB;_tmp9BB.tag=0U,({struct _dyneither_ptr _tmpB5E=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_kindbound2string(k2));_tmp9BB.f1=_tmpB5E;});_tmp9BB;});void*_tmp5F7[3U];_tmp5F7[0]=& _tmp5F9,_tmp5F7[1]=& _tmp5FA,_tmp5F7[2]=& _tmp5FB;({unsigned int _tmpB60=loc;struct _dyneither_ptr _tmpB5F=({const char*_tmp5F8="type variable %s is used with inconsistent kinds %s and %s";_tag_dyneither(_tmp5F8,sizeof(char),59U);});Cyc_Tcutil_terr(_tmpB60,_tmpB5F,_tag_dyneither(_tmp5F7,sizeof(void*),3U));});});
if(tv->identity == - 1)
tv->identity=((struct Cyc_Absyn_Tvar*)tvs2->hd)->identity;else{
if(tv->identity != ((struct Cyc_Absyn_Tvar*)tvs2->hd)->identity)
({void*_tmp5FC=0U;({struct _dyneither_ptr _tmpB61=({const char*_tmp5FD="same type variable has different identity!";_tag_dyneither(_tmp5FD,sizeof(char),43U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpB61,_tag_dyneither(_tmp5FC,sizeof(void*),0U));});});}
return tvs;}}}
# 3553
({int _tmpB62=Cyc_Tcutil_new_tvar_id();tv->identity=_tmpB62;});
return({struct Cyc_List_List*_tmp5FE=_cycalloc(sizeof(*_tmp5FE));_tmp5FE->hd=tv,_tmp5FE->tl=tvs;_tmp5FE;});}
# 3559
static struct Cyc_List_List*Cyc_Tcutil_fast_add_free_tvar(struct Cyc_List_List*tvs,struct Cyc_Absyn_Tvar*tv){
# 3561
if(tv->identity == - 1)
({void*_tmp5FF=0U;({struct _dyneither_ptr _tmpB63=({const char*_tmp600="fast_add_free_tvar: bad identity in tv";_tag_dyneither(_tmp600,sizeof(char),39U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpB63,_tag_dyneither(_tmp5FF,sizeof(void*),0U));});});
{struct Cyc_List_List*tvs2=tvs;for(0;tvs2 != 0;tvs2=tvs2->tl){
# 3565
struct Cyc_Absyn_Tvar*_tmp601=(struct Cyc_Absyn_Tvar*)tvs2->hd;
if(_tmp601->identity == - 1)
({void*_tmp602=0U;({struct _dyneither_ptr _tmpB64=({const char*_tmp603="fast_add_free_tvar: bad identity in tvs2";_tag_dyneither(_tmp603,sizeof(char),41U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpB64,_tag_dyneither(_tmp602,sizeof(void*),0U));});});
if(_tmp601->identity == tv->identity)
return tvs;}}
# 3572
return({struct Cyc_List_List*_tmp604=_cycalloc(sizeof(*_tmp604));_tmp604->hd=tv,_tmp604->tl=tvs;_tmp604;});}struct _tuple29{struct Cyc_Absyn_Tvar*f1;int f2;};
# 3578
static struct Cyc_List_List*Cyc_Tcutil_fast_add_free_tvar_bool(struct _RegionHandle*r,struct Cyc_List_List*tvs,struct Cyc_Absyn_Tvar*tv,int b){
# 3583
if(tv->identity == - 1)
({void*_tmp605=0U;({struct _dyneither_ptr _tmpB65=({const char*_tmp606="fast_add_free_tvar_bool: bad identity in tv";_tag_dyneither(_tmp606,sizeof(char),44U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpB65,_tag_dyneither(_tmp605,sizeof(void*),0U));});});
{struct Cyc_List_List*tvs2=tvs;for(0;tvs2 != 0;tvs2=tvs2->tl){
# 3587
struct _tuple29*_tmp607=(struct _tuple29*)tvs2->hd;struct _tuple29*_tmp608=_tmp607;struct Cyc_Absyn_Tvar*_tmp60C;int*_tmp60B;_LL1: _tmp60C=_tmp608->f1;_tmp60B=(int*)& _tmp608->f2;_LL2:;
if(_tmp60C->identity == - 1)
({void*_tmp609=0U;({struct _dyneither_ptr _tmpB66=({const char*_tmp60A="fast_add_free_tvar_bool: bad identity in tvs2";_tag_dyneither(_tmp60A,sizeof(char),46U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpB66,_tag_dyneither(_tmp609,sizeof(void*),0U));});});
if(_tmp60C->identity == tv->identity){
*_tmp60B=*_tmp60B  || b;
return tvs;}}}
# 3595
return({struct Cyc_List_List*_tmp60E=_region_malloc(r,sizeof(*_tmp60E));({struct _tuple29*_tmpB67=({struct _tuple29*_tmp60D=_region_malloc(r,sizeof(*_tmp60D));_tmp60D->f1=tv,_tmp60D->f2=b;_tmp60D;});_tmp60E->hd=_tmpB67;}),_tmp60E->tl=tvs;_tmp60E;});}
# 3599
static struct Cyc_List_List*Cyc_Tcutil_add_bound_tvar(struct Cyc_List_List*tvs,struct Cyc_Absyn_Tvar*tv){
# 3601
if(tv->identity == - 1)
({struct Cyc_String_pa_PrintArg_struct _tmp611=({struct Cyc_String_pa_PrintArg_struct _tmp9BE;_tmp9BE.tag=0U,({struct _dyneither_ptr _tmpB68=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_tvar2string(tv));_tmp9BE.f1=_tmpB68;});_tmp9BE;});void*_tmp60F[1U];_tmp60F[0]=& _tmp611;({struct _dyneither_ptr _tmpB69=({const char*_tmp610="bound tvar id for %s is NULL";_tag_dyneither(_tmp610,sizeof(char),29U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpB69,_tag_dyneither(_tmp60F,sizeof(void*),1U));});});
return({struct Cyc_List_List*_tmp612=_cycalloc(sizeof(*_tmp612));_tmp612->hd=tv,_tmp612->tl=tvs;_tmp612;});}struct _tuple30{void*f1;int f2;};
# 3610
static struct Cyc_List_List*Cyc_Tcutil_add_free_evar(struct _RegionHandle*r,struct Cyc_List_List*es,void*e,int b){
# 3613
void*_tmp613=Cyc_Tcutil_compress(e);void*_tmp614=_tmp613;int _tmp61E;if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp614)->tag == 1U){_LL1: _tmp61E=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp614)->f3;_LL2:
# 3615
{struct Cyc_List_List*es2=es;for(0;es2 != 0;es2=es2->tl){
struct _tuple30*_tmp615=(struct _tuple30*)es2->hd;struct _tuple30*_tmp616=_tmp615;void*_tmp61B;int*_tmp61A;_LL6: _tmp61B=_tmp616->f1;_tmp61A=(int*)& _tmp616->f2;_LL7:;{
void*_tmp617=Cyc_Tcutil_compress(_tmp61B);void*_tmp618=_tmp617;int _tmp619;if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp618)->tag == 1U){_LL9: _tmp619=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp618)->f3;_LLA:
# 3619
 if(_tmp61E == _tmp619){
if(b != *_tmp61A)*_tmp61A=1;
return es;}
# 3623
goto _LL8;}else{_LLB: _LLC:
 goto _LL8;}_LL8:;};}}
# 3627
return({struct Cyc_List_List*_tmp61D=_region_malloc(r,sizeof(*_tmp61D));({struct _tuple30*_tmpB6A=({struct _tuple30*_tmp61C=_region_malloc(r,sizeof(*_tmp61C));_tmp61C->f1=e,_tmp61C->f2=b;_tmp61C;});_tmp61D->hd=_tmpB6A;}),_tmp61D->tl=es;_tmp61D;});}else{_LL3: _LL4:
 return es;}_LL0:;}
# 3632
static struct Cyc_List_List*Cyc_Tcutil_remove_bound_tvars(struct _RegionHandle*rgn,struct Cyc_List_List*tvs,struct Cyc_List_List*btvs){
# 3635
struct Cyc_List_List*r=0;
for(0;tvs != 0;tvs=tvs->tl){
int present=0;
{struct Cyc_List_List*b=btvs;for(0;b != 0;b=b->tl){
if(((struct Cyc_Absyn_Tvar*)tvs->hd)->identity == ((struct Cyc_Absyn_Tvar*)b->hd)->identity){
present=1;
break;}}}
# 3644
if(!present)r=({struct Cyc_List_List*_tmp61F=_region_malloc(rgn,sizeof(*_tmp61F));_tmp61F->hd=(struct Cyc_Absyn_Tvar*)tvs->hd,_tmp61F->tl=r;_tmp61F;});}
# 3646
r=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(r);
return r;}
# 3651
static struct Cyc_List_List*Cyc_Tcutil_remove_bound_tvars_bool(struct _RegionHandle*r,struct Cyc_List_List*tvs,struct Cyc_List_List*btvs){
# 3655
struct Cyc_List_List*res=0;
for(0;tvs != 0;tvs=tvs->tl){
struct _tuple29 _tmp620=*((struct _tuple29*)tvs->hd);struct _tuple29 _tmp621=_tmp620;struct Cyc_Absyn_Tvar*_tmp624;int _tmp623;_LL1: _tmp624=_tmp621.f1;_tmp623=_tmp621.f2;_LL2:;{
int present=0;
{struct Cyc_List_List*b=btvs;for(0;b != 0;b=b->tl){
if(_tmp624->identity == ((struct Cyc_Absyn_Tvar*)b->hd)->identity){
present=1;
break;}}}
# 3665
if(!present)res=({struct Cyc_List_List*_tmp622=_region_malloc(r,sizeof(*_tmp622));_tmp622->hd=(struct _tuple29*)tvs->hd,_tmp622->tl=res;_tmp622;});};}
# 3667
res=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(res);
return res;}
# 3671
void Cyc_Tcutil_check_bitfield(unsigned int loc,struct Cyc_Tcenv_Tenv*te,void*field_type,struct Cyc_Absyn_Exp*width,struct _dyneither_ptr*fn){
# 3673
if(width != 0){
unsigned int w=0U;
if(!Cyc_Tcutil_is_const_exp(width))
({struct Cyc_String_pa_PrintArg_struct _tmp627=({struct Cyc_String_pa_PrintArg_struct _tmp9BF;_tmp9BF.tag=0U,_tmp9BF.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*fn);_tmp9BF;});void*_tmp625[1U];_tmp625[0]=& _tmp627;({unsigned int _tmpB6C=loc;struct _dyneither_ptr _tmpB6B=({const char*_tmp626="bitfield %s does not have constant width";_tag_dyneither(_tmp626,sizeof(char),41U);});Cyc_Tcutil_terr(_tmpB6C,_tmpB6B,_tag_dyneither(_tmp625,sizeof(void*),1U));});});else{
# 3678
struct _tuple13 _tmp628=Cyc_Evexp_eval_const_uint_exp(width);struct _tuple13 _tmp629=_tmp628;unsigned int _tmp62F;int _tmp62E;_LL1: _tmp62F=_tmp629.f1;_tmp62E=_tmp629.f2;_LL2:;
if(!_tmp62E)
({void*_tmp62A=0U;({unsigned int _tmpB6E=loc;struct _dyneither_ptr _tmpB6D=({const char*_tmp62B="cannot evaluate bitfield width at compile time";_tag_dyneither(_tmp62B,sizeof(char),47U);});Cyc_Tcutil_warn(_tmpB6E,_tmpB6D,_tag_dyneither(_tmp62A,sizeof(void*),0U));});});
if((int)_tmp62F < 0)
({void*_tmp62C=0U;({unsigned int _tmpB70=loc;struct _dyneither_ptr _tmpB6F=({const char*_tmp62D="bitfield has negative width";_tag_dyneither(_tmp62D,sizeof(char),28U);});Cyc_Tcutil_terr(_tmpB70,_tmpB6F,_tag_dyneither(_tmp62C,sizeof(void*),0U));});});
w=_tmp62F;}{
# 3685
void*_tmp630=Cyc_Tcutil_compress(field_type);void*_tmp631=_tmp630;enum Cyc_Absyn_Size_of _tmp63F;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp631)->tag == 0U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp631)->f1)->tag == 1U){_LL4: _tmp63F=((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp631)->f1)->f2;_LL5:
# 3688
{enum Cyc_Absyn_Size_of _tmp632=_tmp63F;switch(_tmp632){case Cyc_Absyn_Char_sz: _LL9: _LLA:
 if(w > (unsigned int)8)({void*_tmp633=0U;({unsigned int _tmpB72=loc;struct _dyneither_ptr _tmpB71=({const char*_tmp634="bitfield larger than type";_tag_dyneither(_tmp634,sizeof(char),26U);});Cyc_Tcutil_warn(_tmpB72,_tmpB71,_tag_dyneither(_tmp633,sizeof(void*),0U));});});goto _LL8;case Cyc_Absyn_Short_sz: _LLB: _LLC:
 if(w > (unsigned int)16)({void*_tmp635=0U;({unsigned int _tmpB74=loc;struct _dyneither_ptr _tmpB73=({const char*_tmp636="bitfield larger than type";_tag_dyneither(_tmp636,sizeof(char),26U);});Cyc_Tcutil_warn(_tmpB74,_tmpB73,_tag_dyneither(_tmp635,sizeof(void*),0U));});});goto _LL8;case Cyc_Absyn_Long_sz: _LLD: _LLE:
 goto _LL10;case Cyc_Absyn_Int_sz: _LLF: _LL10:
# 3693
 if(w > (unsigned int)32)({void*_tmp637=0U;({unsigned int _tmpB76=loc;struct _dyneither_ptr _tmpB75=({const char*_tmp638="bitfield larger than type";_tag_dyneither(_tmp638,sizeof(char),26U);});Cyc_Tcutil_warn(_tmpB76,_tmpB75,_tag_dyneither(_tmp637,sizeof(void*),0U));});});goto _LL8;default: _LL11: _LL12:
# 3695
 if(w > (unsigned int)64)({void*_tmp639=0U;({unsigned int _tmpB78=loc;struct _dyneither_ptr _tmpB77=({const char*_tmp63A="bitfield larger than type";_tag_dyneither(_tmp63A,sizeof(char),26U);});Cyc_Tcutil_warn(_tmpB78,_tmpB77,_tag_dyneither(_tmp639,sizeof(void*),0U));});});goto _LL8;}_LL8:;}
# 3697
goto _LL3;}else{goto _LL6;}}else{_LL6: _LL7:
# 3699
({struct Cyc_String_pa_PrintArg_struct _tmp63D=({struct Cyc_String_pa_PrintArg_struct _tmp9C1;_tmp9C1.tag=0U,_tmp9C1.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*fn);_tmp9C1;});struct Cyc_String_pa_PrintArg_struct _tmp63E=({struct Cyc_String_pa_PrintArg_struct _tmp9C0;_tmp9C0.tag=0U,({
struct _dyneither_ptr _tmpB79=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(field_type));_tmp9C0.f1=_tmpB79;});_tmp9C0;});void*_tmp63B[2U];_tmp63B[0]=& _tmp63D,_tmp63B[1]=& _tmp63E;({unsigned int _tmpB7B=loc;struct _dyneither_ptr _tmpB7A=({const char*_tmp63C="bitfield %s must have integral type but has type %s";_tag_dyneither(_tmp63C,sizeof(char),52U);});Cyc_Tcutil_terr(_tmpB7B,_tmpB7A,_tag_dyneither(_tmp63B,sizeof(void*),2U));});});
goto _LL3;}_LL3:;};}}
# 3706
static void Cyc_Tcutil_check_field_atts(unsigned int loc,struct _dyneither_ptr*fn,struct Cyc_List_List*atts){
for(0;atts != 0;atts=atts->tl){
void*_tmp640=(void*)atts->hd;void*_tmp641=_tmp640;switch(*((int*)_tmp641)){case 7U: _LL1: _LL2:
 continue;case 6U: _LL3: _LL4:
 continue;default: _LL5: _LL6:
({struct Cyc_String_pa_PrintArg_struct _tmp644=({struct Cyc_String_pa_PrintArg_struct _tmp9C3;_tmp9C3.tag=0U,({
struct _dyneither_ptr _tmpB7C=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absyn_attribute2string((void*)atts->hd));_tmp9C3.f1=_tmpB7C;});_tmp9C3;});struct Cyc_String_pa_PrintArg_struct _tmp645=({struct Cyc_String_pa_PrintArg_struct _tmp9C2;_tmp9C2.tag=0U,_tmp9C2.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*fn);_tmp9C2;});void*_tmp642[2U];_tmp642[0]=& _tmp644,_tmp642[1]=& _tmp645;({unsigned int _tmpB7E=loc;struct _dyneither_ptr _tmpB7D=({const char*_tmp643="bad attribute %s on member %s";_tag_dyneither(_tmp643,sizeof(char),30U);});Cyc_Tcutil_terr(_tmpB7E,_tmpB7D,_tag_dyneither(_tmp642,sizeof(void*),2U));});});}_LL0:;}}struct Cyc_Tcutil_CVTEnv{struct _RegionHandle*r;struct Cyc_List_List*kind_env;struct Cyc_List_List*free_vars;struct Cyc_List_List*free_evars;int generalize_evars;int fn_result;};
# 3734
int Cyc_Tcutil_extract_const_from_typedef(unsigned int loc,int declared_const,void*t){
void*_tmp646=t;struct Cyc_Absyn_Typedefdecl*_tmp64A;void*_tmp649;if(((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp646)->tag == 8U){_LL1: _tmp64A=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp646)->f3;_tmp649=(void*)((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp646)->f4;_LL2:
# 3737
 if((((struct Cyc_Absyn_Typedefdecl*)_check_null(_tmp64A))->tq).real_const  || (_tmp64A->tq).print_const){
if(declared_const)({void*_tmp647=0U;({unsigned int _tmpB80=loc;struct _dyneither_ptr _tmpB7F=({const char*_tmp648="extra const";_tag_dyneither(_tmp648,sizeof(char),12U);});Cyc_Tcutil_warn(_tmpB80,_tmpB7F,_tag_dyneither(_tmp647,sizeof(void*),0U));});});
return 1;}
# 3742
if((unsigned int)_tmp649)
return Cyc_Tcutil_extract_const_from_typedef(loc,declared_const,_tmp649);else{
return declared_const;}}else{_LL3: _LL4:
 return declared_const;}_LL0:;}
# 3749
static int Cyc_Tcutil_typedef_tvar_is_ptr_rgn(struct Cyc_Absyn_Tvar*tvar,struct Cyc_Absyn_Typedefdecl*td){
if(td != 0){
if(td->defn != 0){
void*_tmp64B=Cyc_Tcutil_compress((void*)_check_null(td->defn));void*_tmp64C=_tmp64B;void*_tmp650;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp64C)->tag == 3U){_LL1: _tmp650=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp64C)->f1).ptr_atts).rgn;_LL2:
# 3754
{void*_tmp64D=Cyc_Tcutil_compress(_tmp650);void*_tmp64E=_tmp64D;struct Cyc_Absyn_Tvar*_tmp64F;if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp64E)->tag == 2U){_LL6: _tmp64F=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp64E)->f1;_LL7:
# 3756
 return Cyc_Absyn_tvar_cmp(tvar,_tmp64F)== 0;}else{_LL8: _LL9:
 goto _LL5;}_LL5:;}
# 3759
goto _LL0;}else{_LL3: _LL4:
 goto _LL0;}_LL0:;}}else{
# 3765
return 1;}
return 0;}
# 3769
static struct Cyc_Absyn_Kind*Cyc_Tcutil_tvar_inst_kind(struct Cyc_Absyn_Tvar*tvar,struct Cyc_Absyn_Kind*def_kind,struct Cyc_Absyn_Kind*expected_kind,struct Cyc_Absyn_Typedefdecl*td){
# 3772
void*_tmp651=Cyc_Absyn_compress_kb(tvar->kind);void*_tmp652=_tmp651;switch(*((int*)_tmp652)){case 2U: if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp652)->f2)->kind == Cyc_Absyn_RgnKind){if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp652)->f2)->aliasqual == Cyc_Absyn_Top){_LL1: _LL2:
 goto _LL4;}else{goto _LL5;}}else{goto _LL5;}case 0U: if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp652)->f1)->kind == Cyc_Absyn_RgnKind){if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp652)->f1)->aliasqual == Cyc_Absyn_Top){_LL3: _LL4:
# 3781
 if((((int)expected_kind->kind == (int)Cyc_Absyn_BoxKind  || (int)expected_kind->kind == (int)Cyc_Absyn_MemKind) || (int)expected_kind->kind == (int)Cyc_Absyn_AnyKind) && 
# 3784
Cyc_Tcutil_typedef_tvar_is_ptr_rgn(tvar,td)){
if((int)expected_kind->aliasqual == (int)Cyc_Absyn_Aliasable)
return& Cyc_Tcutil_rk;else{
if((int)expected_kind->aliasqual == (int)Cyc_Absyn_Unique)
return& Cyc_Tcutil_urk;}}
# 3790
return& Cyc_Tcutil_trk;}else{goto _LL5;}}else{goto _LL5;}default: _LL5: _LL6:
 return Cyc_Tcutil_tvar_kind(tvar,def_kind);}_LL0:;}
# 3796
static struct Cyc_Tcutil_CVTEnv Cyc_Tcutil_i_check_valid_type_level_exp(struct Cyc_Absyn_Exp*e,struct Cyc_Tcenv_Tenv*te,struct Cyc_Tcutil_CVTEnv cvtenv);struct _tuple31{struct Cyc_Tcutil_CVTEnv f1;struct Cyc_List_List*f2;};
# 3800
static struct _tuple31 Cyc_Tcutil_check_clause(unsigned int loc,struct Cyc_Tcenv_Tenv*te,struct Cyc_Tcutil_CVTEnv cvtenv,struct _dyneither_ptr clause_name,struct Cyc_Absyn_Exp*clause){
# 3803
struct Cyc_List_List*relns=0;
if(clause != 0){
Cyc_Tcexp_tcExp(te,0,clause);
if(!Cyc_Tcutil_is_integral(clause))
({struct Cyc_String_pa_PrintArg_struct _tmp655=({struct Cyc_String_pa_PrintArg_struct _tmp9C5;_tmp9C5.tag=0U,_tmp9C5.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)clause_name);_tmp9C5;});struct Cyc_String_pa_PrintArg_struct _tmp656=({struct Cyc_String_pa_PrintArg_struct _tmp9C4;_tmp9C4.tag=0U,({
struct _dyneither_ptr _tmpB81=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string((void*)_check_null(clause->topt)));_tmp9C4.f1=_tmpB81;});_tmp9C4;});void*_tmp653[2U];_tmp653[0]=& _tmp655,_tmp653[1]=& _tmp656;({unsigned int _tmpB83=loc;struct _dyneither_ptr _tmpB82=({const char*_tmp654="%s clause has type %s instead of integral type";_tag_dyneither(_tmp654,sizeof(char),47U);});Cyc_Tcutil_terr(_tmpB83,_tmpB82,_tag_dyneither(_tmp653,sizeof(void*),2U));});});
cvtenv=Cyc_Tcutil_i_check_valid_type_level_exp(clause,te,cvtenv);
relns=Cyc_Relations_exp2relns(Cyc_Core_heap_region,clause);
if(!Cyc_Relations_consistent_relations(relns))
({struct Cyc_String_pa_PrintArg_struct _tmp659=({struct Cyc_String_pa_PrintArg_struct _tmp9C7;_tmp9C7.tag=0U,_tmp9C7.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)clause_name);_tmp9C7;});struct Cyc_String_pa_PrintArg_struct _tmp65A=({struct Cyc_String_pa_PrintArg_struct _tmp9C6;_tmp9C6.tag=0U,({
struct _dyneither_ptr _tmpB84=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_exp2string(clause));_tmp9C6.f1=_tmpB84;});_tmp9C6;});void*_tmp657[2U];_tmp657[0]=& _tmp659,_tmp657[1]=& _tmp65A;({unsigned int _tmpB86=clause->loc;struct _dyneither_ptr _tmpB85=({const char*_tmp658="%s clause '%s' may be unsatisfiable";_tag_dyneither(_tmp658,sizeof(char),36U);});Cyc_Tcutil_terr(_tmpB86,_tmpB85,_tag_dyneither(_tmp657,sizeof(void*),2U));});});}
# 3815
return({struct _tuple31 _tmp9C8;_tmp9C8.f1=cvtenv,_tmp9C8.f2=relns;_tmp9C8;});}
# 3845 "tcutil.cyc"
static struct Cyc_Tcutil_CVTEnv Cyc_Tcutil_i_check_valid_type(unsigned int loc,struct Cyc_Tcenv_Tenv*te,struct Cyc_Tcutil_CVTEnv cvtenv,struct Cyc_Absyn_Kind*expected_kind,void*t,int put_in_effect,int allow_abs_aggr);
# 3852
static struct Cyc_Tcutil_CVTEnv Cyc_Tcutil_i_check_valid_aggr(unsigned int loc,struct Cyc_Tcenv_Tenv*te,struct Cyc_Tcutil_CVTEnv cvtenv,struct Cyc_Absyn_Kind*expected_kind,union Cyc_Absyn_AggrInfo*info,struct Cyc_List_List**targs,int allow_abs_aggr){
# 3858
{union Cyc_Absyn_AggrInfo _tmp65B=*info;union Cyc_Absyn_AggrInfo _tmp65C=_tmp65B;struct Cyc_Absyn_Aggrdecl*_tmp67F;enum Cyc_Absyn_AggrKind _tmp67E;struct _tuple2*_tmp67D;struct Cyc_Core_Opt*_tmp67C;if((_tmp65C.UnknownAggr).tag == 1){_LL1: _tmp67E=((_tmp65C.UnknownAggr).val).f1;_tmp67D=((_tmp65C.UnknownAggr).val).f2;_tmp67C=((_tmp65C.UnknownAggr).val).f3;_LL2: {
# 3860
struct Cyc_Absyn_Aggrdecl**adp;
{struct _handler_cons _tmp65D;_push_handler(& _tmp65D);{int _tmp65F=0;if(setjmp(_tmp65D.handler))_tmp65F=1;if(!_tmp65F){
adp=Cyc_Tcenv_lookup_aggrdecl(te,loc,_tmp67D);{
struct Cyc_Absyn_Aggrdecl*_tmp660=*adp;
if((int)_tmp660->kind != (int)_tmp67E){
if((int)_tmp660->kind == (int)Cyc_Absyn_StructA)
({struct Cyc_String_pa_PrintArg_struct _tmp663=({struct Cyc_String_pa_PrintArg_struct _tmp9CA;_tmp9CA.tag=0U,({
struct _dyneither_ptr _tmpB87=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_qvar2string(_tmp67D));_tmp9CA.f1=_tmpB87;});_tmp9CA;});struct Cyc_String_pa_PrintArg_struct _tmp664=({struct Cyc_String_pa_PrintArg_struct _tmp9C9;_tmp9C9.tag=0U,({struct _dyneither_ptr _tmpB88=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_qvar2string(_tmp67D));_tmp9C9.f1=_tmpB88;});_tmp9C9;});void*_tmp661[2U];_tmp661[0]=& _tmp663,_tmp661[1]=& _tmp664;({unsigned int _tmpB8A=loc;struct _dyneither_ptr _tmpB89=({const char*_tmp662="expecting struct %s instead of union %s";_tag_dyneither(_tmp662,sizeof(char),40U);});Cyc_Tcutil_terr(_tmpB8A,_tmpB89,_tag_dyneither(_tmp661,sizeof(void*),2U));});});else{
# 3869
({struct Cyc_String_pa_PrintArg_struct _tmp667=({struct Cyc_String_pa_PrintArg_struct _tmp9CC;_tmp9CC.tag=0U,({
struct _dyneither_ptr _tmpB8B=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_qvar2string(_tmp67D));_tmp9CC.f1=_tmpB8B;});_tmp9CC;});struct Cyc_String_pa_PrintArg_struct _tmp668=({struct Cyc_String_pa_PrintArg_struct _tmp9CB;_tmp9CB.tag=0U,({struct _dyneither_ptr _tmpB8C=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_qvar2string(_tmp67D));_tmp9CB.f1=_tmpB8C;});_tmp9CB;});void*_tmp665[2U];_tmp665[0]=& _tmp667,_tmp665[1]=& _tmp668;({unsigned int _tmpB8E=loc;struct _dyneither_ptr _tmpB8D=({const char*_tmp666="expecting union %s instead of struct %s";_tag_dyneither(_tmp666,sizeof(char),40U);});Cyc_Tcutil_terr(_tmpB8E,_tmpB8D,_tag_dyneither(_tmp665,sizeof(void*),2U));});});}}
# 3872
if((unsigned int)_tmp67C  && (int)_tmp67C->v){
if(!((unsigned int)_tmp660->impl) || !((struct Cyc_Absyn_AggrdeclImpl*)_check_null(_tmp660->impl))->tagged)
({struct Cyc_String_pa_PrintArg_struct _tmp66B=({struct Cyc_String_pa_PrintArg_struct _tmp9CD;_tmp9CD.tag=0U,({
struct _dyneither_ptr _tmpB8F=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_qvar2string(_tmp67D));_tmp9CD.f1=_tmpB8F;});_tmp9CD;});void*_tmp669[1U];_tmp669[0]=& _tmp66B;({unsigned int _tmpB91=loc;struct _dyneither_ptr _tmpB90=({const char*_tmp66A="@tagged qualfiers don't agree on union %s";_tag_dyneither(_tmp66A,sizeof(char),42U);});Cyc_Tcutil_terr(_tmpB91,_tmpB90,_tag_dyneither(_tmp669,sizeof(void*),1U));});});}
# 3878
({union Cyc_Absyn_AggrInfo _tmpB92=Cyc_Absyn_KnownAggr(adp);*info=_tmpB92;});};
# 3862
;_pop_handler();}else{void*_tmp65E=(void*)_exn_thrown;void*_tmp66C=_tmp65E;void*_tmp672;if(((struct Cyc_Dict_Absent_exn_struct*)_tmp66C)->tag == Cyc_Dict_Absent){_LL6: _LL7: {
# 3882
struct Cyc_Absyn_Aggrdecl*_tmp66D=({struct Cyc_Absyn_Aggrdecl*_tmp671=_cycalloc(sizeof(*_tmp671));_tmp671->kind=_tmp67E,_tmp671->sc=Cyc_Absyn_Extern,_tmp671->name=_tmp67D,_tmp671->tvs=0,_tmp671->impl=0,_tmp671->attributes=0,_tmp671->expected_mem_kind=0;_tmp671;});
Cyc_Tc_tcAggrdecl(te,loc,_tmp66D);
adp=Cyc_Tcenv_lookup_aggrdecl(te,loc,_tmp67D);
({union Cyc_Absyn_AggrInfo _tmpB93=Cyc_Absyn_KnownAggr(adp);*info=_tmpB93;});
# 3887
if(*targs != 0){
({struct Cyc_String_pa_PrintArg_struct _tmp670=({struct Cyc_String_pa_PrintArg_struct _tmp9CE;_tmp9CE.tag=0U,({struct _dyneither_ptr _tmpB94=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_qvar2string(_tmp67D));_tmp9CE.f1=_tmpB94;});_tmp9CE;});void*_tmp66E[1U];_tmp66E[0]=& _tmp670;({unsigned int _tmpB96=loc;struct _dyneither_ptr _tmpB95=({const char*_tmp66F="declare parameterized type %s before using";_tag_dyneither(_tmp66F,sizeof(char),43U);});Cyc_Tcutil_terr(_tmpB96,_tmpB95,_tag_dyneither(_tmp66E,sizeof(void*),1U));});});
return cvtenv;}
# 3891
goto _LL5;}}else{_LL8: _tmp672=_tmp66C;_LL9:(int)_rethrow(_tmp672);}_LL5:;}};}
# 3893
_tmp67F=*adp;goto _LL4;}}else{_LL3: _tmp67F=*(_tmp65C.KnownAggr).val;_LL4: {
# 3895
struct Cyc_List_List*tvs=_tmp67F->tvs;
struct Cyc_List_List*ts=*targs;
for(0;ts != 0  && tvs != 0;(ts=ts->tl,tvs=tvs->tl)){
struct Cyc_Absyn_Tvar*_tmp673=(struct Cyc_Absyn_Tvar*)tvs->hd;
void*_tmp674=(void*)ts->hd;
# 3903
{struct _tuple0 _tmp675=({struct _tuple0 _tmp9CF;({void*_tmpB97=Cyc_Absyn_compress_kb(_tmp673->kind);_tmp9CF.f1=_tmpB97;}),_tmp9CF.f2=_tmp674;_tmp9CF;});struct _tuple0 _tmp676=_tmp675;struct Cyc_Absyn_Tvar*_tmp677;if(((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmp676.f1)->tag == 1U){if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp676.f2)->tag == 2U){_LLB: _tmp677=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp676.f2)->f1;_LLC:
# 3905
({struct Cyc_List_List*_tmpB98=Cyc_Tcutil_add_free_tvar(loc,cvtenv.kind_env,_tmp677);cvtenv.kind_env=_tmpB98;});
({struct Cyc_List_List*_tmpB99=Cyc_Tcutil_fast_add_free_tvar_bool(cvtenv.r,cvtenv.free_vars,_tmp677,1);cvtenv.free_vars=_tmpB99;});
continue;}else{goto _LLD;}}else{_LLD: _LLE:
 goto _LLA;}_LLA:;}{
# 3910
struct Cyc_Absyn_Kind*k=Cyc_Tcutil_tvar_kind((struct Cyc_Absyn_Tvar*)tvs->hd,& Cyc_Tcutil_bk);
cvtenv=Cyc_Tcutil_i_check_valid_type(loc,te,cvtenv,k,(void*)ts->hd,1,allow_abs_aggr);
Cyc_Tcutil_check_no_qual(loc,(void*)ts->hd);};}
# 3914
if(ts != 0)
({struct Cyc_String_pa_PrintArg_struct _tmp67A=({struct Cyc_String_pa_PrintArg_struct _tmp9D0;_tmp9D0.tag=0U,({struct _dyneither_ptr _tmpB9A=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_qvar2string(_tmp67F->name));_tmp9D0.f1=_tmpB9A;});_tmp9D0;});void*_tmp678[1U];_tmp678[0]=& _tmp67A;({unsigned int _tmpB9C=loc;struct _dyneither_ptr _tmpB9B=({const char*_tmp679="too many parameters for type %s";_tag_dyneither(_tmp679,sizeof(char),32U);});Cyc_Tcutil_terr(_tmpB9C,_tmpB9B,_tag_dyneither(_tmp678,sizeof(void*),1U));});});
if(tvs != 0){
# 3918
struct Cyc_List_List*hidden_ts=0;
for(0;tvs != 0;tvs=tvs->tl){
struct Cyc_Absyn_Kind*k=Cyc_Tcutil_tvar_inst_kind((struct Cyc_Absyn_Tvar*)tvs->hd,& Cyc_Tcutil_bk,expected_kind,0);
void*e=Cyc_Absyn_new_evar(0,0);
hidden_ts=({struct Cyc_List_List*_tmp67B=_cycalloc(sizeof(*_tmp67B));_tmp67B->hd=e,_tmp67B->tl=hidden_ts;_tmp67B;});
cvtenv=Cyc_Tcutil_i_check_valid_type(loc,te,cvtenv,k,e,1,allow_abs_aggr);}
# 3925
({struct Cyc_List_List*_tmpB9E=({struct Cyc_List_List*_tmpB9D=*targs;((struct Cyc_List_List*(*)(struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_imp_append)(_tmpB9D,((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(hidden_ts));});*targs=_tmpB9E;});}
# 3927
if((allow_abs_aggr  && _tmp67F->impl == 0) && !
Cyc_Tcutil_kind_leq(& Cyc_Tcutil_ak,expected_kind))
# 3932
_tmp67F->expected_mem_kind=1;}}_LL0:;}
# 3935
return cvtenv;}
# 3939
static struct Cyc_Tcutil_CVTEnv Cyc_Tcutil_i_check_valid_datatype(unsigned int loc,struct Cyc_Tcenv_Tenv*te,struct Cyc_Tcutil_CVTEnv cvtenv,struct Cyc_Absyn_Kind*expected_kind,union Cyc_Absyn_DatatypeInfo*info,struct Cyc_List_List**targsp,int allow_abs_aggr){
# 3945
struct Cyc_List_List*_tmp680=*targsp;
{union Cyc_Absyn_DatatypeInfo _tmp681=*info;union Cyc_Absyn_DatatypeInfo _tmp682=_tmp681;struct Cyc_Absyn_Datatypedecl*_tmp699;struct _tuple2*_tmp698;int _tmp697;if((_tmp682.UnknownDatatype).tag == 1){_LL1: _tmp698=((_tmp682.UnknownDatatype).val).name;_tmp697=((_tmp682.UnknownDatatype).val).is_extensible;_LL2: {
# 3948
struct Cyc_Absyn_Datatypedecl**tudp;
{struct _handler_cons _tmp683;_push_handler(& _tmp683);{int _tmp685=0;if(setjmp(_tmp683.handler))_tmp685=1;if(!_tmp685){tudp=Cyc_Tcenv_lookup_datatypedecl(te,loc,_tmp698);;_pop_handler();}else{void*_tmp684=(void*)_exn_thrown;void*_tmp686=_tmp684;void*_tmp68C;if(((struct Cyc_Dict_Absent_exn_struct*)_tmp686)->tag == Cyc_Dict_Absent){_LL6: _LL7: {
# 3952
struct Cyc_Absyn_Datatypedecl*_tmp687=({struct Cyc_Absyn_Datatypedecl*_tmp68B=_cycalloc(sizeof(*_tmp68B));_tmp68B->sc=Cyc_Absyn_Extern,_tmp68B->name=_tmp698,_tmp68B->tvs=0,_tmp68B->fields=0,_tmp68B->is_extensible=_tmp697;_tmp68B;});
Cyc_Tc_tcDatatypedecl(te,loc,_tmp687);
tudp=Cyc_Tcenv_lookup_datatypedecl(te,loc,_tmp698);
# 3956
if(_tmp680 != 0){
({struct Cyc_String_pa_PrintArg_struct _tmp68A=({struct Cyc_String_pa_PrintArg_struct _tmp9D1;_tmp9D1.tag=0U,({
struct _dyneither_ptr _tmpB9F=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_qvar2string(_tmp698));_tmp9D1.f1=_tmpB9F;});_tmp9D1;});void*_tmp688[1U];_tmp688[0]=& _tmp68A;({unsigned int _tmpBA1=loc;struct _dyneither_ptr _tmpBA0=({const char*_tmp689="declare parameterized datatype %s before using";_tag_dyneither(_tmp689,sizeof(char),47U);});Cyc_Tcutil_terr(_tmpBA1,_tmpBA0,_tag_dyneither(_tmp688,sizeof(void*),1U));});});
return cvtenv;}
# 3961
goto _LL5;}}else{_LL8: _tmp68C=_tmp686;_LL9:(int)_rethrow(_tmp68C);}_LL5:;}};}
# 3965
if(_tmp697  && !(*tudp)->is_extensible)
({struct Cyc_String_pa_PrintArg_struct _tmp68F=({struct Cyc_String_pa_PrintArg_struct _tmp9D2;_tmp9D2.tag=0U,({struct _dyneither_ptr _tmpBA2=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_qvar2string(_tmp698));_tmp9D2.f1=_tmpBA2;});_tmp9D2;});void*_tmp68D[1U];_tmp68D[0]=& _tmp68F;({unsigned int _tmpBA4=loc;struct _dyneither_ptr _tmpBA3=({const char*_tmp68E="datatype %s was not declared @extensible";_tag_dyneither(_tmp68E,sizeof(char),41U);});Cyc_Tcutil_terr(_tmpBA4,_tmpBA3,_tag_dyneither(_tmp68D,sizeof(void*),1U));});});
({union Cyc_Absyn_DatatypeInfo _tmpBA5=Cyc_Absyn_KnownDatatype(tudp);*info=_tmpBA5;});
_tmp699=*tudp;goto _LL4;}}else{_LL3: _tmp699=*(_tmp682.KnownDatatype).val;_LL4: {
# 3971
struct Cyc_List_List*tvs=_tmp699->tvs;
for(0;_tmp680 != 0  && tvs != 0;(_tmp680=_tmp680->tl,tvs=tvs->tl)){
void*t=(void*)_tmp680->hd;
struct Cyc_Absyn_Tvar*tv=(struct Cyc_Absyn_Tvar*)tvs->hd;
# 3977
{struct _tuple0 _tmp690=({struct _tuple0 _tmp9D3;({void*_tmpBA6=Cyc_Absyn_compress_kb(tv->kind);_tmp9D3.f1=_tmpBA6;}),_tmp9D3.f2=t;_tmp9D3;});struct _tuple0 _tmp691=_tmp690;struct Cyc_Absyn_Tvar*_tmp692;if(((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmp691.f1)->tag == 1U){if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp691.f2)->tag == 2U){_LLB: _tmp692=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp691.f2)->f1;_LLC:
# 3979
({struct Cyc_List_List*_tmpBA7=Cyc_Tcutil_add_free_tvar(loc,cvtenv.kind_env,_tmp692);cvtenv.kind_env=_tmpBA7;});
({struct Cyc_List_List*_tmpBA8=Cyc_Tcutil_fast_add_free_tvar_bool(cvtenv.r,cvtenv.free_vars,_tmp692,1);cvtenv.free_vars=_tmpBA8;});
continue;}else{goto _LLD;}}else{_LLD: _LLE:
 goto _LLA;}_LLA:;}{
# 3984
struct Cyc_Absyn_Kind*k=Cyc_Tcutil_tvar_kind(tv,& Cyc_Tcutil_bk);
cvtenv=Cyc_Tcutil_i_check_valid_type(loc,te,cvtenv,k,t,1,allow_abs_aggr);
Cyc_Tcutil_check_no_qual(loc,t);};}
# 3988
if(_tmp680 != 0)
({struct Cyc_String_pa_PrintArg_struct _tmp695=({struct Cyc_String_pa_PrintArg_struct _tmp9D4;_tmp9D4.tag=0U,({
struct _dyneither_ptr _tmpBA9=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_qvar2string(_tmp699->name));_tmp9D4.f1=_tmpBA9;});_tmp9D4;});void*_tmp693[1U];_tmp693[0]=& _tmp695;({unsigned int _tmpBAB=loc;struct _dyneither_ptr _tmpBAA=({const char*_tmp694="too many type arguments for datatype %s";_tag_dyneither(_tmp694,sizeof(char),40U);});Cyc_Tcutil_terr(_tmpBAB,_tmpBAA,_tag_dyneither(_tmp693,sizeof(void*),1U));});});
if(tvs != 0){
# 3993
struct Cyc_List_List*hidden_ts=0;
for(0;tvs != 0;tvs=tvs->tl){
struct Cyc_Absyn_Kind*k1=Cyc_Tcutil_tvar_inst_kind((struct Cyc_Absyn_Tvar*)tvs->hd,& Cyc_Tcutil_bk,expected_kind,0);
void*e=Cyc_Absyn_new_evar(0,0);
hidden_ts=({struct Cyc_List_List*_tmp696=_cycalloc(sizeof(*_tmp696));_tmp696->hd=e,_tmp696->tl=hidden_ts;_tmp696;});
cvtenv=Cyc_Tcutil_i_check_valid_type(loc,te,cvtenv,k1,e,1,allow_abs_aggr);}
# 4000
({struct Cyc_List_List*_tmpBAD=({struct Cyc_List_List*_tmpBAC=*targsp;((struct Cyc_List_List*(*)(struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_imp_append)(_tmpBAC,((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(hidden_ts));});*targsp=_tmpBAD;});}
# 4002
goto _LL0;}}_LL0:;}
# 4004
return cvtenv;}
# 4008
static struct Cyc_Tcutil_CVTEnv Cyc_Tcutil_i_check_valid_datatype_field(unsigned int loc,struct Cyc_Tcenv_Tenv*te,struct Cyc_Tcutil_CVTEnv cvtenv,struct Cyc_Absyn_Kind*expected_kind,union Cyc_Absyn_DatatypeFieldInfo*info,struct Cyc_List_List*targs,int allow_abs_aggr){
# 4014
{union Cyc_Absyn_DatatypeFieldInfo _tmp69A=*info;union Cyc_Absyn_DatatypeFieldInfo _tmp69B=_tmp69A;struct Cyc_Absyn_Datatypedecl*_tmp6AE;struct Cyc_Absyn_Datatypefield*_tmp6AD;struct _tuple2*_tmp6AC;struct _tuple2*_tmp6AB;int _tmp6AA;if((_tmp69B.UnknownDatatypefield).tag == 1){_LL1: _tmp6AC=((_tmp69B.UnknownDatatypefield).val).datatype_name;_tmp6AB=((_tmp69B.UnknownDatatypefield).val).field_name;_tmp6AA=((_tmp69B.UnknownDatatypefield).val).is_extensible;_LL2: {
# 4017
struct Cyc_Absyn_Datatypedecl*tud=*Cyc_Tcenv_lookup_datatypedecl(te,loc,_tmp6AC);
struct Cyc_Absyn_Datatypefield*tuf;
# 4022
{struct Cyc_List_List*_tmp69C=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(tud->fields))->v;for(0;1;_tmp69C=_tmp69C->tl){
if(_tmp69C == 0)({void*_tmp69D=0U;({struct _dyneither_ptr _tmpBAE=({const char*_tmp69E="Tcutil found a bad datatypefield";_tag_dyneither(_tmp69E,sizeof(char),33U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpBAE,_tag_dyneither(_tmp69D,sizeof(void*),0U));});});
if(Cyc_Absyn_qvar_cmp(((struct Cyc_Absyn_Datatypefield*)_tmp69C->hd)->name,_tmp6AB)== 0){
tuf=(struct Cyc_Absyn_Datatypefield*)_tmp69C->hd;
break;}}}
# 4029
({union Cyc_Absyn_DatatypeFieldInfo _tmpBAF=Cyc_Absyn_KnownDatatypefield(tud,tuf);*info=_tmpBAF;});
_tmp6AE=tud;_tmp6AD=tuf;goto _LL4;}}else{_LL3: _tmp6AE=((_tmp69B.KnownDatatypefield).val).f1;_tmp6AD=((_tmp69B.KnownDatatypefield).val).f2;_LL4: {
# 4033
struct Cyc_List_List*tvs=_tmp6AE->tvs;
for(0;targs != 0  && tvs != 0;(targs=targs->tl,tvs=tvs->tl)){
void*t=(void*)targs->hd;
struct Cyc_Absyn_Tvar*tv=(struct Cyc_Absyn_Tvar*)tvs->hd;
# 4039
{struct _tuple0 _tmp69F=({struct _tuple0 _tmp9D5;({void*_tmpBB0=Cyc_Absyn_compress_kb(tv->kind);_tmp9D5.f1=_tmpBB0;}),_tmp9D5.f2=t;_tmp9D5;});struct _tuple0 _tmp6A0=_tmp69F;struct Cyc_Absyn_Tvar*_tmp6A1;if(((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmp6A0.f1)->tag == 1U){if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp6A0.f2)->tag == 2U){_LL6: _tmp6A1=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp6A0.f2)->f1;_LL7:
# 4041
({struct Cyc_List_List*_tmpBB1=Cyc_Tcutil_add_free_tvar(loc,cvtenv.kind_env,_tmp6A1);cvtenv.kind_env=_tmpBB1;});
({struct Cyc_List_List*_tmpBB2=Cyc_Tcutil_fast_add_free_tvar_bool(cvtenv.r,cvtenv.free_vars,_tmp6A1,1);cvtenv.free_vars=_tmpBB2;});
continue;}else{goto _LL8;}}else{_LL8: _LL9:
 goto _LL5;}_LL5:;}{
# 4046
struct Cyc_Absyn_Kind*k=Cyc_Tcutil_tvar_kind(tv,& Cyc_Tcutil_bk);
cvtenv=Cyc_Tcutil_i_check_valid_type(loc,te,cvtenv,k,t,1,allow_abs_aggr);
Cyc_Tcutil_check_no_qual(loc,t);};}
# 4050
if(targs != 0)
({struct Cyc_String_pa_PrintArg_struct _tmp6A4=({struct Cyc_String_pa_PrintArg_struct _tmp9D7;_tmp9D7.tag=0U,({
struct _dyneither_ptr _tmpBB3=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_qvar2string(_tmp6AE->name));_tmp9D7.f1=_tmpBB3;});_tmp9D7;});struct Cyc_String_pa_PrintArg_struct _tmp6A5=({struct Cyc_String_pa_PrintArg_struct _tmp9D6;_tmp9D6.tag=0U,({struct _dyneither_ptr _tmpBB4=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_qvar2string(_tmp6AD->name));_tmp9D6.f1=_tmpBB4;});_tmp9D6;});void*_tmp6A2[2U];_tmp6A2[0]=& _tmp6A4,_tmp6A2[1]=& _tmp6A5;({unsigned int _tmpBB6=loc;struct _dyneither_ptr _tmpBB5=({const char*_tmp6A3="too many type arguments for datatype %s.%s";_tag_dyneither(_tmp6A3,sizeof(char),43U);});Cyc_Tcutil_terr(_tmpBB6,_tmpBB5,_tag_dyneither(_tmp6A2,sizeof(void*),2U));});});
if(tvs != 0)
({struct Cyc_String_pa_PrintArg_struct _tmp6A8=({struct Cyc_String_pa_PrintArg_struct _tmp9D9;_tmp9D9.tag=0U,({
struct _dyneither_ptr _tmpBB7=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_qvar2string(_tmp6AE->name));_tmp9D9.f1=_tmpBB7;});_tmp9D9;});struct Cyc_String_pa_PrintArg_struct _tmp6A9=({struct Cyc_String_pa_PrintArg_struct _tmp9D8;_tmp9D8.tag=0U,({struct _dyneither_ptr _tmpBB8=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_qvar2string(_tmp6AD->name));_tmp9D8.f1=_tmpBB8;});_tmp9D8;});void*_tmp6A6[2U];_tmp6A6[0]=& _tmp6A8,_tmp6A6[1]=& _tmp6A9;({unsigned int _tmpBBA=loc;struct _dyneither_ptr _tmpBB9=({const char*_tmp6A7="too few type arguments for datatype %s.%s";_tag_dyneither(_tmp6A7,sizeof(char),42U);});Cyc_Tcutil_terr(_tmpBBA,_tmpBB9,_tag_dyneither(_tmp6A6,sizeof(void*),2U));});});
goto _LL0;}}_LL0:;}
# 4058
return cvtenv;}
# 4061
static struct Cyc_Tcutil_CVTEnv Cyc_Tcutil_i_check_valid_type_app(unsigned int loc,struct Cyc_Tcenv_Tenv*te,struct Cyc_Tcutil_CVTEnv cvtenv,struct Cyc_Absyn_Kind*expected_kind,void*c,struct Cyc_List_List**targsp,int put_in_effect,int allow_abs_aggr){
# 4066
struct Cyc_List_List*_tmp6AF=*targsp;
{void*_tmp6B0=c;union Cyc_Absyn_DatatypeFieldInfo*_tmp6D7;union Cyc_Absyn_DatatypeInfo*_tmp6D6;union Cyc_Absyn_AggrInfo*_tmp6D5;struct Cyc_List_List*_tmp6D4;struct _tuple2*_tmp6D3;struct Cyc_Absyn_Enumdecl**_tmp6D2;switch(*((int*)_tmp6B0)){case 1U: _LL1: _LL2:
# 4069
 goto _LL4;case 2U: _LL3: _LL4: goto _LL6;case 0U: _LL5: _LL6: goto _LL8;case 7U: _LL7: _LL8:
 goto _LLA;case 6U: _LL9: _LLA: goto _LLC;case 5U: _LLB: _LLC: goto _LLE;case 12U: _LLD: _LLE:
 goto _LL10;case 11U: _LLF: _LL10: goto _LL12;case 14U: _LL11: _LL12: goto _LL14;case 17U: _LL13: _LL14:
# 4073
 if(_tmp6AF != 0)({struct Cyc_String_pa_PrintArg_struct _tmp6B3=({struct Cyc_String_pa_PrintArg_struct _tmp9DA;_tmp9DA.tag=0U,({
struct _dyneither_ptr _tmpBBB=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string((void*)({struct Cyc_Absyn_AppType_Absyn_Type_struct*_tmp6B4=_cycalloc(sizeof(*_tmp6B4));_tmp6B4->tag=0U,_tmp6B4->f1=c,_tmp6B4->f2=_tmp6AF;_tmp6B4;})));_tmp9DA.f1=_tmpBBB;});_tmp9DA;});void*_tmp6B1[1U];_tmp6B1[0]=& _tmp6B3;({struct _dyneither_ptr _tmpBBC=({const char*_tmp6B2="%s applied to argument(s)";_tag_dyneither(_tmp6B2,sizeof(char),26U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpBBC,_tag_dyneither(_tmp6B1,sizeof(void*),1U));});});
goto _LL0;case 9U: _LL15: _LL16:
# 4077
 for(0;_tmp6AF != 0;_tmp6AF=_tmp6AF->tl){
cvtenv=Cyc_Tcutil_i_check_valid_type(loc,te,cvtenv,& Cyc_Tcutil_ek,(void*)_tmp6AF->hd,1,1);}
goto _LL0;case 4U: _LL17: _LL18:
# 4081
 if(((int(*)(struct Cyc_List_List*x))Cyc_List_length)(_tmp6AF)!= 1)({void*_tmp6B5=0U;({struct _dyneither_ptr _tmpBBD=({const char*_tmp6B6="tag_t applied to wrong number of arguments";_tag_dyneither(_tmp6B6,sizeof(char),43U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpBBD,_tag_dyneither(_tmp6B5,sizeof(void*),0U));});});
cvtenv=Cyc_Tcutil_i_check_valid_type(loc,te,cvtenv,& Cyc_Tcutil_ik,(void*)((struct Cyc_List_List*)_check_null(_tmp6AF))->hd,0,1);goto _LL0;case 15U: _LL19: _tmp6D3=((struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*)_tmp6B0)->f1;_tmp6D2=(struct Cyc_Absyn_Enumdecl**)&((struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*)_tmp6B0)->f2;_LL1A:
# 4084
 if(_tmp6AF != 0)({void*_tmp6B7=0U;({struct _dyneither_ptr _tmpBBE=({const char*_tmp6B8="enum applied to argument(s)";_tag_dyneither(_tmp6B8,sizeof(char),28U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpBBE,_tag_dyneither(_tmp6B7,sizeof(void*),0U));});});
if(*_tmp6D2 == 0  || ((struct Cyc_Absyn_Enumdecl*)_check_null(*_tmp6D2))->fields == 0){
struct _handler_cons _tmp6B9;_push_handler(& _tmp6B9);{int _tmp6BB=0;if(setjmp(_tmp6B9.handler))_tmp6BB=1;if(!_tmp6BB){
{struct Cyc_Absyn_Enumdecl**ed=Cyc_Tcenv_lookup_enumdecl(te,loc,_tmp6D3);
*_tmp6D2=*ed;}
# 4087
;_pop_handler();}else{void*_tmp6BA=(void*)_exn_thrown;void*_tmp6BC=_tmp6BA;void*_tmp6BF;if(((struct Cyc_Dict_Absent_exn_struct*)_tmp6BC)->tag == Cyc_Dict_Absent){_LL2C: _LL2D: {
# 4091
struct Cyc_Absyn_Enumdecl*_tmp6BD=({struct Cyc_Absyn_Enumdecl*_tmp6BE=_cycalloc(sizeof(*_tmp6BE));_tmp6BE->sc=Cyc_Absyn_Extern,_tmp6BE->name=_tmp6D3,_tmp6BE->fields=0;_tmp6BE;});
Cyc_Tc_tcEnumdecl(te,loc,_tmp6BD);{
struct Cyc_Absyn_Enumdecl**ed=Cyc_Tcenv_lookup_enumdecl(te,loc,_tmp6D3);
*_tmp6D2=*ed;
goto _LL2B;};}}else{_LL2E: _tmp6BF=_tmp6BC;_LL2F:(int)_rethrow(_tmp6BF);}_LL2B:;}};}
# 4097
goto _LL0;case 16U: _LL1B: _tmp6D4=((struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct*)_tmp6B0)->f1;_LL1C:
# 4099
 if(_tmp6AF != 0)({void*_tmp6C0=0U;({struct _dyneither_ptr _tmpBBF=({const char*_tmp6C1="enum applied to argument(s)";_tag_dyneither(_tmp6C1,sizeof(char),28U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpBBF,_tag_dyneither(_tmp6C0,sizeof(void*),0U));});});{
# 4101
struct Cyc_List_List*prev_fields=0;
unsigned int tag_count=0U;
for(0;_tmp6D4 != 0;_tmp6D4=_tmp6D4->tl){
struct Cyc_Absyn_Enumfield*_tmp6C2=(struct Cyc_Absyn_Enumfield*)_tmp6D4->hd;
if(((int(*)(int(*compare)(struct _dyneither_ptr*,struct _dyneither_ptr*),struct Cyc_List_List*l,struct _dyneither_ptr*x))Cyc_List_mem)(Cyc_strptrcmp,prev_fields,(*_tmp6C2->name).f2))
({struct Cyc_String_pa_PrintArg_struct _tmp6C5=({struct Cyc_String_pa_PrintArg_struct _tmp9DB;_tmp9DB.tag=0U,_tmp9DB.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*(*_tmp6C2->name).f2);_tmp9DB;});void*_tmp6C3[1U];_tmp6C3[0]=& _tmp6C5;({unsigned int _tmpBC1=_tmp6C2->loc;struct _dyneither_ptr _tmpBC0=({const char*_tmp6C4="duplicate enum field name %s";_tag_dyneither(_tmp6C4,sizeof(char),29U);});Cyc_Tcutil_terr(_tmpBC1,_tmpBC0,_tag_dyneither(_tmp6C3,sizeof(void*),1U));});});else{
# 4108
prev_fields=({struct Cyc_List_List*_tmp6C6=_cycalloc(sizeof(*_tmp6C6));_tmp6C6->hd=(*_tmp6C2->name).f2,_tmp6C6->tl=prev_fields;_tmp6C6;});}
if(_tmp6C2->tag == 0)
({struct Cyc_Absyn_Exp*_tmpBC2=Cyc_Absyn_uint_exp(tag_count,_tmp6C2->loc);_tmp6C2->tag=_tmpBC2;});else{
if(!Cyc_Tcutil_is_const_exp((struct Cyc_Absyn_Exp*)_check_null(_tmp6C2->tag)))
({struct Cyc_String_pa_PrintArg_struct _tmp6C9=({struct Cyc_String_pa_PrintArg_struct _tmp9DC;_tmp9DC.tag=0U,_tmp9DC.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*(*_tmp6C2->name).f2);_tmp9DC;});void*_tmp6C7[1U];_tmp6C7[0]=& _tmp6C9;({unsigned int _tmpBC4=loc;struct _dyneither_ptr _tmpBC3=({const char*_tmp6C8="enum field %s: expression is not constant";_tag_dyneither(_tmp6C8,sizeof(char),42U);});Cyc_Tcutil_terr(_tmpBC4,_tmpBC3,_tag_dyneither(_tmp6C7,sizeof(void*),1U));});});}{
unsigned int t1=(Cyc_Evexp_eval_const_uint_exp((struct Cyc_Absyn_Exp*)_check_null(_tmp6C2->tag))).f1;
tag_count=t1 + (unsigned int)1;};}
# 4116
goto _LL0;};case 10U: _LL1D: _LL1E:
# 4118
 if(((int(*)(struct Cyc_List_List*x))Cyc_List_length)(_tmp6AF)!= 1)({void*_tmp6CA=0U;({struct _dyneither_ptr _tmpBC5=({const char*_tmp6CB="regions has wrong number of arguments";_tag_dyneither(_tmp6CB,sizeof(char),38U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpBC5,_tag_dyneither(_tmp6CA,sizeof(void*),0U));});});
cvtenv=Cyc_Tcutil_i_check_valid_type(loc,te,cvtenv,& Cyc_Tcutil_tak,(void*)((struct Cyc_List_List*)_check_null(_tmp6AF))->hd,1,1);goto _LL0;case 3U: _LL1F: _LL20:
# 4121
 if(((int(*)(struct Cyc_List_List*x))Cyc_List_length)(_tmp6AF)!= 1)({void*_tmp6CC=0U;({struct _dyneither_ptr _tmpBC6=({const char*_tmp6CD="region_t has wrong number of arguments";_tag_dyneither(_tmp6CD,sizeof(char),39U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpBC6,_tag_dyneither(_tmp6CC,sizeof(void*),0U));});});
cvtenv=Cyc_Tcutil_i_check_valid_type(loc,te,cvtenv,& Cyc_Tcutil_trk,(void*)((struct Cyc_List_List*)_check_null(_tmp6AF))->hd,1,1);
goto _LL0;case 13U: _LL21: _LL22:
# 4125
 if(((int(*)(struct Cyc_List_List*x))Cyc_List_length)(_tmp6AF)!= 1)({void*_tmp6CE=0U;({struct _dyneither_ptr _tmpBC7=({const char*_tmp6CF="@thin has wrong number of arguments";_tag_dyneither(_tmp6CF,sizeof(char),36U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpBC7,_tag_dyneither(_tmp6CE,sizeof(void*),0U));});});
cvtenv=Cyc_Tcutil_i_check_valid_type(loc,te,cvtenv,& Cyc_Tcutil_ik,(void*)((struct Cyc_List_List*)_check_null(_tmp6AF))->hd,0,1);
goto _LL0;case 8U: _LL23: _LL24:
# 4129
 if(((int(*)(struct Cyc_List_List*x))Cyc_List_length)(_tmp6AF)!= 1)({void*_tmp6D0=0U;({struct _dyneither_ptr _tmpBC8=({const char*_tmp6D1="access has wrong number of arguments";_tag_dyneither(_tmp6D1,sizeof(char),37U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpBC8,_tag_dyneither(_tmp6D0,sizeof(void*),0U));});});
cvtenv=Cyc_Tcutil_i_check_valid_type(loc,te,cvtenv,& Cyc_Tcutil_trk,(void*)((struct Cyc_List_List*)_check_null(_tmp6AF))->hd,1,1);goto _LL0;case 20U: _LL25: _tmp6D5=(union Cyc_Absyn_AggrInfo*)&((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_tmp6B0)->f1;_LL26:
# 4132
 cvtenv=Cyc_Tcutil_i_check_valid_aggr(loc,te,cvtenv,expected_kind,_tmp6D5,targsp,allow_abs_aggr);
# 4134
goto _LL0;case 18U: _LL27: _tmp6D6=(union Cyc_Absyn_DatatypeInfo*)&((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)_tmp6B0)->f1;_LL28:
# 4136
 cvtenv=Cyc_Tcutil_i_check_valid_datatype(loc,te,cvtenv,expected_kind,_tmp6D6,targsp,allow_abs_aggr);
# 4138
goto _LL0;default: _LL29: _tmp6D7=(union Cyc_Absyn_DatatypeFieldInfo*)&((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)_tmp6B0)->f1;_LL2A:
# 4140
 cvtenv=Cyc_Tcutil_i_check_valid_datatype_field(loc,te,cvtenv,expected_kind,_tmp6D7,_tmp6AF,allow_abs_aggr);
# 4142
goto _LL0;}_LL0:;}
# 4144
return cvtenv;}struct _tuple32{enum Cyc_Absyn_Format_Type f1;void*f2;};
# 4148
static struct Cyc_Tcutil_CVTEnv Cyc_Tcutil_i_check_valid_type(unsigned int loc,struct Cyc_Tcenv_Tenv*te,struct Cyc_Tcutil_CVTEnv cvtenv,struct Cyc_Absyn_Kind*expected_kind,void*t,int put_in_effect,int allow_abs_aggr){
# 4156
{void*_tmp6D8=Cyc_Tcutil_compress(t);void*_tmp6D9=_tmp6D8;struct _tuple2*_tmp7DD;struct Cyc_List_List**_tmp7DC;struct Cyc_Absyn_Typedefdecl**_tmp7DB;void**_tmp7DA;enum Cyc_Absyn_AggrKind _tmp7D9;struct Cyc_List_List*_tmp7D8;struct Cyc_List_List*_tmp7D7;struct Cyc_List_List**_tmp7D6;void**_tmp7D5;struct Cyc_Absyn_Tqual*_tmp7D4;void*_tmp7D3;struct Cyc_List_List*_tmp7D2;int _tmp7D1;struct Cyc_Absyn_VarargInfo*_tmp7D0;struct Cyc_List_List*_tmp7CF;struct Cyc_List_List*_tmp7CE;struct Cyc_Absyn_Exp*_tmp7CD;struct Cyc_List_List**_tmp7CC;struct Cyc_Absyn_Exp*_tmp7CB;struct Cyc_List_List**_tmp7CA;void*_tmp7C9;struct Cyc_Absyn_Tqual*_tmp7C8;struct Cyc_Absyn_Exp**_tmp7C7;void*_tmp7C6;unsigned int _tmp7C5;struct Cyc_Absyn_Exp*_tmp7C4;struct Cyc_Absyn_Exp*_tmp7C3;void*_tmp7C2;struct Cyc_Absyn_Tqual*_tmp7C1;void*_tmp7C0;void*_tmp7BF;void*_tmp7BE;void*_tmp7BD;void*_tmp7BC;void***_tmp7BB;struct Cyc_Absyn_Tvar*_tmp7BA;struct Cyc_Core_Opt**_tmp7B9;void**_tmp7B8;void*_tmp7B7;struct Cyc_List_List**_tmp7B6;switch(*((int*)_tmp6D9)){case 0U: _LL1: _tmp7B7=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp6D9)->f1;_tmp7B6=(struct Cyc_List_List**)&((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp6D9)->f2;_LL2:
# 4158
 cvtenv=Cyc_Tcutil_i_check_valid_type_app(loc,te,cvtenv,expected_kind,_tmp7B7,_tmp7B6,put_in_effect,allow_abs_aggr);
# 4160
goto _LL0;case 1U: _LL3: _tmp7B9=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp6D9)->f1;_tmp7B8=(void**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp6D9)->f2;_LL4:
# 4163
 if(*_tmp7B9 == 0  || 
Cyc_Tcutil_kind_leq(expected_kind,(struct Cyc_Absyn_Kind*)((struct Cyc_Core_Opt*)_check_null(*_tmp7B9))->v) && !Cyc_Tcutil_kind_leq((struct Cyc_Absyn_Kind*)((struct Cyc_Core_Opt*)_check_null(*_tmp7B9))->v,expected_kind))
({struct Cyc_Core_Opt*_tmpBC9=Cyc_Tcutil_kind_to_opt(expected_kind);*_tmp7B9=_tmpBC9;});
if(((cvtenv.fn_result  && cvtenv.generalize_evars) && (int)expected_kind->kind == (int)Cyc_Absyn_RgnKind) && !te->tempest_generalize){
# 4168
if((int)expected_kind->aliasqual == (int)Cyc_Absyn_Unique)
*_tmp7B8=Cyc_Absyn_unique_rgn_type;else{
# 4171
*_tmp7B8=Cyc_Absyn_heap_rgn_type;}}else{
if((cvtenv.generalize_evars  && (int)expected_kind->kind != (int)Cyc_Absyn_BoolKind) && (int)expected_kind->kind != (int)Cyc_Absyn_PtrBndKind){
# 4175
struct Cyc_Absyn_Tvar*_tmp6DA=Cyc_Tcutil_new_tvar((void*)({struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_tmp6DB=_cycalloc(sizeof(*_tmp6DB));_tmp6DB->tag=2U,_tmp6DB->f1=0,_tmp6DB->f2=expected_kind;_tmp6DB;}));
({void*_tmpBCA=Cyc_Absyn_var_type(_tmp6DA);*_tmp7B8=_tmpBCA;});
_tmp7BA=_tmp6DA;goto _LL6;}else{
# 4179
({struct Cyc_List_List*_tmpBCB=Cyc_Tcutil_add_free_evar(cvtenv.r,cvtenv.free_evars,t,put_in_effect);cvtenv.free_evars=_tmpBCB;});}}
# 4181
goto _LL0;case 2U: _LL5: _tmp7BA=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp6D9)->f1;_LL6:
# 4183
{void*_tmp6DC=Cyc_Absyn_compress_kb(_tmp7BA->kind);void*_tmp6DD=_tmp6DC;struct Cyc_Core_Opt**_tmp6E0;if(((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmp6DD)->tag == 1U){_LL1A: _tmp6E0=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmp6DD)->f1;_LL1B:
# 4185
({struct Cyc_Core_Opt*_tmpBCD=({struct Cyc_Core_Opt*_tmp6DF=_cycalloc(sizeof(*_tmp6DF));({void*_tmpBCC=(void*)({struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_tmp6DE=_cycalloc(sizeof(*_tmp6DE));_tmp6DE->tag=2U,_tmp6DE->f1=0,_tmp6DE->f2=expected_kind;_tmp6DE;});_tmp6DF->v=_tmpBCC;});_tmp6DF;});*_tmp6E0=_tmpBCD;});goto _LL19;}else{_LL1C: _LL1D:
 goto _LL19;}_LL19:;}
# 4190
({struct Cyc_List_List*_tmpBCE=Cyc_Tcutil_add_free_tvar(loc,cvtenv.kind_env,_tmp7BA);cvtenv.kind_env=_tmpBCE;});
# 4193
({struct Cyc_List_List*_tmpBCF=Cyc_Tcutil_fast_add_free_tvar_bool(cvtenv.r,cvtenv.free_vars,_tmp7BA,put_in_effect);cvtenv.free_vars=_tmpBCF;});
# 4195
{void*_tmp6E1=Cyc_Absyn_compress_kb(_tmp7BA->kind);void*_tmp6E2=_tmp6E1;struct Cyc_Core_Opt**_tmp6E6;struct Cyc_Absyn_Kind*_tmp6E5;if(((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp6E2)->tag == 2U){_LL1F: _tmp6E6=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp6E2)->f1;_tmp6E5=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp6E2)->f2;_LL20:
# 4197
 if(Cyc_Tcutil_kind_leq(expected_kind,_tmp6E5))
({struct Cyc_Core_Opt*_tmpBD1=({struct Cyc_Core_Opt*_tmp6E4=_cycalloc(sizeof(*_tmp6E4));({void*_tmpBD0=(void*)({struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_tmp6E3=_cycalloc(sizeof(*_tmp6E3));_tmp6E3->tag=2U,_tmp6E3->f1=0,_tmp6E3->f2=expected_kind;_tmp6E3;});_tmp6E4->v=_tmpBD0;});_tmp6E4;});*_tmp6E6=_tmpBD1;});
goto _LL1E;}else{_LL21: _LL22:
 goto _LL1E;}_LL1E:;}
# 4202
goto _LL0;case 10U: _LL7: _tmp7BC=(((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp6D9)->f1)->r;_tmp7BB=(void***)&((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp6D9)->f2;_LL8: {
# 4208
void*new_t=Cyc_Tcutil_copy_type(Cyc_Tcutil_compress(t));
{void*_tmp6E7=_tmp7BC;struct Cyc_Absyn_Datatypedecl*_tmp6EA;struct Cyc_Absyn_Enumdecl*_tmp6E9;struct Cyc_Absyn_Aggrdecl*_tmp6E8;switch(*((int*)_tmp6E7)){case 0U: _LL24: _tmp6E8=((struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct*)_tmp6E7)->f1;_LL25:
# 4211
 if(te->in_extern_c_include)
_tmp6E8->sc=Cyc_Absyn_ExternC;
Cyc_Tc_tcAggrdecl(te,loc,_tmp6E8);goto _LL23;case 1U: _LL26: _tmp6E9=((struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct*)_tmp6E7)->f1;_LL27:
# 4215
 if(te->in_extern_c_include)
_tmp6E9->sc=Cyc_Absyn_ExternC;
Cyc_Tc_tcEnumdecl(te,loc,_tmp6E9);goto _LL23;default: _LL28: _tmp6EA=((struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct*)_tmp6E7)->f1;_LL29:
# 4219
 Cyc_Tc_tcDatatypedecl(te,loc,_tmp6EA);goto _LL23;}_LL23:;}
# 4221
({void**_tmpBD2=({void**_tmp6EB=_cycalloc(sizeof(*_tmp6EB));*_tmp6EB=new_t;_tmp6EB;});*_tmp7BB=_tmpBD2;});
return Cyc_Tcutil_i_check_valid_type(loc,te,cvtenv,expected_kind,new_t,put_in_effect,allow_abs_aggr);}case 3U: _LL9: _tmp7C2=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp6D9)->f1).elt_type;_tmp7C1=(struct Cyc_Absyn_Tqual*)&(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp6D9)->f1).elt_tq;_tmp7C0=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp6D9)->f1).ptr_atts).rgn;_tmp7BF=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp6D9)->f1).ptr_atts).nullable;_tmp7BE=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp6D9)->f1).ptr_atts).bounds;_tmp7BD=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp6D9)->f1).ptr_atts).zero_term;_LLA: {
# 4227
int is_zero_terminated;
# 4229
cvtenv=Cyc_Tcutil_i_check_valid_type(loc,te,cvtenv,& Cyc_Tcutil_tak,_tmp7C2,1,1);
({int _tmpBD3=Cyc_Tcutil_extract_const_from_typedef(loc,_tmp7C1->print_const,_tmp7C2);_tmp7C1->real_const=_tmpBD3;});{
struct Cyc_Absyn_Kind*k;
{enum Cyc_Absyn_AliasQual _tmp6EC=expected_kind->aliasqual;enum Cyc_Absyn_AliasQual _tmp6ED=_tmp6EC;switch(_tmp6ED){case Cyc_Absyn_Aliasable: _LL2B: _LL2C:
 k=& Cyc_Tcutil_rk;goto _LL2A;case Cyc_Absyn_Unique: _LL2D: _LL2E:
 k=& Cyc_Tcutil_urk;goto _LL2A;default: _LL2F: _LL30:
 k=& Cyc_Tcutil_trk;goto _LL2A;}_LL2A:;}
# 4237
cvtenv=Cyc_Tcutil_i_check_valid_type(loc,te,cvtenv,k,_tmp7C0,1,1);
# 4240
cvtenv=Cyc_Tcutil_i_check_valid_type(loc,te,cvtenv,& Cyc_Tcutil_boolk,_tmp7BD,0,1);
cvtenv=Cyc_Tcutil_i_check_valid_type(loc,te,cvtenv,& Cyc_Tcutil_boolk,_tmp7BF,0,1);
({void*_tmpBD4=_tmp7BD;Cyc_Tcutil_unify(_tmpBD4,Cyc_Tcutil_is_char_type(_tmp7C2)?Cyc_Absyn_true_type: Cyc_Absyn_false_type);});
is_zero_terminated=Cyc_Absyn_type2bool(0,_tmp7BD);
if(is_zero_terminated){
# 4246
if(!Cyc_Tcutil_admits_zero(_tmp7C2))
({struct Cyc_String_pa_PrintArg_struct _tmp6F0=({struct Cyc_String_pa_PrintArg_struct _tmp9DD;_tmp9DD.tag=0U,({
struct _dyneither_ptr _tmpBD5=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(_tmp7C2));_tmp9DD.f1=_tmpBD5;});_tmp9DD;});void*_tmp6EE[1U];_tmp6EE[0]=& _tmp6F0;({unsigned int _tmpBD7=loc;struct _dyneither_ptr _tmpBD6=({const char*_tmp6EF="cannot have a pointer to zero-terminated %s elements";_tag_dyneither(_tmp6EF,sizeof(char),53U);});Cyc_Tcutil_terr(_tmpBD7,_tmpBD6,_tag_dyneither(_tmp6EE,sizeof(void*),1U));});});}
# 4251
cvtenv=Cyc_Tcutil_i_check_valid_type(loc,te,cvtenv,& Cyc_Tcutil_ptrbk,_tmp7BE,0,allow_abs_aggr);{
struct Cyc_Absyn_Exp*_tmp6F1=({void*_tmpBD8=Cyc_Absyn_bounds_one();Cyc_Tcutil_get_bounds_exp(_tmpBD8,_tmp7BE);});
if(_tmp6F1 != 0){
struct _tuple13 _tmp6F2=Cyc_Evexp_eval_const_uint_exp(_tmp6F1);struct _tuple13 _tmp6F3=_tmp6F2;unsigned int _tmp6F7;int _tmp6F6;_LL32: _tmp6F7=_tmp6F3.f1;_tmp6F6=_tmp6F3.f2;_LL33:;
if(is_zero_terminated  && (!_tmp6F6  || _tmp6F7 < (unsigned int)1))
({void*_tmp6F4=0U;({unsigned int _tmpBDA=loc;struct _dyneither_ptr _tmpBD9=({const char*_tmp6F5="zero-terminated pointer cannot point to empty sequence";_tag_dyneither(_tmp6F5,sizeof(char),55U);});Cyc_Tcutil_terr(_tmpBDA,_tmpBD9,_tag_dyneither(_tmp6F4,sizeof(void*),0U));});});}
# 4258
goto _LL0;};};}case 9U: _LLB: _tmp7C3=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp6D9)->f1;_LLC:
# 4263
({struct Cyc_Tcenv_Tenv*_tmpBDB=Cyc_Tcenv_allow_valueof(te);Cyc_Tcexp_tcExp(_tmpBDB,0,_tmp7C3);});
if(!Cyc_Tcutil_coerce_uint_type(te,_tmp7C3))
({void*_tmp6F8=0U;({unsigned int _tmpBDD=loc;struct _dyneither_ptr _tmpBDC=({const char*_tmp6F9="valueof_t requires an int expression";_tag_dyneither(_tmp6F9,sizeof(char),37U);});Cyc_Tcutil_terr(_tmpBDD,_tmpBDC,_tag_dyneither(_tmp6F8,sizeof(void*),0U));});});
cvtenv=Cyc_Tcutil_i_check_valid_type_level_exp(_tmp7C3,te,cvtenv);
goto _LL0;case 11U: _LLD: _tmp7C4=((struct Cyc_Absyn_TypeofType_Absyn_Type_struct*)_tmp6D9)->f1;_LLE:
# 4272
({struct Cyc_Tcenv_Tenv*_tmpBDE=Cyc_Tcenv_allow_valueof(te);Cyc_Tcexp_tcExp(_tmpBDE,0,_tmp7C4);});
goto _LL0;case 4U: _LLF: _tmp7C9=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp6D9)->f1).elt_type;_tmp7C8=(struct Cyc_Absyn_Tqual*)&(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp6D9)->f1).tq;_tmp7C7=(struct Cyc_Absyn_Exp**)&(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp6D9)->f1).num_elts;_tmp7C6=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp6D9)->f1).zero_term;_tmp7C5=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp6D9)->f1).zt_loc;_LL10: {
# 4277
struct Cyc_Absyn_Exp*_tmp6FA=*_tmp7C7;
cvtenv=Cyc_Tcutil_i_check_valid_type(loc,te,cvtenv,& Cyc_Tcutil_tmk,_tmp7C9,1,allow_abs_aggr);
({int _tmpBDF=Cyc_Tcutil_extract_const_from_typedef(loc,_tmp7C8->print_const,_tmp7C9);_tmp7C8->real_const=_tmpBDF;});{
# 4281
int is_zero_terminated=Cyc_Tcutil_force_type2bool(0,_tmp7C6);
if(is_zero_terminated){
# 4284
if(!Cyc_Tcutil_admits_zero(_tmp7C9))
({struct Cyc_String_pa_PrintArg_struct _tmp6FD=({struct Cyc_String_pa_PrintArg_struct _tmp9DE;_tmp9DE.tag=0U,({
struct _dyneither_ptr _tmpBE0=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(_tmp7C9));_tmp9DE.f1=_tmpBE0;});_tmp9DE;});void*_tmp6FB[1U];_tmp6FB[0]=& _tmp6FD;({unsigned int _tmpBE2=loc;struct _dyneither_ptr _tmpBE1=({const char*_tmp6FC="cannot have a zero-terminated array of %s elements";_tag_dyneither(_tmp6FC,sizeof(char),51U);});Cyc_Tcutil_terr(_tmpBE2,_tmpBE1,_tag_dyneither(_tmp6FB,sizeof(void*),1U));});});}
# 4290
if(_tmp6FA == 0){
# 4292
if(is_zero_terminated)
# 4294
({struct Cyc_Absyn_Exp*_tmpBE3=_tmp6FA=Cyc_Absyn_uint_exp(1U,0U);*_tmp7C7=_tmpBE3;});else{
# 4297
({void*_tmp6FE=0U;({unsigned int _tmpBE5=loc;struct _dyneither_ptr _tmpBE4=({const char*_tmp6FF="array bound defaults to 1 here";_tag_dyneither(_tmp6FF,sizeof(char),31U);});Cyc_Tcutil_warn(_tmpBE5,_tmpBE4,_tag_dyneither(_tmp6FE,sizeof(void*),0U));});});
({struct Cyc_Absyn_Exp*_tmpBE6=_tmp6FA=Cyc_Absyn_uint_exp(1U,0U);*_tmp7C7=_tmpBE6;});}}
# 4301
({struct Cyc_Tcenv_Tenv*_tmpBE7=Cyc_Tcenv_allow_valueof(te);Cyc_Tcexp_tcExp(_tmpBE7,0,_tmp6FA);});
if(!Cyc_Tcutil_coerce_uint_type(te,_tmp6FA))
({void*_tmp700=0U;({unsigned int _tmpBE9=loc;struct _dyneither_ptr _tmpBE8=({const char*_tmp701="array bounds expression is not an unsigned int";_tag_dyneither(_tmp701,sizeof(char),47U);});Cyc_Tcutil_terr(_tmpBE9,_tmpBE8,_tag_dyneither(_tmp700,sizeof(void*),0U));});});
cvtenv=Cyc_Tcutil_i_check_valid_type_level_exp(_tmp6FA,te,cvtenv);{
# 4309
struct _tuple13 _tmp702=Cyc_Evexp_eval_const_uint_exp(_tmp6FA);struct _tuple13 _tmp703=_tmp702;unsigned int _tmp709;int _tmp708;_LL35: _tmp709=_tmp703.f1;_tmp708=_tmp703.f2;_LL36:;
# 4311
if((is_zero_terminated  && _tmp708) && _tmp709 < (unsigned int)1)
({void*_tmp704=0U;({unsigned int _tmpBEB=loc;struct _dyneither_ptr _tmpBEA=({const char*_tmp705="zero terminated array cannot have zero elements";_tag_dyneither(_tmp705,sizeof(char),48U);});Cyc_Tcutil_warn(_tmpBEB,_tmpBEA,_tag_dyneither(_tmp704,sizeof(void*),0U));});});
# 4314
if((_tmp708  && _tmp709 < (unsigned int)1) && Cyc_Cyclone_tovc_r){
({void*_tmp706=0U;({unsigned int _tmpBED=loc;struct _dyneither_ptr _tmpBEC=({const char*_tmp707="arrays with 0 elements are not supported except with gcc -- changing to 1.";_tag_dyneither(_tmp707,sizeof(char),75U);});Cyc_Tcutil_warn(_tmpBED,_tmpBEC,_tag_dyneither(_tmp706,sizeof(void*),0U));});});
({struct Cyc_Absyn_Exp*_tmpBEE=Cyc_Absyn_uint_exp(1U,0U);*_tmp7C7=_tmpBEE;});}
# 4318
goto _LL0;};};}case 5U: _LL11: _tmp7D6=(struct Cyc_List_List**)&(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp6D9)->f1).tvars;_tmp7D5=(void**)&(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp6D9)->f1).effect;_tmp7D4=(struct Cyc_Absyn_Tqual*)&(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp6D9)->f1).ret_tqual;_tmp7D3=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp6D9)->f1).ret_type;_tmp7D2=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp6D9)->f1).args;_tmp7D1=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp6D9)->f1).c_varargs;_tmp7D0=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp6D9)->f1).cyc_varargs;_tmp7CF=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp6D9)->f1).rgn_po;_tmp7CE=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp6D9)->f1).attributes;_tmp7CD=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp6D9)->f1).requires_clause;_tmp7CC=(struct Cyc_List_List**)&(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp6D9)->f1).requires_relns;_tmp7CB=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp6D9)->f1).ensures_clause;_tmp7CA=(struct Cyc_List_List**)&(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp6D9)->f1).ensures_relns;_LL12: {
# 4325
int num_convs=0;
int seen_cdecl=0;
int seen_stdcall=0;
int seen_fastcall=0;
int seen_format=0;
enum Cyc_Absyn_Format_Type ft=0U;
int fmt_desc_arg=-1;
int fmt_arg_start=-1;
for(0;_tmp7CE != 0;_tmp7CE=_tmp7CE->tl){
if(!Cyc_Absyn_fntype_att((void*)_tmp7CE->hd))
({struct Cyc_String_pa_PrintArg_struct _tmp70C=({struct Cyc_String_pa_PrintArg_struct _tmp9DF;_tmp9DF.tag=0U,({struct _dyneither_ptr _tmpBEF=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absyn_attribute2string((void*)_tmp7CE->hd));_tmp9DF.f1=_tmpBEF;});_tmp9DF;});void*_tmp70A[1U];_tmp70A[0]=& _tmp70C;({unsigned int _tmpBF1=loc;struct _dyneither_ptr _tmpBF0=({const char*_tmp70B="bad function type attribute %s";_tag_dyneither(_tmp70B,sizeof(char),31U);});Cyc_Tcutil_terr(_tmpBF1,_tmpBF0,_tag_dyneither(_tmp70A,sizeof(void*),1U));});});{
void*_tmp70D=(void*)_tmp7CE->hd;void*_tmp70E=_tmp70D;enum Cyc_Absyn_Format_Type _tmp713;int _tmp712;int _tmp711;switch(*((int*)_tmp70E)){case 1U: _LL38: _LL39:
# 4338
 if(!seen_stdcall){seen_stdcall=1;++ num_convs;}goto _LL37;case 2U: _LL3A: _LL3B:
# 4340
 if(!seen_cdecl){seen_cdecl=1;++ num_convs;}goto _LL37;case 3U: _LL3C: _LL3D:
# 4342
 if(!seen_fastcall){seen_fastcall=1;++ num_convs;}goto _LL37;case 19U: _LL3E: _tmp713=((struct Cyc_Absyn_Format_att_Absyn_Attribute_struct*)_tmp70E)->f1;_tmp712=((struct Cyc_Absyn_Format_att_Absyn_Attribute_struct*)_tmp70E)->f2;_tmp711=((struct Cyc_Absyn_Format_att_Absyn_Attribute_struct*)_tmp70E)->f3;_LL3F:
# 4344
 if(!seen_format){
seen_format=1;
ft=_tmp713;
fmt_desc_arg=_tmp712;
fmt_arg_start=_tmp711;}else{
# 4350
({void*_tmp70F=0U;({unsigned int _tmpBF3=loc;struct _dyneither_ptr _tmpBF2=({const char*_tmp710="function can't have multiple format attributes";_tag_dyneither(_tmp710,sizeof(char),47U);});Cyc_Tcutil_terr(_tmpBF3,_tmpBF2,_tag_dyneither(_tmp70F,sizeof(void*),0U));});});}
goto _LL37;default: _LL40: _LL41:
 goto _LL37;}_LL37:;};}
# 4355
if(num_convs > 1)
({void*_tmp714=0U;({unsigned int _tmpBF5=loc;struct _dyneither_ptr _tmpBF4=({const char*_tmp715="function can't have multiple calling conventions";_tag_dyneither(_tmp715,sizeof(char),49U);});Cyc_Tcutil_terr(_tmpBF5,_tmpBF4,_tag_dyneither(_tmp714,sizeof(void*),0U));});});
# 4360
Cyc_Tcutil_check_unique_tvars(loc,*_tmp7D6);
{struct Cyc_List_List*b=*_tmp7D6;for(0;b != 0;b=b->tl){
({int _tmpBF6=Cyc_Tcutil_new_tvar_id();((struct Cyc_Absyn_Tvar*)b->hd)->identity=_tmpBF6;});
({struct Cyc_List_List*_tmpBF7=Cyc_Tcutil_add_bound_tvar(cvtenv.kind_env,(struct Cyc_Absyn_Tvar*)b->hd);cvtenv.kind_env=_tmpBF7;});{
void*_tmp716=Cyc_Absyn_compress_kb(((struct Cyc_Absyn_Tvar*)b->hd)->kind);void*_tmp717=_tmp716;if(((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp717)->tag == 0U){if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp717)->f1)->kind == Cyc_Absyn_MemKind){_LL43: _LL44:
# 4366
({struct Cyc_String_pa_PrintArg_struct _tmp71A=({struct Cyc_String_pa_PrintArg_struct _tmp9E0;_tmp9E0.tag=0U,_tmp9E0.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*((struct Cyc_Absyn_Tvar*)b->hd)->name);_tmp9E0;});void*_tmp718[1U];_tmp718[0]=& _tmp71A;({unsigned int _tmpBF9=loc;struct _dyneither_ptr _tmpBF8=({const char*_tmp719="function attempts to abstract Mem type variable %s";_tag_dyneither(_tmp719,sizeof(char),51U);});Cyc_Tcutil_terr(_tmpBF9,_tmpBF8,_tag_dyneither(_tmp718,sizeof(void*),1U));});});
goto _LL42;}else{goto _LL45;}}else{_LL45: _LL46:
 goto _LL42;}_LL42:;};}}{
# 4374
struct Cyc_Tcutil_CVTEnv _tmp71B=({struct Cyc_Tcutil_CVTEnv _tmp9E3;_tmp9E3.r=Cyc_Core_heap_region,_tmp9E3.kind_env=cvtenv.kind_env,_tmp9E3.free_vars=0,_tmp9E3.free_evars=0,_tmp9E3.generalize_evars=cvtenv.generalize_evars,_tmp9E3.fn_result=1;_tmp9E3;});
# 4376
_tmp71B=Cyc_Tcutil_i_check_valid_type(loc,te,_tmp71B,& Cyc_Tcutil_tmk,_tmp7D3,1,1);
({int _tmpBFA=Cyc_Tcutil_extract_const_from_typedef(loc,_tmp7D4->print_const,_tmp7D3);_tmp7D4->real_const=_tmpBFA;});
_tmp71B.fn_result=0;
# 4382
{struct Cyc_List_List*a=_tmp7D2;for(0;a != 0;a=a->tl){
struct _tuple10*_tmp71C=(struct _tuple10*)a->hd;
void*_tmp71D=(*_tmp71C).f3;
_tmp71B=Cyc_Tcutil_i_check_valid_type(loc,te,_tmp71B,& Cyc_Tcutil_tmk,_tmp71D,1,1);{
int _tmp71E=Cyc_Tcutil_extract_const_from_typedef(loc,((*_tmp71C).f2).print_const,_tmp71D);
((*_tmp71C).f2).real_const=_tmp71E;
# 4390
if(Cyc_Tcutil_is_array_type(_tmp71D)){
# 4392
void*_tmp71F=Cyc_Absyn_new_evar(0,0);
_tmp71B=Cyc_Tcutil_i_check_valid_type(loc,te,_tmp71B,& Cyc_Tcutil_rk,_tmp71F,1,1);
({void*_tmpBFB=Cyc_Tcutil_promote_array(_tmp71D,_tmp71F,0);(*_tmp71C).f3=_tmpBFB;});}};}}
# 4399
if(_tmp7D0 != 0){
if(_tmp7D1)({void*_tmp720=0U;({struct _dyneither_ptr _tmpBFC=({const char*_tmp721="both c_vararg and cyc_vararg";_tag_dyneither(_tmp721,sizeof(char),29U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpBFC,_tag_dyneither(_tmp720,sizeof(void*),0U));});});{
struct Cyc_Absyn_VarargInfo _tmp722=*_tmp7D0;struct Cyc_Absyn_VarargInfo _tmp723=_tmp722;void*_tmp734;int _tmp733;_LL48: _tmp734=_tmp723.type;_tmp733=_tmp723.inject;_LL49:;
_tmp71B=Cyc_Tcutil_i_check_valid_type(loc,te,_tmp71B,& Cyc_Tcutil_tmk,_tmp734,1,1);
({int _tmpBFD=Cyc_Tcutil_extract_const_from_typedef(loc,(_tmp7D0->tq).print_const,_tmp734);(_tmp7D0->tq).real_const=_tmpBFD;});
# 4405
if(_tmp733){
void*_tmp724=Cyc_Tcutil_compress(_tmp734);void*_tmp725=_tmp724;void*_tmp732;void*_tmp731;void*_tmp730;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp725)->tag == 3U){_LL4B: _tmp732=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp725)->f1).elt_type;_tmp731=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp725)->f1).ptr_atts).bounds;_tmp730=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp725)->f1).ptr_atts).zero_term;_LL4C:
# 4408
{void*_tmp726=Cyc_Tcutil_compress(_tmp732);void*_tmp727=_tmp726;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp727)->tag == 0U){if(((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp727)->f1)->tag == 18U){_LL50: _LL51:
# 4410
 if(Cyc_Tcutil_force_type2bool(0,_tmp730))
({void*_tmp728=0U;({unsigned int _tmpBFF=loc;struct _dyneither_ptr _tmpBFE=({const char*_tmp729="can't inject into a zeroterm pointer";_tag_dyneither(_tmp729,sizeof(char),37U);});Cyc_Tcutil_terr(_tmpBFF,_tmpBFE,_tag_dyneither(_tmp728,sizeof(void*),0U));});});
if(!({void*_tmpC00=Cyc_Absyn_bounds_one();Cyc_Tcutil_unify(_tmpC00,_tmp731);}))
({void*_tmp72A=0U;({unsigned int _tmpC02=loc;struct _dyneither_ptr _tmpC01=({const char*_tmp72B="can't inject into a fat pointer to datatype";_tag_dyneither(_tmp72B,sizeof(char),44U);});Cyc_Tcutil_terr(_tmpC02,_tmpC01,_tag_dyneither(_tmp72A,sizeof(void*),0U));});});
goto _LL4F;}else{goto _LL52;}}else{_LL52: _LL53:
({void*_tmp72C=0U;({unsigned int _tmpC04=loc;struct _dyneither_ptr _tmpC03=({const char*_tmp72D="can't inject a non-datatype type";_tag_dyneither(_tmp72D,sizeof(char),33U);});Cyc_Tcutil_terr(_tmpC04,_tmpC03,_tag_dyneither(_tmp72C,sizeof(void*),0U));});});goto _LL4F;}_LL4F:;}
# 4417
goto _LL4A;}else{_LL4D: _LL4E:
({void*_tmp72E=0U;({unsigned int _tmpC06=loc;struct _dyneither_ptr _tmpC05=({const char*_tmp72F="expecting a datatype pointer type";_tag_dyneither(_tmp72F,sizeof(char),34U);});Cyc_Tcutil_terr(_tmpC06,_tmpC05,_tag_dyneither(_tmp72E,sizeof(void*),0U));});});goto _LL4A;}_LL4A:;}};}
# 4423
if(seen_format){
int _tmp735=((int(*)(struct Cyc_List_List*x))Cyc_List_length)(_tmp7D2);
if((((fmt_desc_arg < 0  || fmt_desc_arg > _tmp735) || fmt_arg_start < 0) || 
# 4427
(_tmp7D0 == 0  && !_tmp7D1) && fmt_arg_start != 0) || 
(_tmp7D0 != 0  || _tmp7D1) && fmt_arg_start != _tmp735 + 1)
# 4430
({void*_tmp736=0U;({unsigned int _tmpC08=loc;struct _dyneither_ptr _tmpC07=({const char*_tmp737="bad format descriptor";_tag_dyneither(_tmp737,sizeof(char),22U);});Cyc_Tcutil_terr(_tmpC08,_tmpC07,_tag_dyneither(_tmp736,sizeof(void*),0U));});});else{
# 4433
struct _tuple10 _tmp738=*((struct _tuple10*(*)(struct Cyc_List_List*x,int n))Cyc_List_nth)(_tmp7D2,fmt_desc_arg - 1);struct _tuple10 _tmp739=_tmp738;void*_tmp74E;_LL55: _tmp74E=_tmp739.f3;_LL56:;
# 4435
{void*_tmp73A=Cyc_Tcutil_compress(_tmp74E);void*_tmp73B=_tmp73A;void*_tmp747;void*_tmp746;void*_tmp745;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp73B)->tag == 3U){_LL58: _tmp747=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp73B)->f1).elt_type;_tmp746=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp73B)->f1).ptr_atts).bounds;_tmp745=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp73B)->f1).ptr_atts).zero_term;_LL59:
# 4438
 if(!Cyc_Tcutil_is_char_type(_tmp747))
({void*_tmp73C=0U;({unsigned int _tmpC0A=loc;struct _dyneither_ptr _tmpC09=({const char*_tmp73D="format descriptor is not a string";_tag_dyneither(_tmp73D,sizeof(char),34U);});Cyc_Tcutil_terr(_tmpC0A,_tmpC09,_tag_dyneither(_tmp73C,sizeof(void*),0U));});});else{
# 4441
struct Cyc_Absyn_Exp*_tmp73E=({void*_tmpC0B=Cyc_Absyn_bounds_one();Cyc_Tcutil_get_bounds_exp(_tmpC0B,_tmp746);});
if(_tmp73E == 0  && _tmp7D1)
({void*_tmp73F=0U;({unsigned int _tmpC0D=loc;struct _dyneither_ptr _tmpC0C=({const char*_tmp740="format descriptor is not a char * type";_tag_dyneither(_tmp740,sizeof(char),39U);});Cyc_Tcutil_terr(_tmpC0D,_tmpC0C,_tag_dyneither(_tmp73F,sizeof(void*),0U));});});else{
if(_tmp73E != 0  && !_tmp7D1)
({void*_tmp741=0U;({unsigned int _tmpC0F=loc;struct _dyneither_ptr _tmpC0E=({const char*_tmp742="format descriptor is not a char ? type";_tag_dyneither(_tmp742,sizeof(char),39U);});Cyc_Tcutil_terr(_tmpC0F,_tmpC0E,_tag_dyneither(_tmp741,sizeof(void*),0U));});});}}
# 4447
goto _LL57;}else{_LL5A: _LL5B:
({void*_tmp743=0U;({unsigned int _tmpC11=loc;struct _dyneither_ptr _tmpC10=({const char*_tmp744="format descriptor is not a string type";_tag_dyneither(_tmp744,sizeof(char),39U);});Cyc_Tcutil_terr(_tmpC11,_tmpC10,_tag_dyneither(_tmp743,sizeof(void*),0U));});});goto _LL57;}_LL57:;}
# 4450
if(fmt_arg_start != 0  && !_tmp7D1){
# 4454
int problem;
{struct _tuple32 _tmp748=({struct _tuple32 _tmp9E1;_tmp9E1.f1=ft,({void*_tmpC12=Cyc_Tcutil_compress(Cyc_Tcutil_pointer_elt_type(((struct Cyc_Absyn_VarargInfo*)_check_null(_tmp7D0))->type));_tmp9E1.f2=_tmpC12;});_tmp9E1;});struct _tuple32 _tmp749=_tmp748;struct Cyc_Absyn_Datatypedecl*_tmp74B;struct Cyc_Absyn_Datatypedecl*_tmp74A;if(_tmp749.f1 == Cyc_Absyn_Printf_ft){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp749.f2)->tag == 0U){if(((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp749.f2)->f1)->tag == 18U){if(((((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp749.f2)->f1)->f1).KnownDatatype).tag == 2){_LL5D: _tmp74A=*((((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp749.f2)->f1)->f1).KnownDatatype).val;_LL5E:
# 4457
 problem=Cyc_Absyn_qvar_cmp(_tmp74A->name,Cyc_Absyn_datatype_print_arg_qvar)!= 0;goto _LL5C;}else{goto _LL61;}}else{goto _LL61;}}else{goto _LL61;}}else{if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp749.f2)->tag == 0U){if(((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp749.f2)->f1)->tag == 18U){if(((((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp749.f2)->f1)->f1).KnownDatatype).tag == 2){_LL5F: _tmp74B=*((((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp749.f2)->f1)->f1).KnownDatatype).val;_LL60:
# 4459
 problem=Cyc_Absyn_qvar_cmp(_tmp74B->name,Cyc_Absyn_datatype_scanf_arg_qvar)!= 0;goto _LL5C;}else{goto _LL61;}}else{goto _LL61;}}else{_LL61: _LL62:
# 4461
 problem=1;goto _LL5C;}}_LL5C:;}
# 4463
if(problem)
({void*_tmp74C=0U;({unsigned int _tmpC14=loc;struct _dyneither_ptr _tmpC13=({const char*_tmp74D="format attribute and vararg types don't match";_tag_dyneither(_tmp74D,sizeof(char),46U);});Cyc_Tcutil_terr(_tmpC14,_tmpC13,_tag_dyneither(_tmp74C,sizeof(void*),0U));});});}}}
# 4471
{struct Cyc_List_List*rpo=_tmp7CF;for(0;rpo != 0;rpo=rpo->tl){
struct _tuple0*_tmp74F=(struct _tuple0*)rpo->hd;struct _tuple0*_tmp750=_tmp74F;void*_tmp752;void*_tmp751;_LL64: _tmp752=_tmp750->f1;_tmp751=_tmp750->f2;_LL65:;
_tmp71B=Cyc_Tcutil_i_check_valid_type(loc,te,_tmp71B,& Cyc_Tcutil_ek,_tmp752,1,1);
_tmp71B=Cyc_Tcutil_i_check_valid_type(loc,te,_tmp71B,& Cyc_Tcutil_trk,_tmp751,1,1);}}{
# 4479
struct Cyc_Tcenv_Fenv*_tmp753=Cyc_Tcenv_bogus_fenv(_tmp7D3,_tmp7D2);
struct Cyc_Tcenv_Tenv*_tmp754=({struct Cyc_Tcenv_Tenv*_tmp792=_cycalloc(sizeof(*_tmp792));_tmp792->ns=te->ns,_tmp792->ae=te->ae,_tmp792->le=_tmp753,_tmp792->allow_valueof=1,_tmp792->in_extern_c_include=te->in_extern_c_include,_tmp792->in_tempest=te->in_tempest,_tmp792->tempest_generalize=te->tempest_generalize;_tmp792;});
struct _tuple31 _tmp755=({unsigned int _tmpC18=loc;struct Cyc_Tcenv_Tenv*_tmpC17=_tmp754;struct Cyc_Tcutil_CVTEnv _tmpC16=_tmp71B;struct _dyneither_ptr _tmpC15=({const char*_tmp791="@requires";_tag_dyneither(_tmp791,sizeof(char),10U);});Cyc_Tcutil_check_clause(_tmpC18,_tmpC17,_tmpC16,_tmpC15,_tmp7CD);});struct _tuple31 _tmp756=_tmp755;struct Cyc_Tcutil_CVTEnv _tmp790;struct Cyc_List_List*_tmp78F;_LL67: _tmp790=_tmp756.f1;_tmp78F=_tmp756.f2;_LL68:;
_tmp71B=_tmp790;
*_tmp7CC=_tmp78F;
((void(*)(void(*f)(struct Cyc_List_List*,struct Cyc_Relations_Reln*),struct Cyc_List_List*env,struct Cyc_List_List*x))Cyc_List_iter_c)(Cyc_Tcutil_replace_rops,_tmp7D2,_tmp78F);{
# 4492
struct _tuple31 _tmp757=({unsigned int _tmpC1C=loc;struct Cyc_Tcenv_Tenv*_tmpC1B=_tmp754;struct Cyc_Tcutil_CVTEnv _tmpC1A=_tmp71B;struct _dyneither_ptr _tmpC19=({const char*_tmp78E="@ensures";_tag_dyneither(_tmp78E,sizeof(char),9U);});Cyc_Tcutil_check_clause(_tmpC1C,_tmpC1B,_tmpC1A,_tmpC19,_tmp7CB);});struct _tuple31 _tmp758=_tmp757;struct Cyc_Tcutil_CVTEnv _tmp78D;struct Cyc_List_List*_tmp78C;_LL6A: _tmp78D=_tmp758.f1;_tmp78C=_tmp758.f2;_LL6B:;
_tmp71B=_tmp78D;
*_tmp7CA=_tmp78C;
((void(*)(void(*f)(struct Cyc_List_List*,struct Cyc_Relations_Reln*),struct Cyc_List_List*env,struct Cyc_List_List*x))Cyc_List_iter_c)(Cyc_Tcutil_replace_rops,_tmp7D2,_tmp78C);
# 4497
if(*_tmp7D5 != 0)
_tmp71B=Cyc_Tcutil_i_check_valid_type(loc,te,_tmp71B,& Cyc_Tcutil_ek,(void*)_check_null(*_tmp7D5),1,1);else{
# 4500
struct Cyc_List_List*effect=0;
# 4505
{struct Cyc_List_List*tvs=_tmp71B.free_vars;for(0;tvs != 0;tvs=tvs->tl){
struct _tuple29 _tmp759=*((struct _tuple29*)tvs->hd);struct _tuple29 _tmp75A=_tmp759;struct Cyc_Absyn_Tvar*_tmp768;int _tmp767;_LL6D: _tmp768=_tmp75A.f1;_tmp767=_tmp75A.f2;_LL6E:;
if(!_tmp767)continue;{
void*_tmp75B=Cyc_Absyn_compress_kb(_tmp768->kind);void*_tmp75C=_tmp75B;struct Cyc_Core_Opt**_tmp766;struct Cyc_Absyn_Kind*_tmp765;struct Cyc_Core_Opt**_tmp764;struct Cyc_Core_Opt**_tmp763;struct Cyc_Absyn_Kind*_tmp762;switch(*((int*)_tmp75C)){case 2U: _LL70: _tmp763=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp75C)->f1;_tmp762=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp75C)->f2;if((int)_tmp762->kind == (int)Cyc_Absyn_RgnKind){_LL71:
# 4511
 if((int)_tmp762->aliasqual == (int)Cyc_Absyn_Top){
({struct Cyc_Core_Opt*_tmpC1D=Cyc_Tcutil_kind_to_bound_opt(& Cyc_Tcutil_rk);*_tmp763=_tmpC1D;});_tmp765=_tmp762;goto _LL73;}
# 4514
({struct Cyc_Core_Opt*_tmpC1E=Cyc_Tcutil_kind_to_bound_opt(_tmp762);*_tmp763=_tmpC1E;});_tmp765=_tmp762;goto _LL73;}else{switch(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp75C)->f2)->kind){case Cyc_Absyn_BoolKind: _LL74: _LL75:
# 4517
 goto _LL77;case Cyc_Absyn_PtrBndKind: _LL76: _LL77:
 goto _LL79;case Cyc_Absyn_IntKind: _LL78: _LL79:
 goto _LL7B;case Cyc_Absyn_EffKind: _LL80: _tmp764=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp75C)->f1;_LL81:
# 4524
({struct Cyc_Core_Opt*_tmpC1F=Cyc_Tcutil_kind_to_bound_opt(& Cyc_Tcutil_ek);*_tmp764=_tmpC1F;});goto _LL83;default: goto _LL86;}}case 0U: _LL72: _tmp765=((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp75C)->f1;if((int)_tmp765->kind == (int)Cyc_Absyn_RgnKind){_LL73:
# 4516
 effect=({struct Cyc_List_List*_tmp75D=_cycalloc(sizeof(*_tmp75D));({void*_tmpC20=Cyc_Absyn_access_eff(Cyc_Absyn_var_type(_tmp768));_tmp75D->hd=_tmpC20;}),_tmp75D->tl=effect;_tmp75D;});goto _LL6F;}else{switch(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp75C)->f1)->kind){case Cyc_Absyn_BoolKind: _LL7A: _LL7B:
# 4520
 goto _LL7D;case Cyc_Absyn_PtrBndKind: _LL7C: _LL7D:
 goto _LL7F;case Cyc_Absyn_IntKind: _LL7E: _LL7F:
 goto _LL6F;case Cyc_Absyn_EffKind: _LL82: _LL83:
# 4526
 effect=({struct Cyc_List_List*_tmp75E=_cycalloc(sizeof(*_tmp75E));({void*_tmpC21=Cyc_Absyn_var_type(_tmp768);_tmp75E->hd=_tmpC21;}),_tmp75E->tl=effect;_tmp75E;});goto _LL6F;default: _LL86: _LL87:
# 4531
 effect=({struct Cyc_List_List*_tmp761=_cycalloc(sizeof(*_tmp761));({void*_tmpC22=Cyc_Absyn_regionsof_eff(Cyc_Absyn_var_type(_tmp768));_tmp761->hd=_tmpC22;}),_tmp761->tl=effect;_tmp761;});goto _LL6F;}}default: _LL84: _tmp766=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmp75C)->f1;_LL85:
# 4528
({struct Cyc_Core_Opt*_tmpC24=({struct Cyc_Core_Opt*_tmp760=_cycalloc(sizeof(*_tmp760));({void*_tmpC23=(void*)({struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_tmp75F=_cycalloc(sizeof(*_tmp75F));_tmp75F->tag=2U,_tmp75F->f1=0,_tmp75F->f2=& Cyc_Tcutil_ak;_tmp75F;});_tmp760->v=_tmpC23;});_tmp760;});*_tmp766=_tmpC24;});goto _LL87;}_LL6F:;};}}
# 4535
{struct Cyc_List_List*ts=_tmp71B.free_evars;for(0;ts != 0;ts=ts->tl){
struct _tuple30 _tmp769=*((struct _tuple30*)ts->hd);struct _tuple30 _tmp76A=_tmp769;void*_tmp771;int _tmp770;_LL89: _tmp771=_tmp76A.f1;_tmp770=_tmp76A.f2;_LL8A:;
if(!_tmp770)continue;{
struct Cyc_Absyn_Kind*_tmp76B=Cyc_Tcutil_type_kind(_tmp771);struct Cyc_Absyn_Kind*_tmp76C=_tmp76B;switch(((struct Cyc_Absyn_Kind*)_tmp76C)->kind){case Cyc_Absyn_RgnKind: _LL8C: _LL8D:
# 4540
 effect=({struct Cyc_List_List*_tmp76D=_cycalloc(sizeof(*_tmp76D));({void*_tmpC25=Cyc_Absyn_access_eff(_tmp771);_tmp76D->hd=_tmpC25;}),_tmp76D->tl=effect;_tmp76D;});goto _LL8B;case Cyc_Absyn_EffKind: _LL8E: _LL8F:
# 4542
 effect=({struct Cyc_List_List*_tmp76E=_cycalloc(sizeof(*_tmp76E));_tmp76E->hd=_tmp771,_tmp76E->tl=effect;_tmp76E;});goto _LL8B;case Cyc_Absyn_IntKind: _LL90: _LL91:
 goto _LL8B;default: _LL92: _LL93:
# 4545
 effect=({struct Cyc_List_List*_tmp76F=_cycalloc(sizeof(*_tmp76F));({void*_tmpC26=Cyc_Absyn_regionsof_eff(_tmp771);_tmp76F->hd=_tmpC26;}),_tmp76F->tl=effect;_tmp76F;});goto _LL8B;}_LL8B:;};}}
# 4548
({void*_tmpC27=Cyc_Absyn_join_eff(effect);*_tmp7D5=_tmpC27;});}
# 4554
if(*_tmp7D6 != 0){
struct Cyc_List_List*bs=*_tmp7D6;for(0;bs != 0;bs=bs->tl){
void*_tmp772=Cyc_Absyn_compress_kb(((struct Cyc_Absyn_Tvar*)bs->hd)->kind);void*_tmp773=_tmp772;struct Cyc_Core_Opt**_tmp783;struct Cyc_Absyn_Kind*_tmp782;struct Cyc_Core_Opt**_tmp781;struct Cyc_Core_Opt**_tmp780;struct Cyc_Core_Opt**_tmp77F;struct Cyc_Core_Opt**_tmp77E;struct Cyc_Core_Opt**_tmp77D;struct Cyc_Core_Opt**_tmp77C;struct Cyc_Core_Opt**_tmp77B;struct Cyc_Core_Opt**_tmp77A;struct Cyc_Core_Opt**_tmp779;switch(*((int*)_tmp773)){case 1U: _LL95: _tmp779=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmp773)->f1;_LL96:
# 4558
({struct Cyc_String_pa_PrintArg_struct _tmp776=({struct Cyc_String_pa_PrintArg_struct _tmp9E2;_tmp9E2.tag=0U,_tmp9E2.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*((struct Cyc_Absyn_Tvar*)bs->hd)->name);_tmp9E2;});void*_tmp774[1U];_tmp774[0]=& _tmp776;({unsigned int _tmpC29=loc;struct _dyneither_ptr _tmpC28=({const char*_tmp775="Type variable %s unconstrained, assuming boxed";_tag_dyneither(_tmp775,sizeof(char),47U);});Cyc_Tcutil_warn(_tmpC29,_tmpC28,_tag_dyneither(_tmp774,sizeof(void*),1U));});});
# 4560
_tmp77A=_tmp779;goto _LL98;case 2U: switch(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp773)->f2)->kind){case Cyc_Absyn_BoxKind: if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp773)->f2)->aliasqual == Cyc_Absyn_Top){_LL97: _tmp77A=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp773)->f1;_LL98:
 _tmp77B=_tmp77A;goto _LL9A;}else{goto _LLA7;}case Cyc_Absyn_MemKind: switch(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp773)->f2)->aliasqual){case Cyc_Absyn_Top: _LL99: _tmp77B=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp773)->f1;_LL9A:
 _tmp77C=_tmp77B;goto _LL9C;case Cyc_Absyn_Aliasable: _LL9B: _tmp77C=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp773)->f1;_LL9C:
 _tmp77E=_tmp77C;goto _LL9E;default: _LLA1: _tmp77D=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp773)->f1;_LLA2:
# 4567
 _tmp780=_tmp77D;goto _LLA4;}case Cyc_Absyn_AnyKind: switch(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp773)->f2)->aliasqual){case Cyc_Absyn_Top: _LL9D: _tmp77E=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp773)->f1;_LL9E:
# 4564
 _tmp77F=_tmp77E;goto _LLA0;case Cyc_Absyn_Aliasable: _LL9F: _tmp77F=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp773)->f1;_LLA0:
# 4566
({struct Cyc_Core_Opt*_tmpC2A=Cyc_Tcutil_kind_to_bound_opt(& Cyc_Tcutil_bk);*_tmp77F=_tmpC2A;});goto _LL94;default: _LLA3: _tmp780=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp773)->f1;_LLA4:
# 4569
({struct Cyc_Core_Opt*_tmpC2B=Cyc_Tcutil_kind_to_bound_opt(& Cyc_Tcutil_ubk);*_tmp780=_tmpC2B;});goto _LL94;}case Cyc_Absyn_RgnKind: if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp773)->f2)->aliasqual == Cyc_Absyn_Top){_LLA5: _tmp781=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp773)->f1;_LLA6:
# 4571
({struct Cyc_Core_Opt*_tmpC2C=Cyc_Tcutil_kind_to_bound_opt(& Cyc_Tcutil_rk);*_tmp781=_tmpC2C;});goto _LL94;}else{goto _LLA7;}default: _LLA7: _tmp783=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp773)->f1;_tmp782=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp773)->f2;_LLA8:
# 4573
({struct Cyc_Core_Opt*_tmpC2D=Cyc_Tcutil_kind_to_bound_opt(_tmp782);*_tmp783=_tmpC2D;});goto _LL94;}default: if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp773)->f1)->kind == Cyc_Absyn_MemKind){_LLA9: _LLAA:
# 4575
({void*_tmp777=0U;({unsigned int _tmpC2F=loc;struct _dyneither_ptr _tmpC2E=({const char*_tmp778="functions can't abstract M types";_tag_dyneither(_tmp778,sizeof(char),33U);});Cyc_Tcutil_terr(_tmpC2F,_tmpC2E,_tag_dyneither(_tmp777,sizeof(void*),0U));});});goto _LL94;}else{_LLAB: _LLAC:
 goto _LL94;}}_LL94:;}}
# 4580
({struct Cyc_List_List*_tmpC30=Cyc_Tcutil_remove_bound_tvars(Cyc_Core_heap_region,_tmp71B.kind_env,*_tmp7D6);cvtenv.kind_env=_tmpC30;});
({struct Cyc_List_List*_tmpC31=Cyc_Tcutil_remove_bound_tvars_bool(_tmp71B.r,_tmp71B.free_vars,*_tmp7D6);_tmp71B.free_vars=_tmpC31;});
# 4583
{struct Cyc_List_List*tvs=_tmp71B.free_vars;for(0;tvs != 0;tvs=tvs->tl){
struct _tuple29 _tmp784=*((struct _tuple29*)tvs->hd);struct _tuple29 _tmp785=_tmp784;struct Cyc_Absyn_Tvar*_tmp787;int _tmp786;_LLAE: _tmp787=_tmp785.f1;_tmp786=_tmp785.f2;_LLAF:;
({struct Cyc_List_List*_tmpC32=Cyc_Tcutil_fast_add_free_tvar_bool(cvtenv.r,cvtenv.free_vars,_tmp787,_tmp786);cvtenv.free_vars=_tmpC32;});}}
# 4588
{struct Cyc_List_List*evs=_tmp71B.free_evars;for(0;evs != 0;evs=evs->tl){
struct _tuple30 _tmp788=*((struct _tuple30*)evs->hd);struct _tuple30 _tmp789=_tmp788;void*_tmp78B;int _tmp78A;_LLB1: _tmp78B=_tmp789.f1;_tmp78A=_tmp789.f2;_LLB2:;
({struct Cyc_List_List*_tmpC33=Cyc_Tcutil_add_free_evar(cvtenv.r,cvtenv.free_evars,_tmp78B,_tmp78A);cvtenv.free_evars=_tmpC33;});}}
# 4592
goto _LL0;};};};}case 6U: _LL13: _tmp7D7=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp6D9)->f1;_LL14:
# 4595
 for(0;_tmp7D7 != 0;_tmp7D7=_tmp7D7->tl){
struct _tuple12*_tmp793=(struct _tuple12*)_tmp7D7->hd;
cvtenv=Cyc_Tcutil_i_check_valid_type(loc,te,cvtenv,& Cyc_Tcutil_tmk,(*_tmp793).f2,1,0);
({int _tmpC34=
Cyc_Tcutil_extract_const_from_typedef(loc,((*_tmp793).f1).print_const,(*_tmp793).f2);
# 4598
((*_tmp793).f1).real_const=_tmpC34;});}
# 4601
goto _LL0;case 7U: _LL15: _tmp7D9=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp6D9)->f1;_tmp7D8=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp6D9)->f2;_LL16: {
# 4605
struct Cyc_List_List*prev_fields=0;
for(0;_tmp7D8 != 0;_tmp7D8=_tmp7D8->tl){
struct Cyc_Absyn_Aggrfield*_tmp794=(struct Cyc_Absyn_Aggrfield*)_tmp7D8->hd;struct Cyc_Absyn_Aggrfield*_tmp795=_tmp794;struct _dyneither_ptr*_tmp7A5;struct Cyc_Absyn_Tqual*_tmp7A4;void*_tmp7A3;struct Cyc_Absyn_Exp*_tmp7A2;struct Cyc_List_List*_tmp7A1;struct Cyc_Absyn_Exp*_tmp7A0;_LLB4: _tmp7A5=_tmp795->name;_tmp7A4=(struct Cyc_Absyn_Tqual*)& _tmp795->tq;_tmp7A3=_tmp795->type;_tmp7A2=_tmp795->width;_tmp7A1=_tmp795->attributes;_tmp7A0=_tmp795->requires_clause;_LLB5:;
if(((int(*)(int(*compare)(struct _dyneither_ptr*,struct _dyneither_ptr*),struct Cyc_List_List*l,struct _dyneither_ptr*x))Cyc_List_mem)(Cyc_strptrcmp,prev_fields,_tmp7A5))
({struct Cyc_String_pa_PrintArg_struct _tmp798=({struct Cyc_String_pa_PrintArg_struct _tmp9E4;_tmp9E4.tag=0U,_tmp9E4.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*_tmp7A5);_tmp9E4;});void*_tmp796[1U];_tmp796[0]=& _tmp798;({unsigned int _tmpC36=loc;struct _dyneither_ptr _tmpC35=({const char*_tmp797="duplicate field %s";_tag_dyneither(_tmp797,sizeof(char),19U);});Cyc_Tcutil_terr(_tmpC36,_tmpC35,_tag_dyneither(_tmp796,sizeof(void*),1U));});});
if(({struct _dyneither_ptr _tmpC37=(struct _dyneither_ptr)*_tmp7A5;Cyc_strcmp(_tmpC37,({const char*_tmp799="";_tag_dyneither(_tmp799,sizeof(char),1U);}));})!= 0)
prev_fields=({struct Cyc_List_List*_tmp79A=_cycalloc(sizeof(*_tmp79A));_tmp79A->hd=_tmp7A5,_tmp79A->tl=prev_fields;_tmp79A;});
cvtenv=Cyc_Tcutil_i_check_valid_type(loc,te,cvtenv,& Cyc_Tcutil_tmk,_tmp7A3,1,0);
({int _tmpC38=Cyc_Tcutil_extract_const_from_typedef(loc,_tmp7A4->print_const,_tmp7A3);_tmp7A4->real_const=_tmpC38;});
Cyc_Tcutil_check_bitfield(loc,te,_tmp7A3,_tmp7A2,_tmp7A5);
Cyc_Tcutil_check_field_atts(loc,_tmp7A5,_tmp7A1);
if(_tmp7A0 != 0){
# 4618
if((int)_tmp7D9 != (int)1U)
({void*_tmp79B=0U;({unsigned int _tmpC3A=loc;struct _dyneither_ptr _tmpC39=({const char*_tmp79C="@requires clause is only allowed on union members";_tag_dyneither(_tmp79C,sizeof(char),50U);});Cyc_Tcutil_terr(_tmpC3A,_tmpC39,_tag_dyneither(_tmp79B,sizeof(void*),0U));});});
({struct Cyc_Tcenv_Tenv*_tmpC3B=Cyc_Tcenv_allow_valueof(te);Cyc_Tcexp_tcExp(_tmpC3B,0,_tmp7A0);});
if(!Cyc_Tcutil_is_integral(_tmp7A0))
({struct Cyc_String_pa_PrintArg_struct _tmp79F=({struct Cyc_String_pa_PrintArg_struct _tmp9E5;_tmp9E5.tag=0U,({
struct _dyneither_ptr _tmpC3C=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string((void*)_check_null(_tmp7A0->topt)));_tmp9E5.f1=_tmpC3C;});_tmp9E5;});void*_tmp79D[1U];_tmp79D[0]=& _tmp79F;({unsigned int _tmpC3E=loc;struct _dyneither_ptr _tmpC3D=({const char*_tmp79E="@requires clause has type %s instead of integral type";_tag_dyneither(_tmp79E,sizeof(char),54U);});Cyc_Tcutil_terr(_tmpC3E,_tmpC3D,_tag_dyneither(_tmp79D,sizeof(void*),1U));});});
cvtenv=Cyc_Tcutil_i_check_valid_type_level_exp(_tmp7A0,te,cvtenv);}}
# 4627
goto _LL0;}default: _LL17: _tmp7DD=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp6D9)->f1;_tmp7DC=(struct Cyc_List_List**)&((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp6D9)->f2;_tmp7DB=(struct Cyc_Absyn_Typedefdecl**)&((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp6D9)->f3;_tmp7DA=(void**)&((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp6D9)->f4;_LL18: {
# 4630
struct Cyc_List_List*targs=*_tmp7DC;
# 4632
struct Cyc_Absyn_Typedefdecl*td;
{struct _handler_cons _tmp7A6;_push_handler(& _tmp7A6);{int _tmp7A8=0;if(setjmp(_tmp7A6.handler))_tmp7A8=1;if(!_tmp7A8){td=Cyc_Tcenv_lookup_typedefdecl(te,loc,_tmp7DD);;_pop_handler();}else{void*_tmp7A7=(void*)_exn_thrown;void*_tmp7A9=_tmp7A7;void*_tmp7AD;if(((struct Cyc_Dict_Absent_exn_struct*)_tmp7A9)->tag == Cyc_Dict_Absent){_LLB7: _LLB8:
# 4635
({struct Cyc_String_pa_PrintArg_struct _tmp7AC=({struct Cyc_String_pa_PrintArg_struct _tmp9E6;_tmp9E6.tag=0U,({struct _dyneither_ptr _tmpC3F=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_qvar2string(_tmp7DD));_tmp9E6.f1=_tmpC3F;});_tmp9E6;});void*_tmp7AA[1U];_tmp7AA[0]=& _tmp7AC;({unsigned int _tmpC41=loc;struct _dyneither_ptr _tmpC40=({const char*_tmp7AB="unbound typedef name %s";_tag_dyneither(_tmp7AB,sizeof(char),24U);});Cyc_Tcutil_terr(_tmpC41,_tmpC40,_tag_dyneither(_tmp7AA,sizeof(void*),1U));});});
return cvtenv;}else{_LLB9: _tmp7AD=_tmp7A9;_LLBA:(int)_rethrow(_tmp7AD);}_LLB6:;}};}
# 4638
*_tmp7DB=td;{
struct Cyc_List_List*tvs=td->tvs;
struct Cyc_List_List*ts=targs;
struct Cyc_List_List*inst=0;
# 4643
for(0;ts != 0  && tvs != 0;(ts=ts->tl,tvs=tvs->tl)){
struct Cyc_Absyn_Kind*k=Cyc_Tcutil_tvar_inst_kind((struct Cyc_Absyn_Tvar*)tvs->hd,& Cyc_Tcutil_tbk,expected_kind,td);
# 4648
cvtenv=Cyc_Tcutil_i_check_valid_type(loc,te,cvtenv,k,(void*)ts->hd,1,allow_abs_aggr);
Cyc_Tcutil_check_no_qual(loc,(void*)ts->hd);
inst=({struct Cyc_List_List*_tmp7AF=_cycalloc(sizeof(*_tmp7AF));({struct _tuple15*_tmpC42=({struct _tuple15*_tmp7AE=_cycalloc(sizeof(*_tmp7AE));_tmp7AE->f1=(struct Cyc_Absyn_Tvar*)tvs->hd,_tmp7AE->f2=(void*)ts->hd;_tmp7AE;});_tmp7AF->hd=_tmpC42;}),_tmp7AF->tl=inst;_tmp7AF;});}
# 4652
if(ts != 0)
({struct Cyc_String_pa_PrintArg_struct _tmp7B2=({struct Cyc_String_pa_PrintArg_struct _tmp9E7;_tmp9E7.tag=0U,({struct _dyneither_ptr _tmpC43=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_qvar2string(_tmp7DD));_tmp9E7.f1=_tmpC43;});_tmp9E7;});void*_tmp7B0[1U];_tmp7B0[0]=& _tmp7B2;({unsigned int _tmpC45=loc;struct _dyneither_ptr _tmpC44=({const char*_tmp7B1="too many parameters for typedef %s";_tag_dyneither(_tmp7B1,sizeof(char),35U);});Cyc_Tcutil_terr(_tmpC45,_tmpC44,_tag_dyneither(_tmp7B0,sizeof(void*),1U));});});
if(tvs != 0){
struct Cyc_List_List*hidden_ts=0;
# 4657
for(0;tvs != 0;tvs=tvs->tl){
struct Cyc_Absyn_Kind*k=Cyc_Tcutil_tvar_inst_kind((struct Cyc_Absyn_Tvar*)tvs->hd,& Cyc_Tcutil_bk,expected_kind,td);
void*e=Cyc_Absyn_new_evar(0,0);
hidden_ts=({struct Cyc_List_List*_tmp7B3=_cycalloc(sizeof(*_tmp7B3));_tmp7B3->hd=e,_tmp7B3->tl=hidden_ts;_tmp7B3;});
cvtenv=Cyc_Tcutil_i_check_valid_type(loc,te,cvtenv,k,e,1,allow_abs_aggr);
inst=({struct Cyc_List_List*_tmp7B5=_cycalloc(sizeof(*_tmp7B5));({struct _tuple15*_tmpC46=({struct _tuple15*_tmp7B4=_cycalloc(sizeof(*_tmp7B4));_tmp7B4->f1=(struct Cyc_Absyn_Tvar*)tvs->hd,_tmp7B4->f2=e;_tmp7B4;});_tmp7B5->hd=_tmpC46;}),_tmp7B5->tl=inst;_tmp7B5;});}
# 4665
({struct Cyc_List_List*_tmpC48=({struct Cyc_List_List*_tmpC47=targs;((struct Cyc_List_List*(*)(struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_imp_append)(_tmpC47,((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(hidden_ts));});*_tmp7DC=_tmpC48;});}
# 4667
if(td->defn != 0){
void*new_typ=
inst == 0?(void*)_check_null(td->defn):
 Cyc_Tcutil_substitute(inst,(void*)_check_null(td->defn));
*_tmp7DA=new_typ;}
# 4673
goto _LL0;};}}_LL0:;}
# 4675
if(!({struct Cyc_Absyn_Kind*_tmpC49=Cyc_Tcutil_type_kind(t);Cyc_Tcutil_kind_leq(_tmpC49,expected_kind);}))
({struct Cyc_String_pa_PrintArg_struct _tmp7E0=({struct Cyc_String_pa_PrintArg_struct _tmp9EA;_tmp9EA.tag=0U,({
struct _dyneither_ptr _tmpC4A=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(t));_tmp9EA.f1=_tmpC4A;});_tmp9EA;});struct Cyc_String_pa_PrintArg_struct _tmp7E1=({struct Cyc_String_pa_PrintArg_struct _tmp9E9;_tmp9E9.tag=0U,({struct _dyneither_ptr _tmpC4B=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_kind2string(Cyc_Tcutil_type_kind(t)));_tmp9E9.f1=_tmpC4B;});_tmp9E9;});struct Cyc_String_pa_PrintArg_struct _tmp7E2=({struct Cyc_String_pa_PrintArg_struct _tmp9E8;_tmp9E8.tag=0U,({struct _dyneither_ptr _tmpC4C=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_kind2string(expected_kind));_tmp9E8.f1=_tmpC4C;});_tmp9E8;});void*_tmp7DE[3U];_tmp7DE[0]=& _tmp7E0,_tmp7DE[1]=& _tmp7E1,_tmp7DE[2]=& _tmp7E2;({unsigned int _tmpC4E=loc;struct _dyneither_ptr _tmpC4D=({const char*_tmp7DF="type %s has kind %s but as used here needs kind %s";_tag_dyneither(_tmp7DF,sizeof(char),51U);});Cyc_Tcutil_terr(_tmpC4E,_tmpC4D,_tag_dyneither(_tmp7DE,sizeof(void*),3U));});});
# 4679
return cvtenv;}
# 4687
static struct Cyc_Tcutil_CVTEnv Cyc_Tcutil_i_check_valid_type_level_exp(struct Cyc_Absyn_Exp*e,struct Cyc_Tcenv_Tenv*te,struct Cyc_Tcutil_CVTEnv cvtenv){
# 4689
{void*_tmp7E3=e->r;void*_tmp7E4=_tmp7E3;struct Cyc_Absyn_Exp*_tmp7F7;struct Cyc_Absyn_Exp*_tmp7F6;void*_tmp7F5;void*_tmp7F4;void*_tmp7F3;void*_tmp7F2;struct Cyc_Absyn_Exp*_tmp7F1;struct Cyc_Absyn_Exp*_tmp7F0;struct Cyc_Absyn_Exp*_tmp7EF;struct Cyc_Absyn_Exp*_tmp7EE;struct Cyc_Absyn_Exp*_tmp7ED;struct Cyc_Absyn_Exp*_tmp7EC;struct Cyc_Absyn_Exp*_tmp7EB;struct Cyc_Absyn_Exp*_tmp7EA;struct Cyc_Absyn_Exp*_tmp7E9;struct Cyc_Absyn_Exp*_tmp7E8;struct Cyc_List_List*_tmp7E7;switch(*((int*)_tmp7E4)){case 0U: _LL1: _LL2:
 goto _LL4;case 32U: _LL3: _LL4:
 goto _LL6;case 33U: _LL5: _LL6:
 goto _LL8;case 2U: _LL7: _LL8:
 goto _LLA;case 1U: _LL9: _LLA:
 goto _LL0;case 3U: _LLB: _tmp7E7=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp7E4)->f2;_LLC:
# 4696
 for(0;_tmp7E7 != 0;_tmp7E7=_tmp7E7->tl){
cvtenv=Cyc_Tcutil_i_check_valid_type_level_exp((struct Cyc_Absyn_Exp*)_tmp7E7->hd,te,cvtenv);}
goto _LL0;case 6U: _LLD: _tmp7EA=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp7E4)->f1;_tmp7E9=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp7E4)->f2;_tmp7E8=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp7E4)->f3;_LLE:
# 4700
 cvtenv=Cyc_Tcutil_i_check_valid_type_level_exp(_tmp7EA,te,cvtenv);
cvtenv=Cyc_Tcutil_i_check_valid_type_level_exp(_tmp7E9,te,cvtenv);
cvtenv=Cyc_Tcutil_i_check_valid_type_level_exp(_tmp7E8,te,cvtenv);
goto _LL0;case 7U: _LLF: _tmp7EC=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp7E4)->f1;_tmp7EB=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp7E4)->f2;_LL10:
 _tmp7EE=_tmp7EC;_tmp7ED=_tmp7EB;goto _LL12;case 8U: _LL11: _tmp7EE=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp7E4)->f1;_tmp7ED=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp7E4)->f2;_LL12:
 _tmp7F0=_tmp7EE;_tmp7EF=_tmp7ED;goto _LL14;case 9U: _LL13: _tmp7F0=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp7E4)->f1;_tmp7EF=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp7E4)->f2;_LL14:
# 4707
 cvtenv=Cyc_Tcutil_i_check_valid_type_level_exp(_tmp7F0,te,cvtenv);
cvtenv=Cyc_Tcutil_i_check_valid_type_level_exp(_tmp7EF,te,cvtenv);
goto _LL0;case 14U: _LL15: _tmp7F2=(void*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp7E4)->f1;_tmp7F1=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp7E4)->f2;_LL16:
# 4711
 cvtenv=Cyc_Tcutil_i_check_valid_type_level_exp(_tmp7F1,te,cvtenv);
cvtenv=Cyc_Tcutil_i_check_valid_type(e->loc,te,cvtenv,& Cyc_Tcutil_tak,_tmp7F2,1,0);
goto _LL0;case 19U: _LL17: _tmp7F3=(void*)((struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_tmp7E4)->f1;_LL18:
 _tmp7F4=_tmp7F3;goto _LL1A;case 17U: _LL19: _tmp7F4=(void*)((struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*)_tmp7E4)->f1;_LL1A:
# 4716
 cvtenv=Cyc_Tcutil_i_check_valid_type(e->loc,te,cvtenv,& Cyc_Tcutil_tak,_tmp7F4,1,0);
goto _LL0;case 39U: _LL1B: _tmp7F5=(void*)((struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*)_tmp7E4)->f1;_LL1C:
# 4719
 cvtenv=Cyc_Tcutil_i_check_valid_type(e->loc,te,cvtenv,& Cyc_Tcutil_ik,_tmp7F5,1,0);
goto _LL0;case 18U: _LL1D: _tmp7F6=((struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_tmp7E4)->f1;_LL1E:
# 4722
 cvtenv=Cyc_Tcutil_i_check_valid_type_level_exp(_tmp7F6,te,cvtenv);
goto _LL0;case 41U: _LL1F: _tmp7F7=((struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*)_tmp7E4)->f1;_LL20:
# 4725
 cvtenv=Cyc_Tcutil_i_check_valid_type_level_exp(_tmp7F7,te,cvtenv);
goto _LL0;default: _LL21: _LL22:
# 4728
({void*_tmp7E5=0U;({struct _dyneither_ptr _tmpC4F=({const char*_tmp7E6="non-type-level-expression in Tcutil::i_check_valid_type_level_exp";_tag_dyneither(_tmp7E6,sizeof(char),66U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpC4F,_tag_dyneither(_tmp7E5,sizeof(void*),0U));});});}_LL0:;}
# 4730
return cvtenv;}
# 4733
static struct Cyc_Tcutil_CVTEnv Cyc_Tcutil_check_valid_type(unsigned int loc,struct Cyc_Tcenv_Tenv*te,struct Cyc_Tcutil_CVTEnv cvt,struct Cyc_Absyn_Kind*expected_kind,int allow_abs_aggr,void*t){
# 4738
struct Cyc_List_List*_tmp7F8=cvt.kind_env;
cvt=Cyc_Tcutil_i_check_valid_type(loc,te,cvt,expected_kind,t,1,allow_abs_aggr);
# 4741
{struct Cyc_List_List*vs=cvt.free_vars;for(0;vs != 0;vs=vs->tl){
struct _tuple29 _tmp7F9=*((struct _tuple29*)vs->hd);struct _tuple29 _tmp7FA=_tmp7F9;struct Cyc_Absyn_Tvar*_tmp7FB;_LL1: _tmp7FB=_tmp7FA.f1;_LL2:;
({struct Cyc_List_List*_tmpC50=Cyc_Tcutil_fast_add_free_tvar(_tmp7F8,_tmp7FB);cvt.kind_env=_tmpC50;});}}
# 4749
{struct Cyc_List_List*evs=cvt.free_evars;for(0;evs != 0;evs=evs->tl){
struct _tuple30 _tmp7FC=*((struct _tuple30*)evs->hd);struct _tuple30 _tmp7FD=_tmp7FC;void*_tmp806;_LL4: _tmp806=_tmp7FD.f1;_LL5:;{
void*_tmp7FE=Cyc_Tcutil_compress(_tmp806);void*_tmp7FF=_tmp7FE;struct Cyc_Core_Opt**_tmp805;if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp7FF)->tag == 1U){_LL7: _tmp805=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp7FF)->f4;_LL8:
# 4753
 if(*_tmp805 == 0)
({struct Cyc_Core_Opt*_tmpC51=({struct Cyc_Core_Opt*_tmp800=_cycalloc(sizeof(*_tmp800));_tmp800->v=_tmp7F8;_tmp800;});*_tmp805=_tmpC51;});else{
# 4757
struct Cyc_List_List*_tmp801=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(*_tmp805))->v;
struct Cyc_List_List*_tmp802=0;
for(0;_tmp801 != 0;_tmp801=_tmp801->tl){
if(((int(*)(int(*compare)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct Cyc_Absyn_Tvar*x))Cyc_List_mem)(Cyc_Tcutil_fast_tvar_cmp,_tmp7F8,(struct Cyc_Absyn_Tvar*)_tmp801->hd))
_tmp802=({struct Cyc_List_List*_tmp803=_cycalloc(sizeof(*_tmp803));_tmp803->hd=(struct Cyc_Absyn_Tvar*)_tmp801->hd,_tmp803->tl=_tmp802;_tmp803;});}
({struct Cyc_Core_Opt*_tmpC52=({struct Cyc_Core_Opt*_tmp804=_cycalloc(sizeof(*_tmp804));_tmp804->v=_tmp802;_tmp804;});*_tmp805=_tmpC52;});}
# 4764
goto _LL6;}else{_LL9: _LLA:
 goto _LL6;}_LL6:;};}}
# 4768
return cvt;}
# 4775
void Cyc_Tcutil_check_free_evars(struct Cyc_List_List*free_evars,void*in_t,unsigned int loc){
for(0;free_evars != 0;free_evars=free_evars->tl){
void*e=(void*)free_evars->hd;
{void*_tmp807=Cyc_Tcutil_compress(e);void*_tmp808=_tmp807;if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp808)->tag == 1U){_LL1: _LL2:
 goto _LL0;}else{_LL3: _LL4:
# 4781
 continue;}_LL0:;}{
# 4783
struct Cyc_Absyn_Kind*_tmp809=Cyc_Tcutil_type_kind(e);struct Cyc_Absyn_Kind*_tmp80A=_tmp809;switch(((struct Cyc_Absyn_Kind*)_tmp80A)->kind){case Cyc_Absyn_RgnKind: switch(((struct Cyc_Absyn_Kind*)_tmp80A)->aliasqual){case Cyc_Absyn_Unique: _LL6: _LL7:
# 4785
 if(!Cyc_Tcutil_unify(e,Cyc_Absyn_unique_rgn_type))
({void*_tmp80B=0U;({struct _dyneither_ptr _tmpC53=({const char*_tmp80C="can't unify evar with unique region!";_tag_dyneither(_tmp80C,sizeof(char),37U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpC53,_tag_dyneither(_tmp80B,sizeof(void*),0U));});});
goto _LL5;case Cyc_Absyn_Aliasable: _LL8: _LL9:
 goto _LLB;default: _LLA: _LLB:
# 4790
 if(!Cyc_Tcutil_unify(e,Cyc_Absyn_heap_rgn_type))({void*_tmp80D=0U;({struct _dyneither_ptr _tmpC54=({const char*_tmp80E="can't unify evar with heap!";_tag_dyneither(_tmp80E,sizeof(char),28U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpC54,_tag_dyneither(_tmp80D,sizeof(void*),0U));});});
goto _LL5;}case Cyc_Absyn_EffKind: _LLC: _LLD:
# 4793
 if(!Cyc_Tcutil_unify(e,Cyc_Absyn_empty_effect))({void*_tmp80F=0U;({struct _dyneither_ptr _tmpC55=({const char*_tmp810="can't unify evar with {}!";_tag_dyneither(_tmp810,sizeof(char),26U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpC55,_tag_dyneither(_tmp80F,sizeof(void*),0U));});});
goto _LL5;case Cyc_Absyn_BoolKind: _LLE: _LLF:
# 4796
 if(!Cyc_Tcutil_unify(e,Cyc_Absyn_false_type))({struct Cyc_String_pa_PrintArg_struct _tmp813=({struct Cyc_String_pa_PrintArg_struct _tmp9EB;_tmp9EB.tag=0U,({
struct _dyneither_ptr _tmpC56=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(e));_tmp9EB.f1=_tmpC56;});_tmp9EB;});void*_tmp811[1U];_tmp811[0]=& _tmp813;({struct _dyneither_ptr _tmpC57=({const char*_tmp812="can't unify evar %s with @false!";_tag_dyneither(_tmp812,sizeof(char),33U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpC57,_tag_dyneither(_tmp811,sizeof(void*),1U));});});
goto _LL5;case Cyc_Absyn_PtrBndKind: _LL10: _LL11:
# 4800
 if(!({void*_tmpC58=e;Cyc_Tcutil_unify(_tmpC58,Cyc_Absyn_bounds_one());}))({void*_tmp814=0U;({struct _dyneither_ptr _tmpC59=({const char*_tmp815="can't unify evar with bounds_one!";_tag_dyneither(_tmp815,sizeof(char),34U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpC59,_tag_dyneither(_tmp814,sizeof(void*),0U));});});
goto _LL5;default: _LL12: _LL13:
# 4803
({struct Cyc_String_pa_PrintArg_struct _tmp818=({struct Cyc_String_pa_PrintArg_struct _tmp9ED;_tmp9ED.tag=0U,({
struct _dyneither_ptr _tmpC5A=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(e));_tmp9ED.f1=_tmpC5A;});_tmp9ED;});struct Cyc_String_pa_PrintArg_struct _tmp819=({struct Cyc_String_pa_PrintArg_struct _tmp9EC;_tmp9EC.tag=0U,({struct _dyneither_ptr _tmpC5B=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(in_t));_tmp9EC.f1=_tmpC5B;});_tmp9EC;});void*_tmp816[2U];_tmp816[0]=& _tmp818,_tmp816[1]=& _tmp819;({unsigned int _tmpC5D=loc;struct _dyneither_ptr _tmpC5C=({const char*_tmp817="hidden type variable %s isn't abstracted in type %s";_tag_dyneither(_tmp817,sizeof(char),52U);});Cyc_Tcutil_terr(_tmpC5D,_tmpC5C,_tag_dyneither(_tmp816,sizeof(void*),2U));});});
goto _LL5;}_LL5:;};}}
# 4815
void Cyc_Tcutil_check_valid_toplevel_type(unsigned int loc,struct Cyc_Tcenv_Tenv*te,void*t){
int generalize_evars=Cyc_Tcutil_is_function_type(t);
if(te->in_tempest  && !te->tempest_generalize)generalize_evars=0;{
struct Cyc_List_List*_tmp81A=Cyc_Tcenv_lookup_type_vars(te);
struct Cyc_Absyn_Kind*expected_kind=Cyc_Tcutil_is_function_type(t)?& Cyc_Tcutil_tak:& Cyc_Tcutil_tmk;
struct Cyc_Tcutil_CVTEnv _tmp81B=({unsigned int _tmpC61=loc;struct Cyc_Tcenv_Tenv*_tmpC60=te;struct Cyc_Tcutil_CVTEnv _tmpC5F=({struct Cyc_Tcutil_CVTEnv _tmp9F1;_tmp9F1.r=Cyc_Core_heap_region,_tmp9F1.kind_env=_tmp81A,_tmp9F1.free_vars=0,_tmp9F1.free_evars=0,_tmp9F1.generalize_evars=generalize_evars,_tmp9F1.fn_result=0;_tmp9F1;});struct Cyc_Absyn_Kind*_tmpC5E=expected_kind;Cyc_Tcutil_check_valid_type(_tmpC61,_tmpC60,_tmpC5F,_tmpC5E,1,t);});
# 4824
struct Cyc_List_List*_tmp81C=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(struct _tuple29*),struct Cyc_List_List*x))Cyc_List_map)((struct Cyc_Absyn_Tvar*(*)(struct _tuple29*))Cyc_Core_fst,_tmp81B.free_vars);
struct Cyc_List_List*_tmp81D=((struct Cyc_List_List*(*)(void*(*f)(struct _tuple30*),struct Cyc_List_List*x))Cyc_List_map)((void*(*)(struct _tuple30*))Cyc_Core_fst,_tmp81B.free_evars);
# 4828
if(_tmp81A != 0){
struct Cyc_List_List*_tmp81E=0;
{struct Cyc_List_List*_tmp81F=_tmp81C;for(0;(unsigned int)_tmp81F;_tmp81F=_tmp81F->tl){
struct Cyc_Absyn_Tvar*_tmp820=(struct Cyc_Absyn_Tvar*)_tmp81F->hd;
int found=0;
{struct Cyc_List_List*_tmp821=_tmp81A;for(0;(unsigned int)_tmp821;_tmp821=_tmp821->tl){
if(Cyc_Absyn_tvar_cmp(_tmp820,(struct Cyc_Absyn_Tvar*)_tmp821->hd)== 0){found=1;break;}}}
if(!found)
_tmp81E=({struct Cyc_List_List*_tmp822=_cycalloc(sizeof(*_tmp822));_tmp822->hd=(struct Cyc_Absyn_Tvar*)_tmp81F->hd,_tmp822->tl=_tmp81E;_tmp822;});}}
# 4838
_tmp81C=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(_tmp81E);}
# 4844
{struct Cyc_List_List*x=_tmp81C;for(0;x != 0;x=x->tl){
void*_tmp823=Cyc_Absyn_compress_kb(((struct Cyc_Absyn_Tvar*)x->hd)->kind);void*_tmp824=_tmp823;enum Cyc_Absyn_AliasQual _tmp832;struct Cyc_Core_Opt**_tmp831;struct Cyc_Absyn_Kind*_tmp830;struct Cyc_Core_Opt**_tmp82F;struct Cyc_Core_Opt**_tmp82E;struct Cyc_Core_Opt**_tmp82D;struct Cyc_Core_Opt**_tmp82C;struct Cyc_Core_Opt**_tmp82B;struct Cyc_Core_Opt**_tmp82A;switch(*((int*)_tmp824)){case 1U: _LL1: _tmp82A=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmp824)->f1;_LL2:
 _tmp82B=_tmp82A;goto _LL4;case 2U: switch(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp824)->f2)->kind){case Cyc_Absyn_BoxKind: if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp824)->f2)->aliasqual == Cyc_Absyn_Top){_LL3: _tmp82B=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp824)->f1;_LL4:
 _tmp82C=_tmp82B;goto _LL6;}else{goto _LLD;}case Cyc_Absyn_MemKind: switch(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp824)->f2)->aliasqual){case Cyc_Absyn_Top: _LL5: _tmp82C=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp824)->f1;_LL6:
 _tmp82D=_tmp82C;goto _LL8;case Cyc_Absyn_Aliasable: _LL7: _tmp82D=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp824)->f1;_LL8:
# 4850
({struct Cyc_Core_Opt*_tmpC62=Cyc_Tcutil_kind_to_bound_opt(& Cyc_Tcutil_bk);*_tmp82D=_tmpC62;});goto _LL0;default: _LL9: _tmp82E=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp824)->f1;_LLA:
# 4852
({struct Cyc_Core_Opt*_tmpC63=Cyc_Tcutil_kind_to_bound_opt(& Cyc_Tcutil_ubk);*_tmp82E=_tmpC63;});goto _LL0;}case Cyc_Absyn_RgnKind: if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp824)->f2)->aliasqual == Cyc_Absyn_Top){_LLB: _tmp82F=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp824)->f1;_LLC:
# 4854
({struct Cyc_Core_Opt*_tmpC64=Cyc_Tcutil_kind_to_bound_opt(& Cyc_Tcutil_rk);*_tmp82F=_tmpC64;});goto _LL0;}else{goto _LLD;}default: _LLD: _tmp831=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp824)->f1;_tmp830=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp824)->f2;_LLE:
# 4856
({struct Cyc_Core_Opt*_tmpC65=Cyc_Tcutil_kind_to_bound_opt(_tmp830);*_tmp831=_tmpC65;});goto _LL0;}default: if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp824)->f1)->kind == Cyc_Absyn_MemKind){_LLF: _tmp832=(((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp824)->f1)->aliasqual;_LL10:
# 4858
({struct Cyc_String_pa_PrintArg_struct _tmp827=({struct Cyc_String_pa_PrintArg_struct _tmp9EF;_tmp9EF.tag=0U,({
struct _dyneither_ptr _tmpC66=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_tvar2string((struct Cyc_Absyn_Tvar*)x->hd));_tmp9EF.f1=_tmpC66;});_tmp9EF;});struct Cyc_String_pa_PrintArg_struct _tmp828=({struct Cyc_String_pa_PrintArg_struct _tmp9EE;_tmp9EE.tag=0U,({struct _dyneither_ptr _tmpC67=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_kind2string(({struct Cyc_Absyn_Kind*_tmp829=_cycalloc(sizeof(*_tmp829));_tmp829->kind=Cyc_Absyn_MemKind,_tmp829->aliasqual=_tmp832;_tmp829;})));_tmp9EE.f1=_tmpC67;});_tmp9EE;});void*_tmp825[2U];_tmp825[0]=& _tmp827,_tmp825[1]=& _tmp828;({unsigned int _tmpC69=loc;struct _dyneither_ptr _tmpC68=({const char*_tmp826="type variable %s cannot have kind %s";_tag_dyneither(_tmp826,sizeof(char),37U);});Cyc_Tcutil_terr(_tmpC69,_tmpC68,_tag_dyneither(_tmp825,sizeof(void*),2U));});});
goto _LL0;}else{_LL11: _LL12:
 goto _LL0;}}_LL0:;}}
# 4865
if(_tmp81C != 0  || _tmp81D != 0){
{void*_tmp833=Cyc_Tcutil_compress(t);void*_tmp834=_tmp833;struct Cyc_List_List**_tmp835;if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp834)->tag == 5U){_LL14: _tmp835=(struct Cyc_List_List**)&(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp834)->f1).tvars;_LL15:
# 4868
 if(*_tmp835 == 0){
# 4870
({struct Cyc_List_List*_tmpC6A=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_copy)(_tmp81C);*_tmp835=_tmpC6A;});
_tmp81C=0;}
# 4873
goto _LL13;}else{_LL16: _LL17:
 goto _LL13;}_LL13:;}
# 4876
if(_tmp81C != 0)
({struct Cyc_String_pa_PrintArg_struct _tmp838=({struct Cyc_String_pa_PrintArg_struct _tmp9F0;_tmp9F0.tag=0U,_tmp9F0.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*((struct Cyc_Absyn_Tvar*)_tmp81C->hd)->name);_tmp9F0;});void*_tmp836[1U];_tmp836[0]=& _tmp838;({unsigned int _tmpC6C=loc;struct _dyneither_ptr _tmpC6B=({const char*_tmp837="unbound type variable %s";_tag_dyneither(_tmp837,sizeof(char),25U);});Cyc_Tcutil_terr(_tmpC6C,_tmpC6B,_tag_dyneither(_tmp836,sizeof(void*),1U));});});
if(!Cyc_Tcutil_is_function_type(t) || !te->in_tempest)
Cyc_Tcutil_check_free_evars(_tmp81D,t,loc);}};}
# 4887
void Cyc_Tcutil_check_fndecl_valid_type(unsigned int loc,struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Fndecl*fd){
void*t=Cyc_Tcutil_fndecl2type(fd);
# 4890
Cyc_Tcutil_check_valid_toplevel_type(loc,te,t);
{void*_tmp839=Cyc_Tcutil_compress(t);void*_tmp83A=_tmp839;struct Cyc_Absyn_FnInfo _tmp83D;if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp83A)->tag == 5U){_LL1: _tmp83D=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp83A)->f1;_LL2:
# 4893
 fd->i=_tmp83D;
({int _tmpC6D=
Cyc_Tcutil_extract_const_from_typedef(loc,(_tmp83D.ret_tqual).print_const,_tmp83D.ret_type);
# 4894
((fd->i).ret_tqual).real_const=_tmpC6D;});
# 4896
goto _LL0;}else{_LL3: _LL4:
({void*_tmp83B=0U;({struct _dyneither_ptr _tmpC6E=({const char*_tmp83C="check_fndecl_valid_type: not a FnType";_tag_dyneither(_tmp83C,sizeof(char),38U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpC6E,_tag_dyneither(_tmp83B,sizeof(void*),0U));});});}_LL0:;}{
# 4899
struct _RegionHandle _tmp83E=_new_region("r");struct _RegionHandle*r=& _tmp83E;_push_region(r);
({struct Cyc_List_List*_tmpC70=((struct Cyc_List_List*(*)(struct _dyneither_ptr*(*f)(struct _tuple10*),struct Cyc_List_List*x))Cyc_List_map)((struct _dyneither_ptr*(*)(struct _tuple10*t))Cyc_Tcutil_fst_fdarg,(fd->i).args);unsigned int _tmpC6F=loc;Cyc_Tcutil_check_unique_vars(_tmpC70,_tmpC6F,({const char*_tmp83F="function declaration has repeated parameter";_tag_dyneither(_tmp83F,sizeof(char),44U);}));});
# 4903
fd->cached_type=t;
# 4900
;_pop_region(r);};}
# 4908
void Cyc_Tcutil_check_type(unsigned int loc,struct Cyc_Tcenv_Tenv*te,struct Cyc_List_List*bound_tvars,struct Cyc_Absyn_Kind*expected_kind,int allow_evars,int allow_abs_aggr,void*t){
# 4914
struct Cyc_Tcutil_CVTEnv _tmp840=({unsigned int _tmpC75=loc;struct Cyc_Tcenv_Tenv*_tmpC74=te;struct Cyc_Tcutil_CVTEnv _tmpC73=({struct Cyc_Tcutil_CVTEnv _tmp9F4;_tmp9F4.r=Cyc_Core_heap_region,_tmp9F4.kind_env=bound_tvars,_tmp9F4.free_vars=0,_tmp9F4.free_evars=0,_tmp9F4.generalize_evars=0,_tmp9F4.fn_result=0;_tmp9F4;});struct Cyc_Absyn_Kind*_tmpC72=expected_kind;int _tmpC71=allow_abs_aggr;Cyc_Tcutil_check_valid_type(_tmpC75,_tmpC74,_tmpC73,_tmpC72,_tmpC71,t);});
# 4918
struct Cyc_List_List*_tmp841=({struct _RegionHandle*_tmpC77=Cyc_Core_heap_region;struct Cyc_List_List*_tmpC76=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(struct _tuple29*),struct Cyc_List_List*x))Cyc_List_map)((struct Cyc_Absyn_Tvar*(*)(struct _tuple29*))Cyc_Core_fst,_tmp840.free_vars);Cyc_Tcutil_remove_bound_tvars(_tmpC77,_tmpC76,bound_tvars);});
# 4920
struct Cyc_List_List*_tmp842=((struct Cyc_List_List*(*)(void*(*f)(struct _tuple30*),struct Cyc_List_List*x))Cyc_List_map)((void*(*)(struct _tuple30*))Cyc_Core_fst,_tmp840.free_evars);
{struct Cyc_List_List*fs=_tmp841;for(0;fs != 0;fs=fs->tl){
struct _dyneither_ptr*_tmp843=((struct Cyc_Absyn_Tvar*)fs->hd)->name;
({struct Cyc_String_pa_PrintArg_struct _tmp846=({struct Cyc_String_pa_PrintArg_struct _tmp9F3;_tmp9F3.tag=0U,_tmp9F3.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*_tmp843);_tmp9F3;});struct Cyc_String_pa_PrintArg_struct _tmp847=({struct Cyc_String_pa_PrintArg_struct _tmp9F2;_tmp9F2.tag=0U,({struct _dyneither_ptr _tmpC78=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(t));_tmp9F2.f1=_tmpC78;});_tmp9F2;});void*_tmp844[2U];_tmp844[0]=& _tmp846,_tmp844[1]=& _tmp847;({unsigned int _tmpC7A=loc;struct _dyneither_ptr _tmpC79=({const char*_tmp845="unbound type variable %s in type %s";_tag_dyneither(_tmp845,sizeof(char),36U);});Cyc_Tcutil_terr(_tmpC7A,_tmpC79,_tag_dyneither(_tmp844,sizeof(void*),2U));});});}}
# 4925
if(!allow_evars)
Cyc_Tcutil_check_free_evars(_tmp842,t,loc);}
# 4929
void Cyc_Tcutil_add_tvar_identity(struct Cyc_Absyn_Tvar*tv){
if(tv->identity == - 1)
({int _tmpC7B=Cyc_Tcutil_new_tvar_id();tv->identity=_tmpC7B;});}
# 4934
void Cyc_Tcutil_add_tvar_identities(struct Cyc_List_List*tvs){
((void(*)(void(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x))Cyc_List_iter)(Cyc_Tcutil_add_tvar_identity,tvs);}
# 4940
static void Cyc_Tcutil_check_unique_unsorted(int(*cmp)(void*,void*),struct Cyc_List_List*vs,unsigned int loc,struct _dyneither_ptr(*a2string)(void*),struct _dyneither_ptr msg){
# 4943
for(0;vs != 0;vs=vs->tl){
struct Cyc_List_List*vs2=vs->tl;for(0;vs2 != 0;vs2=vs2->tl){
if(cmp(vs->hd,vs2->hd)== 0)
({struct Cyc_String_pa_PrintArg_struct _tmp84A=({struct Cyc_String_pa_PrintArg_struct _tmp9F6;_tmp9F6.tag=0U,_tmp9F6.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)msg);_tmp9F6;});struct Cyc_String_pa_PrintArg_struct _tmp84B=({struct Cyc_String_pa_PrintArg_struct _tmp9F5;_tmp9F5.tag=0U,({struct _dyneither_ptr _tmpC7C=(struct _dyneither_ptr)((struct _dyneither_ptr)a2string(vs->hd));_tmp9F5.f1=_tmpC7C;});_tmp9F5;});void*_tmp848[2U];_tmp848[0]=& _tmp84A,_tmp848[1]=& _tmp84B;({unsigned int _tmpC7E=loc;struct _dyneither_ptr _tmpC7D=({const char*_tmp849="%s: %s";_tag_dyneither(_tmp849,sizeof(char),7U);});Cyc_Tcutil_terr(_tmpC7E,_tmpC7D,_tag_dyneither(_tmp848,sizeof(void*),2U));});});}}}
# 4949
static struct _dyneither_ptr Cyc_Tcutil_strptr2string(struct _dyneither_ptr*s){
return*s;}
# 4953
void Cyc_Tcutil_check_unique_vars(struct Cyc_List_List*vs,unsigned int loc,struct _dyneither_ptr msg){
((void(*)(int(*cmp)(struct _dyneither_ptr*,struct _dyneither_ptr*),struct Cyc_List_List*vs,unsigned int loc,struct _dyneither_ptr(*a2string)(struct _dyneither_ptr*),struct _dyneither_ptr msg))Cyc_Tcutil_check_unique_unsorted)(Cyc_strptrcmp,vs,loc,Cyc_Tcutil_strptr2string,msg);}
# 4957
void Cyc_Tcutil_check_unique_tvars(unsigned int loc,struct Cyc_List_List*tvs){
({struct Cyc_List_List*_tmpC80=tvs;unsigned int _tmpC7F=loc;((void(*)(int(*cmp)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*vs,unsigned int loc,struct _dyneither_ptr(*a2string)(struct Cyc_Absyn_Tvar*),struct _dyneither_ptr msg))Cyc_Tcutil_check_unique_unsorted)(Cyc_Absyn_tvar_cmp,_tmpC80,_tmpC7F,Cyc_Absynpp_tvar2string,({const char*_tmp84C="duplicate type variable";_tag_dyneither(_tmp84C,sizeof(char),24U);}));});}struct _tuple33{struct Cyc_Absyn_Aggrfield*f1;int f2;};struct _tuple34{struct Cyc_List_List*f1;void*f2;};struct _tuple35{struct Cyc_Absyn_Aggrfield*f1;void*f2;};
# 4971 "tcutil.cyc"
struct Cyc_List_List*Cyc_Tcutil_resolve_aggregate_designators(struct _RegionHandle*rgn,unsigned int loc,struct Cyc_List_List*des,enum Cyc_Absyn_AggrKind aggr_kind,struct Cyc_List_List*sdfields){
# 4976
struct _RegionHandle _tmp84D=_new_region("temp");struct _RegionHandle*temp=& _tmp84D;_push_region(temp);
# 4980
{struct Cyc_List_List*fields=0;
{struct Cyc_List_List*sd_fields=sdfields;for(0;sd_fields != 0;sd_fields=sd_fields->tl){
if(({struct _dyneither_ptr _tmpC81=(struct _dyneither_ptr)*((struct Cyc_Absyn_Aggrfield*)sd_fields->hd)->name;Cyc_strcmp(_tmpC81,({const char*_tmp84E="";_tag_dyneither(_tmp84E,sizeof(char),1U);}));})!= 0)
fields=({struct Cyc_List_List*_tmp850=_region_malloc(temp,sizeof(*_tmp850));({struct _tuple33*_tmpC82=({struct _tuple33*_tmp84F=_region_malloc(temp,sizeof(*_tmp84F));_tmp84F->f1=(struct Cyc_Absyn_Aggrfield*)sd_fields->hd,_tmp84F->f2=0;_tmp84F;});_tmp850->hd=_tmpC82;}),_tmp850->tl=fields;_tmp850;});}}
# 4985
fields=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(fields);{
# 4987
struct _dyneither_ptr aggr_str=(int)aggr_kind == (int)0U?({const char*_tmp875="struct";_tag_dyneither(_tmp875,sizeof(char),7U);}):({const char*_tmp876="union";_tag_dyneither(_tmp876,sizeof(char),6U);});
# 4990
struct Cyc_List_List*ans=0;
for(0;des != 0;des=des->tl){
struct _tuple34*_tmp851=(struct _tuple34*)des->hd;struct _tuple34*_tmp852=_tmp851;struct Cyc_List_List*_tmp86D;void*_tmp86C;_LL1: _tmp86D=_tmp852->f1;_tmp86C=_tmp852->f2;_LL2:;
if(_tmp86D == 0){
# 4995
struct Cyc_List_List*_tmp853=fields;
for(0;_tmp853 != 0;_tmp853=_tmp853->tl){
if(!(*((struct _tuple33*)_tmp853->hd)).f2){
(*((struct _tuple33*)_tmp853->hd)).f2=1;
({struct Cyc_List_List*_tmpC84=({struct Cyc_List_List*_tmp855=_cycalloc(sizeof(*_tmp855));({void*_tmpC83=(void*)({struct Cyc_Absyn_FieldName_Absyn_Designator_struct*_tmp854=_cycalloc(sizeof(*_tmp854));_tmp854->tag=1U,_tmp854->f1=((*((struct _tuple33*)_tmp853->hd)).f1)->name;_tmp854;});_tmp855->hd=_tmpC83;}),_tmp855->tl=0;_tmp855;});(*((struct _tuple34*)des->hd)).f1=_tmpC84;});
ans=({struct Cyc_List_List*_tmp857=_region_malloc(rgn,sizeof(*_tmp857));({struct _tuple35*_tmpC85=({struct _tuple35*_tmp856=_region_malloc(rgn,sizeof(*_tmp856));_tmp856->f1=(*((struct _tuple33*)_tmp853->hd)).f1,_tmp856->f2=_tmp86C;_tmp856;});_tmp857->hd=_tmpC85;}),_tmp857->tl=ans;_tmp857;});
break;}}
# 5003
if(_tmp853 == 0)
({struct Cyc_String_pa_PrintArg_struct _tmp85A=({struct Cyc_String_pa_PrintArg_struct _tmp9F7;_tmp9F7.tag=0U,_tmp9F7.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)aggr_str);_tmp9F7;});void*_tmp858[1U];_tmp858[0]=& _tmp85A;({unsigned int _tmpC87=loc;struct _dyneither_ptr _tmpC86=({const char*_tmp859="too many arguments to %s";_tag_dyneither(_tmp859,sizeof(char),25U);});Cyc_Tcutil_terr(_tmpC87,_tmpC86,_tag_dyneither(_tmp858,sizeof(void*),1U));});});}else{
if(_tmp86D->tl != 0)
# 5007
({void*_tmp85B=0U;({unsigned int _tmpC89=loc;struct _dyneither_ptr _tmpC88=({const char*_tmp85C="multiple designators are not yet supported";_tag_dyneither(_tmp85C,sizeof(char),43U);});Cyc_Tcutil_terr(_tmpC89,_tmpC88,_tag_dyneither(_tmp85B,sizeof(void*),0U));});});else{
# 5010
void*_tmp85D=(void*)_tmp86D->hd;void*_tmp85E=_tmp85D;struct _dyneither_ptr*_tmp86B;if(((struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct*)_tmp85E)->tag == 0U){_LL4: _LL5:
# 5012
({struct Cyc_String_pa_PrintArg_struct _tmp861=({struct Cyc_String_pa_PrintArg_struct _tmp9F8;_tmp9F8.tag=0U,_tmp9F8.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)aggr_str);_tmp9F8;});void*_tmp85F[1U];_tmp85F[0]=& _tmp861;({unsigned int _tmpC8B=loc;struct _dyneither_ptr _tmpC8A=({const char*_tmp860="array designator used in argument to %s";_tag_dyneither(_tmp860,sizeof(char),40U);});Cyc_Tcutil_terr(_tmpC8B,_tmpC8A,_tag_dyneither(_tmp85F,sizeof(void*),1U));});});
goto _LL3;}else{_LL6: _tmp86B=((struct Cyc_Absyn_FieldName_Absyn_Designator_struct*)_tmp85E)->f1;_LL7: {
# 5015
struct Cyc_List_List*_tmp862=fields;
for(0;_tmp862 != 0;_tmp862=_tmp862->tl){
if(Cyc_strptrcmp(_tmp86B,((*((struct _tuple33*)_tmp862->hd)).f1)->name)== 0){
if((*((struct _tuple33*)_tmp862->hd)).f2)
({struct Cyc_String_pa_PrintArg_struct _tmp865=({struct Cyc_String_pa_PrintArg_struct _tmp9F9;_tmp9F9.tag=0U,_tmp9F9.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*_tmp86B);_tmp9F9;});void*_tmp863[1U];_tmp863[0]=& _tmp865;({unsigned int _tmpC8D=loc;struct _dyneither_ptr _tmpC8C=({const char*_tmp864="member %s has already been used as an argument";_tag_dyneither(_tmp864,sizeof(char),47U);});Cyc_Tcutil_terr(_tmpC8D,_tmpC8C,_tag_dyneither(_tmp863,sizeof(void*),1U));});});
(*((struct _tuple33*)_tmp862->hd)).f2=1;
ans=({struct Cyc_List_List*_tmp867=_region_malloc(rgn,sizeof(*_tmp867));({struct _tuple35*_tmpC8E=({struct _tuple35*_tmp866=_region_malloc(rgn,sizeof(*_tmp866));_tmp866->f1=(*((struct _tuple33*)_tmp862->hd)).f1,_tmp866->f2=_tmp86C;_tmp866;});_tmp867->hd=_tmpC8E;}),_tmp867->tl=ans;_tmp867;});
break;}}
# 5024
if(_tmp862 == 0)
({struct Cyc_String_pa_PrintArg_struct _tmp86A=({struct Cyc_String_pa_PrintArg_struct _tmp9FA;_tmp9FA.tag=0U,_tmp9FA.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*_tmp86B);_tmp9FA;});void*_tmp868[1U];_tmp868[0]=& _tmp86A;({unsigned int _tmpC90=loc;struct _dyneither_ptr _tmpC8F=({const char*_tmp869="bad field designator %s";_tag_dyneither(_tmp869,sizeof(char),24U);});Cyc_Tcutil_terr(_tmpC90,_tmpC8F,_tag_dyneither(_tmp868,sizeof(void*),1U));});});
goto _LL3;}}_LL3:;}}}
# 5029
if((int)aggr_kind == (int)0U)
# 5031
for(0;fields != 0;fields=fields->tl){
if(!(*((struct _tuple33*)fields->hd)).f2){
({void*_tmp86E=0U;({unsigned int _tmpC92=loc;struct _dyneither_ptr _tmpC91=({const char*_tmp86F="too few arguments to struct";_tag_dyneither(_tmp86F,sizeof(char),28U);});Cyc_Tcutil_terr(_tmpC92,_tmpC91,_tag_dyneither(_tmp86E,sizeof(void*),0U));});});
break;}}else{
# 5038
int found=0;
for(0;fields != 0;fields=fields->tl){
if((*((struct _tuple33*)fields->hd)).f2){
if(found)({void*_tmp870=0U;({unsigned int _tmpC94=loc;struct _dyneither_ptr _tmpC93=({const char*_tmp871="only one member of a union is allowed";_tag_dyneither(_tmp871,sizeof(char),38U);});Cyc_Tcutil_terr(_tmpC94,_tmpC93,_tag_dyneither(_tmp870,sizeof(void*),0U));});});
found=1;}}
# 5045
if(!found)({void*_tmp872=0U;({unsigned int _tmpC96=loc;struct _dyneither_ptr _tmpC95=({const char*_tmp873="missing member for union";_tag_dyneither(_tmp873,sizeof(char),25U);});Cyc_Tcutil_terr(_tmpC96,_tmpC95,_tag_dyneither(_tmp872,sizeof(void*),0U));});});}{
# 5048
struct Cyc_List_List*_tmp874=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(ans);_npop_handler(0U);return _tmp874;};};}
# 4980
;_pop_region(temp);}
# 5054
int Cyc_Tcutil_is_zero_ptr_deref(struct Cyc_Absyn_Exp*e1,void**ptr_type,int*is_dyneither,void**elt_type){
# 5056
void*_tmp877=e1->r;void*_tmp878=_tmp877;struct Cyc_Absyn_Exp*_tmp88A;struct Cyc_Absyn_Exp*_tmp889;struct Cyc_Absyn_Exp*_tmp888;struct Cyc_Absyn_Exp*_tmp887;struct Cyc_Absyn_Exp*_tmp886;struct Cyc_Absyn_Exp*_tmp885;switch(*((int*)_tmp878)){case 14U: _LL1: _LL2:
# 5058
({struct Cyc_String_pa_PrintArg_struct _tmp87B=({struct Cyc_String_pa_PrintArg_struct _tmp9FB;_tmp9FB.tag=0U,({struct _dyneither_ptr _tmpC97=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_exp2string(e1));_tmp9FB.f1=_tmpC97;});_tmp9FB;});void*_tmp879[1U];_tmp879[0]=& _tmp87B;({struct _dyneither_ptr _tmpC98=({const char*_tmp87A="we have a cast in a lhs:  %s";_tag_dyneither(_tmp87A,sizeof(char),29U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpC98,_tag_dyneither(_tmp879,sizeof(void*),1U));});});case 20U: _LL3: _tmp885=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp878)->f1;_LL4:
 _tmp886=_tmp885;goto _LL6;case 23U: _LL5: _tmp886=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp878)->f1;_LL6:
# 5061
 return Cyc_Tcutil_is_zero_ptr_type((void*)_check_null(_tmp886->topt),ptr_type,is_dyneither,elt_type);case 22U: _LL7: _tmp887=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp878)->f1;_LL8:
 _tmp888=_tmp887;goto _LLA;case 21U: _LL9: _tmp888=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp878)->f1;_LLA:
# 5065
 if(Cyc_Tcutil_is_zero_ptr_type((void*)_check_null(_tmp888->topt),ptr_type,is_dyneither,elt_type))
({struct Cyc_String_pa_PrintArg_struct _tmp87E=({struct Cyc_String_pa_PrintArg_struct _tmp9FC;_tmp9FC.tag=0U,({
struct _dyneither_ptr _tmpC99=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_exp2string(e1));_tmp9FC.f1=_tmpC99;});_tmp9FC;});void*_tmp87C[1U];_tmp87C[0]=& _tmp87E;({struct _dyneither_ptr _tmpC9A=({const char*_tmp87D="found zero pointer aggregate member assignment: %s";_tag_dyneither(_tmp87D,sizeof(char),51U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpC9A,_tag_dyneither(_tmp87C,sizeof(void*),1U));});});
return 0;case 13U: _LLB: _tmp889=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmp878)->f1;_LLC:
 _tmp88A=_tmp889;goto _LLE;case 12U: _LLD: _tmp88A=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmp878)->f1;_LLE:
# 5071
 if(Cyc_Tcutil_is_zero_ptr_type((void*)_check_null(_tmp88A->topt),ptr_type,is_dyneither,elt_type))
({struct Cyc_String_pa_PrintArg_struct _tmp881=({struct Cyc_String_pa_PrintArg_struct _tmp9FD;_tmp9FD.tag=0U,({
struct _dyneither_ptr _tmpC9B=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_exp2string(e1));_tmp9FD.f1=_tmpC9B;});_tmp9FD;});void*_tmp87F[1U];_tmp87F[0]=& _tmp881;({struct _dyneither_ptr _tmpC9C=({const char*_tmp880="found zero pointer instantiate/noinstantiate: %s";_tag_dyneither(_tmp880,sizeof(char),49U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpC9C,_tag_dyneither(_tmp87F,sizeof(void*),1U));});});
return 0;case 1U: _LLF: _LL10:
 return 0;default: _LL11: _LL12:
({struct Cyc_String_pa_PrintArg_struct _tmp884=({struct Cyc_String_pa_PrintArg_struct _tmp9FE;_tmp9FE.tag=0U,({struct _dyneither_ptr _tmpC9D=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_exp2string(e1));_tmp9FE.f1=_tmpC9D;});_tmp9FE;});void*_tmp882[1U];_tmp882[0]=& _tmp884;({struct _dyneither_ptr _tmpC9E=({const char*_tmp883="found bad lhs in is_zero_ptr_deref: %s";_tag_dyneither(_tmp883,sizeof(char),39U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpC9E,_tag_dyneither(_tmp882,sizeof(void*),1U));});});}_LL0:;}
# 5086
int Cyc_Tcutil_is_noalias_region(void*r,int must_be_unique){
# 5089
void*_tmp88B=Cyc_Tcutil_compress(r);void*_tmp88C=_tmp88B;struct Cyc_Absyn_Tvar*_tmp898;enum Cyc_Absyn_KindQual _tmp897;enum Cyc_Absyn_AliasQual _tmp896;switch(*((int*)_tmp88C)){case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp88C)->f1)){case 7U: _LL1: _LL2:
 return !must_be_unique;case 6U: _LL3: _LL4:
 return 1;default: goto _LL9;}case 8U: if(((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp88C)->f3 != 0){if(((struct Cyc_Absyn_Typedefdecl*)((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp88C)->f3)->kind != 0){if(((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp88C)->f4 == 0){_LL5: _tmp897=((struct Cyc_Absyn_Kind*)((((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp88C)->f3)->kind)->v)->kind;_tmp896=((struct Cyc_Absyn_Kind*)((((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp88C)->f3)->kind)->v)->aliasqual;_LL6:
# 5093
 return(int)_tmp897 == (int)3U  && ((int)_tmp896 == (int)1U  || (int)_tmp896 == (int)2U  && !must_be_unique);}else{goto _LL9;}}else{goto _LL9;}}else{goto _LL9;}case 2U: _LL7: _tmp898=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp88C)->f1;_LL8: {
# 5097
struct Cyc_Absyn_Kind*_tmp88D=Cyc_Tcutil_tvar_kind(_tmp898,& Cyc_Tcutil_rk);struct Cyc_Absyn_Kind*_tmp88E=_tmp88D;enum Cyc_Absyn_KindQual _tmp895;enum Cyc_Absyn_AliasQual _tmp894;_LLC: _tmp895=_tmp88E->kind;_tmp894=_tmp88E->aliasqual;_LLD:;
if((int)_tmp895 == (int)3U  && ((int)_tmp894 == (int)1U  || (int)_tmp894 == (int)2U  && !must_be_unique)){
void*_tmp88F=Cyc_Absyn_compress_kb(_tmp898->kind);void*_tmp890=_tmp88F;struct Cyc_Core_Opt**_tmp893;if(((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp890)->tag == 2U){if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp890)->f2)->kind == Cyc_Absyn_RgnKind){if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp890)->f2)->aliasqual == Cyc_Absyn_Top){_LLF: _tmp893=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp890)->f1;_LL10:
# 5101
({struct Cyc_Core_Opt*_tmpCA0=({struct Cyc_Core_Opt*_tmp892=_cycalloc(sizeof(*_tmp892));({void*_tmpC9F=(void*)({struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_tmp891=_cycalloc(sizeof(*_tmp891));_tmp891->tag=2U,_tmp891->f1=0,_tmp891->f2=& Cyc_Tcutil_rk;_tmp891;});_tmp892->v=_tmpC9F;});_tmp892;});*_tmp893=_tmpCA0;});
return 0;}else{goto _LL11;}}else{goto _LL11;}}else{_LL11: _LL12:
 return 1;}_LLE:;}
# 5106
return 0;}default: _LL9: _LLA:
 return 0;}_LL0:;}
# 5113
int Cyc_Tcutil_is_noalias_pointer(void*t,int must_be_unique){
void*_tmp899=Cyc_Tcutil_compress(t);void*_tmp89A=_tmp899;struct Cyc_Absyn_Tvar*_tmp8A8;void*_tmp8A7;switch(*((int*)_tmp89A)){case 3U: _LL1: _tmp8A7=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp89A)->f1).ptr_atts).rgn;_LL2:
# 5116
 return Cyc_Tcutil_is_noalias_region(_tmp8A7,must_be_unique);case 2U: _LL3: _tmp8A8=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp89A)->f1;_LL4: {
# 5118
struct Cyc_Absyn_Kind*_tmp89B=Cyc_Tcutil_tvar_kind(_tmp8A8,& Cyc_Tcutil_bk);struct Cyc_Absyn_Kind*_tmp89C=_tmp89B;enum Cyc_Absyn_KindQual _tmp8A6;enum Cyc_Absyn_AliasQual _tmp8A5;_LL8: _tmp8A6=_tmp89C->kind;_tmp8A5=_tmp89C->aliasqual;_LL9:;{
enum Cyc_Absyn_KindQual _tmp89D=_tmp8A6;switch(_tmp89D){case Cyc_Absyn_BoxKind: _LLB: _LLC:
 goto _LLE;case Cyc_Absyn_AnyKind: _LLD: _LLE: goto _LL10;case Cyc_Absyn_MemKind: _LLF: _LL10:
 if((int)_tmp8A5 == (int)1U  || (int)_tmp8A5 == (int)2U  && !must_be_unique){
void*_tmp89E=Cyc_Absyn_compress_kb(_tmp8A8->kind);void*_tmp89F=_tmp89E;struct Cyc_Core_Opt**_tmp8A4;enum Cyc_Absyn_KindQual _tmp8A3;if(((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp89F)->tag == 2U){if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp89F)->f2)->aliasqual == Cyc_Absyn_Top){_LL14: _tmp8A4=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp89F)->f1;_tmp8A3=(((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp89F)->f2)->kind;_LL15:
# 5124
({struct Cyc_Core_Opt*_tmpCA3=({struct Cyc_Core_Opt*_tmp8A2=_cycalloc(sizeof(*_tmp8A2));({void*_tmpCA2=(void*)({struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_tmp8A1=_cycalloc(sizeof(*_tmp8A1));_tmp8A1->tag=2U,_tmp8A1->f1=0,({struct Cyc_Absyn_Kind*_tmpCA1=({struct Cyc_Absyn_Kind*_tmp8A0=_cycalloc(sizeof(*_tmp8A0));_tmp8A0->kind=_tmp8A3,_tmp8A0->aliasqual=Cyc_Absyn_Aliasable;_tmp8A0;});_tmp8A1->f2=_tmpCA1;});_tmp8A1;});_tmp8A2->v=_tmpCA2;});_tmp8A2;});*_tmp8A4=_tmpCA3;});
return 0;}else{goto _LL16;}}else{_LL16: _LL17:
# 5128
 return 1;}_LL13:;}
# 5131
return 0;default: _LL11: _LL12:
 return 0;}_LLA:;};}default: _LL5: _LL6:
# 5134
 return 0;}_LL0:;}
# 5137
int Cyc_Tcutil_is_noalias_pointer_or_aggr(void*t){
void*_tmp8A9=Cyc_Tcutil_compress(t);
if(Cyc_Tcutil_is_noalias_pointer(_tmp8A9,0))return 1;{
void*_tmp8AA=_tmp8A9;struct Cyc_List_List*_tmp8BE;union Cyc_Absyn_DatatypeFieldInfo _tmp8BD;struct Cyc_List_List*_tmp8BC;union Cyc_Absyn_DatatypeInfo _tmp8BB;struct Cyc_List_List*_tmp8BA;struct Cyc_Absyn_Aggrdecl**_tmp8B9;struct Cyc_List_List*_tmp8B8;struct Cyc_List_List*_tmp8B7;switch(*((int*)_tmp8AA)){case 6U: _LL1: _tmp8B7=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp8AA)->f1;_LL2:
# 5142
 while(_tmp8B7 != 0){
if(Cyc_Tcutil_is_noalias_pointer_or_aggr((*((struct _tuple12*)_tmp8B7->hd)).f2))return 1;
_tmp8B7=_tmp8B7->tl;}
# 5146
return 0;case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp8AA)->f1)){case 20U: if(((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp8AA)->f1)->f1).KnownAggr).tag == 2){_LL3: _tmp8B9=((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp8AA)->f1)->f1).KnownAggr).val;_tmp8B8=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp8AA)->f2;_LL4:
# 5148
 if((*_tmp8B9)->impl == 0)return 0;else{
# 5150
struct Cyc_List_List*_tmp8AB=((struct Cyc_List_List*(*)(struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_zip)((*_tmp8B9)->tvs,_tmp8B8);
struct Cyc_List_List*_tmp8AC=((struct Cyc_Absyn_AggrdeclImpl*)_check_null((*_tmp8B9)->impl))->fields;
void*t;
while(_tmp8AC != 0){
t=_tmp8AB == 0?((struct Cyc_Absyn_Aggrfield*)_tmp8AC->hd)->type: Cyc_Tcutil_substitute(_tmp8AB,((struct Cyc_Absyn_Aggrfield*)_tmp8AC->hd)->type);
if(Cyc_Tcutil_is_noalias_pointer_or_aggr(t))return 1;
_tmp8AC=_tmp8AC->tl;}
# 5158
return 0;}}else{_LL7: _LL8:
# 5168
 return 0;}case 18U: _LL9: _tmp8BB=((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp8AA)->f1)->f1;_tmp8BA=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp8AA)->f2;_LLA: {
# 5170
union Cyc_Absyn_DatatypeInfo _tmp8AD=_tmp8BB;struct Cyc_List_List*_tmp8B1;struct Cyc_Core_Opt*_tmp8B0;struct _tuple2*_tmp8AF;int _tmp8AE;if((_tmp8AD.UnknownDatatype).tag == 1){_LL10: _tmp8AF=((_tmp8AD.UnknownDatatype).val).name;_tmp8AE=((_tmp8AD.UnknownDatatype).val).is_extensible;_LL11:
# 5173
 return 0;}else{_LL12: _tmp8B1=(*(_tmp8AD.KnownDatatype).val)->tvs;_tmp8B0=(*(_tmp8AD.KnownDatatype).val)->fields;_LL13:
# 5176
 return 0;}_LLF:;}case 19U: _LLB: _tmp8BD=((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp8AA)->f1)->f1;_tmp8BC=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp8AA)->f2;_LLC: {
# 5179
union Cyc_Absyn_DatatypeFieldInfo _tmp8B2=_tmp8BD;struct Cyc_Absyn_Datatypedecl*_tmp8B6;struct Cyc_Absyn_Datatypefield*_tmp8B5;if((_tmp8B2.UnknownDatatypefield).tag == 1){_LL15: _LL16:
# 5182
 return 0;}else{_LL17: _tmp8B6=((_tmp8B2.KnownDatatypefield).val).f1;_tmp8B5=((_tmp8B2.KnownDatatypefield).val).f2;_LL18: {
# 5184
struct Cyc_List_List*_tmp8B3=((struct Cyc_List_List*(*)(struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_zip)(_tmp8B6->tvs,_tmp8BC);
struct Cyc_List_List*_tmp8B4=_tmp8B5->typs;
while(_tmp8B4 != 0){
_tmp8A9=_tmp8B3 == 0?(*((struct _tuple12*)_tmp8B4->hd)).f2: Cyc_Tcutil_substitute(_tmp8B3,(*((struct _tuple12*)_tmp8B4->hd)).f2);
if(Cyc_Tcutil_is_noalias_pointer_or_aggr(_tmp8A9))return 1;
_tmp8B4=_tmp8B4->tl;}
# 5191
return 0;}}_LL14:;}default: goto _LLD;}case 7U: _LL5: _tmp8BE=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp8AA)->f2;_LL6:
# 5161
 while(_tmp8BE != 0){
if(Cyc_Tcutil_is_noalias_pointer_or_aggr(((struct Cyc_Absyn_Aggrfield*)_tmp8BE->hd)->type))return 1;
_tmp8BE=_tmp8BE->tl;}
# 5165
return 0;default: _LLD: _LLE:
# 5193
 return 0;}_LL0:;};}
# 5200
int Cyc_Tcutil_is_noalias_path(struct Cyc_Absyn_Exp*e){
void*_tmp8BF=e->r;void*_tmp8C0=_tmp8BF;struct Cyc_Absyn_Stmt*_tmp8D6;struct Cyc_Absyn_Exp*_tmp8D5;struct Cyc_Absyn_Exp*_tmp8D4;struct Cyc_Absyn_Exp*_tmp8D3;struct Cyc_Absyn_Exp*_tmp8D2;struct Cyc_Absyn_Exp*_tmp8D1;struct Cyc_Absyn_Exp*_tmp8D0;struct Cyc_Absyn_Exp*_tmp8CF;struct _dyneither_ptr*_tmp8CE;struct Cyc_Absyn_Exp*_tmp8CD;struct Cyc_Absyn_Exp*_tmp8CC;switch(*((int*)_tmp8C0)){case 1U: if(((struct Cyc_Absyn_Global_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp8C0)->f1)->tag == 1U){_LL1: _LL2:
 return 0;}else{goto _LL13;}case 22U: _LL3: _tmp8CC=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp8C0)->f1;_LL4:
 _tmp8CD=_tmp8CC;goto _LL6;case 20U: _LL5: _tmp8CD=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp8C0)->f1;_LL6:
# 5205
 return Cyc_Tcutil_is_noalias_pointer((void*)_check_null(_tmp8CD->topt),1) && Cyc_Tcutil_is_noalias_path(_tmp8CD);case 21U: _LL7: _tmp8CF=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp8C0)->f1;_tmp8CE=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp8C0)->f2;_LL8:
 return Cyc_Tcutil_is_noalias_path(_tmp8CF);case 23U: _LL9: _tmp8D1=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp8C0)->f1;_tmp8D0=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp8C0)->f2;_LLA: {
# 5208
void*_tmp8C1=Cyc_Tcutil_compress((void*)_check_null(_tmp8D1->topt));void*_tmp8C2=_tmp8C1;if(((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp8C2)->tag == 6U){_LL16: _LL17:
 return Cyc_Tcutil_is_noalias_path(_tmp8D1);}else{_LL18: _LL19:
 return 0;}_LL15:;}case 6U: _LLB: _tmp8D3=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp8C0)->f2;_tmp8D2=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp8C0)->f3;_LLC:
# 5213
 return Cyc_Tcutil_is_noalias_path(_tmp8D3) && Cyc_Tcutil_is_noalias_path(_tmp8D2);case 9U: _LLD: _tmp8D4=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp8C0)->f2;_LLE:
 _tmp8D5=_tmp8D4;goto _LL10;case 14U: _LLF: _tmp8D5=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp8C0)->f2;_LL10:
 return Cyc_Tcutil_is_noalias_path(_tmp8D5);case 37U: _LL11: _tmp8D6=((struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*)_tmp8C0)->f1;_LL12:
# 5217
 while(1){
void*_tmp8C3=_tmp8D6->r;void*_tmp8C4=_tmp8C3;struct Cyc_Absyn_Exp*_tmp8CB;struct Cyc_Absyn_Decl*_tmp8CA;struct Cyc_Absyn_Stmt*_tmp8C9;struct Cyc_Absyn_Stmt*_tmp8C8;struct Cyc_Absyn_Stmt*_tmp8C7;switch(*((int*)_tmp8C4)){case 2U: _LL1B: _tmp8C8=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp8C4)->f1;_tmp8C7=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp8C4)->f2;_LL1C:
 _tmp8D6=_tmp8C7;goto _LL1A;case 12U: _LL1D: _tmp8CA=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp8C4)->f1;_tmp8C9=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp8C4)->f2;_LL1E:
 _tmp8D6=_tmp8C9;goto _LL1A;case 1U: _LL1F: _tmp8CB=((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_tmp8C4)->f1;_LL20:
 return Cyc_Tcutil_is_noalias_path(_tmp8CB);default: _LL21: _LL22:
({void*_tmp8C5=0U;({struct _dyneither_ptr _tmpCA4=({const char*_tmp8C6="is_noalias_stmt_exp: ill-formed StmtExp";_tag_dyneither(_tmp8C6,sizeof(char),40U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpCA4,_tag_dyneither(_tmp8C5,sizeof(void*),0U));});});}_LL1A:;}default: _LL13: _LL14:
# 5225
 return 1;}_LL0:;}
# 5242 "tcutil.cyc"
struct _tuple17 Cyc_Tcutil_addressof_props(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*e){
# 5244
struct _tuple17 bogus_ans=({struct _tuple17 _tmpA0B;_tmpA0B.f1=0,_tmpA0B.f2=Cyc_Absyn_heap_rgn_type;_tmpA0B;});
void*_tmp8D7=e->r;void*_tmp8D8=_tmp8D7;struct Cyc_Absyn_Exp*_tmp911;struct Cyc_Absyn_Exp*_tmp910;struct Cyc_Absyn_Exp*_tmp90F;struct Cyc_Absyn_Exp*_tmp90E;struct _dyneither_ptr*_tmp90D;int _tmp90C;struct Cyc_Absyn_Exp*_tmp90B;struct _dyneither_ptr*_tmp90A;int _tmp909;void*_tmp908;switch(*((int*)_tmp8D8)){case 1U: _LL1: _tmp908=(void*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp8D8)->f1;_LL2: {
# 5248
void*_tmp8D9=_tmp908;struct Cyc_Absyn_Vardecl*_tmp8E1;struct Cyc_Absyn_Vardecl*_tmp8E0;struct Cyc_Absyn_Vardecl*_tmp8DF;struct Cyc_Absyn_Vardecl*_tmp8DE;switch(*((int*)_tmp8D9)){case 0U: _LLE: _LLF:
 goto _LL11;case 2U: _LL10: _LL11:
 return bogus_ans;case 1U: _LL12: _tmp8DE=((struct Cyc_Absyn_Global_b_Absyn_Binding_struct*)_tmp8D9)->f1;_LL13: {
# 5252
void*_tmp8DA=Cyc_Tcutil_compress((void*)_check_null(e->topt));void*_tmp8DB=_tmp8DA;if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp8DB)->tag == 4U){_LL1B: _LL1C:
# 5254
 return({struct _tuple17 _tmp9FF;_tmp9FF.f1=1,_tmp9FF.f2=Cyc_Absyn_heap_rgn_type;_tmp9FF;});}else{_LL1D: _LL1E:
 return({struct _tuple17 _tmpA00;_tmpA00.f1=(_tmp8DE->tq).real_const,_tmpA00.f2=Cyc_Absyn_heap_rgn_type;_tmpA00;});}_LL1A:;}case 4U: _LL14: _tmp8DF=((struct Cyc_Absyn_Local_b_Absyn_Binding_struct*)_tmp8D9)->f1;_LL15: {
# 5258
void*_tmp8DC=Cyc_Tcutil_compress((void*)_check_null(e->topt));void*_tmp8DD=_tmp8DC;if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp8DD)->tag == 4U){_LL20: _LL21:
 return({struct _tuple17 _tmpA01;_tmpA01.f1=1,_tmpA01.f2=(void*)_check_null(_tmp8DF->rgn);_tmpA01;});}else{_LL22: _LL23:
# 5261
 _tmp8DF->escapes=1;
return({struct _tuple17 _tmpA02;_tmpA02.f1=(_tmp8DF->tq).real_const,_tmpA02.f2=(void*)_check_null(_tmp8DF->rgn);_tmpA02;});}_LL1F:;}case 5U: _LL16: _tmp8E0=((struct Cyc_Absyn_Pat_b_Absyn_Binding_struct*)_tmp8D9)->f1;_LL17:
# 5264
 _tmp8E1=_tmp8E0;goto _LL19;default: _LL18: _tmp8E1=((struct Cyc_Absyn_Param_b_Absyn_Binding_struct*)_tmp8D9)->f1;_LL19:
# 5266
 _tmp8E1->escapes=1;
return({struct _tuple17 _tmpA03;_tmpA03.f1=(_tmp8E1->tq).real_const,_tmpA03.f2=(void*)_check_null(_tmp8E1->rgn);_tmpA03;});}_LLD:;}case 21U: _LL3: _tmp90B=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp8D8)->f1;_tmp90A=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp8D8)->f2;_tmp909=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp8D8)->f3;_LL4:
# 5271
 if(_tmp909)return bogus_ans;{
# 5274
void*_tmp8E2=Cyc_Tcutil_compress((void*)_check_null(_tmp90B->topt));void*_tmp8E3=_tmp8E2;struct Cyc_Absyn_Aggrdecl*_tmp8EF;struct Cyc_List_List*_tmp8EE;switch(*((int*)_tmp8E3)){case 7U: _LL25: _tmp8EE=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp8E3)->f2;_LL26: {
# 5276
struct Cyc_Absyn_Aggrfield*_tmp8E4=Cyc_Absyn_lookup_field(_tmp8EE,_tmp90A);
if(_tmp8E4 != 0  && _tmp8E4->width == 0){
struct _tuple17 _tmp8E5=Cyc_Tcutil_addressof_props(te,_tmp90B);struct _tuple17 _tmp8E6=_tmp8E5;int _tmp8E8;void*_tmp8E7;_LL2C: _tmp8E8=_tmp8E6.f1;_tmp8E7=_tmp8E6.f2;_LL2D:;
return({struct _tuple17 _tmpA04;_tmpA04.f1=(_tmp8E4->tq).real_const  || _tmp8E8,_tmpA04.f2=_tmp8E7;_tmpA04;});}
# 5281
return bogus_ans;}case 0U: if(((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp8E3)->f1)->tag == 20U){if(((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp8E3)->f1)->f1).KnownAggr).tag == 2){_LL27: _tmp8EF=*((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp8E3)->f1)->f1).KnownAggr).val;_LL28: {
# 5283
struct Cyc_Absyn_Aggrfield*_tmp8E9=Cyc_Absyn_lookup_decl_field(_tmp8EF,_tmp90A);
if(_tmp8E9 != 0  && _tmp8E9->width == 0){
struct _tuple17 _tmp8EA=Cyc_Tcutil_addressof_props(te,_tmp90B);struct _tuple17 _tmp8EB=_tmp8EA;int _tmp8ED;void*_tmp8EC;_LL2F: _tmp8ED=_tmp8EB.f1;_tmp8EC=_tmp8EB.f2;_LL30:;
return({struct _tuple17 _tmpA05;_tmpA05.f1=(_tmp8E9->tq).real_const  || _tmp8ED,_tmpA05.f2=_tmp8EC;_tmpA05;});}
# 5288
return bogus_ans;}}else{goto _LL29;}}else{goto _LL29;}default: _LL29: _LL2A:
 return bogus_ans;}_LL24:;};case 22U: _LL5: _tmp90E=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp8D8)->f1;_tmp90D=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp8D8)->f2;_tmp90C=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp8D8)->f3;_LL6:
# 5293
 if(_tmp90C)return bogus_ans;{
# 5296
void*_tmp8F0=Cyc_Tcutil_compress((void*)_check_null(_tmp90E->topt));void*_tmp8F1=_tmp8F0;void*_tmp8F7;void*_tmp8F6;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp8F1)->tag == 3U){_LL32: _tmp8F7=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp8F1)->f1).elt_type;_tmp8F6=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp8F1)->f1).ptr_atts).rgn;_LL33: {
# 5298
struct Cyc_Absyn_Aggrfield*finfo;
{void*_tmp8F2=Cyc_Tcutil_compress(_tmp8F7);void*_tmp8F3=_tmp8F2;struct Cyc_Absyn_Aggrdecl*_tmp8F5;struct Cyc_List_List*_tmp8F4;switch(*((int*)_tmp8F3)){case 7U: _LL37: _tmp8F4=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp8F3)->f2;_LL38:
# 5301
 finfo=Cyc_Absyn_lookup_field(_tmp8F4,_tmp90D);goto _LL36;case 0U: if(((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp8F3)->f1)->tag == 20U){if(((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp8F3)->f1)->f1).KnownAggr).tag == 2){_LL39: _tmp8F5=*((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp8F3)->f1)->f1).KnownAggr).val;_LL3A:
# 5303
 finfo=Cyc_Absyn_lookup_decl_field(_tmp8F5,_tmp90D);goto _LL36;}else{goto _LL3B;}}else{goto _LL3B;}default: _LL3B: _LL3C:
 return bogus_ans;}_LL36:;}
# 5306
if(finfo != 0  && finfo->width == 0)
return({struct _tuple17 _tmpA06;_tmpA06.f1=(finfo->tq).real_const,_tmpA06.f2=_tmp8F6;_tmpA06;});
return bogus_ans;}}else{_LL34: _LL35:
 return bogus_ans;}_LL31:;};case 20U: _LL7: _tmp90F=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp8D8)->f1;_LL8: {
# 5313
void*_tmp8F8=Cyc_Tcutil_compress((void*)_check_null(_tmp90F->topt));void*_tmp8F9=_tmp8F8;struct Cyc_Absyn_Tqual _tmp8FB;void*_tmp8FA;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp8F9)->tag == 3U){_LL3E: _tmp8FB=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp8F9)->f1).elt_tq;_tmp8FA=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp8F9)->f1).ptr_atts).rgn;_LL3F:
# 5315
 return({struct _tuple17 _tmpA07;_tmpA07.f1=_tmp8FB.real_const,_tmpA07.f2=_tmp8FA;_tmpA07;});}else{_LL40: _LL41:
 return bogus_ans;}_LL3D:;}case 23U: _LL9: _tmp911=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp8D8)->f1;_tmp910=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp8D8)->f2;_LLA: {
# 5321
void*t=Cyc_Tcutil_compress((void*)_check_null(_tmp911->topt));
void*_tmp8FC=t;struct Cyc_Absyn_Tqual _tmp905;struct Cyc_Absyn_Tqual _tmp904;void*_tmp903;struct Cyc_List_List*_tmp902;switch(*((int*)_tmp8FC)){case 6U: _LL43: _tmp902=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp8FC)->f1;_LL44: {
# 5325
struct _tuple13 _tmp8FD=Cyc_Evexp_eval_const_uint_exp(_tmp910);struct _tuple13 _tmp8FE=_tmp8FD;unsigned int _tmp901;int _tmp900;_LL4C: _tmp901=_tmp8FE.f1;_tmp900=_tmp8FE.f2;_LL4D:;
if(!_tmp900)
return bogus_ans;{
struct _tuple12*_tmp8FF=Cyc_Absyn_lookup_tuple_field(_tmp902,(int)_tmp901);
if(_tmp8FF != 0)
return({struct _tuple17 _tmpA08;_tmpA08.f1=((*_tmp8FF).f1).real_const,({void*_tmpCA5=(Cyc_Tcutil_addressof_props(te,_tmp911)).f2;_tmpA08.f2=_tmpCA5;});_tmpA08;});
return bogus_ans;};}case 3U: _LL45: _tmp904=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp8FC)->f1).elt_tq;_tmp903=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp8FC)->f1).ptr_atts).rgn;_LL46:
# 5333
 return({struct _tuple17 _tmpA09;_tmpA09.f1=_tmp904.real_const,_tmpA09.f2=_tmp903;_tmpA09;});case 4U: _LL47: _tmp905=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp8FC)->f1).tq;_LL48:
# 5339
 return({struct _tuple17 _tmpA0A;_tmpA0A.f1=_tmp905.real_const,({void*_tmpCA6=(Cyc_Tcutil_addressof_props(te,_tmp911)).f2;_tmpA0A.f2=_tmpCA6;});_tmpA0A;});default: _LL49: _LL4A:
 return bogus_ans;}_LL42:;}default: _LLB: _LLC:
# 5343
({void*_tmp906=0U;({unsigned int _tmpCA8=e->loc;struct _dyneither_ptr _tmpCA7=({const char*_tmp907="unary & applied to non-lvalue";_tag_dyneither(_tmp907,sizeof(char),30U);});Cyc_Tcutil_terr(_tmpCA8,_tmpCA7,_tag_dyneither(_tmp906,sizeof(void*),0U));});});
return bogus_ans;}_LL0:;}
# 5350
void Cyc_Tcutil_check_bound(unsigned int loc,unsigned int i,void*b,int do_warn){
struct Cyc_Absyn_Exp*_tmp912=({void*_tmpCA9=Cyc_Absyn_bounds_one();Cyc_Tcutil_get_bounds_exp(_tmpCA9,b);});
if(_tmp912 == 0)return;{
struct Cyc_Absyn_Exp*_tmp913=_tmp912;
struct _tuple13 _tmp914=Cyc_Evexp_eval_const_uint_exp(_tmp913);struct _tuple13 _tmp915=_tmp914;unsigned int _tmp91F;int _tmp91E;_LL1: _tmp91F=_tmp915.f1;_tmp91E=_tmp915.f2;_LL2:;
if(_tmp91E  && _tmp91F <= i){
if(do_warn)
({struct Cyc_Int_pa_PrintArg_struct _tmp918=({struct Cyc_Int_pa_PrintArg_struct _tmpA0D;_tmpA0D.tag=1U,_tmpA0D.f1=(unsigned long)((int)_tmp91F);_tmpA0D;});struct Cyc_Int_pa_PrintArg_struct _tmp919=({struct Cyc_Int_pa_PrintArg_struct _tmpA0C;_tmpA0C.tag=1U,_tmpA0C.f1=(unsigned long)((int)i);_tmpA0C;});void*_tmp916[2U];_tmp916[0]=& _tmp918,_tmp916[1]=& _tmp919;({unsigned int _tmpCAB=loc;struct _dyneither_ptr _tmpCAA=({const char*_tmp917="a dereference will be out of bounds: %d <= %d";_tag_dyneither(_tmp917,sizeof(char),46U);});Cyc_Tcutil_warn(_tmpCAB,_tmpCAA,_tag_dyneither(_tmp916,sizeof(void*),2U));});});else{
# 5359
({struct Cyc_Int_pa_PrintArg_struct _tmp91C=({struct Cyc_Int_pa_PrintArg_struct _tmpA0F;_tmpA0F.tag=1U,_tmpA0F.f1=(unsigned long)((int)_tmp91F);_tmpA0F;});struct Cyc_Int_pa_PrintArg_struct _tmp91D=({struct Cyc_Int_pa_PrintArg_struct _tmpA0E;_tmpA0E.tag=1U,_tmpA0E.f1=(unsigned long)((int)i);_tmpA0E;});void*_tmp91A[2U];_tmp91A[0]=& _tmp91C,_tmp91A[1]=& _tmp91D;({unsigned int _tmpCAD=loc;struct _dyneither_ptr _tmpCAC=({const char*_tmp91B="dereference is out of bounds: %d <= %d";_tag_dyneither(_tmp91B,sizeof(char),39U);});Cyc_Tcutil_terr(_tmpCAD,_tmpCAC,_tag_dyneither(_tmp91A,sizeof(void*),2U));});});}}
return;};}
# 5363
void Cyc_Tcutil_check_nonzero_bound(unsigned int loc,void*b){
Cyc_Tcutil_check_bound(loc,0U,b,0);}
# 5372
static int Cyc_Tcutil_cnst_exp(int var_okay,struct Cyc_Absyn_Exp*e){
void*_tmp920=e->r;void*_tmp921=_tmp920;struct Cyc_List_List*_tmp940;struct Cyc_List_List*_tmp93F;enum Cyc_Absyn_Primop _tmp93E;struct Cyc_List_List*_tmp93D;struct Cyc_List_List*_tmp93C;struct Cyc_List_List*_tmp93B;struct Cyc_List_List*_tmp93A;struct Cyc_Absyn_Exp*_tmp939;struct Cyc_Absyn_Exp*_tmp938;struct Cyc_Absyn_Exp*_tmp937;struct Cyc_Absyn_Exp*_tmp936;void*_tmp935;struct Cyc_Absyn_Exp*_tmp934;void*_tmp933;struct Cyc_Absyn_Exp*_tmp932;struct Cyc_Absyn_Exp*_tmp931;struct Cyc_Absyn_Exp*_tmp930;struct Cyc_Absyn_Exp*_tmp92F;struct Cyc_Absyn_Exp*_tmp92E;struct Cyc_Absyn_Exp*_tmp92D;struct Cyc_Absyn_Exp*_tmp92C;struct Cyc_Absyn_Exp*_tmp92B;struct Cyc_Absyn_Exp*_tmp92A;void*_tmp929;switch(*((int*)_tmp921)){case 0U: _LL1: _LL2:
 goto _LL4;case 2U: _LL3: _LL4:
 goto _LL6;case 17U: _LL5: _LL6:
 goto _LL8;case 18U: _LL7: _LL8:
 goto _LLA;case 19U: _LL9: _LLA:
 goto _LLC;case 32U: _LLB: _LLC:
 goto _LLE;case 33U: _LLD: _LLE:
 return 1;case 1U: _LLF: _tmp929=(void*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp921)->f1;_LL10: {
# 5384
void*_tmp922=_tmp929;struct Cyc_Absyn_Vardecl*_tmp928;struct Cyc_Absyn_Vardecl*_tmp927;switch(*((int*)_tmp922)){case 2U: _LL34: _LL35:
 return 1;case 1U: _LL36: _tmp927=((struct Cyc_Absyn_Global_b_Absyn_Binding_struct*)_tmp922)->f1;_LL37: {
# 5387
void*_tmp923=Cyc_Tcutil_compress(_tmp927->type);void*_tmp924=_tmp923;switch(*((int*)_tmp924)){case 4U: _LL3F: _LL40:
 goto _LL42;case 5U: _LL41: _LL42:
 return 1;default: _LL43: _LL44:
 return var_okay;}_LL3E:;}case 4U: _LL38: _tmp928=((struct Cyc_Absyn_Local_b_Absyn_Binding_struct*)_tmp922)->f1;_LL39:
# 5394
 if((int)_tmp928->sc == (int)Cyc_Absyn_Static){
void*_tmp925=Cyc_Tcutil_compress(_tmp928->type);void*_tmp926=_tmp925;switch(*((int*)_tmp926)){case 4U: _LL46: _LL47:
 goto _LL49;case 5U: _LL48: _LL49:
 return 1;default: _LL4A: _LL4B:
 return var_okay;}_LL45:;}else{
# 5401
return var_okay;}case 0U: _LL3A: _LL3B:
 return 0;default: _LL3C: _LL3D:
 return var_okay;}_LL33:;}case 6U: _LL11: _tmp92C=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp921)->f1;_tmp92B=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp921)->f2;_tmp92A=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp921)->f3;_LL12:
# 5407
 return(Cyc_Tcutil_cnst_exp(0,_tmp92C) && 
Cyc_Tcutil_cnst_exp(0,_tmp92B)) && 
Cyc_Tcutil_cnst_exp(0,_tmp92A);case 9U: _LL13: _tmp92E=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp921)->f1;_tmp92D=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp921)->f2;_LL14:
# 5411
 return Cyc_Tcutil_cnst_exp(0,_tmp92E) && Cyc_Tcutil_cnst_exp(0,_tmp92D);case 41U: _LL15: _tmp92F=((struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*)_tmp921)->f1;_LL16:
 _tmp930=_tmp92F;goto _LL18;case 12U: _LL17: _tmp930=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmp921)->f1;_LL18:
 _tmp931=_tmp930;goto _LL1A;case 13U: _LL19: _tmp931=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmp921)->f1;_LL1A:
# 5415
 return Cyc_Tcutil_cnst_exp(var_okay,_tmp931);case 14U: if(((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp921)->f4 == Cyc_Absyn_No_coercion){_LL1B: _tmp933=(void*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp921)->f1;_tmp932=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp921)->f2;_LL1C:
# 5417
 return Cyc_Tcutil_cnst_exp(var_okay,_tmp932);}else{_LL1D: _tmp935=(void*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp921)->f1;_tmp934=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp921)->f2;_LL1E:
# 5420
 return Cyc_Tcutil_cnst_exp(var_okay,_tmp934);}case 15U: _LL1F: _tmp936=((struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_tmp921)->f1;_LL20:
# 5422
 return Cyc_Tcutil_cnst_exp(1,_tmp936);case 27U: _LL21: _tmp938=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp921)->f2;_tmp937=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp921)->f3;_LL22:
# 5424
 return Cyc_Tcutil_cnst_exp(0,_tmp938) && Cyc_Tcutil_cnst_exp(0,_tmp937);case 28U: _LL23: _tmp939=((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmp921)->f1;_LL24:
# 5426
 return Cyc_Tcutil_cnst_exp(0,_tmp939);case 26U: _LL25: _tmp93A=((struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_tmp921)->f1;_LL26:
 _tmp93B=_tmp93A;goto _LL28;case 30U: _LL27: _tmp93B=((struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*)_tmp921)->f2;_LL28:
 _tmp93C=_tmp93B;goto _LL2A;case 29U: _LL29: _tmp93C=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp921)->f3;_LL2A:
# 5430
 for(0;_tmp93C != 0;_tmp93C=_tmp93C->tl){
if(!Cyc_Tcutil_cnst_exp(0,(*((struct _tuple18*)_tmp93C->hd)).f2))
return 0;}
return 1;case 3U: _LL2B: _tmp93E=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp921)->f1;_tmp93D=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp921)->f2;_LL2C:
# 5435
 _tmp93F=_tmp93D;goto _LL2E;case 24U: _LL2D: _tmp93F=((struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct*)_tmp921)->f1;_LL2E:
 _tmp940=_tmp93F;goto _LL30;case 31U: _LL2F: _tmp940=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmp921)->f1;_LL30:
# 5438
 for(0;_tmp940 != 0;_tmp940=_tmp940->tl){
if(!Cyc_Tcutil_cnst_exp(0,(struct Cyc_Absyn_Exp*)_tmp940->hd))
return 0;}
return 1;default: _LL31: _LL32:
 return 0;}_LL0:;}
# 5446
int Cyc_Tcutil_is_const_exp(struct Cyc_Absyn_Exp*e){
return Cyc_Tcutil_cnst_exp(0,e);}
# 5450
static int Cyc_Tcutil_fields_zeroable(struct Cyc_List_List*tvs,struct Cyc_List_List*ts,struct Cyc_List_List*fs);
# 5452
int Cyc_Tcutil_zeroable_type(void*t){
void*_tmp941=Cyc_Tcutil_compress(t);void*_tmp942=_tmp941;struct Cyc_List_List*_tmp953;struct Cyc_List_List*_tmp952;void*_tmp951;void*_tmp950;void*_tmp94F;void*_tmp94E;struct Cyc_List_List*_tmp94D;switch(*((int*)_tmp942)){case 0U: _LL1: _tmp94E=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp942)->f1;_tmp94D=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp942)->f2;_LL2: {
# 5455
void*_tmp943=_tmp94E;union Cyc_Absyn_AggrInfo _tmp94C;struct Cyc_List_List*_tmp94B;struct Cyc_Absyn_Enumdecl*_tmp94A;switch(*((int*)_tmp943)){case 0U: _LLE: _LLF:
 goto _LL11;case 1U: _LL10: _LL11:
 goto _LL13;case 2U: _LL12: _LL13:
 return 1;case 15U: _LL14: _tmp94A=((struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*)_tmp943)->f2;_LL15:
# 5461
 if(_tmp94A == 0  || _tmp94A->fields == 0)
return 0;
_tmp94B=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(_tmp94A->fields))->v;goto _LL17;case 16U: _LL16: _tmp94B=((struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct*)_tmp943)->f1;_LL17:
# 5465
{struct Cyc_List_List*_tmp944=_tmp94B;for(0;_tmp944 != 0;_tmp944=_tmp944->tl){
if(((struct Cyc_Absyn_Enumfield*)_tmp944->hd)->tag == 0)
return _tmp944 == _tmp94B;{
struct _tuple13 _tmp945=Cyc_Evexp_eval_const_uint_exp((struct Cyc_Absyn_Exp*)_check_null(((struct Cyc_Absyn_Enumfield*)_tmp944->hd)->tag));struct _tuple13 _tmp946=_tmp945;unsigned int _tmp948;int _tmp947;_LL1D: _tmp948=_tmp946.f1;_tmp947=_tmp946.f2;_LL1E:;
if(_tmp947  && _tmp948 == (unsigned int)0)
return 1;};}}
# 5472
return 0;case 20U: _LL18: _tmp94C=((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_tmp943)->f1;_LL19: {
# 5476
struct Cyc_Absyn_Aggrdecl*_tmp949=Cyc_Absyn_get_known_aggrdecl(_tmp94C);
if(_tmp949->impl == 0)return 0;
if(((struct Cyc_Absyn_AggrdeclImpl*)_check_null(_tmp949->impl))->exist_vars != 0)return 0;
return Cyc_Tcutil_fields_zeroable(_tmp949->tvs,_tmp94D,((struct Cyc_Absyn_AggrdeclImpl*)_check_null(_tmp949->impl))->fields);}default: _LL1A: _LL1B:
 return 0;}_LLD:;}case 3U: _LL3: _tmp950=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp942)->f1).ptr_atts).nullable;_tmp94F=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp942)->f1).ptr_atts).bounds;_LL4:
# 5483
 return Cyc_Tcutil_unify(_tmp94F,Cyc_Absyn_fat_bound_type) || Cyc_Tcutil_force_type2bool(1,_tmp950);case 4U: _LL5: _tmp951=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp942)->f1).elt_type;_LL6:
 return Cyc_Tcutil_zeroable_type(_tmp951);case 6U: _LL7: _tmp952=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp942)->f1;_LL8:
# 5486
 for(0;_tmp952 != 0;_tmp952=_tmp952->tl){
if(!Cyc_Tcutil_zeroable_type((*((struct _tuple12*)_tmp952->hd)).f2))return 0;}
return 1;case 7U: _LL9: _tmp953=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp942)->f2;_LLA:
 return Cyc_Tcutil_fields_zeroable(0,0,_tmp953);default: _LLB: _LLC:
 return 0;}_LL0:;}
# 5493
static int Cyc_Tcutil_fields_zeroable(struct Cyc_List_List*tvs,struct Cyc_List_List*ts,struct Cyc_List_List*fs){
# 5495
struct _RegionHandle _tmp954=_new_region("rgn");struct _RegionHandle*rgn=& _tmp954;_push_region(rgn);
{struct Cyc_List_List*_tmp955=((struct Cyc_List_List*(*)(struct _RegionHandle*r1,struct _RegionHandle*r2,struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_rzip)(rgn,rgn,tvs,ts);
for(0;fs != 0;fs=fs->tl){
void*t=((struct Cyc_Absyn_Aggrfield*)fs->hd)->type;
if(Cyc_Tcutil_zeroable_type(t))continue;
t=Cyc_Tcutil_rsubstitute(rgn,_tmp955,((struct Cyc_Absyn_Aggrfield*)fs->hd)->type);
if(!Cyc_Tcutil_zeroable_type(t)){int _tmp956=0;_npop_handler(0U);return _tmp956;}}{
# 5503
int _tmp957=1;_npop_handler(0U);return _tmp957;};}
# 5496
;_pop_region(rgn);}
# 5507
void Cyc_Tcutil_check_no_qual(unsigned int loc,void*t){
void*_tmp958=t;struct Cyc_Absyn_Typedefdecl*_tmp95D;if(((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp958)->tag == 8U){_LL1: _tmp95D=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp958)->f3;_LL2:
# 5510
 if(_tmp95D != 0){
struct Cyc_Absyn_Tqual _tmp959=_tmp95D->tq;
if(((_tmp959.print_const  || _tmp959.q_volatile) || _tmp959.q_restrict) || _tmp959.real_const)
# 5515
({struct Cyc_String_pa_PrintArg_struct _tmp95C=({struct Cyc_String_pa_PrintArg_struct _tmpA10;_tmpA10.tag=0U,({struct _dyneither_ptr _tmpCAE=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(t));_tmpA10.f1=_tmpCAE;});_tmpA10;});void*_tmp95A[1U];_tmp95A[0]=& _tmp95C;({unsigned int _tmpCB0=loc;struct _dyneither_ptr _tmpCAF=({const char*_tmp95B="qualifier within typedef type %s is ignored";_tag_dyneither(_tmp95B,sizeof(char),44U);});Cyc_Tcutil_warn(_tmpCB0,_tmpCAF,_tag_dyneither(_tmp95A,sizeof(void*),1U));});});}
# 5518
goto _LL0;}else{_LL3: _LL4:
 goto _LL0;}_LL0:;}
# 5526
struct Cyc_List_List*Cyc_Tcutil_transfer_fn_type_atts(void*t,struct Cyc_List_List*atts){
void*_tmp95E=Cyc_Tcutil_compress(t);void*_tmp95F=_tmp95E;struct Cyc_List_List**_tmp965;if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp95F)->tag == 5U){_LL1: _tmp965=(struct Cyc_List_List**)&(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp95F)->f1).attributes;_LL2: {
# 5529
struct Cyc_List_List*_tmp960=0;
for(0;atts != 0;atts=atts->tl){
if(Cyc_Absyn_fntype_att((void*)atts->hd)){
if(!((int(*)(int(*compare)(void*,void*),struct Cyc_List_List*l,void*x))Cyc_List_mem)(Cyc_Tcutil_attribute_cmp,*_tmp965,(void*)atts->hd))
({struct Cyc_List_List*_tmpCB1=({struct Cyc_List_List*_tmp961=_cycalloc(sizeof(*_tmp961));_tmp961->hd=(void*)atts->hd,_tmp961->tl=*_tmp965;_tmp961;});*_tmp965=_tmpCB1;});}else{
# 5536
_tmp960=({struct Cyc_List_List*_tmp962=_cycalloc(sizeof(*_tmp962));_tmp962->hd=(void*)atts->hd,_tmp962->tl=_tmp960;_tmp962;});}}
return _tmp960;}}else{_LL3: _LL4:
({void*_tmp963=0U;({struct _dyneither_ptr _tmpCB2=({const char*_tmp964="transfer_fn_type_atts";_tag_dyneither(_tmp964,sizeof(char),22U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpCB2,_tag_dyneither(_tmp963,sizeof(void*),0U));});});}_LL0:;}
# 5543
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_type_bound(void*t){
void*_tmp966=Cyc_Tcutil_compress(t);void*_tmp967=_tmp966;struct Cyc_Absyn_Exp*_tmp969;struct Cyc_Absyn_PtrInfo*_tmp968;switch(*((int*)_tmp967)){case 3U: _LL1: _tmp968=(struct Cyc_Absyn_PtrInfo*)&((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp967)->f1;_LL2:
# 5546
 return({void*_tmpCB3=Cyc_Absyn_bounds_one();Cyc_Tcutil_get_bounds_exp(_tmpCB3,(_tmp968->ptr_atts).bounds);});case 4U: _LL3: _tmp969=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp967)->f1).num_elts;_LL4:
# 5548
 return _tmp969;default: _LL5: _LL6:
 return 0;}_LL0:;}
# 5555
struct Cyc_Absyn_Vardecl*Cyc_Tcutil_nonesc_vardecl(void*b){
{void*_tmp96A=b;struct Cyc_Absyn_Vardecl*_tmp96E;struct Cyc_Absyn_Vardecl*_tmp96D;struct Cyc_Absyn_Vardecl*_tmp96C;struct Cyc_Absyn_Vardecl*_tmp96B;switch(*((int*)_tmp96A)){case 5U: _LL1: _tmp96B=((struct Cyc_Absyn_Pat_b_Absyn_Binding_struct*)_tmp96A)->f1;_LL2:
 _tmp96C=_tmp96B;goto _LL4;case 4U: _LL3: _tmp96C=((struct Cyc_Absyn_Local_b_Absyn_Binding_struct*)_tmp96A)->f1;_LL4:
 _tmp96D=_tmp96C;goto _LL6;case 3U: _LL5: _tmp96D=((struct Cyc_Absyn_Param_b_Absyn_Binding_struct*)_tmp96A)->f1;_LL6:
 _tmp96E=_tmp96D;goto _LL8;case 1U: _LL7: _tmp96E=((struct Cyc_Absyn_Global_b_Absyn_Binding_struct*)_tmp96A)->f1;_LL8:
# 5561
 if(!_tmp96E->escapes)return _tmp96E;
goto _LL0;default: _LL9: _LLA:
 goto _LL0;}_LL0:;}
# 5565
return 0;}
# 5569
struct Cyc_List_List*Cyc_Tcutil_filter_nulls(struct Cyc_List_List*l){
struct Cyc_List_List*_tmp96F=0;
{struct Cyc_List_List*x=l;for(0;x != 0;x=x->tl){
if((void**)x->hd != 0)_tmp96F=({struct Cyc_List_List*_tmp970=_cycalloc(sizeof(*_tmp970));_tmp970->hd=*((void**)_check_null((void**)x->hd)),_tmp970->tl=_tmp96F;_tmp970;});}}
return _tmp96F;}
# 5576
void*Cyc_Tcutil_promote_array(void*t,void*rgn,int convert_tag){
void*_tmp971=Cyc_Tcutil_compress(t);void*_tmp972=_tmp971;void*_tmp97C;struct Cyc_Absyn_Tqual _tmp97B;struct Cyc_Absyn_Exp*_tmp97A;void*_tmp979;unsigned int _tmp978;if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp972)->tag == 4U){_LL1: _tmp97C=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp972)->f1).elt_type;_tmp97B=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp972)->f1).tq;_tmp97A=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp972)->f1).num_elts;_tmp979=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp972)->f1).zero_term;_tmp978=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp972)->f1).zt_loc;_LL2: {
# 5579
void*b;
if(_tmp97A == 0)
b=Cyc_Absyn_fat_bound_type;else{
# 5583
struct Cyc_Absyn_Exp*bnd=_tmp97A;
if(convert_tag){
if(bnd->topt == 0)
({void*_tmp973=0U;({struct _dyneither_ptr _tmpCB4=({const char*_tmp974="cannot convert tag without type!";_tag_dyneither(_tmp974,sizeof(char),33U);});((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(_tmpCB4,_tag_dyneither(_tmp973,sizeof(void*),0U));});});{
void*_tmp975=Cyc_Tcutil_compress((void*)_check_null(bnd->topt));void*_tmp976=_tmp975;void*_tmp977;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp976)->tag == 0U){if(((struct Cyc_Absyn_TagCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp976)->f1)->tag == 4U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp976)->f2 != 0){_LL6: _tmp977=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp976)->f2)->hd;_LL7:
# 5589
 b=Cyc_Absyn_thin_bounds_exp(({void*_tmpCB5=Cyc_Absyn_uint_type;Cyc_Absyn_cast_exp(_tmpCB5,Cyc_Absyn_valueof_exp(_tmp977,0U),0,Cyc_Absyn_No_coercion,0U);}));
goto _LL5;}else{goto _LL8;}}else{goto _LL8;}}else{_LL8: _LL9:
# 5592
 if(Cyc_Tcutil_is_const_exp(bnd))
b=Cyc_Absyn_thin_bounds_exp(bnd);else{
# 5595
b=Cyc_Absyn_fat_bound_type;}}_LL5:;};}else{
# 5599
b=Cyc_Absyn_thin_bounds_exp(bnd);}}
# 5601
return Cyc_Absyn_atb_type(_tmp97C,rgn,_tmp97B,b,_tmp979);}}else{_LL3: _LL4:
 return t;}_LL0:;}
