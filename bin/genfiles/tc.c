#include <setjmp.h>
/* This is a C header file to be used by the output of the Cyclone to
   C translator.  The corresponding definitions are in file lib/runtime_*.c */
#ifndef _CYC_INCLUDE_H_
#define _CYC_INCLUDE_H_

/* Need one of these per thread (see runtime_stack.c). The runtime maintains 
   a stack that contains either _handler_cons structs or _RegionHandle structs.
   The tag is 0 for a handler_cons and 1 for a region handle.  */
struct _RuntimeStack {
  int tag; 
  struct _RuntimeStack *next;
  void (*cleanup)(struct _RuntimeStack *frame);
};

#ifndef offsetof
/* should be size_t, but int is fine. */
#define offsetof(t,n) ((int)(&(((t *)0)->n)))
#endif

/* Fat pointers */
struct _fat_ptr {
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
  struct _RegionPage *next;
  char data[1];
}
#endif
; // abstract -- defined in runtime_memory.c
struct _pool;
struct _RegionHandle {
  struct _RuntimeStack s;
  struct _RegionPage *curr;
  char               *offset;
  char               *last_plus_one;
  struct _DynRegionHandle *sub_regions;
  struct _pool *released_ptrs;
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
// A dynamic region is just a region handle.  The wrapper struct is for type
// abstraction.
struct Cyc_Core_DynamicRegion {
  struct _RegionHandle h;
};

struct _RegionHandle _new_region(const char*);
void* _region_malloc(struct _RegionHandle*, unsigned);
void* _region_calloc(struct _RegionHandle*, unsigned t, unsigned n);
void   _free_region(struct _RegionHandle*);
struct _RegionHandle*_open_dynregion(struct _DynRegionFrame*,struct _DynRegionHandle*);
void   _pop_dynregion();

/* Exceptions */
struct _handler_cons {
  struct _RuntimeStack s;
  jmp_buf handler;
};
void _push_handler(struct _handler_cons *);
void _push_region(struct _RegionHandle *);
void _npop_handler(int);
void _pop_handler();
void _pop_region();

#ifndef _throw
void* _throw_null_fn(const char*,unsigned);
void* _throw_arraybounds_fn(const char*,unsigned);
void* _throw_badalloc_fn(const char*,unsigned);
void* _throw_match_fn(const char*,unsigned);
void* _throw_fn(void*,const char*,unsigned);
void* _rethrow(void*);
#define _throw_null() (_throw_null_fn(__FILE__,__LINE__))
#define _throw_arraybounds() (_throw_arraybounds_fn(__FILE__,__LINE__))
#define _throw_badalloc() (_throw_badalloc_fn(__FILE__,__LINE__))
#define _throw_match() (_throw_match_fn(__FILE__,__LINE__))
#define _throw(e) (_throw_fn((e),__FILE__,__LINE__))
#endif

struct _xtunion_struct* Cyc_Core_get_exn_thrown();
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
#define _check_known_subscript_notnull(ptr,bound,elt_sz,index)\
   (((char*)ptr) + (elt_sz)*(index))
#ifdef NO_CYC_NULL_CHECKS
#define _check_known_subscript_null _check_known_subscript_notnull
#else
#define _check_known_subscript_null(ptr,bound,elt_sz,index) ({ \
  char*_cks_ptr = (char*)(ptr);\
  int _index = (index);\
  if (!_cks_ptr) _throw_null(); \
  _cks_ptr + (elt_sz)*_index; })
#endif
#define _zero_arr_plus_fn(orig_x,orig_sz,orig_i,f,l) ((orig_x)+(orig_i))
#define _zero_arr_plus_char_fn _zero_arr_plus_fn
#define _zero_arr_plus_short_fn _zero_arr_plus_fn
#define _zero_arr_plus_int_fn _zero_arr_plus_fn
#define _zero_arr_plus_float_fn _zero_arr_plus_fn
#define _zero_arr_plus_double_fn _zero_arr_plus_fn
#define _zero_arr_plus_longdouble_fn _zero_arr_plus_fn
#define _zero_arr_plus_voidstar_fn _zero_arr_plus_fn
#else
#define _check_known_subscript_null(ptr,bound,elt_sz,index) ({ \
  char*_cks_ptr = (char*)(ptr); \
  unsigned _cks_index = (index); \
  if (!_cks_ptr) _throw_null(); \
  if (_cks_index >= (bound)) _throw_arraybounds(); \
  _cks_ptr + (elt_sz)*_cks_index; })
#define _check_known_subscript_notnull(ptr,bound,elt_sz,index) ({ \
  char*_cks_ptr = (char*)(ptr); \
  unsigned _cks_index = (index); \
  if (_cks_index >= (bound)) _throw_arraybounds(); \
  _cks_ptr + (elt_sz)*_cks_index; })

/* _zero_arr_plus_*_fn(x,sz,i,filename,lineno) adds i to zero-terminated ptr
   x that has at least sz elements */
char* _zero_arr_plus_char_fn(char*,unsigned,int,const char*,unsigned);
short* _zero_arr_plus_short_fn(short*,unsigned,int,const char*,unsigned);
int* _zero_arr_plus_int_fn(int*,unsigned,int,const char*,unsigned);
float* _zero_arr_plus_float_fn(float*,unsigned,int,const char*,unsigned);
double* _zero_arr_plus_double_fn(double*,unsigned,int,const char*,unsigned);
long double* _zero_arr_plus_longdouble_fn(long double*,unsigned,int,const char*, unsigned);
void** _zero_arr_plus_voidstar_fn(void**,unsigned,int,const char*,unsigned);
#endif

/* _get_zero_arr_size_*(x,sz) returns the number of elements in a
   zero-terminated array that is NULL or has at least sz elements */
int _get_zero_arr_size_char(const char*,unsigned);
int _get_zero_arr_size_short(const short*,unsigned);
int _get_zero_arr_size_int(const int*,unsigned);
int _get_zero_arr_size_float(const float*,unsigned);
int _get_zero_arr_size_double(const double*,unsigned);
int _get_zero_arr_size_longdouble(const long double*,unsigned);
int _get_zero_arr_size_voidstar(const void**,unsigned);

/* _zero_arr_inplace_plus_*_fn(x,i,filename,lineno) sets
   zero-terminated pointer *x to *x + i */
char* _zero_arr_inplace_plus_char_fn(char**,int,const char*,unsigned);
short* _zero_arr_inplace_plus_short_fn(short**,int,const char*,unsigned);
int* _zero_arr_inplace_plus_int(int**,int,const char*,unsigned);
float* _zero_arr_inplace_plus_float_fn(float**,int,const char*,unsigned);
double* _zero_arr_inplace_plus_double_fn(double**,int,const char*,unsigned);
long double* _zero_arr_inplace_plus_longdouble_fn(long double**,int,const char*,unsigned);
void** _zero_arr_inplace_plus_voidstar_fn(void***,int,const char*,unsigned);
/* like the previous functions, but does post-addition (as in e++) */
char* _zero_arr_inplace_plus_post_char_fn(char**,int,const char*,unsigned);
short* _zero_arr_inplace_plus_post_short_fn(short**x,int,const char*,unsigned);
int* _zero_arr_inplace_plus_post_int_fn(int**,int,const char*,unsigned);
float* _zero_arr_inplace_plus_post_float_fn(float**,int,const char*,unsigned);
double* _zero_arr_inplace_plus_post_double_fn(double**,int,const char*,unsigned);
long double* _zero_arr_inplace_plus_post_longdouble_fn(long double**,int,const char *,unsigned);
void** _zero_arr_inplace_plus_post_voidstar_fn(void***,int,const char*,unsigned);
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

#ifdef NO_CYC_BOUNDS_CHECKS
#define _check_fat_subscript(arr,elt_sz,index) ((arr).curr + (elt_sz) * (index))
#define _untag_fat_ptr(arr,elt_sz,num_elts) ((arr).curr)
#else
#define _check_fat_subscript(arr,elt_sz,index) ({ \
  struct _fat_ptr _cus_arr = (arr); \
  unsigned char *_cus_ans = _cus_arr.curr + (elt_sz) * (index); \
  /* JGM: not needed! if (!_cus_arr.base) _throw_null();*/ \
  if (_cus_ans < _cus_arr.base || _cus_ans >= _cus_arr.last_plus_one) \
    _throw_arraybounds(); \
  _cus_ans; })
#define _untag_fat_ptr(arr,elt_sz,num_elts) ({ \
  struct _fat_ptr _arr = (arr); \
  unsigned char *_curr = _arr.curr; \
  if ((_curr < _arr.base || _curr + (elt_sz) * (num_elts) > _arr.last_plus_one) &&\
      _curr != (unsigned char *)0) \
    _throw_arraybounds(); \
  _curr; })
#endif

#define _tag_fat(tcurr,elt_sz,num_elts) ({ \
  struct _fat_ptr _ans; \
  unsigned _num_elts = (num_elts);\
  _ans.base = _ans.curr = (void*)(tcurr); \
  /* JGM: if we're tagging NULL, ignore num_elts */ \
  _ans.last_plus_one = _ans.base ? (_ans.base + (elt_sz) * _num_elts) : 0; \
  _ans; })

#define _get_fat_size(arr,elt_sz) \
  ({struct _fat_ptr _arr = (arr); \
    unsigned char *_arr_curr=_arr.curr; \
    unsigned char *_arr_last=_arr.last_plus_one; \
    (_arr_curr < _arr.base || _arr_curr >= _arr_last) ? 0 : \
    ((_arr_last - _arr_curr) / (elt_sz));})

#define _fat_ptr_plus(arr,elt_sz,change) ({ \
  struct _fat_ptr _ans = (arr); \
  int _change = (change);\
  _ans.curr += (elt_sz) * _change;\
  _ans; })
#define _fat_ptr_inplace_plus(arr_ptr,elt_sz,change) ({ \
  struct _fat_ptr * _arr_ptr = (arr_ptr); \
  _arr_ptr->curr += (elt_sz) * (change);\
  *_arr_ptr; })
#define _fat_ptr_inplace_plus_post(arr_ptr,elt_sz,change) ({ \
  struct _fat_ptr * _arr_ptr = (arr_ptr); \
  struct _fat_ptr _ans = *_arr_ptr; \
  _arr_ptr->curr += (elt_sz) * (change);\
  _ans; })

//Not a macro since initialization order matters. Defined in runtime_zeroterm.c.
struct _fat_ptr _fat_ptr_decrease_size(struct _fat_ptr,unsigned sz,unsigned numelts);

/* Allocation */
void* GC_malloc(int);
void* GC_malloc_atomic(int);
void* GC_calloc(unsigned,unsigned);
void* GC_calloc_atomic(unsigned,unsigned);
// bound the allocation size to be < MAX_ALLOC_SIZE. See macros below for usage.
#define MAX_MALLOC_SIZE (1 << 28)
void* _bounded_GC_malloc(int,const char*,int);
void* _bounded_GC_malloc_atomic(int,const char*,int);
void* _bounded_GC_calloc(unsigned,unsigned,const char*,int);
void* _bounded_GC_calloc_atomic(unsigned,unsigned,const char*,int);
/* these macros are overridden below ifdef CYC_REGION_PROFILE */
#ifndef CYC_REGION_PROFILE
#define _cycalloc(n) _bounded_GC_malloc(n,__FILE__,__LINE__)
#define _cycalloc_atomic(n) _bounded_GC_malloc_atomic(n,__FILE__,__LINE__)
#define _cyccalloc(n,s) _bounded_GC_calloc(n,s,__FILE__,__LINE__)
#define _cyccalloc_atomic(n,s) _bounded_GC_calloc_atomic(n,s,__FILE__,__LINE__)
#endif

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

#ifdef CYC_REGION_PROFILE
/* see macros below for usage. defined in runtime_memory.c */
void* _profile_GC_malloc(int,const char*,const char*,int);
void* _profile_GC_malloc_atomic(int,const char*,const char*,int);
void* _profile_GC_calloc(unsigned,unsigned,const char*,const char*,int);
void* _profile_GC_calloc_atomic(unsigned,unsigned,const char*,const char*,int);
void* _profile_region_malloc(struct _RegionHandle*,unsigned,const char*,const char*,int);
void* _profile_region_calloc(struct _RegionHandle*,unsigned,unsigned,const char *,const char*,int);
struct _RegionHandle _profile_new_region(const char*,const char*,const char*,int);
void _profile_free_region(struct _RegionHandle*,const char*,const char*,int);
#ifndef RUNTIME_CYC
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
 struct Cyc_Core_Opt{void*v;};extern char Cyc_Core_Invalid_argument[17U];struct Cyc_Core_Invalid_argument_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Failure[8U];struct Cyc_Core_Failure_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Impossible[11U];struct Cyc_Core_Impossible_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Not_found[10U];struct Cyc_Core_Not_found_exn_struct{char*tag;};extern char Cyc_Core_Unreachable[12U];struct Cyc_Core_Unreachable_exn_struct{char*tag;struct _fat_ptr f1;};
