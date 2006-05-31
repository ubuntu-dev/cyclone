#include <setjmp.h>
/* This is a C header used by the output of the Cyclone to
   C translator.  Corresponding definitions are in file lib/runtime_*.c */
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
/* should be size_t but int is fine */
#define offsetof(t,n) ((int)(&(((t*)0)->n)))
#endif

/* Fat pointers */
struct _fat_ptr {
  unsigned char *curr; 
  unsigned char *base; 
  unsigned char *last_plus_one; 
};  

/* Regions */
struct _RegionPage
{ 
#ifdef CYC_REGION_PROFILE
  unsigned total_bytes;
  unsigned free_bytes;
#endif
  struct _RegionPage *next;
  char data[1];
};

struct _pool;
struct bget_region_key;
struct _RegionAllocFunctions;

struct _RegionHandle {
  struct _RuntimeStack s;
  struct _RegionPage *curr;
#if(defined(__linux__) && defined(__KERNEL__))
  struct _RegionPage *vpage;
#endif 
  struct _RegionAllocFunctions *fcns;
  char               *offset;
  char               *last_plus_one;
  struct _pool *released_ptrs;
  struct bget_region_key *key;
#ifdef CYC_REGION_PROFILE
  const char *name;
#endif
  unsigned used_bytes;
  unsigned wasted_bytes;
};


// A dynamic region is just a region handle.  The wrapper struct is for type
// abstraction.
struct Cyc_Core_DynamicRegion {
  struct _RegionHandle h;
};

/* Alias qualifier stuff */
typedef unsigned int _AliasQualHandle_t; // must match aqualt_type() in toc.cyc

struct _RegionHandle _new_region(unsigned int, const char*);
void* _region_malloc(struct _RegionHandle*, _AliasQualHandle_t, unsigned);
void* _region_calloc(struct _RegionHandle*, _AliasQualHandle_t, unsigned t, unsigned n);
void* _region_vmalloc(struct _RegionHandle*, unsigned);
void * _aqual_malloc(_AliasQualHandle_t aq, unsigned int s);
void * _aqual_calloc(_AliasQualHandle_t aq, unsigned int n, unsigned int t);
void _free_region(struct _RegionHandle*);

/* Exceptions */
struct _handler_cons {
  struct _RuntimeStack s;
  jmp_buf handler;
};
void _push_handler(struct _handler_cons*);
void _push_region(struct _RegionHandle*);
void _npop_handler(int);
void _pop_handler();
void _pop_region();


#ifndef _throw
void* _throw_null_fn(const char*,unsigned);
void* _throw_arraybounds_fn(const char*,unsigned);
void* _throw_badalloc_fn(const char*,unsigned);
void* _throw_match_fn(const char*,unsigned);
void* _throw_assert_fn(const char *,unsigned);
void* _throw_fn(void*,const char*,unsigned);
void* _rethrow(void*);
#define _throw_null() (_throw_null_fn(__FILE__,__LINE__))
#define _throw_arraybounds() (_throw_arraybounds_fn(__FILE__,__LINE__))
#define _throw_badalloc() (_throw_badalloc_fn(__FILE__,__LINE__))
#define _throw_match() (_throw_match_fn(__FILE__,__LINE__))
#define _throw_assert() (_throw_assert_fn(__FILE__,__LINE__))
#define _throw(e) (_throw_fn((e),__FILE__,__LINE__))
#endif

void* Cyc_Core_get_exn_thrown();
/* Built-in Exceptions */
struct Cyc_Null_Exception_exn_struct { char *tag; };
struct Cyc_Array_bounds_exn_struct { char *tag; };
struct Cyc_Match_Exception_exn_struct { char *tag; };
struct Cyc_Bad_alloc_exn_struct { char *tag; };
struct Cyc_Assert_exn_struct { char *tag; };
extern char Cyc_Null_Exception[];
extern char Cyc_Array_bounds[];
extern char Cyc_Match_Exception[];
extern char Cyc_Bad_alloc[];
extern char Cyc_Assert[];

/* Built-in Run-time Checks and company */
#ifdef NO_CYC_NULL_CHECKS
#define _check_null(ptr) (ptr)
#else
#define _check_null(ptr) \
  ({ typeof(ptr) _cks_null = (ptr); \
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
#define _zero_arr_plus_char_fn(orig_x,orig_sz,orig_i,f,l) ((orig_x)+(orig_i))
#define _zero_arr_plus_other_fn(t_sz,orig_x,orig_sz,orig_i,f,l)((orig_x)+(orig_i))
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
void* _zero_arr_plus_other_fn(unsigned,void*,unsigned,int,const char*,unsigned);
#endif

/* _get_zero_arr_size_*(x,sz) returns the number of elements in a
   zero-terminated array that is NULL or has at least sz elements */
unsigned _get_zero_arr_size_char(const char*,unsigned);
unsigned _get_zero_arr_size_other(unsigned,const void*,unsigned);

/* _zero_arr_inplace_plus_*_fn(x,i,filename,lineno) sets
   zero-terminated pointer *x to *x + i */
char* _zero_arr_inplace_plus_char_fn(char**,int,const char*,unsigned);
char* _zero_arr_inplace_plus_post_char_fn(char**,int,const char*,unsigned);
// note: must cast result in toc.cyc
void* _zero_arr_inplace_plus_other_fn(unsigned,void**,int,const char*,unsigned);
void* _zero_arr_inplace_plus_post_other_fn(unsigned,void**,int,const char*,unsigned);
#define _zero_arr_plus_char(x,s,i) \
  (_zero_arr_plus_char_fn(x,s,i,__FILE__,__LINE__))
#define _zero_arr_inplace_plus_char(x,i) \
  _zero_arr_inplace_plus_char_fn((char**)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_char(x,i) \
  _zero_arr_inplace_plus_post_char_fn((char**)(x),(i),__FILE__,__LINE__)
#define _zero_arr_plus_other(t,x,s,i) \
  (_zero_arr_plus_other_fn(t,x,s,i,__FILE__,__LINE__))
#define _zero_arr_inplace_plus_other(t,x,i) \
  _zero_arr_inplace_plus_other_fn(t,(void**)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_other(t,x,i) \
  _zero_arr_inplace_plus_post_other_fn(t,(void**)(x),(i),__FILE__,__LINE__)

#ifdef NO_CYC_BOUNDS_CHECKS
#define _check_fat_subscript(arr,elt_sz,index) ((arr).curr + (elt_sz) * (index))
#define _untag_fat_ptr(arr,elt_sz,num_elts) ((arr).curr)
#define _untag_fat_ptr_check_bound(arr,elt_sz,num_elts) ((arr).curr)
#define _check_fat_at_base(arr) (arr)
#else
#define _check_fat_subscript(arr,elt_sz,index) ({ \
  struct _fat_ptr _cus_arr = (arr); \
  unsigned char *_cus_ans = _cus_arr.curr + (elt_sz) * (index); \
  /* JGM: not needed! if (!_cus_arr.base) _throw_null();*/ \
  if (_cus_ans < _cus_arr.base || _cus_ans >= _cus_arr.last_plus_one) \
    _throw_arraybounds(); \
  _cus_ans; })
#define _untag_fat_ptr(arr,elt_sz,num_elts) ((arr).curr)
#define _untag_fat_ptr_check_bound(arr,elt_sz,num_elts) ({ \
  struct _fat_ptr _arr = (arr); \
  unsigned char *_curr = _arr.curr; \
  if ((_curr < _arr.base || _curr + (elt_sz) * (num_elts) > _arr.last_plus_one) &&\
      _curr != (unsigned char*)0) \
    _throw_arraybounds(); \
  _curr; })
#define _check_fat_at_base(arr) ({ \
  struct _fat_ptr _arr = (arr); \
  if (_arr.base != _arr.curr) _throw_arraybounds(); \
  _arr; })
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

#ifdef CYC_GC_PTHREAD_REDIRECTS
# define pthread_create GC_pthread_create
# define pthread_sigmask GC_pthread_sigmask
# define pthread_join GC_pthread_join
# define pthread_detach GC_pthread_detach
# define dlopen GC_dlopen
#endif
/* Allocation */
void* GC_malloc(int);
void* GC_malloc_atomic(int);
void* GC_calloc(unsigned,unsigned);
void* GC_calloc_atomic(unsigned,unsigned);

#if(defined(__linux__) && defined(__KERNEL__))
void *cyc_vmalloc(unsigned);
void cyc_vfree(void*);
#endif
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

static inline unsigned int _check_times(unsigned x, unsigned y) {
  unsigned long long whole_ans = 
    ((unsigned long long) x)*((unsigned long long)y);
  unsigned word_ans = (unsigned)whole_ans;
  if(word_ans < whole_ans || word_ans > MAX_MALLOC_SIZE)
    _throw_badalloc();
  return word_ans;
}

#define _CYC_MAX_REGION_CONST 0
#define _CYC_MIN_ALIGNMENT (sizeof(double))

#ifdef CYC_REGION_PROFILE
extern int rgn_total_bytes;
#endif

static inline void*_fast_region_malloc(struct _RegionHandle*r, _AliasQualHandle_t aq, unsigned orig_s) {  
  if (r > (struct _RegionHandle*)_CYC_MAX_REGION_CONST && r->curr != 0) { 
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
  return _region_malloc(r,aq,orig_s); 
}

//doesn't make sense to fast malloc with reaps
#ifndef DISABLE_REAPS
#define _fast_region_malloc _region_malloc
#endif

#ifdef CYC_REGION_PROFILE
/* see macros below for usage. defined in runtime_memory.c */
void* _profile_GC_malloc(int,const char*,const char*,int);
void* _profile_GC_malloc_atomic(int,const char*,const char*,int);
void* _profile_GC_calloc(unsigned,unsigned,const char*,const char*,int);
void* _profile_GC_calloc_atomic(unsigned,unsigned,const char*,const char*,int);
void* _profile_region_malloc(struct _RegionHandle*,_AliasQualHandle_t,unsigned,const char*,const char*,int);
void* _profile_region_calloc(struct _RegionHandle*,_AliasQualHandle_t,unsigned,unsigned,const char *,const char*,int);
void * _profile_aqual_malloc(_AliasQualHandle_t aq, unsigned int s,const char *file, const char *func, int lineno);
void * _profile_aqual_calloc(_AliasQualHandle_t aq, unsigned int t1,unsigned int t2,const char *file, const char *func, int lineno);
struct _RegionHandle _profile_new_region(unsigned int i, const char*,const char*,const char*,int);
void _profile_free_region(struct _RegionHandle*,const char*,const char*,int);
#ifndef RUNTIME_CYC
#define _new_region(i,n) _profile_new_region(i,n,__FILE__,__FUNCTION__,__LINE__)
#define _free_region(r) _profile_free_region(r,__FILE__,__FUNCTION__,__LINE__)
#define _region_malloc(rh,aq,n) _profile_region_malloc(rh,aq,n,__FILE__,__FUNCTION__,__LINE__)
#define _region_calloc(rh,aq,n,t) _profile_region_calloc(rh,aq,n,t,__FILE__,__FUNCTION__,__LINE__)
#define _aqual_malloc(aq,n) _profile_aqual_malloc(aq,n,__FILE__,__FUNCTION__,__LINE__)
#define _aqual_calloc(aq,n,t) _profile_aqual_calloc(aq,n,t,__FILE__,__FUNCTION__,__LINE__)
#endif
#define _cycalloc(n) _profile_GC_malloc(n,__FILE__,__FUNCTION__,__LINE__)
#define _cycalloc_atomic(n) _profile_GC_malloc_atomic(n,__FILE__,__FUNCTION__,__LINE__)
#define _cyccalloc(n,s) _profile_GC_calloc(n,s,__FILE__,__FUNCTION__,__LINE__)
#define _cyccalloc_atomic(n,s) _profile_GC_calloc_atomic(n,s,__FILE__,__FUNCTION__,__LINE__)
#endif //CYC_REGION_PROFILE
#endif //_CYC_INCLUDE_H
 struct Cyc_Core_Opt{void*v;};struct _tuple0{void*f0;void*f1;};
# 115 "core.h"
void*Cyc_Core_snd(struct _tuple0*);struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 61 "list.h"
extern int Cyc_List_length(struct Cyc_List_List*);
# 76
extern struct Cyc_List_List*Cyc_List_map(void*(*)(void*),struct Cyc_List_List*);
# 178
extern struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*);
# 184
extern struct Cyc_List_List*Cyc_List_append(struct Cyc_List_List*,struct Cyc_List_List*);
# 195
extern struct Cyc_List_List*Cyc_List_imp_append(struct Cyc_List_List*,struct Cyc_List_List*);struct Cyc___cycFILE;
# 51 "cycboot.h"
extern struct Cyc___cycFILE*Cyc_stdout;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};
# 73
extern struct _fat_ptr Cyc_aprintf(struct _fat_ptr,struct _fat_ptr);
# 100
extern int Cyc_fprintf(struct Cyc___cycFILE*,struct _fat_ptr,struct _fat_ptr);
# 104
extern int Cyc_fputc(int,struct Cyc___cycFILE*);
# 278 "cycboot.h"
extern int Cyc_file_string_write(struct Cyc___cycFILE*,struct _fat_ptr,int,int);
# 38 "string.h"
extern unsigned long Cyc_strlen(struct _fat_ptr);
# 50 "string.h"
extern int Cyc_strptrcmp(struct _fat_ptr*,struct _fat_ptr*);struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};
# 62 "dict.h"
extern struct Cyc_Dict_Dict Cyc_Dict_empty(int(*)(void*,void*));
# 83
extern int Cyc_Dict_member(struct Cyc_Dict_Dict,void*);
# 87
extern struct Cyc_Dict_Dict Cyc_Dict_insert(struct Cyc_Dict_Dict,void*,void*);
# 101
extern struct Cyc_Dict_Dict Cyc_Dict_singleton(int(*)(void*,void*),void*,void*);struct Cyc_AssnDef_ExistAssnFn;struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};struct _tuple1{union Cyc_Absyn_Nmspace f0;struct _fat_ptr*f1;};
# 140 "absyn.h"
enum Cyc_Absyn_Scope{Cyc_Absyn_Static =0U,Cyc_Absyn_Abstract =1U,Cyc_Absyn_Public =2U,Cyc_Absyn_Extern =3U,Cyc_Absyn_ExternC =4U,Cyc_Absyn_Register =5U};struct Cyc_Absyn_Tqual{int print_const: 1;int q_volatile: 1;int q_restrict: 1;int real_const: 1;unsigned loc;};
# 161
enum Cyc_Absyn_Size_of{Cyc_Absyn_Char_sz =0U,Cyc_Absyn_Short_sz =1U,Cyc_Absyn_Int_sz =2U,Cyc_Absyn_Long_sz =3U,Cyc_Absyn_LongLong_sz =4U};
enum Cyc_Absyn_Sign{Cyc_Absyn_Signed =0U,Cyc_Absyn_Unsigned =1U,Cyc_Absyn_None =2U};
enum Cyc_Absyn_AggrKind{Cyc_Absyn_StructA =0U,Cyc_Absyn_UnionA =1U};
# 165
enum Cyc_Absyn_AliasQualVal{Cyc_Absyn_Aliasable_qual =0U,Cyc_Absyn_Unique_qual =1U,Cyc_Absyn_Refcnt_qual =2U,Cyc_Absyn_Restricted_qual =3U};
# 181 "absyn.h"
enum Cyc_Absyn_AliasHint{Cyc_Absyn_UniqueHint =0U,Cyc_Absyn_RefcntHint =1U,Cyc_Absyn_RestrictedHint =2U,Cyc_Absyn_NoHint =3U};
# 187
enum Cyc_Absyn_KindQual{Cyc_Absyn_AnyKind =0U,Cyc_Absyn_MemKind =1U,Cyc_Absyn_BoxKind =2U,Cyc_Absyn_EffKind =3U,Cyc_Absyn_IntKind =4U,Cyc_Absyn_BoolKind =5U,Cyc_Absyn_PtrBndKind =6U,Cyc_Absyn_AqualKind =7U};struct Cyc_Absyn_Kind{enum Cyc_Absyn_KindQual kind;enum Cyc_Absyn_AliasHint aliashint;};struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct{int tag;struct Cyc_Absyn_Kind*f1;};struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_Tvar{struct _fat_ptr*name;int identity;void*kind;void*aquals_bound;};struct Cyc_Absyn_PtrLoc{unsigned ptr_loc;unsigned rgn_loc;unsigned zt_loc;};struct Cyc_Absyn_PtrAtts{void*eff;void*nullable;void*bounds;void*zero_term;struct Cyc_Absyn_PtrLoc*ptrloc;void*autoreleased;void*aqual;};struct Cyc_Absyn_PtrInfo{void*elt_type;struct Cyc_Absyn_Tqual elt_tq;struct Cyc_Absyn_PtrAtts ptr_atts;};struct Cyc_Absyn_VarargInfo{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;int inject;};struct Cyc_Absyn_FnInfo{struct Cyc_List_List*tvars;void*effect;struct Cyc_Absyn_Tqual ret_tqual;void*ret_type;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*cyc_varargs;struct Cyc_List_List*qual_bnd;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*checks_clause;struct Cyc_AssnDef_ExistAssnFn*checks_assn;struct Cyc_Absyn_Exp*requires_clause;struct Cyc_AssnDef_ExistAssnFn*requires_assn;struct Cyc_Absyn_Exp*ensures_clause;struct Cyc_AssnDef_ExistAssnFn*ensures_assn;struct Cyc_Absyn_Exp*throws_clause;struct Cyc_AssnDef_ExistAssnFn*throws_assn;struct Cyc_Absyn_Vardecl*return_value;struct Cyc_List_List*arg_vardecls;struct Cyc_List_List*effconstr;};struct Cyc_Absyn_UnknownDatatypeInfo{struct _tuple1*name;int is_extensible;};struct _union_DatatypeInfo_UnknownDatatype{int tag;struct Cyc_Absyn_UnknownDatatypeInfo val;};struct _union_DatatypeInfo_KnownDatatype{int tag;struct Cyc_Absyn_Datatypedecl**val;};union Cyc_Absyn_DatatypeInfo{struct _union_DatatypeInfo_UnknownDatatype UnknownDatatype;struct _union_DatatypeInfo_KnownDatatype KnownDatatype;};struct Cyc_Absyn_UnknownDatatypeFieldInfo{struct _tuple1*datatype_name;struct _tuple1*field_name;int is_extensible;};struct _union_DatatypeFieldInfo_UnknownDatatypefield{int tag;struct Cyc_Absyn_UnknownDatatypeFieldInfo val;};struct _tuple2{struct Cyc_Absyn_Datatypedecl*f0;struct Cyc_Absyn_Datatypefield*f1;};struct _union_DatatypeFieldInfo_KnownDatatypefield{int tag;struct _tuple2 val;};union Cyc_Absyn_DatatypeFieldInfo{struct _union_DatatypeFieldInfo_UnknownDatatypefield UnknownDatatypefield;struct _union_DatatypeFieldInfo_KnownDatatypefield KnownDatatypefield;};struct _tuple3{enum Cyc_Absyn_AggrKind f0;struct _tuple1*f1;struct Cyc_Core_Opt*f2;};struct _union_AggrInfo_UnknownAggr{int tag;struct _tuple3 val;};struct _union_AggrInfo_KnownAggr{int tag;struct Cyc_Absyn_Aggrdecl**val;};union Cyc_Absyn_AggrInfo{struct _union_AggrInfo_UnknownAggr UnknownAggr;struct _union_AggrInfo_KnownAggr KnownAggr;};struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_TypeDecl{void*r;unsigned loc;};struct Cyc_Absyn_IntCon_Absyn_TyCon_struct{int tag;enum Cyc_Absyn_Sign f1;enum Cyc_Absyn_Size_of f2;};struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct{int tag;int f1;};struct Cyc_Absyn_AqualConstCon_Absyn_TyCon_struct{int tag;enum Cyc_Absyn_AliasQualVal f1;};struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct{int tag;struct _tuple1*f1;struct Cyc_Absyn_Enumdecl*f2;};struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct{int tag;struct _fat_ptr f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeInfo f1;};struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeFieldInfo f1;};struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_AggrInfo f1;};struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct{int tag;void*f1;};struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct{int tag;void*f1;void*f2;};struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct{int tag;void*f1;void*f2;};struct Cyc_Absyn_AppType_Absyn_Type_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Evar_Absyn_Type_struct{int tag;struct Cyc_Core_Opt*f1;void*f2;int f3;struct Cyc_Core_Opt*f4;};struct Cyc_Absyn_VarType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Tvar*f1;};struct Cyc_Absyn_Cvar_Absyn_Type_struct{int tag;struct Cyc_Core_Opt*f1;void*f2;int f3;void*f4;const char*f5;const char*f6;int f7;};struct Cyc_Absyn_PointerType_Absyn_Type_struct{int tag;struct Cyc_Absyn_PtrInfo f1;};struct Cyc_Absyn_FnType_Absyn_Type_struct{int tag;struct Cyc_Absyn_FnInfo f1;};struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct{int tag;enum Cyc_Absyn_AggrKind f1;int f2;struct Cyc_List_List*f3;};struct Cyc_Absyn_TypedefType_Absyn_Type_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_Typedefdecl*f3;void*f4;};struct Cyc_Absyn_ValueofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct{int tag;struct Cyc_Absyn_TypeDecl*f1;void**f2;};struct Cyc_Absyn_TypeofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Stdcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Cdecl_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Fastcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Unused_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;unsigned f2;};struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;int f2;struct Cyc_Absyn_VarargInfo*f3;void*f4;struct Cyc_List_List*f5;struct Cyc_List_List*f6;struct Cyc_Absyn_Exp*f7;struct Cyc_Absyn_Exp*f8;struct Cyc_Absyn_Exp*f9;struct Cyc_Absyn_Exp*f10;};struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct{int tag;void*f1;unsigned f2;};struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;unsigned f3;};struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_PtrAtts f1;struct Cyc_Absyn_Tqual f2;};struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct{int tag;void*f1;};struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_List_List*f1;unsigned f2;int f3;};struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct{int tag;unsigned f1;struct Cyc_List_List*f2;};struct _union_Cnst_Null_c{int tag;int val;};struct _tuple4{enum Cyc_Absyn_Sign f0;char f1;};struct _union_Cnst_Char_c{int tag;struct _tuple4 val;};struct _union_Cnst_Wchar_c{int tag;struct _fat_ptr val;};struct _tuple5{enum Cyc_Absyn_Sign f0;short f1;};struct _union_Cnst_Short_c{int tag;struct _tuple5 val;};struct _tuple6{enum Cyc_Absyn_Sign f0;int f1;};struct _union_Cnst_Int_c{int tag;struct _tuple6 val;};struct _tuple7{enum Cyc_Absyn_Sign f0;long long f1;};struct _union_Cnst_LongLong_c{int tag;struct _tuple7 val;};struct _tuple8{struct _fat_ptr f0;int f1;};struct _union_Cnst_Float_c{int tag;struct _tuple8 val;};struct _union_Cnst_String_c{int tag;struct _fat_ptr val;};struct _union_Cnst_Wstring_c{int tag;struct _fat_ptr val;};union Cyc_Absyn_Cnst{struct _union_Cnst_Null_c Null_c;struct _union_Cnst_Char_c Char_c;struct _union_Cnst_Wchar_c Wchar_c;struct _union_Cnst_Short_c Short_c;struct _union_Cnst_Int_c Int_c;struct _union_Cnst_LongLong_c LongLong_c;struct _union_Cnst_Float_c Float_c;struct _union_Cnst_String_c String_c;struct _union_Cnst_Wstring_c Wstring_c;};
# 524 "absyn.h"
enum Cyc_Absyn_Primop{Cyc_Absyn_Plus =0U,Cyc_Absyn_Times =1U,Cyc_Absyn_Minus =2U,Cyc_Absyn_Div =3U,Cyc_Absyn_Mod =4U,Cyc_Absyn_Eq =5U,Cyc_Absyn_Neq =6U,Cyc_Absyn_Gt =7U,Cyc_Absyn_Lt =8U,Cyc_Absyn_Gte =9U,Cyc_Absyn_Lte =10U,Cyc_Absyn_Not =11U,Cyc_Absyn_Bitnot =12U,Cyc_Absyn_Bitand =13U,Cyc_Absyn_Bitor =14U,Cyc_Absyn_Bitxor =15U,Cyc_Absyn_Bitlshift =16U,Cyc_Absyn_Bitlrshift =17U,Cyc_Absyn_Numelts =18U,Cyc_Absyn_Tagof =19U,Cyc_Absyn_UDiv =20U,Cyc_Absyn_UMod =21U,Cyc_Absyn_UGt =22U,Cyc_Absyn_ULt =23U,Cyc_Absyn_UGte =24U,Cyc_Absyn_ULte =25U};
# 531
enum Cyc_Absyn_Incrementor{Cyc_Absyn_PreInc =0U,Cyc_Absyn_PostInc =1U,Cyc_Absyn_PreDec =2U,Cyc_Absyn_PostDec =3U};struct Cyc_Absyn_VarargCallInfo{int num_varargs;struct Cyc_List_List*injectors;struct Cyc_Absyn_VarargInfo*vai;};struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct{int tag;struct _fat_ptr*f1;};
# 549
enum Cyc_Absyn_Coercion{Cyc_Absyn_Unknown_coercion =0U,Cyc_Absyn_No_coercion =1U,Cyc_Absyn_Null_to_NonNull =2U,Cyc_Absyn_Other_coercion =3U};struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_FieldName_Absyn_Designator_struct{int tag;struct _fat_ptr*f1;};
# 563
enum Cyc_Absyn_MallocKind{Cyc_Absyn_Malloc =0U,Cyc_Absyn_Calloc =1U,Cyc_Absyn_Vmalloc =2U};struct Cyc_Absyn_MallocInfo{enum Cyc_Absyn_MallocKind mknd;struct Cyc_Absyn_Exp*rgn;struct Cyc_Absyn_Exp*aqual;void**elt_type;struct Cyc_Absyn_Exp*num_elts;int fat_result;int inline_call;};struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct{int tag;union Cyc_Absyn_Cnst f1;};struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct{int tag;enum Cyc_Absyn_Primop f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;enum Cyc_Absyn_Incrementor f2;};struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_VarargCallInfo*f3;int f4;};struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;};struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Exp*f2;int f3;enum Cyc_Absyn_Coercion f4;};struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct _tuple9{struct _fat_ptr*f0;struct Cyc_Absyn_Tqual f1;void*f2;};struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct{int tag;struct _tuple9*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;int f4;};struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;int f3;};struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*f4;};struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct{int tag;void*f1;int f2;struct Cyc_List_List*f3;};struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_MallocInfo f1;};struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;};struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct{int tag;int f1;struct _fat_ptr f2;struct Cyc_List_List*f3;struct Cyc_List_List*f4;struct Cyc_List_List*f5;};struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Assert_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;int f3;};struct Cyc_Absyn_Assert_false_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Exp{void*topt;void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;struct Cyc_Absyn_Stmt*f3;};struct _tuple10{struct Cyc_Absyn_Exp*f0;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct{int tag;struct _tuple10 f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _tuple10 f2;struct _tuple10 f3;struct Cyc_Absyn_Stmt*f4;};struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Switch_clause**f2;};struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct _tuple10 f2;};struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Stmt{void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Pat*f1;};struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct{int tag;void*f1;int f2;struct Cyc_List_List*f3;struct Cyc_List_List*f4;int f5;};struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct{int tag;enum Cyc_Absyn_Sign f1;int f2;};struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct{int tag;char f1;};struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct{int tag;struct _fat_ptr f1;int f2;};struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_UnknownId_p_Absyn_Raw_pat_struct{int tag;struct _tuple1*f1;};struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;int f3;};struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Exp*f1;};
# 731 "absyn.h"
extern struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct Cyc_Absyn_Wild_p_val;struct Cyc_Absyn_Pat{void*r;void*topt;unsigned loc;};struct Cyc_Absyn_Switch_clause{struct Cyc_Absyn_Pat*pattern;struct Cyc_Core_Opt*pat_vars;struct Cyc_Absyn_Exp*where_clause;struct Cyc_Absyn_Stmt*body;unsigned loc;};struct Cyc_Absyn_Vardecl{enum Cyc_Absyn_Scope sc;struct _tuple1*name;unsigned varloc;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*initializer;void*rgn;struct Cyc_List_List*attributes;int escapes;int is_proto;struct Cyc_Absyn_Exp*rename;};struct Cyc_Absyn_Fndecl{enum Cyc_Absyn_Scope sc;int is_inline;struct _tuple1*name;struct Cyc_Absyn_Stmt*body;struct Cyc_Absyn_FnInfo i;void*cached_type;struct Cyc_Core_Opt*param_vardecls;struct Cyc_Absyn_Vardecl*fn_vardecl;enum Cyc_Absyn_Scope orig_scope;int escapes;};struct Cyc_Absyn_Aggrfield{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*width;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;};struct Cyc_Absyn_AggrdeclImpl{struct Cyc_List_List*exist_vars;struct Cyc_List_List*qual_bnd;struct Cyc_List_List*fields;int tagged;struct Cyc_List_List*effconstr;};struct Cyc_Absyn_Aggrdecl{enum Cyc_Absyn_AggrKind kind;enum Cyc_Absyn_Scope sc;struct _tuple1*name;struct Cyc_List_List*tvs;struct Cyc_Absyn_AggrdeclImpl*impl;struct Cyc_List_List*attributes;int expected_mem_kind;};struct Cyc_Absyn_Datatypefield{struct _tuple1*name;struct Cyc_List_List*typs;unsigned loc;enum Cyc_Absyn_Scope sc;};struct Cyc_Absyn_Datatypedecl{enum Cyc_Absyn_Scope sc;struct _tuple1*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*fields;int is_extensible;};struct Cyc_Absyn_Enumfield{struct _tuple1*name;struct Cyc_Absyn_Exp*tag;unsigned loc;};struct Cyc_Absyn_Enumdecl{enum Cyc_Absyn_Scope sc;struct _tuple1*name;struct Cyc_Core_Opt*fields;};struct Cyc_Absyn_Typedefdecl{struct _tuple1*name;struct Cyc_Absyn_Tqual tq;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*kind;void*defn;struct Cyc_List_List*atts;int extern_c;};struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Pat*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;void*f4;};struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Typedefdecl*f1;};struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct{int tag;struct _fat_ptr*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct _tuple11{unsigned f0;struct Cyc_List_List*f1;};struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct _tuple11*f4;};struct Cyc_Absyn_Decl{void*r;unsigned loc;};
# 918
int Cyc_Absyn_qvar_cmp(struct _tuple1*,struct _tuple1*);
# 933
struct Cyc_Absyn_Tqual Cyc_Absyn_empty_tqual(unsigned);
# 939
void*Cyc_Absyn_compress(void*);
# 943
int Cyc_Absyn_type2bool(int,void*);
# 952
void*Cyc_Absyn_new_evar(struct Cyc_Core_Opt*,struct Cyc_Core_Opt*);
# 1081
struct Cyc_Absyn_Exp*Cyc_Absyn_times_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
# 1109
struct Cyc_Absyn_Exp*Cyc_Absyn_sizeoftype_exp(void*,unsigned);struct _tuple12{enum Cyc_Absyn_AggrKind f0;struct _tuple1*f1;};
# 1227
struct _tuple12 Cyc_Absyn_aggr_kinded_name(union Cyc_Absyn_AggrInfo);
# 1235
struct _tuple1*Cyc_Absyn_binding2qvar(void*);struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;int gen_clean_cyclone;};
# 54 "absynpp.h"
void Cyc_Absynpp_set_params(struct Cyc_Absynpp_Params*);
# 62
struct _fat_ptr Cyc_Absynpp_longlong2string(unsigned long long);
# 65
struct _fat_ptr Cyc_Absynpp_kind2string(struct Cyc_Absyn_Kind*);
struct _fat_ptr Cyc_Absynpp_attribute2string(void*);
# 81
extern struct _fat_ptr*Cyc_Absynpp_cyc_stringptr;
int Cyc_Absynpp_exp_prec(struct Cyc_Absyn_Exp*);
struct _fat_ptr Cyc_Absynpp_char_escape(char);
struct _fat_ptr Cyc_Absynpp_string_escape(struct _fat_ptr);
struct _fat_ptr Cyc_Absynpp_prim2str(enum Cyc_Absyn_Primop);
int Cyc_Absynpp_is_declaration(struct Cyc_Absyn_Stmt*);struct _tuple13{struct Cyc_Absyn_Tqual f0;void*f1;struct Cyc_List_List*f2;};
struct _tuple13 Cyc_Absynpp_to_tms(struct _RegionHandle*,struct Cyc_Absyn_Tqual,void*);
# 25 "attributes.h"
extern struct Cyc_Absyn_Stdcall_att_Absyn_Attribute_struct Cyc_Atts_Stdcall_att_val;
extern struct Cyc_Absyn_Cdecl_att_Absyn_Attribute_struct Cyc_Atts_Cdecl_att_val;
extern struct Cyc_Absyn_Fastcall_att_Absyn_Attribute_struct Cyc_Atts_Fastcall_att_val;
# 33
extern struct Cyc_Absyn_Unused_att_Absyn_Attribute_struct Cyc_Atts_Unused_att_val;struct _tuple14{unsigned f0;int f1;};
# 28 "evexp.h"
extern struct _tuple14 Cyc_Evexp_eval_const_uint_exp(struct Cyc_Absyn_Exp*);
# 48 "warn.h"
void*Cyc_Warn_impos(struct _fat_ptr,struct _fat_ptr);struct Cyc_Warn_String_Warn_Warg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Warn_Typ_Warn_Warg_struct{int tag;void*f1;};
# 79
void*Cyc_Warn_impos2(struct _fat_ptr);
# 89 "kinds.h"
void*Cyc_Kinds_compress_kb(void*);
# 99 "flags.h"
enum Cyc_Flags_C_Compilers{Cyc_Flags_Gcc_c =0U,Cyc_Flags_Vc_c =1U};
# 103
extern enum Cyc_Flags_C_Compilers Cyc_Flags_c_compiler;struct _tuple15{struct _fat_ptr f0;unsigned f1;};
# 29 "absyndump.cyc"
extern struct _tuple15 Cyc_Lex_xlate_pos(unsigned);
# 38
static int Cyc_Absyndump_expand_typedefs;
# 42
static int Cyc_Absyndump_qvar_to_Cids;
# 45
static int Cyc_Absyndump_add_cyc_prefix;
# 48
static int Cyc_Absyndump_generate_line_directives;
# 52
static int Cyc_Absyndump_to_VC;static char _TmpG0[14U]="__NoCycPrefix";
# 54
static struct _fat_ptr Cyc_Absyndump_nocyc_str={_TmpG0,_TmpG0,_TmpG0 + 14U};
static struct _fat_ptr*Cyc_Absyndump_nocyc_strptr=(struct _fat_ptr*)& Cyc_Absyndump_nocyc_str;
# 57
void Cyc_Absyndump_set_params(struct Cyc_Absynpp_Params*fs){struct Cyc_Absynpp_Params*_T0;struct Cyc_Absynpp_Params*_T1;struct Cyc_Absynpp_Params*_T2;struct Cyc_Absynpp_Params*_T3;struct Cyc_Absynpp_Params*_T4;_T0=fs;
Cyc_Absyndump_expand_typedefs=_T0->expand_typedefs;_T1=fs;
Cyc_Absyndump_qvar_to_Cids=_T1->qvar_to_Cids;_T2=fs;
Cyc_Absyndump_add_cyc_prefix=_T2->add_cyc_prefix;_T3=fs;
Cyc_Absyndump_to_VC=_T3->to_VC;_T4=fs;
Cyc_Absyndump_generate_line_directives=_T4->generate_line_directives;
# 72 "absyndump.cyc"
Cyc_Absynpp_set_params(fs);}
# 75
static void Cyc_Absyndump_dumptyp(void*);
static void Cyc_Absyndump_dumpntyp(void*);
static void Cyc_Absyndump_dumpexp(struct Cyc_Absyn_Exp*);
static void Cyc_Absyndump_dumpexp_prec(int,struct Cyc_Absyn_Exp*);
static void Cyc_Absyndump_dumppat(struct Cyc_Absyn_Pat*);
static void Cyc_Absyndump_dumpstmt(struct Cyc_Absyn_Stmt*,int,struct Cyc_Dict_Dict);
static void Cyc_Absyndump_dumpvardecl(struct Cyc_Absyn_Vardecl*,unsigned,int);
static void Cyc_Absyndump_dump_aggrdecl(struct Cyc_Absyn_Aggrdecl*);
static void Cyc_Absyndump_dump_enumdecl(struct Cyc_Absyn_Enumdecl*);
static void Cyc_Absyndump_dump_datatypedecl(struct Cyc_Absyn_Datatypedecl*);
static void Cyc_Absyndump_dumpdecl(struct Cyc_Absyn_Decl*);
static void Cyc_Absyndump_dumptms(int,struct Cyc_List_List*,void(*)(void*),void*);
static void Cyc_Absyndump_dumptqtd(struct Cyc_Absyn_Tqual,void*,void(*)(void*),void*);
static void Cyc_Absyndump_dumpaggrfields(struct Cyc_List_List*);
static void Cyc_Absyndump_dumpenumfields(struct Cyc_List_List*);
# 97
struct Cyc___cycFILE**Cyc_Absyndump_dump_file=& Cyc_stdout;
static char Cyc_Absyndump_prev_char='x';
# 100
static struct Cyc_Dict_Dict*Cyc_Absyndump_mt_set_value=0;
static struct Cyc_Dict_Dict Cyc_Absyndump_mt_set (void){struct Cyc_Dict_Dict(*_T0)(int(*)(struct _tuple1*,struct _tuple1*));struct Cyc_Dict_Dict(*_T1)(int(*)(void*,void*));int(*_T2)(struct _tuple1*,struct _tuple1*);struct Cyc_Dict_Dict*_T3;struct Cyc_Dict_Dict _T4;struct Cyc_Dict_Dict*_T5;struct Cyc_Dict_Dict _T6;
if(Cyc_Absyndump_mt_set_value!=0)goto _TL0;_T1=Cyc_Dict_empty;{
struct Cyc_Dict_Dict(*_T7)(int(*)(struct _tuple1*,struct _tuple1*))=(struct Cyc_Dict_Dict(*)(int(*)(struct _tuple1*,struct _tuple1*)))_T1;_T0=_T7;}_T2=Cyc_Absyn_qvar_cmp;{struct Cyc_Dict_Dict s=_T0(_T2);{struct Cyc_Dict_Dict*_T7=_cycalloc(sizeof(struct Cyc_Dict_Dict));
*_T7=s;_T3=(struct Cyc_Dict_Dict*)_T7;}Cyc_Absyndump_mt_set_value=_T3;_T4=s;
return _T4;}_TL0: _T5=Cyc_Absyndump_mt_set_value;_T6=*_T5;
# 107
return _T6;}
# 110
static int Cyc_Absyndump_need_space_before (void){char _T0;int _T1;_T0=Cyc_Absyndump_prev_char;_T1=(int)_T0;switch(_T1){case 123: goto _LL4;case 125: _LL4: goto _LL6;case 40: _LL6: goto _LL8;case 41: _LL8: goto _LLA;case 91: _LLA: goto _LLC;case 93: _LLC: goto _LLE;case 59: _LLE: goto _LL10;case 44: _LL10: goto _LL12;case 61: _LL12: goto _LL14;case 63: _LL14: goto _LL16;case 33: _LL16: goto _LL18;case 32: _LL18: goto _LL1A;case 10: _LL1A: goto _LL1C;case 42: _LL1C:
# 125
 return 0;default:
 return 1;};}