# 168 "core.h"
extern struct _RegionHandle*Cyc_Core_heap_region;
# 171
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 61 "list.h"
extern int Cyc_List_length(struct Cyc_List_List*);extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 184
extern struct Cyc_List_List*Cyc_List_append(struct Cyc_List_List*,struct Cyc_List_List*);
# 190
extern struct Cyc_List_List*Cyc_List_rappend(struct _RegionHandle*,struct Cyc_List_List*,struct Cyc_List_List*);extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};
# 322
extern int Cyc_List_mem(int(*)(void*,void*),struct Cyc_List_List*,void*);
# 394
extern struct Cyc_List_List*Cyc_List_filter_c(int(*)(void*,void*),void*,struct Cyc_List_List*);struct Cyc___cycFILE;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};struct Cyc_timeval{long tv_sec;long tv_usec;};
# 38 "string.h"
extern unsigned long Cyc_strlen(struct _fat_ptr);
# 49 "string.h"
extern int Cyc_strcmp(struct _fat_ptr,struct _fat_ptr);
extern int Cyc_strptrcmp(struct _fat_ptr*,struct _fat_ptr*);struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;struct Cyc_Position_Error;struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};struct _tuple0{union Cyc_Absyn_Nmspace f1;struct _fat_ptr*f2;};
# 149 "absyn.h"
enum Cyc_Absyn_Scope{Cyc_Absyn_Static =0U,Cyc_Absyn_Abstract =1U,Cyc_Absyn_Public =2U,Cyc_Absyn_Extern =3U,Cyc_Absyn_ExternC =4U,Cyc_Absyn_Register =5U};struct Cyc_Absyn_Tqual{int print_const: 1;int q_volatile: 1;int q_restrict: 1;int real_const: 1;unsigned loc;};
# 170
enum Cyc_Absyn_Size_of{Cyc_Absyn_Char_sz =0U,Cyc_Absyn_Short_sz =1U,Cyc_Absyn_Int_sz =2U,Cyc_Absyn_Long_sz =3U,Cyc_Absyn_LongLong_sz =4U};
enum Cyc_Absyn_Sign{Cyc_Absyn_Signed =0U,Cyc_Absyn_Unsigned =1U,Cyc_Absyn_None =2U};
enum Cyc_Absyn_AggrKind{Cyc_Absyn_StructA =0U,Cyc_Absyn_UnionA =1U};
# 175
enum Cyc_Absyn_AliasQual{Cyc_Absyn_Aliasable =0U,Cyc_Absyn_Unique =1U,Cyc_Absyn_Top =2U};
# 180
enum Cyc_Absyn_KindQual{Cyc_Absyn_AnyKind =0U,Cyc_Absyn_MemKind =1U,Cyc_Absyn_BoxKind =2U,Cyc_Absyn_RgnKind =3U,Cyc_Absyn_EffKind =4U,Cyc_Absyn_IntKind =5U,Cyc_Absyn_BoolKind =6U,Cyc_Absyn_PtrBndKind =7U};struct Cyc_Absyn_Kind{enum Cyc_Absyn_KindQual kind;enum Cyc_Absyn_AliasQual aliasqual;};struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct{int tag;struct Cyc_Absyn_Kind*f1;};struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;};struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_Tvar{struct _fat_ptr*name;int identity;void*kind;};struct Cyc_Absyn_PtrLoc{unsigned ptr_loc;unsigned rgn_loc;unsigned zt_loc;};struct Cyc_Absyn_PtrAtts{void*rgn;void*nullable;void*bounds;void*zero_term;struct Cyc_Absyn_PtrLoc*ptrloc;void*released;};struct Cyc_Absyn_PtrInfo{void*elt_type;struct Cyc_Absyn_Tqual elt_tq;struct Cyc_Absyn_PtrAtts ptr_atts;};struct Cyc_Absyn_VarargInfo{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;int inject;};struct Cyc_Absyn_FnInfo{struct Cyc_List_List*tvars;void*effect;struct Cyc_Absyn_Tqual ret_tqual;void*ret_type;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*cyc_varargs;struct Cyc_List_List*rgn_po;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;struct Cyc_List_List*requires_relns;struct Cyc_Absyn_Exp*ensures_clause;struct Cyc_List_List*ensures_relns;struct Cyc_Absyn_Vardecl*return_value;};struct Cyc_Absyn_UnknownDatatypeInfo{struct _tuple0*name;int is_extensible;};struct _union_DatatypeInfo_UnknownDatatype{int tag;struct Cyc_Absyn_UnknownDatatypeInfo val;};struct _union_DatatypeInfo_KnownDatatype{int tag;struct Cyc_Absyn_Datatypedecl**val;};union Cyc_Absyn_DatatypeInfo{struct _union_DatatypeInfo_UnknownDatatype UnknownDatatype;struct _union_DatatypeInfo_KnownDatatype KnownDatatype;};struct Cyc_Absyn_UnknownDatatypeFieldInfo{struct _tuple0*datatype_name;struct _tuple0*field_name;int is_extensible;};struct _union_DatatypeFieldInfo_UnknownDatatypefield{int tag;struct Cyc_Absyn_UnknownDatatypeFieldInfo val;};struct _tuple1{struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;};struct _union_DatatypeFieldInfo_KnownDatatypefield{int tag;struct _tuple1 val;};union Cyc_Absyn_DatatypeFieldInfo{struct _union_DatatypeFieldInfo_UnknownDatatypefield UnknownDatatypefield;struct _union_DatatypeFieldInfo_KnownDatatypefield KnownDatatypefield;};struct _tuple2{enum Cyc_Absyn_AggrKind f1;struct _tuple0*f2;struct Cyc_Core_Opt*f3;};struct _union_AggrInfo_UnknownAggr{int tag;struct _tuple2 val;};struct _union_AggrInfo_KnownAggr{int tag;struct Cyc_Absyn_Aggrdecl**val;};union Cyc_Absyn_AggrInfo{struct _union_AggrInfo_UnknownAggr UnknownAggr;struct _union_AggrInfo_KnownAggr KnownAggr;};struct Cyc_Absyn_ArrayInfo{void*elt_type;struct Cyc_Absyn_Tqual tq;struct Cyc_Absyn_Exp*num_elts;void*zero_term;unsigned zt_loc;};struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_TypeDecl{void*r;unsigned loc;};struct Cyc_Absyn_VoidCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_IntCon_Absyn_TyCon_struct{int tag;enum Cyc_Absyn_Sign f1;enum Cyc_Absyn_Size_of f2;};struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct{int tag;int f1;};struct Cyc_Absyn_RgnHandleCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TagCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_HeapCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_UniqueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RefCntCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_AccessCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RgnsCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TrueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FalseCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_ThinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FatCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct{int tag;struct _tuple0*f1;struct Cyc_Absyn_Enumdecl*f2;};struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct{int tag;struct _fat_ptr f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeInfo f1;};struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeFieldInfo f1;};struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_AggrInfo f1;};struct Cyc_Absyn_AppType_Absyn_Type_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Evar_Absyn_Type_struct{int tag;struct Cyc_Core_Opt*f1;void*f2;int f3;struct Cyc_Core_Opt*f4;};struct Cyc_Absyn_VarType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Tvar*f1;};struct Cyc_Absyn_PointerType_Absyn_Type_struct{int tag;struct Cyc_Absyn_PtrInfo f1;};struct Cyc_Absyn_ArrayType_Absyn_Type_struct{int tag;struct Cyc_Absyn_ArrayInfo f1;};struct Cyc_Absyn_FnType_Absyn_Type_struct{int tag;struct Cyc_Absyn_FnInfo f1;};struct Cyc_Absyn_TupleType_Absyn_Type_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct{int tag;enum Cyc_Absyn_AggrKind f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_TypedefType_Absyn_Type_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_Typedefdecl*f3;void*f4;};struct Cyc_Absyn_ValueofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct{int tag;struct Cyc_Absyn_TypeDecl*f1;void**f2;};struct Cyc_Absyn_TypeofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};
# 392 "absyn.h"
enum Cyc_Absyn_Format_Type{Cyc_Absyn_Printf_ft =0U,Cyc_Absyn_Scanf_ft =1U};struct Cyc_Absyn_Regparm_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Stdcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Cdecl_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Fastcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Noreturn_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Const_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Packed_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Section_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Nocommon_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Shared_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Unused_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Weak_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllimport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllexport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_instrument_function_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Constructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Destructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_check_memory_usage_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Format_att_Absyn_Attribute_struct{int tag;enum Cyc_Absyn_Format_Type f1;int f2;int f3;};struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Noliveunique_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Consume_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Pure_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Mode_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Alias_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Always_inline_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_throw_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;unsigned f2;};struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;int f2;struct Cyc_Absyn_VarargInfo*f3;void*f4;struct Cyc_List_List*f5;struct Cyc_Absyn_Exp*f6;struct Cyc_Absyn_Exp*f7;};struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct{int tag;void*f1;unsigned f2;};struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;unsigned f3;};struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_PtrAtts f1;struct Cyc_Absyn_Tqual f2;};struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct{int tag;void*f1;};struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_List_List*f1;unsigned f2;int f3;};struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct{int tag;unsigned f1;struct Cyc_List_List*f2;};struct _union_Cnst_Null_c{int tag;int val;};struct _tuple3{enum Cyc_Absyn_Sign f1;char f2;};struct _union_Cnst_Char_c{int tag;struct _tuple3 val;};struct _union_Cnst_Wchar_c{int tag;struct _fat_ptr val;};struct _tuple4{enum Cyc_Absyn_Sign f1;short f2;};struct _union_Cnst_Short_c{int tag;struct _tuple4 val;};struct _tuple5{enum Cyc_Absyn_Sign f1;int f2;};struct _union_Cnst_Int_c{int tag;struct _tuple5 val;};struct _tuple6{enum Cyc_Absyn_Sign f1;long long f2;};struct _union_Cnst_LongLong_c{int tag;struct _tuple6 val;};struct _tuple7{struct _fat_ptr f1;int f2;};struct _union_Cnst_Float_c{int tag;struct _tuple7 val;};struct _union_Cnst_String_c{int tag;struct _fat_ptr val;};struct _union_Cnst_Wstring_c{int tag;struct _fat_ptr val;};union Cyc_Absyn_Cnst{struct _union_Cnst_Null_c Null_c;struct _union_Cnst_Char_c Char_c;struct _union_Cnst_Wchar_c Wchar_c;struct _union_Cnst_Short_c Short_c;struct _union_Cnst_Int_c Int_c;struct _union_Cnst_LongLong_c LongLong_c;struct _union_Cnst_Float_c Float_c;struct _union_Cnst_String_c String_c;struct _union_Cnst_Wstring_c Wstring_c;};
# 465
enum Cyc_Absyn_Primop{Cyc_Absyn_Plus =0U,Cyc_Absyn_Times =1U,Cyc_Absyn_Minus =2U,Cyc_Absyn_Div =3U,Cyc_Absyn_Mod =4U,Cyc_Absyn_Eq =5U,Cyc_Absyn_Neq =6U,Cyc_Absyn_Gt =7U,Cyc_Absyn_Lt =8U,Cyc_Absyn_Gte =9U,Cyc_Absyn_Lte =10U,Cyc_Absyn_Not =11U,Cyc_Absyn_Bitnot =12U,Cyc_Absyn_Bitand =13U,Cyc_Absyn_Bitor =14U,Cyc_Absyn_Bitxor =15U,Cyc_Absyn_Bitlshift =16U,Cyc_Absyn_Bitlrshift =17U,Cyc_Absyn_Numelts =18U};
# 472
enum Cyc_Absyn_Incrementor{Cyc_Absyn_PreInc =0U,Cyc_Absyn_PostInc =1U,Cyc_Absyn_PreDec =2U,Cyc_Absyn_PostDec =3U};struct Cyc_Absyn_VarargCallInfo{int num_varargs;struct Cyc_List_List*injectors;struct Cyc_Absyn_VarargInfo*vai;};struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct{int tag;unsigned f1;};
# 490
enum Cyc_Absyn_Coercion{Cyc_Absyn_Unknown_coercion =0U,Cyc_Absyn_No_coercion =1U,Cyc_Absyn_Null_to_NonNull =2U,Cyc_Absyn_Other_coercion =3U};struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_FieldName_Absyn_Designator_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_MallocInfo{int is_calloc;struct Cyc_Absyn_Exp*rgn;void**elt_type;struct Cyc_Absyn_Exp*num_elts;int fat_result;int inline_call;};struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct{int tag;union Cyc_Absyn_Cnst f1;};struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct{int tag;enum Cyc_Absyn_Primop f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;enum Cyc_Absyn_Incrementor f2;};struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_VarargCallInfo*f3;int f4;};struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;};struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Exp*f2;int f3;enum Cyc_Absyn_Coercion f4;};struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct _tuple8{struct _fat_ptr*f1;struct Cyc_Absyn_Tqual f2;void*f3;};struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct{int tag;struct _tuple8*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;int f4;};struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;int f3;};struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*f4;};struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_MallocInfo f1;};struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;};struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct{int tag;int f1;struct _fat_ptr f2;struct Cyc_List_List*f3;struct Cyc_List_List*f4;struct Cyc_List_List*f5;};struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Assert_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Exp{void*topt;void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Skip_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;struct Cyc_Absyn_Stmt*f3;};struct _tuple9{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct{int tag;struct _tuple9 f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Break_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Continue_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _tuple9 f2;struct _tuple9 f3;struct Cyc_Absyn_Stmt*f4;};struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Switch_clause**f2;};struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct _tuple9 f2;};struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Stmt{void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct{int tag;struct Cyc_List_List*f1;int f2;};struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Pat*f1;};struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct{int tag;union Cyc_Absyn_AggrInfo*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Null_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct{int tag;enum Cyc_Absyn_Sign f1;int f2;};struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct{int tag;char f1;};struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct{int tag;struct _fat_ptr f1;int f2;};struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_UnknownId_p_Absyn_Raw_pat_struct{int tag;struct _tuple0*f1;};struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;int f3;};struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Pat{void*r;void*topt;unsigned loc;};struct Cyc_Absyn_Switch_clause{struct Cyc_Absyn_Pat*pattern;struct Cyc_Core_Opt*pat_vars;struct Cyc_Absyn_Exp*where_clause;struct Cyc_Absyn_Stmt*body;unsigned loc;};struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct{int tag;struct _tuple0*f1;};struct Cyc_Absyn_Global_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Funname_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Param_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Local_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Pat_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Vardecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;unsigned varloc;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*initializer;void*rgn;struct Cyc_List_List*attributes;int escapes;int is_proto;};struct Cyc_Absyn_Fndecl{enum Cyc_Absyn_Scope sc;int is_inline;struct _tuple0*name;struct Cyc_Absyn_Stmt*body;struct Cyc_Absyn_FnInfo i;void*cached_type;struct Cyc_Core_Opt*param_vardecls;struct Cyc_Absyn_Vardecl*fn_vardecl;enum Cyc_Absyn_Scope orig_scope;};struct Cyc_Absyn_Aggrfield{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*width;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;};struct Cyc_Absyn_AggrdeclImpl{struct Cyc_List_List*exist_vars;struct Cyc_List_List*rgn_po;struct Cyc_List_List*fields;int tagged;};struct Cyc_Absyn_Aggrdecl{enum Cyc_Absyn_AggrKind kind;enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Absyn_AggrdeclImpl*impl;struct Cyc_List_List*attributes;int expected_mem_kind;};struct Cyc_Absyn_Datatypefield{struct _tuple0*name;struct Cyc_List_List*typs;unsigned loc;enum Cyc_Absyn_Scope sc;};struct Cyc_Absyn_Datatypedecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*fields;int is_extensible;};struct Cyc_Absyn_Enumfield{struct _tuple0*name;struct Cyc_Absyn_Exp*tag;unsigned loc;};struct Cyc_Absyn_Enumdecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_Core_Opt*fields;};struct Cyc_Absyn_Typedefdecl{struct _tuple0*name;struct Cyc_Absyn_Tqual tq;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*kind;void*defn;struct Cyc_List_List*atts;int extern_c;};struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Pat*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;void*f4;};struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Typedefdecl*f1;};struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct{int tag;struct _fat_ptr*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct _tuple10{unsigned f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct _tuple10*f4;};struct Cyc_Absyn_Porton_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Portoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempeston_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempestoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Decl{void*r;unsigned loc;};extern char Cyc_Absyn_EmptyAnnot[11U];struct Cyc_Absyn_EmptyAnnot_Absyn_AbsynAnnot_struct{char*tag;};
# 844 "absyn.h"
int Cyc_Absyn_qvar_cmp(struct _tuple0*,struct _tuple0*);
# 854
union Cyc_Absyn_Nmspace Cyc_Absyn_Abs_n(struct Cyc_List_List*,int);
# 857
struct Cyc_Absyn_Tqual Cyc_Absyn_const_tqual(unsigned);
struct Cyc_Absyn_Tqual Cyc_Absyn_empty_tqual(unsigned);
# 864
void*Cyc_Absyn_compress(void*);
# 877
void*Cyc_Absyn_new_evar(struct Cyc_Core_Opt*,struct Cyc_Core_Opt*);
# 893
extern void*Cyc_Absyn_false_type;
# 917
void*Cyc_Absyn_string_type(void*);
void*Cyc_Absyn_const_string_type(void*);
# 938
void*Cyc_Absyn_fatptr_type(void*,void*,struct Cyc_Absyn_Tqual,void*,void*);
# 946
void*Cyc_Absyn_array_type(void*,struct Cyc_Absyn_Tqual,struct Cyc_Absyn_Exp*,void*,unsigned);
# 974
struct Cyc_Absyn_Exp*Cyc_Absyn_uint_exp(unsigned,unsigned);
# 1032
struct _tuple0*Cyc_Absyn_uniquergn_qvar (void);struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};
# 54 "absynpp.h"
void Cyc_Absynpp_set_params(struct Cyc_Absynpp_Params*);
# 56
extern struct Cyc_Absynpp_Params Cyc_Absynpp_tc_params_r;
# 63
struct _fat_ptr Cyc_Absynpp_typ2string(void*);
# 71
struct _fat_ptr Cyc_Absynpp_qvar2string(struct _tuple0*);
# 35 "warn.h"
void Cyc_Warn_err(unsigned,struct _fat_ptr,struct _fat_ptr);struct Cyc_Warn_String_Warn_Warg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Warn_Qvar_Warn_Warg_struct{int tag;struct _tuple0*f1;};struct Cyc_Warn_Typ_Warn_Warg_struct{int tag;void*f1;};struct Cyc_Warn_TypOpt_Warn_Warg_struct{int tag;void*f1;};struct Cyc_Warn_Exp_Warn_Warg_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Warn_Stmt_Warn_Warg_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Warn_Aggrdecl_Warn_Warg_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Warn_Tvar_Warn_Warg_struct{int tag;struct Cyc_Absyn_Tvar*f1;};struct Cyc_Warn_KindBound_Warn_Warg_struct{int tag;void*f1;};struct Cyc_Warn_Kind_Warn_Warg_struct{int tag;struct Cyc_Absyn_Kind*f1;};struct Cyc_Warn_Attribute_Warn_Warg_struct{int tag;void*f1;};struct Cyc_Warn_Vardecl_Warn_Warg_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Warn_Int_Warn_Warg_struct{int tag;int f1;};
# 67
void Cyc_Warn_err2(unsigned,struct _fat_ptr);
# 69
void Cyc_Warn_warn2(unsigned,struct _fat_ptr);
# 43 "flags.h"
extern int Cyc_Flags_tc_aggressive_warn;
# 73
enum Cyc_Flags_C_Compilers{Cyc_Flags_Gcc_c =0U,Cyc_Flags_Vc_c =1U};
# 87 "flags.h"
enum Cyc_Flags_Cyclone_Passes{Cyc_Flags_Cpp =0U,Cyc_Flags_Parsing =1U,Cyc_Flags_Binding =2U,Cyc_Flags_CurrentRegion =3U,Cyc_Flags_TypeChecking =4U,Cyc_Flags_Jumps =5U,Cyc_Flags_FlowAnalysis =6U,Cyc_Flags_VCGen =7U,Cyc_Flags_CheckInsertion =8U,Cyc_Flags_Toc =9U,Cyc_Flags_AggregateRemoval =10U,Cyc_Flags_LabelRemoval =11U,Cyc_Flags_EvalOrder =12U,Cyc_Flags_CCompiler =13U,Cyc_Flags_AllPasses =14U};
# 27 "unify.h"
void Cyc_Unify_explain_failure (void);
# 29
int Cyc_Unify_unify(void*,void*);struct _union_RelnOp_RConst{int tag;unsigned val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned val;};struct _union_RelnOp_RReturn{int tag;unsigned val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 50 "relations-ap.h"
enum Cyc_Relations_Relation{Cyc_Relations_Req =0U,Cyc_Relations_Rneq =1U,Cyc_Relations_Rlte =2U,Cyc_Relations_Rlt =3U};struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};
# 84
struct Cyc_List_List*Cyc_Relations_exp2relns(struct _RegionHandle*,struct Cyc_Absyn_Exp*);
# 129
int Cyc_Relations_consistent_relations(struct Cyc_List_List*);struct Cyc_RgnOrder_RgnPO;
# 43 "tcutil.h"
int Cyc_Tcutil_is_function_type(void*);
# 46
int Cyc_Tcutil_is_array_type(void*);
# 58
int Cyc_Tcutil_is_bits_only_type(void*);
# 86
int Cyc_Tcutil_is_integral(struct Cyc_Absyn_Exp*);
# 107
int Cyc_Tcutil_coerce_assign(struct Cyc_RgnOrder_RgnPO*,struct Cyc_Absyn_Exp*,void*);
# 146
void*Cyc_Tcutil_fndecl2type(struct Cyc_Absyn_Fndecl*);
# 156
void Cyc_Tcutil_check_bitfield(unsigned,void*,struct Cyc_Absyn_Exp*,struct _fat_ptr*);
# 159
void Cyc_Tcutil_check_unique_tvars(unsigned,struct Cyc_List_List*);
# 190
int Cyc_Tcutil_is_noalias_pointer_or_aggr(void*);
# 205
void Cyc_Tcutil_add_tvar_identities(struct Cyc_List_List*);
# 210
int Cyc_Tcutil_is_const_exp(struct Cyc_Absyn_Exp*);
# 214
int Cyc_Tcutil_extract_const_from_typedef(unsigned,int,void*);
# 231
int Cyc_Tcutil_zeroable_type(void*);
# 238
void*Cyc_Tcutil_any_bool(struct Cyc_List_List*);
# 28 "kinds.h"
extern struct Cyc_Absyn_Kind Cyc_Kinds_rk;
extern struct Cyc_Absyn_Kind Cyc_Kinds_ak;
extern struct Cyc_Absyn_Kind Cyc_Kinds_bk;
# 32
extern struct Cyc_Absyn_Kind Cyc_Kinds_ek;
extern struct Cyc_Absyn_Kind Cyc_Kinds_ik;
# 37
extern struct Cyc_Absyn_Kind Cyc_Kinds_trk;
extern struct Cyc_Absyn_Kind Cyc_Kinds_tak;
extern struct Cyc_Absyn_Kind Cyc_Kinds_tbk;
extern struct Cyc_Absyn_Kind Cyc_Kinds_tmk;
# 67
void*Cyc_Kinds_kind_to_bound(struct Cyc_Absyn_Kind*);
# 78
void*Cyc_Kinds_compress_kb(void*);
# 43 "attributes.h"
extern struct Cyc_Absyn_No_throw_att_Absyn_Attribute_struct Cyc_Atts_No_throw_att_val;
# 62
struct Cyc_List_List*Cyc_Atts_transfer_fn_type_atts(void*,struct Cyc_List_List*);
# 70
void Cyc_Atts_check_fndecl_atts(unsigned,struct Cyc_List_List*,int);
void Cyc_Atts_check_variable_atts(unsigned,struct Cyc_Absyn_Vardecl*,struct Cyc_List_List*);
void Cyc_Atts_check_field_atts(unsigned,struct _fat_ptr*,struct Cyc_List_List*);
# 74
void Cyc_Atts_fnTypeAttsOK(unsigned,void*);
# 79
int Cyc_Atts_attribute_cmp(void*,void*);struct Cyc_Iter_Iter{void*env;int(*next)(void*,void*);};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};
# 83 "dict.h"
extern int Cyc_Dict_member(struct Cyc_Dict_Dict,void*);
# 87
extern struct Cyc_Dict_Dict Cyc_Dict_insert(struct Cyc_Dict_Dict,void*,void*);
# 110
extern void*Cyc_Dict_lookup(struct Cyc_Dict_Dict,void*);
# 122 "dict.h"
extern void**Cyc_Dict_lookup_opt(struct Cyc_Dict_Dict,void*);extern char Cyc_Tcenv_Env_error[10U];struct Cyc_Tcenv_Env_error_exn_struct{char*tag;};struct Cyc_Tcenv_Genv{struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;};struct Cyc_Tcenv_Fenv;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Tcenv_Genv*ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof: 1;int in_extern_c_include: 1;int in_tempest: 1;int tempest_generalize: 1;int in_extern_c_inc_repeat: 1;};
# 70 "tcenv.h"
struct Cyc_Tcenv_Fenv*Cyc_Tcenv_new_fenv(unsigned,struct Cyc_Absyn_Fndecl*);
# 79
struct Cyc_Absyn_Datatypedecl***Cyc_Tcenv_lookup_xdatatypedecl(struct _RegionHandle*,struct Cyc_Tcenv_Tenv*,unsigned,struct _tuple0*);
# 83
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_allow_valueof(struct Cyc_Tcenv_Tenv*);
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_enter_extern_c_include(struct Cyc_Tcenv_Tenv*);
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_enter_tempest(struct Cyc_Tcenv_Tenv*);
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_clear_tempest(struct Cyc_Tcenv_Tenv*);
# 89
enum Cyc_Tcenv_NewStatus{Cyc_Tcenv_NoneNew =0U,Cyc_Tcenv_InNew =1U,Cyc_Tcenv_InNewAggr =2U};
# 140
struct Cyc_RgnOrder_RgnPO*Cyc_Tcenv_curr_rgnpo(struct Cyc_Tcenv_Tenv*);
# 153
void Cyc_Tcenv_check_delayed_effects(struct Cyc_Tcenv_Tenv*);
void Cyc_Tcenv_check_delayed_constraints(struct Cyc_Tcenv_Tenv*);
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_copy_tenv_dicts(struct Cyc_Tcenv_Tenv*);
# 34 "tctyp.h"
void Cyc_Tctyp_check_valid_toplevel_type(unsigned,struct Cyc_Tcenv_Tenv*,void*);
void Cyc_Tctyp_check_fndecl_valid_type(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Fndecl*);
# 44 "tctyp.h"
void Cyc_Tctyp_check_type(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_List_List*,struct Cyc_Absyn_Kind*,int,int,void*);
# 26 "tcexp.h"
void*Cyc_Tcexp_tcExp(struct Cyc_Tcenv_Tenv*,void**,struct Cyc_Absyn_Exp*);
void*Cyc_Tcexp_tcExpInitializer(struct Cyc_Tcenv_Tenv*,void**,struct Cyc_Absyn_Exp*);
# 26 "tcstmt.h"
void Cyc_Tcstmt_tcStmt(struct Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Stmt*,int);
int Cyc_Tcstmt_ensure_no_throws(struct Cyc_Absyn_Stmt*);struct _tuple11{unsigned f1;int f2;};
# 28 "evexp.h"
extern struct _tuple11 Cyc_Evexp_eval_const_uint_exp(struct Cyc_Absyn_Exp*);extern char Cyc_Tcdecl_Incompatible[13U];struct Cyc_Tcdecl_Incompatible_exn_struct{char*tag;};struct Cyc_Tcdecl_Xdatatypefielddecl{struct Cyc_Absyn_Datatypedecl*base;struct Cyc_Absyn_Datatypefield*field;};
# 54 "tcdecl.h"
struct Cyc_Absyn_Aggrdecl*Cyc_Tcdecl_merge_aggrdecl(struct Cyc_Absyn_Aggrdecl*,struct Cyc_Absyn_Aggrdecl*,unsigned,struct _fat_ptr*);
# 57
struct Cyc_Absyn_Datatypedecl*Cyc_Tcdecl_merge_datatypedecl(struct Cyc_Absyn_Datatypedecl*,struct Cyc_Absyn_Datatypedecl*,unsigned,struct _fat_ptr*);
# 59
struct Cyc_Absyn_Enumdecl*Cyc_Tcdecl_merge_enumdecl(struct Cyc_Absyn_Enumdecl*,struct Cyc_Absyn_Enumdecl*,unsigned,struct _fat_ptr*);
# 66
void*Cyc_Tcdecl_merge_binding(void*,void*,unsigned,struct _fat_ptr*);
# 74
struct Cyc_List_List*Cyc_Tcdecl_sort_xdatatype_fields(struct Cyc_List_List*,int*,struct _fat_ptr*,unsigned,struct _fat_ptr*);struct Cyc_Set_Set;extern char Cyc_Set_Absent[7U];struct Cyc_Set_Absent_exn_struct{char*tag;};
# 92 "graph.h"
extern struct Cyc_Dict_Dict Cyc_Graph_scc(struct Cyc_Dict_Dict);
# 30 "callgraph.h"
struct Cyc_Dict_Dict Cyc_Callgraph_compute_callgraph(struct Cyc_List_List*);
# 36 "tc.h"
void Cyc_Tc_tcAggrdecl(struct Cyc_Tcenv_Tenv*,unsigned,struct Cyc_Absyn_Aggrdecl*);
void Cyc_Tc_tcDatatypedecl(struct Cyc_Tcenv_Tenv*,unsigned,struct Cyc_Absyn_Datatypedecl*);
void Cyc_Tc_tcEnumdecl(struct Cyc_Tcenv_Tenv*,unsigned,struct Cyc_Absyn_Enumdecl*);
# 36 "cifc.h"
void Cyc_Cifc_user_overrides(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_List_List**,struct Cyc_List_List*);struct Cyc_Hashtable_Table;extern char Cyc_Toc_Dest[5U];struct Cyc_Toc_Dest_Absyn_AbsynAnnot_struct{char*tag;struct Cyc_Absyn_Exp*f1;};
# 47 "toc.h"
void Cyc_Toc_init (void);
void Cyc_Toc_finish (void);static char _tmp0[1U]="";
# 46 "tc.cyc"
static struct _fat_ptr Cyc_Tc_tc_msg_c={_tmp0,_tmp0,_tmp0 + 1U};
static struct _fat_ptr*Cyc_Tc_tc_msg=& Cyc_Tc_tc_msg_c;struct _tuple12{unsigned f1;struct _tuple0*f2;int f3;};
# 49
static int Cyc_Tc_export_member(struct _tuple0*x,struct Cyc_List_List*exports){
for(1;exports != 0;exports=exports->tl){
struct _tuple12*_tmp1=(struct _tuple12*)exports->hd;struct _tuple12*p=_tmp1;
if(!Cyc_Absyn_qvar_cmp(x,(*p).f2)== 0)
continue;
# 56
(*p).f3=1;
return 1;}
# 59
return 0;}struct _tuple13{void*f1;int f2;};
# 62
static void Cyc_Tc_tcVardecl(struct Cyc_Tcenv_Tenv*te,unsigned loc,struct Cyc_Absyn_Vardecl*vd,int check_var_init,int in_cinclude,struct Cyc_List_List**exports){
# 64
struct Cyc_Absyn_Vardecl*_tmp2=vd;void*_tmp8;void*_tmp7;void*_tmp6;void*_tmp5;void*_tmp4;enum Cyc_Absyn_Scope _tmp3;_tmp3=_tmp2->sc;_tmp4=_tmp2->name;_tmp5=_tmp2->type;_tmp6=_tmp2->initializer;_tmp7=_tmp2->attributes;_tmp8=(int*)& _tmp2->is_proto;{enum Cyc_Absyn_Scope sc=_tmp3;struct _tuple0*q=_tmp4;void*t=_tmp5;struct Cyc_Absyn_Exp*initializer=_tmp6;struct Cyc_List_List*atts=_tmp7;int*is_proto=(int*)_tmp8;
# 71
{void*_tmp9=Cyc_Absyn_compress(t);void*_stmttmp0=_tmp9;void*_tmpA=_stmttmp0;unsigned _tmpE;void*_tmpD;struct Cyc_Absyn_Tqual _tmpC;void*_tmpB;if(*((int*)_tmpA)== 4){if((((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmpA)->f1).num_elts == 0){_tmpB=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmpA)->f1).elt_type;_tmpC=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmpA)->f1).tq;_tmpD=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmpA)->f1).zero_term;_tmpE=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmpA)->f1).zt_loc;if(initializer != 0){void*telt=_tmpB;struct Cyc_Absyn_Tqual tq=_tmpC;void*zt=_tmpD;unsigned ztl=_tmpE;
# 73
{void*_tmpF=initializer->r;void*_stmttmp1=_tmpF;void*_tmp10=_stmttmp1;void*_tmp11;struct _fat_ptr _tmp12;switch(*((int*)_tmp10)){case 0: switch(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp10)->f1).Wstring_c).tag){case 8: _tmp12=((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp10)->f1).String_c).val;{struct _fat_ptr s=_tmp12;
# 75
t=({void*_tmp22C=({void*_tmp22B=telt;struct Cyc_Absyn_Tqual _tmp22A=tq;struct Cyc_Absyn_Exp*_tmp229=Cyc_Absyn_uint_exp(_get_fat_size(s,sizeof(char)),0U);void*_tmp228=zt;Cyc_Absyn_array_type(_tmp22B,_tmp22A,_tmp229,_tmp228,ztl);});vd->type=_tmp22C;});goto _LL8;}case 9: _tmp12=((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp10)->f1).Wstring_c).val;{struct _fat_ptr s=_tmp12;
# 77
t=({void*_tmp231=({void*_tmp230=telt;struct Cyc_Absyn_Tqual _tmp22F=tq;struct Cyc_Absyn_Exp*_tmp22E=Cyc_Absyn_uint_exp(1U,0U);void*_tmp22D=zt;Cyc_Absyn_array_type(_tmp230,_tmp22F,_tmp22E,_tmp22D,ztl);});vd->type=_tmp231;});goto _LL8;}default: goto _LL15;}case 27: _tmp11=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp10)->f2;{struct Cyc_Absyn_Exp*e=_tmp11;
_tmp11=e;goto _LL10;}case 28: _tmp11=((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmp10)->f1;_LL10: {struct Cyc_Absyn_Exp*e=_tmp11;
# 80
t=({void*_tmp232=Cyc_Absyn_array_type(telt,tq,e,zt,ztl);vd->type=_tmp232;});goto _LL8;}case 36: _tmp11=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmp10)->f2;{struct Cyc_List_List*es=_tmp11;
_tmp11=es;goto _LL14;}case 26: _tmp11=((struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_tmp10)->f1;_LL14: {struct Cyc_List_List*es=_tmp11;
# 83
t=({void*_tmp237=({void*_tmp236=telt;struct Cyc_Absyn_Tqual _tmp235=tq;struct Cyc_Absyn_Exp*_tmp234=Cyc_Absyn_uint_exp((unsigned)((int(*)(struct Cyc_List_List*))Cyc_List_length)(es),0U);void*_tmp233=zt;Cyc_Absyn_array_type(_tmp236,_tmp235,_tmp234,_tmp233,ztl);});vd->type=_tmp237;});
goto _LL8;}default: _LL15:
 goto _LL8;}_LL8:;}
# 87
goto _LL3;}else{goto _LL6;}}else{goto _LL6;}}else{_LL6:
 goto _LL3;}_LL3:;}
# 91
Cyc_Tctyp_check_valid_toplevel_type(loc,te,t);
# 93
({int _tmp238=Cyc_Tcutil_extract_const_from_typedef(loc,(vd->tq).print_const,t);(vd->tq).real_const=_tmp238;});
# 96
({int _tmp239=!Cyc_Tcutil_is_array_type(t);vd->escapes=_tmp239;});
# 98
if(Cyc_Tcutil_is_function_type(t)){
*is_proto=0;
atts=Cyc_Atts_transfer_fn_type_atts(t,atts);
Cyc_Atts_fnTypeAttsOK(loc,t);}
# 104
if((int)sc == (int)3U ||(int)sc == (int)4U){
if(initializer != 0 && !in_cinclude)
({struct Cyc_Warn_String_Warn_Warg_struct _tmp14=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1BD;_tmp1BD.tag=0,({struct _fat_ptr _tmp23A=({const char*_tmp15="extern declaration should not have initializer";_tag_fat(_tmp15,sizeof(char),47U);});_tmp1BD.f1=_tmp23A;});_tmp1BD;});void*_tmp13[1];_tmp13[0]=& _tmp14;({unsigned _tmp23B=loc;Cyc_Warn_err2(_tmp23B,_tag_fat(_tmp13,sizeof(void*),1));});});}else{
if(!Cyc_Tcutil_is_function_type(t)){
# 111
Cyc_Atts_check_variable_atts(loc,vd,atts);
if(initializer == 0 || in_cinclude){
if((check_var_init && !in_cinclude)&& !Cyc_Tcutil_zeroable_type(t))
({struct Cyc_Warn_String_Warn_Warg_struct _tmp17=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1C1;_tmp1C1.tag=0,({struct _fat_ptr _tmp23C=({const char*_tmp1C="initializer required for variable ";_tag_fat(_tmp1C,sizeof(char),35U);});_tmp1C1.f1=_tmp23C;});_tmp1C1;});struct Cyc_Warn_Vardecl_Warn_Warg_struct _tmp18=({struct Cyc_Warn_Vardecl_Warn_Warg_struct _tmp1C0;_tmp1C0.tag=11,_tmp1C0.f1=vd;_tmp1C0;});struct Cyc_Warn_String_Warn_Warg_struct _tmp19=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1BF;_tmp1BF.tag=0,({struct _fat_ptr _tmp23D=({const char*_tmp1B=" of type ";_tag_fat(_tmp1B,sizeof(char),10U);});_tmp1BF.f1=_tmp23D;});_tmp1BF;});struct Cyc_Warn_Typ_Warn_Warg_struct _tmp1A=({struct Cyc_Warn_Typ_Warn_Warg_struct _tmp1BE;_tmp1BE.tag=2,_tmp1BE.f1=(void*)t;_tmp1BE;});void*_tmp16[4];_tmp16[0]=& _tmp17,_tmp16[1]=& _tmp18,_tmp16[2]=& _tmp19,_tmp16[3]=& _tmp1A;({unsigned _tmp23E=loc;Cyc_Warn_err2(_tmp23E,_tag_fat(_tmp16,sizeof(void*),4));});});}else{
# 119
struct _handler_cons _tmp1D;_push_handler(& _tmp1D);{int _tmp1F=0;if(setjmp(_tmp1D.handler))_tmp1F=1;if(!_tmp1F){
{void*_tmp20=Cyc_Tcexp_tcExpInitializer(te,& t,initializer);void*t2=_tmp20;
if(!({struct Cyc_RgnOrder_RgnPO*_tmp240=Cyc_Tcenv_curr_rgnpo(te);struct Cyc_Absyn_Exp*_tmp23F=initializer;Cyc_Tcutil_coerce_assign(_tmp240,_tmp23F,t);})){
struct _fat_ptr _tmp21=Cyc_Absynpp_qvar2string(vd->name);struct _fat_ptr s0=_tmp21;
const char*_tmp22=" declared with type ";const char*s1=_tmp22;
struct _fat_ptr _tmp23=Cyc_Absynpp_typ2string(t);struct _fat_ptr s2=_tmp23;
const char*_tmp24=" but initialized with type ";const char*s3=_tmp24;
struct _fat_ptr _tmp25=Cyc_Absynpp_typ2string(t2);struct _fat_ptr s4=_tmp25;
if(({unsigned long _tmp244=({unsigned long _tmp243=({unsigned long _tmp242=({unsigned long _tmp241=Cyc_strlen((struct _fat_ptr)s0);_tmp241 + Cyc_strlen(({const char*_tmp26=s1;_tag_fat(_tmp26,sizeof(char),21U);}));});_tmp242 + Cyc_strlen((struct _fat_ptr)s2);});_tmp243 + Cyc_strlen(({const char*_tmp27=s3;_tag_fat(_tmp27,sizeof(char),28U);}));});_tmp244 + Cyc_strlen((struct _fat_ptr)s4);})> (unsigned long)70)
({struct Cyc_String_pa_PrintArg_struct _tmp2A=({struct Cyc_String_pa_PrintArg_struct _tmp1C6;_tmp1C6.tag=0,_tmp1C6.f1=(struct _fat_ptr)((struct _fat_ptr)s0);_tmp1C6;});struct Cyc_String_pa_PrintArg_struct _tmp2B=({struct Cyc_String_pa_PrintArg_struct _tmp1C5;_tmp1C5.tag=0,({struct _fat_ptr _tmp245=(struct _fat_ptr)({const char*_tmp30=s1;_tag_fat(_tmp30,sizeof(char),21U);});_tmp1C5.f1=_tmp245;});_tmp1C5;});struct Cyc_String_pa_PrintArg_struct _tmp2C=({struct Cyc_String_pa_PrintArg_struct _tmp1C4;_tmp1C4.tag=0,_tmp1C4.f1=(struct _fat_ptr)((struct _fat_ptr)s2);_tmp1C4;});struct Cyc_String_pa_PrintArg_struct _tmp2D=({struct Cyc_String_pa_PrintArg_struct _tmp1C3;_tmp1C3.tag=0,({struct _fat_ptr _tmp246=(struct _fat_ptr)({const char*_tmp2F=s3;_tag_fat(_tmp2F,sizeof(char),28U);});_tmp1C3.f1=_tmp246;});_tmp1C3;});struct Cyc_String_pa_PrintArg_struct _tmp2E=({struct Cyc_String_pa_PrintArg_struct _tmp1C2;_tmp1C2.tag=0,_tmp1C2.f1=(struct _fat_ptr)((struct _fat_ptr)s4);_tmp1C2;});void*_tmp28[5];_tmp28[0]=& _tmp2A,_tmp28[1]=& _tmp2B,_tmp28[2]=& _tmp2C,_tmp28[3]=& _tmp2D,_tmp28[4]=& _tmp2E;({unsigned _tmp248=loc;struct _fat_ptr _tmp247=({const char*_tmp29="%s%s\n\t%s\n%s\n\t%s";_tag_fat(_tmp29,sizeof(char),16U);});Cyc_Warn_err(_tmp248,_tmp247,_tag_fat(_tmp28,sizeof(void*),5));});});else{
# 130
({struct Cyc_String_pa_PrintArg_struct _tmp33=({struct Cyc_String_pa_PrintArg_struct _tmp1CB;_tmp1CB.tag=0,_tmp1CB.f1=(struct _fat_ptr)((struct _fat_ptr)s0);_tmp1CB;});struct Cyc_String_pa_PrintArg_struct _tmp34=({struct Cyc_String_pa_PrintArg_struct _tmp1CA;_tmp1CA.tag=0,({struct _fat_ptr _tmp249=(struct _fat_ptr)({const char*_tmp39=s1;_tag_fat(_tmp39,sizeof(char),21U);});_tmp1CA.f1=_tmp249;});_tmp1CA;});struct Cyc_String_pa_PrintArg_struct _tmp35=({struct Cyc_String_pa_PrintArg_struct _tmp1C9;_tmp1C9.tag=0,_tmp1C9.f1=(struct _fat_ptr)((struct _fat_ptr)s2);_tmp1C9;});struct Cyc_String_pa_PrintArg_struct _tmp36=({struct Cyc_String_pa_PrintArg_struct _tmp1C8;_tmp1C8.tag=0,({struct _fat_ptr _tmp24A=(struct _fat_ptr)({const char*_tmp38=s3;_tag_fat(_tmp38,sizeof(char),28U);});_tmp1C8.f1=_tmp24A;});_tmp1C8;});struct Cyc_String_pa_PrintArg_struct _tmp37=({struct Cyc_String_pa_PrintArg_struct _tmp1C7;_tmp1C7.tag=0,_tmp1C7.f1=(struct _fat_ptr)((struct _fat_ptr)s4);_tmp1C7;});void*_tmp31[5];_tmp31[0]=& _tmp33,_tmp31[1]=& _tmp34,_tmp31[2]=& _tmp35,_tmp31[3]=& _tmp36,_tmp31[4]=& _tmp37;({unsigned _tmp24C=loc;struct _fat_ptr _tmp24B=({const char*_tmp32="%s%s%s%s%s";_tag_fat(_tmp32,sizeof(char),11U);});Cyc_Warn_err(_tmp24C,_tmp24B,_tag_fat(_tmp31,sizeof(void*),5));});});}
Cyc_Unify_explain_failure();}
# 134
if(!Cyc_Tcutil_is_const_exp(initializer))
({struct Cyc_Warn_String_Warn_Warg_struct _tmp3B=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1CC;_tmp1CC.tag=0,({struct _fat_ptr _tmp24D=({const char*_tmp3C="initializer is not a constant expression";_tag_fat(_tmp3C,sizeof(char),41U);});_tmp1CC.f1=_tmp24D;});_tmp1CC;});void*_tmp3A[1];_tmp3A[0]=& _tmp3B;({unsigned _tmp24E=loc;Cyc_Warn_err2(_tmp24E,_tag_fat(_tmp3A,sizeof(void*),1));});});}
# 120
;_pop_handler();}else{void*_tmp1E=(void*)Cyc_Core_get_exn_thrown();void*_tmp3D=_tmp1E;void*_tmp3E;if(((struct Cyc_Tcenv_Env_error_exn_struct*)_tmp3D)->tag == Cyc_Tcenv_Env_error){
# 137
({struct Cyc_Warn_String_Warn_Warg_struct _tmp40=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1CD;_tmp1CD.tag=0,({struct _fat_ptr _tmp24F=({const char*_tmp41="initializer is not a constant expression";_tag_fat(_tmp41,sizeof(char),41U);});_tmp1CD.f1=_tmp24F;});_tmp1CD;});void*_tmp3F[1];_tmp3F[0]=& _tmp40;({unsigned _tmp250=loc;Cyc_Warn_err2(_tmp250,_tag_fat(_tmp3F,sizeof(void*),1));});});goto _LL17;}else{_tmp3E=_tmp3D;{void*exn=_tmp3E;(int)_rethrow(exn);}}_LL17:;}}}}else{
# 141
Cyc_Atts_check_fndecl_atts(loc,atts,0);}}
# 144
{struct _handler_cons _tmp42;_push_handler(& _tmp42);{int _tmp44=0;if(setjmp(_tmp42.handler))_tmp44=1;if(!_tmp44){
{struct _tuple13*_tmp45=((struct _tuple13*(*)(struct Cyc_Dict_Dict,struct _tuple0*))Cyc_Dict_lookup)((te->ae)->ordinaries,q);struct _tuple13*ans=_tmp45;
void*_tmp46=(*ans).f1;void*b0=_tmp46;
struct Cyc_Absyn_Global_b_Absyn_Binding_struct*_tmp47=({struct Cyc_Absyn_Global_b_Absyn_Binding_struct*_tmp4A=_cycalloc(sizeof(*_tmp4A));_tmp4A->tag=1,_tmp4A->f1=vd;_tmp4A;});struct Cyc_Absyn_Global_b_Absyn_Binding_struct*b1=_tmp47;
void*_tmp48=Cyc_Tcdecl_merge_binding(b0,(void*)b1,loc,Cyc_Tc_tc_msg);void*b=_tmp48;
if(b == 0){_npop_handler(0);return;}
# 151
if(exports == 0 || Cyc_Tc_export_member(vd->name,*exports)){
if(b != b0 ||(*ans).f2)
# 154
({struct Cyc_Dict_Dict _tmp253=({struct Cyc_Dict_Dict _tmp252=(te->ae)->ordinaries;struct _tuple0*_tmp251=q;((struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict,struct _tuple0*,struct _tuple13*))Cyc_Dict_insert)(_tmp252,_tmp251,({struct _tuple13*_tmp49=_cycalloc(sizeof(*_tmp49));
_tmp49->f1=b,_tmp49->f2=(*ans).f2;_tmp49;}));});
# 154
(te->ae)->ordinaries=_tmp253;});}
# 156
_npop_handler(0);return;}
# 145
;_pop_handler();}else{void*_tmp43=(void*)Cyc_Core_get_exn_thrown();void*_tmp4B=_tmp43;void*_tmp4C;if(((struct Cyc_Dict_Absent_exn_struct*)_tmp4B)->tag == Cyc_Dict_Absent)
# 157
goto _LL1C;else{_tmp4C=_tmp4B;{void*exn=_tmp4C;(int)_rethrow(exn);}}_LL1C:;}}}
if(exports == 0 || Cyc_Tc_export_member(vd->name,*exports))
({struct Cyc_Dict_Dict _tmp257=({struct Cyc_Dict_Dict _tmp256=(te->ae)->ordinaries;struct _tuple0*_tmp255=q;((struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict,struct _tuple0*,struct _tuple13*))Cyc_Dict_insert)(_tmp256,_tmp255,({struct _tuple13*_tmp4E=_cycalloc(sizeof(*_tmp4E));
({void*_tmp254=(void*)({struct Cyc_Absyn_Global_b_Absyn_Binding_struct*_tmp4D=_cycalloc(sizeof(*_tmp4D));_tmp4D->tag=1,_tmp4D->f1=vd;_tmp4D;});_tmp4E->f1=_tmp254;}),_tmp4E->f2=0;_tmp4E;}));});
# 159
(te->ae)->ordinaries=_tmp257;});}}
# 163
static int Cyc_Tc_is_main(struct _tuple0*n){
struct _tuple0*_tmp4F=n;void*_tmp51;union Cyc_Absyn_Nmspace _tmp50;_tmp50=_tmp4F->f1;_tmp51=_tmp4F->f2;{union Cyc_Absyn_Nmspace nms=_tmp50;struct _fat_ptr*v=_tmp51;
union Cyc_Absyn_Nmspace _tmp52=nms;if((_tmp52.Abs_n).tag == 2){if((_tmp52.Abs_n).val == 0)
return({struct _fat_ptr _tmp258=(struct _fat_ptr)*v;Cyc_strcmp(_tmp258,({const char*_tmp53="main";_tag_fat(_tmp53,sizeof(char),5U);}));})== 0;else{goto _LL6;}}else{_LL6:
 return 0;};}}