# 130
static void Cyc_Absyndump_dump(struct _fat_ptr s){int _T0;struct Cyc___cycFILE**_T1;struct Cyc___cycFILE*_T2;struct _fat_ptr _T3;unsigned char*_T4;const char*_T5;unsigned _T6;int _T7;struct Cyc___cycFILE**_T8;struct Cyc___cycFILE*_T9;struct _fat_ptr _TA;unsigned _TB;int _TC;
unsigned sz=Cyc_strlen(s);
# 137
if(sz < 1U)goto _TL3;_T0=
Cyc_Absyndump_need_space_before();if(!_T0)goto _TL5;_T1=Cyc_Absyndump_dump_file;_T2=*_T1;
Cyc_fputc(32,_T2);goto _TL6;_TL5: _TL6: _T3=s;_T4=_T3.curr;_T5=(const char*)_T4;_T6=sz - 1U;_T7=(int)_T6;
Cyc_Absyndump_prev_char=_T5[_T7];_T8=Cyc_Absyndump_dump_file;_T9=*_T8;_TA=s;_TB=sz;_TC=(int)_TB;
Cyc_file_string_write(_T9,_TA,0,_TC);goto _TL4;_TL3: _TL4:;}
# 145
static void Cyc_Absyndump_dump_nospace(struct _fat_ptr s){unsigned long _T0;struct Cyc___cycFILE**_T1;struct Cyc___cycFILE*_T2;struct _fat_ptr _T3;int _T4;struct _fat_ptr _T5;unsigned char*_T6;const char*_T7;int _T8;_T0=
Cyc_strlen(s);{int sz=(int)_T0;
# 148
if(sz < 1)goto _TL7;_T1=Cyc_Absyndump_dump_file;_T2=*_T1;_T3=s;_T4=sz;
Cyc_file_string_write(_T2,_T3,0,_T4);_T5=s;_T6=_T5.curr;_T7=(const char*)_T6;_T8=sz - 1;
Cyc_Absyndump_prev_char=_T7[_T8];goto _TL8;_TL7: _TL8:;}}
# 153
static void Cyc_Absyndump_dump_char(int c){int _T0;struct Cyc___cycFILE**_T1;struct Cyc___cycFILE*_T2;int _T3;_T0=c;_T1=Cyc_Absyndump_dump_file;_T2=*_T1;
# 155
Cyc_fputc(_T0,_T2);_T3=c;
Cyc_Absyndump_prev_char=(char)_T3;}static char _TmpG1[1U]="";
# 159
static void Cyc_Absyndump_dumploc(unsigned loc){int _T0;struct _fat_ptr _T1;unsigned char*_T2;char*_T3;struct _fat_ptr _T4;unsigned char*_T5;char*_T6;struct _fat_ptr _T7;unsigned char*_T8;char*_T9;struct _fat_ptr _TA;unsigned char*_TB;char*_TC;struct _fat_ptr _TD;unsigned char*_TE;char*_TF;unsigned _T10;unsigned _T11;struct Cyc___cycFILE**_T12;struct Cyc___cycFILE*_T13;struct _fat_ptr _T14;struct _fat_ptr _T15;struct _fat_ptr _T16;unsigned char*_T17;char*_T18;struct _fat_ptr _T19;unsigned char*_T1A;char*_T1B;struct Cyc_Int_pa_PrintArg_struct _T1C;unsigned _T1D;int _T1E;void**_T1F;struct Cyc___cycFILE**_T20;struct Cyc___cycFILE*_T21;struct _fat_ptr _T22;struct _fat_ptr _T23;struct Cyc_Int_pa_PrintArg_struct _T24;unsigned _T25;int _T26;struct Cyc_String_pa_PrintArg_struct _T27;void**_T28;void**_T29;struct Cyc___cycFILE**_T2A;struct Cyc___cycFILE*_T2B;struct _fat_ptr _T2C;struct _fat_ptr _T2D;
static struct _fat_ptr last_file={_TmpG1,_TmpG1,_TmpG1 + 1U};
static unsigned last_line=0U;
static unsigned last_loc=0U;_T0=Cyc_Absyndump_generate_line_directives;
if(_T0)goto _TL9;else{goto _TLB;}_TLB: return;_TL9:
 if(loc==0U)goto _TLE;else{goto _TLF;}_TLF: if(loc==last_loc)goto _TLE;else{goto _TLC;}_TLE: return;_TLC:
 last_loc=loc;{
# 167
struct _tuple15 _T2E=Cyc_Lex_xlate_pos(loc);unsigned _T2F;struct _fat_ptr _T30;_T30=_T2E.f0;_T2F=_T2E.f1;{struct _fat_ptr f=_T30;unsigned d=_T2F;_T1=f;_T2=_T1.curr;_T3=(char*)_T2;
if(_T3==0)goto _TL12;else{goto _TL13;}_TL13: _T4=f;_T5=_T4.curr;_T6=(char*)_T5;_T7=last_file;_T8=_T7.curr;_T9=(char*)_T8;if(_T6==_T9)goto _TL14;else{goto _TL10;}_TL14: if(d==last_line)goto _TL12;else{goto _TL10;}_TL12: return;_TL10: _TA=f;_TB=_TA.curr;_TC=(char*)_TB;_TD=last_file;_TE=_TD.curr;_TF=(char*)_TE;
if(_TC!=_TF)goto _TL15;_T10=d;_T11=last_line + 1U;if(_T10!=_T11)goto _TL15;_T12=Cyc_Absyndump_dump_file;_T13=*_T12;_T14=
_tag_fat("\n",sizeof(char),2U);_T15=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T13,_T14,_T15);goto _TL16;
_TL15: _T16=f;_T17=_T16.curr;_T18=(char*)_T17;_T19=last_file;_T1A=_T19.curr;_T1B=(char*)_T1A;if(_T18!=_T1B)goto _TL17;{struct Cyc_Int_pa_PrintArg_struct _T31;_T31.tag=1;_T1D=d;_T1E=(int)_T1D;
_T31.f1=(unsigned long)_T1E;_T1C=_T31;}{struct Cyc_Int_pa_PrintArg_struct _T31=_T1C;void*_T32[1];_T1F=_T32 + 0;*_T1F=& _T31;_T20=Cyc_Absyndump_dump_file;_T21=*_T20;_T22=_tag_fat("\n# %d\n",sizeof(char),7U);_T23=_tag_fat(_T32,sizeof(void*),1);Cyc_fprintf(_T21,_T22,_T23);}goto _TL18;
# 174
_TL17:{struct Cyc_Int_pa_PrintArg_struct _T31;_T31.tag=1;_T25=d;_T26=(int)_T25;_T31.f1=(unsigned long)_T26;_T24=_T31;}{struct Cyc_Int_pa_PrintArg_struct _T31=_T24;{struct Cyc_String_pa_PrintArg_struct _T32;_T32.tag=0;_T32.f1=f;_T27=_T32;}{struct Cyc_String_pa_PrintArg_struct _T32=_T27;void*_T33[2];_T28=_T33 + 0;*_T28=& _T31;_T29=_T33 + 1;*_T29=& _T32;_T2A=Cyc_Absyndump_dump_file;_T2B=*_T2A;_T2C=_tag_fat("\n# %d %s\n",sizeof(char),10U);_T2D=_tag_fat(_T33,sizeof(void*),2);Cyc_fprintf(_T2B,_T2C,_T2D);}}_TL18: _TL16:
# 176
 last_file=f;
last_line=d;}}}
# 182
static void Cyc_Absyndump_dump_str(struct _fat_ptr*s){struct _fat_ptr*_T0;struct _fat_ptr _T1;_T0=s;_T1=*_T0;
Cyc_Absyndump_dump(_T1);}
# 185
static void Cyc_Absyndump_dump_str_nospace(struct _fat_ptr*s){struct _fat_ptr*_T0;struct _fat_ptr _T1;_T0=s;_T1=*_T0;
Cyc_Absyndump_dump_nospace(_T1);}
# 189
static void Cyc_Absyndump_dump_semi (void){
Cyc_Absyndump_dump_char(59);}
# 193
static void Cyc_Absyndump_dump_sep(void(*f)(void*),struct Cyc_List_List*l,struct _fat_ptr sep){struct Cyc_List_List*_T0;struct Cyc_List_List*_T1;struct Cyc_List_List*_T2;void*_T3;struct Cyc_List_List*_T4;struct Cyc_List_List*_T5;void*_T6;
if(l!=0)goto _TL19;
return;_TL19:
 _TL1E: _T0=_check_null(l);_T1=_T0->tl;if(_T1!=0)goto _TL1C;else{goto _TL1D;}
_TL1C: _T2=l;_T3=_T2->hd;f(_T3);
Cyc_Absyndump_dump_nospace(sep);_T4=l;
# 196
l=_T4->tl;goto _TL1E;_TL1D: _T5=l;_T6=_T5->hd;
# 200
f(_T6);}
# 202
static void Cyc_Absyndump_dump_sep_c(void(*f)(void*,void*),void*env,struct Cyc_List_List*l,struct _fat_ptr sep){struct Cyc_List_List*_T0;struct Cyc_List_List*_T1;void*_T2;struct Cyc_List_List*_T3;void*_T4;struct Cyc_List_List*_T5;void*_T6;struct Cyc_List_List*_T7;void*_T8;
if(l!=0)goto _TL1F;
return;_TL1F:
 _TL24: _T0=_check_null(l);_T1=_T0->tl;if(_T1!=0)goto _TL22;else{goto _TL23;}
_TL22: _T2=env;_T3=l;_T4=_T3->hd;f(_T2,_T4);
Cyc_Absyndump_dump_nospace(sep);_T5=l;
# 205
l=_T5->tl;goto _TL24;_TL23: _T6=env;_T7=l;_T8=_T7->hd;
# 209
f(_T6,_T8);}
# 211
static void Cyc_Absyndump_group(void(*f)(void*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep){void(*_T0)(void*);struct Cyc_List_List*_T1;struct _fat_ptr _T2;
# 213
Cyc_Absyndump_dump_nospace(start);_T0=f;_T1=l;_T2=sep;
Cyc_Absyndump_dump_sep(_T0,_T1,_T2);
Cyc_Absyndump_dump_nospace(end);}
# 217
static void Cyc_Absyndump_group_c(void(*f)(void*,void*),void*env,struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep){void(*_T0)(void*,void*);void*_T1;struct Cyc_List_List*_T2;struct _fat_ptr _T3;
# 219
Cyc_Absyndump_dump_nospace(start);_T0=f;_T1=env;_T2=l;_T3=sep;
Cyc_Absyndump_dump_sep_c(_T0,_T1,_T2,_T3);
Cyc_Absyndump_dump_nospace(end);}
# 223
static void Cyc_Absyndump_egroup(void(*f)(void*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep){void(*_T0)(void*);struct Cyc_List_List*_T1;struct _fat_ptr _T2;struct _fat_ptr _T3;struct _fat_ptr _T4;
# 225
if(l==0)goto _TL25;_T0=f;_T1=l;_T2=start;_T3=end;_T4=sep;
Cyc_Absyndump_group(_T0,_T1,_T2,_T3,_T4);goto _TL26;_TL25: _TL26:;}
# 229
static void Cyc_Absyndump_ignore(void*x){;}
# 231
static void Cyc_Absyndump_dumpqvar(struct _tuple1*v){struct _tuple1*_T0;struct _tuple1 _T1;struct _union_Nmspace_C_n _T2;unsigned _T3;struct _union_Nmspace_Rel_n _T4;struct _union_Nmspace_C_n _T5;struct _union_Nmspace_Abs_n _T6;int _T7;int _T8;int _T9;struct _fat_ptr _TA;int _TB;struct Cyc_List_List*_TC;unsigned _TD;struct Cyc_List_List*_TE;void*_TF;struct _fat_ptr*_T10;struct _fat_ptr*_T11;int _T12;struct Cyc_List_List*_T13;struct _fat_ptr**_T14;struct _fat_ptr*_T15;struct _fat_ptr _T16;void(*_T17)(void(*)(struct _fat_ptr*),struct Cyc_List_List*,struct _fat_ptr);struct Cyc_List_List*_T18;struct _fat_ptr _T19;struct _tuple1*_T1A;struct _tuple1 _T1B;struct _fat_ptr*_T1C;struct _fat_ptr _T1D;
struct Cyc_List_List*nsl=0;
struct _fat_ptr**prefix=0;_T0=v;_T1=*_T0;{
union Cyc_Absyn_Nmspace _T1E=_T1.f0;struct Cyc_List_List*_T1F;_T2=_T1E.C_n;_T3=_T2.tag;switch(_T3){case 4: goto _LL0;case 2: _T4=_T1E.Rel_n;_T1F=_T4.val;{struct Cyc_List_List*x=_T1F;
# 236
nsl=x;goto _LL0;}case 3: _T5=_T1E.C_n;_T1F=_T5.val;{struct Cyc_List_List*x=_T1F;goto _LL0;}default: _T6=_T1E.Abs_n;_T1F=_T6.val;{struct Cyc_List_List*x=_T1F;_T7=Cyc_Absyndump_qvar_to_Cids;
# 239
if(_T7)goto _TL2A;else{goto _TL2B;}_TL2B: _T8=Cyc_Absyndump_add_cyc_prefix;if(_T8)goto _TL2A;else{goto _TL28;}
_TL2A: prefix=& Cyc_Absynpp_cyc_stringptr;goto _TL29;_TL28: _TL29:
 nsl=x;goto _LL0;}}_LL0:;}_T9=
# 244
Cyc_Absyndump_need_space_before();if(!_T9)goto _TL2C;
Cyc_Absyndump_dump_char(32);goto _TL2D;_TL2C: _TL2D: _TB=Cyc_Absyndump_qvar_to_Cids;
if(!_TB)goto _TL2E;_TA=_tag_fat("_",sizeof(char),2U);goto _TL2F;_TL2E: _TA=_tag_fat("::",sizeof(char),3U);_TL2F: {struct _fat_ptr sep=_TA;_TC=nsl;_TD=(unsigned)_TC;
if(!_TD)goto _TL30;_TE=nsl;_TF=_TE->hd;_T10=(struct _fat_ptr*)_TF;_T11=Cyc_Absyndump_nocyc_strptr;_T12=Cyc_strptrcmp(_T10,_T11);if(_T12)goto _TL30;else{goto _TL32;}
_TL32: _T13=nsl;nsl=_T13->tl;
prefix=0;goto _TL31;_TL30: _TL31:
# 251
 if(prefix==0)goto _TL33;_T14=prefix;_T15=*_T14;_T16=*_T15;
Cyc_Absyndump_dump_nospace(_T16);Cyc_Absyndump_dump_nospace(sep);goto _TL34;_TL33: _TL34:
# 254
 if(nsl==0)goto _TL35;{
void(*_T1E)(void(*)(struct _fat_ptr*),struct Cyc_List_List*,struct _fat_ptr)=(void(*)(void(*)(struct _fat_ptr*),struct Cyc_List_List*,struct _fat_ptr))Cyc_Absyndump_dump_sep;_T17=_T1E;}_T18=nsl;_T19=sep;_T17(Cyc_Absyndump_dump_str_nospace,_T18,_T19);Cyc_Absyndump_dump_nospace(sep);goto _TL36;_TL35: _TL36: _T1A=v;_T1B=*_T1A;_T1C=_T1B.f1;_T1D=*_T1C;
# 257
Cyc_Absyndump_dump_nospace(_T1D);}}
# 260
static void Cyc_Absyndump_dumptq(struct Cyc_Absyn_Tqual tq){struct Cyc_Absyn_Tqual _T0;int _T1;struct _fat_ptr _T2;struct Cyc_Absyn_Tqual _T3;int _T4;struct _fat_ptr _T5;struct Cyc_Absyn_Tqual _T6;int _T7;struct _fat_ptr _T8;_T0=tq;_T1=_T0.q_restrict;
if(!_T1)goto _TL37;_T2=_tag_fat("restrict",sizeof(char),9U);Cyc_Absyndump_dump(_T2);goto _TL38;_TL37: _TL38: _T3=tq;_T4=_T3.q_volatile;
if(!_T4)goto _TL39;_T5=_tag_fat("volatile",sizeof(char),9U);Cyc_Absyndump_dump(_T5);goto _TL3A;_TL39: _TL3A: _T6=tq;_T7=_T6.print_const;
if(!_T7)goto _TL3B;_T8=_tag_fat("const",sizeof(char),6U);Cyc_Absyndump_dump(_T8);goto _TL3C;_TL3B: _TL3C:;}
# 265
static void Cyc_Absyndump_dumpscope(enum Cyc_Absyn_Scope sc){enum Cyc_Absyn_Scope _T0;int _T1;struct _fat_ptr _T2;struct _fat_ptr _T3;struct _fat_ptr _T4;struct _fat_ptr _T5;struct _fat_ptr _T6;_T0=sc;_T1=(int)_T0;switch(_T1){case Cyc_Absyn_Static: _T2=
# 267
_tag_fat("static",sizeof(char),7U);Cyc_Absyndump_dump(_T2);return;case Cyc_Absyn_Public:
 return;case Cyc_Absyn_Extern: _T3=
_tag_fat("extern",sizeof(char),7U);Cyc_Absyndump_dump(_T3);return;case Cyc_Absyn_ExternC: _T4=
_tag_fat("extern \"C\"",sizeof(char),11U);Cyc_Absyndump_dump(_T4);return;case Cyc_Absyn_Abstract: _T5=
_tag_fat("abstract",sizeof(char),9U);Cyc_Absyndump_dump(_T5);return;case Cyc_Absyn_Register: _T6=
_tag_fat("register",sizeof(char),9U);Cyc_Absyndump_dump(_T6);return;default:
 return;};}
# 276
static void Cyc_Absyndump_dumpaggr_kind(enum Cyc_Absyn_AggrKind k){enum Cyc_Absyn_AggrKind _T0;struct _fat_ptr _T1;struct _fat_ptr _T2;_T0=k;if(_T0!=Cyc_Absyn_StructA)goto _TL3E;_T1=
# 278
_tag_fat("struct",sizeof(char),7U);Cyc_Absyndump_dump(_T1);return;_TL3E: _T2=
_tag_fat("union",sizeof(char),6U);Cyc_Absyndump_dump(_T2);return;;}
# 283
static void Cyc_Absyndump_dumpkind(struct Cyc_Absyn_Kind*ka){struct _fat_ptr _T0;_T0=
Cyc_Absynpp_kind2string(ka);Cyc_Absyndump_dump(_T0);}
# 287
static void Cyc_Absyndump_dumptps(struct Cyc_List_List*ts){void(*_T0)(void*);struct Cyc_List_List*_T1;struct _fat_ptr _T2;struct _fat_ptr _T3;struct _fat_ptr _T4;_T0=Cyc_Absyndump_dumptyp;_T1=ts;_T2=
_tag_fat("<",sizeof(char),2U);_T3=_tag_fat(">",sizeof(char),2U);_T4=_tag_fat(",",sizeof(char),2U);Cyc_Absyndump_egroup(_T0,_T1,_T2,_T3,_T4);}
# 290
static void Cyc_Absyndump_dumptvar(struct Cyc_Absyn_Tvar*tv){struct Cyc_Absyn_Tvar*_T0;struct _fat_ptr*_T1;struct _fat_ptr _T2;char*_T3;const char*_T4;char _T5;int _T6;struct _fat_ptr _T7;struct Cyc_Absyn_Tvar*_T8;void*_T9;int*_TA;unsigned _TB;struct _fat_ptr _TC;struct _fat_ptr _TD;struct _fat_ptr _TE;_T0=tv;_T1=_T0->name;{
struct _fat_ptr n=*_T1;_T2=n;_T3=_check_fat_subscript(_T2,sizeof(char),0);_T4=(const char*)_T3;_T5=*_T4;_T6=(int)_T5;
if(_T6!=35)goto _TL40;_T7=
_tag_fat("`G",sizeof(char),3U);Cyc_Absyndump_dump(_T7);_T8=tv;_T9=_T8->kind;{
void*_TF=Cyc_Kinds_compress_kb(_T9);struct Cyc_Absyn_Kind*_T10;_TA=(int*)_TF;_TB=*_TA;switch(_TB){case 0:{struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*_T11=(struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_TF;_T10=_T11->f1;}{struct Cyc_Absyn_Kind*k=_T10;_T10=k;goto _LL4;}case 2:{struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_T11=(struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_TF;_T10=_T11->f2;}_LL4: {struct Cyc_Absyn_Kind*k=_T10;
# 296
Cyc_Absyndump_dumpkind(k);goto _LL0;}default: _TC=
_tag_fat("K",sizeof(char),2U);Cyc_Absyndump_dump_nospace(_TC);goto _LL0;}_LL0:;}_TD=n;_TE=
# 299
_fat_ptr_plus(_TD,sizeof(char),1);Cyc_Absyndump_dump_nospace(_TE);goto _TL41;
# 301
_TL40: Cyc_Absyndump_dump(n);_TL41:;}}
# 303
static void Cyc_Absyndump_dumpkindedtvar(struct Cyc_Absyn_Tvar*tv){struct Cyc_Absyn_Tvar*_T0;void*_T1;int*_T2;unsigned _T3;struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*_T4;struct Cyc_Absyn_Kind*_T5;struct Cyc_Absyn_Kind*_T6;enum Cyc_Absyn_KindQual _T7;struct _fat_ptr _T8;
Cyc_Absyndump_dumptvar(tv);_T0=tv;_T1=_T0->kind;{
void*_T9=Cyc_Kinds_compress_kb(_T1);struct Cyc_Absyn_Kind*_TA;_T2=(int*)_T9;_T3=*_T2;switch(_T3){case 1: goto _LL4;case 2: _LL4: goto _LL6;default: _T4=(struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_T9;_T5=_T4->f1;_T6=(struct Cyc_Absyn_Kind*)_T5;_T7=_T6->kind;if(_T7!=Cyc_Absyn_BoxKind)goto _TL44;_LL6: goto _LL0;_TL44:{struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*_TB=(struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_T9;_TA=_TB->f1;}{struct Cyc_Absyn_Kind*k=_TA;_T8=
# 309
_tag_fat("::",sizeof(char),3U);Cyc_Absyndump_dump(_T8);Cyc_Absyndump_dumpkind(k);goto _LL0;}}_LL0:;}}
# 312
static void Cyc_Absyndump_dumptvars(struct Cyc_List_List*tvs){void(*_T0)(void(*)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr);struct Cyc_List_List*_T1;struct _fat_ptr _T2;struct _fat_ptr _T3;struct _fat_ptr _T4;{
void(*_T5)(void(*)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr)=(void(*)(void(*)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr))Cyc_Absyndump_egroup;_T0=_T5;}_T1=tvs;_T2=_tag_fat("<",sizeof(char),2U);_T3=_tag_fat(">",sizeof(char),2U);_T4=_tag_fat(",",sizeof(char),2U);_T0(Cyc_Absyndump_dumptvar,_T1,_T2,_T3,_T4);}
# 315
static void Cyc_Absyndump_dumpkindedtvars(struct Cyc_List_List*tvs){void(*_T0)(void(*)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr);struct Cyc_List_List*_T1;struct _fat_ptr _T2;struct _fat_ptr _T3;struct _fat_ptr _T4;{
void(*_T5)(void(*)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr)=(void(*)(void(*)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr))Cyc_Absyndump_egroup;_T0=_T5;}_T1=tvs;_T2=_tag_fat("<",sizeof(char),2U);_T3=_tag_fat(">",sizeof(char),2U);_T4=_tag_fat(",",sizeof(char),2U);_T0(Cyc_Absyndump_dumpkindedtvar,_T1,_T2,_T3,_T4);}struct _tuple16{struct Cyc_Absyn_Tqual f0;void*f1;};
# 318
static void Cyc_Absyndump_dumparg(struct _tuple16*pr){void(*_T0)(struct Cyc_Absyn_Tqual,void*,void(*)(int),int);void(*_T1)(struct Cyc_Absyn_Tqual,void*,void(*)(void*),void*);struct _tuple16*_T2;struct _tuple16 _T3;struct Cyc_Absyn_Tqual _T4;struct _tuple16*_T5;struct _tuple16 _T6;void*_T7;void(*_T8)(int);_T1=Cyc_Absyndump_dumptqtd;{
void(*_T9)(struct Cyc_Absyn_Tqual,void*,void(*)(int),int)=(void(*)(struct Cyc_Absyn_Tqual,void*,void(*)(int),int))_T1;_T0=_T9;}_T2=pr;_T3=*_T2;_T4=_T3.f0;_T5=pr;_T6=*_T5;_T7=_T6.f1;{void(*_T9)(int)=(void(*)(int))Cyc_Absyndump_ignore;_T8=_T9;}_T0(_T4,_T7,_T8,0);}
# 321
static void Cyc_Absyndump_dumpargs(struct Cyc_List_List*ts){void(*_T0)(void(*)(struct _tuple16*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr);struct Cyc_List_List*_T1;struct _fat_ptr _T2;struct _fat_ptr _T3;struct _fat_ptr _T4;{
void(*_T5)(void(*)(struct _tuple16*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr)=(void(*)(void(*)(struct _tuple16*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr))Cyc_Absyndump_group;_T0=_T5;}_T1=ts;_T2=_tag_fat("(",sizeof(char),2U);_T3=_tag_fat(")",sizeof(char),2U);_T4=_tag_fat(",",sizeof(char),2U);_T0(Cyc_Absyndump_dumparg,_T1,_T2,_T3,_T4);}
# 325
static void Cyc_Absyndump_dump_callconv(struct Cyc_List_List*atts){struct Cyc_List_List*_T0;int*_T1;unsigned _T2;struct _fat_ptr _T3;struct _fat_ptr _T4;struct _fat_ptr _T5;struct Cyc_List_List*_T6;
_TL49: if(atts!=0)goto _TL47;else{goto _TL48;}
_TL47: _T0=atts;{void*_T7=_T0->hd;_T1=(int*)_T7;_T2=*_T1;switch(_T2){case 1: _T3=
_tag_fat("_stdcall",sizeof(char),9U);Cyc_Absyndump_dump(_T3);return;case 2: _T4=
_tag_fat("_cdecl",sizeof(char),7U);Cyc_Absyndump_dump(_T4);return;case 3: _T5=
_tag_fat("_fastcall",sizeof(char),10U);Cyc_Absyndump_dump(_T5);return;default: goto _LL0;}_LL0:;}_T6=atts;
# 326
atts=_T6->tl;goto _TL49;_TL48:;}
# 335
static void Cyc_Absyndump_dump_noncallconv(struct Cyc_List_List*atts){struct Cyc_List_List*_T0;int*_T1;unsigned _T2;struct Cyc_List_List*_T3;int _T4;struct _fat_ptr _T5;struct Cyc_List_List*_T6;int*_T7;unsigned _T8;struct Cyc_List_List*_T9;void*_TA;struct _fat_ptr _TB;struct Cyc_List_List*_TC;
# 337
int hasatt=0;{
struct Cyc_List_List*atts2=atts;_TL4E: if(atts2!=0)goto _TL4C;else{goto _TL4D;}
_TL4C: _T0=atts;{void*_TD=_T0->hd;_T1=(int*)_TD;_T2=*_T1;switch(_T2){case 1: goto _LL4;case 2: _LL4: goto _LL6;case 3: _LL6: goto _LL0;default:
# 343
 hasatt=1;goto _LL0;}_LL0:;}_T3=atts2;
# 338
atts2=_T3->tl;goto _TL4E;_TL4D:;}_T4=hasatt;
# 345
if(_T4)goto _TL50;else{goto _TL52;}
_TL52: return;_TL50: _T5=
_tag_fat("__declspec(",sizeof(char),12U);Cyc_Absyndump_dump(_T5);
_TL56: if(atts!=0)goto _TL54;else{goto _TL55;}
_TL54: _T6=atts;{void*_TD=_T6->hd;_T7=(int*)_TD;_T8=*_T7;switch(_T8){case 1: goto _LLD;case 2: _LLD: goto _LLF;case 3: _LLF: goto _LL9;default: _T9=atts;_TA=_T9->hd;_TB=
# 353
Cyc_Absynpp_attribute2string(_TA);Cyc_Absyndump_dump(_TB);goto _LL9;}_LL9:;}_TC=atts;
# 348
atts=_TC->tl;goto _TL56;_TL55:
# 355
 Cyc_Absyndump_dump_char(41);}
# 358
static void Cyc_Absyndump_dumpatts(struct Cyc_List_List*atts){enum Cyc_Flags_C_Compilers _T0;struct _fat_ptr _T1;struct Cyc_List_List*_T2;void*_T3;struct _fat_ptr _T4;struct Cyc_List_List*_T5;struct Cyc_List_List*_T6;struct _fat_ptr _T7;struct Cyc_List_List*_T8;struct _fat_ptr _T9;
if(atts!=0)goto _TL58;return;_TL58: _T0=Cyc_Flags_c_compiler;if(_T0!=Cyc_Flags_Gcc_c)goto _TL5A;_T1=
# 362
_tag_fat(" __attribute__((",sizeof(char),17U);Cyc_Absyndump_dump(_T1);
_TL5F: if(atts!=0)goto _TL5D;else{goto _TL5E;}
_TL5D: _T2=atts;_T3=_T2->hd;_T4=Cyc_Absynpp_attribute2string(_T3);Cyc_Absyndump_dump(_T4);_T5=atts;_T6=_T5->tl;
if(_T6==0)goto _TL60;_T7=_tag_fat(",",sizeof(char),2U);Cyc_Absyndump_dump(_T7);goto _TL61;_TL60: _TL61: _T8=atts;
# 363
atts=_T8->tl;goto _TL5F;_TL5E: _T9=
# 367
_tag_fat(")) ",sizeof(char),4U);Cyc_Absyndump_dump(_T9);
return;_TL5A:
# 370
 Cyc_Absyndump_dump_noncallconv(atts);
return;;}
# 375
static void Cyc_Absyndump_dumprgn(void*t){
Cyc_Absyndump_dumpntyp(t);}struct _tuple17{struct Cyc_List_List*f0;struct Cyc_List_List*f1;};
# 379
static struct _tuple17 Cyc_Absyndump_effects_split(void*t){int*_T0;int _T1;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T2;void*_T3;int*_T4;int _T5;struct Cyc_List_List*_T6;void*_T7;struct Cyc_List_List*_T8;struct Cyc_List_List*_T9;struct _tuple17 _TA;
struct Cyc_List_List*rgions=0;
struct Cyc_List_List*effects=0;{
void*_TB=Cyc_Absyn_compress(t);struct Cyc_List_List*_TC;_T0=(int*)_TB;_T1=*_T0;if(_T1!=0)goto _TL62;_T2=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_TB;_T3=_T2->f1;_T4=(int*)_T3;_T5=*_T4;if(_T5!=9)goto _TL64;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_TD=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_TB;_TC=_TD->f2;}{struct Cyc_List_List*ts=_TC;
# 385
_TL69: if(ts!=0)goto _TL67;else{goto _TL68;}
_TL67: _T6=ts;_T7=_T6->hd;{struct _tuple17 _TD=Cyc_Absyndump_effects_split(_T7);struct Cyc_List_List*_TE;struct Cyc_List_List*_TF;_TF=_TD.f0;_TE=_TD.f1;{struct Cyc_List_List*rs=_TF;struct Cyc_List_List*es=_TE;
rgions=Cyc_List_imp_append(rs,rgions);
effects=Cyc_List_imp_append(es,effects);}}_T8=ts;
# 385
ts=_T8->tl;goto _TL69;_TL68: goto _LL0;}_TL64: goto _LL3;_TL62: _LL3:{struct Cyc_List_List*_TD=_cycalloc(sizeof(struct Cyc_List_List));
# 391
_TD->hd=t;_TD->tl=effects;_T9=(struct Cyc_List_List*)_TD;}effects=_T9;goto _LL0;_LL0:;}{struct _tuple17 _TB;
# 393
_TB.f0=rgions;_TB.f1=effects;_TA=_TB;}return _TA;}
# 396
static void Cyc_Absyndump_dumpeff(void*t){void(*_T0)(void*);struct Cyc_List_List*_T1;struct _fat_ptr _T2;struct _fat_ptr _T3;struct _fat_ptr _T4;struct Cyc_List_List*_T5;struct _fat_ptr _T6;struct _fat_ptr _T7;struct _fat_ptr _T8;
struct _tuple17 _T9=Cyc_Absyndump_effects_split(t);struct Cyc_List_List*_TA;struct Cyc_List_List*_TB;_TB=_T9.f0;_TA=_T9.f1;{struct Cyc_List_List*rgions=_TB;struct Cyc_List_List*effects=_TA;
rgions=Cyc_List_imp_rev(rgions);
effects=Cyc_List_imp_rev(effects);
if(effects==0)goto _TL6A;_T0=Cyc_Absyndump_dumpntyp;_T1=effects;_T2=
_tag_fat("",sizeof(char),1U);_T3=_tag_fat("",sizeof(char),1U);_T4=_tag_fat("+",sizeof(char),2U);Cyc_Absyndump_group(_T0,_T1,_T2,_T3,_T4);
if(rgions==0)goto _TL6C;Cyc_Absyndump_dump_char(43);goto _TL6D;_TL6C: _TL6D: goto _TL6B;_TL6A: _TL6B:
# 404
 if(rgions!=0)goto _TL70;else{goto _TL71;}_TL71: if(effects==0)goto _TL70;else{goto _TL6E;}
_TL70: _T5=rgions;_T6=_tag_fat("{",sizeof(char),2U);_T7=_tag_fat("}",sizeof(char),2U);_T8=_tag_fat(",",sizeof(char),2U);Cyc_Absyndump_group(Cyc_Absyndump_dumprgn,_T5,_T6,_T7,_T8);goto _TL6F;_TL6E: _TL6F:;}}
# 408
static void Cyc_Absyndump_dump_aqual_val(enum Cyc_Absyn_AliasQualVal aqv){enum Cyc_Absyn_AliasQualVal _T0;int _T1;struct _fat_ptr _T2;struct _fat_ptr _T3;struct _fat_ptr _T4;struct _fat_ptr _T5;_T0=aqv;_T1=(int)_T0;switch(_T1){case Cyc_Absyn_Aliasable_qual: _T2=
# 410
_tag_fat("ALIASABLE",sizeof(char),10U);Cyc_Absyndump_dump(_T2);goto _LL0;case Cyc_Absyn_Unique_qual: _T3=
_tag_fat("UNIQUE",sizeof(char),7U);Cyc_Absyndump_dump(_T3);goto _LL0;case Cyc_Absyn_Refcnt_qual: _T4=
_tag_fat("REFCNT",sizeof(char),7U);Cyc_Absyndump_dump(_T4);goto _LL0;case Cyc_Absyn_Restricted_qual: _T5=
_tag_fat("RESTRICTED",sizeof(char),11U);Cyc_Absyndump_dump(_T5);goto _LL0;default: goto _LL0;}_LL0:;}
# 418
static struct _tuple16*Cyc_Absyndump_aggrfield2arg(struct Cyc_Absyn_Aggrfield*f){struct _tuple16*_T0;struct Cyc_Absyn_Aggrfield*_T1;struct Cyc_Absyn_Aggrfield*_T2;{struct _tuple16*_T3=_cycalloc(sizeof(struct _tuple16));_T1=f;
_T3->f0=_T1->tq;_T2=f;_T3->f1=_T2->type;_T0=(struct _tuple16*)_T3;}return _T0;}
# 423
static void Cyc_Absyndump_dumpntyp(void*t){void*_T0;int*_T1;unsigned _T2;void*_T3;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T4;void*_T5;int*_T6;int _T7;struct _fat_ptr _T8;void*_T9;void*_TA;void*_TB;int*_TC;unsigned _TD;void*_TE;union Cyc_Absyn_DatatypeInfo _TF;struct _union_DatatypeInfo_UnknownDatatype _T10;unsigned _T11;union Cyc_Absyn_DatatypeInfo _T12;struct _union_DatatypeInfo_UnknownDatatype _T13;struct Cyc_Absyn_UnknownDatatypeInfo _T14;union Cyc_Absyn_DatatypeInfo _T15;struct _union_DatatypeInfo_UnknownDatatype _T16;struct Cyc_Absyn_UnknownDatatypeInfo _T17;union Cyc_Absyn_DatatypeInfo _T18;struct _union_DatatypeInfo_KnownDatatype _T19;struct Cyc_Absyn_Datatypedecl**_T1A;int _T1B;struct _fat_ptr _T1C;struct _fat_ptr _T1D;void*_T1E;union Cyc_Absyn_DatatypeFieldInfo _T1F;struct _union_DatatypeFieldInfo_UnknownDatatypefield _T20;unsigned _T21;union Cyc_Absyn_DatatypeFieldInfo _T22;struct _union_DatatypeFieldInfo_UnknownDatatypefield _T23;struct Cyc_Absyn_UnknownDatatypeFieldInfo _T24;union Cyc_Absyn_DatatypeFieldInfo _T25;struct _union_DatatypeFieldInfo_UnknownDatatypefield _T26;struct Cyc_Absyn_UnknownDatatypeFieldInfo _T27;union Cyc_Absyn_DatatypeFieldInfo _T28;struct _union_DatatypeFieldInfo_UnknownDatatypefield _T29;struct Cyc_Absyn_UnknownDatatypeFieldInfo _T2A;union Cyc_Absyn_DatatypeFieldInfo _T2B;struct _union_DatatypeFieldInfo_KnownDatatypefield _T2C;struct _tuple2 _T2D;struct Cyc_Absyn_Datatypedecl*_T2E;union Cyc_Absyn_DatatypeFieldInfo _T2F;struct _union_DatatypeFieldInfo_KnownDatatypefield _T30;struct _tuple2 _T31;struct Cyc_Absyn_Datatypefield*_T32;int _T33;struct _fat_ptr _T34;struct _fat_ptr _T35;struct _fat_ptr _T36;struct _fat_ptr _T37;struct Cyc_List_List*_T38;void*_T39;void*_T3A;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T3B;enum Cyc_Absyn_Sign _T3C;int _T3D;void*_T3E;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T3F;enum Cyc_Absyn_Size_of _T40;int _T41;struct _fat_ptr _T42;void*_T43;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T44;enum Cyc_Absyn_Size_of _T45;int _T46;struct _fat_ptr _T47;struct _fat_ptr _T48;struct _fat_ptr _T49;struct _fat_ptr _T4A;enum Cyc_Flags_C_Compilers _T4B;struct _fat_ptr _T4C;struct _fat_ptr _T4D;void*_T4E;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T4F;enum Cyc_Absyn_Size_of _T50;int _T51;struct _fat_ptr _T52;struct _fat_ptr _T53;struct _fat_ptr _T54;struct _fat_ptr _T55;struct _fat_ptr _T56;void*_T57;int _T58;int _T59;struct _fat_ptr _T5A;struct _fat_ptr _T5B;struct _fat_ptr _T5C;void*_T5D;void*_T5E;struct _fat_ptr _T5F;void*_T60;struct _fat_ptr _T61;struct _fat_ptr _T62;void*_T63;struct _fat_ptr _T64;struct Cyc_List_List*_T65;void*_T66;struct _fat_ptr _T67;struct _fat_ptr _T68;struct Cyc_List_List*_T69;void*_T6A;struct _fat_ptr _T6B;struct _fat_ptr _T6C;struct _fat_ptr _T6D;struct _fat_ptr _T6E;struct _fat_ptr _T6F;struct Cyc_List_List*_T70;void*_T71;struct _fat_ptr _T72;void(*_T73)(void*);struct Cyc_List_List*_T74;struct _fat_ptr _T75;struct _fat_ptr _T76;struct _fat_ptr _T77;struct _fat_ptr _T78;struct _fat_ptr _T79;struct _fat_ptr _T7A;struct _fat_ptr _T7B;void*_T7C;struct _fat_ptr _T7D;struct Cyc_List_List*_T7E;void*_T7F;struct _fat_ptr _T80;struct _fat_ptr _T81;struct Cyc_List_List*_T82;void*_T83;struct _fat_ptr _T84;struct Cyc_List_List*_T85;void*_T86;struct Cyc_Warn_String_Warn_Warg_struct _T87;void**_T88;int(*_T89)(struct _fat_ptr);void*(*_T8A)(struct _fat_ptr);struct _fat_ptr _T8B;void*_T8C;void*_T8D;struct Cyc_Absyn_Cvar_Absyn_Type_struct*_T8E;void*_T8F;void*_T90;struct _fat_ptr _T91;struct _fat_ptr _T92;struct Cyc_Core_Opt*_T93;void*_T94;struct Cyc_Absyn_Kind*_T95;struct _fat_ptr _T96;const char*_T97;unsigned _T98;struct _fat_ptr _T99;void*_T9A;void*_T9B;unsigned _T9C;struct _fat_ptr _T9D;struct Cyc_Int_pa_PrintArg_struct _T9E;int _T9F;void**_TA0;struct _fat_ptr _TA1;struct _fat_ptr _TA2;void*_TA3;void*_TA4;void*_TA5;void*_TA6;struct Cyc_Absyn_Evar_Absyn_Type_struct*_TA7;void*_TA8;void*_TA9;struct _fat_ptr _TAA;struct _fat_ptr _TAB;struct Cyc_Core_Opt*_TAC;void*_TAD;struct Cyc_Absyn_Kind*_TAE;struct _fat_ptr _TAF;struct Cyc_Int_pa_PrintArg_struct _TB0;int _TB1;void**_TB2;struct _fat_ptr _TB3;struct _fat_ptr _TB4;void*_TB5;void*_TB6;void*_TB7;void*_TB8;int _TB9;struct Cyc_List_List*(*_TBA)(struct _tuple16*(*)(struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*);struct Cyc_List_List*(*_TBB)(void*(*)(void*),struct Cyc_List_List*);struct Cyc_List_List*_TBC;struct Cyc_List_List*_TBD;void*_TBE;struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*_TBF;struct Cyc_Absyn_TypeDecl*_TC0;struct Cyc_Absyn_TypeDecl*_TC1;void*_TC2;int*_TC3;unsigned _TC4;void*_TC5;struct Cyc_Absyn_TypeDecl*_TC6;void*_TC7;void*_TC8;struct Cyc_Absyn_TypeDecl*_TC9;void*_TCA;void*_TCB;struct Cyc_Absyn_TypeDecl*_TCC;void*_TCD;void*_TCE;void*_TCF;struct _fat_ptr _TD0;struct _fat_ptr _TD1;void*_TD2;struct _fat_ptr _TD3;struct _fat_ptr _TD4;struct Cyc_Absyn_Exp*_TD5;struct _tuple1*_TD6;struct Cyc_Absyn_Datatypedecl*_TD7;struct Cyc_Absyn_Enumdecl*_TD8;struct Cyc_Absyn_Aggrdecl*_TD9;enum Cyc_Absyn_AggrKind _TDA;const char*_TDB;int _TDC;struct Cyc_Core_Opt*_TDD;struct Cyc_List_List*_TDE;void*_TDF;_T0=t;_T1=(int*)_T0;_T2=*_T1;switch(_T2){case 5: goto _LL4;case 6: _LL4: goto _LL6;case 4: _LL6:
# 428
 return;case 0: _T3=t;_T4=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T3;_T5=_T4->f1;_T6=(int*)_T5;_T7=*_T6;if(_T7!=0)goto _TL74;_T8=
_tag_fat("void",sizeof(char),5U);Cyc_Absyndump_dump(_T8);return;_TL74: _T9=t;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_TE0=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T9;_TA=_TE0->f1;_TDF=(void*)_TA;_TDE=_TE0->f2;}{void*c=_TDF;struct Cyc_List_List*ts=_TDE;enum Cyc_Absyn_AliasQualVal _TE0;struct _fat_ptr _TE1;struct Cyc_List_List*_TE2;struct _tuple1*_TE3;union Cyc_Absyn_AggrInfo _TE4;int _TE5;union Cyc_Absyn_DatatypeFieldInfo _TE6;union Cyc_Absyn_DatatypeInfo _TE7;_TB=c;_TC=(int*)_TB;_TD=*_TC;switch(_TD){case 22: _TE=c;{struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*_TE8=(struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)_TE;_TE7=_TE8->f1;}{union Cyc_Absyn_DatatypeInfo tu_info=_TE7;{int _TE8;struct _tuple1*_TE9;_TF=tu_info;_T10=_TF.UnknownDatatype;_T11=_T10.tag;if(_T11!=1)goto _TL77;_T12=tu_info;_T13=_T12.UnknownDatatype;_T14=_T13.val;_TE9=_T14.name;_T15=tu_info;_T16=_T15.UnknownDatatype;_T17=_T16.val;_TE8=_T17.is_extensible;{struct _tuple1*n=_TE9;int is_x=_TE8;_TE9=n;_TE8=is_x;goto _LL76;}_TL77: _T18=tu_info;_T19=_T18.KnownDatatype;_T1A=_T19.val;{struct Cyc_Absyn_Datatypedecl*_TEA=*_T1A;struct Cyc_Absyn_Datatypedecl _TEB=*_TEA;_TE9=_TEB.name;_TE8=_TEB.is_extensible;}_LL76:{struct _tuple1*n=_TE9;int is_x=_TE8;_T1B=is_x;
# 466
if(!_T1B)goto _TL79;_T1C=_tag_fat("@extensible",sizeof(char),12U);Cyc_Absyndump_dump(_T1C);goto _TL7A;_TL79: _TL7A: _T1D=
_tag_fat("datatype",sizeof(char),9U);Cyc_Absyndump_dump(_T1D);Cyc_Absyndump_dumpqvar(n);Cyc_Absyndump_dumptps(ts);goto _LL72;}_LL72:;}
# 470
return;}case 23: _T1E=c;{struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*_TE8=(struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)_T1E;_TE6=_TE8->f1;}{union Cyc_Absyn_DatatypeFieldInfo tuf_info=_TE6;{int _TE8;struct _tuple1*_TE9;struct _tuple1*_TEA;_T1F=tuf_info;_T20=_T1F.UnknownDatatypefield;_T21=_T20.tag;if(_T21!=1)goto _TL7B;_T22=tuf_info;_T23=_T22.UnknownDatatypefield;_T24=_T23.val;_TEA=_T24.datatype_name;_T25=tuf_info;_T26=_T25.UnknownDatatypefield;_T27=_T26.val;_TE9=_T27.field_name;_T28=tuf_info;_T29=_T28.UnknownDatatypefield;_T2A=_T29.val;_TE8=_T2A.is_extensible;{struct _tuple1*tname=_TEA;struct _tuple1*fname=_TE9;int is_x=_TE8;_TEA=tname;_TE8=is_x;_TE9=fname;goto _LL7B;}_TL7B: _T2B=tuf_info;_T2C=_T2B.KnownDatatypefield;_T2D=_T2C.val;_T2E=_T2D.f0;{struct Cyc_Absyn_Datatypedecl _TEB=*_T2E;_TEA=_TEB.name;_TE8=_TEB.is_extensible;}_T2F=tuf_info;_T30=_T2F.KnownDatatypefield;_T31=_T30.val;_T32=_T31.f1;{struct Cyc_Absyn_Datatypefield _TEB=*_T32;_TE9=_TEB.name;}_LL7B:{struct _tuple1*tname=_TEA;int is_x=_TE8;struct _tuple1*fname=_TE9;_T33=is_x;
# 477
if(!_T33)goto _TL7D;_T34=_tag_fat("@extensible ",sizeof(char),13U);Cyc_Absyndump_dump(_T34);goto _TL7E;_TL7D: _TL7E: _T35=
_tag_fat("datatype ",sizeof(char),10U);Cyc_Absyndump_dump(_T35);Cyc_Absyndump_dumpqvar(tname);_T36=_tag_fat(".",sizeof(char),2U);Cyc_Absyndump_dump(_T36);Cyc_Absyndump_dumpqvar(fname);
Cyc_Absyndump_dumptps(ts);goto _LL77;}_LL77:;}
# 482
return;}case 3: _T37=
_tag_fat("_Complex ",sizeof(char),10U);Cyc_Absyndump_dump(_T37);_T38=_check_null(ts);_T39=_T38->hd;Cyc_Absyndump_dumpntyp(_T39);return;case 1: _T3A=c;_T3B=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T3A;_T3C=_T3B->f1;_T3D=(int)_T3C;switch(_T3D){case Cyc_Absyn_None: _T3E=c;_T3F=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T3E;_T40=_T3F->f2;_T41=(int)_T40;switch(_T41){case Cyc_Absyn_Int_sz: goto _LL2D;case Cyc_Absyn_Long_sz: goto _LL31;case Cyc_Absyn_Char_sz: _T42=
# 488
_tag_fat("char",sizeof(char),5U);Cyc_Absyndump_dump(_T42);return;case Cyc_Absyn_Short_sz: goto _LL3B;case Cyc_Absyn_LongLong_sz: _LL45: goto _LL47;default: goto _LL70;};case Cyc_Absyn_Signed: _T43=c;_T44=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T43;_T45=_T44->f2;_T46=(int)_T45;switch(_T46){case Cyc_Absyn_Int_sz: _LL2D: _T47=
# 485
_tag_fat("int",sizeof(char),4U);Cyc_Absyndump_dump(_T47);return;case Cyc_Absyn_Long_sz: _LL31: _T48=
# 487
_tag_fat("long",sizeof(char),5U);Cyc_Absyndump_dump(_T48);return;case Cyc_Absyn_Char_sz: _T49=
# 489
_tag_fat("signed char",sizeof(char),12U);Cyc_Absyndump_dump(_T49);return;case Cyc_Absyn_Short_sz: _LL3B: _T4A=
# 492
_tag_fat("short",sizeof(char),6U);Cyc_Absyndump_dump(_T4A);return;case Cyc_Absyn_LongLong_sz: _LL47: _T4B=Cyc_Flags_c_compiler;if(_T4B!=Cyc_Flags_Gcc_c)goto _TL82;_T4C=
# 500
_tag_fat("long long",sizeof(char),10U);Cyc_Absyndump_dump(_T4C);return;_TL82: _T4D=
_tag_fat("__int64",sizeof(char),8U);Cyc_Absyndump_dump(_T4D);return;;default: goto _LL70;};case Cyc_Absyn_Unsigned: _T4E=c;_T4F=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T4E;_T50=_T4F->f2;_T51=(int)_T50;switch(_T51){case Cyc_Absyn_Char_sz: _T52=
# 490
_tag_fat("unsigned char",sizeof(char),14U);Cyc_Absyndump_dump(_T52);return;case Cyc_Absyn_Short_sz: _T53=
# 493
_tag_fat("unsigned short",sizeof(char),15U);Cyc_Absyndump_dump(_T53);return;case Cyc_Absyn_Int_sz: _T54=
_tag_fat("unsigned",sizeof(char),9U);Cyc_Absyndump_dump(_T54);return;case Cyc_Absyn_Long_sz: _T55=
_tag_fat("unsigned long",sizeof(char),14U);Cyc_Absyndump_dump(_T55);return;case Cyc_Absyn_LongLong_sz: _T56=
_tag_fat("unsigned",sizeof(char),9U);Cyc_Absyndump_dump(_T56);goto _LL45;default: goto _LL70;};default: goto _LL70;};case 2: _T57=c;{struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*_TE8=(struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)_T57;_TE5=_TE8->f1;}{int i=_TE5;_T58=i;_T59=(int)_T58;switch(_T59){case 0: _T5A=
# 505
_tag_fat("float",sizeof(char),6U);Cyc_Absyndump_dump(_T5A);return;case 1: _T5B=
_tag_fat("double",sizeof(char),7U);Cyc_Absyndump_dump(_T5B);return;default: _T5C=
_tag_fat("long double",sizeof(char),12U);Cyc_Absyndump_dump(_T5C);return;};}case 24: _T5D=c;{struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*_TE8=(struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_T5D;_TE4=_TE8->f1;}{union Cyc_Absyn_AggrInfo info=_TE4;
# 510
struct _tuple12 _TE8=Cyc_Absyn_aggr_kinded_name(info);struct _tuple1*_TE9;enum Cyc_Absyn_AggrKind _TEA;_TEA=_TE8.f0;_TE9=_TE8.f1;{enum Cyc_Absyn_AggrKind k=_TEA;struct _tuple1*n=_TE9;
Cyc_Absyndump_dumpaggr_kind(k);Cyc_Absyndump_dumpqvar(n);Cyc_Absyndump_dumptps(ts);
return;}}case 19: _T5E=c;{struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*_TE8=(struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*)_T5E;_TE3=_TE8->f1;}{struct _tuple1*n=_TE3;_T5F=
_tag_fat("enum",sizeof(char),5U);Cyc_Absyndump_dump(_T5F);Cyc_Absyndump_dumpqvar(n);return;}case 20: _T60=c;{struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct*_TE8=(struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct*)_T60;_TE2=_TE8->f1;}{struct Cyc_List_List*fs=_TE2;_T61=
_tag_fat("enum{",sizeof(char),6U);Cyc_Absyndump_dump(_T61);Cyc_Absyndump_dumpenumfields(fs);_T62=_tag_fat("}",sizeof(char),2U);Cyc_Absyndump_dump(_T62);return;}case 21: _T63=c;{struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct*_TE8=(struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct*)_T63;_TE1=_TE8->f1;}{struct _fat_ptr t=_TE1;
Cyc_Absyndump_dump(t);return;}case 4: _T64=
_tag_fat("region_t<",sizeof(char),10U);Cyc_Absyndump_dump(_T64);_T65=_check_null(ts);_T66=_T65->hd;Cyc_Absyndump_dumprgn(_T66);_T67=_tag_fat(">",sizeof(char),2U);Cyc_Absyndump_dump(_T67);return;case 5: _T68=
_tag_fat("tag_t<",sizeof(char),7U);Cyc_Absyndump_dump(_T68);_T69=_check_null(ts);_T6A=_T69->hd;Cyc_Absyndump_dumptyp(_T6A);_T6B=_tag_fat(">",sizeof(char),2U);Cyc_Absyndump_dump(_T6B);return;case 7: _T6C=
_tag_fat("`U",sizeof(char),3U);Cyc_Absyndump_dump(_T6C);return;case 8: _T6D=
_tag_fat("`RC",sizeof(char),4U);Cyc_Absyndump_dump(_T6D);return;case 6: _T6E=
_tag_fat("`H",sizeof(char),3U);Cyc_Absyndump_dump(_T6E);return;case 10: _T6F=
# 522
_tag_fat("regions(",sizeof(char),9U);Cyc_Absyndump_dump(_T6F);_T70=_check_null(ts);_T71=_T70->hd;Cyc_Absyndump_dumptyp(_T71);_T72=_tag_fat(")",sizeof(char),2U);Cyc_Absyndump_dump(_T72);return;case 9: _T73=Cyc_Absyndump_dumptyp;_T74=ts;_T75=
_tag_fat("",sizeof(char),1U);_T76=_tag_fat("",sizeof(char),1U);_T77=_tag_fat("+",sizeof(char),2U);Cyc_Absyndump_group(_T73,_T74,_T75,_T76,_T77);return;case 11: _T78=
_tag_fat("@true",sizeof(char),6U);Cyc_Absyndump_dump(_T78);return;case 12: _T79=
_tag_fat("@false",sizeof(char),7U);Cyc_Absyndump_dump(_T79);return;case 13: _T7A=
_tag_fat("@thin",sizeof(char),6U);Cyc_Absyndump_dump(_T7A);return;case 14: _T7B=
_tag_fat("@fat",sizeof(char),5U);Cyc_Absyndump_dump(_T7B);return;case 16: _T7C=c;{struct Cyc_Absyn_AqualConstCon_Absyn_TyCon_struct*_TE8=(struct Cyc_Absyn_AqualConstCon_Absyn_TyCon_struct*)_T7C;_TE0=_TE8->f1;}{enum Cyc_Absyn_AliasQualVal v=_TE0;
Cyc_Absyndump_dump_aqual_val(v);return;}case 15: _T7D=
_tag_fat("aquals(",sizeof(char),8U);Cyc_Absyndump_dump(_T7D);_T7E=_check_null(ts);_T7F=_T7E->hd;Cyc_Absyndump_dumptyp(_T7F);_T80=_tag_fat(")",sizeof(char),2U);Cyc_Absyndump_dump(_T80);return;case 18: _T81=
_tag_fat("aqual_t<",sizeof(char),9U);Cyc_Absyndump_dump(_T81);_T82=_check_null(ts);_T83=_T82->hd;Cyc_Absyndump_dumptyp(_T83);_T84=_tag_fat(">",sizeof(char),2U);Cyc_Absyndump_dump(_T84);return;case 17: _T85=
_check_null(ts);_T86=_T85->hd;Cyc_Absyndump_dumptyp(_T86);return;default: _LL70:{struct Cyc_Warn_String_Warn_Warg_struct _TE8;_TE8.tag=0;
_TE8.f1=_tag_fat("bad type constructor",sizeof(char),21U);_T87=_TE8;}{struct Cyc_Warn_String_Warn_Warg_struct _TE8=_T87;void*_TE9[1];_T88=_TE9 + 0;*_T88=& _TE8;_T8A=Cyc_Warn_impos2;{int(*_TEA)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_T8A;_T89=_TEA;}_T8B=_tag_fat(_TE9,sizeof(void*),1);_T89(_T8B);}};};case 2: _T8C=t;{struct Cyc_Absyn_VarType_Absyn_Type_struct*_TE0=(struct Cyc_Absyn_VarType_Absyn_Type_struct*)_T8C;_TDF=_TE0->f1;}{struct Cyc_Absyn_Tvar*tv=_TDF;
# 430
Cyc_Absyndump_dumptvar(tv);return;}case 3: _T8D=t;_T8E=(struct Cyc_Absyn_Cvar_Absyn_Type_struct*)_T8D;_T8F=_T8E->f2;if(_T8F!=0)goto _TL86;_T90=t;{struct Cyc_Absyn_Cvar_Absyn_Type_struct*_TE0=(struct Cyc_Absyn_Cvar_Absyn_Type_struct*)_T90;_TDD=_TE0->f1;_TDC=_TE0->f3;_TDB=_TE0->f5;}{struct Cyc_Core_Opt*k=_TDD;int i=_TDC;const char*name=_TDB;_T91=
# 432
_tag_fat("`C_",sizeof(char),4U);Cyc_Absyndump_dump(_T91);
if(k!=0)goto _TL88;_T92=_tag_fat("K",sizeof(char),2U);Cyc_Absyndump_dump(_T92);goto _TL89;_TL88: _T93=k;_T94=_T93->v;_T95=(struct Cyc_Absyn_Kind*)_T94;Cyc_Absyndump_dumpkind(_T95);_TL89: _T96=
_tag_fat("_",sizeof(char),2U);Cyc_Absyndump_dump(_T96);_T97=name;_T98=(unsigned)_T97;
if(!_T98)goto _TL8A;{const char*_TE0=name;_T9A=(void*)_TE0;_T9B=(void*)_TE0;_T9C=_get_zero_arr_size_char(_T9B,1U);_T99=_tag_fat(_T9A,sizeof(char),_T9C);}
Cyc_Absyndump_dump(_T99);goto _TL8B;
# 438
_TL8A:{struct Cyc_Int_pa_PrintArg_struct _TE0;_TE0.tag=1;_T9F=i;_TE0.f1=(unsigned long)_T9F;_T9E=_TE0;}{struct Cyc_Int_pa_PrintArg_struct _TE0=_T9E;void*_TE1[1];_TA0=_TE1 + 0;*_TA0=& _TE0;_TA1=_tag_fat("%d",sizeof(char),3U);_TA2=_tag_fat(_TE1,sizeof(void*),1);_T9D=Cyc_aprintf(_TA1,_TA2);}Cyc_Absyndump_dump(_T9D);_TL8B:
 return;}_TL86: _TA3=t;{struct Cyc_Absyn_Cvar_Absyn_Type_struct*_TE0=(struct Cyc_Absyn_Cvar_Absyn_Type_struct*)_TA3;_TDD=_TE0->f1;_TA4=_TE0->f2;_TDF=(void*)_TA4;_TDC=_TE0->f3;}{struct Cyc_Core_Opt*k=_TDD;void*t=_TDF;int i=_TDC;_TA5=
# 444
_check_null(t);Cyc_Absyndump_dumpntyp(_TA5);return;}case 1: _TA6=t;_TA7=(struct Cyc_Absyn_Evar_Absyn_Type_struct*)_TA6;_TA8=_TA7->f2;if(_TA8!=0)goto _TL8C;_TA9=t;{struct Cyc_Absyn_Evar_Absyn_Type_struct*_TE0=(struct Cyc_Absyn_Evar_Absyn_Type_struct*)_TA9;_TDD=_TE0->f1;_TDC=_TE0->f3;}{struct Cyc_Core_Opt*k=_TDD;int i=_TDC;_TAA=
# 441
_tag_fat("`E",sizeof(char),3U);Cyc_Absyndump_dump(_TAA);
if(k!=0)goto _TL8E;_TAB=_tag_fat("K",sizeof(char),2U);Cyc_Absyndump_dump(_TAB);goto _TL8F;_TL8E: _TAC=k;_TAD=_TAC->v;_TAE=(struct Cyc_Absyn_Kind*)_TAD;Cyc_Absyndump_dumpkind(_TAE);_TL8F:{struct Cyc_Int_pa_PrintArg_struct _TE0;_TE0.tag=1;_TB1=i;
_TE0.f1=(unsigned long)_TB1;_TB0=_TE0;}{struct Cyc_Int_pa_PrintArg_struct _TE0=_TB0;void*_TE1[1];_TB2=_TE1 + 0;*_TB2=& _TE0;_TB3=_tag_fat("%d",sizeof(char),3U);_TB4=_tag_fat(_TE1,sizeof(void*),1);_TAF=Cyc_aprintf(_TB3,_TB4);}Cyc_Absyndump_dump(_TAF);return;}_TL8C: _TB5=t;{struct Cyc_Absyn_Evar_Absyn_Type_struct*_TE0=(struct Cyc_Absyn_Evar_Absyn_Type_struct*)_TB5;_TDD=_TE0->f1;_TB6=_TE0->f2;_TDF=(void*)_TB6;_TDC=_TE0->f3;}{struct Cyc_Core_Opt*k=_TDD;void*t=_TDF;int i=_TDC;_TB7=
# 445
_check_null(t);Cyc_Absyndump_dumpntyp(_TB7);return;}case 7: _TB8=t;{struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*_TE0=(struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_TB8;_TDA=_TE0->f1;_TDC=_TE0->f2;_TDE=_TE0->f3;}{enum Cyc_Absyn_AggrKind k=_TDA;int is_tuple=_TDC;struct Cyc_List_List*fs=_TDE;_TB9=is_tuple;
# 447
if(!_TB9)goto _TL90;
Cyc_Absyndump_dump_char(36);_TBB=Cyc_List_map;{
struct Cyc_List_List*(*_TE0)(struct _tuple16*(*)(struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct _tuple16*(*)(struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*))_TBB;_TBA=_TE0;}_TBC=fs;_TBD=_TBA(Cyc_Absyndump_aggrfield2arg,_TBC);Cyc_Absyndump_dumpargs(_TBD);goto _TL91;
# 451
_TL90: Cyc_Absyndump_dumpaggr_kind(k);Cyc_Absyndump_dump_char(123);Cyc_Absyndump_dumpaggrfields(fs);Cyc_Absyndump_dump_char(125);_TL91:
# 453
 return;}case 10: _TBE=t;_TBF=(struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_TBE;_TC0=_TBF->f1;_TC1=(struct Cyc_Absyn_TypeDecl*)_TC0;_TC2=_TC1->r;_TC3=(int*)_TC2;_TC4=*_TC3;switch(_TC4){case 0: _TC5=t;{struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*_TE0=(struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_TC5;_TC6=_TE0->f1;{struct Cyc_Absyn_TypeDecl _TE1=*_TC6;_TC7=_TE1.r;{struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct*_TE2=(struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct*)_TC7;_TD9=_TE2->f1;}}}{struct Cyc_Absyn_Aggrdecl*d=_TD9;
Cyc_Absyndump_dump_aggrdecl(d);return;}case 1: _TC8=t;{struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*_TE0=(struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_TC8;_TC9=_TE0->f1;{struct Cyc_Absyn_TypeDecl _TE1=*_TC9;_TCA=_TE1.r;{struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct*_TE2=(struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct*)_TCA;_TD8=_TE2->f1;}}}{struct Cyc_Absyn_Enumdecl*d=_TD8;
Cyc_Absyndump_dump_enumdecl(d);return;}default: _TCB=t;{struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*_TE0=(struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_TCB;_TCC=_TE0->f1;{struct Cyc_Absyn_TypeDecl _TE1=*_TCC;_TCD=_TE1.r;{struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct*_TE2=(struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct*)_TCD;_TD7=_TE2->f1;}}}{struct Cyc_Absyn_Datatypedecl*d=_TD7;
Cyc_Absyndump_dump_datatypedecl(d);return;}};case 8: _TCE=t;{struct Cyc_Absyn_TypedefType_Absyn_Type_struct*_TE0=(struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_TCE;_TD6=_TE0->f1;_TDE=_TE0->f2;}{struct _tuple1*n=_TD6;struct Cyc_List_List*ts=_TDE;
Cyc_Absyndump_dumpqvar(n);Cyc_Absyndump_dumptps(ts);return;}case 9: _TCF=t;{struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_TE0=(struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_TCF;_TD5=_TE0->f1;}{struct Cyc_Absyn_Exp*e=_TD5;_TD0=
_tag_fat("valueof_t(",sizeof(char),11U);Cyc_Absyndump_dump(_TD0);Cyc_Absyndump_dumpexp(e);_TD1=_tag_fat(")",sizeof(char),2U);Cyc_Absyndump_dump(_TD1);return;}default: _TD2=t;{struct Cyc_Absyn_TypeofType_Absyn_Type_struct*_TE0=(struct Cyc_Absyn_TypeofType_Absyn_Type_struct*)_TD2;_TD5=_TE0->f1;}{struct Cyc_Absyn_Exp*e=_TD5;_TD3=
_tag_fat("typeof(",sizeof(char),8U);Cyc_Absyndump_dump(_TD3);Cyc_Absyndump_dumpexp(e);_TD4=_tag_fat(")",sizeof(char),2U);Cyc_Absyndump_dump(_TD4);return;}};}
# 537
static void Cyc_Absyndump_dumpvaropt(struct _fat_ptr*vo){
if(vo==0)goto _TL93;Cyc_Absyndump_dump_str(vo);goto _TL94;_TL93: _TL94:;}
# 540
static void Cyc_Absyndump_dumpfunarg(struct _tuple9*t){void(*_T0)(struct Cyc_Absyn_Tqual,void*,void(*)(struct _fat_ptr*),struct _fat_ptr*);void(*_T1)(struct Cyc_Absyn_Tqual,void*,void(*)(void*),void*);struct _tuple9*_T2;struct _tuple9 _T3;struct Cyc_Absyn_Tqual _T4;struct _tuple9*_T5;struct _tuple9 _T6;void*_T7;struct _tuple9*_T8;struct _tuple9 _T9;struct _fat_ptr*_TA;_T1=Cyc_Absyndump_dumptqtd;{
void(*_TB)(struct Cyc_Absyn_Tqual,void*,void(*)(struct _fat_ptr*),struct _fat_ptr*)=(void(*)(struct Cyc_Absyn_Tqual,void*,void(*)(struct _fat_ptr*),struct _fat_ptr*))_T1;_T0=_TB;}_T2=t;_T3=*_T2;_T4=_T3.f1;_T5=t;_T6=*_T5;_T7=_T6.f2;_T8=t;_T9=*_T8;_TA=_T9.f0;_T0(_T4,_T7,Cyc_Absyndump_dumpvaropt,_TA);}
# 543
static void Cyc_Absyndump_dump_rgncmp(struct _tuple0*cmp){struct _tuple0*_T0;struct _tuple0 _T1;void*_T2;struct _tuple0*_T3;struct _tuple0 _T4;void*_T5;_T0=cmp;_T1=*_T0;_T2=_T1.f0;
Cyc_Absyndump_dumpeff(_T2);Cyc_Absyndump_dump_char(62);_T3=cmp;_T4=*_T3;_T5=_T4.f1;Cyc_Absyndump_dumprgn(_T5);}
# 546
static void Cyc_Absyndump_dump_rgnpo(struct Cyc_List_List*rgn_po){void(*_T0)(void(*)(struct _tuple0*),struct Cyc_List_List*,struct _fat_ptr);struct Cyc_List_List*_T1;struct _fat_ptr _T2;{
void(*_T3)(void(*)(struct _tuple0*),struct Cyc_List_List*,struct _fat_ptr)=(void(*)(void(*)(struct _tuple0*),struct Cyc_List_List*,struct _fat_ptr))Cyc_Absyndump_dump_sep;_T0=_T3;}_T1=rgn_po;_T2=_tag_fat(",",sizeof(char),2U);_T0(Cyc_Absyndump_dump_rgncmp,_T1,_T2);}
# 549
static void Cyc_Absyndump_dump_oneeffc(void*fc){void*_T0;int*_T1;unsigned _T2;void*_T3;void*_T4;void*_T5;struct _fat_ptr _T6;void*_T7;void*_T8;void*_T9;void*_TA;void*_TB;struct _fat_ptr _TC;void*_TD;void*_TE;_T0=fc;_T1=(int*)_T0;_T2=*_T1;switch(_T2){case 2: _T3=fc;{struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*_TF=(struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*)_T3;_T4=_TF->f1;_TE=(void*)_T4;_T5=_TF->f2;_TD=(void*)_T5;}{void*e1=_TE;void*e2=_TD;
# 552
Cyc_Absyndump_dumpeff(e1);_T6=_tag_fat("<=",sizeof(char),3U);Cyc_Absyndump_dump(_T6);Cyc_Absyndump_dumpeff(e2);goto _LL0;}case 1: _T7=fc;{struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct*_TF=(struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct*)_T7;_T8=_TF->f1;_TE=(void*)_T8;_T9=_TF->f2;_TD=(void*)_T9;}{void*e1=_TE;void*e2=_TD;
# 555
Cyc_Absyndump_dumpeff(e1);Cyc_Absyndump_dump_char(124);Cyc_Absyndump_dumpeff(e2);goto _LL0;}default: _TA=fc;{struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct*_TF=(struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct*)_TA;_TB=_TF->f1;_TE=(void*)_TB;}{void*e=_TE;_TC=
# 558
_tag_fat("single(",sizeof(char),8U);Cyc_Absyndump_dump(_TC);Cyc_Absyndump_dumpeff(e);Cyc_Absyndump_dump_char(41);goto _LL0;}}_LL0:;}
# 562
static void Cyc_Absyndump_dump_effc(struct Cyc_List_List*effc){struct Cyc_List_List*_T0;struct _fat_ptr _T1;_T0=effc;_T1=
_tag_fat(",",sizeof(char),2U);Cyc_Absyndump_dump_sep(Cyc_Absyndump_dump_oneeffc,_T0,_T1);}
# 565
static void Cyc_Absyndump_dump_qb_elt(struct _tuple0*q){struct _tuple0*_T0;struct _tuple0 _T1;void*_T2;struct _fat_ptr _T3;struct _tuple0*_T4;struct _tuple0 _T5;void*_T6;_T0=q;_T1=*_T0;_T2=_T1.f1;
Cyc_Absyndump_dumptyp(_T2);_T3=_tag_fat(">=",sizeof(char),3U);Cyc_Absyndump_dump(_T3);_T4=q;_T5=*_T4;_T6=_T5.f0;Cyc_Absyndump_dumptyp(_T6);}
# 568
static void Cyc_Absyndump_dump_qualbnd(struct Cyc_List_List*qual_bnd){void(*_T0)(void(*)(struct _tuple0*),struct Cyc_List_List*,struct _fat_ptr);struct Cyc_List_List*_T1;struct _fat_ptr _T2;{
void(*_T3)(void(*)(struct _tuple0*),struct Cyc_List_List*,struct _fat_ptr)=(void(*)(void(*)(struct _tuple0*),struct Cyc_List_List*,struct _fat_ptr))Cyc_Absyndump_dump_sep;_T0=_T3;}_T1=qual_bnd;_T2=_tag_fat(",",sizeof(char),2U);_T0(Cyc_Absyndump_dump_qb_elt,_T1,_T2);}
# 571
static void Cyc_Absyndump_dumpfunargs(struct Cyc_List_List*args,int c_varargs,struct Cyc_Absyn_VarargInfo*cyc_varargs,void*effopt,struct Cyc_List_List*effc,struct Cyc_List_List*qual_bnd,struct Cyc_Absyn_Exp*chk,struct Cyc_Absyn_Exp*req,struct Cyc_Absyn_Exp*ens,struct Cyc_Absyn_Exp*thrws){int _T0;struct _fat_ptr _T1;void(*_T2)(void(*)(struct _tuple9*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr);struct Cyc_List_List*_T3;struct _fat_ptr _T4;struct _fat_ptr _T5;struct _fat_ptr _T6;int _T7;struct _fat_ptr _T8;struct Cyc_Absyn_VarargInfo*_T9;int _TA;struct _fat_ptr _TB;struct _tuple9*_TC;struct Cyc_Absyn_VarargInfo*_TD;struct Cyc_Absyn_VarargInfo*_TE;struct Cyc_Absyn_VarargInfo*_TF;char _T10;char _T11;int _T12;struct _fat_ptr _T13;struct _fat_ptr _T14;struct _fat_ptr _T15;struct _fat_ptr _T16;
# 576
if(args!=0)goto _TL96;_T0=c_varargs;if(_T0)goto _TL96;else{goto _TL98;}_TL98: if(cyc_varargs!=0)goto _TL96;if(effopt!=0)goto _TL96;if(effc!=0)goto _TL96;_T1=
# 578
_tag_fat("(void)",sizeof(char),7U);Cyc_Absyndump_dump(_T1);goto _TL97;
# 581
_TL96:{void(*_T17)(void(*)(struct _tuple9*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr)=(void(*)(void(*)(struct _tuple9*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr))Cyc_Absyndump_group;_T2=_T17;}_T3=args;_T4=_tag_fat("(",sizeof(char),2U);_T5=_tag_fat("",sizeof(char),1U);_T6=_tag_fat(",",sizeof(char),2U);_T2(Cyc_Absyndump_dumpfunarg,_T3,_T4,_T5,_T6);_T7=c_varargs;
if(_T7)goto _TL9B;else{goto _TL9C;}_TL9C: if(cyc_varargs!=0)goto _TL9B;else{goto _TL99;}
_TL9B: _T8=_tag_fat(",...",sizeof(char),5U);Cyc_Absyndump_dump(_T8);goto _TL9A;_TL99: _TL9A:
 if(cyc_varargs==0)goto _TL9D;_T9=cyc_varargs;_TA=_T9->inject;
if(!_TA)goto _TL9F;_TB=_tag_fat(" inject ",sizeof(char),9U);Cyc_Absyndump_dump(_TB);goto _TLA0;_TL9F: _TLA0:{struct _tuple9*_T17=_cycalloc(sizeof(struct _tuple9));_TD=cyc_varargs;
_T17->f0=_TD->name;_TE=cyc_varargs;_T17->f1=_TE->tq;_TF=cyc_varargs;_T17->f2=_TF->type;_TC=(struct _tuple9*)_T17;}Cyc_Absyndump_dumpfunarg(_TC);goto _TL9E;_TL9D: _TL9E:
# 588
 if(effopt==0)goto _TLA1;
Cyc_Absyndump_dump_semi();Cyc_Absyndump_dumpeff(effopt);goto _TLA2;_TLA1: _TLA2:
# 591
 if(effc==0)goto _TLA3;
Cyc_Absyndump_dump_char(58);Cyc_Absyndump_dump_effc(effc);goto _TLA4;_TLA3: _TLA4:
# 594
 if(qual_bnd==0)goto _TLA5;
if(effc!=0)goto _TLA7;_T10=':';goto _TLA8;_TLA7: _T10=',';_TLA8:{char sep=_T10;_T11=sep;_T12=(int)_T11;
Cyc_Absyndump_dump_char(_T12);Cyc_Absyndump_dump_qualbnd(qual_bnd);}goto _TLA6;_TLA5: _TLA6:
# 598
 Cyc_Absyndump_dump_char(41);_TL97:
# 600
 if(chk==0)goto _TLA9;_T13=
_tag_fat(" @checks(",sizeof(char),10U);Cyc_Absyndump_dump(_T13);Cyc_Absyndump_dumpexp(chk);Cyc_Absyndump_dump_char(41);goto _TLAA;_TLA9: _TLAA:
# 603
 if(req==0)goto _TLAB;_T14=
_tag_fat(" @requires(",sizeof(char),12U);Cyc_Absyndump_dump(_T14);Cyc_Absyndump_dumpexp(req);Cyc_Absyndump_dump_char(41);goto _TLAC;_TLAB: _TLAC:
# 606
 if(ens==0)goto _TLAD;_T15=
_tag_fat(" @ensures(",sizeof(char),11U);Cyc_Absyndump_dump(_T15);Cyc_Absyndump_dumpexp(ens);Cyc_Absyndump_dump_char(41);goto _TLAE;_TLAD: _TLAE:
# 609
 if(thrws==0)goto _TLAF;_T16=
_tag_fat(" @throws(",sizeof(char),10U);Cyc_Absyndump_dump(_T16);Cyc_Absyndump_dumpexp(thrws);Cyc_Absyndump_dump_char(41);goto _TLB0;_TLAF: _TLB0:;}
# 614
static void Cyc_Absyndump_dumptyp(void*t){void(*_T0)(struct Cyc_Absyn_Tqual,void*,void(*)(int),int);void(*_T1)(struct Cyc_Absyn_Tqual,void*,void(*)(void*),void*);struct Cyc_Absyn_Tqual _T2;void*_T3;void(*_T4)(int);_T1=Cyc_Absyndump_dumptqtd;{
void(*_T5)(struct Cyc_Absyn_Tqual,void*,void(*)(int),int)=(void(*)(struct Cyc_Absyn_Tqual,void*,void(*)(int),int))_T1;_T0=_T5;}_T2=Cyc_Absyn_empty_tqual(0U);_T3=t;{void(*_T5)(int)=(void(*)(int))Cyc_Absyndump_ignore;_T4=_T5;}_T0(_T2,_T3,_T4,0);}
# 618
static void Cyc_Absyndump_dumpdesignator(void*d){void*_T0;int*_T1;int _T2;void*_T3;struct _fat_ptr _T4;void*_T5;struct _fat_ptr*_T6;struct _fat_ptr _T7;struct _fat_ptr*_T8;struct Cyc_Absyn_Exp*_T9;_T0=d;_T1=(int*)_T0;_T2=*_T1;if(_T2!=0)goto _TLB1;_T3=d;{struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct*_TA=(struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct*)_T3;_T9=_TA->f1;}{struct Cyc_Absyn_Exp*e=_T9;_T4=
# 620
_tag_fat(".[",sizeof(char),3U);Cyc_Absyndump_dump(_T4);Cyc_Absyndump_dumpexp(e);Cyc_Absyndump_dump_char(93);goto _LL0;}_TLB1: _T5=d;{struct Cyc_Absyn_FieldName_Absyn_Designator_struct*_TA=(struct Cyc_Absyn_FieldName_Absyn_Designator_struct*)_T5;_T8=_TA->f1;}{struct _fat_ptr*v=_T8;
Cyc_Absyndump_dump_char(46);_T6=v;_T7=*_T6;Cyc_Absyndump_dump_nospace(_T7);goto _LL0;}_LL0:;}struct _tuple18{struct Cyc_List_List*f0;struct Cyc_Absyn_Exp*f1;};
# 624
static void Cyc_Absyndump_dumpde(struct _tuple18*de){struct _tuple18*_T0;struct _tuple18 _T1;struct Cyc_List_List*_T2;struct _fat_ptr _T3;struct _fat_ptr _T4;struct _fat_ptr _T5;struct _tuple18*_T6;struct _tuple18 _T7;struct Cyc_Absyn_Exp*_T8;_T0=de;_T1=*_T0;_T2=_T1.f0;_T3=
_tag_fat("",sizeof(char),1U);_T4=_tag_fat("=",sizeof(char),2U);_T5=_tag_fat("=",sizeof(char),2U);Cyc_Absyndump_egroup(Cyc_Absyndump_dumpdesignator,_T2,_T3,_T4,_T5);_T6=de;_T7=*_T6;_T8=_T7.f1;
Cyc_Absyndump_dumpexp(_T8);}
# 629
static void Cyc_Absyndump_dumpoffset_field(void*f){void*_T0;struct _fat_ptr*_T1;struct _fat_ptr _T2;struct _fat_ptr*_T3;_T0=f;{struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct*_T4=(struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct*)_T0;_T3=_T4->f1;}{struct _fat_ptr*n=_T3;_T1=n;_T2=*_T1;
# 631
Cyc_Absyndump_dump_nospace(_T2);return;};}struct _tuple19{struct _fat_ptr f0;struct Cyc_Absyn_Exp*f1;};
# 635
static void Cyc_Absyndump_dump_asm_iolist(struct Cyc_List_List*iolist){struct Cyc_List_List*_T0;unsigned _T1;struct Cyc_List_List*_T2;void*_T3;struct _fat_ptr _T4;struct Cyc_List_List*_T5;struct Cyc_List_List*_T6;unsigned _T7;
_TLB3: _T0=iolist;_T1=(unsigned)_T0;if(_T1)goto _TLB4;else{goto _TLB5;}
_TLB4: _T2=iolist;_T3=_T2->hd;{struct _tuple19*_T8=(struct _tuple19*)_T3;struct Cyc_Absyn_Exp*_T9;struct _fat_ptr _TA;{struct _tuple19 _TB=*_T8;_TA=_TB.f0;_T9=_TB.f1;}{struct _fat_ptr constr=_TA;struct Cyc_Absyn_Exp*e1=_T9;
Cyc_Absyndump_dump_char(34);_T4=Cyc_Absynpp_string_escape(constr);Cyc_Absyndump_dump_nospace(_T4);Cyc_Absyndump_dump_char(34);
Cyc_Absyndump_dump_char(40);Cyc_Absyndump_dumpexp(e1);Cyc_Absyndump_dump_char(41);_T5=iolist;
iolist=_T5->tl;_T6=iolist;_T7=(unsigned)_T6;
if(!_T7)goto _TLB6;
Cyc_Absyndump_dump_char(44);goto _TLB7;_TLB6: _TLB7:;}}goto _TLB3;_TLB5:;}
# 646
static void Cyc_Absyndump_dumpexps_prec(int inprec,struct Cyc_List_List*es){void(*_T0)(void(*)(int,struct Cyc_Absyn_Exp*),int,struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr);void(*_T1)(int,struct Cyc_Absyn_Exp*);int _T2;struct Cyc_List_List*_T3;struct _fat_ptr _T4;struct _fat_ptr _T5;struct _fat_ptr _T6;{
void(*_T7)(void(*)(int,struct Cyc_Absyn_Exp*),int,struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr)=(void(*)(void(*)(int,struct Cyc_Absyn_Exp*),int,struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr))Cyc_Absyndump_group_c;_T0=_T7;}_T1=Cyc_Absyndump_dumpexp_prec;_T2=inprec;_T3=es;_T4=_tag_fat("",sizeof(char),1U);_T5=_tag_fat("",sizeof(char),1U);_T6=_tag_fat(",",sizeof(char),2U);_T0(_T1,_T2,_T3,_T4,_T5,_T6);}
# 650
static void Cyc_Absyndump_dumpexp_prec(int inprec,struct Cyc_Absyn_Exp*e){struct _fat_ptr _T0;struct Cyc_Absyn_Exp*_T1;int*_T2;unsigned _T3;struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*_T4;union Cyc_Absyn_Cnst _T5;struct _union_Cnst_String_c _T6;unsigned _T7;union Cyc_Absyn_Cnst _T8;struct _union_Cnst_Char_c _T9;struct _tuple4 _TA;union Cyc_Absyn_Cnst _TB;struct _union_Cnst_Char_c _TC;struct _tuple4 _TD;struct _fat_ptr _TE;union Cyc_Absyn_Cnst _TF;struct _union_Cnst_Wchar_c _T10;struct _fat_ptr _T11;struct Cyc_String_pa_PrintArg_struct _T12;void**_T13;struct _fat_ptr _T14;struct _fat_ptr _T15;union Cyc_Absyn_Cnst _T16;struct _union_Cnst_Short_c _T17;struct _tuple5 _T18;union Cyc_Absyn_Cnst _T19;struct _union_Cnst_Short_c _T1A;struct _tuple5 _T1B;struct _fat_ptr _T1C;struct Cyc_Int_pa_PrintArg_struct _T1D;short _T1E;int _T1F;void**_T20;struct _fat_ptr _T21;struct _fat_ptr _T22;struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*_T23;union Cyc_Absyn_Cnst _T24;struct _union_Cnst_Int_c _T25;struct _tuple6 _T26;enum Cyc_Absyn_Sign _T27;int _T28;union Cyc_Absyn_Cnst _T29;struct _union_Cnst_Int_c _T2A;struct _tuple6 _T2B;union Cyc_Absyn_Cnst _T2C;struct _union_Cnst_Int_c _T2D;struct _tuple6 _T2E;struct _fat_ptr _T2F;struct Cyc_Int_pa_PrintArg_struct _T30;int _T31;void**_T32;struct _fat_ptr _T33;struct _fat_ptr _T34;union Cyc_Absyn_Cnst _T35;struct _union_Cnst_Int_c _T36;struct _tuple6 _T37;struct _fat_ptr _T38;struct Cyc_Int_pa_PrintArg_struct _T39;int _T3A;void**_T3B;struct _fat_ptr _T3C;struct _fat_ptr _T3D;union Cyc_Absyn_Cnst _T3E;struct _union_Cnst_LongLong_c _T3F;struct _tuple7 _T40;union Cyc_Absyn_Cnst _T41;struct _union_Cnst_LongLong_c _T42;struct _tuple7 _T43;long long _T44;unsigned long long _T45;struct _fat_ptr _T46;union Cyc_Absyn_Cnst _T47;struct _union_Cnst_Float_c _T48;struct _tuple8 _T49;struct _fat_ptr _T4A;union Cyc_Absyn_Cnst _T4B;struct _union_Cnst_String_c _T4C;struct _fat_ptr _T4D;union Cyc_Absyn_Cnst _T4E;struct _union_Cnst_Wstring_c _T4F;struct _fat_ptr _T50;void*_T51;struct _tuple1*_T52;struct _fat_ptr _T53;int _T54;enum Cyc_Absyn_Primop _T55;int _T56;struct _fat_ptr _T57;struct Cyc_List_List*_T58;void*_T59;struct Cyc_Absyn_Exp*_T5A;struct _fat_ptr _T5B;enum Cyc_Absyn_Primop _T5C;int _T5D;struct _fat_ptr _T5E;struct Cyc_List_List*_T5F;void*_T60;struct Cyc_Absyn_Exp*_T61;struct _fat_ptr _T62;int _T63;struct Cyc_List_List*_T64;void*_T65;struct Cyc_Absyn_Exp*_T66;int _T67;struct Cyc_List_List*_T68;void*_T69;struct Cyc_Absyn_Exp*_T6A;enum Cyc_Absyn_Primop _T6B;int _T6C;int _T6D;struct Cyc_List_List*_T6E;struct Cyc_List_List*_T6F;struct Cyc_List_List*_T70;void*_T71;struct Cyc_Absyn_Exp*_T72;int(*_T73)(struct _fat_ptr,struct _fat_ptr);void*(*_T74)(struct _fat_ptr,struct _fat_ptr);struct _fat_ptr _T75;struct _fat_ptr _T76;struct Cyc_Core_Opt*_T77;void*_T78;enum Cyc_Absyn_Primop _T79;struct _fat_ptr _T7A;struct _fat_ptr _T7B;struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*_T7C;enum Cyc_Absyn_Incrementor _T7D;int _T7E;struct _fat_ptr _T7F;struct _fat_ptr _T80;struct _fat_ptr _T81;struct _fat_ptr _T82;struct Cyc_Absyn_Exp*_T83;unsigned _T84;struct Cyc_Absyn_Exp*_T85;unsigned _T86;struct _fat_ptr _T87;struct Cyc_Absyn_Exp*_T88;unsigned _T89;struct _fat_ptr _T8A;struct Cyc_Absyn_Exp*_T8B;unsigned _T8C;int _T8D;struct Cyc_Absyn_Exp*_T8E;int _T8F;struct Cyc_Absyn_Exp*_T90;struct Cyc_Absyn_Exp*_T91;unsigned _T92;struct _fat_ptr _T93;int _T94;int _T95;struct Cyc_Absyn_Exp*_T96;unsigned _T97;struct Cyc_Absyn_Exp*_T98;unsigned _T99;int _T9A;struct _fat_ptr _T9B;struct Cyc_Absyn_Exp*_T9C;unsigned _T9D;struct _fat_ptr _T9E;void*_T9F;struct Cyc_Absyn_Exp*_TA0;unsigned _TA1;struct _fat_ptr _TA2;struct Cyc_Absyn_Exp*_TA3;unsigned _TA4;struct _fat_ptr _TA5;struct Cyc_Absyn_Exp*_TA6;unsigned _TA7;void*_TA8;struct _fat_ptr _TA9;struct _fat_ptr _TAA;void*_TAB;struct _fat_ptr _TAC;struct _fat_ptr _TAD;int _TAE;struct _fat_ptr _TAF;struct _fat_ptr _TB0;struct _fat_ptr _TB1;struct _fat_ptr _TB2;int _TB3;struct _fat_ptr _TB4;struct _fat_ptr _TB5;struct _fat_ptr _TB6;struct Cyc_List_List*_TB7;unsigned _TB8;struct Cyc_List_List*_TB9;unsigned _TBA;struct Cyc_List_List*_TBB;unsigned _TBC;struct Cyc_List_List*_TBD;unsigned _TBE;int _TBF;struct Cyc_List_List*_TC0;unsigned _TC1;int _TC2;struct Cyc_List_List*_TC3;unsigned _TC4;int _TC5;int _TC6;struct _fat_ptr _TC7;struct Cyc_List_List*_TC8;unsigned _TC9;struct Cyc_List_List*_TCA;void*_TCB;struct _fat_ptr*_TCC;struct _fat_ptr _TCD;struct _fat_ptr _TCE;struct Cyc_List_List*_TCF;struct Cyc_List_List*_TD0;unsigned _TD1;struct _fat_ptr _TD2;struct _fat_ptr*_TD3;struct _fat_ptr _TD4;void*_TD5;struct _fat_ptr _TD6;struct Cyc_List_List*_TD7;struct _fat_ptr _TD8;struct _fat_ptr _TD9;struct _fat_ptr _TDA;struct _fat_ptr*_TDB;struct _fat_ptr _TDC;struct _fat_ptr _TDD;struct _fat_ptr*_TDE;struct _fat_ptr _TDF;struct _tuple9*_TE0;struct _tuple9 _TE1;void*_TE2;void(*_TE3)(void(*)(struct _tuple18*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr);struct Cyc_List_List*_TE4;struct _fat_ptr _TE5;struct _fat_ptr _TE6;struct _fat_ptr _TE7;void(*_TE8)(void(*)(struct _tuple18*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr);struct Cyc_List_List*_TE9;struct _fat_ptr _TEA;struct _fat_ptr _TEB;struct _fat_ptr _TEC;struct _fat_ptr _TED;struct Cyc_Absyn_Vardecl*_TEE;struct _tuple1*_TEF;struct _tuple1 _TF0;struct _fat_ptr*_TF1;void*_TF2;struct _fat_ptr _TF3;void(*_TF4)(void(*)(struct _tuple18*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr);struct Cyc_List_List*_TF5;struct _fat_ptr _TF6;struct _fat_ptr _TF7;struct _fat_ptr _TF8;int _TF9;struct _fat_ptr _TFA;struct Cyc_List_List*(*_TFB)(struct Cyc_Absyn_Exp*(*)(struct _tuple18*),struct Cyc_List_List*);struct Cyc_List_List*(*_TFC)(void*(*)(void*),struct Cyc_List_List*);struct Cyc_Absyn_Exp*(*_TFD)(struct _tuple18*);void*(*_TFE)(struct _tuple0*);struct Cyc_List_List*_TFF;struct Cyc_List_List*_T100;void(*_T101)(void(*)(struct _tuple18*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr);struct Cyc_List_List*_T102;struct _fat_ptr _T103;struct _fat_ptr _T104;struct _fat_ptr _T105;struct Cyc_Absyn_Enumfield*_T106;struct _tuple1*_T107;struct Cyc_Absyn_Datatypefield*_T108;struct _tuple1*_T109;void(*_T10A)(void(*)(struct Cyc_Absyn_Exp*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr);void(*_T10B)(struct Cyc_Absyn_Exp*);struct Cyc_List_List*_T10C;struct _fat_ptr _T10D;struct _fat_ptr _T10E;struct _fat_ptr _T10F;struct Cyc_Absyn_MallocInfo _T110;struct Cyc_Absyn_MallocInfo _T111;struct Cyc_Absyn_MallocInfo _T112;struct Cyc_Absyn_MallocInfo _T113;struct Cyc_Absyn_MallocInfo _T114;struct Cyc_Absyn_MallocInfo _T115;struct Cyc_Absyn_Exp*_T116;unsigned _T117;enum Cyc_Absyn_MallocKind _T118;int _T119;struct _fat_ptr _T11A;struct _fat_ptr _T11B;struct _fat_ptr _T11C;struct _fat_ptr _T11D;struct _fat_ptr _T11E;struct _fat_ptr _T11F;struct _fat_ptr _T120;void**_T121;void*_T122;struct Cyc_Absyn_Exp*_T123;struct _fat_ptr _T124;struct _fat_ptr _T125;enum Cyc_Absyn_MallocKind _T126;int _T127;struct _fat_ptr _T128;int _T129;struct _fat_ptr _T12A;struct _fat_ptr _T12B;struct _fat_ptr _T12C;struct _fat_ptr _T12D;struct _fat_ptr _T12E;void**_T12F;void*_T130;struct Cyc_Absyn_Exp*_T131;struct Cyc_Absyn_Exp*_T132;struct Cyc_Absyn_Exp*_T133;struct _fat_ptr _T134;struct _fat_ptr _T135;void(*_T136)(void(*)(struct _tuple18*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr);struct Cyc_List_List*_T137;struct _fat_ptr _T138;struct _fat_ptr _T139;struct _fat_ptr _T13A;struct _fat_ptr _T13B;struct Cyc_Absyn_Stmt*_T13C;struct Cyc_Dict_Dict _T13D;struct _fat_ptr _T13E;
int myprec=Cyc_Absynpp_exp_prec(e);
if(inprec < myprec)goto _TLB8;if(inprec!=140)goto _TLBA;else{goto _TLBB;}_TLBB: if(myprec!=140)goto _TLBA;else{goto _TLB8;}
# 654
_TLBA: _T0=_tag_fat("(",sizeof(char),2U);Cyc_Absyndump_dump_nospace(_T0);goto _TLB9;_TLB8: _TLB9: _T1=e;{
void*_T13F=_T1->r;struct Cyc_Absyn_Stmt*_T140;struct Cyc_Absyn_Exp*_T141;void**_T142;enum Cyc_Absyn_MallocKind _T143;struct Cyc_Absyn_Datatypefield*_T144;struct Cyc_Absyn_Enumfield*_T145;struct _tuple1*_T146;struct _fat_ptr*_T147;struct Cyc_List_List*_T148;struct Cyc_List_List*_T149;struct Cyc_Absyn_Exp*_T14A;struct Cyc_Absyn_Exp*_T14B;struct Cyc_Core_Opt*_T14C;enum Cyc_Absyn_Primop _T14D;void*_T14E;long long _T14F;int _T150;short _T151;struct _fat_ptr _T152;char _T153;enum Cyc_Absyn_Sign _T154;_T2=(int*)_T13F;_T3=*_T2;switch(_T3){case 0: _T4=(struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_T13F;_T5=_T4->f1;_T6=_T5.String_c;_T7=_T6.tag;switch(_T7){case 2:{struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_T13F;_T8=_T155->f1;_T9=_T8.Char_c;_TA=_T9.val;_T154=_TA.f0;_TB=_T155->f1;_TC=_TB.Char_c;_TD=_TC.val;_T153=_TD.f1;}{enum Cyc_Absyn_Sign sg=_T154;char ch=_T153;
# 657
Cyc_Absyndump_dump_char(39);_TE=Cyc_Absynpp_char_escape(ch);Cyc_Absyndump_dump_nospace(_TE);Cyc_Absyndump_dump_char(39);goto _LL0;}case 3:{struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_T13F;_TF=_T155->f1;_T10=_TF.Wchar_c;_T152=_T10.val;}{struct _fat_ptr s=_T152;{struct Cyc_String_pa_PrintArg_struct _T155;_T155.tag=0;
# 659
_T155.f1=s;_T12=_T155;}{struct Cyc_String_pa_PrintArg_struct _T155=_T12;void*_T156[1];_T13=_T156 + 0;*_T13=& _T155;_T14=_tag_fat("L'%s'",sizeof(char),6U);_T15=_tag_fat(_T156,sizeof(void*),1);_T11=Cyc_aprintf(_T14,_T15);}Cyc_Absyndump_dump(_T11);goto _LL0;}case 4:{struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_T13F;_T16=_T155->f1;_T17=_T16.Short_c;_T18=_T17.val;_T154=_T18.f0;_T19=_T155->f1;_T1A=_T19.Short_c;_T1B=_T1A.val;_T151=_T1B.f1;}{enum Cyc_Absyn_Sign sg=_T154;short s=_T151;{struct Cyc_Int_pa_PrintArg_struct _T155;_T155.tag=1;_T1E=s;_T1F=(int)_T1E;
_T155.f1=(unsigned long)_T1F;_T1D=_T155;}{struct Cyc_Int_pa_PrintArg_struct _T155=_T1D;void*_T156[1];_T20=_T156 + 0;*_T20=& _T155;_T21=_tag_fat("%d",sizeof(char),3U);_T22=_tag_fat(_T156,sizeof(void*),1);_T1C=Cyc_aprintf(_T21,_T22);}Cyc_Absyndump_dump(_T1C);goto _LL0;}case 5: _T23=(struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_T13F;_T24=_T23->f1;_T25=_T24.Int_c;_T26=_T25.val;_T27=_T26.f0;_T28=(int)_T27;switch(_T28){case Cyc_Absyn_None:{struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_T13F;_T29=_T155->f1;_T2A=_T29.Int_c;_T2B=_T2A.val;_T150=_T2B.f1;}{int i=_T150;_T150=i;goto _LLA;}case Cyc_Absyn_Signed:{struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_T13F;_T2C=_T155->f1;_T2D=_T2C.Int_c;_T2E=_T2D.val;_T150=_T2E.f1;}_LLA: {int i=_T150;{struct Cyc_Int_pa_PrintArg_struct _T155;_T155.tag=1;_T31=i;
# 662
_T155.f1=(unsigned long)_T31;_T30=_T155;}{struct Cyc_Int_pa_PrintArg_struct _T155=_T30;void*_T156[1];_T32=_T156 + 0;*_T32=& _T155;_T33=_tag_fat("%d",sizeof(char),3U);_T34=_tag_fat(_T156,sizeof(void*),1);_T2F=Cyc_aprintf(_T33,_T34);}Cyc_Absyndump_dump(_T2F);goto _LL0;}default:{struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_T13F;_T35=_T155->f1;_T36=_T35.Int_c;_T37=_T36.val;_T150=_T37.f1;}{int i=_T150;{struct Cyc_Int_pa_PrintArg_struct _T155;_T155.tag=1;_T3A=i;
_T155.f1=(unsigned)_T3A;_T39=_T155;}{struct Cyc_Int_pa_PrintArg_struct _T155=_T39;void*_T156[1];_T3B=_T156 + 0;*_T3B=& _T155;_T3C=_tag_fat("%uU",sizeof(char),4U);_T3D=_tag_fat(_T156,sizeof(void*),1);_T38=Cyc_aprintf(_T3C,_T3D);}Cyc_Absyndump_dump(_T38);goto _LL0;}};case 6:{struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_T13F;_T3E=_T155->f1;_T3F=_T3E.LongLong_c;_T40=_T3F.val;_T154=_T40.f0;_T41=_T155->f1;_T42=_T41.LongLong_c;_T43=_T42.val;_T14F=_T43.f1;}{enum Cyc_Absyn_Sign sg=_T154;long long i=_T14F;_T44=i;_T45=(unsigned long long)_T44;_T46=
# 666
Cyc_Absynpp_longlong2string(_T45);Cyc_Absyndump_dump(_T46);goto _LL0;}case 7:{struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_T13F;_T47=_T155->f1;_T48=_T47.Float_c;_T49=_T48.val;_T152=_T49.f0;}{struct _fat_ptr x=_T152;
Cyc_Absyndump_dump(x);goto _LL0;}case 1: _T4A=
_tag_fat("NULL",sizeof(char),5U);Cyc_Absyndump_dump(_T4A);goto _LL0;case 8:{struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_T13F;_T4B=_T155->f1;_T4C=_T4B.String_c;_T152=_T4C.val;}{struct _fat_ptr s=_T152;
# 670
Cyc_Absyndump_dump_char(34);_T4D=Cyc_Absynpp_string_escape(s);Cyc_Absyndump_dump_nospace(_T4D);Cyc_Absyndump_dump_char(34);goto _LL0;}default:{struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_T13F;_T4E=_T155->f1;_T4F=_T4E.Wstring_c;_T152=_T4F.val;}{struct _fat_ptr s=_T152;_T50=
# 673
_tag_fat("L\"",sizeof(char),3U);Cyc_Absyndump_dump(_T50);Cyc_Absyndump_dump_nospace(s);Cyc_Absyndump_dump_char(34);goto _LL0;}};case 1:{struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_T13F;_T51=_T155->f1;_T14E=(void*)_T51;}{void*b=_T14E;_T52=
# 675
Cyc_Absyn_binding2qvar(b);Cyc_Absyndump_dumpqvar(_T52);goto _LL0;}case 2:{struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct*)_T13F;_T152=_T155->f1;}{struct _fat_ptr p=_T152;_T53=
# 678
_tag_fat("__cyclone_pragma__(",sizeof(char),20U);Cyc_Absyndump_dump(_T53);Cyc_Absyndump_dump_nospace(p);Cyc_Absyndump_dump_char(41);goto _LL0;}case 3:{struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_T13F;_T14D=_T155->f1;_T14E=_T155->f2;}{enum Cyc_Absyn_Primop p=_T14D;struct Cyc_List_List*es=_T14E;
# 681
struct _fat_ptr pstr=Cyc_Absynpp_prim2str(p);{
int _T155=Cyc_List_length(es);_T54=(int)_T155;switch(_T54){case 1: _T55=p;_T56=(int)_T55;
# 684
if(_T56!=18)goto _TLC0;_T57=
_tag_fat("numelts(",sizeof(char),9U);Cyc_Absyndump_dump(_T57);_T58=
_check_null(es);_T59=_T58->hd;_T5A=(struct Cyc_Absyn_Exp*)_T59;Cyc_Absyndump_dumpexp(_T5A);_T5B=
_tag_fat(")",sizeof(char),2U);Cyc_Absyndump_dump(_T5B);goto _TLC1;
_TLC0: _T5C=p;_T5D=(int)_T5C;if(_T5D!=19)goto _TLC2;_T5E=
_tag_fat("tagof(",sizeof(char),7U);Cyc_Absyndump_dump(_T5E);_T5F=
_check_null(es);_T60=_T5F->hd;_T61=(struct Cyc_Absyn_Exp*)_T60;Cyc_Absyndump_dumpexp(_T61);_T62=
_tag_fat(")",sizeof(char),2U);Cyc_Absyndump_dump(_T62);goto _TLC3;
# 693
_TLC2: Cyc_Absyndump_dump(pstr);_T63=myprec;_T64=
_check_null(es);_T65=_T64->hd;_T66=(struct Cyc_Absyn_Exp*)_T65;Cyc_Absyndump_dumpexp_prec(_T63,_T66);_TLC3: _TLC1: goto _LL71;case 2: _T67=myprec;_T68=
# 698
_check_null(es);_T69=_T68->hd;_T6A=(struct Cyc_Absyn_Exp*)_T69;Cyc_Absyndump_dumpexp_prec(_T67,_T6A);_T6B=p;_T6C=(int)_T6B;switch(_T6C){case Cyc_Absyn_Eq: goto _LL7C;case Cyc_Absyn_Neq: _LL7C:
# 701
 Cyc_Absyndump_dump_nospace(pstr);goto _LL78;default:
# 703
 Cyc_Absyndump_dump(pstr);
Cyc_Absyndump_dump_char(32);}_LL78: _T6D=myprec;_T6E=es;_T6F=_T6E->tl;_T70=
# 706
_check_null(_T6F);_T71=_T70->hd;_T72=(struct Cyc_Absyn_Exp*)_T71;Cyc_Absyndump_dumpexp_prec(_T6D,_T72);goto _LL71;default: _T74=Cyc_Warn_impos;{
# 708
int(*_T156)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))_T74;_T73=_T156;}_T75=_tag_fat("Absyndump -- Bad number of arguments to primop",sizeof(char),47U);_T76=_tag_fat(0U,sizeof(void*),0);_T73(_T75,_T76);}_LL71:;}goto _LL0;}case 4:{struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_T13F;_T14E=_T155->f1;_T14C=_T155->f2;_T14B=_T155->f3;}{struct Cyc_Absyn_Exp*e1=_T14E;struct Cyc_Core_Opt*popt=_T14C;struct Cyc_Absyn_Exp*e2=_T14B;
# 713
Cyc_Absyndump_dumpexp_prec(myprec,e1);
if(popt==0)goto _TLC5;_T77=popt;_T78=_T77->v;_T79=(enum Cyc_Absyn_Primop)_T78;_T7A=
Cyc_Absynpp_prim2str(_T79);Cyc_Absyndump_dump(_T7A);goto _TLC6;_TLC5: _TLC6: _T7B=
_tag_fat("=",sizeof(char),2U);Cyc_Absyndump_dump_nospace(_T7B);
Cyc_Absyndump_dumpexp_prec(myprec,e2);goto _LL0;}case 5: _T7C=(struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_T13F;_T7D=_T7C->f2;_T7E=(int)_T7D;switch(_T7E){case Cyc_Absyn_PreInc:{struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_T13F;_T14E=_T155->f1;}{struct Cyc_Absyn_Exp*e2=_T14E;_T7F=
# 720
_tag_fat("++",sizeof(char),3U);Cyc_Absyndump_dump(_T7F);Cyc_Absyndump_dumpexp_prec(myprec,e2);goto _LL0;}case Cyc_Absyn_PreDec:{struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_T13F;_T14E=_T155->f1;}{struct Cyc_Absyn_Exp*e2=_T14E;_T80=
_tag_fat("--",sizeof(char),3U);Cyc_Absyndump_dump(_T80);Cyc_Absyndump_dumpexp_prec(myprec,e2);goto _LL0;}case Cyc_Absyn_PostInc:{struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_T13F;_T14E=_T155->f1;}{struct Cyc_Absyn_Exp*e2=_T14E;
Cyc_Absyndump_dumpexp_prec(myprec,e2);_T81=_tag_fat("++",sizeof(char),3U);Cyc_Absyndump_dump(_T81);goto _LL0;}default:{struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_T13F;_T14E=_T155->f1;}{struct Cyc_Absyn_Exp*e2=_T14E;
Cyc_Absyndump_dumpexp_prec(myprec,e2);_T82=_tag_fat("--",sizeof(char),3U);Cyc_Absyndump_dump(_T82);goto _LL0;}};case 6:{struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_T13F;_T14E=_T155->f1;_T14B=_T155->f2;_T14A=_T155->f3;}{struct Cyc_Absyn_Exp*e1=_T14E;struct Cyc_Absyn_Exp*e2=_T14B;struct Cyc_Absyn_Exp*e3=_T14A;_T83=e;_T84=_T83->loc;
# 726
Cyc_Absyndump_dumploc(_T84);
Cyc_Absyndump_dumpexp_prec(myprec,e1);
Cyc_Absyndump_dump_char(63);Cyc_Absyndump_dumpexp_prec(0,e2);
Cyc_Absyndump_dump_char(58);Cyc_Absyndump_dumpexp_prec(myprec,e3);goto _LL0;}case 7:{struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_T13F;_T14E=_T155->f1;_T14B=_T155->f2;}{struct Cyc_Absyn_Exp*e1=_T14E;struct Cyc_Absyn_Exp*e2=_T14B;_T85=e;_T86=_T85->loc;
# 733
Cyc_Absyndump_dumploc(_T86);
Cyc_Absyndump_dumpexp_prec(myprec,e1);_T87=_tag_fat("&&",sizeof(char),3U);Cyc_Absyndump_dump(_T87);Cyc_Absyndump_dumpexp_prec(myprec,e2);goto _LL0;}case 8:{struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_T13F;_T14E=_T155->f1;_T14B=_T155->f2;}{struct Cyc_Absyn_Exp*e1=_T14E;struct Cyc_Absyn_Exp*e2=_T14B;_T88=e;_T89=_T88->loc;
# 737
Cyc_Absyndump_dumploc(_T89);
Cyc_Absyndump_dumpexp_prec(myprec,e1);_T8A=_tag_fat("||",sizeof(char),3U);Cyc_Absyndump_dump(_T8A);Cyc_Absyndump_dumpexp_prec(myprec,e2);goto _LL0;}case 9:{struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_T13F;_T14E=_T155->f1;_T14B=_T155->f2;}{struct Cyc_Absyn_Exp*e1=_T14E;struct Cyc_Absyn_Exp*e2=_T14B;_T8B=e;_T8C=_T8B->loc;
# 743
Cyc_Absyndump_dumploc(_T8C);_T8D=myprec - 1;_T8E=e1;
# 746
Cyc_Absyndump_dumpexp_prec(_T8D,_T8E);Cyc_Absyndump_dump_char(44);_T8F=myprec - 1;_T90=e2;Cyc_Absyndump_dumpexp_prec(_T8F,_T90);goto _LL0;}case 10:{struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_T13F;_T14E=_T155->f1;_T149=_T155->f2;}{struct Cyc_Absyn_Exp*e1=_T14E;struct Cyc_List_List*es=_T149;_T91=e;_T92=_T91->loc;
# 750
Cyc_Absyndump_dumploc(_T92);
Cyc_Absyndump_dumpexp_prec(myprec,e1);_T93=
_tag_fat("(",sizeof(char),2U);Cyc_Absyndump_dump_nospace(_T93);{
# 755
int old_generate_line_directives=Cyc_Absyndump_generate_line_directives;_T95=old_generate_line_directives;
# 757
if(!_T95)goto _TLC8;_T96=e;_T97=_T96->loc;if(_T97==0U)goto _TLC8;else{goto _TLCA;}_TLCA: _T98=e1;_T99=_T98->loc;_T9A=_T99==0U;_T94=!_T9A;goto _TLC9;_TLC8: _T94=0;_TLC9:
# 756
 Cyc_Absyndump_generate_line_directives=_T94;
# 758
Cyc_Absyndump_dumpexps_prec(20,es);_T9B=
_tag_fat(")",sizeof(char),2U);Cyc_Absyndump_dump_nospace(_T9B);
Cyc_Absyndump_generate_line_directives=old_generate_line_directives;goto _LL0;}}case 11:{struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*)_T13F;_T14E=_T155->f1;}{struct Cyc_Absyn_Exp*e1=_T14E;_T9C=e;_T9D=_T9C->loc;
# 764
Cyc_Absyndump_dumploc(_T9D);_T9E=_tag_fat("throw",sizeof(char),6U);Cyc_Absyndump_dump(_T9E);Cyc_Absyndump_dumpexp_prec(myprec,e1);goto _LL0;}case 12:{struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_T13F;_T14E=_T155->f1;}{struct Cyc_Absyn_Exp*e1=_T14E;_T14E=e1;goto _LL36;}case 13:{struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_T13F;_T14E=_T155->f1;}_LL36: {struct Cyc_Absyn_Exp*e1=_T14E;
# 767
Cyc_Absyndump_dumpexp_prec(inprec,e1);goto _LL0;}case 14:{struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_T13F;_T9F=_T155->f1;_T14E=(void*)_T9F;_T14B=_T155->f2;}{void*t=_T14E;struct Cyc_Absyn_Exp*e1=_T14B;
# 770
Cyc_Absyndump_dump_char(40);Cyc_Absyndump_dumptyp(t);Cyc_Absyndump_dump_char(41);Cyc_Absyndump_dumpexp_prec(myprec,e1);goto _LL0;}case 15:{struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_T13F;_T14E=_T155->f1;}{struct Cyc_Absyn_Exp*e1=_T14E;
# 773
Cyc_Absyndump_dump_char(38);Cyc_Absyndump_dumpexp_prec(myprec,e1);goto _LL0;}case 20:{struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_T13F;_T14E=_T155->f1;}{struct Cyc_Absyn_Exp*e1=_T14E;
Cyc_Absyndump_dump_char(42);Cyc_Absyndump_dumpexp_prec(myprec,e1);goto _LL0;}case 16:{struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_T13F;_T14E=_T155->f1;_T14B=_T155->f2;_T14A=_T155->f3;}{struct Cyc_Absyn_Exp*ropt=_T14E;struct Cyc_Absyn_Exp*e1=_T14B;struct Cyc_Absyn_Exp*qual=_T14A;_TA0=ropt;_TA1=(unsigned)_TA0;
# 777
if(!_TA1)goto _TLCB;_TA2=
_tag_fat("rnew(",sizeof(char),6U);Cyc_Absyndump_dump(_TA2);Cyc_Absyndump_dumpexp_prec(140,ropt);_TA3=qual;_TA4=(unsigned)_TA3;
if(!_TA4)goto _TLCD;
Cyc_Absyndump_dump_char(44);Cyc_Absyndump_dumpexp_prec(140,qual);goto _TLCE;_TLCD: _TLCE:
# 782
 Cyc_Absyndump_dump_char(41);goto _TLCC;
# 785
_TLCB: _TA5=_tag_fat("new",sizeof(char),4U);Cyc_Absyndump_dump(_TA5);_TA6=qual;_TA7=(unsigned)_TA6;
if(!_TA7)goto _TLCF;
Cyc_Absyndump_dump_char(40);Cyc_Absyndump_dumpexp_prec(140,qual);Cyc_Absyndump_dump_char(41);goto _TLD0;_TLCF: _TLD0: _TLCC:
# 790
 Cyc_Absyndump_dumpexp_prec(myprec,e1);goto _LL0;}case 17:{struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*)_T13F;_TA8=_T155->f1;_T14E=(void*)_TA8;}{void*t=_T14E;_TA9=
_tag_fat("sizeof(",sizeof(char),8U);Cyc_Absyndump_dump(_TA9);Cyc_Absyndump_dumptyp(t);Cyc_Absyndump_dump_char(41);goto _LL0;}case 18:{struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_T13F;_T14E=_T155->f1;}{struct Cyc_Absyn_Exp*e1=_T14E;_TAA=
_tag_fat("sizeof(",sizeof(char),8U);Cyc_Absyndump_dump(_TAA);Cyc_Absyndump_dumpexp(e1);Cyc_Absyndump_dump_char(41);goto _LL0;}case 38:{struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*)_T13F;_TAB=_T155->f1;_T14E=(void*)_TAB;}{void*t=_T14E;_TAC=
_tag_fat("valueof(",sizeof(char),9U);Cyc_Absyndump_dump(_TAC);Cyc_Absyndump_dumptyp(t);Cyc_Absyndump_dump_char(41);goto _LL0;}case 40:{struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*)_T13F;_T14E=_T155->f1;}{struct Cyc_Absyn_Exp*e1=_T14E;_TAD=
# 796
_tag_fat("__extension__(",sizeof(char),15U);Cyc_Absyndump_dump(_TAD);Cyc_Absyndump_dumpexp(e1);Cyc_Absyndump_dump_char(41);goto _LL0;}case 41:{struct Cyc_Absyn_Assert_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Assert_e_Absyn_Raw_exp_struct*)_T13F;_T14E=_T155->f1;_T150=_T155->f2;}{struct Cyc_Absyn_Exp*e1=_T14E;int static_only=_T150;_TAE=static_only;
# 798
if(!_TAE)goto _TLD1;_TAF=
_tag_fat("@assert(",sizeof(char),9U);Cyc_Absyndump_dump(_TAF);goto _TLD2;
# 801
_TLD1: _TB0=_tag_fat("assert(",sizeof(char),8U);Cyc_Absyndump_dump(_TB0);_TLD2:
 Cyc_Absyndump_dumpexp(e1);Cyc_Absyndump_dump_char(41);goto _LL0;}case 42:{struct Cyc_Absyn_Assert_false_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Assert_false_e_Absyn_Raw_exp_struct*)_T13F;_T14E=_T155->f1;}{struct Cyc_Absyn_Exp*e1=_T14E;_TB1=
# 804
_tag_fat("@assert_false(",sizeof(char),15U);Cyc_Absyndump_dump(_TB1);Cyc_Absyndump_dumpexp(e1);Cyc_Absyndump_dump_char(41);goto _LL0;}case 39:{struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*)_T13F;_T150=_T155->f1;_T152=_T155->f2;_T14E=_T155->f3;_T149=_T155->f4;_T148=_T155->f5;}{int vol=_T150;struct _fat_ptr t=_T152;struct Cyc_List_List*o=_T14E;struct Cyc_List_List*i=_T149;struct Cyc_List_List*cl=_T148;_TB2=
# 807
_tag_fat("__asm__",sizeof(char),8U);Cyc_Absyndump_dump(_TB2);_TB3=vol;
if(!_TB3)goto _TLD3;_TB4=_tag_fat(" volatile ",sizeof(char),11U);Cyc_Absyndump_dump(_TB4);goto _TLD4;_TLD3: _TLD4: _TB5=
_tag_fat("(\"",sizeof(char),3U);Cyc_Absyndump_dump_nospace(_TB5);_TB6=Cyc_Absynpp_string_escape(t);Cyc_Absyndump_dump_nospace(_TB6);Cyc_Absyndump_dump_char(34);_TB7=o;_TB8=(unsigned)_TB7;
if(!_TB8)goto _TLD5;
Cyc_Absyndump_dump_char(58);
Cyc_Absyndump_dump_asm_iolist(o);goto _TLD6;_TLD5: _TLD6: _TB9=i;_TBA=(unsigned)_TB9;
# 814
if(!_TBA)goto _TLD7;_TBB=o;_TBC=(unsigned)_TBB;
if(_TBC)goto _TLD9;else{goto _TLDB;}
_TLDB: Cyc_Absyndump_dump_char(58);goto _TLDA;_TLD9: _TLDA:
 Cyc_Absyndump_dump_char(58);
Cyc_Absyndump_dump_asm_iolist(i);goto _TLD8;_TLD7: _TLD8: _TBD=cl;_TBE=(unsigned)_TBD;
# 820
if(!_TBE)goto _TLDC;_TC0=i;_TC1=(unsigned)_TC0;
if(!_TC1)goto _TLDE;_TBF=2;goto _TLDF;_TLDE: _TC3=o;_TC4=(unsigned)_TC3;if(!_TC4)goto _TLE0;_TC2=1;goto _TLE1;_TLE0: _TC2=0;_TLE1: _TBF=_TC2;_TLDF:{int ncol=_TBF;{
int cols=0;_TLE5: _TC5=cols;_TC6=3 - ncol;if(_TC5 < _TC6)goto _TLE3;else{goto _TLE4;}
_TLE3: _TC7=_tag_fat(" :",sizeof(char),3U);Cyc_Absyndump_dump_nospace(_TC7);
# 822
cols=cols + 1;goto _TLE5;_TLE4:;}
# 824
_TLE6: _TC8=cl;_TC9=(unsigned)_TC8;if(_TC9)goto _TLE7;else{goto _TLE8;}
_TLE7: Cyc_Absyndump_dump_char(34);_TCA=cl;_TCB=_TCA->hd;_TCC=(struct _fat_ptr*)_TCB;_TCD=*_TCC;_TCE=
Cyc_Absynpp_string_escape(_TCD);Cyc_Absyndump_dump_nospace(_TCE);
Cyc_Absyndump_dump_char(34);_TCF=cl;
cl=_TCF->tl;_TD0=cl;_TD1=(unsigned)_TD0;
if(!_TD1)goto _TLE9;
Cyc_Absyndump_dump_char(44);goto _TLEA;_TLE9: _TLEA: goto _TLE6;_TLE8:;}goto _TLDD;_TLDC: _TLDD:
# 833
 Cyc_Absyndump_dump_char(41);goto _LL0;}case 37:{struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_T13F;_T14E=_T155->f1;_T147=_T155->f2;}{struct Cyc_Absyn_Exp*e=_T14E;struct _fat_ptr*f=_T147;_TD2=
# 837
_tag_fat("tagcheck(",sizeof(char),10U);Cyc_Absyndump_dump(_TD2);Cyc_Absyndump_dumpexp(e);Cyc_Absyndump_dump_char(46);_TD3=f;_TD4=*_TD3;Cyc_Absyndump_dump_nospace(_TD4);
Cyc_Absyndump_dump_char(41);goto _LL0;}case 19:{struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_T13F;_TD5=_T155->f1;_T14E=(void*)_TD5;_T149=_T155->f2;}{void*t=_T14E;struct Cyc_List_List*l=_T149;_TD6=
# 842
_tag_fat("offsetof(",sizeof(char),10U);Cyc_Absyndump_dump(_TD6);Cyc_Absyndump_dumptyp(t);_TD7=l;_TD8=_tag_fat(",",sizeof(char),2U);_TD9=_tag_fat(")",sizeof(char),2U);_TDA=_tag_fat(".",sizeof(char),2U);Cyc_Absyndump_group(Cyc_Absyndump_dumpoffset_field,_TD7,_TD8,_TD9,_TDA);goto _LL0;}case 21:{struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_T13F;_T14E=_T155->f1;_T147=_T155->f2;}{struct Cyc_Absyn_Exp*e1=_T14E;struct _fat_ptr*n=_T147;
# 845
Cyc_Absyndump_dumpexp_prec(myprec,e1);Cyc_Absyndump_dump_char(46);_TDB=n;_TDC=*_TDB;Cyc_Absyndump_dump_nospace(_TDC);goto _LL0;}case 22:{struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_T13F;_T14E=_T155->f1;_T147=_T155->f2;}{struct Cyc_Absyn_Exp*e1=_T14E;struct _fat_ptr*n=_T147;
# 847
Cyc_Absyndump_dumpexp_prec(myprec,e1);_TDD=_tag_fat("->",sizeof(char),3U);Cyc_Absyndump_dump_nospace(_TDD);_TDE=n;_TDF=*_TDE;Cyc_Absyndump_dump_nospace(_TDF);goto _LL0;}case 23:{struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_T13F;_T14E=_T155->f1;_T14B=_T155->f2;}{struct Cyc_Absyn_Exp*e1=_T14E;struct Cyc_Absyn_Exp*e2=_T14B;
# 850
Cyc_Absyndump_dumpexp_prec(myprec,e1);Cyc_Absyndump_dump_char(91);Cyc_Absyndump_dumpexp(e2);Cyc_Absyndump_dump_char(93);goto _LL0;}case 24:{struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*)_T13F;_T14E=_T155->f1;_T149=_T155->f2;}{struct _tuple9*vat=_T14E;struct Cyc_List_List*des=_T149;
# 853
Cyc_Absyndump_dump_char(40);_TE0=vat;_TE1=*_TE0;_TE2=_TE1.f2;
Cyc_Absyndump_dumptyp(_TE2);
Cyc_Absyndump_dump_char(41);{
void(*_T155)(void(*)(struct _tuple18*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr)=(void(*)(void(*)(struct _tuple18*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr))Cyc_Absyndump_group;_TE3=_T155;}_TE4=des;_TE5=_tag_fat("{",sizeof(char),2U);_TE6=_tag_fat("}",sizeof(char),2U);_TE7=_tag_fat(",",sizeof(char),2U);_TE3(Cyc_Absyndump_dumpde,_TE4,_TE5,_TE6,_TE7);goto _LL0;}case 25:{struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_T13F;_T14E=_T155->f1;}{struct Cyc_List_List*des=_T14E;{
# 859
void(*_T155)(void(*)(struct _tuple18*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr)=(void(*)(void(*)(struct _tuple18*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr))Cyc_Absyndump_group;_TE8=_T155;}_TE9=des;_TEA=_tag_fat("{",sizeof(char),2U);_TEB=_tag_fat("}",sizeof(char),2U);_TEC=_tag_fat(",",sizeof(char),2U);_TE8(Cyc_Absyndump_dumpde,_TE9,_TEA,_TEB,_TEC);goto _LL0;}case 26:{struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_T13F;_T14E=_T155->f1;_T14B=_T155->f2;_T14A=_T155->f3;}{struct Cyc_Absyn_Vardecl*vd=_T14E;struct Cyc_Absyn_Exp*e1=_T14B;struct Cyc_Absyn_Exp*e2=_T14A;_TED=
# 862
_tag_fat("{for",sizeof(char),5U);Cyc_Absyndump_dump(_TED);_TEE=vd;_TEF=_TEE->name;_TF0=*_TEF;_TF1=_TF0.f1;Cyc_Absyndump_dump_str(_TF1);Cyc_Absyndump_dump_char(60);Cyc_Absyndump_dumpexp(e1);
Cyc_Absyndump_dump_char(58);Cyc_Absyndump_dumpexp(e2);Cyc_Absyndump_dump_char(125);goto _LL0;}case 27:{struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_T13F;_T14B=_T155->f1;_TF2=_T155->f2;_T14E=(void*)_TF2;}{struct Cyc_Absyn_Exp*e=_T14B;void*t=_T14E;_TF3=
# 867
_tag_fat("{for x ",sizeof(char),8U);Cyc_Absyndump_dump(_TF3);Cyc_Absyndump_dump_char(60);Cyc_Absyndump_dumpexp(e);
Cyc_Absyndump_dump_char(58);Cyc_Absyndump_dumptyp(t);Cyc_Absyndump_dump_char(125);goto _LL0;}case 28:{struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_T13F;_T146=_T155->f1;_T149=_T155->f2;_T148=_T155->f3;}{struct _tuple1*n=_T146;struct Cyc_List_List*ts=_T149;struct Cyc_List_List*des=_T148;
# 872
Cyc_Absyndump_dumpqvar(n);
Cyc_Absyndump_dump_char(123);
if(ts==0)goto _TLEB;
Cyc_Absyndump_dumptps(ts);goto _TLEC;_TLEB: _TLEC:{
void(*_T155)(void(*)(struct _tuple18*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr)=(void(*)(void(*)(struct _tuple18*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr))Cyc_Absyndump_group;_TF4=_T155;}_TF5=des;_TF6=_tag_fat("",sizeof(char),1U);_TF7=_tag_fat("}",sizeof(char),2U);_TF8=_tag_fat(",",sizeof(char),2U);_TF4(Cyc_Absyndump_dumpde,_TF5,_TF6,_TF7,_TF8);goto _LL0;}case 29:{struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*)_T13F;_T150=_T155->f2;_T149=_T155->f3;}{int is_tuple=_T150;struct Cyc_List_List*des=_T149;_TF9=is_tuple;
# 880
if(!_TF9)goto _TLED;_TFA=
_tag_fat("$(",sizeof(char),3U);Cyc_Absyndump_dump(_TFA);_TFC=Cyc_List_map;{struct Cyc_List_List*(*_T155)(struct Cyc_Absyn_Exp*(*)(struct _tuple18*),struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*)(struct _tuple18*),struct Cyc_List_List*))_TFC;_TFB=_T155;}_TFE=Cyc_Core_snd;{struct Cyc_Absyn_Exp*(*_T155)(struct _tuple18*)=(struct Cyc_Absyn_Exp*(*)(struct _tuple18*))_TFE;_TFD=_T155;}_TFF=des;_T100=_TFB(_TFD,_TFF);Cyc_Absyndump_dumpexps_prec(20,_T100);Cyc_Absyndump_dump_char(41);goto _TLEE;
# 883
_TLED:{void(*_T155)(void(*)(struct _tuple18*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr)=(void(*)(void(*)(struct _tuple18*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr))Cyc_Absyndump_group;_T101=_T155;}_T102=des;_T103=_tag_fat("{",sizeof(char),2U);_T104=_tag_fat("}",sizeof(char),2U);_T105=_tag_fat(",",sizeof(char),2U);_T101(Cyc_Absyndump_dumpde,_T102,_T103,_T104,_T105);_TLEE: goto _LL0;}case 31:{struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct*)_T13F;_T145=_T155->f2;}{struct Cyc_Absyn_Enumfield*ef=_T145;_T145=ef;goto _LL66;}case 32:{struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct*)_T13F;_T145=_T155->f2;}_LL66: {struct Cyc_Absyn_Enumfield*ef=_T145;_T106=ef;_T107=_T106->name;
# 887
Cyc_Absyndump_dumpqvar(_T107);goto _LL0;}case 30:{struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_T13F;_T149=_T155->f1;_T144=_T155->f3;}{struct Cyc_List_List*es=_T149;struct Cyc_Absyn_Datatypefield*tuf=_T144;_T108=tuf;_T109=_T108->name;
# 890
Cyc_Absyndump_dumpqvar(_T109);
if(es==0)goto _TLEF;{void(*_T155)(void(*)(struct Cyc_Absyn_Exp*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr)=(void(*)(void(*)(struct Cyc_Absyn_Exp*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr))Cyc_Absyndump_group;_T10A=_T155;}_T10B=Cyc_Absyndump_dumpexp;_T10C=es;_T10D=_tag_fat("(",sizeof(char),2U);_T10E=_tag_fat(")",sizeof(char),2U);_T10F=_tag_fat(",",sizeof(char),2U);_T10A(_T10B,_T10C,_T10D,_T10E,_T10F);goto _TLF0;_TLEF: _TLF0: goto _LL0;}case 33:{struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_T13F;_T110=_T155->f1;_T143=_T110.mknd;_T111=_T155->f1;_T14B=_T111.rgn;_T112=_T155->f1;_T14A=_T112.aqual;_T113=_T155->f1;_T142=_T113.elt_type;_T114=_T155->f1;_T141=_T114.num_elts;_T115=_T155->f1;_T150=_T115.inline_call;}{enum Cyc_Absyn_MallocKind mknd=_T143;struct Cyc_Absyn_Exp*ropt=_T14B;struct Cyc_Absyn_Exp*aqopt=_T14A;void**topt=_T142;struct Cyc_Absyn_Exp*e=_T141;int inline_call=_T150;_T116=e;_T117=_T116->loc;
# 895
Cyc_Absyndump_dumploc(_T117);_T118=mknd;_T119=(int)_T118;
if(_T119!=1)goto _TLF1;
if(ropt==0)goto _TLF3;_T11A=
_tag_fat("rcalloc(",sizeof(char),9U);Cyc_Absyndump_dump(_T11A);Cyc_Absyndump_dumpexp(ropt);_T11B=_tag_fat(",",sizeof(char),2U);Cyc_Absyndump_dump(_T11B);
if(aqopt==0)goto _TLF5;
Cyc_Absyndump_dumpexp(aqopt);_T11C=_tag_fat(",",sizeof(char),2U);Cyc_Absyndump_dump(_T11C);goto _TLF6;_TLF5: _TLF6: goto _TLF4;
# 903
_TLF3: if(aqopt==0)goto _TLF7;_T11D=
_tag_fat("qcalloc(",sizeof(char),9U);Cyc_Absyndump_dump(_T11D);Cyc_Absyndump_dumpexp(aqopt);_T11E=_tag_fat(",",sizeof(char),2U);Cyc_Absyndump_dump(_T11E);goto _TLF8;
# 907
_TLF7: _T11F=_tag_fat("calloc(",sizeof(char),8U);Cyc_Absyndump_dump(_T11F);_TLF8: _TLF4:
# 909
 Cyc_Absyndump_dumpexp(e);_T120=_tag_fat(",",sizeof(char),2U);Cyc_Absyndump_dump(_T120);_T121=_check_null(topt);_T122=*_T121;_T123=Cyc_Absyn_sizeoftype_exp(_T122,0U);Cyc_Absyndump_dumpexp(_T123);_T124=_tag_fat(")",sizeof(char),2U);Cyc_Absyndump_dump(_T124);goto _TLF2;
# 912
_TLF1: if(ropt==0)goto _TLF9;_T126=mknd;_T127=(int)_T126;
if(_T127!=2)goto _TLFB;_T125=_tag_fat("rvmalloc(",sizeof(char),10U);goto _TLFC;_TLFB: _T129=inline_call;if(!_T129)goto _TLFD;_T128=_tag_fat("rmalloc_inline(",sizeof(char),16U);goto _TLFE;_TLFD: _T128=_tag_fat("rmalloc(",sizeof(char),9U);_TLFE: _T125=_T128;_TLFC: Cyc_Absyndump_dump(_T125);
Cyc_Absyndump_dumpexp(ropt);_T12A=_tag_fat(",",sizeof(char),2U);Cyc_Absyndump_dump(_T12A);
if(aqopt==0)goto _TLFF;
Cyc_Absyndump_dumpexp(aqopt);_T12B=_tag_fat(",",sizeof(char),2U);Cyc_Absyndump_dump(_T12B);goto _TL100;_TLFF: _TL100: goto _TLFA;
# 919
_TLF9: if(aqopt==0)goto _TL101;_T12C=
_tag_fat("qmalloc(",sizeof(char),9U);Cyc_Absyndump_dump(_T12C);Cyc_Absyndump_dumpexp(aqopt);_T12D=_tag_fat(",",sizeof(char),2U);Cyc_Absyndump_dump(_T12D);goto _TL102;
# 923
_TL101: _T12E=_tag_fat("malloc(",sizeof(char),8U);Cyc_Absyndump_dump(_T12E);_TL102: _TLFA:
# 926
 if(topt==0)goto _TL103;_T12F=topt;_T130=*_T12F;_T131=
Cyc_Absyn_sizeoftype_exp(_T130,0U);_T132=e;_T133=Cyc_Absyn_times_exp(_T131,_T132,0U);Cyc_Absyndump_dumpexp(_T133);goto _TL104;
# 929
_TL103: Cyc_Absyndump_dumpexp(e);_TL104: _T134=
_tag_fat(")",sizeof(char),2U);Cyc_Absyndump_dump(_T134);_TLF2: goto _LL0;}case 34:{struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_T13F;_T14B=_T155->f1;_T14A=_T155->f2;}{struct Cyc_Absyn_Exp*e1=_T14B;struct Cyc_Absyn_Exp*e2=_T14A;
# 935
Cyc_Absyndump_dumpexp_prec(myprec,e1);_T135=_tag_fat(":=:",sizeof(char),4U);Cyc_Absyndump_dump_nospace(_T135);Cyc_Absyndump_dumpexp_prec(myprec,e2);goto _LL0;}case 35:{struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_T13F;_T14C=_T155->f1;_T149=_T155->f2;}{struct Cyc_Core_Opt*n=_T14C;struct Cyc_List_List*des=_T149;{
# 938
void(*_T155)(void(*)(struct _tuple18*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr)=(void(*)(void(*)(struct _tuple18*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr))Cyc_Absyndump_group;_T136=_T155;}_T137=des;_T138=_tag_fat("{",sizeof(char),2U);_T139=_tag_fat("}",sizeof(char),2U);_T13A=_tag_fat(",",sizeof(char),2U);_T136(Cyc_Absyndump_dumpde,_T137,_T138,_T139,_T13A);goto _LL0;}default:{struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*_T155=(struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*)_T13F;_T140=_T155->f1;}{struct Cyc_Absyn_Stmt*s=_T140;_T13B=
# 941
_tag_fat("({",sizeof(char),3U);Cyc_Absyndump_dump_nospace(_T13B);_T13C=s;_T13D=Cyc_Absyndump_mt_set();Cyc_Absyndump_dumpstmt(_T13C,1,_T13D);_T13E=_tag_fat("})",sizeof(char),3U);Cyc_Absyndump_dump_nospace(_T13E);goto _LL0;}}_LL0:;}
# 943
if(inprec < myprec)goto _TL105;if(inprec!=140)goto _TL107;else{goto _TL108;}_TL108: if(myprec!=140)goto _TL107;else{goto _TL105;}
# 945
_TL107: Cyc_Absyndump_dump_char(41);goto _TL106;_TL105: _TL106:;}
# 948
static void Cyc_Absyndump_dumpexp(struct Cyc_Absyn_Exp*e){
Cyc_Absyndump_dumpexp_prec(0,e);}
# 952
static void Cyc_Absyndump_dumpswitchclauses(struct Cyc_List_List*scs,struct Cyc_Dict_Dict varsinblock){struct Cyc_List_List*_T0;void*_T1;struct Cyc_Absyn_Switch_clause*_T2;struct Cyc_Absyn_Exp*_T3;struct Cyc_Absyn_Switch_clause*_T4;struct Cyc_Absyn_Pat*_T5;void*_T6;struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct*_T7;struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct*_T8;void*_T9;struct _fat_ptr _TA;struct _fat_ptr _TB;struct Cyc_Absyn_Switch_clause*_TC;struct Cyc_Absyn_Pat*_TD;struct Cyc_Absyn_Switch_clause*_TE;struct Cyc_Absyn_Exp*_TF;struct _fat_ptr _T10;struct Cyc_Absyn_Switch_clause*_T11;struct Cyc_Absyn_Exp*_T12;struct Cyc_Absyn_Exp*_T13;struct _fat_ptr _T14;struct Cyc_Absyn_Switch_clause*_T15;struct Cyc_Absyn_Stmt*_T16;int _T17;struct _fat_ptr _T18;struct Cyc_Absyn_Switch_clause*_T19;struct Cyc_Absyn_Stmt*_T1A;struct Cyc_Dict_Dict _T1B;struct Cyc_Absyn_Switch_clause*_T1C;struct Cyc_Absyn_Stmt*_T1D;struct Cyc_Dict_Dict _T1E;struct Cyc_List_List*_T1F;
# 954
_TL10C: if(scs!=0)goto _TL10A;else{goto _TL10B;}
_TL10A: _T0=scs;_T1=_T0->hd;{struct Cyc_Absyn_Switch_clause*c=(struct Cyc_Absyn_Switch_clause*)_T1;_T2=c;_T3=_T2->where_clause;
if(_T3!=0)goto _TL10D;_T4=c;_T5=_T4->pattern;_T6=_T5->r;_T7=& Cyc_Absyn_Wild_p_val;_T8=(struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct*)_T7;_T9=(void*)_T8;if(_T6!=_T9)goto _TL10D;_TA=
_tag_fat("default:",sizeof(char),9U);Cyc_Absyndump_dump(_TA);goto _TL10E;
# 959
_TL10D: _TB=_tag_fat("case",sizeof(char),5U);Cyc_Absyndump_dump(_TB);_TC=c;_TD=_TC->pattern;
Cyc_Absyndump_dumppat(_TD);_TE=c;_TF=_TE->where_clause;
if(_TF==0)goto _TL10F;_T10=
_tag_fat("&&",sizeof(char),3U);Cyc_Absyndump_dump(_T10);_T11=c;_T12=_T11->where_clause;_T13=
_check_null(_T12);Cyc_Absyndump_dumpexp(_T13);goto _TL110;_TL10F: _TL110: _T14=
# 965
_tag_fat(":",sizeof(char),2U);Cyc_Absyndump_dump_nospace(_T14);_TL10E: _T15=c;_T16=_T15->body;_T17=
# 967
Cyc_Absynpp_is_declaration(_T16);if(!_T17)goto _TL111;_T18=
_tag_fat(" {",sizeof(char),3U);Cyc_Absyndump_dump(_T18);_T19=c;_T1A=_T19->body;_T1B=Cyc_Absyndump_mt_set();Cyc_Absyndump_dumpstmt(_T1A,0,_T1B);Cyc_Absyndump_dump_char(125);goto _TL112;
# 970
_TL111: _T1C=c;_T1D=_T1C->body;_T1E=varsinblock;Cyc_Absyndump_dumpstmt(_T1D,0,_T1E);_TL112:;}_T1F=scs;
# 954
scs=_T1F->tl;goto _TL10C;_TL10B:;}struct _tuple20{int f0;struct Cyc_Dict_Dict f1;};
# 974
static struct _tuple20 Cyc_Absyndump_shadows(struct Cyc_Absyn_Decl*d,struct Cyc_Dict_Dict s){struct Cyc_Absyn_Decl*_T0;int*_T1;int _T2;int(*_T3)(struct Cyc_Dict_Dict,struct _tuple1*);int(*_T4)(struct Cyc_Dict_Dict,void*);struct Cyc_Dict_Dict _T5;struct Cyc_Absyn_Vardecl*_T6;struct _tuple1*_T7;int _T8;struct _tuple20 _T9;struct Cyc_Dict_Dict(*_TA)(int(*)(struct _tuple1*,struct _tuple1*),struct _tuple1*,struct _tuple1*);struct Cyc_Dict_Dict(*_TB)(int(*)(void*,void*),void*,void*);int(*_TC)(struct _tuple1*,struct _tuple1*);struct Cyc_Absyn_Vardecl*_TD;struct _tuple1*_TE;struct Cyc_Absyn_Vardecl*_TF;struct _tuple1*_T10;struct _tuple20 _T11;struct Cyc_Dict_Dict(*_T12)(struct Cyc_Dict_Dict,struct _tuple1*,struct _tuple1*);struct Cyc_Dict_Dict(*_T13)(struct Cyc_Dict_Dict,void*,void*);struct Cyc_Dict_Dict _T14;struct Cyc_Absyn_Vardecl*_T15;struct _tuple1*_T16;struct Cyc_Absyn_Vardecl*_T17;struct _tuple1*_T18;struct _tuple20 _T19;_T0=d;{
void*_T1A=_T0->r;struct Cyc_Absyn_Vardecl*_T1B;_T1=(int*)_T1A;_T2=*_T1;if(_T2!=0)goto _TL113;{struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_T1C=(struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_T1A;_T1B=_T1C->f1;}{struct Cyc_Absyn_Vardecl*vd=_T1B;_T4=Cyc_Dict_member;{
# 977
int(*_T1C)(struct Cyc_Dict_Dict,struct _tuple1*)=(int(*)(struct Cyc_Dict_Dict,struct _tuple1*))_T4;_T3=_T1C;}_T5=s;_T6=vd;_T7=_T6->name;_T8=_T3(_T5,_T7);if(!_T8)goto _TL115;{struct _tuple20 _T1C;_T1C.f0=1;_TB=Cyc_Dict_singleton;{struct Cyc_Dict_Dict(*_T1D)(int(*)(struct _tuple1*,struct _tuple1*),struct _tuple1*,struct _tuple1*)=(struct Cyc_Dict_Dict(*)(int(*)(struct _tuple1*,struct _tuple1*),struct _tuple1*,struct _tuple1*))_TB;_TA=_T1D;}_TC=Cyc_Absyn_qvar_cmp;_TD=vd;_TE=_TD->name;_TF=vd;_T10=_TF->name;_T1C.f1=_TA(_TC,_TE,_T10);_T9=_T1C;}return _T9;_TL115:{struct _tuple20 _T1C;
_T1C.f0=0;_T13=Cyc_Dict_insert;{struct Cyc_Dict_Dict(*_T1D)(struct Cyc_Dict_Dict,struct _tuple1*,struct _tuple1*)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict,struct _tuple1*,struct _tuple1*))_T13;_T12=_T1D;}_T14=s;_T15=vd;_T16=_T15->name;_T17=vd;_T18=_T17->name;_T1C.f1=_T12(_T14,_T16,_T18);_T11=_T1C;}return _T11;}_TL113:{struct _tuple20 _T1C;
_T1C.f0=0;_T1C.f1=s;_T19=_T1C;}return _T19;;}}
# 983
static void Cyc_Absyndump_dumpstmt(struct Cyc_Absyn_Stmt*s,int expstmt,struct Cyc_Dict_Dict varsinblock){struct Cyc_Absyn_Stmt*_T0;unsigned _T1;struct Cyc_Absyn_Stmt*_T2;int*_T3;unsigned _T4;int _T5;struct Cyc_Absyn_Stmt*_T6;struct Cyc_Dict_Dict _T7;struct Cyc_Absyn_Stmt*_T8;int*_T9;int _TA;struct Cyc_Absyn_Stmt*_TB;unsigned _TC;int _TD;int _TE;struct Cyc_Absyn_Stmt*_TF;int _T10;struct Cyc_Dict_Dict _T11;int _T12;struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*_T13;struct Cyc_Absyn_Exp*_T14;struct _fat_ptr _T15;struct _fat_ptr _T16;struct Cyc_Absyn_Exp*_T17;struct _fat_ptr _T18;struct Cyc_Absyn_Stmt*_T19;int*_T1A;unsigned _T1B;struct _fat_ptr _T1C;struct Cyc_Absyn_Stmt*_T1D;struct Cyc_Dict_Dict _T1E;struct Cyc_Absyn_Stmt*_T1F;int*_T20;int _T21;struct _fat_ptr _T22;struct Cyc_Absyn_Stmt*_T23;struct Cyc_Dict_Dict _T24;struct _tuple10 _T25;struct _fat_ptr _T26;struct _fat_ptr _T27;struct Cyc_Absyn_Stmt*_T28;struct Cyc_Dict_Dict _T29;struct _tuple10 _T2A;struct _tuple10 _T2B;struct _fat_ptr _T2C;struct _fat_ptr _T2D;struct Cyc_Absyn_Stmt*_T2E;struct Cyc_Dict_Dict _T2F;struct _tuple10 _T30;struct _fat_ptr _T31;struct Cyc_Absyn_Stmt*_T32;struct Cyc_Dict_Dict _T33;struct _fat_ptr _T34;struct _fat_ptr _T35;struct _fat_ptr _T36;struct _fat_ptr _T37;struct _fat_ptr _T38;struct _fat_ptr _T39;struct _fat_ptr _T3A;struct _fat_ptr _T3B;struct _fat_ptr _T3C;int _T3D;int _T3E;struct _fat_ptr _T3F;struct _fat_ptr _T40;struct Cyc_Absyn_Stmt*_T41;int _T42;struct Cyc_Dict_Dict _T43;int _T44;struct _fat_ptr _T45;struct _fat_ptr _T46;struct Cyc_Absyn_Stmt*_T47;int*_T48;int _T49;int _T4A;struct Cyc_Absyn_Stmt*_T4B;unsigned _T4C;int _T4D;int _T4E;struct _fat_ptr _T4F;struct _fat_ptr _T50;struct Cyc_Absyn_Stmt*_T51;int _T52;struct Cyc_Dict_Dict _T53;int _T54;struct _fat_ptr _T55;struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct*_T56;struct Cyc_List_List*_T57;struct _fat_ptr _T58;struct _fat_ptr _T59;struct _fat_ptr _T5A;
LOOP: _T0=s;_T1=_T0->loc;
Cyc_Absyndump_dumploc(_T1);_T2=s;{
void*_T5B=_T2->r;struct Cyc_Absyn_Decl*_T5C;struct Cyc_List_List*_T5D;struct _fat_ptr*_T5E;struct Cyc_Absyn_Exp*_T5F;struct Cyc_Absyn_Exp*_T60;struct Cyc_Absyn_Stmt*_T61;struct Cyc_Absyn_Stmt*_T62;struct Cyc_Absyn_Exp*_T63;_T3=(int*)_T5B;_T4=*_T3;switch(_T4){case 0:
 Cyc_Absyndump_dump_semi();goto _LL0;case 1:{struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*_T64=(struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_T5B;_T63=_T64->f1;}{struct Cyc_Absyn_Exp*e=_T63;
Cyc_Absyndump_dumpexp_prec(-100,e);Cyc_Absyndump_dump_semi();goto _LL0;}case 2:{struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*_T64=(struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_T5B;_T62=_T64->f1;_T61=_T64->f2;}{struct Cyc_Absyn_Stmt*s1=_T62;struct Cyc_Absyn_Stmt*s2=_T61;
# 993
_TL118: if(1)goto _TL119;else{goto _TL11A;}
_TL119: _T5=Cyc_Absynpp_is_declaration(s1);if(!_T5)goto _TL11B;
Cyc_Absyndump_dump_char(123);_T6=s1;_T7=Cyc_Absyndump_mt_set();Cyc_Absyndump_dumpstmt(_T6,0,_T7);Cyc_Absyndump_dump_char(125);goto _TL11C;
# 997
_TL11B: Cyc_Absyndump_dumpstmt(s1,0,varsinblock);_TL11C: _T8=s2;{
void*_T64=_T8->r;struct Cyc_Absyn_Stmt*_T65;struct Cyc_Absyn_Stmt*_T66;_T9=(int*)_T64;_TA=*_T9;if(_TA!=2)goto _TL11D;{struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*_T67=(struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_T64;_T66=_T67->f1;_T65=_T67->f2;}{struct Cyc_Absyn_Stmt*s3=_T66;struct Cyc_Absyn_Stmt*s4=_T65;_TB=s3;_TC=_TB->loc;
# 1000
Cyc_Absyndump_dumploc(_TC);
s1=s3;s2=s4;goto _TL118;}_TL11D: _TD=
# 1003
Cyc_Absynpp_is_declaration(s2);if(!_TD)goto _TL11F;_TE=expstmt;
if(!_TE)goto _TL121;Cyc_Absyndump_dump_char(40);goto _TL122;_TL121: _TL122:
 Cyc_Absyndump_dump_char(123);_TF=s2;_T10=expstmt;_T11=Cyc_Absyndump_mt_set();Cyc_Absyndump_dumpstmt(_TF,_T10,_T11);Cyc_Absyndump_dump_char(125);_T12=expstmt;
if(!_T12)goto _TL123;Cyc_Absyndump_dump_char(41);Cyc_Absyndump_dump_semi();goto _TL124;_TL123: _TL124: goto _TL120;
# 1008
_TL11F: Cyc_Absyndump_dumpstmt(s2,expstmt,varsinblock);_TL120:
 return;;}goto _TL118;_TL11A:;}case 3: _T13=(struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*)_T5B;_T14=_T13->f1;if(_T14!=0)goto _TL125;_T15=
# 1012
_tag_fat("return;",sizeof(char),8U);Cyc_Absyndump_dump(_T15);goto _LL0;_TL125:{struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*_T64=(struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*)_T5B;_T63=_T64->f1;}{struct Cyc_Absyn_Exp*e=_T63;_T16=
_tag_fat("return",sizeof(char),7U);Cyc_Absyndump_dump(_T16);_T17=_check_null(e);Cyc_Absyndump_dumpexp(_T17);Cyc_Absyndump_dump_semi();goto _LL0;}case 4:{struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*_T64=(struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_T5B;_T63=_T64->f1;_T62=_T64->f2;_T61=_T64->f3;}{struct Cyc_Absyn_Exp*e=_T63;struct Cyc_Absyn_Stmt*s1=_T62;struct Cyc_Absyn_Stmt*s2=_T61;_T18=
# 1015
_tag_fat("if(",sizeof(char),4U);Cyc_Absyndump_dump(_T18);Cyc_Absyndump_dumpexp(e);_T19=s1;{
void*_T64=_T19->r;_T1A=(int*)_T64;_T1B=*_T1A;switch(_T1B){case 2: goto _LL2E;case 12: _LL2E: goto _LL30;case 4: _LL30: goto _LL32;case 13: _LL32: _T1C=
# 1021
_tag_fat("){",sizeof(char),3U);Cyc_Absyndump_dump_nospace(_T1C);_T1D=s1;_T1E=Cyc_Absyndump_mt_set();Cyc_Absyndump_dumpstmt(_T1D,0,_T1E);Cyc_Absyndump_dump_char(125);goto _LL2A;default:
 Cyc_Absyndump_dump_char(41);Cyc_Absyndump_dumpstmt(s1,0,varsinblock);}_LL2A:;}_T1F=s2;{
# 1024
void*_T64=_T1F->r;_T20=(int*)_T64;_T21=*_T20;if(_T21!=0)goto _TL128;goto _LL35;_TL128: _T22=
# 1026
_tag_fat("else{",sizeof(char),6U);Cyc_Absyndump_dump(_T22);_T23=s2;_T24=Cyc_Absyndump_mt_set();Cyc_Absyndump_dumpstmt(_T23,0,_T24);Cyc_Absyndump_dump_char(125);goto _LL35;_LL35:;}goto _LL0;}case 5:{struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*_T64=(struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_T5B;_T25=_T64->f1;_T63=_T25.f0;_T62=_T64->f2;}{struct Cyc_Absyn_Exp*e=_T63;struct Cyc_Absyn_Stmt*s1=_T62;_T26=
# 1031
_tag_fat("while(",sizeof(char),7U);Cyc_Absyndump_dump(_T26);Cyc_Absyndump_dumpexp(e);_T27=_tag_fat("){",sizeof(char),3U);Cyc_Absyndump_dump_nospace(_T27);_T28=s1;_T29=
Cyc_Absyndump_mt_set();Cyc_Absyndump_dumpstmt(_T28,0,_T29);Cyc_Absyndump_dump_char(125);goto _LL0;}case 9:{struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*_T64=(struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_T5B;_T63=_T64->f1;_T2A=_T64->f2;_T60=_T2A.f0;_T2B=_T64->f3;_T5F=_T2B.f0;_T62=_T64->f4;}{struct Cyc_Absyn_Exp*e1=_T63;struct Cyc_Absyn_Exp*e2=_T60;struct Cyc_Absyn_Exp*e3=_T5F;struct Cyc_Absyn_Stmt*s1=_T62;_T2C=
# 1035
_tag_fat("for(",sizeof(char),5U);Cyc_Absyndump_dump(_T2C);Cyc_Absyndump_dumpexp(e1);Cyc_Absyndump_dump_semi();Cyc_Absyndump_dumpexp(e2);Cyc_Absyndump_dump_semi();Cyc_Absyndump_dumpexp(e3);_T2D=
_tag_fat("){",sizeof(char),3U);Cyc_Absyndump_dump_nospace(_T2D);_T2E=s1;_T2F=Cyc_Absyndump_mt_set();Cyc_Absyndump_dumpstmt(_T2E,0,_T2F);Cyc_Absyndump_dump_char(125);goto _LL0;}case 14:{struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*_T64=(struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_T5B;_T62=_T64->f1;_T30=_T64->f2;_T63=_T30.f0;}{struct Cyc_Absyn_Stmt*s1=_T62;struct Cyc_Absyn_Exp*e=_T63;_T31=
# 1039
_tag_fat("do{",sizeof(char),4U);Cyc_Absyndump_dump(_T31);_T32=s1;_T33=Cyc_Absyndump_mt_set();Cyc_Absyndump_dumpstmt(_T32,0,_T33);_T34=
_tag_fat("}while(",sizeof(char),8U);Cyc_Absyndump_dump_nospace(_T34);Cyc_Absyndump_dumpexp(e);_T35=_tag_fat(");",sizeof(char),3U);Cyc_Absyndump_dump_nospace(_T35);goto _LL0;}case 6: _T36=
# 1043
_tag_fat("break;",sizeof(char),7U);Cyc_Absyndump_dump(_T36);goto _LL0;case 7: _T37=
_tag_fat("continue;",sizeof(char),10U);Cyc_Absyndump_dump(_T37);goto _LL0;case 8:{struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct*_T64=(struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct*)_T5B;_T5E=_T64->f1;}{struct _fat_ptr*x=_T5E;_T38=
_tag_fat("goto",sizeof(char),5U);Cyc_Absyndump_dump(_T38);Cyc_Absyndump_dump_str(x);Cyc_Absyndump_dump_semi();goto _LL0;}case 10:{struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*_T64=(struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_T5B;_T63=_T64->f1;_T5D=_T64->f2;}{struct Cyc_Absyn_Exp*e=_T63;struct Cyc_List_List*ss=_T5D;_T39=
# 1048
_tag_fat("switch(",sizeof(char),8U);Cyc_Absyndump_dump(_T39);Cyc_Absyndump_dumpexp(e);_T3A=_tag_fat("){",sizeof(char),3U);Cyc_Absyndump_dump_nospace(_T3A);
Cyc_Absyndump_dumpswitchclauses(ss,varsinblock);
Cyc_Absyndump_dump_char(125);goto _LL0;}case 15:{struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*_T64=(struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*)_T5B;_T62=_T64->f1;_T5D=_T64->f2;}{struct Cyc_Absyn_Stmt*s1=_T62;struct Cyc_List_List*ss=_T5D;_T3B=
# 1053
_tag_fat("try",sizeof(char),4U);Cyc_Absyndump_dump(_T3B);Cyc_Absyndump_dumpstmt(s1,0,varsinblock);_T3C=
_tag_fat("catch{",sizeof(char),7U);Cyc_Absyndump_dump(_T3C);
Cyc_Absyndump_dumpswitchclauses(ss,varsinblock);Cyc_Absyndump_dump_char(125);goto _LL0;}case 12:{struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*_T64=(struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_T5B;_T5C=_T64->f1;_T62=_T64->f2;}{struct Cyc_Absyn_Decl*d=_T5C;struct Cyc_Absyn_Stmt*s1=_T62;
# 1059
struct _tuple20 _T64=Cyc_Absyndump_shadows(d,varsinblock);struct Cyc_Dict_Dict _T65;int _T66;_T66=_T64.f0;_T65=_T64.f1;{int newblock=_T66;struct Cyc_Dict_Dict newvarsinblock=_T65;_T3D=newblock;
if(!_T3D)goto _TL12A;_T3E=expstmt;
if(!_T3E)goto _TL12C;_T3F=_tag_fat("({",sizeof(char),3U);Cyc_Absyndump_dump(_T3F);goto _TL12D;_TL12C: _T40=_tag_fat("{",sizeof(char),2U);Cyc_Absyndump_dump(_T40);_TL12D:
 Cyc_Absyndump_dumpdecl(d);_T41=s1;_T42=expstmt;_T43=
Cyc_Absyndump_mt_set();Cyc_Absyndump_dumpstmt(_T41,_T42,_T43);_T44=expstmt;
if(!_T44)goto _TL12E;_T45=_tag_fat("});",sizeof(char),4U);Cyc_Absyndump_dump_nospace(_T45);goto _TL12F;_TL12E: _T46=_tag_fat("}",sizeof(char),2U);Cyc_Absyndump_dump(_T46);_TL12F: goto _TL12B;
# 1066
_TL12A: _TL130: if(1)goto _TL131;else{goto _TL132;}
_TL131: Cyc_Absyndump_dumpdecl(d);_T47=s1;{
void*_T67=_T47->r;struct Cyc_Absyn_Stmt*_T68;struct Cyc_Absyn_Decl*_T69;_T48=(int*)_T67;_T49=*_T48;if(_T49!=12)goto _TL133;{struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*_T6A=(struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_T67;_T69=_T6A->f1;_T68=_T6A->f2;}{struct Cyc_Absyn_Decl*d2=_T69;struct Cyc_Absyn_Stmt*s2=_T68;
# 1070
struct _tuple20 _T6A=Cyc_Absyndump_shadows(d2,newvarsinblock);struct Cyc_Dict_Dict _T6B;int _T6C;_T6C=_T6A.f0;_T6B=_T6A.f1;{int newblock=_T6C;struct Cyc_Dict_Dict newvarsinblock2=_T6B;_T4A=newblock;
if(_T4A)goto _TL135;else{goto _TL137;}
_TL137: _T4B=s1;_T4C=_T4B->loc;Cyc_Absyndump_dumploc(_T4C);
newvarsinblock=newvarsinblock2;
d=d2;s1=s2;goto _TL130;_TL135: goto _LL41;}}_TL133: _LL41:
# 1078
 Cyc_Absyndump_dumpstmt(s1,expstmt,newvarsinblock);return;;}goto _TL130;_TL132: _TL12B: goto _LL0;}}case 13:{struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*_T64=(struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*)_T5B;_T5E=_T64->f1;_T62=_T64->f2;}{struct _fat_ptr*x=_T5E;struct Cyc_Absyn_Stmt*s1=_T62;_T4D=
# 1088
Cyc_Absynpp_is_declaration(s1);if(!_T4D)goto _TL138;
Cyc_Absyndump_dump_str(x);_T4E=expstmt;
if(!_T4E)goto _TL13A;_T4F=_tag_fat(": ({",sizeof(char),5U);Cyc_Absyndump_dump_nospace(_T4F);goto _TL13B;_TL13A: _T50=_tag_fat(": {",sizeof(char),4U);Cyc_Absyndump_dump_nospace(_T50);_TL13B: _T51=s1;_T52=expstmt;_T53=
Cyc_Absyndump_mt_set();Cyc_Absyndump_dumpstmt(_T51,_T52,_T53);_T54=expstmt;
if(!_T54)goto _TL13C;_T55=_tag_fat("});",sizeof(char),4U);Cyc_Absyndump_dump_nospace(_T55);goto _TL13D;_TL13C: Cyc_Absyndump_dump_char(125);_TL13D: goto _TL139;
# 1094
_TL138: Cyc_Absyndump_dump_str(x);Cyc_Absyndump_dump_char(58);
s=s1;goto LOOP;_TL139: goto _LL0;}default: _T56=(struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct*)_T5B;_T57=_T56->f1;if(_T57!=0)goto _TL13E;_T58=
# 1100
_tag_fat("fallthru;",sizeof(char),10U);Cyc_Absyndump_dump(_T58);goto _LL0;_TL13E:{struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct*_T64=(struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct*)_T5B;_T5D=_T64->f1;}{struct Cyc_List_List*es=_T5D;_T59=
# 1102
_tag_fat("fallthru(",sizeof(char),10U);Cyc_Absyndump_dump(_T59);Cyc_Absyndump_dumpexps_prec(20,es);_T5A=_tag_fat(");",sizeof(char),3U);Cyc_Absyndump_dump_nospace(_T5A);goto _LL0;}}_LL0:;}}struct _tuple21{struct Cyc_List_List*f0;struct Cyc_Absyn_Pat*f1;};
# 1106
static void Cyc_Absyndump_dumpdp(struct _tuple21*dp){struct _tuple21*_T0;struct _tuple21 _T1;struct Cyc_List_List*_T2;struct _fat_ptr _T3;struct _fat_ptr _T4;struct _fat_ptr _T5;struct _tuple21*_T6;struct _tuple21 _T7;struct Cyc_Absyn_Pat*_T8;_T0=dp;_T1=*_T0;_T2=_T1.f0;_T3=
_tag_fat("",sizeof(char),1U);_T4=_tag_fat("=",sizeof(char),2U);_T5=_tag_fat("=",sizeof(char),2U);Cyc_Absyndump_egroup(Cyc_Absyndump_dumpdesignator,_T2,_T3,_T4,_T5);_T6=dp;_T7=*_T6;_T8=_T7.f1;
Cyc_Absyndump_dumppat(_T8);}
# 1110
static struct _fat_ptr Cyc_Absyndump_pat_term(int dots){struct _fat_ptr _T0;int _T1;_T1=dots;if(!_T1)goto _TL140;_T0=_tag_fat("...)",sizeof(char),5U);goto _TL141;_TL140: _T0=_tag_fat(")",sizeof(char),2U);_TL141: return _T0;}
# 1112
static void Cyc_Absyndump_dumppat(struct Cyc_Absyn_Pat*p){struct Cyc_Absyn_Pat*_T0;int*_T1;unsigned _T2;struct _fat_ptr _T3;struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct*_T4;enum Cyc_Absyn_Sign _T5;int _T6;struct _fat_ptr _T7;struct Cyc_Int_pa_PrintArg_struct _T8;int _T9;void**_TA;struct _fat_ptr _TB;struct _fat_ptr _TC;struct _fat_ptr _TD;struct Cyc_Int_pa_PrintArg_struct _TE;int _TF;void**_T10;struct _fat_ptr _T11;struct _fat_ptr _T12;struct Cyc_Absyn_Enumfield*_T13;struct _tuple1*_T14;struct _fat_ptr _T15;struct _fat_ptr _T16;struct _fat_ptr _T17;struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct*_T18;struct Cyc_Absyn_Pat*_T19;struct Cyc_Absyn_Pat*_T1A;void*_T1B;int*_T1C;int _T1D;struct _fat_ptr _T1E;struct _fat_ptr _T1F;struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*_T20;struct Cyc_Absyn_Pat*_T21;struct Cyc_Absyn_Pat*_T22;void*_T23;int*_T24;int _T25;struct Cyc_Absyn_Vardecl*_T26;struct _tuple1*_T27;struct Cyc_Absyn_Vardecl*_T28;struct _tuple1*_T29;struct _fat_ptr _T2A;struct _fat_ptr _T2B;struct _fat_ptr _T2C;struct _fat_ptr _T2D;struct Cyc_Absyn_Vardecl*_T2E;struct Cyc_Absyn_Pat*_T2F;unsigned _T30;struct _fat_ptr _T31;struct Cyc_Absyn_Vardecl*_T32;struct _tuple1*_T33;void(*_T34)(void(*)(struct Cyc_Absyn_Pat*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr);struct Cyc_List_List*_T35;struct _fat_ptr _T36;struct _fat_ptr _T37;struct _fat_ptr _T38;void*_T39;int _T3A;void(*_T3B)(void(*)(struct Cyc_Absyn_Pat*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr);struct Cyc_List_List*(*_T3C)(struct Cyc_Absyn_Pat*(*)(struct _tuple21*),struct Cyc_List_List*);struct Cyc_List_List*(*_T3D)(void*(*)(void*),struct Cyc_List_List*);struct Cyc_Absyn_Pat*(*_T3E)(struct _tuple21*);void*(*_T3F)(struct _tuple0*);struct Cyc_List_List*_T40;struct Cyc_List_List*_T41;struct _fat_ptr _T42;struct _fat_ptr _T43;struct _fat_ptr _T44;int*_T45;int _T46;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T47;void*_T48;int*_T49;int _T4A;void*_T4B;struct _tuple12 _T4C;struct _tuple1*_T4D;void(*_T4E)(void(*)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr);struct Cyc_List_List*_T4F;struct _fat_ptr _T50;struct _fat_ptr _T51;struct _fat_ptr _T52;void(*_T53)(void(*)(struct _tuple21*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr);struct Cyc_List_List*_T54;struct _fat_ptr _T55;struct _fat_ptr _T56;struct _fat_ptr _T57;struct Cyc_Absyn_Datatypefield*_T58;struct _tuple1*_T59;void(*_T5A)(void(*)(struct Cyc_Absyn_Pat*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr);struct Cyc_List_List*_T5B;struct _fat_ptr _T5C;struct _fat_ptr _T5D;struct _fat_ptr _T5E;_T0=p;{
void*_T5F=_T0->r;struct Cyc_Absyn_Exp*_T60;struct Cyc_Absyn_Datatypefield*_T61;int _T62;struct Cyc_List_List*_T63;struct Cyc_List_List*_T64;struct Cyc_Absyn_Vardecl*_T65;struct Cyc_Absyn_Pat*_T66;char _T67;void*_T68;struct _fat_ptr _T69;int _T6A;_T1=(int*)_T5F;_T2=*_T1;switch(_T2){case 0:
 Cyc_Absyndump_dump_char(95);goto _LL0;case 8: _T3=
_tag_fat("NULL",sizeof(char),5U);Cyc_Absyndump_dump(_T3);goto _LL0;case 9: _T4=(struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct*)_T5F;_T5=_T4->f1;_T6=(int)_T5;switch(_T6){case Cyc_Absyn_None:{struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct*_T6B=(struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct*)_T5F;_T6A=_T6B->f2;}{int i=_T6A;_T6A=i;goto _LL8;}case Cyc_Absyn_Signed:{struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct*_T6B=(struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct*)_T5F;_T6A=_T6B->f2;}_LL8: {int i=_T6A;{struct Cyc_Int_pa_PrintArg_struct _T6B;_T6B.tag=1;_T9=i;
# 1117
_T6B.f1=(unsigned long)_T9;_T8=_T6B;}{struct Cyc_Int_pa_PrintArg_struct _T6B=_T8;void*_T6C[1];_TA=_T6C + 0;*_TA=& _T6B;_TB=_tag_fat("%d",sizeof(char),3U);_TC=_tag_fat(_T6C,sizeof(void*),1);_T7=Cyc_aprintf(_TB,_TC);}Cyc_Absyndump_dump(_T7);goto _LL0;}default:{struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct*_T6B=(struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct*)_T5F;_T6A=_T6B->f2;}{int i=_T6A;{struct Cyc_Int_pa_PrintArg_struct _T6B;_T6B.tag=1;_TF=i;
_T6B.f1=(unsigned)_TF;_TE=_T6B;}{struct Cyc_Int_pa_PrintArg_struct _T6B=_TE;void*_T6C[1];_T10=_T6C + 0;*_T10=& _T6B;_T11=_tag_fat("%u",sizeof(char),3U);_T12=_tag_fat(_T6C,sizeof(void*),1);_TD=Cyc_aprintf(_T11,_T12);}Cyc_Absyndump_dump(_TD);goto _LL0;}};case 11:{struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct*_T6B=(struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct*)_T5F;_T69=_T6B->f1;}{struct _fat_ptr x=_T69;
Cyc_Absyndump_dump(x);goto _LL0;}case 12:{struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct*_T6B=(struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct*)_T5F;_T68=_T6B->f2;}{struct Cyc_Absyn_Enumfield*ef=_T68;_T68=ef;goto _LL10;}case 13:{struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct*_T6B=(struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct*)_T5F;_T68=_T6B->f2;}_LL10: {struct Cyc_Absyn_Enumfield*ef=_T68;_T13=ef;_T14=_T13->name;
# 1121
Cyc_Absyndump_dumpqvar(_T14);goto _LL0;}case 10:{struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct*_T6B=(struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct*)_T5F;_T67=_T6B->f1;}{char ch=_T67;_T15=
# 1123
_tag_fat("'",sizeof(char),2U);Cyc_Absyndump_dump(_T15);_T16=Cyc_Absynpp_char_escape(ch);Cyc_Absyndump_dump_nospace(_T16);_T17=_tag_fat("'",sizeof(char),2U);Cyc_Absyndump_dump_nospace(_T17);goto _LL0;}case 3: _T18=(struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct*)_T5F;_T19=_T18->f2;_T1A=(struct Cyc_Absyn_Pat*)_T19;_T1B=_T1A->r;_T1C=(int*)_T1B;_T1D=*_T1C;if(_T1D!=0)goto _TL144;{struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct*_T6B=(struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct*)_T5F;_T68=_T6B->f1;}{struct Cyc_Absyn_Vardecl*vd=_T68;_T1E=
_tag_fat("*",sizeof(char),2U);Cyc_Absyndump_dump(_T1E);_T68=vd;goto _LL16;}_TL144:{struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct*_T6B=(struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct*)_T5F;_T68=_T6B->f1;_T66=_T6B->f2;}{struct Cyc_Absyn_Vardecl*vd=_T68;struct Cyc_Absyn_Pat*p2=_T66;_T1F=
# 1126
_tag_fat("*",sizeof(char),2U);Cyc_Absyndump_dump(_T1F);_T68=vd;_T66=p2;goto _LL1A;}case 1: _T20=(struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*)_T5F;_T21=_T20->f2;_T22=(struct Cyc_Absyn_Pat*)_T21;_T23=_T22->r;_T24=(int*)_T23;_T25=*_T24;if(_T25!=0)goto _TL146;{struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*_T6B=(struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*)_T5F;_T68=_T6B->f1;}_LL16:{struct Cyc_Absyn_Vardecl*vd=_T68;_T26=vd;_T27=_T26->name;
# 1125
Cyc_Absyndump_dumpqvar(_T27);goto _LL0;}_TL146:{struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*_T6B=(struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*)_T5F;_T68=_T6B->f1;_T66=_T6B->f2;}_LL1A: {struct Cyc_Absyn_Vardecl*vd=_T68;struct Cyc_Absyn_Pat*p2=_T66;_T28=vd;_T29=_T28->name;
# 1127
Cyc_Absyndump_dumpqvar(_T29);_T2A=_tag_fat(" as ",sizeof(char),5U);Cyc_Absyndump_dump(_T2A);Cyc_Absyndump_dumppat(p2);goto _LL0;}case 2:{struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct*_T6B=(struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct*)_T5F;_T68=_T6B->f1;_T65=_T6B->f2;}{struct Cyc_Absyn_Tvar*tv=_T68;struct Cyc_Absyn_Vardecl*vd=_T65;_T2B=
# 1129
_tag_fat("alias",sizeof(char),6U);Cyc_Absyndump_dump(_T2B);_T2C=_tag_fat("<",sizeof(char),2U);Cyc_Absyndump_dump(_T2C);Cyc_Absyndump_dumptvar(tv);_T2D=_tag_fat(">",sizeof(char),2U);Cyc_Absyndump_dump(_T2D);_T2E=vd;_T2F=p;_T30=_T2F->loc;
Cyc_Absyndump_dumpvardecl(_T2E,_T30,0);goto _LL0;}case 5:{struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct*_T6B=(struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct*)_T5F;_T68=_T6B->f1;}{struct Cyc_Absyn_Pat*p2=_T68;_T31=
# 1132
_tag_fat("&",sizeof(char),2U);Cyc_Absyndump_dump(_T31);Cyc_Absyndump_dumppat(p2);goto _LL0;}case 4:{struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct*_T6B=(struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct*)_T5F;_T68=_T6B->f1;_T65=_T6B->f2;}{struct Cyc_Absyn_Tvar*tv=_T68;struct Cyc_Absyn_Vardecl*vd=_T65;_T32=vd;_T33=_T32->name;
# 1134
Cyc_Absyndump_dumpqvar(_T33);Cyc_Absyndump_dump_char(60);Cyc_Absyndump_dumptvar(tv);Cyc_Absyndump_dump_char(62);goto _LL0;}case 14:{struct Cyc_Absyn_UnknownId_p_Absyn_Raw_pat_struct*_T6B=(struct Cyc_Absyn_UnknownId_p_Absyn_Raw_pat_struct*)_T5F;_T68=_T6B->f1;}{struct _tuple1*q=_T68;
Cyc_Absyndump_dumpqvar(q);goto _LL0;}case 15:{struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct*_T6B=(struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct*)_T5F;_T68=_T6B->f1;_T64=_T6B->f2;_T6A=_T6B->f3;}{struct _tuple1*q=_T68;struct Cyc_List_List*ps=_T64;int dots=_T6A;
# 1137
Cyc_Absyndump_dumpqvar(q);{void(*_T6B)(void(*)(struct Cyc_Absyn_Pat*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr)=(void(*)(void(*)(struct Cyc_Absyn_Pat*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr))Cyc_Absyndump_group;_T34=_T6B;}_T35=ps;_T36=_tag_fat("(",sizeof(char),2U);_T37=Cyc_Absyndump_pat_term(dots);_T38=_tag_fat(",",sizeof(char),2U);_T34(Cyc_Absyndump_dumppat,_T35,_T36,_T37,_T38);goto _LL0;}case 6:{struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*_T6B=(struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_T5F;_T39=_T6B->f1;_T68=(void*)_T39;_T6A=_T6B->f2;_T64=_T6B->f3;_T63=_T6B->f4;_T62=_T6B->f5;}{void*topt=_T68;int is_tuple=_T6A;struct Cyc_List_List*exists=_T64;struct Cyc_List_List*dps=_T63;int dots=_T62;_T3A=is_tuple;
# 1139
if(!_T3A)goto _TL148;{
void(*_T6B)(void(*)(struct Cyc_Absyn_Pat*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr)=(void(*)(void(*)(struct Cyc_Absyn_Pat*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr))Cyc_Absyndump_group;_T3B=_T6B;}_T3D=Cyc_List_map;{struct Cyc_List_List*(*_T6B)(struct Cyc_Absyn_Pat*(*)(struct _tuple21*),struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Pat*(*)(struct _tuple21*),struct Cyc_List_List*))_T3D;_T3C=_T6B;}_T3F=Cyc_Core_snd;{struct Cyc_Absyn_Pat*(*_T6B)(struct _tuple21*)=(struct Cyc_Absyn_Pat*(*)(struct _tuple21*))_T3F;_T3E=_T6B;}_T40=dps;_T41=_T3C(_T3E,_T40);_T42=_tag_fat("$(",sizeof(char),3U);_T43=Cyc_Absyndump_pat_term(dots);_T44=_tag_fat(",",sizeof(char),2U);_T3B(Cyc_Absyndump_dumppat,_T41,_T42,_T43,_T44);goto _TL149;
# 1142
_TL148: if(topt==0)goto _TL14A;{
void*_T6B=Cyc_Absyn_compress(topt);union Cyc_Absyn_AggrInfo _T6C;_T45=(int*)_T6B;_T46=*_T45;if(_T46!=0)goto _TL14C;_T47=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T6B;_T48=_T47->f1;_T49=(int*)_T48;_T4A=*_T49;if(_T4A!=24)goto _TL14E;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T6D=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T6B;_T4B=_T6D->f1;{struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*_T6E=(struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_T4B;_T6C=_T6E->f1;}}{union Cyc_Absyn_AggrInfo info=_T6C;_T4C=
# 1145
Cyc_Absyn_aggr_kinded_name(info);_T4D=_T4C.f1;Cyc_Absyndump_dumpqvar(_T4D);goto _LL2B;}_TL14E: goto _LL2E;_TL14C: _LL2E: goto _LL2B;_LL2B:;}goto _TL14B;_TL14A: _TL14B:
# 1149
 Cyc_Absyndump_dump_char(123);{
void(*_T6B)(void(*)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr)=(void(*)(void(*)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr))Cyc_Absyndump_egroup;_T4E=_T6B;}_T4F=exists;_T50=_tag_fat("<",sizeof(char),2U);_T51=_tag_fat(">",sizeof(char),2U);_T52=_tag_fat(",",sizeof(char),2U);_T4E(Cyc_Absyndump_dumptvar,_T4F,_T50,_T51,_T52);{
void(*_T6B)(void(*)(struct _tuple21*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr)=(void(*)(void(*)(struct _tuple21*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr))Cyc_Absyndump_group;_T53=_T6B;}_T54=dps;_T55=_tag_fat("",sizeof(char),1U);_T56=Cyc_Absyndump_pat_term(dots);_T57=_tag_fat(",",sizeof(char),2U);_T53(Cyc_Absyndump_dumpdp,_T54,_T55,_T56,_T57);_TL149: goto _LL0;}case 7:{struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct*_T6B=(struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct*)_T5F;_T61=_T6B->f2;_T64=_T6B->f3;_T6A=_T6B->f4;}{struct Cyc_Absyn_Datatypefield*tuf=_T61;struct Cyc_List_List*ps=_T64;int dots=_T6A;_T58=tuf;_T59=_T58->name;
# 1155
Cyc_Absyndump_dumpqvar(_T59);
if(ps==0)goto _TL150;{void(*_T6B)(void(*)(struct Cyc_Absyn_Pat*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr)=(void(*)(void(*)(struct Cyc_Absyn_Pat*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr))Cyc_Absyndump_group;_T5A=_T6B;}_T5B=ps;_T5C=_tag_fat("(",sizeof(char),2U);_T5D=Cyc_Absyndump_pat_term(dots);_T5E=_tag_fat(",",sizeof(char),2U);_T5A(Cyc_Absyndump_dumppat,_T5B,_T5C,_T5D,_T5E);goto _TL151;_TL150: _TL151: goto _LL0;}default:{struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct*_T6B=(struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct*)_T5F;_T60=_T6B->f1;}{struct Cyc_Absyn_Exp*e=_T60;
# 1158
Cyc_Absyndump_dumpexp(e);goto _LL0;}}_LL0:;}}
# 1162
static void Cyc_Absyndump_dumpdatatypefield(struct Cyc_Absyn_Datatypefield*ef){struct Cyc_Absyn_Datatypefield*_T0;struct _tuple1*_T1;struct Cyc_Absyn_Datatypefield*_T2;struct Cyc_List_List*_T3;struct Cyc_Absyn_Datatypefield*_T4;struct Cyc_List_List*_T5;_T0=ef;_T1=_T0->name;
Cyc_Absyndump_dumpqvar(_T1);_T2=ef;_T3=_T2->typs;
if(_T3==0)goto _TL152;_T4=ef;_T5=_T4->typs;
Cyc_Absyndump_dumpargs(_T5);goto _TL153;_TL152: _TL153:;}
# 1167
static void Cyc_Absyndump_dumpdatatypefields(struct Cyc_List_List*fields){void(*_T0)(void(*)(struct Cyc_Absyn_Datatypefield*),struct Cyc_List_List*,struct _fat_ptr);struct Cyc_List_List*_T1;struct _fat_ptr _T2;{
void(*_T3)(void(*)(struct Cyc_Absyn_Datatypefield*),struct Cyc_List_List*,struct _fat_ptr)=(void(*)(void(*)(struct Cyc_Absyn_Datatypefield*),struct Cyc_List_List*,struct _fat_ptr))Cyc_Absyndump_dump_sep;_T0=_T3;}_T1=fields;_T2=_tag_fat(",",sizeof(char),2U);_T0(Cyc_Absyndump_dumpdatatypefield,_T1,_T2);}
# 1170
static void Cyc_Absyndump_dumpenumfield(struct Cyc_Absyn_Enumfield*ef){struct Cyc_Absyn_Enumfield*_T0;struct _tuple1*_T1;struct Cyc_Absyn_Enumfield*_T2;struct Cyc_Absyn_Exp*_T3;struct _fat_ptr _T4;struct Cyc_Absyn_Enumfield*_T5;struct Cyc_Absyn_Exp*_T6;struct Cyc_Absyn_Exp*_T7;_T0=ef;_T1=_T0->name;
Cyc_Absyndump_dumpqvar(_T1);_T2=ef;_T3=_T2->tag;
if(_T3==0)goto _TL154;_T4=
_tag_fat("=",sizeof(char),2U);Cyc_Absyndump_dump(_T4);_T5=ef;_T6=_T5->tag;_T7=_check_null(_T6);Cyc_Absyndump_dumpexp(_T7);goto _TL155;_TL154: _TL155:;}
# 1176
static void Cyc_Absyndump_dumpenumfields(struct Cyc_List_List*fields){void(*_T0)(void(*)(struct Cyc_Absyn_Enumfield*),struct Cyc_List_List*,struct _fat_ptr);struct Cyc_List_List*_T1;struct _fat_ptr _T2;{
void(*_T3)(void(*)(struct Cyc_Absyn_Enumfield*),struct Cyc_List_List*,struct _fat_ptr)=(void(*)(void(*)(struct Cyc_Absyn_Enumfield*),struct Cyc_List_List*,struct _fat_ptr))Cyc_Absyndump_dump_sep;_T0=_T3;}_T1=fields;_T2=_tag_fat(",",sizeof(char),2U);_T0(Cyc_Absyndump_dumpenumfield,_T1,_T2);}
# 1180
static void Cyc_Absyndump_dumpaggrfields(struct Cyc_List_List*fields){struct Cyc_List_List*_T0;void*_T1;enum Cyc_Flags_C_Compilers _T2;void(*_T3)(struct Cyc_Absyn_Tqual,void*,void(*)(struct _fat_ptr*),struct _fat_ptr*);void(*_T4)(struct Cyc_Absyn_Tqual,void*,void(*)(void*),void*);struct Cyc_Absyn_Tqual _T5;void*_T6;struct _fat_ptr*_T7;void(*_T8)(struct Cyc_Absyn_Tqual,void*,void(*)(struct _fat_ptr*),struct _fat_ptr*);void(*_T9)(struct Cyc_Absyn_Tqual,void*,void(*)(void*),void*);struct Cyc_Absyn_Tqual _TA;void*_TB;struct _fat_ptr*_TC;struct Cyc_Absyn_Exp*_TD;unsigned _TE;struct _fat_ptr _TF;struct Cyc_List_List*_T10;
_TL159: if(fields!=0)goto _TL157;else{goto _TL158;}
_TL157: _T0=fields;_T1=_T0->hd;{struct Cyc_Absyn_Aggrfield*_T11=(struct Cyc_Absyn_Aggrfield*)_T1;struct Cyc_Absyn_Exp*_T12;struct Cyc_List_List*_T13;struct Cyc_Absyn_Exp*_T14;void*_T15;struct Cyc_Absyn_Tqual _T16;struct _fat_ptr*_T17;{struct Cyc_Absyn_Aggrfield _T18=*_T11;_T17=_T18.name;_T16=_T18.tq;_T15=_T18.type;_T14=_T18.width;_T13=_T18.attributes;_T12=_T18.requires_clause;}{struct _fat_ptr*name=_T17;struct Cyc_Absyn_Tqual tq=_T16;void*type=_T15;struct Cyc_Absyn_Exp*width=_T14;struct Cyc_List_List*atts=_T13;struct Cyc_Absyn_Exp*req=_T12;_T2=Cyc_Flags_c_compiler;if(_T2!=Cyc_Flags_Gcc_c)goto _TL15A;_T4=Cyc_Absyndump_dumptqtd;{
# 1184
void(*_T18)(struct Cyc_Absyn_Tqual,void*,void(*)(struct _fat_ptr*),struct _fat_ptr*)=(void(*)(struct Cyc_Absyn_Tqual,void*,void(*)(struct _fat_ptr*),struct _fat_ptr*))_T4;_T3=_T18;}_T5=tq;_T6=type;_T7=name;_T3(_T5,_T6,Cyc_Absyndump_dump_str,_T7);Cyc_Absyndump_dumpatts(atts);goto _LL3;_TL15A:
 Cyc_Absyndump_dumpatts(atts);_T9=Cyc_Absyndump_dumptqtd;{void(*_T18)(struct Cyc_Absyn_Tqual,void*,void(*)(struct _fat_ptr*),struct _fat_ptr*)=(void(*)(struct Cyc_Absyn_Tqual,void*,void(*)(struct _fat_ptr*),struct _fat_ptr*))_T9;_T8=_T18;}_TA=tq;_TB=type;_TC=name;_T8(_TA,_TB,Cyc_Absyndump_dump_str,_TC);goto _LL3;_LL3: _TD=req;_TE=(unsigned)_TD;
# 1187
if(!_TE)goto _TL15C;_TF=
_tag_fat("@requires ",sizeof(char),11U);Cyc_Absyndump_dump(_TF);Cyc_Absyndump_dumpexp(req);goto _TL15D;_TL15C: _TL15D:
# 1190
 if(width==0)goto _TL15E;
Cyc_Absyndump_dump_char(58);Cyc_Absyndump_dumpexp(width);goto _TL15F;_TL15E: _TL15F:
# 1193
 Cyc_Absyndump_dump_semi();}}_T10=fields;
# 1181
fields=_T10->tl;goto _TL159;_TL158:;}
# 1197
static void Cyc_Absyndump_dumptypedefname(struct Cyc_Absyn_Typedefdecl*td){struct Cyc_Absyn_Typedefdecl*_T0;struct _tuple1*_T1;struct Cyc_Absyn_Typedefdecl*_T2;struct Cyc_List_List*_T3;_T0=td;_T1=_T0->name;
Cyc_Absyndump_dumpqvar(_T1);_T2=td;_T3=_T2->tvs;
Cyc_Absyndump_dumptvars(_T3);}
# 1202
static void Cyc_Absyndump_dump_atts_qvar(struct Cyc_Absyn_Fndecl*fd){struct Cyc_Absyn_Fndecl*_T0;struct Cyc_Absyn_FnInfo _T1;struct Cyc_List_List*_T2;struct Cyc_Absyn_Fndecl*_T3;struct _tuple1*_T4;_T0=fd;_T1=_T0->i;_T2=_T1.attributes;
Cyc_Absyndump_dumpatts(_T2);_T3=fd;_T4=_T3->name;
Cyc_Absyndump_dumpqvar(_T4);}struct _tuple22{void*f0;struct _tuple1*f1;};
# 1206
static void Cyc_Absyndump_dump_callconv_qvar(struct _tuple22*pr){struct _tuple22*_T0;struct _tuple22 _T1;int*_T2;unsigned _T3;struct _fat_ptr _T4;struct _fat_ptr _T5;struct _fat_ptr _T6;struct _tuple22*_T7;struct _tuple22 _T8;struct _tuple1*_T9;_T0=pr;_T1=*_T0;{
void*_TA=_T1.f0;_T2=(int*)_TA;_T3=*_T2;switch(_T3){case 11: goto _LL0;case 1: _T4=
# 1209
_tag_fat("_stdcall",sizeof(char),9U);Cyc_Absyndump_dump(_T4);goto _LL0;case 2: _T5=
_tag_fat("_cdecl",sizeof(char),7U);Cyc_Absyndump_dump(_T5);goto _LL0;case 3: _T6=
_tag_fat("_fastcall",sizeof(char),10U);Cyc_Absyndump_dump(_T6);goto _LL0;default: goto _LL0;}_LL0:;}_T7=pr;_T8=*_T7;_T9=_T8.f1;
# 1214
Cyc_Absyndump_dumpqvar(_T9);}
# 1216
static void Cyc_Absyndump_dump_callconv_fdqvar(struct Cyc_Absyn_Fndecl*fd){struct Cyc_Absyn_Fndecl*_T0;struct Cyc_Absyn_FnInfo _T1;struct Cyc_List_List*_T2;struct Cyc_Absyn_Fndecl*_T3;struct _tuple1*_T4;_T0=fd;_T1=_T0->i;_T2=_T1.attributes;
Cyc_Absyndump_dump_callconv(_T2);_T3=fd;_T4=_T3->name;
Cyc_Absyndump_dumpqvar(_T4);}
# 1220
static void Cyc_Absyndump_dumpid(struct Cyc_Absyn_Vardecl*vd){struct Cyc_Absyn_Vardecl*_T0;struct _tuple1*_T1;_T0=vd;_T1=_T0->name;
Cyc_Absyndump_dumpqvar(_T1);}
# 1224
static int Cyc_Absyndump_is_char_ptr(void*t){void*_T0;int*_T1;unsigned _T2;void*_T3;void*_T4;void*_T5;int _T6;void*_T7;struct Cyc_Absyn_PtrInfo _T8;void*_T9;int*_TA;unsigned _TB;void*_TC;void*_TD;void*_TE;void*_TF;void*_T10;void*_T11;void*_T12;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T13;void*_T14;int*_T15;int _T16;void*_T17;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T18;void*_T19;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T1A;enum Cyc_Absyn_Size_of _T1B;void*_T1C;_T0=t;_T1=(int*)_T0;_T2=*_T1;switch(_T2){case 1: _T3=t;{struct Cyc_Absyn_Evar_Absyn_Type_struct*_T1D=(struct Cyc_Absyn_Evar_Absyn_Type_struct*)_T3;_T4=_T1D->f2;_T1C=(void*)_T4;}_T5=(void*)_T1C;if(_T5==0)goto _TL162;{void*def=_T1C;_T6=
# 1227
Cyc_Absyndump_is_char_ptr(def);return _T6;}_TL162: goto _LL5;case 4: _T7=t;{struct Cyc_Absyn_PointerType_Absyn_Type_struct*_T1D=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_T7;_T8=_T1D->f1;_T1C=_T8.elt_type;}{void*elt_typ=_T1C;
# 1229
_TL164: if(1)goto _TL165;else{goto _TL166;}
_TL165:{void*_T1D;_T9=elt_typ;_TA=(int*)_T9;_TB=*_TA;switch(_TB){case 1: _TC=elt_typ;{struct Cyc_Absyn_Evar_Absyn_Type_struct*_T1E=(struct Cyc_Absyn_Evar_Absyn_Type_struct*)_TC;_TD=_T1E->f2;_T1D=(void*)_TD;}_TE=(void*)_T1D;if(_TE==0)goto _TL168;{void*t=_T1D;
elt_typ=t;goto _LL7;}_TL168: goto _LLE;case 8: _TF=elt_typ;{struct Cyc_Absyn_TypedefType_Absyn_Type_struct*_T1E=(struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_TF;_T10=_T1E->f4;_T1D=(void*)_T10;}_T11=(void*)_T1D;if(_T11==0)goto _TL16A;{void*t=_T1D;
elt_typ=t;goto _LL7;}_TL16A: goto _LLE;case 0: _T12=elt_typ;_T13=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T12;_T14=_T13->f1;_T15=(int*)_T14;_T16=*_T15;if(_T16!=1)goto _TL16C;_T17=elt_typ;_T18=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T17;_T19=_T18->f1;_T1A=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T19;_T1B=_T1A->f2;if(_T1B!=Cyc_Absyn_Char_sz)goto _TL16E;
return 1;_TL16E: goto _LLE;_TL16C: goto _LLE;default: _LLE:
 return 0;}_LL7:;}goto _TL164;_TL166:;}default: _LL5:
# 1236
 return 0;};}
# 1240
static void Cyc_Absyndump_dumpvardecl(struct Cyc_Absyn_Vardecl*vd,unsigned loc,int do_semi){struct Cyc_Absyn_Vardecl*_T0;enum Cyc_Flags_C_Compilers _T1;enum Cyc_Absyn_Scope _T2;int _T3;int _T4;int*_T5;int _T6;int _T7;void(*_T8)(struct Cyc_Absyn_Tqual,void*,void(*)(struct _tuple1*),struct _tuple1*);void(*_T9)(struct Cyc_Absyn_Tqual,void*,void(*)(void*),void*);struct Cyc_Absyn_Tqual _TA;void*_TB;struct _tuple1*_TC;struct Cyc_Absyn_Unused_att_Absyn_Attribute_struct*_TD;struct Cyc_Absyn_Unused_att_Absyn_Attribute_struct*_TE;struct Cyc_List_List*_TF;int*_T10;int _T11;struct Cyc_List_List*_T12;int*_T13;unsigned _T14;struct Cyc_Absyn_Stdcall_att_Absyn_Attribute_struct*_T15;struct Cyc_Absyn_Stdcall_att_Absyn_Attribute_struct*_T16;struct Cyc_Absyn_Cdecl_att_Absyn_Attribute_struct*_T17;struct Cyc_Absyn_Cdecl_att_Absyn_Attribute_struct*_T18;struct Cyc_Absyn_Fastcall_att_Absyn_Attribute_struct*_T19;struct Cyc_Absyn_Fastcall_att_Absyn_Attribute_struct*_T1A;struct Cyc_List_List*_T1B;struct Cyc_List_List*_T1C;struct _tuple22 _T1D;void(*_T1E)(int,struct Cyc_List_List*,void(*)(struct _tuple22*),struct _tuple22*);void(*_T1F)(int,struct Cyc_List_List*,void(*)(void*),void*);int _T20;struct Cyc_List_List*_T21;struct _tuple22*_T22;int _T23;struct Cyc_Absyn_Exp*_T24;int _T25;struct Cyc_List_List*_T26;struct Cyc_Absyn_Exp*_T27;void*_T28;struct Cyc_Absyn_Tqual _T29;unsigned _T2A;struct _tuple1*_T2B;enum Cyc_Absyn_Scope _T2C;_T0=vd;{struct Cyc_Absyn_Vardecl _T2D=*_T0;_T2C=_T2D.sc;_T2B=_T2D.name;_T2A=_T2D.varloc;_T29=_T2D.tq;_T28=_T2D.type;_T27=_T2D.initializer;_T26=_T2D.attributes;_T25=_T2D.is_proto;_T24=_T2D.rename;}{enum Cyc_Absyn_Scope sc=_T2C;struct _tuple1*name=_T2B;unsigned varloc=_T2A;struct Cyc_Absyn_Tqual tq=_T29;void*type=_T28;struct Cyc_Absyn_Exp*initializer=_T27;struct Cyc_List_List*atts=_T26;int is_proto=_T25;struct Cyc_Absyn_Exp*rename=_T24;_T1=Cyc_Flags_c_compiler;if(_T1!=Cyc_Flags_Gcc_c)goto _TL170;_T2=sc;_T3=(int)_T2;
# 1245
if(_T3!=3)goto _TL172;_T4=Cyc_Absyndump_qvar_to_Cids;if(!_T4)goto _TL172;{
void*_T2D=Cyc_Absyn_compress(type);_T5=(int*)_T2D;_T6=*_T5;if(_T6!=6)goto _TL174;_T7=is_proto;
# 1248
if(_T7)goto _TL176;else{goto _TL178;}
_TL178: Cyc_Absyndump_dumpscope(sc);goto _TL177;_TL176: _TL177: goto _LL8;_TL174:
# 1251
 Cyc_Absyndump_dumpscope(sc);_LL8:;}goto _TL173;
# 1254
_TL172: Cyc_Absyndump_dumpscope(sc);_TL173: _T9=Cyc_Absyndump_dumptqtd;{
void(*_T2D)(struct Cyc_Absyn_Tqual,void*,void(*)(struct _tuple1*),struct _tuple1*)=(void(*)(struct Cyc_Absyn_Tqual,void*,void(*)(struct _tuple1*),struct _tuple1*))_T9;_T8=_T2D;}_TA=tq;_TB=type;_TC=name;_T8(_TA,_TB,Cyc_Absyndump_dumpqvar,_TC);
Cyc_Absyndump_dumpatts(atts);goto _LL3;_TL170:
# 1259
 Cyc_Absyndump_dumpatts(atts);
Cyc_Absyndump_dumpscope(sc);{struct _RegionHandle _T2D=_new_region(0U,"temp");struct _RegionHandle*temp=& _T2D;_push_region(temp);{
# 1262
int is_cp=Cyc_Absyndump_is_char_ptr(type);
struct _tuple13 _T2E=Cyc_Absynpp_to_tms(temp,tq,type);struct Cyc_List_List*_T2F;void*_T30;struct Cyc_Absyn_Tqual _T31;_T31=_T2E.f0;_T30=_T2E.f1;_T2F=_T2E.f2;{struct Cyc_Absyn_Tqual tq=_T31;void*t=_T30;struct Cyc_List_List*tms=_T2F;_TD=& Cyc_Atts_Unused_att_val;_TE=(struct Cyc_Absyn_Unused_att_Absyn_Attribute_struct*)_TD;{
# 1265
void*call_conv=(void*)_TE;{
struct Cyc_List_List*tms2=tms;_TL17C: if(tms2!=0)goto _TL17A;else{goto _TL17B;}
_TL17A: _TF=tms2;{void*_T32=_TF->hd;struct Cyc_List_List*_T33;_T10=(int*)_T32;_T11=*_T10;if(_T11!=5)goto _TL17D;{struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct*_T34=(struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct*)_T32;_T33=_T34->f2;}{struct Cyc_List_List*atts=_T33;
# 1269
_TL182: if(atts!=0)goto _TL180;else{goto _TL181;}
_TL180: _T12=atts;{void*_T34=_T12->hd;_T13=(int*)_T34;_T14=*_T13;switch(_T14){case 1: _T15=& Cyc_Atts_Stdcall_att_val;_T16=(struct Cyc_Absyn_Stdcall_att_Absyn_Attribute_struct*)_T15;
call_conv=(void*)_T16;goto _LL15;case 2: _T17=& Cyc_Atts_Cdecl_att_val;_T18=(struct Cyc_Absyn_Cdecl_att_Absyn_Attribute_struct*)_T17;
call_conv=(void*)_T18;goto _LL15;case 3: _T19=& Cyc_Atts_Fastcall_att_val;_T1A=(struct Cyc_Absyn_Fastcall_att_Absyn_Attribute_struct*)_T19;
call_conv=(void*)_T1A;goto _LL15;default: goto _LL15;}_LL15:;}_T1B=atts;
# 1269
atts=_T1B->tl;goto _TL182;_TL181: goto _LL10;}_TL17D: goto _LL10;_LL10:;}_T1C=tms2;
# 1266
tms2=_T1C->tl;goto _TL17C;_TL17B:;}
# 1279
Cyc_Absyndump_dumptq(tq);
Cyc_Absyndump_dumpntyp(t);{struct _tuple22 _T32;
_T32.f0=call_conv;_T32.f1=name;_T1D=_T32;}{struct _tuple22 pr=_T1D;_T1F=Cyc_Absyndump_dumptms;{
void(*_T32)(int,struct Cyc_List_List*,void(*)(struct _tuple22*),struct _tuple22*)=(void(*)(int,struct Cyc_List_List*,void(*)(struct _tuple22*),struct _tuple22*))_T1F;_T1E=_T32;}_T20=is_cp;_T21=Cyc_List_imp_rev(tms);_T22=& pr;_T1E(_T20,_T21,Cyc_Absyndump_dump_callconv_qvar,_T22);_npop_handler(0);goto _LL3;}}}}_pop_region();}_LL3:
# 1286
 if(rename==0)goto _TL184;Cyc_Absyndump_dumpexp(rename);goto _TL185;_TL184: _TL185:
# 1288
 if(initializer==0)goto _TL186;
Cyc_Absyndump_dump_char(61);Cyc_Absyndump_dumpexp(initializer);goto _TL187;_TL186: _TL187: _T23=do_semi;
# 1291
if(!_T23)goto _TL188;Cyc_Absyndump_dump_semi();goto _TL189;_TL188: _TL189:;}}
# 1294
static void Cyc_Absyndump_dump_aggrdecl(struct Cyc_Absyn_Aggrdecl*ad){struct Cyc_Absyn_Aggrdecl*_T0;enum Cyc_Absyn_Scope _T1;struct Cyc_Absyn_Aggrdecl*_T2;struct Cyc_Absyn_AggrdeclImpl*_T3;struct Cyc_Absyn_Aggrdecl*_T4;struct Cyc_Absyn_AggrdeclImpl*_T5;int _T6;struct _fat_ptr _T7;struct Cyc_Absyn_Aggrdecl*_T8;enum Cyc_Absyn_AggrKind _T9;struct Cyc_Absyn_Aggrdecl*_TA;struct _tuple1*_TB;struct Cyc_Absyn_Aggrdecl*_TC;struct Cyc_List_List*_TD;struct Cyc_Absyn_Aggrdecl*_TE;struct Cyc_Absyn_AggrdeclImpl*_TF;void(*_T10)(void(*)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr);struct Cyc_Absyn_Aggrdecl*_T11;struct Cyc_Absyn_AggrdeclImpl*_T12;struct Cyc_Absyn_AggrdeclImpl*_T13;struct Cyc_List_List*_T14;struct _fat_ptr _T15;struct _fat_ptr _T16;struct _fat_ptr _T17;struct Cyc_Absyn_Aggrdecl*_T18;struct Cyc_Absyn_AggrdeclImpl*_T19;struct Cyc_Absyn_AggrdeclImpl*_T1A;struct Cyc_List_List*_T1B;struct Cyc_Absyn_Aggrdecl*_T1C;struct Cyc_Absyn_AggrdeclImpl*_T1D;struct Cyc_Absyn_AggrdeclImpl*_T1E;struct Cyc_List_List*_T1F;struct Cyc_Absyn_Aggrdecl*_T20;struct Cyc_Absyn_AggrdeclImpl*_T21;struct Cyc_Absyn_AggrdeclImpl*_T22;struct Cyc_List_List*_T23;char _T24;struct Cyc_Absyn_Aggrdecl*_T25;struct Cyc_Absyn_AggrdeclImpl*_T26;struct Cyc_List_List*_T27;char _T28;int _T29;struct Cyc_Absyn_Aggrdecl*_T2A;struct Cyc_Absyn_AggrdeclImpl*_T2B;struct Cyc_Absyn_AggrdeclImpl*_T2C;struct Cyc_List_List*_T2D;struct Cyc_Absyn_Aggrdecl*_T2E;struct Cyc_Absyn_AggrdeclImpl*_T2F;struct Cyc_Absyn_AggrdeclImpl*_T30;struct Cyc_List_List*_T31;struct Cyc_Absyn_Aggrdecl*_T32;struct Cyc_List_List*_T33;_T0=ad;_T1=_T0->sc;
Cyc_Absyndump_dumpscope(_T1);_T2=ad;_T3=_T2->impl;
if(_T3==0)goto _TL18A;_T4=ad;_T5=_T4->impl;_T6=_T5->tagged;if(!_T6)goto _TL18A;_T7=
_tag_fat("@tagged ",sizeof(char),9U);Cyc_Absyndump_dump(_T7);goto _TL18B;_TL18A: _TL18B: _T8=ad;_T9=_T8->kind;
Cyc_Absyndump_dumpaggr_kind(_T9);_TA=ad;_TB=_TA->name;Cyc_Absyndump_dumpqvar(_TB);_TC=ad;_TD=_TC->tvs;Cyc_Absyndump_dumpkindedtvars(_TD);_TE=ad;_TF=_TE->impl;
if(_TF==0)goto _TL18C;
Cyc_Absyndump_dump_char(123);{
void(*_T34)(void(*)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr)=(void(*)(void(*)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr))Cyc_Absyndump_egroup;_T10=_T34;}_T11=ad;_T12=_T11->impl;_T13=_check_null(_T12);_T14=_T13->exist_vars;_T15=_tag_fat("<",sizeof(char),2U);_T16=_tag_fat(">",sizeof(char),2U);_T17=_tag_fat(",",sizeof(char),2U);_T10(Cyc_Absyndump_dumpkindedtvar,_T14,_T15,_T16,_T17);_T18=ad;_T19=_T18->impl;_T1A=
_check_null(_T19);_T1B=_T1A->effconstr;if(_T1B==0)goto _TL18E;
Cyc_Absyndump_dump_char(58);_T1C=ad;_T1D=_T1C->impl;_T1E=_check_null(_T1D);_T1F=_T1E->effconstr;Cyc_Absyndump_dump_effc(_T1F);goto _TL18F;_TL18E: _TL18F: _T20=ad;_T21=_T20->impl;_T22=
# 1305
_check_null(_T21);_T23=_T22->qual_bnd;if(_T23==0)goto _TL190;_T25=ad;_T26=_T25->impl;_T27=_T26->effconstr;
if(_T27!=0)goto _TL192;_T24=':';goto _TL193;_TL192: _T24=',';_TL193:{char sep=_T24;_T28=sep;_T29=(int)_T28;
Cyc_Absyndump_dump_char(_T29);_T2A=ad;_T2B=_T2A->impl;_T2C=_check_null(_T2B);_T2D=_T2C->qual_bnd;Cyc_Absyndump_dump_qualbnd(_T2D);}goto _TL191;_TL190: _TL191: _T2E=ad;_T2F=_T2E->impl;_T30=
# 1309
_check_null(_T2F);_T31=_T30->fields;Cyc_Absyndump_dumpaggrfields(_T31);
Cyc_Absyndump_dump_char(125);_T32=ad;_T33=_T32->attributes;
Cyc_Absyndump_dumpatts(_T33);goto _TL18D;_TL18C: _TL18D:;}
# 1314
static void Cyc_Absyndump_dump_enumdecl(struct Cyc_Absyn_Enumdecl*ed){struct Cyc_Absyn_Enumdecl*_T0;struct _fat_ptr _T1;struct Cyc_Core_Opt*_T2;void*_T3;struct Cyc_List_List*_T4;struct Cyc_Core_Opt*_T5;struct _tuple1*_T6;enum Cyc_Absyn_Scope _T7;_T0=ed;{struct Cyc_Absyn_Enumdecl _T8=*_T0;_T7=_T8.sc;_T6=_T8.name;_T5=_T8.fields;}{enum Cyc_Absyn_Scope sc=_T7;struct _tuple1*nm=_T6;struct Cyc_Core_Opt*fields=_T5;
# 1316
Cyc_Absyndump_dumpscope(sc);_T1=_tag_fat("enum ",sizeof(char),6U);Cyc_Absyndump_dump(_T1);Cyc_Absyndump_dumpqvar(nm);
if(fields==0)goto _TL194;
Cyc_Absyndump_dump_char(123);_T2=fields;_T3=_T2->v;_T4=(struct Cyc_List_List*)_T3;Cyc_Absyndump_dumpenumfields(_T4);Cyc_Absyndump_dump_char(125);goto _TL195;_TL194: _TL195:;}}
# 1321
static void Cyc_Absyndump_dump_datatypedecl(struct Cyc_Absyn_Datatypedecl*dd){struct Cyc_Absyn_Datatypedecl*_T0;int _T1;struct _fat_ptr _T2;struct _fat_ptr _T3;struct Cyc_Core_Opt*_T4;void*_T5;struct Cyc_List_List*_T6;int _T7;struct Cyc_Core_Opt*_T8;struct Cyc_List_List*_T9;struct _tuple1*_TA;enum Cyc_Absyn_Scope _TB;_T0=dd;{struct Cyc_Absyn_Datatypedecl _TC=*_T0;_TB=_TC.sc;_TA=_TC.name;_T9=_TC.tvs;_T8=_TC.fields;_T7=_TC.is_extensible;}{enum Cyc_Absyn_Scope sc=_TB;struct _tuple1*name=_TA;struct Cyc_List_List*tvs=_T9;struct Cyc_Core_Opt*fields=_T8;int is_x=_T7;
# 1323
Cyc_Absyndump_dumpscope(sc);_T1=is_x;
if(!_T1)goto _TL196;_T2=
_tag_fat("@extensible ",sizeof(char),13U);Cyc_Absyndump_dump(_T2);goto _TL197;_TL196: _TL197: _T3=
_tag_fat("datatype ",sizeof(char),10U);Cyc_Absyndump_dump(_T3);Cyc_Absyndump_dumpqvar(name);Cyc_Absyndump_dumptvars(tvs);
if(fields==0)goto _TL198;
Cyc_Absyndump_dump_char(123);_T4=fields;_T5=_T4->v;_T6=(struct Cyc_List_List*)_T5;Cyc_Absyndump_dumpdatatypefields(_T6);Cyc_Absyndump_dump_char(125);goto _TL199;_TL198: _TL199:;}}struct _tuple23{unsigned f0;struct _tuple1*f1;int f2;};
# 1332
static void Cyc_Absyndump_dumpexport(struct _tuple23*tup){struct _tuple23*_T0;struct _tuple23 _T1;struct _tuple1*_T2;_T0=tup;_T1=*_T0;_T2=_T1.f1;
Cyc_Absyndump_dumpqvar(_T2);}
# 1335
static void Cyc_Absyndump_dump_braced_decls(struct Cyc_List_List*tdl){struct Cyc_List_List*_T0;void*_T1;struct Cyc_Absyn_Decl*_T2;struct Cyc_List_List*_T3;
Cyc_Absyndump_dump_char(123);
_TL19D: if(tdl!=0)goto _TL19B;else{goto _TL19C;}
_TL19B: _T0=tdl;_T1=_T0->hd;_T2=(struct Cyc_Absyn_Decl*)_T1;Cyc_Absyndump_dumpdecl(_T2);_T3=tdl;
# 1337
tdl=_T3->tl;goto _TL19D;_TL19C:
# 1339
 Cyc_Absyndump_dump_char(125);}
# 1342
static void Cyc_Absyndump_dumpdecl(struct Cyc_Absyn_Decl*d){struct Cyc_Absyn_Decl*_T0;unsigned _T1;struct Cyc_Absyn_Decl*_T2;int*_T3;unsigned _T4;enum Cyc_Flags_C_Compilers _T5;struct Cyc_Absyn_Fndecl*_T6;struct Cyc_Absyn_FnInfo _T7;struct Cyc_List_List*_T8;struct Cyc_Absyn_Fndecl*_T9;int _TA;enum Cyc_Flags_C_Compilers _TB;struct _fat_ptr _TC;struct _fat_ptr _TD;struct Cyc_Absyn_Fndecl*_TE;enum Cyc_Absyn_Scope _TF;struct Cyc_Absyn_Fndecl*_T10;struct Cyc_Absyn_FnType_Absyn_Type_struct*_T11;struct Cyc_Absyn_Fndecl*_T12;void*_T13;struct Cyc_Absyn_Fndecl*_T14;void*_T15;int*_T16;int _T17;struct Cyc_Absyn_Fndecl*_T18;struct Cyc_Absyn_Fndecl*_T19;struct Cyc_Absyn_FnInfo _T1A;struct Cyc_List_List*_T1B;struct Cyc_Absyn_FnInfo _T1C;struct Cyc_List_List*_T1D;int(*_T1E)(struct _fat_ptr,struct _fat_ptr);void*(*_T1F)(struct _fat_ptr,struct _fat_ptr);struct _fat_ptr _T20;struct _fat_ptr _T21;enum Cyc_Flags_C_Compilers _T22;void(*_T23)(struct Cyc_Absyn_Tqual,void*,void(*)(struct Cyc_Absyn_Fndecl*),struct Cyc_Absyn_Fndecl*);void(*_T24)(struct Cyc_Absyn_Tqual,void*,void(*)(void*),void*);struct Cyc_Absyn_Tqual _T25;void*_T26;struct Cyc_Absyn_Fndecl*_T27;void(*_T28)(struct Cyc_Absyn_Tqual,void*,void(*)(struct Cyc_Absyn_Fndecl*),struct Cyc_Absyn_Fndecl*);void(*_T29)(struct Cyc_Absyn_Tqual,void*,void(*)(void*),void*);struct Cyc_Absyn_Tqual _T2A;void*_T2B;struct Cyc_Absyn_Fndecl*_T2C;struct Cyc_Absyn_Fndecl*_T2D;struct Cyc_Absyn_Stmt*_T2E;struct Cyc_Dict_Dict _T2F;struct Cyc_Absyn_Vardecl*_T30;struct Cyc_Absyn_Decl*_T31;unsigned _T32;void(*_T33)(void(*)(struct Cyc_Absyn_Vardecl*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr);struct Cyc_List_List*_T34;struct _fat_ptr _T35;struct _fat_ptr _T36;struct _fat_ptr _T37;struct _fat_ptr _T38;struct _fat_ptr _T39;struct _fat_ptr _T3A;struct _fat_ptr _T3B;struct Cyc_Absyn_Vardecl*_T3C;struct _tuple1*_T3D;struct Cyc_Absyn_Exp*_T3E;unsigned _T3F;struct _fat_ptr _T40;struct _fat_ptr _T41;struct _fat_ptr _T42;void*_T43;struct Cyc_Absyn_Typedefdecl*_T44;void*_T45;struct Cyc_Absyn_Typedefdecl*_T46;struct Cyc_Core_Opt*_T47;struct Cyc_Absyn_Typedefdecl*_T48;void(*_T49)(struct Cyc_Absyn_Tqual,void*,void(*)(struct Cyc_Absyn_Typedefdecl*),struct Cyc_Absyn_Typedefdecl*);void(*_T4A)(struct Cyc_Absyn_Tqual,void*,void(*)(void*),void*);struct Cyc_Absyn_Typedefdecl*_T4B;struct Cyc_Absyn_Tqual _T4C;void*_T4D;struct Cyc_Absyn_Typedefdecl*_T4E;struct Cyc_Absyn_Typedefdecl*_T4F;struct Cyc_List_List*_T50;struct _fat_ptr _T51;struct _fat_ptr _T52;struct _fat_ptr _T53;struct _fat_ptr _T54;struct _fat_ptr _T55;struct _tuple11*_T56;unsigned _T57;struct _fat_ptr _T58;struct _tuple11*_T59;struct Cyc_List_List*_T5A;unsigned _T5B;void(*_T5C)(void(*)(struct _tuple1*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr);struct Cyc_List_List*_T5D;struct _fat_ptr _T5E;struct _fat_ptr _T5F;struct _fat_ptr _T60;void(*_T61)(void(*)(struct _tuple23*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr);struct Cyc_List_List*_T62;struct _fat_ptr _T63;struct _fat_ptr _T64;struct _fat_ptr _T65;struct _fat_ptr _T66;struct _fat_ptr _T67;struct _fat_ptr _T68;struct _fat_ptr _T69;_T0=d;_T1=_T0->loc;
Cyc_Absyndump_dumploc(_T1);_T2=d;{
void*_T6A=_T2->r;struct _tuple11*_T6B;struct Cyc_List_List*_T6C;struct Cyc_List_List*_T6D;struct _tuple1*_T6E;struct _fat_ptr*_T6F;struct Cyc_Absyn_Typedefdecl*_T70;struct Cyc_Absyn_Tvar*_T71;struct Cyc_Absyn_Exp*_T72;struct Cyc_Absyn_Pat*_T73;struct Cyc_List_List*_T74;struct Cyc_Absyn_Enumdecl*_T75;struct Cyc_Absyn_Datatypedecl*_T76;struct Cyc_Absyn_Aggrdecl*_T77;struct Cyc_Absyn_Vardecl*_T78;struct Cyc_Absyn_Fndecl*_T79;_T3=(int*)_T6A;_T4=*_T3;switch(_T4){case 1:{struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*_T7A=(struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_T6A;_T79=_T7A->f1;}{struct Cyc_Absyn_Fndecl*fd=_T79;_T5=Cyc_Flags_c_compiler;if(_T5!=Cyc_Flags_Vc_c)goto _TL19F;_T6=fd;_T7=_T6->i;_T8=_T7.attributes;
# 1347
Cyc_Absyndump_dumpatts(_T8);goto _LL23;_TL19F: goto _LL23;_LL23: _T9=fd;_TA=_T9->is_inline;
# 1350
if(!_TA)goto _TL1A1;_TB=Cyc_Flags_c_compiler;if(_TB!=Cyc_Flags_Vc_c)goto _TL1A3;_TC=
# 1352
_tag_fat("__inline",sizeof(char),9U);Cyc_Absyndump_dump(_TC);goto _LL28;_TL1A3: _TD=
_tag_fat("inline",sizeof(char),7U);Cyc_Absyndump_dump(_TD);goto _LL28;_LL28: goto _TL1A2;_TL1A1: _TL1A2: _TE=fd;_TF=_TE->sc;
# 1355
Cyc_Absyndump_dumpscope(_TF);_T10=fd;{
# 1357
struct Cyc_Absyn_FnInfo type_info=_T10->i;
type_info.attributes=0;{struct Cyc_Absyn_FnType_Absyn_Type_struct*_T7A=_cycalloc(sizeof(struct Cyc_Absyn_FnType_Absyn_Type_struct));_T7A->tag=6;
_T7A->f1=type_info;_T11=(struct Cyc_Absyn_FnType_Absyn_Type_struct*)_T7A;}{void*t=(void*)_T11;_T12=fd;_T13=_T12->cached_type;
if(_T13==0)goto _TL1A5;_T14=fd;_T15=_T14->cached_type;{
void*_T7A=Cyc_Absyn_compress(_T15);struct Cyc_Absyn_FnInfo _T7B;_T16=(int*)_T7A;_T17=*_T16;if(_T17!=6)goto _TL1A7;{struct Cyc_Absyn_FnType_Absyn_Type_struct*_T7C=(struct Cyc_Absyn_FnType_Absyn_Type_struct*)_T7A;_T7B=_T7C->f1;}{struct Cyc_Absyn_FnInfo i=_T7B;_T18=fd;_T19=fd;_T1A=_T19->i;_T1B=_T1A.attributes;_T1C=i;_T1D=_T1C.attributes;
# 1363
_T18->i.attributes=Cyc_List_append(_T1B,_T1D);goto _LL2D;}_TL1A7: _T1F=Cyc_Warn_impos;{
int(*_T7C)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))_T1F;_T1E=_T7C;}_T20=_tag_fat("function has non-function type",sizeof(char),31U);_T21=_tag_fat(0U,sizeof(void*),0);_T1E(_T20,_T21);_LL2D:;}goto _TL1A6;_TL1A5: _TL1A6: _T22=Cyc_Flags_c_compiler;if(_T22!=Cyc_Flags_Gcc_c)goto _TL1A9;_T24=Cyc_Absyndump_dumptqtd;{
# 1368
void(*_T7A)(struct Cyc_Absyn_Tqual,void*,void(*)(struct Cyc_Absyn_Fndecl*),struct Cyc_Absyn_Fndecl*)=(void(*)(struct Cyc_Absyn_Tqual,void*,void(*)(struct Cyc_Absyn_Fndecl*),struct Cyc_Absyn_Fndecl*))_T24;_T23=_T7A;}_T25=Cyc_Absyn_empty_tqual(0U);_T26=t;_T27=fd;_T23(_T25,_T26,Cyc_Absyndump_dump_atts_qvar,_T27);goto _LL32;_TL1A9: _T29=Cyc_Absyndump_dumptqtd;{
void(*_T7A)(struct Cyc_Absyn_Tqual,void*,void(*)(struct Cyc_Absyn_Fndecl*),struct Cyc_Absyn_Fndecl*)=(void(*)(struct Cyc_Absyn_Tqual,void*,void(*)(struct Cyc_Absyn_Fndecl*),struct Cyc_Absyn_Fndecl*))_T29;_T28=_T7A;}_T2A=Cyc_Absyn_empty_tqual(0U);_T2B=t;_T2C=fd;_T28(_T2A,_T2B,Cyc_Absyndump_dump_callconv_fdqvar,_T2C);goto _LL32;_LL32:
# 1371
 Cyc_Absyndump_dump_char(123);_T2D=fd;_T2E=_T2D->body;_T2F=
Cyc_Absyndump_mt_set();Cyc_Absyndump_dumpstmt(_T2E,0,_T2F);
Cyc_Absyndump_dump_char(125);goto _LL0;}}}case 0:{struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_T7A=(struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_T6A;_T78=_T7A->f1;}{struct Cyc_Absyn_Vardecl*vd=_T78;_T30=vd;_T31=d;_T32=_T31->loc;
# 1375
Cyc_Absyndump_dumpvardecl(_T30,_T32,1);goto _LL0;}case 5:{struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*_T7A=(struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*)_T6A;_T77=_T7A->f1;}{struct Cyc_Absyn_Aggrdecl*ad=_T77;
Cyc_Absyndump_dump_aggrdecl(ad);Cyc_Absyndump_dump_semi();goto _LL0;}case 6:{struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct*_T7A=(struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct*)_T6A;_T76=_T7A->f1;}{struct Cyc_Absyn_Datatypedecl*dd=_T76;
Cyc_Absyndump_dump_datatypedecl(dd);Cyc_Absyndump_dump_semi();goto _LL0;}case 7:{struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*_T7A=(struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*)_T6A;_T75=_T7A->f1;}{struct Cyc_Absyn_Enumdecl*ed=_T75;
Cyc_Absyndump_dump_enumdecl(ed);Cyc_Absyndump_dump_semi();goto _LL0;}case 3:{struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct*_T7A=(struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct*)_T6A;_T74=_T7A->f1;}{struct Cyc_List_List*vds=_T74;{
void(*_T7A)(void(*)(struct Cyc_Absyn_Vardecl*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr)=(void(*)(void(*)(struct Cyc_Absyn_Vardecl*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr))Cyc_Absyndump_group;_T33=_T7A;}_T34=vds;_T35=_tag_fat(" let ",sizeof(char),6U);_T36=_tag_fat(";",sizeof(char),2U);_T37=_tag_fat(",",sizeof(char),2U);_T33(Cyc_Absyndump_dumpid,_T34,_T35,_T36,_T37);goto _LL0;}case 2:{struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*_T7A=(struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)_T6A;_T73=_T7A->f1;_T72=_T7A->f3;}{struct Cyc_Absyn_Pat*p=_T73;struct Cyc_Absyn_Exp*e=_T72;_T38=
# 1381
_tag_fat("let",sizeof(char),4U);Cyc_Absyndump_dump(_T38);Cyc_Absyndump_dumppat(p);Cyc_Absyndump_dump_char(61);Cyc_Absyndump_dumpexp(e);Cyc_Absyndump_dump_semi();goto _LL0;}case 4:{struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*_T7A=(struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_T6A;_T71=_T7A->f1;_T78=_T7A->f2;_T72=_T7A->f3;}{struct Cyc_Absyn_Tvar*tv=_T71;struct Cyc_Absyn_Vardecl*vd=_T78;struct Cyc_Absyn_Exp*open_exp_opt=_T72;_T39=
# 1383
_tag_fat("region",sizeof(char),7U);Cyc_Absyndump_dump(_T39);_T3A=_tag_fat("<",sizeof(char),2U);Cyc_Absyndump_dump(_T3A);Cyc_Absyndump_dumptvar(tv);_T3B=_tag_fat("> ",sizeof(char),3U);Cyc_Absyndump_dump(_T3B);_T3C=vd;_T3D=_T3C->name;Cyc_Absyndump_dumpqvar(_T3D);_T3E=open_exp_opt;_T3F=(unsigned)_T3E;
if(!_T3F)goto _TL1AB;_T40=
_tag_fat(" = open(",sizeof(char),9U);Cyc_Absyndump_dump(_T40);Cyc_Absyndump_dumpexp(open_exp_opt);_T41=_tag_fat(")",sizeof(char),2U);Cyc_Absyndump_dump(_T41);goto _TL1AC;_TL1AB: _TL1AC:
# 1387
 Cyc_Absyndump_dump_semi();goto _LL0;}case 8:{struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct*_T7A=(struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct*)_T6A;_T70=_T7A->f1;}{struct Cyc_Absyn_Typedefdecl*td=_T70;_T42=
# 1394
_tag_fat("typedef",sizeof(char),8U);Cyc_Absyndump_dump(_T42);_T44=td;_T45=_T44->defn;
if(_T45!=0)goto _TL1AD;_T46=td;_T47=_T46->kind;_T43=Cyc_Absyn_new_evar(_T47,0);goto _TL1AE;_TL1AD: _T48=td;_T43=_T48->defn;_TL1AE: {void*t=_T43;_T4A=Cyc_Absyndump_dumptqtd;{
void(*_T7A)(struct Cyc_Absyn_Tqual,void*,void(*)(struct Cyc_Absyn_Typedefdecl*),struct Cyc_Absyn_Typedefdecl*)=(void(*)(struct Cyc_Absyn_Tqual,void*,void(*)(struct Cyc_Absyn_Typedefdecl*),struct Cyc_Absyn_Typedefdecl*))_T4A;_T49=_T7A;}_T4B=td;_T4C=_T4B->tq;_T4D=t;_T4E=td;_T49(_T4C,_T4D,Cyc_Absyndump_dumptypedefname,_T4E);_T4F=td;_T50=_T4F->atts;
Cyc_Absyndump_dumpatts(_T50);
Cyc_Absyndump_dump_semi();goto _LL0;}}case 9:{struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*_T7A=(struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_T6A;_T6F=_T7A->f1;_T74=_T7A->f2;}{struct _fat_ptr*v=_T6F;struct Cyc_List_List*tdl=_T74;_T51=
# 1402
_tag_fat("namespace",sizeof(char),10U);Cyc_Absyndump_dump(_T51);Cyc_Absyndump_dump_str(v);Cyc_Absyndump_dump_braced_decls(tdl);goto _LL0;}case 10:{struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*_T7A=(struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*)_T6A;_T6E=_T7A->f1;_T74=_T7A->f2;}{struct _tuple1*q=_T6E;struct Cyc_List_List*tdl=_T74;_T52=
# 1404
_tag_fat("using",sizeof(char),6U);Cyc_Absyndump_dump(_T52);Cyc_Absyndump_dumpqvar(q);Cyc_Absyndump_dump_braced_decls(tdl);goto _LL0;}case 11:{struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct*_T7A=(struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct*)_T6A;_T74=_T7A->f1;}{struct Cyc_List_List*tdl=_T74;_T53=
# 1406
_tag_fat("extern \"C\" ",sizeof(char),12U);Cyc_Absyndump_dump(_T53);Cyc_Absyndump_dump_braced_decls(tdl);goto _LL0;}case 12:{struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*_T7A=(struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*)_T6A;_T74=_T7A->f1;_T6D=_T7A->f2;_T6C=_T7A->f3;_T6B=_T7A->f4;}{struct Cyc_List_List*tdl=_T74;struct Cyc_List_List*ovrs=_T6D;struct Cyc_List_List*exs=_T6C;struct _tuple11*wc=_T6B;_T54=
# 1408
_tag_fat("extern \"C include\" ",sizeof(char),20U);Cyc_Absyndump_dump(_T54);Cyc_Absyndump_dump_braced_decls(tdl);
if(ovrs==0)goto _TL1AF;_T55=
_tag_fat(" cyclone_override ",sizeof(char),19U);Cyc_Absyndump_dump(_T55);
Cyc_Absyndump_dump_braced_decls(ovrs);goto _TL1B0;_TL1AF: _TL1B0: _T56=wc;_T57=(unsigned)_T56;
# 1413
if(!_T57)goto _TL1B1;_T58=
_tag_fat(" export { * }\n",sizeof(char),15U);Cyc_Absyndump_dump(_T58);{struct Cyc_List_List*_T7A;_T59=wc;{struct _tuple11 _T7B=*_T59;_T7A=_T7B.f1;}{struct Cyc_List_List*hides=_T7A;_T5A=hides;_T5B=(unsigned)_T5A;
# 1416
if(!_T5B)goto _TL1B3;{
void(*_T7B)(void(*)(struct _tuple1*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr)=(void(*)(void(*)(struct _tuple1*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr))Cyc_Absyndump_egroup;_T5C=_T7B;}_T5D=hides;_T5E=_tag_fat(" cyclone_hide {",sizeof(char),16U);_T5F=_tag_fat("}",sizeof(char),2U);_T60=_tag_fat(",",sizeof(char),2U);_T5C(Cyc_Absyndump_dumpqvar,_T5D,_T5E,_T5F,_T60);goto _TL1B4;_TL1B3: _TL1B4:;}}goto _TL1B2;
# 1421
_TL1B1:{void(*_T7A)(void(*)(struct _tuple23*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr)=(void(*)(void(*)(struct _tuple23*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr))Cyc_Absyndump_egroup;_T61=_T7A;}_T62=exs;_T63=_tag_fat(" export {",sizeof(char),10U);_T64=_tag_fat("}",sizeof(char),2U);_T65=_tag_fat(",",sizeof(char),2U);_T61(Cyc_Absyndump_dumpexport,_T62,_T63,_T64,_T65);_TL1B2: goto _LL0;}case 13: _T66=
# 1424
_tag_fat(" __cyclone_port_on__; ",sizeof(char),23U);Cyc_Absyndump_dump(_T66);goto _LL0;case 14: _T67=
_tag_fat(" __cyclone_port_off__; ",sizeof(char),24U);Cyc_Absyndump_dump(_T67);goto _LL0;case 15: _T68=
_tag_fat(" __tempest_on__; ",sizeof(char),18U);Cyc_Absyndump_dump(_T68);goto _LL0;default: _T69=
_tag_fat(" __tempest_off__; ",sizeof(char),19U);Cyc_Absyndump_dump(_T69);goto _LL0;}_LL0:;}}
# 1431
static void Cyc_Absyndump_dump_upperbound(struct Cyc_Absyn_Exp*e){struct _tuple14 _T0;unsigned _T1;struct _tuple14 _T2;int _T3;
struct _tuple14 pr=Cyc_Evexp_eval_const_uint_exp(e);_T0=pr;_T1=_T0.f0;
if(_T1!=1U)goto _TL1B7;else{goto _TL1B8;}_TL1B8: _T2=pr;_T3=_T2.f1;if(_T3)goto _TL1B5;else{goto _TL1B7;}
_TL1B7: Cyc_Absyndump_dump_char(123);Cyc_Absyndump_dumpexp(e);Cyc_Absyndump_dump_char(125);goto _TL1B6;_TL1B5: _TL1B6:;}
# 1438
static void Cyc_Absyndump_dumptms(int is_char_ptr,struct Cyc_List_List*tms,void(*f)(void*),void*a){struct Cyc_List_List*_T0;int*_T1;int _T2;struct Cyc_Absyn_PtrAtts _T3;struct Cyc_Absyn_PtrAtts _T4;struct Cyc_Absyn_PtrAtts _T5;struct Cyc_Absyn_PtrAtts _T6;struct Cyc_Absyn_PtrAtts _T7;struct Cyc_Absyn_PtrAtts _T8;int*_T9;int _TA;struct Cyc_Absyn_AppType_Absyn_Type_struct*_TB;void*_TC;int*_TD;unsigned _TE;struct Cyc_Absyn_AppType_Absyn_Type_struct*_TF;struct Cyc_List_List*_T10;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T11;struct Cyc_List_List*_T12;struct Cyc_List_List*_T13;void*_T14;int*_T15;int _T16;struct Cyc_List_List*_T17;void*_T18;char _T19;int _T1A;int _T1B;char _T1C;int _T1D;int _T1E;int _T1F;int*_T20;unsigned _T21;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T22;void*_T23;int*_T24;unsigned _T25;void*_T26;enum Cyc_Absyn_AliasQualVal _T27;struct _fat_ptr _T28;struct _fat_ptr _T29;struct _fat_ptr _T2A;struct Cyc_List_List*_T2B;void*_T2C;struct _fat_ptr _T2D;struct _fat_ptr _T2E;struct _fat_ptr _T2F;struct Cyc_Warn_String_Warn_Warg_struct _T30;struct Cyc_Warn_Typ_Warn_Warg_struct _T31;void**_T32;void**_T33;struct _fat_ptr _T34;int _T35;int _T36;int _T37;struct _fat_ptr _T38;int _T39;int _T3A;int _T3B;struct _fat_ptr _T3C;int _T3D;int _T3E;struct _fat_ptr _T3F;int*_T40;unsigned _T41;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T42;void*_T43;int*_T44;unsigned _T45;int _T46;struct _fat_ptr _T47;struct _fat_ptr _T48;struct _fat_ptr _T49;struct Cyc_Absyn_Evar_Absyn_Type_struct*_T4A;void*_T4B;void*_T4C;struct _fat_ptr _T4D;struct _fat_ptr _T4E;struct _fat_ptr _T4F;struct _fat_ptr _T50;struct Cyc_List_List*_T51;struct Cyc_List_List*_T52;void(*_T53)(void*);void*_T54;struct Cyc_List_List*_T55;struct Cyc_List_List*_T56;struct Cyc_List_List*_T57;struct Cyc_List_List*_T58;int*_T59;int _T5A;int _T5B;struct Cyc_List_List*_T5C;struct Cyc_List_List*_T5D;void(*_T5E)(void*);void*_T5F;int _T60;struct Cyc_List_List*_T61;int*_T62;unsigned _T63;void*_T64;struct _fat_ptr _T65;int _T66;struct _fat_ptr _T67;void*_T68;int _T69;struct _fat_ptr _T6A;struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*_T6B;void*_T6C;int*_T6D;int _T6E;void*_T6F;void*_T70;void*_T71;void(*_T72)(void(*)(struct _fat_ptr*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr);struct Cyc_List_List*_T73;struct _fat_ptr _T74;struct _fat_ptr _T75;struct _fat_ptr _T76;int _T77;struct _fat_ptr _T78;struct _fat_ptr _T79;
# 1440
if(tms!=0)goto _TL1B9;
f(a);
return;_TL1B9: _T0=tms;{
# 1444
void*_T7A=_T0->hd;struct Cyc_Absyn_Tqual _T7B;void*_T7C;void*_T7D;void*_T7E;void*_T7F;void*_T80;void*_T81;_T1=(int*)_T7A;_T2=*_T1;if(_T2!=2)goto _TL1BB;{struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct*_T82=(struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct*)_T7A;_T3=_T82->f1;_T81=_T3.eff;_T4=_T82->f1;_T80=_T4.nullable;_T5=_T82->f1;_T7F=_T5.bounds;_T6=_T82->f1;_T7E=_T6.zero_term;_T7=_T82->f1;_T7D=_T7.autoreleased;_T8=_T82->f1;_T7C=_T8.aqual;_T7B=_T82->f2;}{void*rgn=_T81;void*nullable=_T80;void*bd=_T7F;void*zt=_T7E;void*rel=_T7D;void*aq=_T7C;struct Cyc_Absyn_Tqual tq2=_T7B;{
# 1448
void*_T82=Cyc_Absyn_compress(bd);struct Cyc_Absyn_Exp*_T83;_T9=(int*)_T82;_TA=*_T9;if(_TA!=0)goto _TL1BD;_TB=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T82;_TC=_TB->f1;_TD=(int*)_TC;_TE=*_TD;switch(_TE){case 14:
 Cyc_Absyndump_dump_char(63);goto _LL5;case 13: _TF=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T82;_T10=_TF->f2;if(_T10==0)goto _TL1C0;_T11=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T82;_T12=_T11->f2;_T13=(struct Cyc_List_List*)_T12;_T14=_T13->hd;_T15=(int*)_T14;_T16=*_T15;if(_T16!=9)goto _TL1C2;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T84=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T82;_T17=_T84->f2;{struct Cyc_List_List _T85=*_T17;_T18=_T85.hd;{struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_T86=(struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_T18;_T83=_T86->f1;}}}{struct Cyc_Absyn_Exp*e=_T83;_T1A=
# 1451
Cyc_Absyn_type2bool(1,nullable);if(!_T1A)goto _TL1C4;_T19='*';goto _TL1C5;_TL1C4: _T19='@';_TL1C5: _T1B=(int)_T19;Cyc_Absyndump_dump_char(_T1B);Cyc_Absyndump_dump_upperbound(e);goto _LL5;}_TL1C2: goto _LLA;_TL1C0: goto _LLA;default: goto _LLA;}goto _TL1BE;_TL1BD: _LLA: _T1D=
# 1453
Cyc_Absyn_type2bool(1,nullable);if(!_T1D)goto _TL1C6;_T1C='*';goto _TL1C7;_TL1C6: _T1C='@';_TL1C7: _T1E=(int)_T1C;Cyc_Absyndump_dump_char(_T1E);_TL1BE: _LL5:;}_T1F=Cyc_Absyndump_qvar_to_Cids;
# 1455
if(_T1F)goto _TL1C8;else{goto _TL1CA;}
_TL1CA:{void*_T82=Cyc_Absyn_compress(aq);struct Cyc_List_List*_T83;enum Cyc_Absyn_AliasQualVal _T84;_T20=(int*)_T82;_T21=*_T20;switch(_T21){case 0: _T22=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T82;_T23=_T22->f1;_T24=(int*)_T23;_T25=*_T24;switch(_T25){case 16:{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T85=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T82;_T26=_T85->f1;{struct Cyc_Absyn_AqualConstCon_Absyn_TyCon_struct*_T86=(struct Cyc_Absyn_AqualConstCon_Absyn_TyCon_struct*)_T26;_T84=_T86->f1;}}{enum Cyc_Absyn_AliasQualVal aqv=_T84;_T27=aqv;if(_T27!=Cyc_Absyn_Aliasable_qual)goto _TL1CD;goto _LL15;_TL1CD: _T28=
# 1461
_tag_fat("@aqual(",sizeof(char),8U);Cyc_Absyndump_dump(_T28);Cyc_Absyndump_dump_aqual_val(aqv);_T29=_tag_fat(")",sizeof(char),2U);Cyc_Absyndump_dump(_T29);goto _LL15;_LL15: goto _LLC;}case 17:{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T85=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T82;_T83=_T85->f2;}{struct Cyc_List_List*tv=_T83;_T2A=
# 1465
_tag_fat("@aqual(",sizeof(char),8U);Cyc_Absyndump_dump(_T2A);_T2B=_check_null(tv);_T2C=_T2B->hd;Cyc_Absyndump_dumpntyp(_T2C);_T2D=_tag_fat(")",sizeof(char),2U);Cyc_Absyndump_dump(_T2D);goto _LLC;}default: goto _LL13;};case 1: _T2E=
# 1468
_tag_fat("@aqual(",sizeof(char),8U);Cyc_Absyndump_dump(_T2E);Cyc_Absyndump_dumpntyp(aq);_T2F=_tag_fat(")",sizeof(char),2U);Cyc_Absyndump_dump(_T2F);goto _LLC;default: _LL13:{struct Cyc_Warn_String_Warn_Warg_struct _T85;_T85.tag=0;
# 1471
_T85.f1=_tag_fat("unexpected alias qualifier",sizeof(char),27U);_T30=_T85;}{struct Cyc_Warn_String_Warn_Warg_struct _T85=_T30;{struct Cyc_Warn_Typ_Warn_Warg_struct _T86;_T86.tag=2;_T86.f1=aq;_T31=_T86;}{struct Cyc_Warn_Typ_Warn_Warg_struct _T86=_T31;void*_T87[2];_T32=_T87 + 0;*_T32=& _T85;_T33=_T87 + 1;*_T33=& _T86;_T34=_tag_fat(_T87,sizeof(void*),2);Cyc_Warn_impos2(_T34);}}}_LLC:;}goto _TL1C9;_TL1C8: _TL1C9: _T35=Cyc_Absyndump_qvar_to_Cids;
# 1473
if(_T35)goto _TL1CF;else{goto _TL1D1;}_TL1D1: _T36=is_char_ptr;if(_T36)goto _TL1CF;else{goto _TL1D2;}_TL1D2: _T37=Cyc_Absyn_type2bool(0,zt);if(!_T37)goto _TL1CF;_T38=_tag_fat("@zeroterm",sizeof(char),10U);Cyc_Absyndump_dump(_T38);goto _TL1D0;_TL1CF: _TL1D0: _T39=Cyc_Absyndump_qvar_to_Cids;
if(_T39)goto _TL1D3;else{goto _TL1D5;}_TL1D5: _T3A=is_char_ptr;if(!_T3A)goto _TL1D3;_T3B=Cyc_Absyn_type2bool(0,zt);if(_T3B)goto _TL1D3;else{goto _TL1D6;}_TL1D6: _T3C=_tag_fat("@nozeroterm",sizeof(char),12U);Cyc_Absyndump_dump(_T3C);goto _TL1D4;_TL1D3: _TL1D4: _T3D=Cyc_Absyndump_qvar_to_Cids;
if(_T3D)goto _TL1D7;else{goto _TL1D9;}_TL1D9: _T3E=Cyc_Absyn_type2bool(0,rel);if(!_T3E)goto _TL1D7;_T3F=_tag_fat("@autoreleased",sizeof(char),14U);Cyc_Absyndump_dump(_T3F);goto _TL1D8;_TL1D7: _TL1D8:{
void*_T82=Cyc_Absyn_compress(rgn);struct Cyc_Absyn_Tvar*_T83;_T40=(int*)_T82;_T41=*_T40;switch(_T41){case 0: _T42=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T82;_T43=_T42->f1;_T44=(int*)_T43;_T45=*_T44;switch(_T45){case 6: _T46=Cyc_Absyndump_qvar_to_Cids;
if(_T46)goto _TL1DC;else{goto _TL1DE;}_TL1DE: _T47=_tag_fat("`H",sizeof(char),3U);Cyc_Absyndump_dump(_T47);goto _TL1DD;_TL1DC: _TL1DD: goto _LL1A;case 7: _T48=
_tag_fat("`U",sizeof(char),3U);Cyc_Absyndump_dump(_T48);goto _LL1A;case 8: _T49=
_tag_fat("`RC",sizeof(char),4U);Cyc_Absyndump_dump(_T49);goto _LL1A;default: goto _LL27;};case 2:{struct Cyc_Absyn_VarType_Absyn_Type_struct*_T84=(struct Cyc_Absyn_VarType_Absyn_Type_struct*)_T82;_T83=_T84->f1;}{struct Cyc_Absyn_Tvar*tv=_T83;
Cyc_Absyndump_dumptvar(tv);goto _LL1A;}case 1: _T4A=(struct Cyc_Absyn_Evar_Absyn_Type_struct*)_T82;_T4B=_T4A->f2;if(_T4B!=0)goto _TL1DF;_T4C=
Cyc_Absyn_compress(rgn);Cyc_Absyndump_dumpntyp(_T4C);goto _LL1A;_TL1DF: goto _LL27;case 8: _T4D=
_tag_fat("@region(",sizeof(char),9U);Cyc_Absyndump_dump(_T4D);Cyc_Absyndump_dumptyp(rgn);_T4E=_tag_fat(")",sizeof(char),2U);Cyc_Absyndump_dump(_T4E);goto _LL1A;default: _LL27: _T4F=
_tag_fat("dumptms: bad rgn type in Pointer_mod",sizeof(char),37U);_T50=_tag_fat(0U,sizeof(void*),0);Cyc_Warn_impos(_T4F,_T50);}_LL1A:;}
# 1485
Cyc_Absyndump_dumptq(tq2);_T51=tms;_T52=_T51->tl;_T53=f;_T54=a;
Cyc_Absyndump_dumptms(0,_T52,_T53,_T54);
return;}_TL1BB:{
# 1490
int next_is_pointer=0;_T55=tms;_T56=_T55->tl;
if(_T56==0)goto _TL1E1;_T57=tms;_T58=_T57->tl;{
void*_T82=_T58->hd;_T59=(int*)_T82;_T5A=*_T59;if(_T5A!=2)goto _TL1E3;
next_is_pointer=1;goto _LL29;_TL1E3: goto _LL29;_LL29:;}goto _TL1E2;_TL1E1: _TL1E2: _T5B=next_is_pointer;
# 1496
if(!_T5B)goto _TL1E5;
Cyc_Absyndump_dump_char(40);goto _TL1E6;_TL1E5: _TL1E6: _T5C=tms;_T5D=_T5C->tl;_T5E=f;_T5F=a;
Cyc_Absyndump_dumptms(0,_T5D,_T5E,_T5F);_T60=next_is_pointer;
if(!_T60)goto _TL1E7;
Cyc_Absyndump_dump_char(41);goto _TL1E8;_TL1E7: _TL1E8: _T61=tms;{
void*_T82=_T61->hd;struct Cyc_Absyn_Exp*_T83;struct Cyc_Absyn_Exp*_T84;struct Cyc_Absyn_Exp*_T85;struct Cyc_Absyn_Exp*_T86;struct Cyc_List_List*_T87;struct Cyc_List_List*_T88;struct Cyc_Absyn_VarargInfo*_T89;int _T8A;struct Cyc_List_List*_T8B;void*_T8C;void*_T8D;_T62=(int*)_T82;_T63=*_T62;switch(_T63){case 0:{struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct*_T8E=(struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct*)_T82;_T64=_T8E->f1;_T8D=(void*)_T64;}{void*zeroterm=_T8D;_T65=
# 1503
_tag_fat("[]",sizeof(char),3U);Cyc_Absyndump_dump(_T65);_T66=
Cyc_Absyn_type2bool(0,zeroterm);if(!_T66)goto _TL1EA;_T67=_tag_fat("@zeroterm",sizeof(char),10U);Cyc_Absyndump_dump(_T67);goto _TL1EB;_TL1EA: _TL1EB: goto _LL2E;}case 1:{struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct*_T8E=(struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct*)_T82;_T8D=_T8E->f1;_T68=_T8E->f2;_T8C=(void*)_T68;}{struct Cyc_Absyn_Exp*e=_T8D;void*zeroterm=_T8C;
# 1507
Cyc_Absyndump_dump_char(91);Cyc_Absyndump_dumpexp(e);Cyc_Absyndump_dump_char(93);_T69=
Cyc_Absyn_type2bool(0,zeroterm);if(!_T69)goto _TL1EC;_T6A=_tag_fat("@zeroterm",sizeof(char),10U);Cyc_Absyndump_dump(_T6A);goto _TL1ED;_TL1EC: _TL1ED: goto _LL2E;}case 3: _T6B=(struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*)_T82;_T6C=_T6B->f1;_T6D=(int*)_T6C;_T6E=*_T6D;if(_T6E!=1)goto _TL1EE;{struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*_T8E=(struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*)_T82;_T6F=_T8E->f1;{struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*_T8F=(struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)_T6F;_T8B=_T8F->f1;_T8A=_T8F->f2;_T89=_T8F->f3;_T70=_T8F->f4;_T8D=(void*)_T70;_T88=_T8F->f5;_T87=_T8F->f6;_T86=_T8F->f7;_T85=_T8F->f8;_T84=_T8F->f9;_T83=_T8F->f10;}}{struct Cyc_List_List*args=_T8B;int c_varargs=_T8A;struct Cyc_Absyn_VarargInfo*cyc_varargs=_T89;void*effopt=_T8D;struct Cyc_List_List*effc=_T88;struct Cyc_List_List*qb=_T87;struct Cyc_Absyn_Exp*chk=_T86;struct Cyc_Absyn_Exp*req=_T85;struct Cyc_Absyn_Exp*ens=_T84;struct Cyc_Absyn_Exp*thrws=_T83;
# 1511
Cyc_Absyndump_dumpfunargs(args,c_varargs,cyc_varargs,effopt,effc,qb,chk,req,ens,thrws);goto _LL2E;}_TL1EE:{struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*_T8E=(struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*)_T82;_T71=_T8E->f1;{struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct*_T8F=(struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct*)_T71;_T8B=_T8F->f1;}}{struct Cyc_List_List*sl=_T8B;{
# 1513
void(*_T8E)(void(*)(struct _fat_ptr*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr)=(void(*)(void(*)(struct _fat_ptr*),struct Cyc_List_List*,struct _fat_ptr,struct _fat_ptr,struct _fat_ptr))Cyc_Absyndump_group;_T72=_T8E;}_T73=sl;_T74=_tag_fat("(",sizeof(char),2U);_T75=_tag_fat(")",sizeof(char),2U);_T76=_tag_fat(",",sizeof(char),2U);_T72(Cyc_Absyndump_dump_str,_T73,_T74,_T75,_T76);goto _LL2E;}case 4:{struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct*_T8E=(struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct*)_T82;_T8B=_T8E->f1;_T8A=_T8E->f3;}{struct Cyc_List_List*ts=_T8B;int print_kinds=_T8A;_T77=print_kinds;
# 1515
if(!_T77)goto _TL1F0;Cyc_Absyndump_dumpkindedtvars(ts);goto _TL1F1;_TL1F0: Cyc_Absyndump_dumptvars(ts);_TL1F1: goto _LL2E;}case 5:{struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct*_T8E=(struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct*)_T82;_T8B=_T8E->f2;}{struct Cyc_List_List*atts=_T8B;
Cyc_Absyndump_dumpatts(atts);goto _LL2E;}default: _T78=
_tag_fat("dumptms",sizeof(char),8U);_T79=_tag_fat(0U,sizeof(void*),0);Cyc_Warn_impos(_T78,_T79);}_LL2E:;}
# 1519
return;};}}
# 1523
static void Cyc_Absyndump_dumptqtd(struct Cyc_Absyn_Tqual tq,void*t,void(*f)(void*),void*a){int _T0;struct Cyc_List_List*_T1;void(*_T2)(void*);void*_T3;
int cp=Cyc_Absyndump_is_char_ptr(t);struct _RegionHandle _T4=_new_region(0U,"temp");struct _RegionHandle*temp=& _T4;_push_region(temp);{
# 1526
struct _tuple13 _T5=Cyc_Absynpp_to_tms(temp,tq,t);struct Cyc_List_List*_T6;void*_T7;struct Cyc_Absyn_Tqual _T8;_T8=_T5.f0;_T7=_T5.f1;_T6=_T5.f2;{struct Cyc_Absyn_Tqual tq=_T8;void*t=_T7;struct Cyc_List_List*tms=_T6;
Cyc_Absyndump_dumptq(tq);
Cyc_Absyndump_dumpntyp(t);_T0=cp;_T1=
Cyc_List_imp_rev(tms);_T2=f;_T3=a;Cyc_Absyndump_dumptms(_T0,_T1,_T2,_T3);}}_pop_region();}
# 1532
void Cyc_Absyndump_dumpdecllist2file(struct Cyc_List_List*tdl,struct Cyc___cycFILE*f){struct Cyc___cycFILE**_T0;struct Cyc_List_List*_T1;void*_T2;struct Cyc_Absyn_Decl*_T3;struct Cyc_List_List*_T4;struct Cyc___cycFILE*_T5;struct _fat_ptr _T6;struct _fat_ptr _T7;_T0=Cyc_Absyndump_dump_file;
# 1534
*_T0=f;
_TL1F5: if(tdl!=0)goto _TL1F3;else{goto _TL1F4;}
_TL1F3: _T1=tdl;_T2=_T1->hd;_T3=(struct Cyc_Absyn_Decl*)_T2;Cyc_Absyndump_dumpdecl(_T3);_T4=tdl;
# 1535
tdl=_T4->tl;goto _TL1F5;_TL1F4: _T5=f;_T6=
# 1537
_tag_fat("\n",sizeof(char),2U);_T7=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T5,_T6,_T7);}
# 1540
static void Cyc_Absyndump_dump_decl_interface(struct Cyc_Absyn_Decl*d){struct Cyc_Absyn_Decl*_T0;int*_T1;unsigned _T2;struct Cyc_Absyn_Vardecl*_T3;enum Cyc_Absyn_Scope _T4;int _T5;struct Cyc_Absyn_Vardecl*_T6;struct Cyc_Absyn_Vardecl*_T7;struct Cyc_Absyn_Vardecl*_T8;enum Cyc_Absyn_Scope _T9;int _TA;struct _fat_ptr _TB;struct Cyc_Absyn_Vardecl*_TC;int _TD;struct Cyc_Absyn_Vardecl*_TE;void*_TF;int*_T10;int _T11;struct _fat_ptr _T12;struct Cyc_Absyn_Vardecl*_T13;struct Cyc_Absyn_Decl*_T14;unsigned _T15;struct _fat_ptr _T16;struct Cyc_Absyn_Vardecl*_T17;struct Cyc_Absyn_Fndecl*_T18;enum Cyc_Absyn_Scope _T19;int _T1A;struct Cyc_Absyn_Fndecl*_T1B;enum Cyc_Absyn_Scope _T1C;struct Cyc_Absyn_Fndecl*_T1D;struct Cyc_Absyn_FnType_Absyn_Type_struct*_T1E;void(*_T1F)(struct Cyc_Absyn_Tqual,void*,void(*)(struct Cyc_Absyn_Fndecl*),struct Cyc_Absyn_Fndecl*);struct Cyc_Absyn_Tqual _T20;void*_T21;struct Cyc_Absyn_Fndecl*_T22;struct _fat_ptr _T23;struct Cyc_Absyn_Aggrdecl*_T24;enum Cyc_Absyn_Scope _T25;int _T26;struct Cyc_Absyn_Aggrdecl*_T27;struct Cyc_Absyn_Aggrdecl*_T28;enum Cyc_Absyn_Scope _T29;int _T2A;struct Cyc_Absyn_Aggrdecl*_T2B;struct Cyc_Absyn_Aggrdecl*_T2C;struct _fat_ptr _T2D;struct _fat_ptr _T2E;struct _fat_ptr _T2F;struct Cyc_Absyn_Typedefdecl*_T30;void*_T31;struct _fat_ptr _T32;struct _fat_ptr _T33;struct _fat_ptr _T34;struct Cyc_List_List*_T35;void*_T36;struct Cyc_Absyn_Decl*_T37;struct Cyc_List_List*_T38;struct _fat_ptr _T39;struct Cyc_List_List*_T3A;void*_T3B;struct Cyc_Absyn_Decl*_T3C;struct Cyc_List_List*_T3D;struct _fat_ptr _T3E;struct Cyc_List_List*_T3F;void*_T40;struct Cyc_Absyn_Decl*_T41;struct Cyc_List_List*_T42;struct _fat_ptr _T43;int(*_T44)(struct _fat_ptr,struct _fat_ptr);void*(*_T45)(struct _fat_ptr,struct _fat_ptr);struct _fat_ptr _T46;struct _fat_ptr _T47;_T0=d;{
void*_T48=_T0->r;struct _tuple11*_T49;struct Cyc_List_List*_T4A;struct Cyc_List_List*_T4B;struct Cyc_List_List*_T4C;struct _fat_ptr*_T4D;struct Cyc_Absyn_Typedefdecl*_T4E;struct Cyc_Absyn_Enumdecl*_T4F;struct Cyc_Absyn_Datatypedecl*_T50;struct Cyc_Absyn_Aggrdecl*_T51;struct Cyc_Absyn_Fndecl*_T52;struct Cyc_Absyn_Vardecl*_T53;_T1=(int*)_T48;_T2=*_T1;switch(_T2){case 0:{struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_T54=(struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_T48;_T53=_T54->f1;}{struct Cyc_Absyn_Vardecl*vd=_T53;_T3=vd;_T4=_T3->sc;_T5=(int)_T4;
# 1543
if(_T5!=0)goto _TL1F7;return;_TL1F7: _T6=vd;{
struct Cyc_Absyn_Exp*init=_T6->initializer;_T7=vd;
_T7->initializer=0;_T8=vd;_T9=_T8->sc;_TA=(int)_T9;
if(_TA!=2)goto _TL1F9;_TB=
_tag_fat("extern ",sizeof(char),8U);Cyc_Absyndump_dump(_TB);goto _TL1FA;
_TL1F9: _TC=vd;_TD=_TC->is_proto;if(_TD)goto _TL1FB;else{goto _TL1FD;}
_TL1FD: _TE=vd;_TF=_TE->type;{void*_T54=Cyc_Absyn_compress(_TF);_T10=(int*)_T54;_T11=*_T10;if(_T11!=6)goto _TL1FE;_T12=
_tag_fat("extern ",sizeof(char),8U);Cyc_Absyndump_dump(_T12);goto _LL17;_TL1FE: goto _LL17;_LL17:;}goto _TL1FC;_TL1FB: _TL1FC: _TL1FA: _T13=vd;_T14=d;_T15=_T14->loc;
# 1553
Cyc_Absyndump_dumpvardecl(_T13,_T15,1);_T16=
_tag_fat("\n",sizeof(char),2U);Cyc_Absyndump_dump(_T16);_T17=vd;
_T17->initializer=init;goto _LL0;}}case 1:{struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*_T54=(struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_T48;_T52=_T54->f1;}{struct Cyc_Absyn_Fndecl*fd=_T52;_T18=fd;_T19=_T18->sc;_T1A=(int)_T19;
# 1558
if(_T1A!=0)goto _TL200;return;_TL200: _T1B=fd;_T1C=_T1B->sc;
Cyc_Absyndump_dumpscope(_T1C);_T1D=fd;{
struct Cyc_Absyn_FnInfo type_info=_T1D->i;
type_info.attributes=0;{struct Cyc_Absyn_FnType_Absyn_Type_struct*_T54=_cycalloc(sizeof(struct Cyc_Absyn_FnType_Absyn_Type_struct));_T54->tag=6;
_T54->f1=type_info;_T1E=(struct Cyc_Absyn_FnType_Absyn_Type_struct*)_T54;}{void*t=(void*)_T1E;{
void(*_T54)(struct Cyc_Absyn_Tqual,void*,void(*)(struct Cyc_Absyn_Fndecl*),struct Cyc_Absyn_Fndecl*)=(void(*)(struct Cyc_Absyn_Tqual,void*,void(*)(struct Cyc_Absyn_Fndecl*),struct Cyc_Absyn_Fndecl*))Cyc_Absyndump_dumptqtd;_T1F=_T54;}_T20=Cyc_Absyn_empty_tqual(0U);_T21=t;_T22=fd;_T1F(_T20,_T21,Cyc_Absyndump_dump_atts_qvar,_T22);_T23=
_tag_fat(";\n",sizeof(char),3U);Cyc_Absyndump_dump(_T23);goto _LL0;}}}case 5:{struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*_T54=(struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*)_T48;_T51=_T54->f1;}{struct Cyc_Absyn_Aggrdecl*ad=_T51;_T24=ad;_T25=_T24->sc;_T26=(int)_T25;
# 1567
if(_T26!=0)goto _TL202;return;_TL202: _T27=ad;{
struct Cyc_Absyn_AggrdeclImpl*impl=_T27->impl;_T28=ad;_T29=_T28->sc;_T2A=(int)_T29;
if(_T2A!=1)goto _TL204;_T2B=ad;_T2B->impl=0;goto _TL205;_TL204: _TL205:
 Cyc_Absyndump_dump_aggrdecl(ad);_T2C=ad;
_T2C->impl=impl;_T2D=
_tag_fat(";\n",sizeof(char),3U);Cyc_Absyndump_dump(_T2D);goto _LL0;}}case 6:{struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct*_T54=(struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct*)_T48;_T50=_T54->f1;}{struct Cyc_Absyn_Datatypedecl*dd=_T50;
# 1574
Cyc_Absyndump_dump_datatypedecl(dd);_T2E=_tag_fat(";\n",sizeof(char),3U);Cyc_Absyndump_dump(_T2E);goto _LL0;}case 7:{struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*_T54=(struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*)_T48;_T4F=_T54->f1;}{struct Cyc_Absyn_Enumdecl*ed=_T4F;
Cyc_Absyndump_dump_enumdecl(ed);_T2F=_tag_fat(";\n",sizeof(char),3U);Cyc_Absyndump_dump(_T2F);goto _LL0;}case 8:{struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct*_T54=(struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct*)_T48;_T4E=_T54->f1;}{struct Cyc_Absyn_Typedefdecl*td=_T4E;_T30=td;_T31=_T30->defn;
# 1577
if(_T31!=0)goto _TL206;
Cyc_Absyndump_dumpdecl(d);_T32=
_tag_fat("\n",sizeof(char),2U);Cyc_Absyndump_dump(_T32);goto _TL207;_TL206: _TL207: goto _LL0;}case 9:{struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*_T54=(struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_T48;_T4D=_T54->f1;_T4C=_T54->f2;}{struct _fat_ptr*v=_T4D;struct Cyc_List_List*ds=_T4C;_T33=
# 1583
_tag_fat("namespace ",sizeof(char),11U);Cyc_Absyndump_dump(_T33);
Cyc_Absyndump_dump_str(v);_T34=
_tag_fat("{\n",sizeof(char),3U);Cyc_Absyndump_dump(_T34);
_TL20B: if(ds!=0)goto _TL209;else{goto _TL20A;}
_TL209: _T35=ds;_T36=_T35->hd;_T37=(struct Cyc_Absyn_Decl*)_T36;Cyc_Absyndump_dump_decl_interface(_T37);_T38=ds;
# 1586
ds=_T38->tl;goto _TL20B;_TL20A: _T39=
# 1588
_tag_fat("}\n",sizeof(char),3U);Cyc_Absyndump_dump(_T39);goto _LL0;}case 10:{struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*_T54=(struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*)_T48;_T4C=_T54->f2;}{struct Cyc_List_List*ds=_T4C;
# 1591
_TL20F: if(ds!=0)goto _TL20D;else{goto _TL20E;}
_TL20D: _T3A=ds;_T3B=_T3A->hd;_T3C=(struct Cyc_Absyn_Decl*)_T3B;Cyc_Absyndump_dump_decl_interface(_T3C);_T3D=ds;
# 1591
ds=_T3D->tl;goto _TL20F;_TL20E: goto _LL0;}case 11:{struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct*_T54=(struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct*)_T48;_T4C=_T54->f1;}{struct Cyc_List_List*ds=_T4C;_T3E=
# 1595
_tag_fat("extern \"C\" {",sizeof(char),13U);Cyc_Absyndump_dump(_T3E);
_TL213: if(ds!=0)goto _TL211;else{goto _TL212;}
_TL211: _T3F=ds;_T40=_T3F->hd;_T41=(struct Cyc_Absyn_Decl*)_T40;Cyc_Absyndump_dump_decl_interface(_T41);_T42=ds;
# 1596
ds=_T42->tl;goto _TL213;_TL212: _T43=
# 1598
_tag_fat("}\n",sizeof(char),3U);Cyc_Absyndump_dump(_T43);goto _LL0;}case 12:{struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*_T54=(struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*)_T48;_T4C=_T54->f1;_T4B=_T54->f2;_T4A=_T54->f3;_T49=_T54->f4;}{struct Cyc_List_List*ds=_T4C;struct Cyc_List_List*ovrs=_T4B;struct Cyc_List_List*exs=_T4A;struct _tuple11*wc=_T49;goto _LL0;}default: _T45=Cyc_Warn_impos;{
# 1617 "absyndump.cyc"
int(*_T54)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))_T45;_T44=_T54;}_T46=_tag_fat("bad top-level declaration",sizeof(char),26U);_T47=_tag_fat(0U,sizeof(void*),0);_T44(_T46,_T47);}_LL0:;}}
# 1621
void Cyc_Absyndump_dump_interface(struct Cyc_List_List*ds,struct Cyc___cycFILE*f){struct Cyc___cycFILE**_T0;struct Cyc_List_List*_T1;void*_T2;struct Cyc_Absyn_Decl*_T3;struct Cyc_List_List*_T4;_T0=Cyc_Absyndump_dump_file;
*_T0=f;
_TL217: if(ds!=0)goto _TL215;else{goto _TL216;}
_TL215: _T1=ds;_T2=_T1->hd;_T3=(struct Cyc_Absyn_Decl*)_T2;Cyc_Absyndump_dump_decl_interface(_T3);_T4=ds;
# 1623
ds=_T4->tl;goto _TL217;_TL216:;}