# 174
static int Cyc_Tc_check_catchall_handler(struct Cyc_Absyn_Fndecl*fd){
struct Cyc_Absyn_Stmt*_tmp54=fd->body;struct Cyc_Absyn_Stmt*b=_tmp54;
void*_tmp55=b->r;void*_stmttmp2=_tmp55;void*_tmp56=_stmttmp2;void*_tmp57;if(*((int*)_tmp56)== 15){_tmp57=((struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*)_tmp56)->f2;{struct Cyc_List_List*case_list=_tmp57;
# 178
int found_wild=0;
while((unsigned)case_list){
struct Cyc_Absyn_Switch_clause*_tmp58=(struct Cyc_Absyn_Switch_clause*)case_list->hd;struct Cyc_Absyn_Switch_clause*clause=_tmp58;
{void*_tmp59=(clause->pattern)->r;void*_stmttmp3=_tmp59;void*_tmp5A=_stmttmp3;if(*((int*)_tmp5A)== 0){
# 183
found_wild=1;
goto _LL5;}else{
# 186
goto _LL5;}_LL5:;}
# 188
if(!Cyc_Tcstmt_ensure_no_throws(clause->body))
return 0;
case_list=case_list->tl;}
# 192
return found_wild;}}else{
# 194
return 0;};}
# 198
static void Cyc_Tc_tcFndecl(struct Cyc_Tcenv_Tenv*te,unsigned loc,struct Cyc_Absyn_Fndecl*fd,struct Cyc_List_List**exports){
# 200
struct _tuple0*q=fd->name;
# 204
if((int)fd->sc == (int)Cyc_Absyn_ExternC && !te->in_extern_c_include)
({struct Cyc_Warn_String_Warn_Warg_struct _tmp5C=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1CE;_tmp1CE.tag=0,({struct _fat_ptr _tmp259=({const char*_tmp5D="extern \"C\" functions cannot be implemented in Cyclone";_tag_fat(_tmp5D,sizeof(char),54U);});_tmp1CE.f1=_tmp259;});_tmp1CE;});void*_tmp5B[1];_tmp5B[0]=& _tmp5C;({unsigned _tmp25A=loc;Cyc_Warn_err2(_tmp25A,_tag_fat(_tmp5B,sizeof(void*),1));});});
# 208
Cyc_Atts_check_fndecl_atts(loc,(fd->i).attributes,1);
# 211
if(te->in_extern_c_inc_repeat)
fd->cached_type=0;
Cyc_Tctyp_check_fndecl_valid_type(loc,te,fd);{
void*t=Cyc_Tcutil_fndecl2type(fd);
# 216
int nothrow=((int(*)(int(*)(void*,void*),struct Cyc_List_List*,void*))Cyc_List_mem)(Cyc_Atts_attribute_cmp,(fd->i).attributes,(void*)& Cyc_Atts_No_throw_att_val);
# 219
({struct Cyc_List_List*_tmp25B=Cyc_Atts_transfer_fn_type_atts(t,(fd->i).attributes);(fd->i).attributes=_tmp25B;});
Cyc_Atts_fnTypeAttsOK(loc,t);
# 223
{struct _handler_cons _tmp5E;_push_handler(& _tmp5E);{int _tmp60=0;if(setjmp(_tmp5E.handler))_tmp60=1;if(!_tmp60){
{struct _tuple13*_tmp61=((struct _tuple13*(*)(struct Cyc_Dict_Dict,struct _tuple0*))Cyc_Dict_lookup)((te->ae)->ordinaries,q);struct _tuple13*ans=_tmp61;
void*_tmp62=(*ans).f1;void*b0=_tmp62;
struct Cyc_Absyn_Funname_b_Absyn_Binding_struct*_tmp63=({struct Cyc_Absyn_Funname_b_Absyn_Binding_struct*_tmp66=_cycalloc(sizeof(*_tmp66));_tmp66->tag=2,_tmp66->f1=fd;_tmp66;});struct Cyc_Absyn_Funname_b_Absyn_Binding_struct*b1=_tmp63;
void*_tmp64=Cyc_Tcdecl_merge_binding(b0,(void*)b1,loc,Cyc_Tc_tc_msg);void*b=_tmp64;
if(b != 0){
# 230
if(exports == 0 || Cyc_Tc_export_member(q,*exports)){
if(!(b == b0 &&(*ans).f2))
({struct Cyc_Dict_Dict _tmp25E=({struct Cyc_Dict_Dict _tmp25D=(te->ae)->ordinaries;struct _tuple0*_tmp25C=q;((struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict,struct _tuple0*,struct _tuple13*))Cyc_Dict_insert)(_tmp25D,_tmp25C,({struct _tuple13*_tmp65=_cycalloc(sizeof(*_tmp65));
_tmp65->f1=b,_tmp65->f2=(*ans).f2;_tmp65;}));});
# 232
(te->ae)->ordinaries=_tmp25E;});}}}
# 224
;_pop_handler();}else{void*_tmp5F=(void*)Cyc_Core_get_exn_thrown();void*_tmp67=_tmp5F;void*_tmp68;if(((struct Cyc_Dict_Absent_exn_struct*)_tmp67)->tag == Cyc_Dict_Absent){
# 235
if(exports == 0 || Cyc_Tc_export_member(fd->name,*exports))
({struct Cyc_Dict_Dict _tmp262=({struct Cyc_Dict_Dict _tmp261=(te->ae)->ordinaries;struct _tuple0*_tmp260=q;((struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict,struct _tuple0*,struct _tuple13*))Cyc_Dict_insert)(_tmp261,_tmp260,({struct _tuple13*_tmp6A=_cycalloc(sizeof(*_tmp6A));
({void*_tmp25F=(void*)({struct Cyc_Absyn_Funname_b_Absyn_Binding_struct*_tmp69=_cycalloc(sizeof(*_tmp69));_tmp69->tag=2,_tmp69->f1=fd;_tmp69;});_tmp6A->f1=_tmp25F;}),_tmp6A->f2=0;_tmp6A;}));});
# 236
(te->ae)->ordinaries=_tmp262;});
# 238
goto _LL0;}else{_tmp68=_tmp67;{void*exn=_tmp68;(int)_rethrow(exn);}}_LL0:;}}}
# 242
if(te->in_extern_c_include)return;{
# 247
struct Cyc_Tcenv_Fenv*_tmp6B=Cyc_Tcenv_new_fenv(loc,fd);struct Cyc_Tcenv_Fenv*fenv=_tmp6B;
struct Cyc_Tcenv_Tenv*_tmp6C=({struct Cyc_Tcenv_Tenv*_tmp9F=_cycalloc(sizeof(*_tmp9F));_tmp9F->ns=te->ns,_tmp9F->ae=te->ae,_tmp9F->le=fenv,_tmp9F->allow_valueof=0,_tmp9F->in_extern_c_include=0,_tmp9F->in_tempest=te->in_tempest,_tmp9F->tempest_generalize=te->tempest_generalize,_tmp9F->in_extern_c_inc_repeat=0;_tmp9F;});struct Cyc_Tcenv_Tenv*te=_tmp6C;
# 250
Cyc_Tcstmt_tcStmt(te,fd->body,0);
# 253
Cyc_Tcenv_check_delayed_effects(te);
Cyc_Tcenv_check_delayed_constraints(te);
# 256
if(te->in_tempest){
te->tempest_generalize=1;
Cyc_Tctyp_check_fndecl_valid_type(loc,te,fd);
te->tempest_generalize=0;}
# 261
if(nothrow && !Cyc_Tc_check_catchall_handler(fd))
({struct Cyc_Warn_String_Warn_Warg_struct _tmp6E=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1CF;_tmp1CF.tag=0,({struct _fat_ptr _tmp263=({const char*_tmp6F="Function with attribute no_throw must have a catch-all handler";_tag_fat(_tmp6F,sizeof(char),63U);});_tmp1CF.f1=_tmp263;});_tmp1CF;});void*_tmp6D[1];_tmp6D[0]=& _tmp6E;({unsigned _tmp264=loc;Cyc_Warn_err2(_tmp264,_tag_fat(_tmp6D,sizeof(void*),1));});});
# 264
if(Cyc_Tc_is_main(q)){
# 266
{void*_tmp70=Cyc_Absyn_compress((fd->i).ret_type);void*_stmttmp4=_tmp70;void*_tmp71=_stmttmp4;if(*((int*)_tmp71)== 0)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp71)->f1)){case 0:
# 268
({struct Cyc_Warn_String_Warn_Warg_struct _tmp73=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1D0;_tmp1D0.tag=0,({struct _fat_ptr _tmp265=({const char*_tmp74="main declared with return type void";_tag_fat(_tmp74,sizeof(char),36U);});_tmp1D0.f1=_tmp265;});_tmp1D0;});void*_tmp72[1];_tmp72[0]=& _tmp73;({unsigned _tmp266=loc;Cyc_Warn_warn2(_tmp266,_tag_fat(_tmp72,sizeof(void*),1));});});goto _LL5;case 1: switch((int)((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp71)->f1)->f2){case Cyc_Absyn_Int_sz:
 goto _LLB;case Cyc_Absyn_Long_sz: _LLB:
 goto _LL5;default: goto _LLC;}default: goto _LLC;}else{_LLC:
({struct Cyc_Warn_String_Warn_Warg_struct _tmp76=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1D3;_tmp1D3.tag=0,({struct _fat_ptr _tmp267=({const char*_tmp7A="main declared with return type ";_tag_fat(_tmp7A,sizeof(char),32U);});_tmp1D3.f1=_tmp267;});_tmp1D3;});struct Cyc_Warn_Typ_Warn_Warg_struct _tmp77=({struct Cyc_Warn_Typ_Warn_Warg_struct _tmp1D2;_tmp1D2.tag=2,_tmp1D2.f1=(void*)(fd->i).ret_type;_tmp1D2;});struct Cyc_Warn_String_Warn_Warg_struct _tmp78=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1D1;_tmp1D1.tag=0,({
struct _fat_ptr _tmp268=({const char*_tmp79=" instead of int or void";_tag_fat(_tmp79,sizeof(char),24U);});_tmp1D1.f1=_tmp268;});_tmp1D1;});void*_tmp75[3];_tmp75[0]=& _tmp76,_tmp75[1]=& _tmp77,_tmp75[2]=& _tmp78;({unsigned _tmp269=loc;Cyc_Warn_err2(_tmp269,_tag_fat(_tmp75,sizeof(void*),3));});});}_LL5:;}
# 274
if((fd->i).c_varargs ||(fd->i).cyc_varargs != 0)
({struct Cyc_Warn_String_Warn_Warg_struct _tmp7C=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1D4;_tmp1D4.tag=0,({struct _fat_ptr _tmp26A=({const char*_tmp7D="main declared with varargs";_tag_fat(_tmp7D,sizeof(char),27U);});_tmp1D4.f1=_tmp26A;});_tmp1D4;});void*_tmp7B[1];_tmp7B[0]=& _tmp7C;({unsigned _tmp26B=loc;Cyc_Warn_err2(_tmp26B,_tag_fat(_tmp7B,sizeof(void*),1));});});{
struct Cyc_List_List*_tmp7E=(fd->i).args;struct Cyc_List_List*args=_tmp7E;
if(args != 0){
struct _tuple8*_tmp7F=(struct _tuple8*)args->hd;struct _tuple8*_stmttmp5=_tmp7F;struct _tuple8*_tmp80=_stmttmp5;void*_tmp81;_tmp81=_tmp80->f3;{void*t1=_tmp81;
{void*_tmp82=Cyc_Absyn_compress(t1);void*_stmttmp6=_tmp82;void*_tmp83=_stmttmp6;if(*((int*)_tmp83)== 0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp83)->f1)== 1)switch((int)((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp83)->f1)->f2){case Cyc_Absyn_Int_sz:
 goto _LL15;case Cyc_Absyn_Long_sz: _LL15:
 goto _LL11;default: goto _LL16;}else{goto _LL16;}}else{_LL16:
({struct Cyc_Warn_String_Warn_Warg_struct _tmp85=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1D7;_tmp1D7.tag=0,({struct _fat_ptr _tmp26C=({const char*_tmp89="main declared with first argument of type ";_tag_fat(_tmp89,sizeof(char),43U);});_tmp1D7.f1=_tmp26C;});_tmp1D7;});struct Cyc_Warn_Typ_Warn_Warg_struct _tmp86=({struct Cyc_Warn_Typ_Warn_Warg_struct _tmp1D6;_tmp1D6.tag=2,_tmp1D6.f1=(void*)t1;_tmp1D6;});struct Cyc_Warn_String_Warn_Warg_struct _tmp87=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1D5;_tmp1D5.tag=0,({
struct _fat_ptr _tmp26D=({const char*_tmp88=" instead of int";_tag_fat(_tmp88,sizeof(char),16U);});_tmp1D5.f1=_tmp26D;});_tmp1D5;});void*_tmp84[3];_tmp84[0]=& _tmp85,_tmp84[1]=& _tmp86,_tmp84[2]=& _tmp87;({unsigned _tmp26E=loc;Cyc_Warn_err2(_tmp26E,_tag_fat(_tmp84,sizeof(void*),3));});});}_LL11:;}
# 285
args=args->tl;
if(args != 0){
struct _tuple8*_tmp8A=(struct _tuple8*)args->hd;struct _tuple8*_stmttmp7=_tmp8A;struct _tuple8*_tmp8B=_stmttmp7;void*_tmp8C;_tmp8C=_tmp8B->f3;{void*t2=_tmp8C;
args=args->tl;
if(args != 0)
({struct Cyc_Warn_String_Warn_Warg_struct _tmp8E=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1D8;_tmp1D8.tag=0,({struct _fat_ptr _tmp26F=({const char*_tmp8F="main declared with too many arguments";_tag_fat(_tmp8F,sizeof(char),38U);});_tmp1D8.f1=_tmp26F;});_tmp1D8;});void*_tmp8D[1];_tmp8D[0]=& _tmp8E;({unsigned _tmp270=loc;Cyc_Warn_err2(_tmp270,_tag_fat(_tmp8D,sizeof(void*),1));});});{
struct Cyc_Core_Opt*tvs=({struct Cyc_Core_Opt*_tmp9E=_cycalloc(sizeof(*_tmp9E));_tmp9E->v=(fd->i).tvars;_tmp9E;});
if(((!({void*_tmp28C=t2;Cyc_Unify_unify(_tmp28C,({void*_tmp28B=Cyc_Absyn_string_type(({struct Cyc_Core_Opt*_tmp286=({struct Cyc_Core_Opt*_tmp90=_cycalloc(sizeof(*_tmp90));_tmp90->v=& Cyc_Kinds_rk;_tmp90;});Cyc_Absyn_new_evar(_tmp286,tvs);}));void*_tmp28A=({
struct Cyc_Core_Opt*_tmp287=({struct Cyc_Core_Opt*_tmp91=_cycalloc(sizeof(*_tmp91));_tmp91->v=& Cyc_Kinds_rk;_tmp91;});Cyc_Absyn_new_evar(_tmp287,tvs);});
# 292
struct Cyc_Absyn_Tqual _tmp289=
# 294
Cyc_Absyn_empty_tqual(0U);
# 292
void*_tmp288=
# 294
Cyc_Tcutil_any_bool((struct Cyc_List_List*)tvs->v);
# 292
Cyc_Absyn_fatptr_type(_tmp28B,_tmp28A,_tmp289,_tmp288,Cyc_Absyn_false_type);}));})&& !({
# 295
void*_tmp285=t2;Cyc_Unify_unify(_tmp285,({void*_tmp284=Cyc_Absyn_const_string_type(({struct Cyc_Core_Opt*_tmp27F=({struct Cyc_Core_Opt*_tmp92=_cycalloc(sizeof(*_tmp92));_tmp92->v=& Cyc_Kinds_rk;_tmp92;});Cyc_Absyn_new_evar(_tmp27F,tvs);}));void*_tmp283=({
# 297
struct Cyc_Core_Opt*_tmp280=({struct Cyc_Core_Opt*_tmp93=_cycalloc(sizeof(*_tmp93));_tmp93->v=& Cyc_Kinds_rk;_tmp93;});Cyc_Absyn_new_evar(_tmp280,tvs);});
# 295
struct Cyc_Absyn_Tqual _tmp282=
# 298
Cyc_Absyn_empty_tqual(0U);
# 295
void*_tmp281=
# 298
Cyc_Tcutil_any_bool((struct Cyc_List_List*)tvs->v);
# 295
Cyc_Absyn_fatptr_type(_tmp284,_tmp283,_tmp282,_tmp281,Cyc_Absyn_false_type);}));}))&& !({
# 299
void*_tmp27E=t2;Cyc_Unify_unify(_tmp27E,({void*_tmp27D=Cyc_Absyn_string_type(({struct Cyc_Core_Opt*_tmp278=({struct Cyc_Core_Opt*_tmp94=_cycalloc(sizeof(*_tmp94));_tmp94->v=& Cyc_Kinds_rk;_tmp94;});Cyc_Absyn_new_evar(_tmp278,tvs);}));void*_tmp27C=({
struct Cyc_Core_Opt*_tmp279=({struct Cyc_Core_Opt*_tmp95=_cycalloc(sizeof(*_tmp95));_tmp95->v=& Cyc_Kinds_rk;_tmp95;});Cyc_Absyn_new_evar(_tmp279,tvs);});
# 299
struct Cyc_Absyn_Tqual _tmp27B=
# 301
Cyc_Absyn_const_tqual(0U);
# 299
void*_tmp27A=
# 301
Cyc_Tcutil_any_bool((struct Cyc_List_List*)tvs->v);
# 299
Cyc_Absyn_fatptr_type(_tmp27D,_tmp27C,_tmp27B,_tmp27A,Cyc_Absyn_false_type);}));}))&& !({
# 302
void*_tmp277=t2;Cyc_Unify_unify(_tmp277,({void*_tmp276=Cyc_Absyn_const_string_type(({struct Cyc_Core_Opt*_tmp271=({struct Cyc_Core_Opt*_tmp96=_cycalloc(sizeof(*_tmp96));_tmp96->v=& Cyc_Kinds_rk;_tmp96;});Cyc_Absyn_new_evar(_tmp271,tvs);}));void*_tmp275=({
# 304
struct Cyc_Core_Opt*_tmp272=({struct Cyc_Core_Opt*_tmp97=_cycalloc(sizeof(*_tmp97));_tmp97->v=& Cyc_Kinds_rk;_tmp97;});Cyc_Absyn_new_evar(_tmp272,tvs);});
# 302
struct Cyc_Absyn_Tqual _tmp274=
# 305
Cyc_Absyn_const_tqual(0U);
# 302
void*_tmp273=
# 305
Cyc_Tcutil_any_bool((struct Cyc_List_List*)tvs->v);
# 302
Cyc_Absyn_fatptr_type(_tmp276,_tmp275,_tmp274,_tmp273,Cyc_Absyn_false_type);}));}))
# 306
({struct Cyc_Warn_String_Warn_Warg_struct _tmp99=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1DB;_tmp1DB.tag=0,({struct _fat_ptr _tmp28D=({const char*_tmp9D="second argument of main has type ";_tag_fat(_tmp9D,sizeof(char),34U);});_tmp1DB.f1=_tmp28D;});_tmp1DB;});struct Cyc_Warn_Typ_Warn_Warg_struct _tmp9A=({struct Cyc_Warn_Typ_Warn_Warg_struct _tmp1DA;_tmp1DA.tag=2,_tmp1DA.f1=(void*)t2;_tmp1DA;});struct Cyc_Warn_String_Warn_Warg_struct _tmp9B=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1D9;_tmp1D9.tag=0,({
struct _fat_ptr _tmp28E=({const char*_tmp9C=" instead of char??";_tag_fat(_tmp9C,sizeof(char),19U);});_tmp1D9.f1=_tmp28E;});_tmp1D9;});void*_tmp98[3];_tmp98[0]=& _tmp99,_tmp98[1]=& _tmp9A,_tmp98[2]=& _tmp9B;({unsigned _tmp28F=loc;Cyc_Warn_err2(_tmp28F,_tag_fat(_tmp98,sizeof(void*),3));});});}}}}}}}}}}
# 314
static void Cyc_Tc_tcTypedefdecl(struct Cyc_Tcenv_Tenv*te,unsigned loc,struct Cyc_Absyn_Typedefdecl*td){
struct _tuple0*q=td->name;
# 320
if(((int(*)(struct Cyc_Dict_Dict,struct _tuple0*))Cyc_Dict_member)((te->ae)->typedefs,q)){
({struct Cyc_Warn_String_Warn_Warg_struct _tmpA1=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1DD;_tmp1DD.tag=0,({struct _fat_ptr _tmp290=({const char*_tmpA3="redeclaration of typedef ";_tag_fat(_tmpA3,sizeof(char),26U);});_tmp1DD.f1=_tmp290;});_tmp1DD;});struct Cyc_Warn_Qvar_Warn_Warg_struct _tmpA2=({struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp1DC;_tmp1DC.tag=1,_tmp1DC.f1=q;_tmp1DC;});void*_tmpA0[2];_tmpA0[0]=& _tmpA1,_tmpA0[1]=& _tmpA2;({unsigned _tmp291=loc;Cyc_Warn_err2(_tmp291,_tag_fat(_tmpA0,sizeof(void*),2));});});
return;}
# 325
Cyc_Tcutil_check_unique_tvars(loc,td->tvs);
Cyc_Tcutil_add_tvar_identities(td->tvs);
if(td->defn != 0){
Cyc_Tctyp_check_type(loc,te,td->tvs,& Cyc_Kinds_tak,0,1,(void*)_check_null(td->defn));
({int _tmp292=
Cyc_Tcutil_extract_const_from_typedef(loc,(td->tq).print_const,(void*)_check_null(td->defn));
# 329
(td->tq).real_const=_tmp292;});}
# 334
{struct Cyc_List_List*tvs=td->tvs;for(0;tvs != 0;tvs=tvs->tl){
void*_tmpA4=Cyc_Kinds_compress_kb(((struct Cyc_Absyn_Tvar*)tvs->hd)->kind);void*_stmttmp8=_tmpA4;void*_tmpA5=_stmttmp8;void*_tmpA7;void*_tmpA6;switch(*((int*)_tmpA5)){case 1: _tmpA6=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmpA5)->f1;{struct Cyc_Core_Opt**f=_tmpA6;
# 337
if(td->defn != 0)
({struct Cyc_Warn_String_Warn_Warg_struct _tmpA9=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1E0;_tmp1E0.tag=0,({struct _fat_ptr _tmp293=({const char*_tmpAD="type variable ";_tag_fat(_tmpAD,sizeof(char),15U);});_tmp1E0.f1=_tmp293;});_tmp1E0;});struct Cyc_Warn_Tvar_Warn_Warg_struct _tmpAA=({struct Cyc_Warn_Tvar_Warn_Warg_struct _tmp1DF;_tmp1DF.tag=7,_tmp1DF.f1=(struct Cyc_Absyn_Tvar*)tvs->hd;_tmp1DF;});struct Cyc_Warn_String_Warn_Warg_struct _tmpAB=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1DE;_tmp1DE.tag=0,({struct _fat_ptr _tmp294=({const char*_tmpAC=" is not used in typedef";_tag_fat(_tmpAC,sizeof(char),24U);});_tmp1DE.f1=_tmp294;});_tmp1DE;});void*_tmpA8[3];_tmpA8[0]=& _tmpA9,_tmpA8[1]=& _tmpAA,_tmpA8[2]=& _tmpAB;({unsigned _tmp295=loc;Cyc_Warn_warn2(_tmp295,_tag_fat(_tmpA8,sizeof(void*),3));});});
({struct Cyc_Core_Opt*_tmp297=({struct Cyc_Core_Opt*_tmpAE=_cycalloc(sizeof(*_tmpAE));({void*_tmp296=Cyc_Kinds_kind_to_bound(& Cyc_Kinds_tbk);_tmpAE->v=_tmp296;});_tmpAE;});*f=_tmp297;});goto _LL0;}case 2: _tmpA6=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmpA5)->f1;_tmpA7=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmpA5)->f2;{struct Cyc_Core_Opt**f=(struct Cyc_Core_Opt**)_tmpA6;struct Cyc_Absyn_Kind*k=_tmpA7;
# 346
({struct Cyc_Core_Opt*_tmp299=({struct Cyc_Core_Opt*_tmpAF=_cycalloc(sizeof(*_tmpAF));({void*_tmp298=Cyc_Kinds_kind_to_bound(k);_tmpAF->v=_tmp298;});_tmpAF;});*f=_tmp299;});
goto _LL0;}default:
 continue;}_LL0:;}}
# 351
({struct Cyc_Dict_Dict _tmp29A=((struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict,struct _tuple0*,struct Cyc_Absyn_Typedefdecl*))Cyc_Dict_insert)((te->ae)->typedefs,q,td);(te->ae)->typedefs=_tmp29A;});}struct _tuple14{void*f1;void*f2;};
# 354
static void Cyc_Tc_tcAggrImpl(struct Cyc_Tcenv_Tenv*te,unsigned loc,enum Cyc_Absyn_AggrKind str_or_union,struct Cyc_List_List*tvs,struct Cyc_List_List*rpo,struct Cyc_List_List*fields){
# 361
struct _RegionHandle _tmpB0=_new_region("uprev_rgn");struct _RegionHandle*uprev_rgn=& _tmpB0;_push_region(uprev_rgn);
# 363
for(1;rpo != 0;rpo=rpo->tl){
struct _tuple14*_tmpB1=(struct _tuple14*)rpo->hd;struct _tuple14*_stmttmp9=_tmpB1;struct _tuple14*_tmpB2=_stmttmp9;void*_tmpB4;void*_tmpB3;_tmpB3=_tmpB2->f1;_tmpB4=_tmpB2->f2;{void*e=_tmpB3;void*r=_tmpB4;
Cyc_Tctyp_check_type(loc,te,tvs,& Cyc_Kinds_ek,0,0,e);
Cyc_Tctyp_check_type(loc,te,tvs,& Cyc_Kinds_trk,0,0,r);}}{
# 369
struct Cyc_List_List*prev_fields=0;
struct Cyc_List_List*prev_relations=0;
# 372
struct Cyc_List_List*_tmpB5=fields;struct Cyc_List_List*fs=_tmpB5;for(0;fs != 0;fs=fs->tl){
struct Cyc_Absyn_Aggrfield*_tmpB6=(struct Cyc_Absyn_Aggrfield*)fs->hd;struct Cyc_Absyn_Aggrfield*_stmttmpA=_tmpB6;struct Cyc_Absyn_Aggrfield*_tmpB7=_stmttmpA;void*_tmpBD;void*_tmpBC;void*_tmpBB;void*_tmpBA;struct Cyc_Absyn_Tqual _tmpB9;void*_tmpB8;_tmpB8=_tmpB7->name;_tmpB9=_tmpB7->tq;_tmpBA=_tmpB7->type;_tmpBB=_tmpB7->width;_tmpBC=_tmpB7->attributes;_tmpBD=_tmpB7->requires_clause;{struct _fat_ptr*fn=_tmpB8;struct Cyc_Absyn_Tqual tq=_tmpB9;void*t=_tmpBA;struct Cyc_Absyn_Exp*width=_tmpBB;struct Cyc_List_List*atts=_tmpBC;struct Cyc_Absyn_Exp*requires_clause=_tmpBD;
# 375
if(((int(*)(int(*)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*,struct _fat_ptr*))Cyc_List_mem)(Cyc_strptrcmp,prev_fields,fn))
({struct Cyc_Warn_String_Warn_Warg_struct _tmpBF=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1E2;_tmp1E2.tag=0,({struct _fat_ptr _tmp29B=({const char*_tmpC1="duplicate member ";_tag_fat(_tmpC1,sizeof(char),18U);});_tmp1E2.f1=_tmp29B;});_tmp1E2;});struct Cyc_Warn_String_Warn_Warg_struct _tmpC0=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1E1;_tmp1E1.tag=0,_tmp1E1.f1=*fn;_tmp1E1;});void*_tmpBE[2];_tmpBE[0]=& _tmpBF,_tmpBE[1]=& _tmpC0;({unsigned _tmp29C=loc;Cyc_Warn_err2(_tmp29C,_tag_fat(_tmpBE,sizeof(void*),2));});});
# 379
if(({struct _fat_ptr _tmp29D=(struct _fat_ptr)*fn;Cyc_strcmp(_tmp29D,({const char*_tmpC2="";_tag_fat(_tmpC2,sizeof(char),1U);}));})!= 0)
prev_fields=({struct Cyc_List_List*_tmpC3=_region_malloc(uprev_rgn,sizeof(*_tmpC3));_tmpC3->hd=fn,_tmpC3->tl=prev_fields;_tmpC3;});{
# 382
struct Cyc_Absyn_Kind*field_kind=& Cyc_Kinds_tmk;
# 386
if((int)str_or_union == (int)1U ||
 fs->tl == 0 &&(int)str_or_union == (int)0U)
field_kind=& Cyc_Kinds_tak;
Cyc_Tctyp_check_type(loc,te,tvs,field_kind,0,0,t);
# 391
({int _tmp29E=Cyc_Tcutil_extract_const_from_typedef(loc,(((struct Cyc_Absyn_Aggrfield*)fs->hd)->tq).print_const,t);(((struct Cyc_Absyn_Aggrfield*)fs->hd)->tq).real_const=_tmp29E;});
# 394
Cyc_Tcutil_check_bitfield(loc,t,width,fn);
# 396
if((unsigned)requires_clause){
if((int)str_or_union != (int)1U)
({struct Cyc_Warn_String_Warn_Warg_struct _tmpC5=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1E3;_tmp1E3.tag=0,({struct _fat_ptr _tmp29F=({const char*_tmpC6="@requires clauses are allowed only on union members";_tag_fat(_tmpC6,sizeof(char),52U);});_tmp1E3.f1=_tmp29F;});_tmp1E3;});void*_tmpC4[1];_tmpC4[0]=& _tmpC5;({unsigned _tmp2A0=loc;Cyc_Warn_err2(_tmp2A0,_tag_fat(_tmpC4,sizeof(void*),1));});});{
struct Cyc_Tcenv_Tenv*_tmpC7=Cyc_Tcenv_allow_valueof(te);struct Cyc_Tcenv_Tenv*te2=_tmpC7;
Cyc_Tcexp_tcExp(te2,0,requires_clause);
if(!Cyc_Tcutil_is_integral(requires_clause))
({struct Cyc_Warn_String_Warn_Warg_struct _tmpC9=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1E6;_tmp1E6.tag=0,({
struct _fat_ptr _tmp2A1=({const char*_tmpCD="@requires clause has type ";_tag_fat(_tmpCD,sizeof(char),27U);});_tmp1E6.f1=_tmp2A1;});_tmp1E6;});struct Cyc_Warn_Typ_Warn_Warg_struct _tmpCA=({struct Cyc_Warn_Typ_Warn_Warg_struct _tmp1E5;_tmp1E5.tag=2,_tmp1E5.f1=(void*)_check_null(requires_clause->topt);_tmp1E5;});struct Cyc_Warn_String_Warn_Warg_struct _tmpCB=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1E4;_tmp1E4.tag=0,({
struct _fat_ptr _tmp2A2=({const char*_tmpCC=" instead of integral type";_tag_fat(_tmpCC,sizeof(char),26U);});_tmp1E4.f1=_tmp2A2;});_tmp1E4;});void*_tmpC8[3];_tmpC8[0]=& _tmpC9,_tmpC8[1]=& _tmpCA,_tmpC8[2]=& _tmpCB;({unsigned _tmp2A3=requires_clause->loc;Cyc_Warn_err2(_tmp2A3,_tag_fat(_tmpC8,sizeof(void*),3));});});else{
# 406
({unsigned _tmp2A6=requires_clause->loc;struct Cyc_Tcenv_Tenv*_tmp2A5=te;struct Cyc_List_List*_tmp2A4=tvs;Cyc_Tctyp_check_type(_tmp2A6,_tmp2A5,_tmp2A4,& Cyc_Kinds_ik,0,0,(void*)({struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_tmpCE=_cycalloc(sizeof(*_tmpCE));_tmpCE->tag=9,_tmpCE->f1=requires_clause;_tmpCE;}));});{
# 408
struct Cyc_List_List*_tmpCF=Cyc_Relations_exp2relns(uprev_rgn,requires_clause);struct Cyc_List_List*relns=_tmpCF;
# 411
if(!Cyc_Relations_consistent_relations(relns))
({struct Cyc_Warn_String_Warn_Warg_struct _tmpD1=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1E7;_tmp1E7.tag=0,({struct _fat_ptr _tmp2A7=({const char*_tmpD2="@requires clause may be unsatisfiable";_tag_fat(_tmpD2,sizeof(char),38U);});_tmp1E7.f1=_tmp2A7;});_tmp1E7;});void*_tmpD0[1];_tmpD0[0]=& _tmpD1;({unsigned _tmp2A8=requires_clause->loc;Cyc_Warn_err2(_tmp2A8,_tag_fat(_tmpD0,sizeof(void*),1));});});
# 417
{struct Cyc_List_List*_tmpD3=prev_relations;struct Cyc_List_List*p=_tmpD3;for(0;p != 0;p=p->tl){
if(Cyc_Relations_consistent_relations(((struct Cyc_List_List*(*)(struct _RegionHandle*,struct Cyc_List_List*,struct Cyc_List_List*))Cyc_List_rappend)(uprev_rgn,relns,(struct Cyc_List_List*)p->hd)))
({struct Cyc_Warn_String_Warn_Warg_struct _tmpD5=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1E8;_tmp1E8.tag=0,({
struct _fat_ptr _tmp2A9=({const char*_tmpD6="@requires clause may overlap with previous clauses";_tag_fat(_tmpD6,sizeof(char),51U);});_tmp1E8.f1=_tmp2A9;});_tmp1E8;});void*_tmpD4[1];_tmpD4[0]=& _tmpD5;({unsigned _tmp2AA=requires_clause->loc;Cyc_Warn_err2(_tmp2AA,_tag_fat(_tmpD4,sizeof(void*),1));});});}}
prev_relations=({struct Cyc_List_List*_tmpD7=_region_malloc(uprev_rgn,sizeof(*_tmpD7));_tmpD7->hd=relns,_tmpD7->tl=prev_relations;_tmpD7;});}}}}else{
# 424
if(prev_relations != 0)
({struct Cyc_Warn_String_Warn_Warg_struct _tmpD9=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1E9;_tmp1E9.tag=0,({struct _fat_ptr _tmp2AB=({const char*_tmpDA="if one field has a @requires clause, they all must";_tag_fat(_tmpDA,sizeof(char),51U);});_tmp1E9.f1=_tmp2AB;});_tmp1E9;});void*_tmpD8[1];_tmpD8[0]=& _tmpD9;({unsigned _tmp2AC=loc;Cyc_Warn_err2(_tmp2AC,_tag_fat(_tmpD8,sizeof(void*),1));});});}}}}}
# 363
;_pop_region();}
# 429
static void Cyc_Tc_rule_out_memkind(unsigned loc,struct _tuple0*n,struct Cyc_List_List*tvs,int constrain_top_kind){
# 431
struct Cyc_List_List*tvs2=tvs;for(0;tvs2 != 0;tvs2=tvs2->tl){
void*_tmpDB=Cyc_Kinds_compress_kb(((struct Cyc_Absyn_Tvar*)tvs2->hd)->kind);void*_stmttmpB=_tmpDB;void*_tmpDC=_stmttmpB;void*_tmpDF;enum Cyc_Absyn_AliasQual _tmpDD;void*_tmpDE;switch(*((int*)_tmpDC)){case 1: _tmpDE=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmpDC)->f1;{struct Cyc_Core_Opt**f=_tmpDE;
# 434
({struct Cyc_Core_Opt*_tmp2AE=({struct Cyc_Core_Opt*_tmpE0=_cycalloc(sizeof(*_tmpE0));({void*_tmp2AD=Cyc_Kinds_kind_to_bound(& Cyc_Kinds_bk);_tmpE0->v=_tmp2AD;});_tmpE0;});*f=_tmp2AE;});continue;}case 2: switch((int)((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmpDC)->f2)->kind){case Cyc_Absyn_MemKind: _tmpDE=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmpDC)->f1;_tmpDD=(((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmpDC)->f2)->aliasqual;{struct Cyc_Core_Opt**f=_tmpDE;enum Cyc_Absyn_AliasQual a=_tmpDD;
# 436
if(constrain_top_kind &&(int)a == (int)2U)
({struct Cyc_Core_Opt*_tmp2B0=({struct Cyc_Core_Opt*_tmpE2=_cycalloc(sizeof(*_tmpE2));({void*_tmp2AF=Cyc_Kinds_kind_to_bound(({struct Cyc_Absyn_Kind*_tmpE1=_cycalloc(sizeof(*_tmpE1));_tmpE1->kind=Cyc_Absyn_BoxKind,_tmpE1->aliasqual=Cyc_Absyn_Aliasable;_tmpE1;}));_tmpE2->v=_tmp2AF;});_tmpE2;});*f=_tmp2B0;});else{
# 439
({struct Cyc_Core_Opt*_tmp2B2=({struct Cyc_Core_Opt*_tmpE4=_cycalloc(sizeof(*_tmpE4));({void*_tmp2B1=Cyc_Kinds_kind_to_bound(({struct Cyc_Absyn_Kind*_tmpE3=_cycalloc(sizeof(*_tmpE3));_tmpE3->kind=Cyc_Absyn_BoxKind,_tmpE3->aliasqual=a;_tmpE3;}));_tmpE4->v=_tmp2B1;});_tmpE4;});*f=_tmp2B2;});}
continue;}case Cyc_Absyn_BoxKind: if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmpDC)->f2)->aliasqual == Cyc_Absyn_Top){_tmpDE=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmpDC)->f1;if(constrain_top_kind){struct Cyc_Core_Opt**f=_tmpDE;
# 442
({struct Cyc_Core_Opt*_tmp2B4=({struct Cyc_Core_Opt*_tmpE6=_cycalloc(sizeof(*_tmpE6));({void*_tmp2B3=Cyc_Kinds_kind_to_bound(({struct Cyc_Absyn_Kind*_tmpE5=_cycalloc(sizeof(*_tmpE5));_tmpE5->kind=Cyc_Absyn_BoxKind,_tmpE5->aliasqual=Cyc_Absyn_Aliasable;_tmpE5;}));_tmpE6->v=_tmp2B3;});_tmpE6;});*f=_tmp2B4;});
continue;}else{goto _LL7;}}else{goto _LL7;}default: _LL7: _tmpDE=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmpDC)->f1;_tmpDF=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmpDC)->f2;{struct Cyc_Core_Opt**f=(struct Cyc_Core_Opt**)_tmpDE;struct Cyc_Absyn_Kind*k=_tmpDF;
# 445
({struct Cyc_Core_Opt*_tmp2B6=({struct Cyc_Core_Opt*_tmpE7=_cycalloc(sizeof(*_tmpE7));({void*_tmp2B5=Cyc_Kinds_kind_to_bound(k);_tmpE7->v=_tmp2B5;});_tmpE7;});*f=_tmp2B6;});continue;}}default: if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmpDC)->f1)->kind == Cyc_Absyn_MemKind){_tmpDD=(((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmpDC)->f1)->aliasqual;{enum Cyc_Absyn_AliasQual a=_tmpDD;
# 447
({struct Cyc_Warn_String_Warn_Warg_struct _tmpE9=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1EF;_tmp1EF.tag=0,({struct _fat_ptr _tmp2B7=({const char*_tmpF2="type ";_tag_fat(_tmpF2,sizeof(char),6U);});_tmp1EF.f1=_tmp2B7;});_tmp1EF;});struct Cyc_Warn_Qvar_Warn_Warg_struct _tmpEA=({struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp1EE;_tmp1EE.tag=1,_tmp1EE.f1=n;_tmp1EE;});struct Cyc_Warn_String_Warn_Warg_struct _tmpEB=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1ED;_tmp1ED.tag=0,({struct _fat_ptr _tmp2B8=({const char*_tmpF1=" attempts to abstract type variable ";_tag_fat(_tmpF1,sizeof(char),37U);});_tmp1ED.f1=_tmp2B8;});_tmp1ED;});struct Cyc_Warn_Tvar_Warn_Warg_struct _tmpEC=({struct Cyc_Warn_Tvar_Warn_Warg_struct _tmp1EC;_tmp1EC.tag=7,_tmp1EC.f1=(struct Cyc_Absyn_Tvar*)tvs2->hd;_tmp1EC;});struct Cyc_Warn_String_Warn_Warg_struct _tmpED=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1EB;_tmp1EB.tag=0,({
struct _fat_ptr _tmp2B9=({const char*_tmpF0=" of kind ";_tag_fat(_tmpF0,sizeof(char),10U);});_tmp1EB.f1=_tmp2B9;});_tmp1EB;});struct Cyc_Warn_Kind_Warn_Warg_struct _tmpEE=({struct Cyc_Warn_Kind_Warn_Warg_struct _tmp1EA;_tmp1EA.tag=9,({struct Cyc_Absyn_Kind*_tmp2BA=({struct Cyc_Absyn_Kind*_tmpEF=_cycalloc(sizeof(*_tmpEF));_tmpEF->kind=Cyc_Absyn_MemKind,_tmpEF->aliasqual=a;_tmpEF;});_tmp1EA.f1=_tmp2BA;});_tmp1EA;});void*_tmpE8[6];_tmpE8[0]=& _tmpE9,_tmpE8[1]=& _tmpEA,_tmpE8[2]=& _tmpEB,_tmpE8[3]=& _tmpEC,_tmpE8[4]=& _tmpED,_tmpE8[5]=& _tmpEE;({unsigned _tmp2BB=loc;Cyc_Warn_err2(_tmp2BB,_tag_fat(_tmpE8,sizeof(void*),6));});});
continue;}}else{
continue;}};}}
# 453
static void Cyc_Tc_rule_out_mem_and_unique(unsigned loc,struct _tuple0*q,struct Cyc_List_List*tvs){
struct Cyc_List_List*tvs2=tvs;for(0;tvs2 != 0;tvs2=tvs2->tl){
void*_tmpF3=Cyc_Kinds_compress_kb(((struct Cyc_Absyn_Tvar*)tvs2->hd)->kind);void*_stmttmpC=_tmpF3;void*_tmpF4=_stmttmpC;enum Cyc_Absyn_AliasQual _tmpF5;enum Cyc_Absyn_KindQual _tmpF6;void*_tmpF7;switch(*((int*)_tmpF4)){case 1: _tmpF7=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmpF4)->f1;{struct Cyc_Core_Opt**f=_tmpF7;
_tmpF7=f;goto _LL4;}case 2: switch((int)((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmpF4)->f2)->kind){case Cyc_Absyn_MemKind: switch((int)((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmpF4)->f2)->aliasqual){case Cyc_Absyn_Top: _tmpF7=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmpF4)->f1;_LL4: {struct Cyc_Core_Opt**f=_tmpF7;
# 458
_tmpF7=f;goto _LL6;}case Cyc_Absyn_Aliasable: _tmpF7=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmpF4)->f1;_LL6: {struct Cyc_Core_Opt**f=_tmpF7;
# 460
({struct Cyc_Core_Opt*_tmp2BD=({struct Cyc_Core_Opt*_tmpF8=_cycalloc(sizeof(*_tmpF8));({void*_tmp2BC=Cyc_Kinds_kind_to_bound(& Cyc_Kinds_bk);_tmpF8->v=_tmp2BC;});_tmpF8;});*f=_tmp2BD;});goto _LL0;}case Cyc_Absyn_Unique: goto _LLF;default: goto _LL15;}case Cyc_Absyn_AnyKind: switch((int)((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmpF4)->f2)->aliasqual){case Cyc_Absyn_Top: _tmpF7=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmpF4)->f1;{struct Cyc_Core_Opt**f=_tmpF7;
_tmpF7=f;goto _LLA;}case Cyc_Absyn_Aliasable: _tmpF7=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmpF4)->f1;_LLA: {struct Cyc_Core_Opt**f=_tmpF7;
# 463
({struct Cyc_Core_Opt*_tmp2BF=({struct Cyc_Core_Opt*_tmpF9=_cycalloc(sizeof(*_tmpF9));({void*_tmp2BE=Cyc_Kinds_kind_to_bound(& Cyc_Kinds_ak);_tmpF9->v=_tmp2BE;});_tmpF9;});*f=_tmp2BF;});goto _LL0;}case Cyc_Absyn_Unique: goto _LLF;default: goto _LL15;}case Cyc_Absyn_RgnKind: switch((int)((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmpF4)->f2)->aliasqual){case Cyc_Absyn_Top: _tmpF7=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmpF4)->f1;{struct Cyc_Core_Opt**f=_tmpF7;
# 465
({struct Cyc_Core_Opt*_tmp2C1=({struct Cyc_Core_Opt*_tmpFA=_cycalloc(sizeof(*_tmpFA));({void*_tmp2C0=Cyc_Kinds_kind_to_bound(& Cyc_Kinds_rk);_tmpFA->v=_tmp2C0;});_tmpFA;});*f=_tmp2C1;});goto _LL0;}case Cyc_Absyn_Unique: goto _LLF;default: goto _LL15;}default: if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmpF4)->f2)->aliasqual == Cyc_Absyn_Unique){_LLF: _tmpF7=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmpF4)->f1;_tmpF6=(((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmpF4)->f2)->kind;{struct Cyc_Core_Opt**f=(struct Cyc_Core_Opt**)_tmpF7;enum Cyc_Absyn_KindQual k=_tmpF6;
# 469
_tmpF6=k;goto _LL12;}}else{goto _LL15;}}default: if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmpF4)->f1)->kind == Cyc_Absyn_RgnKind)switch((int)((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmpF4)->f1)->aliasqual){case Cyc_Absyn_Top:
# 467
({struct Cyc_Warn_String_Warn_Warg_struct _tmpFC=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1F4;_tmp1F4.tag=0,({struct _fat_ptr _tmp2C2=({const char*_tmp103="type ";_tag_fat(_tmp103,sizeof(char),6U);});_tmp1F4.f1=_tmp2C2;});_tmp1F4;});struct Cyc_Warn_Qvar_Warn_Warg_struct _tmpFD=({struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp1F3;_tmp1F3.tag=1,_tmp1F3.f1=q;_tmp1F3;});struct Cyc_Warn_String_Warn_Warg_struct _tmpFE=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1F2;_tmp1F2.tag=0,({struct _fat_ptr _tmp2C3=({const char*_tmp102=" attempts to abstract type variable ";_tag_fat(_tmp102,sizeof(char),37U);});_tmp1F2.f1=_tmp2C3;});_tmp1F2;});struct Cyc_Warn_Tvar_Warn_Warg_struct _tmpFF=({struct Cyc_Warn_Tvar_Warn_Warg_struct _tmp1F1;_tmp1F1.tag=7,_tmp1F1.f1=(struct Cyc_Absyn_Tvar*)tvs2->hd;_tmp1F1;});struct Cyc_Warn_String_Warn_Warg_struct _tmp100=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1F0;_tmp1F0.tag=0,({
struct _fat_ptr _tmp2C4=({const char*_tmp101=" of kind TR";_tag_fat(_tmp101,sizeof(char),12U);});_tmp1F0.f1=_tmp2C4;});_tmp1F0;});void*_tmpFB[5];_tmpFB[0]=& _tmpFC,_tmpFB[1]=& _tmpFD,_tmpFB[2]=& _tmpFE,_tmpFB[3]=& _tmpFF,_tmpFB[4]=& _tmp100;({unsigned _tmp2C5=loc;Cyc_Warn_err2(_tmp2C5,_tag_fat(_tmpFB,sizeof(void*),5));});});goto _LL0;case Cyc_Absyn_Unique: goto _LL11;default: goto _LL15;}else{if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmpF4)->f1)->aliasqual == Cyc_Absyn_Unique){_LL11: _tmpF6=(((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmpF4)->f1)->kind;_LL12: {enum Cyc_Absyn_KindQual k=_tmpF6;
# 471
({struct Cyc_Warn_String_Warn_Warg_struct _tmp105=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1FA;_tmp1FA.tag=0,({struct _fat_ptr _tmp2C6=({const char*_tmp10E="type ";_tag_fat(_tmp10E,sizeof(char),6U);});_tmp1FA.f1=_tmp2C6;});_tmp1FA;});struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp106=({struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp1F9;_tmp1F9.tag=1,_tmp1F9.f1=q;_tmp1F9;});struct Cyc_Warn_String_Warn_Warg_struct _tmp107=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1F8;_tmp1F8.tag=0,({struct _fat_ptr _tmp2C7=({const char*_tmp10D=" attempts to abstract type variable ";_tag_fat(_tmp10D,sizeof(char),37U);});_tmp1F8.f1=_tmp2C7;});_tmp1F8;});struct Cyc_Warn_Tvar_Warn_Warg_struct _tmp108=({struct Cyc_Warn_Tvar_Warn_Warg_struct _tmp1F7;_tmp1F7.tag=7,_tmp1F7.f1=(struct Cyc_Absyn_Tvar*)tvs2->hd;_tmp1F7;});struct Cyc_Warn_String_Warn_Warg_struct _tmp109=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1F6;_tmp1F6.tag=0,({
struct _fat_ptr _tmp2C8=({const char*_tmp10C=" of kind ";_tag_fat(_tmp10C,sizeof(char),10U);});_tmp1F6.f1=_tmp2C8;});_tmp1F6;});struct Cyc_Warn_Kind_Warn_Warg_struct _tmp10A=({struct Cyc_Warn_Kind_Warn_Warg_struct _tmp1F5;_tmp1F5.tag=9,({struct Cyc_Absyn_Kind*_tmp2C9=({struct Cyc_Absyn_Kind*_tmp10B=_cycalloc(sizeof(*_tmp10B));_tmp10B->kind=k,_tmp10B->aliasqual=Cyc_Absyn_Unique;_tmp10B;});_tmp1F5.f1=_tmp2C9;});_tmp1F5;});void*_tmp104[6];_tmp104[0]=& _tmp105,_tmp104[1]=& _tmp106,_tmp104[2]=& _tmp107,_tmp104[3]=& _tmp108,_tmp104[4]=& _tmp109,_tmp104[5]=& _tmp10A;({unsigned _tmp2CA=loc;Cyc_Warn_err2(_tmp2CA,_tag_fat(_tmp104,sizeof(void*),6));});});goto _LL0;}}else{if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmpF4)->f1)->kind == Cyc_Absyn_MemKind){_tmpF5=(((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmpF4)->f1)->aliasqual;{enum Cyc_Absyn_AliasQual a=_tmpF5;
# 474
({struct Cyc_Warn_String_Warn_Warg_struct _tmp110=({struct Cyc_Warn_String_Warn_Warg_struct _tmp200;_tmp200.tag=0,({struct _fat_ptr _tmp2CB=({const char*_tmp119="type ";_tag_fat(_tmp119,sizeof(char),6U);});_tmp200.f1=_tmp2CB;});_tmp200;});struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp111=({struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp1FF;_tmp1FF.tag=1,_tmp1FF.f1=q;_tmp1FF;});struct Cyc_Warn_String_Warn_Warg_struct _tmp112=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1FE;_tmp1FE.tag=0,({struct _fat_ptr _tmp2CC=({const char*_tmp118=" attempts to abstract type variable ";_tag_fat(_tmp118,sizeof(char),37U);});_tmp1FE.f1=_tmp2CC;});_tmp1FE;});struct Cyc_Warn_Tvar_Warn_Warg_struct _tmp113=({struct Cyc_Warn_Tvar_Warn_Warg_struct _tmp1FD;_tmp1FD.tag=7,_tmp1FD.f1=(struct Cyc_Absyn_Tvar*)tvs2->hd;_tmp1FD;});struct Cyc_Warn_String_Warn_Warg_struct _tmp114=({struct Cyc_Warn_String_Warn_Warg_struct _tmp1FC;_tmp1FC.tag=0,({
struct _fat_ptr _tmp2CD=({const char*_tmp117=" of kind ";_tag_fat(_tmp117,sizeof(char),10U);});_tmp1FC.f1=_tmp2CD;});_tmp1FC;});struct Cyc_Warn_Kind_Warn_Warg_struct _tmp115=({struct Cyc_Warn_Kind_Warn_Warg_struct _tmp1FB;_tmp1FB.tag=9,({struct Cyc_Absyn_Kind*_tmp2CE=({struct Cyc_Absyn_Kind*_tmp116=_cycalloc(sizeof(*_tmp116));_tmp116->kind=Cyc_Absyn_MemKind,_tmp116->aliasqual=a;_tmp116;});_tmp1FB.f1=_tmp2CE;});_tmp1FB;});void*_tmp10F[6];_tmp10F[0]=& _tmp110,_tmp10F[1]=& _tmp111,_tmp10F[2]=& _tmp112,_tmp10F[3]=& _tmp113,_tmp10F[4]=& _tmp114,_tmp10F[5]=& _tmp115;({unsigned _tmp2CF=loc;Cyc_Warn_err2(_tmp2CF,_tag_fat(_tmp10F,sizeof(void*),6));});});goto _LL0;}}else{_LL15:
 goto _LL0;}}}}_LL0:;}}struct _tuple15{struct Cyc_Absyn_AggrdeclImpl*f1;struct Cyc_Absyn_Aggrdecl***f2;};
# 482
void Cyc_Tc_tcAggrdecl(struct Cyc_Tcenv_Tenv*te,unsigned loc,struct Cyc_Absyn_Aggrdecl*ad){
struct _tuple0*_tmp11A=ad->name;struct _tuple0*q=_tmp11A;
# 488
Cyc_Atts_check_field_atts(loc,(*q).f2,ad->attributes);{
# 490
struct Cyc_List_List*_tmp11B=ad->tvs;struct Cyc_List_List*tvs=_tmp11B;
# 493
Cyc_Tcutil_check_unique_tvars(loc,ad->tvs);
Cyc_Tcutil_add_tvar_identities(ad->tvs);{
# 498
struct _tuple15 _tmp11C=({struct _tuple15 _tmp20B;_tmp20B.f1=ad->impl,({struct Cyc_Absyn_Aggrdecl***_tmp2D0=((struct Cyc_Absyn_Aggrdecl***(*)(struct Cyc_Dict_Dict,struct _tuple0*))Cyc_Dict_lookup_opt)((te->ae)->aggrdecls,q);_tmp20B.f2=_tmp2D0;});_tmp20B;});struct _tuple15 _stmttmpD=_tmp11C;struct _tuple15 _tmp11D=_stmttmpD;void*_tmp122;int _tmp121;void*_tmp120;void*_tmp11F;void*_tmp11E;if(_tmp11D.f1 == 0){if(_tmp11D.f2 == 0){
# 501
Cyc_Tc_rule_out_memkind(loc,q,tvs,0);
# 503
({struct Cyc_Dict_Dict _tmp2D3=({struct Cyc_Dict_Dict _tmp2D2=(te->ae)->aggrdecls;struct _tuple0*_tmp2D1=q;((struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict,struct _tuple0*,struct Cyc_Absyn_Aggrdecl**))Cyc_Dict_insert)(_tmp2D2,_tmp2D1,({struct Cyc_Absyn_Aggrdecl**_tmp123=_cycalloc(sizeof(*_tmp123));*_tmp123=ad;_tmp123;}));});(te->ae)->aggrdecls=_tmp2D3;});
goto _LL0;}else{_tmp11E=*_tmp11D.f2;_LL8: {struct Cyc_Absyn_Aggrdecl**adp=_tmp11E;
# 562
struct Cyc_Absyn_Aggrdecl*_tmp13E=Cyc_Tcdecl_merge_aggrdecl(*adp,ad,loc,Cyc_Tc_tc_msg);struct Cyc_Absyn_Aggrdecl*ad2=_tmp13E;
if(ad2 == 0)
return;
Cyc_Tc_rule_out_memkind(loc,q,tvs,0);
# 568
if(ad->impl != 0)
Cyc_Tc_rule_out_memkind(loc,q,((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->exist_vars,1);
*adp=ad2;}}}else{if(_tmp11D.f2 == 0){_tmp11E=(_tmp11D.f1)->exist_vars;_tmp11F=(_tmp11D.f1)->rgn_po;_tmp120=(_tmp11D.f1)->fields;_tmp121=(_tmp11D.f1)->tagged;{struct Cyc_List_List*exist_vars=_tmp11E;struct Cyc_List_List*rgn_po=_tmp11F;struct Cyc_List_List*fs=_tmp120;int tagged=_tmp121;
# 508
struct Cyc_Absyn_Aggrdecl**_tmp124=({struct Cyc_Absyn_Aggrdecl**_tmp135=_cycalloc(sizeof(*_tmp135));({struct Cyc_Absyn_Aggrdecl*_tmp2D4=({struct Cyc_Absyn_Aggrdecl*_tmp134=_cycalloc(sizeof(*_tmp134));_tmp134->kind=ad->kind,_tmp134->sc=Cyc_Absyn_Extern,_tmp134->name=ad->name,_tmp134->tvs=tvs,_tmp134->impl=0,_tmp134->attributes=ad->attributes,_tmp134->expected_mem_kind=0;_tmp134;});*_tmp135=_tmp2D4;});_tmp135;});struct Cyc_Absyn_Aggrdecl**adp=_tmp124;
# 510
({struct Cyc_Dict_Dict _tmp2D5=((struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict,struct _tuple0*,struct Cyc_Absyn_Aggrdecl**))Cyc_Dict_insert)((te->ae)->aggrdecls,q,adp);(te->ae)->aggrdecls=_tmp2D5;});
# 515
Cyc_Tcutil_check_unique_tvars(loc,exist_vars);
Cyc_Tcutil_add_tvar_identities(exist_vars);
# 519
if(tagged &&(int)ad->kind == (int)Cyc_Absyn_StructA)
({struct Cyc_Warn_String_Warn_Warg_struct _tmp126=({struct Cyc_Warn_String_Warn_Warg_struct _tmp201;_tmp201.tag=0,({struct _fat_ptr _tmp2D6=({const char*_tmp127="@tagged is allowed only on union declarations";_tag_fat(_tmp127,sizeof(char),46U);});_tmp201.f1=_tmp2D6;});_tmp201;});void*_tmp125[1];_tmp125[0]=& _tmp126;({unsigned _tmp2D7=loc;Cyc_Warn_err2(_tmp2D7,_tag_fat(_tmp125,sizeof(void*),1));});});
({struct Cyc_Tcenv_Tenv*_tmp2DC=te;unsigned _tmp2DB=loc;enum Cyc_Absyn_AggrKind _tmp2DA=ad->kind;struct Cyc_List_List*_tmp2D9=((struct Cyc_List_List*(*)(struct Cyc_List_List*,struct Cyc_List_List*))Cyc_List_append)(tvs,exist_vars);struct Cyc_List_List*_tmp2D8=rgn_po;Cyc_Tc_tcAggrImpl(_tmp2DC,_tmp2DB,_tmp2DA,_tmp2D9,_tmp2D8,fs);});
# 523
Cyc_Tc_rule_out_memkind(loc,q,tvs,0);
# 526
Cyc_Tc_rule_out_memkind(loc,q,exist_vars,1);
# 528
if((int)ad->kind == (int)Cyc_Absyn_UnionA && !tagged){
# 531
struct Cyc_List_List*f=fs;for(0;f != 0;f=f->tl){
if((Cyc_Flags_tc_aggressive_warn && !
Cyc_Tcutil_is_bits_only_type(((struct Cyc_Absyn_Aggrfield*)f->hd)->type))&&((struct Cyc_Absyn_Aggrfield*)f->hd)->requires_clause == 0)
({struct Cyc_Warn_String_Warn_Warg_struct _tmp129=({struct Cyc_Warn_String_Warn_Warg_struct _tmp208;_tmp208.tag=0,({struct _fat_ptr _tmp2DD=({const char*_tmp133="member ";_tag_fat(_tmp133,sizeof(char),8U);});_tmp208.f1=_tmp2DD;});_tmp208;});struct Cyc_Warn_String_Warn_Warg_struct _tmp12A=({struct Cyc_Warn_String_Warn_Warg_struct _tmp207;_tmp207.tag=0,_tmp207.f1=*((struct Cyc_Absyn_Aggrfield*)f->hd)->name;_tmp207;});struct Cyc_Warn_String_Warn_Warg_struct _tmp12B=({struct Cyc_Warn_String_Warn_Warg_struct _tmp206;_tmp206.tag=0,({struct _fat_ptr _tmp2DE=({const char*_tmp132=" of union ";_tag_fat(_tmp132,sizeof(char),11U);});_tmp206.f1=_tmp2DE;});_tmp206;});struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp12C=({struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp205;_tmp205.tag=1,_tmp205.f1=q;_tmp205;});struct Cyc_Warn_String_Warn_Warg_struct _tmp12D=({struct Cyc_Warn_String_Warn_Warg_struct _tmp204;_tmp204.tag=0,({struct _fat_ptr _tmp2DF=({const char*_tmp131=" has type ";_tag_fat(_tmp131,sizeof(char),11U);});_tmp204.f1=_tmp2DF;});_tmp204;});struct Cyc_Warn_Typ_Warn_Warg_struct _tmp12E=({struct Cyc_Warn_Typ_Warn_Warg_struct _tmp203;_tmp203.tag=2,_tmp203.f1=(void*)((struct Cyc_Absyn_Aggrfield*)f->hd)->type;_tmp203;});struct Cyc_Warn_String_Warn_Warg_struct _tmp12F=({struct Cyc_Warn_String_Warn_Warg_struct _tmp202;_tmp202.tag=0,({
struct _fat_ptr _tmp2E0=({const char*_tmp130=" so it can only be written and not read";_tag_fat(_tmp130,sizeof(char),40U);});_tmp202.f1=_tmp2E0;});_tmp202;});void*_tmp128[7];_tmp128[0]=& _tmp129,_tmp128[1]=& _tmp12A,_tmp128[2]=& _tmp12B,_tmp128[3]=& _tmp12C,_tmp128[4]=& _tmp12D,_tmp128[5]=& _tmp12E,_tmp128[6]=& _tmp12F;({unsigned _tmp2E1=loc;Cyc_Warn_warn2(_tmp2E1,_tag_fat(_tmp128,sizeof(void*),7));});});}}
*adp=ad;
goto _LL0;}}else{_tmp11E=(_tmp11D.f1)->exist_vars;_tmp11F=(_tmp11D.f1)->rgn_po;_tmp120=(_tmp11D.f1)->fields;_tmp121=(_tmp11D.f1)->tagged;_tmp122=*_tmp11D.f2;{struct Cyc_List_List*exist_vars=_tmp11E;struct Cyc_List_List*rgn_po=_tmp11F;struct Cyc_List_List*fs=_tmp120;int tagged=_tmp121;struct Cyc_Absyn_Aggrdecl**adp=_tmp122;
# 540
if((int)ad->kind != (int)(*adp)->kind)
({struct Cyc_Warn_String_Warn_Warg_struct _tmp137=({struct Cyc_Warn_String_Warn_Warg_struct _tmp209;_tmp209.tag=0,({struct _fat_ptr _tmp2E2=({const char*_tmp138="cannot reuse struct names for unions and vice-versa";_tag_fat(_tmp138,sizeof(char),52U);});_tmp209.f1=_tmp2E2;});_tmp209;});void*_tmp136[1];_tmp136[0]=& _tmp137;({unsigned _tmp2E3=loc;Cyc_Warn_err2(_tmp2E3,_tag_fat(_tmp136,sizeof(void*),1));});});{
# 543
struct Cyc_Absyn_Aggrdecl*_tmp139=*adp;struct Cyc_Absyn_Aggrdecl*ad0=_tmp139;
# 545
({struct Cyc_Absyn_Aggrdecl*_tmp2E4=({struct Cyc_Absyn_Aggrdecl*_tmp13A=_cycalloc(sizeof(*_tmp13A));_tmp13A->kind=ad->kind,_tmp13A->sc=Cyc_Absyn_Extern,_tmp13A->name=ad->name,_tmp13A->tvs=tvs,_tmp13A->impl=0,_tmp13A->attributes=ad->attributes,_tmp13A->expected_mem_kind=0;_tmp13A;});*adp=_tmp2E4;});
# 551
Cyc_Tcutil_check_unique_tvars(loc,exist_vars);
Cyc_Tcutil_add_tvar_identities(exist_vars);
# 554
if(tagged &&(int)ad->kind == (int)Cyc_Absyn_StructA)
({struct Cyc_Warn_String_Warn_Warg_struct _tmp13C=({struct Cyc_Warn_String_Warn_Warg_struct _tmp20A;_tmp20A.tag=0,({struct _fat_ptr _tmp2E5=({const char*_tmp13D="@tagged is allowed only on union declarations";_tag_fat(_tmp13D,sizeof(char),46U);});_tmp20A.f1=_tmp2E5;});_tmp20A;});void*_tmp13B[1];_tmp13B[0]=& _tmp13C;({unsigned _tmp2E6=loc;Cyc_Warn_err2(_tmp2E6,_tag_fat(_tmp13B,sizeof(void*),1));});});
({struct Cyc_Tcenv_Tenv*_tmp2EB=te;unsigned _tmp2EA=loc;enum Cyc_Absyn_AggrKind _tmp2E9=ad->kind;struct Cyc_List_List*_tmp2E8=((struct Cyc_List_List*(*)(struct Cyc_List_List*,struct Cyc_List_List*))Cyc_List_append)(tvs,exist_vars);struct Cyc_List_List*_tmp2E7=rgn_po;Cyc_Tc_tcAggrImpl(_tmp2EB,_tmp2EA,_tmp2E9,_tmp2E8,_tmp2E7,fs);});
# 558
*adp=ad0;
_tmp11E=adp;goto _LL8;}}}}_LL0:;}}}struct _tuple16{struct Cyc_Absyn_Tqual f1;void*f2;};
# 574
static struct Cyc_List_List*Cyc_Tc_tcDatatypeFields(struct Cyc_Tcenv_Tenv*te,unsigned loc,struct _fat_ptr obj,int is_extensible,struct _tuple0*name,struct Cyc_List_List*fields,struct Cyc_List_List*tvs,struct Cyc_Absyn_Datatypedecl*tudres){
# 582
{struct Cyc_List_List*_tmp13F=fields;struct Cyc_List_List*fs=_tmp13F;for(0;fs != 0;fs=fs->tl){
struct Cyc_Absyn_Datatypefield*_tmp140=(struct Cyc_Absyn_Datatypefield*)fs->hd;struct Cyc_Absyn_Datatypefield*f=_tmp140;
struct Cyc_List_List*typs=f->typs;for(0;typs != 0;typs=typs->tl){
Cyc_Tctyp_check_type(f->loc,te,tvs,& Cyc_Kinds_tmk,0,0,(*((struct _tuple16*)typs->hd)).f2);
# 587
if(Cyc_Tcutil_is_noalias_pointer_or_aggr((*((struct _tuple16*)typs->hd)).f2))
({struct Cyc_Warn_String_Warn_Warg_struct _tmp142=({struct Cyc_Warn_String_Warn_Warg_struct _tmp20D;_tmp20D.tag=0,({struct _fat_ptr _tmp2EC=({const char*_tmp144="noalias pointers in datatypes are not allowed: ";_tag_fat(_tmp144,sizeof(char),48U);});_tmp20D.f1=_tmp2EC;});_tmp20D;});struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp143=({struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp20C;_tmp20C.tag=1,_tmp20C.f1=f->name;_tmp20C;});void*_tmp141[2];_tmp141[0]=& _tmp142,_tmp141[1]=& _tmp143;({unsigned _tmp2ED=f->loc;Cyc_Warn_err2(_tmp2ED,_tag_fat(_tmp141,sizeof(void*),2));});});
# 590
({int _tmp2EE=
Cyc_Tcutil_extract_const_from_typedef(f->loc,((*((struct _tuple16*)typs->hd)).f1).print_const,(*((struct _tuple16*)typs->hd)).f2);
# 590
((*((struct _tuple16*)typs->hd)).f1).real_const=_tmp2EE;});}}}
# 596
if(is_extensible){
# 598
int _tmp145=1;int res=_tmp145;
struct Cyc_List_List*_tmp146=Cyc_Tcdecl_sort_xdatatype_fields(fields,& res,(*name).f2,loc,Cyc_Tc_tc_msg);struct Cyc_List_List*fs=_tmp146;
return res?fs: 0;}{
# 602
struct _RegionHandle _tmp147=_new_region("uprev_rgn");struct _RegionHandle*uprev_rgn=& _tmp147;_push_region(uprev_rgn);
# 604
{struct Cyc_List_List*prev_fields=0;
{struct Cyc_List_List*fs=fields;for(0;fs != 0;fs=fs->tl){
struct Cyc_Absyn_Datatypefield*_tmp148=(struct Cyc_Absyn_Datatypefield*)fs->hd;struct Cyc_Absyn_Datatypefield*f=_tmp148;
if(((int(*)(int(*)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*,struct _fat_ptr*))Cyc_List_mem)(Cyc_strptrcmp,prev_fields,(*f->name).f2))
({struct Cyc_Warn_String_Warn_Warg_struct _tmp14A=({struct Cyc_Warn_String_Warn_Warg_struct _tmp211;_tmp211.tag=0,({struct _fat_ptr _tmp2EF=({const char*_tmp14F="duplicate field ";_tag_fat(_tmp14F,sizeof(char),17U);});_tmp211.f1=_tmp2EF;});_tmp211;});struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp14B=({struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp210;_tmp210.tag=1,_tmp210.f1=f->name;_tmp210;});struct Cyc_Warn_String_Warn_Warg_struct _tmp14C=({struct Cyc_Warn_String_Warn_Warg_struct _tmp20F;_tmp20F.tag=0,({struct _fat_ptr _tmp2F0=({const char*_tmp14E=" in ";_tag_fat(_tmp14E,sizeof(char),5U);});_tmp20F.f1=_tmp2F0;});_tmp20F;});struct Cyc_Warn_String_Warn_Warg_struct _tmp14D=({struct Cyc_Warn_String_Warn_Warg_struct _tmp20E;_tmp20E.tag=0,_tmp20E.f1=obj;_tmp20E;});void*_tmp149[4];_tmp149[0]=& _tmp14A,_tmp149[1]=& _tmp14B,_tmp149[2]=& _tmp14C,_tmp149[3]=& _tmp14D;({unsigned _tmp2F1=f->loc;Cyc_Warn_err2(_tmp2F1,_tag_fat(_tmp149,sizeof(void*),4));});});else{
# 610
prev_fields=({struct Cyc_List_List*_tmp150=_region_malloc(uprev_rgn,sizeof(*_tmp150));_tmp150->hd=(*f->name).f2,_tmp150->tl=prev_fields;_tmp150;});}
# 612
if((int)f->sc != (int)Cyc_Absyn_Public){
({struct Cyc_Warn_String_Warn_Warg_struct _tmp152=({struct Cyc_Warn_String_Warn_Warg_struct _tmp213;_tmp213.tag=0,({struct _fat_ptr _tmp2F2=({const char*_tmp154="ignoring scope of field ";_tag_fat(_tmp154,sizeof(char),25U);});_tmp213.f1=_tmp2F2;});_tmp213;});struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp153=({struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp212;_tmp212.tag=1,_tmp212.f1=f->name;_tmp212;});void*_tmp151[2];_tmp151[0]=& _tmp152,_tmp151[1]=& _tmp153;({unsigned _tmp2F3=loc;Cyc_Warn_warn2(_tmp2F3,_tag_fat(_tmp151,sizeof(void*),2));});});
f->sc=Cyc_Absyn_Public;}}}{
# 617
struct Cyc_List_List*_tmp155=fields;_npop_handler(0);return _tmp155;}}
# 604
;_pop_region();}}struct _tuple17{struct Cyc_Core_Opt*f1;struct Cyc_Absyn_Datatypedecl***f2;};
# 620
void Cyc_Tc_tcDatatypedecl(struct Cyc_Tcenv_Tenv*te,unsigned loc,struct Cyc_Absyn_Datatypedecl*tud){
struct _tuple0*q=tud->name;
struct _fat_ptr obj=tud->is_extensible?({const char*_tmp169="@extensible datatype";_tag_fat(_tmp169,sizeof(char),21U);}):({const char*_tmp16A="datatype";_tag_fat(_tmp16A,sizeof(char),9U);});
# 626
struct Cyc_List_List*tvs=tud->tvs;
# 629
Cyc_Tcutil_check_unique_tvars(loc,tvs);
Cyc_Tcutil_add_tvar_identities(tvs);{
# 635
struct Cyc_Absyn_Datatypedecl***tud_opt;
# 645 "tc.cyc"
{struct _handler_cons _tmp156;_push_handler(& _tmp156);{int _tmp158=0;if(setjmp(_tmp156.handler))_tmp158=1;if(!_tmp158){
tud_opt=Cyc_Tcenv_lookup_xdatatypedecl(Cyc_Core_heap_region,te,loc,tud->name);
if(tud_opt == 0 && !tud->is_extensible)(int)_throw((void*)({struct Cyc_Dict_Absent_exn_struct*_tmp159=_cycalloc(sizeof(*_tmp159));_tmp159->tag=Cyc_Dict_Absent;_tmp159;}));
if(tud_opt != 0)
tud->name=(*(*tud_opt))->name;else{
# 651
({union Cyc_Absyn_Nmspace _tmp2F4=Cyc_Absyn_Abs_n(te->ns,0);(*tud->name).f1=_tmp2F4;});}
# 646
;_pop_handler();}else{void*_tmp157=(void*)Cyc_Core_get_exn_thrown();void*_tmp15A=_tmp157;void*_tmp15B;if(((struct Cyc_Dict_Absent_exn_struct*)_tmp15A)->tag == Cyc_Dict_Absent){
# 653
struct Cyc_Absyn_Datatypedecl***_tmp15C=((struct Cyc_Absyn_Datatypedecl***(*)(struct Cyc_Dict_Dict,struct _tuple0*))Cyc_Dict_lookup_opt)((te->ae)->datatypedecls,q);struct Cyc_Absyn_Datatypedecl***tdopt=_tmp15C;
tud_opt=(unsigned)tdopt?({struct Cyc_Absyn_Datatypedecl***_tmp15D=_cycalloc(sizeof(*_tmp15D));*_tmp15D=*tdopt;_tmp15D;}): 0;
goto _LL0;}else{_tmp15B=_tmp15A;{void*exn=_tmp15B;(int)_rethrow(exn);}}_LL0:;}}}{
# 660
struct _tuple17 _tmp15E=({struct _tuple17 _tmp214;_tmp214.f1=tud->fields,_tmp214.f2=tud_opt;_tmp214;});struct _tuple17 _stmttmpE=_tmp15E;struct _tuple17 _tmp15F=_stmttmpE;void*_tmp161;void*_tmp160;if(_tmp15F.f1 == 0){if(_tmp15F.f2 == 0){
# 663
Cyc_Tc_rule_out_mem_and_unique(loc,q,tvs);
({struct Cyc_Dict_Dict _tmp2F7=({struct Cyc_Dict_Dict _tmp2F6=(te->ae)->datatypedecls;struct _tuple0*_tmp2F5=q;((struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict,struct _tuple0*,struct Cyc_Absyn_Datatypedecl**))Cyc_Dict_insert)(_tmp2F6,_tmp2F5,({struct Cyc_Absyn_Datatypedecl**_tmp162=_cycalloc(sizeof(*_tmp162));*_tmp162=tud;_tmp162;}));});(te->ae)->datatypedecls=_tmp2F7;});
goto _LL5;}else{_tmp160=*_tmp15F.f2;_LLD: {struct Cyc_Absyn_Datatypedecl**tudp=_tmp160;
# 696
struct Cyc_Absyn_Datatypedecl*_tmp168=Cyc_Tcdecl_merge_datatypedecl(*tudp,tud,loc,Cyc_Tc_tc_msg);struct Cyc_Absyn_Datatypedecl*tud2=_tmp168;
Cyc_Tc_rule_out_mem_and_unique(loc,q,tvs);
if(tud2 != 0)
*tudp=tud2;
goto _LL5;}}}else{if(_tmp15F.f2 == 0){_tmp160=(struct Cyc_List_List**)&(_tmp15F.f1)->v;{struct Cyc_List_List**fs=_tmp160;
# 668
struct Cyc_Absyn_Datatypedecl**_tmp163=({struct Cyc_Absyn_Datatypedecl**_tmp165=_cycalloc(sizeof(*_tmp165));({struct Cyc_Absyn_Datatypedecl*_tmp2F8=({struct Cyc_Absyn_Datatypedecl*_tmp164=_cycalloc(sizeof(*_tmp164));_tmp164->sc=Cyc_Absyn_Extern,_tmp164->name=tud->name,_tmp164->tvs=tvs,_tmp164->fields=0,_tmp164->is_extensible=tud->is_extensible;_tmp164;});*_tmp165=_tmp2F8;});_tmp165;});struct Cyc_Absyn_Datatypedecl**tudp=_tmp163;
# 670
({struct Cyc_Dict_Dict _tmp2F9=((struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict,struct _tuple0*,struct Cyc_Absyn_Datatypedecl**))Cyc_Dict_insert)((te->ae)->datatypedecls,q,tudp);(te->ae)->datatypedecls=_tmp2F9;});
# 673
({struct Cyc_List_List*_tmp2FA=Cyc_Tc_tcDatatypeFields(te,loc,obj,tud->is_extensible,tud->name,*fs,tvs,tud);*fs=_tmp2FA;});
Cyc_Tc_rule_out_mem_and_unique(loc,q,tvs);
*tudp=tud;
goto _LL5;}}else{_tmp160=(struct Cyc_List_List**)&(_tmp15F.f1)->v;_tmp161=*_tmp15F.f2;{struct Cyc_List_List**fs=(struct Cyc_List_List**)_tmp160;struct Cyc_Absyn_Datatypedecl**tudp=_tmp161;
# 678
struct Cyc_Absyn_Datatypedecl*_tmp166=*tudp;struct Cyc_Absyn_Datatypedecl*tud0=_tmp166;
# 681
if((!tud->is_extensible &&(unsigned)tud0)&& tud0->is_extensible)
tud->is_extensible=1;
# 684
({struct Cyc_Absyn_Datatypedecl*_tmp2FB=({struct Cyc_Absyn_Datatypedecl*_tmp167=_cycalloc(sizeof(*_tmp167));_tmp167->sc=Cyc_Absyn_Extern,_tmp167->name=tud->name,_tmp167->tvs=tvs,_tmp167->fields=0,_tmp167->is_extensible=tud->is_extensible;_tmp167;});*tudp=_tmp2FB;});
# 688
({struct Cyc_List_List*_tmp2FC=Cyc_Tc_tcDatatypeFields(te,loc,obj,tud->is_extensible,tud->name,*fs,tvs,tud);*fs=_tmp2FC;});
# 691
*tudp=tud0;
_tmp160=tudp;goto _LLD;}}}_LL5:;}}}
# 704
void Cyc_Tc_tcEnumdecl(struct Cyc_Tcenv_Tenv*te,unsigned loc,struct Cyc_Absyn_Enumdecl*ed){
struct _tuple0*q=ed->name;
# 710
if(ed->fields != 0){
struct _RegionHandle _tmp16B=_new_region("uprev_rgn");struct _RegionHandle*uprev_rgn=& _tmp16B;_push_region(uprev_rgn);
{struct Cyc_List_List*prev_fields=0;
unsigned tag_count=0U;
struct Cyc_List_List*fs=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(ed->fields))->v;for(0;fs != 0;fs=fs->tl){
struct Cyc_Absyn_Enumfield*_tmp16C=(struct Cyc_Absyn_Enumfield*)fs->hd;struct Cyc_Absyn_Enumfield*f=_tmp16C;
# 717
if(((int(*)(int(*)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*,struct _fat_ptr*))Cyc_List_mem)(Cyc_strptrcmp,prev_fields,(*f->name).f2))
({struct Cyc_Warn_String_Warn_Warg_struct _tmp16E=({struct Cyc_Warn_String_Warn_Warg_struct _tmp216;_tmp216.tag=0,({struct _fat_ptr _tmp2FD=({const char*_tmp170="duplicate enum constructor ";_tag_fat(_tmp170,sizeof(char),28U);});_tmp216.f1=_tmp2FD;});_tmp216;});struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp16F=({struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp215;_tmp215.tag=1,_tmp215.f1=f->name;_tmp215;});void*_tmp16D[2];_tmp16D[0]=& _tmp16E,_tmp16D[1]=& _tmp16F;({unsigned _tmp2FE=f->loc;Cyc_Warn_err2(_tmp2FE,_tag_fat(_tmp16D,sizeof(void*),2));});});else{
# 720
prev_fields=({struct Cyc_List_List*_tmp171=_region_malloc(uprev_rgn,sizeof(*_tmp171));_tmp171->hd=(*f->name).f2,_tmp171->tl=prev_fields;_tmp171;});}
# 722
if(((struct _tuple13**(*)(struct Cyc_Dict_Dict,struct _tuple0*))Cyc_Dict_lookup_opt)((te->ae)->ordinaries,f->name)!= 0)
({struct Cyc_Warn_String_Warn_Warg_struct _tmp173=({struct Cyc_Warn_String_Warn_Warg_struct _tmp219;_tmp219.tag=0,({struct _fat_ptr _tmp2FF=({const char*_tmp177="enum field name ";_tag_fat(_tmp177,sizeof(char),17U);});_tmp219.f1=_tmp2FF;});_tmp219;});struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp174=({struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp218;_tmp218.tag=1,_tmp218.f1=f->name;_tmp218;});struct Cyc_Warn_String_Warn_Warg_struct _tmp175=({struct Cyc_Warn_String_Warn_Warg_struct _tmp217;_tmp217.tag=0,({struct _fat_ptr _tmp300=({const char*_tmp176=" shadows global name";_tag_fat(_tmp176,sizeof(char),21U);});_tmp217.f1=_tmp300;});_tmp217;});void*_tmp172[3];_tmp172[0]=& _tmp173,_tmp172[1]=& _tmp174,_tmp172[2]=& _tmp175;({unsigned _tmp301=f->loc;Cyc_Warn_err2(_tmp301,_tag_fat(_tmp172,sizeof(void*),3));});});
if(f->tag == 0)
({struct Cyc_Absyn_Exp*_tmp302=Cyc_Absyn_uint_exp(tag_count ++,f->loc);f->tag=_tmp302;});else{
# 727
if(Cyc_Tcutil_is_const_exp((struct Cyc_Absyn_Exp*)_check_null(f->tag))){
struct _tuple11 _tmp178=Cyc_Evexp_eval_const_uint_exp((struct Cyc_Absyn_Exp*)_check_null(f->tag));struct _tuple11 _stmttmpF=_tmp178;struct _tuple11 _tmp179=_stmttmpF;int _tmp17B;unsigned _tmp17A;_tmp17A=_tmp179.f1;_tmp17B=_tmp179.f2;{unsigned t1=_tmp17A;int known=_tmp17B;
if(known)
tag_count=t1 + (unsigned)1;}}}}}
# 712
;_pop_region();}
# 736
{struct _handler_cons _tmp17C;_push_handler(& _tmp17C);{int _tmp17E=0;if(setjmp(_tmp17C.handler))_tmp17E=1;if(!_tmp17E){
{struct Cyc_Absyn_Enumdecl**_tmp17F=((struct Cyc_Absyn_Enumdecl**(*)(struct Cyc_Dict_Dict,struct _tuple0*))Cyc_Dict_lookup)((te->ae)->enumdecls,q);struct Cyc_Absyn_Enumdecl**edp=_tmp17F;
struct Cyc_Absyn_Enumdecl*_tmp180=Cyc_Tcdecl_merge_enumdecl(*edp,ed,loc,Cyc_Tc_tc_msg);struct Cyc_Absyn_Enumdecl*ed2=_tmp180;
if(ed2 == 0){_npop_handler(0);return;}
*edp=ed2;}
# 737
;_pop_handler();}else{void*_tmp17D=(void*)Cyc_Core_get_exn_thrown();void*_tmp181=_tmp17D;void*_tmp182;if(((struct Cyc_Dict_Absent_exn_struct*)_tmp181)->tag == Cyc_Dict_Absent){
# 742
struct Cyc_Absyn_Enumdecl**_tmp183=({struct Cyc_Absyn_Enumdecl**_tmp184=_cycalloc(sizeof(*_tmp184));*_tmp184=ed;_tmp184;});struct Cyc_Absyn_Enumdecl**edp=_tmp183;
({struct Cyc_Dict_Dict _tmp303=((struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict,struct _tuple0*,struct Cyc_Absyn_Enumdecl**))Cyc_Dict_insert)((te->ae)->enumdecls,q,edp);(te->ae)->enumdecls=_tmp303;});
goto _LL3;}else{_tmp182=_tmp181;{void*exn=_tmp182;(int)_rethrow(exn);}}_LL3:;}}}
# 748
if(ed->fields != 0){
struct Cyc_List_List*fs=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(ed->fields))->v;for(0;fs != 0;fs=fs->tl){
struct Cyc_Absyn_Enumfield*_tmp185=(struct Cyc_Absyn_Enumfield*)fs->hd;struct Cyc_Absyn_Enumfield*f=_tmp185;
Cyc_Tcexp_tcExp(te,0,(struct Cyc_Absyn_Exp*)_check_null(f->tag));
if(!Cyc_Tcutil_is_const_exp((struct Cyc_Absyn_Exp*)_check_null(f->tag)))
({struct Cyc_Warn_String_Warn_Warg_struct _tmp187=({struct Cyc_Warn_String_Warn_Warg_struct _tmp21E;_tmp21E.tag=0,({struct _fat_ptr _tmp304=({const char*_tmp18E="enum ";_tag_fat(_tmp18E,sizeof(char),6U);});_tmp21E.f1=_tmp304;});_tmp21E;});struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp188=({struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp21D;_tmp21D.tag=1,_tmp21D.f1=q;_tmp21D;});struct Cyc_Warn_String_Warn_Warg_struct _tmp189=({struct Cyc_Warn_String_Warn_Warg_struct _tmp21C;_tmp21C.tag=0,({struct _fat_ptr _tmp305=({const char*_tmp18D=", field ";_tag_fat(_tmp18D,sizeof(char),9U);});_tmp21C.f1=_tmp305;});_tmp21C;});struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp18A=({struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp21B;_tmp21B.tag=1,_tmp21B.f1=f->name;_tmp21B;});struct Cyc_Warn_String_Warn_Warg_struct _tmp18B=({struct Cyc_Warn_String_Warn_Warg_struct _tmp21A;_tmp21A.tag=0,({
struct _fat_ptr _tmp306=({const char*_tmp18C=": expression is not constant";_tag_fat(_tmp18C,sizeof(char),29U);});_tmp21A.f1=_tmp306;});_tmp21A;});void*_tmp186[5];_tmp186[0]=& _tmp187,_tmp186[1]=& _tmp188,_tmp186[2]=& _tmp189,_tmp186[3]=& _tmp18A,_tmp186[4]=& _tmp18B;({unsigned _tmp307=loc;Cyc_Warn_err2(_tmp307,_tag_fat(_tmp186,sizeof(void*),5));});});}}}
# 758
static int Cyc_Tc_okay_externC(unsigned loc,enum Cyc_Absyn_Scope sc,int in_include,int in_inc_rep){
enum Cyc_Absyn_Scope _tmp18F=sc;switch((int)_tmp18F){case Cyc_Absyn_Static:
# 761
 if(!in_include)
({struct Cyc_Warn_String_Warn_Warg_struct _tmp191=({struct Cyc_Warn_String_Warn_Warg_struct _tmp21F;_tmp21F.tag=0,({struct _fat_ptr _tmp308=({const char*_tmp192="static declaration within extern \"C\"";_tag_fat(_tmp192,sizeof(char),37U);});_tmp21F.f1=_tmp308;});_tmp21F;});void*_tmp190[1];_tmp190[0]=& _tmp191;({unsigned _tmp309=loc;Cyc_Warn_warn2(_tmp309,_tag_fat(_tmp190,sizeof(void*),1));});});
return 0;case Cyc_Absyn_Abstract:
# 765
({struct Cyc_Warn_String_Warn_Warg_struct _tmp194=({struct Cyc_Warn_String_Warn_Warg_struct _tmp220;_tmp220.tag=0,({struct _fat_ptr _tmp30A=({const char*_tmp195="abstract declaration within extern \"C\"";_tag_fat(_tmp195,sizeof(char),39U);});_tmp220.f1=_tmp30A;});_tmp220;});void*_tmp193[1];_tmp193[0]=& _tmp194;({unsigned _tmp30B=loc;Cyc_Warn_warn2(_tmp30B,_tag_fat(_tmp193,sizeof(void*),1));});});
return 0;case Cyc_Absyn_Public:
 goto _LL8;case Cyc_Absyn_Register: _LL8:
 goto _LLA;case Cyc_Absyn_Extern: _LLA:
 return 1;case Cyc_Absyn_ExternC:
 goto _LLE;default: _LLE:
# 772
 if(!in_inc_rep)
({struct Cyc_Warn_String_Warn_Warg_struct _tmp197=({struct Cyc_Warn_String_Warn_Warg_struct _tmp221;_tmp221.tag=0,({struct _fat_ptr _tmp30C=({const char*_tmp198="nested extern \"C\" declaration";_tag_fat(_tmp198,sizeof(char),30U);});_tmp221.f1=_tmp30C;});_tmp221;});void*_tmp196[1];_tmp196[0]=& _tmp197;({unsigned _tmp30D=loc;Cyc_Warn_warn2(_tmp30D,_tag_fat(_tmp196,sizeof(void*),1));});});
return 1;};}
# 778
static void Cyc_Tc_set_scopes(struct Cyc_List_List*ovrs,enum Cyc_Absyn_Scope sc){
for(1;ovrs != 0;ovrs=ovrs->tl){
void*_tmp199=((struct Cyc_Absyn_Decl*)ovrs->hd)->r;void*_stmttmp10=_tmp199;void*_tmp19A=_stmttmp10;void*_tmp19B;switch(*((int*)_tmp19A)){case 0: _tmp19B=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp19A)->f1;{struct Cyc_Absyn_Vardecl*vd=_tmp19B;
vd->sc=sc;goto _LL0;}case 1: _tmp19B=((struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_tmp19A)->f1;{struct Cyc_Absyn_Fndecl*fd=_tmp19B;
fd->sc=sc;goto _LL0;}case 5: _tmp19B=((struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*)_tmp19A)->f1;{struct Cyc_Absyn_Aggrdecl*ad=_tmp19B;
ad->sc=sc;goto _LL0;}case 6: _tmp19B=((struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct*)_tmp19A)->f1;{struct Cyc_Absyn_Datatypedecl*tud=_tmp19B;
tud->sc=sc;goto _LL0;}case 7: _tmp19B=((struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*)_tmp19A)->f1;{struct Cyc_Absyn_Enumdecl*ed=_tmp19B;
ed->sc=sc;goto _LL0;}default:
 goto _LL0;}_LL0:;}}
# 795
static void Cyc_Tc_tc_decls(struct Cyc_Tcenv_Tenv*te,struct Cyc_List_List*ds0,int in_externC,int check_var_init,struct Cyc_List_List**exports){
# 799
struct Cyc_List_List*_tmp19C=ds0;struct Cyc_List_List*ds=_tmp19C;for(0;ds != 0;ds=ds->tl){
struct Cyc_Absyn_Decl*d=(struct Cyc_Absyn_Decl*)ds->hd;
unsigned loc=d->loc;
void*_tmp19D=d->r;void*_stmttmp11=_tmp19D;void*_tmp19E=_stmttmp11;void*_tmp1A2;void*_tmp1A1;void*_tmp1A0;void*_tmp19F;switch(*((int*)_tmp19E)){case 2:
 goto _LL4;case 3: _LL4:
# 805
({struct Cyc_Warn_String_Warn_Warg_struct _tmp1A4=({struct Cyc_Warn_String_Warn_Warg_struct _tmp222;_tmp222.tag=0,({struct _fat_ptr _tmp30E=({const char*_tmp1A5="top level let-declarations are not implemented";_tag_fat(_tmp1A5,sizeof(char),47U);});_tmp222.f1=_tmp30E;});_tmp222;});void*_tmp1A3[1];_tmp1A3[0]=& _tmp1A4;({unsigned _tmp30F=loc;Cyc_Warn_err2(_tmp30F,_tag_fat(_tmp1A3,sizeof(void*),1));});});goto _LL0;case 4:
# 807
({struct Cyc_Warn_String_Warn_Warg_struct _tmp1A7=({struct Cyc_Warn_String_Warn_Warg_struct _tmp223;_tmp223.tag=0,({struct _fat_ptr _tmp310=({const char*_tmp1A8="top level region declarations are not implemented";_tag_fat(_tmp1A8,sizeof(char),50U);});_tmp223.f1=_tmp310;});_tmp223;});void*_tmp1A6[1];_tmp1A6[0]=& _tmp1A7;({unsigned _tmp311=loc;Cyc_Warn_err2(_tmp311,_tag_fat(_tmp1A6,sizeof(void*),1));});});goto _LL0;case 0: _tmp19F=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp19E)->f1;{struct Cyc_Absyn_Vardecl*vd=_tmp19F;
# 809
if(in_externC && Cyc_Tc_okay_externC(d->loc,vd->sc,te->in_extern_c_include,te->in_extern_c_inc_repeat))
vd->sc=Cyc_Absyn_ExternC;
Cyc_Tc_tcVardecl(te,loc,vd,check_var_init,te->in_extern_c_include,exports);
goto _LL0;}case 1: _tmp19F=((struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_tmp19E)->f1;{struct Cyc_Absyn_Fndecl*fd=_tmp19F;
# 814
if(in_externC && Cyc_Tc_okay_externC(d->loc,fd->sc,te->in_extern_c_include,te->in_extern_c_inc_repeat))
fd->sc=Cyc_Absyn_ExternC;
if(te->in_extern_c_include)
fd->orig_scope=Cyc_Absyn_ExternC;
Cyc_Tc_tcFndecl(te,loc,fd,exports);
goto _LL0;}case 8: _tmp19F=((struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct*)_tmp19E)->f1;{struct Cyc_Absyn_Typedefdecl*td=_tmp19F;
# 821
td->extern_c=te->in_extern_c_include;
# 825
Cyc_Tc_tcTypedefdecl(te,loc,td);
goto _LL0;}case 5: _tmp19F=((struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*)_tmp19E)->f1;{struct Cyc_Absyn_Aggrdecl*ad=_tmp19F;
# 828
if(in_externC && Cyc_Tc_okay_externC(d->loc,ad->sc,te->in_extern_c_include,te->in_extern_c_inc_repeat))
ad->sc=Cyc_Absyn_ExternC;
Cyc_Tc_tcAggrdecl(te,loc,ad);
goto _LL0;}case 6: _tmp19F=((struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct*)_tmp19E)->f1;{struct Cyc_Absyn_Datatypedecl*tud=_tmp19F;
# 833
if(in_externC && Cyc_Tc_okay_externC(d->loc,tud->sc,te->in_extern_c_include,te->in_extern_c_inc_repeat))
tud->sc=Cyc_Absyn_ExternC;
Cyc_Tc_tcDatatypedecl(te,loc,tud);
goto _LL0;}case 7: _tmp19F=((struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*)_tmp19E)->f1;{struct Cyc_Absyn_Enumdecl*ed=_tmp19F;
# 838
if(in_externC && Cyc_Tc_okay_externC(d->loc,ed->sc,te->in_extern_c_include,te->in_extern_c_inc_repeat))
ed->sc=Cyc_Absyn_ExternC;
Cyc_Tc_tcEnumdecl(te,loc,ed);
goto _LL0;}case 13:
({struct Cyc_Warn_String_Warn_Warg_struct _tmp1AA=({struct Cyc_Warn_String_Warn_Warg_struct _tmp224;_tmp224.tag=0,({struct _fat_ptr _tmp312=({const char*_tmp1AB="spurious __cyclone_port_on__";_tag_fat(_tmp1AB,sizeof(char),29U);});_tmp224.f1=_tmp312;});_tmp224;});void*_tmp1A9[1];_tmp1A9[0]=& _tmp1AA;({unsigned _tmp313=d->loc;Cyc_Warn_warn2(_tmp313,_tag_fat(_tmp1A9,sizeof(void*),1));});});goto _LL0;case 14:
 goto _LL0;case 15:
 te=Cyc_Tcenv_enter_tempest(te);goto _LL0;case 16:
 te=Cyc_Tcenv_clear_tempest(te);goto _LL0;case 9: _tmp19F=((struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmp19E)->f1;_tmp1A0=((struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmp19E)->f2;{struct _fat_ptr*v=_tmp19F;struct Cyc_List_List*ds2=_tmp1A0;
# 848
struct Cyc_List_List*_tmp1AC=te->ns;struct Cyc_List_List*ns=_tmp1AC;
({struct Cyc_List_List*_tmp315=({struct Cyc_List_List*_tmp314=ns;((struct Cyc_List_List*(*)(struct Cyc_List_List*,struct Cyc_List_List*))Cyc_List_append)(_tmp314,({struct Cyc_List_List*_tmp1AD=_cycalloc(sizeof(*_tmp1AD));_tmp1AD->hd=v,_tmp1AD->tl=0;_tmp1AD;}));});te->ns=_tmp315;});
Cyc_Tc_tc_decls(te,ds2,in_externC,check_var_init,exports);
te->ns=ns;
goto _LL0;}case 10: _tmp19F=((struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*)_tmp19E)->f2;{struct Cyc_List_List*ds2=_tmp19F;
# 855
Cyc_Tc_tc_decls(te,ds2,in_externC,check_var_init,exports);goto _LL0;}case 11: _tmp19F=((struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct*)_tmp19E)->f1;{struct Cyc_List_List*ds2=_tmp19F;
# 858
Cyc_Tc_tc_decls(te,ds2,1,check_var_init,exports);goto _LL0;}default: _tmp19F=(struct Cyc_List_List**)&((struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*)_tmp19E)->f1;_tmp1A0=((struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*)_tmp19E)->f2;_tmp1A1=((struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*)_tmp19E)->f3;_tmp1A2=((struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*)_tmp19E)->f4;{struct Cyc_List_List**ds2=(struct Cyc_List_List**)_tmp19F;struct Cyc_List_List*ovrs=_tmp1A0;struct Cyc_List_List*exports2=_tmp1A1;struct _tuple10*wc=_tmp1A2;
# 863
if((unsigned)ovrs){
struct Cyc_Tcenv_Tenv*_tmp1AE=Cyc_Tcenv_copy_tenv_dicts(te);struct Cyc_Tcenv_Tenv*tecpy=_tmp1AE;
tecpy->in_extern_c_include=1;
Cyc_Tc_tc_decls(tecpy,*ds2,1,check_var_init,0);
Cyc_Toc_init();
Cyc_Tc_set_scopes(ovrs,Cyc_Absyn_ExternC);
Cyc_Cifc_user_overrides(d->loc,tecpy,ds2,ovrs);
Cyc_Toc_finish();}{
# 872
struct Cyc_List_List*newexs=
((struct Cyc_List_List*(*)(struct Cyc_List_List*,struct Cyc_List_List*))Cyc_List_append)(exports2,(unsigned)exports?*exports: 0);
struct Cyc_Tcenv_Tenv*_tmp1AF=Cyc_Tcenv_enter_extern_c_include(te);struct Cyc_Tcenv_Tenv*te2=_tmp1AF;
te2->in_extern_c_inc_repeat=ovrs != 0;
Cyc_Tc_tc_decls(te2,*ds2,1,check_var_init,& newexs);
# 879
for(1;exports2 != 0;exports2=exports2->tl){
struct _tuple12*_tmp1B0=(struct _tuple12*)exports2->hd;struct _tuple12*exp=_tmp1B0;
if(!(*exp).f3)
({struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp1B2=({struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp226;_tmp226.tag=1,_tmp226.f1=(*exp).f2;_tmp226;});struct Cyc_Warn_String_Warn_Warg_struct _tmp1B3=({struct Cyc_Warn_String_Warn_Warg_struct _tmp225;_tmp225.tag=0,({struct _fat_ptr _tmp316=({const char*_tmp1B4=" is exported but not defined";_tag_fat(_tmp1B4,sizeof(char),29U);});_tmp225.f1=_tmp316;});_tmp225;});void*_tmp1B1[2];_tmp1B1[0]=& _tmp1B2,_tmp1B1[1]=& _tmp1B3;({unsigned _tmp317=(*exp).f1;Cyc_Warn_warn2(_tmp317,_tag_fat(_tmp1B1,sizeof(void*),2));});});}
# 884
goto _LL0;}}}_LL0:;}}
# 889
void Cyc_Tc_tc(struct Cyc_Tcenv_Tenv*te,int check_var_init,struct Cyc_List_List*ds){
Cyc_Absynpp_set_params(& Cyc_Absynpp_tc_params_r);{
struct Cyc_Dict_Dict _tmp1B5=Cyc_Callgraph_compute_callgraph(ds);struct Cyc_Dict_Dict cg=_tmp1B5;
# 894
struct Cyc_Dict_Dict _tmp1B6=((struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict))Cyc_Graph_scc)(cg);struct Cyc_Dict_Dict scc=_tmp1B6;
# 897
Cyc_Tc_tc_decls(te,ds,0,check_var_init,0);}}struct Cyc_Tc_TreeshakeEnv{int in_cinclude;struct Cyc_Dict_Dict ordinaries;};
# 915 "tc.cyc"
static int Cyc_Tc_vardecl_needed(struct Cyc_Tc_TreeshakeEnv*,struct Cyc_Absyn_Decl*);
# 917
static struct Cyc_List_List*Cyc_Tc_treeshake_f(struct Cyc_Tc_TreeshakeEnv*env,struct Cyc_List_List*ds){
return((struct Cyc_List_List*(*)(int(*)(struct Cyc_Tc_TreeshakeEnv*,struct Cyc_Absyn_Decl*),struct Cyc_Tc_TreeshakeEnv*,struct Cyc_List_List*))Cyc_List_filter_c)(Cyc_Tc_vardecl_needed,env,ds);}
# 921
static int Cyc_Tc_is_extern(struct Cyc_Absyn_Vardecl*vd){
if((int)vd->sc == (int)Cyc_Absyn_Extern ||(int)vd->sc == (int)Cyc_Absyn_ExternC)
return 1;{
void*_tmp1B7=Cyc_Absyn_compress(vd->type);void*_stmttmp12=_tmp1B7;void*_tmp1B8=_stmttmp12;if(*((int*)_tmp1B8)== 5)
return 1;else{
return 0;};}}
# 930
static int Cyc_Tc_vardecl_needed(struct Cyc_Tc_TreeshakeEnv*env,struct Cyc_Absyn_Decl*d){
void*_tmp1B9=d->r;void*_stmttmp13=_tmp1B9;void*_tmp1BA=_stmttmp13;void*_tmp1BB;switch(*((int*)_tmp1BA)){case 0: _tmp1BB=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp1BA)->f1;{struct Cyc_Absyn_Vardecl*vd=_tmp1BB;
# 934
if((env->in_cinclude || !Cyc_Tc_is_extern(vd))|| !({
struct _tuple0*_tmp318=vd->name;Cyc_Absyn_qvar_cmp(_tmp318,Cyc_Absyn_uniquergn_qvar());}))
return 1;
return(*((struct _tuple13*(*)(struct Cyc_Dict_Dict,struct _tuple0*))Cyc_Dict_lookup)(env->ordinaries,vd->name)).f2;}case 11: _tmp1BB=(struct Cyc_List_List**)&((struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct*)_tmp1BA)->f1;{struct Cyc_List_List**ds2p=_tmp1BB;
_tmp1BB=ds2p;goto _LL6;}case 10: _tmp1BB=(struct Cyc_List_List**)&((struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*)_tmp1BA)->f2;_LL6: {struct Cyc_List_List**ds2p=_tmp1BB;
_tmp1BB=ds2p;goto _LL8;}case 9: _tmp1BB=(struct Cyc_List_List**)&((struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmp1BA)->f2;_LL8: {struct Cyc_List_List**ds2p=_tmp1BB;
({struct Cyc_List_List*_tmp319=Cyc_Tc_treeshake_f(env,*ds2p);*ds2p=_tmp319;});return 1;}case 12: _tmp1BB=(struct Cyc_List_List**)&((struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*)_tmp1BA)->f1;{struct Cyc_List_List**ds2p=(struct Cyc_List_List**)_tmp1BB;
# 942
int in_cinclude=env->in_cinclude;
env->in_cinclude=1;
({struct Cyc_List_List*_tmp31A=Cyc_Tc_treeshake_f(env,*ds2p);*ds2p=_tmp31A;});
env->in_cinclude=in_cinclude;
return 1;}default:
 return 1;};}
# 951
struct Cyc_List_List*Cyc_Tc_treeshake(struct Cyc_Tcenv_Tenv*te,struct Cyc_List_List*ds){
struct Cyc_Tc_TreeshakeEnv _tmp1BC=({struct Cyc_Tc_TreeshakeEnv _tmp227;_tmp227.in_cinclude=0,_tmp227.ordinaries=(te->ae)->ordinaries;_tmp227;});struct Cyc_Tc_TreeshakeEnv env=_tmp1BC;
return Cyc_Tc_treeshake_f(& env,ds);}
