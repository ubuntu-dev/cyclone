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
 struct Cyc___cycFILE;
# 53 "cycboot.h"
extern struct Cyc___cycFILE*Cyc_stderr;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};
# 73
extern struct _fat_ptr Cyc_aprintf(struct _fat_ptr,struct _fat_ptr);
# 79
extern int Cyc_fclose(struct Cyc___cycFILE*);
# 98
extern struct Cyc___cycFILE*Cyc_fopen(const char*,const char*);
# 100
extern int Cyc_fprintf(struct Cyc___cycFILE*,struct _fat_ptr,struct _fat_ptr);
# 142 "cycboot.h"
extern int Cyc_getc(struct Cyc___cycFILE*);
# 222 "cycboot.h"
extern int Cyc_ungetc(int,struct Cyc___cycFILE*);extern char Cyc_Core_Failure[8U];struct Cyc_Core_Failure_exn_struct{char*tag;struct _fat_ptr f1;};
# 321 "core.h"
void Cyc_Core_rethrow(void*);struct Cyc_Hashtable_Table;
# 39 "hashtable.h"
extern struct Cyc_Hashtable_Table*Cyc_Hashtable_create(int,int(*)(void*,void*),int(*)(void*));
# 50
extern void Cyc_Hashtable_insert(struct Cyc_Hashtable_Table*,void*,void*);
# 59
extern void**Cyc_Hashtable_lookup_other_opt(struct Cyc_Hashtable_Table*,void*,int(*)(void*,void*),int(*)(void*));
# 82
extern int Cyc_Hashtable_hash_string(struct _fat_ptr);struct Cyc_Sexp_Class;struct Cyc_Sexp_Obj;struct Cyc_Sexp_Object;struct Cyc_Sexp_Visitor;struct Cyc_Sexp_Parser{void*env;int(*getc)(void*);int(*ungetc)(int,void*);void(*error)(void*,int,struct _fat_ptr);};struct Cyc_Sexp_Printer{void*env;void(*print)(void*,struct _fat_ptr);};struct Cyc_Sexp_Class{struct Cyc_Sexp_Obj*cast_value;char tag;struct _fat_ptr name;void(*print)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Printer*);struct Cyc_Sexp_Obj*(*parse)(struct Cyc_Sexp_Parser*);int(*cmp)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Obj*);int(*hash)(struct Cyc_Sexp_Obj*);void*(*accept)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Visitor*,void*);struct Cyc_Hashtable_Table*hash_table;};struct Cyc_Sexp_Obj{struct Cyc_Sexp_Class*vtable;void*v[0U] __attribute__((aligned )) ;};struct Cyc_Sexp_Object{struct Cyc_Sexp_Obj*self;};
# 107 "sexp.h"
extern struct Cyc_Sexp_Class Cyc_Sexp_uchar_class;
extern struct Cyc_Sexp_Class Cyc_Sexp_schar_class;
extern struct Cyc_Sexp_Class Cyc_Sexp_ushort_class;
extern struct Cyc_Sexp_Class Cyc_Sexp_sshort_class;
extern struct Cyc_Sexp_Class Cyc_Sexp_uint_class;
extern struct Cyc_Sexp_Class Cyc_Sexp_sint_class;
extern struct Cyc_Sexp_Class Cyc_Sexp_ulonglong_class;
extern struct Cyc_Sexp_Class Cyc_Sexp_slonglong_class;
extern struct Cyc_Sexp_Class Cyc_Sexp_float_class;
extern struct Cyc_Sexp_Class Cyc_Sexp_double_class;
extern struct Cyc_Sexp_Class Cyc_Sexp_str_class;
extern struct Cyc_Sexp_Class Cyc_Sexp_symbol_class;
extern struct Cyc_Sexp_Class Cyc_Sexp_tuple_class;struct _tuple0{struct Cyc_Sexp_Class*vtable;unsigned char v  __attribute__((aligned )) ;};
# 122
struct _tuple0*Cyc_Sexp_mk_uchar(unsigned char);struct _tuple1{struct Cyc_Sexp_Class*vtable;signed char v  __attribute__((aligned )) ;};
struct _tuple1*Cyc_Sexp_mk_schar(signed char);struct _tuple2{struct Cyc_Sexp_Class*vtable;unsigned short v  __attribute__((aligned )) ;};
struct _tuple2*Cyc_Sexp_mk_ushort(unsigned short);struct _tuple3{struct Cyc_Sexp_Class*vtable;short v  __attribute__((aligned )) ;};
struct _tuple3*Cyc_Sexp_mk_sshort(short);struct _tuple4{struct Cyc_Sexp_Class*vtable;unsigned v  __attribute__((aligned )) ;};
struct _tuple4*Cyc_Sexp_mk_uint(unsigned);struct _tuple5{struct Cyc_Sexp_Class*vtable;int v  __attribute__((aligned )) ;};
struct _tuple5*Cyc_Sexp_mk_sint(int);struct _tuple6{struct Cyc_Sexp_Class*vtable;unsigned long long v  __attribute__((aligned )) ;};
struct _tuple6*Cyc_Sexp_mk_ulonglong(unsigned long long);struct _tuple7{struct Cyc_Sexp_Class*vtable;long long v  __attribute__((aligned )) ;};
struct _tuple7*Cyc_Sexp_mk_slonglong(long long);struct _tuple8{struct Cyc_Sexp_Class*vtable;float v  __attribute__((aligned )) ;};
struct _tuple8*Cyc_Sexp_mk_float(float);struct _tuple9{struct Cyc_Sexp_Class*vtable;double v  __attribute__((aligned )) ;};
struct _tuple9*Cyc_Sexp_mk_double(double);struct _tuple10{struct Cyc_Sexp_Class*vtable;struct _fat_ptr v  __attribute__((aligned )) ;};
struct _tuple10*Cyc_Sexp_mk_str(struct _fat_ptr);
struct _tuple10*Cyc_Sexp_mk_symbol(struct _fat_ptr);
struct _tuple10*Cyc_Sexp_mk_tuple(struct _fat_ptr);struct Cyc_Sexp_Visitor{void*(*visit_uchar)(void*,struct _tuple0*,struct Cyc_Sexp_Visitor*);void*(*visit_schar)(void*,struct _tuple1*,struct Cyc_Sexp_Visitor*);void*(*visit_ushort)(void*,struct _tuple2*,struct Cyc_Sexp_Visitor*);void*(*visit_sshort)(void*,struct _tuple3*,struct Cyc_Sexp_Visitor*);void*(*visit_uint)(void*,struct _tuple4*,struct Cyc_Sexp_Visitor*);void*(*visit_sint)(void*,struct _tuple5*,struct Cyc_Sexp_Visitor*);void*(*visit_ulonglong)(void*,struct _tuple6*,struct Cyc_Sexp_Visitor*);void*(*visit_slonglong)(void*,struct _tuple7*,struct Cyc_Sexp_Visitor*);void*(*visit_float)(void*,struct _tuple8*,struct Cyc_Sexp_Visitor*);void*(*visit_double)(void*,struct _tuple9*,struct Cyc_Sexp_Visitor*);void*(*visit_symbol)(void*,struct _tuple10*,struct Cyc_Sexp_Visitor*);void*(*visit_str)(void*,struct _tuple10*,struct Cyc_Sexp_Visitor*);void*(*visit_tuple)(void*,struct _tuple10*,struct Cyc_Sexp_Visitor*);void*(*visit_default)(void*,struct Cyc_Sexp_Object,struct Cyc_Sexp_Visitor*);};struct Cyc_Xarray_Xarray{struct _fat_ptr elmts;int num_elmts;};
# 54 "xarray.h"
extern struct Cyc_Xarray_Xarray*Cyc_Xarray_create_empty (void);
# 66
extern void Cyc_Xarray_add(struct Cyc_Xarray_Xarray*,void*);
# 104
extern void Cyc_Xarray_iter_c(void(*)(void*,void*),void*,struct Cyc_Xarray_Xarray*);struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 61 "list.h"
extern int Cyc_List_length(struct Cyc_List_List*);
# 178
extern struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*);
# 38 "string.h"
extern unsigned long Cyc_strlen(struct _fat_ptr);
# 49 "string.h"
extern int Cyc_strcmp(struct _fat_ptr,struct _fat_ptr);
# 55 "sexp.cyc"
struct Cyc_Sexp_Object Cyc_Sexp_up(struct Cyc_Sexp_Obj*self){struct Cyc_Sexp_Object _T0;{struct Cyc_Sexp_Object _T1;
_T1.self=self;_T0=_T1;}return _T0;}
# 60
struct Cyc_Sexp_Obj*Cyc_Sexp_down(struct Cyc_Sexp_Class*B,struct Cyc_Sexp_Object x){struct Cyc_Sexp_Object _T0;struct Cyc_Sexp_Class*_T1;struct Cyc_Sexp_Obj*_T2;struct Cyc_Sexp_Class*_T3;struct Cyc_Sexp_Class*_T4;struct Cyc_Sexp_Obj*_T5;struct Cyc_Sexp_Obj*_T6;_T0=x;_T6=_T0.self;{struct Cyc_Sexp_Obj*self=_T6;_T1=B;
# 62
_T1->cast_value=0;_T2=self;_T3=_T2->vtable;
_T3->cast_value=self;_T4=B;_T5=_T4->cast_value;
return _T5;}}
# 68
int Cyc_Sexp_hash(struct Cyc_Sexp_Object self){struct Cyc_Sexp_Object _T0;struct Cyc_Sexp_Obj*_T1;struct Cyc_Sexp_Class*_T2;int(*_T3)(struct Cyc_Sexp_Obj*);int _T4;struct Cyc_Sexp_Obj*_T5;_T0=self;_T5=_T0.self;{struct Cyc_Sexp_Obj*self=_T5;_T1=self;_T2=_T1->vtable;_T3=_T2->hash;_T4=
# 70
_T3(self);return _T4;}}
# 74
int Cyc_Sexp_cmp(struct Cyc_Sexp_Object x,struct Cyc_Sexp_Object y){struct Cyc_Sexp_Object _T0;struct Cyc_Sexp_Object _T1;struct Cyc_Sexp_Obj*_T2;void*_T3;struct Cyc_Sexp_Obj*_T4;void*_T5;struct Cyc_Sexp_Obj*_T6;struct Cyc_Sexp_Class*_T7;int _T8;struct Cyc_Sexp_Obj*_T9;struct Cyc_Sexp_Class*_TA;int _TB;int _TC;struct Cyc_Sexp_Obj*_TD;struct Cyc_Sexp_Class*_TE;struct Cyc_Sexp_Object _TF;struct Cyc_Sexp_Obj*_T10;struct Cyc_Sexp_Class*_T11;int(*_T12)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Obj*);struct Cyc_Sexp_Obj*_T13;struct Cyc_Sexp_Obj*_T14;int _T15;struct Cyc_Sexp_Obj*_T16;_T0=x;_T16=_T0.self;{struct Cyc_Sexp_Obj*xself=_T16;struct Cyc_Sexp_Obj*_T17;_T1=y;_T17=_T1.self;{struct Cyc_Sexp_Obj*yself=_T17;_T2=xself;_T3=(void*)_T2;_T4=yself;_T5=(void*)_T4;
# 77
if(_T3!=_T5)goto _TL0;return 0;_TL0: _T6=xself;_T7=_T6->vtable;_T8=(int)_T7;_T9=yself;_TA=_T9->vtable;_TB=(int)_TA;{
# 79
int diff=_T8 - _TB;
if(diff==0)goto _TL2;_TC=diff;return _TC;_TL2: _TD=xself;_TE=_TD->vtable;_TF=y;{
# 82
struct Cyc_Sexp_Obj*yasx=Cyc_Sexp_down(_TE,_TF);_T10=xself;_T11=_T10->vtable;_T12=_T11->cmp;_T13=xself;_T14=
_check_null(yasx);_T15=_T12(_T13,_T14);return _T15;}}}}}
# 87
void*Cyc_Sexp_visit(struct Cyc_Sexp_Object x,struct Cyc_Sexp_Visitor*v,void*env){struct Cyc_Sexp_Object _T0;struct Cyc_Sexp_Obj*_T1;struct Cyc_Sexp_Class*_T2;void*(*_T3)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Visitor*,void*);void*_T4;struct Cyc_Sexp_Obj*_T5;_T0=x;_T5=_T0.self;{struct Cyc_Sexp_Obj*xself=_T5;_T1=xself;_T2=_T1->vtable;_T3=_T2->accept;_T4=
# 89
_T3(xself,v,env);return _T4;}}
# 93
void Cyc_Sexp_print(struct Cyc_Sexp_Printer*p,struct Cyc_Sexp_Object x){struct Cyc_Sexp_Object _T0;struct Cyc_Sexp_Printer*_T1;void(*_T2)(void*,struct _fat_ptr);struct Cyc_Sexp_Printer*_T3;void*_T4;struct _fat_ptr _T5;struct Cyc_Int_pa_PrintArg_struct _T6;struct Cyc_Sexp_Obj*_T7;struct Cyc_Sexp_Class*_T8;char _T9;int _TA;void**_TB;struct _fat_ptr _TC;struct _fat_ptr _TD;struct Cyc_Sexp_Obj*_TE;struct Cyc_Sexp_Class*_TF;void(*_T10)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Printer*);struct Cyc_Sexp_Printer*_T11;void(*_T12)(void*,struct _fat_ptr);struct Cyc_Sexp_Printer*_T13;void*_T14;struct _fat_ptr _T15;struct Cyc_Sexp_Obj*_T16;_T0=x;_T16=_T0.self;{struct Cyc_Sexp_Obj*xself=_T16;_T1=p;_T2=_T1->print;_T3=p;_T4=_T3->env;{struct Cyc_Int_pa_PrintArg_struct _T17;_T17.tag=1;_T7=xself;_T8=_T7->vtable;_T9=_T8->tag;_TA=(int)_T9;
# 95
_T17.f1=(unsigned long)_TA;_T6=_T17;}{struct Cyc_Int_pa_PrintArg_struct _T17=_T6;void*_T18[1];_TB=_T18 + 0;*_TB=& _T17;_TC=_tag_fat("%c(",sizeof(char),4U);_TD=_tag_fat(_T18,sizeof(void*),1);_T5=Cyc_aprintf(_TC,_TD);}_T2(_T4,_T5);_TE=xself;_TF=_TE->vtable;_T10=_TF->print;
_T10(xself,p);_T11=p;_T12=_T11->print;_T13=p;_T14=_T13->env;_T15=
_tag_fat(")",sizeof(char),2U);_T12(_T14,_T15);}}
# 102
static void Cyc_Sexp_printfile(struct Cyc___cycFILE*f,struct _fat_ptr s){struct Cyc_String_pa_PrintArg_struct _T0;void**_T1;struct Cyc___cycFILE*_T2;struct _fat_ptr _T3;struct _fat_ptr _T4;{struct Cyc_String_pa_PrintArg_struct _T5;_T5.tag=0;
_T5.f1=s;_T0=_T5;}{struct Cyc_String_pa_PrintArg_struct _T5=_T0;void*_T6[1];_T1=_T6 + 0;*_T1=& _T5;_T2=f;_T3=_tag_fat("%s",sizeof(char),3U);_T4=_tag_fat(_T6,sizeof(void*),1);Cyc_fprintf(_T2,_T3,_T4);}}
# 107
void Cyc_Sexp_tofile(struct Cyc___cycFILE*f,struct Cyc_Sexp_Object x){struct Cyc_Sexp_Printer _T0;struct Cyc_Sexp_Printer*_T1;struct Cyc_Sexp_Printer*_T2;struct Cyc_Sexp_Object _T3;{struct Cyc_Sexp_Printer _T4;
_T4.env=f;_T4.print=Cyc_Sexp_printfile;_T0=_T4;}{struct Cyc_Sexp_Printer p=_T0;_T1=& p;_T2=(struct Cyc_Sexp_Printer*)_T1;_T3=x;
Cyc_Sexp_print(_T2,_T3);}}
# 113
void Cyc_Sexp_tofilename(const char*filename,struct Cyc_Sexp_Object x){struct Cyc_String_pa_PrintArg_struct _T0;struct _fat_ptr _T1;void*_T2;void*_T3;unsigned _T4;void**_T5;struct Cyc___cycFILE*_T6;struct _fat_ptr _T7;struct _fat_ptr _T8;struct Cyc___cycFILE*_T9;struct Cyc_Sexp_Object _TA;
struct Cyc___cycFILE*fopt=Cyc_fopen(filename,"w");
if(fopt!=0)goto _TL4;{struct Cyc_String_pa_PrintArg_struct _TB;_TB.tag=0;{const char*_TC=filename;_T2=(void*)_TC;_T3=(void*)_TC;_T4=_get_zero_arr_size_char(_T3,1U);_T1=_tag_fat(_T2,sizeof(char),_T4);}_TB.f1=_T1;_T0=_TB;}{struct Cyc_String_pa_PrintArg_struct _TB=_T0;void*_TC[1];_T5=_TC + 0;*_T5=& _TB;_T6=Cyc_stderr;_T7=_tag_fat("unable to open file %s\n",sizeof(char),24U);_T8=_tag_fat(_TC,sizeof(void*),1);Cyc_fprintf(_T6,_T7,_T8);}goto _TL5;_TL4: _TL5: _T9=
_check_null(fopt);_TA=x;Cyc_Sexp_tofile(_T9,_TA);
Cyc_fclose(fopt);}
# 121
void Cyc_Sexp_printstring(struct Cyc_Xarray_Xarray*strings,struct _fat_ptr s){void(*_T0)(struct Cyc_Xarray_Xarray*,const char*);void(*_T1)(struct Cyc_Xarray_Xarray*,void*);struct Cyc_Xarray_Xarray*_T2;struct _fat_ptr _T3;char*_T4;char*_T5;const char*_T6;_T1=Cyc_Xarray_add;{
# 123
void(*_T7)(struct Cyc_Xarray_Xarray*,const char*)=(void(*)(struct Cyc_Xarray_Xarray*,const char*))_T1;_T0=_T7;}_T2=strings;_T3=s;_T4=_untag_fat_ptr_check_bound(_T3,sizeof(char),1U);_T5=_check_null(_T4);_T6=(const char*)_T5;_T0(_T2,_T6);}
# 127
static void Cyc_Sexp_addlength(int*c,const char*s){const char*_T0;char _T1;int _T2;int*_T3;int*_T4;int _T5;const char*_T6;int _T7;
_TL9: _T0=s;_T1=*_T0;_T2=(int)_T1;if(_T2!=0)goto _TL7;else{goto _TL8;}_TL7: _T3=c;_T4=c;_T5=*_T4;*_T3=_T5 + 1;{const char**_T8=& s;_T6=*_T8;_T7=*_T6;if(_T7==0)goto _TLA;*_T8=*_T8 + 1;goto _TLB;_TLA: _throw_arraybounds();_TLB:;}goto _TL9;_TL8:;}
# 131
static void Cyc_Sexp_addstring(struct _fat_ptr*p,const char*s){const char*_T0;char _T1;int _T2;struct _fat_ptr*_T3;char*_T4;char*_T5;const char*_T6;unsigned _T7;unsigned char*_T8;char*_T9;struct _fat_ptr*_TA;struct _fat_ptr*_TB;struct _fat_ptr _TC;const char*_TD;int _TE;
_TLF: _T0=s;_T1=*_T0;_T2=(int)_T1;if(_T2!=0)goto _TLD;else{goto _TLE;}
_TLD: _T3=p;{struct _fat_ptr _TF=*_T3;_T4=_check_fat_subscript(_TF,sizeof(char),0U);_T5=(char*)_T4;{char _T10=*_T5;_T6=s;{char _T11=*_T6;_T7=_get_fat_size(_TF,sizeof(char));if(_T7!=1U)goto _TL10;if(_T10!=0)goto _TL10;if(_T11==0)goto _TL10;_throw_arraybounds();goto _TL11;_TL10: _TL11: _T8=_TF.curr;_T9=(char*)_T8;*_T9=_T11;}}}_TA=p;_TB=p;_TC=*_TB;
*_TA=_fat_ptr_plus(_TC,sizeof(char),1);{const char**_TF=& s;_TD=*_TF;_TE=*_TD;if(_TE==0)goto _TL12;*_TF=*_TF + 1;goto _TL13;_TL12: _throw_arraybounds();_TL13:;}goto _TLF;_TLE:;}
# 139
struct _fat_ptr Cyc_Sexp_tostring(struct Cyc_Sexp_Object v){struct Cyc_Sexp_Printer _T0;struct Cyc_Sexp_Printer*_T1;struct Cyc_Sexp_Printer*_T2;struct Cyc_Sexp_Object _T3;void(*_T4)(void(*)(int*,const char*),int*,struct Cyc_Xarray_Xarray*);void(*_T5)(void(*)(void*,void*),void*,struct Cyc_Xarray_Xarray*);int*_T6;struct Cyc_Xarray_Xarray*_T7;struct _fat_ptr _T8;void*_T9;void(*_TA)(void(*)(struct _fat_ptr*,const char*),struct _fat_ptr*,struct Cyc_Xarray_Xarray*);void(*_TB)(void(*)(void*,void*),void*,struct Cyc_Xarray_Xarray*);struct _fat_ptr*_TC;struct _fat_ptr*_TD;struct Cyc_Xarray_Xarray*_TE;struct _fat_ptr _TF;
struct Cyc_Xarray_Xarray*x=Cyc_Xarray_create_empty();{struct Cyc_Sexp_Printer _T10;
# 142
_T10.env=x;_T10.print=Cyc_Sexp_printstring;_T0=_T10;}{
# 141
struct Cyc_Sexp_Printer p=_T0;_T1=& p;_T2=(struct Cyc_Sexp_Printer*)_T1;_T3=v;
# 143
Cyc_Sexp_print(_T2,_T3);{
int len=1;_T5=Cyc_Xarray_iter_c;{
void(*_T10)(void(*)(int*,const char*),int*,struct Cyc_Xarray_Xarray*)=(void(*)(void(*)(int*,const char*),int*,struct Cyc_Xarray_Xarray*))_T5;_T4=_T10;}_T6=& len;_T7=x;_T4(Cyc_Sexp_addlength,_T6,_T7);{unsigned _T10=len;_T9=_cyccalloc_atomic(sizeof(char),_T10);_T8=_tag_fat(_T9,sizeof(char),_T10);}{
struct _fat_ptr res=_T8;
struct _fat_ptr p=res;_TB=Cyc_Xarray_iter_c;{
void(*_T10)(void(*)(struct _fat_ptr*,const char*),struct _fat_ptr*,struct Cyc_Xarray_Xarray*)=(void(*)(void(*)(struct _fat_ptr*,const char*),struct _fat_ptr*,struct Cyc_Xarray_Xarray*))_TB;_TA=_T10;}_TC=& p;_TD=(struct _fat_ptr*)_TC;_TE=x;_TA(Cyc_Sexp_addstring,_TD,_TE);_TF=res;
return _TF;}}}}struct Cyc_Sexp_Cls{struct Cyc_Sexp_Class*vtable;};
# 160
extern struct Cyc_List_List*Cyc_Sexp_classes;
# 163
void Cyc_Sexp_register_class(struct Cyc_Sexp_Class*c){struct Cyc_List_List*_T0;struct Cyc_Sexp_Cls*_T1;{struct Cyc_List_List*_T2=_cycalloc(sizeof(struct Cyc_List_List));{struct Cyc_Sexp_Cls*_T3=_cycalloc(sizeof(struct Cyc_Sexp_Cls));
_T3->vtable=c;_T1=(struct Cyc_Sexp_Cls*)_T3;}_T2->hd=_T1;_T2->tl=Cyc_Sexp_classes;_T0=(struct Cyc_List_List*)_T2;}Cyc_Sexp_classes=_T0;}
# 169
static int Cyc_Sexp_pgetc(struct Cyc_Sexp_Parser*p){struct Cyc_Sexp_Parser*_T0;int(*_T1)(void*);struct Cyc_Sexp_Parser*_T2;void*_T3;int _T4;_T0=p;_T1=_T0->getc;_T2=p;_T3=_T2->env;{
int res=_T1(_T3);_T4=res;
return _T4;}}
# 175
static void Cyc_Sexp_pungetc(struct Cyc_Sexp_Parser*p,int ch){struct Cyc_Sexp_Parser*_T0;int(*_T1)(int,void*);int _T2;struct Cyc_Sexp_Parser*_T3;void*_T4;_T0=p;_T1=_T0->ungetc;_T2=ch;_T3=p;_T4=_T3->env;
_T1(_T2,_T4);}
# 180
static void Cyc_Sexp_perror(struct Cyc_Sexp_Parser*p,int ch,struct _fat_ptr s){struct Cyc_Sexp_Parser*_T0;void(*_T1)(void*,int,struct _fat_ptr);struct Cyc_Sexp_Parser*_T2;void*_T3;int _T4;struct _fat_ptr _T5;_T0=p;_T1=_T0->error;_T2=p;_T3=_T2->env;_T4=ch;_T5=s;
# 182
_T1(_T3,_T4,_T5);}
# 186
static int Cyc_Sexp_whitespace(int ch){int _T0;
if(ch==32)goto _TL16;else{goto _TL18;}_TL18: if(ch==9)goto _TL16;else{goto _TL17;}_TL17: if(ch==10)goto _TL16;else{goto _TL14;}_TL16: _T0=1;goto _TL15;_TL14: _T0=ch==13;_TL15: return _T0;}
# 191
static void Cyc_Sexp_expectws(struct Cyc_Sexp_Parser*p,int expected_ch){int _T0;struct Cyc_Sexp_Parser*_T1;int _T2;struct _fat_ptr _T3;struct Cyc_Int_pa_PrintArg_struct _T4;int _T5;void**_T6;struct _fat_ptr _T7;struct _fat_ptr _T8;
int ch=Cyc_Sexp_pgetc(p);
_TL19: _T0=Cyc_Sexp_whitespace(ch);if(_T0)goto _TL1A;else{goto _TL1B;}_TL1A: ch=Cyc_Sexp_pgetc(p);goto _TL19;_TL1B:
 if(ch==expected_ch)goto _TL1C;_T1=p;_T2=ch;{struct Cyc_Int_pa_PrintArg_struct _T9;_T9.tag=1;_T5=expected_ch;_T9.f1=(unsigned long)_T5;_T4=_T9;}{struct Cyc_Int_pa_PrintArg_struct _T9=_T4;void*_TA[1];_T6=_TA + 0;*_T6=& _T9;_T7=_tag_fat("expected '%c'",sizeof(char),14U);_T8=_tag_fat(_TA,sizeof(void*),1);_T3=Cyc_aprintf(_T7,_T8);}Cyc_Sexp_perror(_T1,_T2,_T3);goto _TL1D;_TL1C: _TL1D:;}
# 198
static void Cyc_Sexp_expect(struct Cyc_Sexp_Parser*p,int expected_ch){struct Cyc_Sexp_Parser*_T0;int _T1;struct _fat_ptr _T2;struct Cyc_Int_pa_PrintArg_struct _T3;int _T4;void**_T5;struct _fat_ptr _T6;struct _fat_ptr _T7;
int ch=Cyc_Sexp_pgetc(p);
if(ch==expected_ch)goto _TL1E;_T0=p;_T1=ch;{struct Cyc_Int_pa_PrintArg_struct _T8;_T8.tag=1;_T4=expected_ch;_T8.f1=(unsigned long)_T4;_T3=_T8;}{struct Cyc_Int_pa_PrintArg_struct _T8=_T3;void*_T9[1];_T5=_T9 + 0;*_T5=& _T8;_T6=_tag_fat("expected '%c'",sizeof(char),14U);_T7=_tag_fat(_T9,sizeof(void*),1);_T2=Cyc_aprintf(_T6,_T7);}Cyc_Sexp_perror(_T0,_T1,_T2);goto _TL1F;_TL1E: _TL1F:;}
# 206
struct Cyc_Sexp_Object Cyc_Sexp_parse(struct Cyc_Sexp_Parser*p){int _T0;struct Cyc_List_List*_T1;void*_T2;int _T3;struct Cyc_Sexp_Class*_T4;char _T5;int _T6;struct Cyc_Sexp_Class*_T7;struct Cyc_Sexp_Obj*(*_T8)(struct Cyc_Sexp_Parser*);struct Cyc_Sexp_Object _T9;struct Cyc_List_List*_TA;struct Cyc_Sexp_Parser*_TB;void(*_TC)(void*,int,struct _fat_ptr);struct Cyc_Sexp_Parser*_TD;void*_TE;int _TF;struct _fat_ptr _T10;
int ch=Cyc_Sexp_pgetc(p);
_TL20: _T0=Cyc_Sexp_whitespace(ch);if(_T0)goto _TL21;else{goto _TL22;}_TL21: ch=Cyc_Sexp_pgetc(p);goto _TL20;_TL22:{
struct Cyc_List_List*cs=Cyc_Sexp_classes;_TL26: if(cs!=0)goto _TL24;else{goto _TL25;}
_TL24: _T1=cs;_T2=_T1->hd;{struct Cyc_Sexp_Cls*_T11=(struct Cyc_Sexp_Cls*)_T2;struct Cyc_Sexp_Class*_T12;{struct Cyc_Sexp_Cls _T13=*_T11;_T12=_T13.vtable;}{struct Cyc_Sexp_Class*v=_T12;_T3=ch;_T4=v;_T5=_T4->tag;_T6=(int)_T5;
if(_T3!=_T6)goto _TL27;
Cyc_Sexp_expectws(p,40);_T7=v;_T8=_T7->parse;{
struct Cyc_Sexp_Obj*obj=_T8(p);
Cyc_Sexp_expectws(p,41);_T9=
Cyc_Sexp_up(obj);return _T9;}_TL27:;}}_TA=cs;
# 209
cs=_TA->tl;goto _TL26;_TL25:;}_TB=p;_TC=_TB->error;_TD=p;_TE=_TD->env;_TF=ch;_T10=
# 218
_tag_fat("unexpected tag",sizeof(char),15U);_TC(_TE,_TF,_T10);}
# 223
static void Cyc_Sexp_file_error(struct Cyc___cycFILE*f,int ch,struct _fat_ptr msg){struct Cyc_Int_pa_PrintArg_struct _T0;int _T1;struct Cyc_String_pa_PrintArg_struct _T2;void**_T3;void**_T4;struct Cyc___cycFILE*_T5;struct _fat_ptr _T6;struct _fat_ptr _T7;struct Cyc_Core_Failure_exn_struct*_T8;void*_T9;{struct Cyc_Int_pa_PrintArg_struct _TA;_TA.tag=1;_T1=ch;
# 226
_TA.f1=(unsigned long)_T1;_T0=_TA;}{struct Cyc_Int_pa_PrintArg_struct _TA=_T0;{struct Cyc_String_pa_PrintArg_struct _TB;_TB.tag=0;_TB.f1=msg;_T2=_TB;}{struct Cyc_String_pa_PrintArg_struct _TB=_T2;void*_TC[2];_T3=_TC + 0;*_T3=& _TA;_T4=_TC + 1;*_T4=& _TB;_T5=Cyc_stderr;_T6=_tag_fat("found '%c'.  %s\n",sizeof(char),17U);_T7=_tag_fat(_TC,sizeof(void*),2);Cyc_fprintf(_T5,_T6,_T7);}}
Cyc_fclose(f);{struct Cyc_Core_Failure_exn_struct*_TA=_cycalloc(sizeof(struct Cyc_Core_Failure_exn_struct));_TA->tag=Cyc_Core_Failure;
_TA->f1=_tag_fat("Sexp::file2object error",sizeof(char),24U);_T8=(struct Cyc_Core_Failure_exn_struct*)_TA;}_T9=(void*)_T8;_throw(_T9);}
# 232
struct Cyc_Sexp_Object Cyc_Sexp_fromfile(struct Cyc___cycFILE*f){struct Cyc_Sexp_Parser _T0;struct _handler_cons*_T1;int*_T2;int _T3;struct Cyc_Sexp_Parser*_T4;struct Cyc_Sexp_Parser*_T5;void*_T6;{struct Cyc_Sexp_Parser _T7;
_T7.env=f;_T7.getc=Cyc_getc;_T7.ungetc=Cyc_ungetc;
_T7.error=Cyc_Sexp_file_error;_T0=_T7;}{
# 233
struct Cyc_Sexp_Parser p=_T0;struct _handler_cons _T7;_T1=& _T7;_push_handler(_T1);{int _T8=0;_T2=_T7.handler;_T3=setjmp(_T2);if(!_T3)goto _TL29;_T8=1;goto _TL2A;_TL29: _TL2A: if(_T8)goto _TL2B;else{goto _TL2D;}_TL2D: _T4=& p;_T5=(struct Cyc_Sexp_Parser*)_T4;{
# 236
struct Cyc_Sexp_Object s=Cyc_Sexp_parse(_T5);
Cyc_fclose(f);{struct Cyc_Sexp_Object _T9=s;_npop_handler(0);return _T9;}}_pop_handler();goto _TL2C;_TL2B: _T6=Cyc_Core_get_exn_thrown();{void*_T9=(void*)_T6;void*_TA;_TA=_T9;{void*e=_TA;
# 240
Cyc_Core_rethrow(e);};}_TL2C:;}}}
# 245
struct Cyc_Sexp_Object Cyc_Sexp_fromfilename(const char*filename){struct Cyc_Core_Failure_exn_struct*_T0;void*_T1;struct Cyc_Sexp_Object _T2;
struct Cyc___cycFILE*fopt=Cyc_fopen(filename,"r");
if(fopt!=0)goto _TL2E;{struct Cyc_Core_Failure_exn_struct*_T3=_cycalloc(sizeof(struct Cyc_Core_Failure_exn_struct));_T3->tag=Cyc_Core_Failure;_T3->f1=_tag_fat("file not found",sizeof(char),15U);_T0=(struct Cyc_Core_Failure_exn_struct*)_T3;}_T1=(void*)_T0;_throw(_T1);goto _TL2F;_TL2E: _TL2F: {
struct Cyc_Sexp_Object res=Cyc_Sexp_fromfile(fopt);
Cyc_fclose(fopt);_T2=res;
return _T2;}}struct Cyc_Sexp_StringEnv{struct _fat_ptr str;unsigned length;unsigned offset;};
# 261
static int Cyc_Sexp_string_getc(struct Cyc_Sexp_StringEnv*env){struct Cyc_Sexp_StringEnv*_T0;unsigned _T1;struct Cyc_Sexp_StringEnv*_T2;unsigned _T3;struct Cyc_Sexp_StringEnv*_T4;struct _fat_ptr _T5;struct Cyc_Sexp_StringEnv*_T6;unsigned _T7;int _T8;char*_T9;const char*_TA;char _TB;struct Cyc_Sexp_StringEnv*_TC;int _TD;_T0=env;_T1=_T0->offset;_T2=env;_T3=_T2->length;
if(_T1 < _T3)goto _TL30;return -1;_TL30: _T4=env;_T5=_T4->str;_T6=env;_T7=_T6->offset;_T8=(int)_T7;_T9=_check_fat_subscript(_T5,sizeof(char),_T8);_TA=(const char*)_T9;_TB=*_TA;{
int ch=(int)_TB;_TC=env;
_TC->offset=_TC->offset + 1;_TD=ch;
return _TD;}}
# 269
static int Cyc_Sexp_string_ungetc(int ch,struct Cyc_Sexp_StringEnv*env){struct Cyc_Sexp_StringEnv*_T0;
if(ch!=-1)goto _TL32;return -1;_TL32: _T0=env;
_T0->offset=_T0->offset + -1;
return 0;}
# 276
static void Cyc_Sexp_string_error(struct Cyc_Sexp_StringEnv*env,int ch,struct _fat_ptr msg){struct Cyc_Int_pa_PrintArg_struct _T0;int _T1;struct Cyc_String_pa_PrintArg_struct _T2;void**_T3;void**_T4;struct Cyc___cycFILE*_T5;struct _fat_ptr _T6;struct _fat_ptr _T7;struct Cyc_Core_Failure_exn_struct*_T8;void*_T9;{struct Cyc_Int_pa_PrintArg_struct _TA;_TA.tag=1;_T1=ch;
# 279
_TA.f1=(unsigned long)_T1;_T0=_TA;}{struct Cyc_Int_pa_PrintArg_struct _TA=_T0;{struct Cyc_String_pa_PrintArg_struct _TB;_TB.tag=0;_TB.f1=msg;_T2=_TB;}{struct Cyc_String_pa_PrintArg_struct _TB=_T2;void*_TC[2];_T3=_TC + 0;*_T3=& _TA;_T4=_TC + 1;*_T4=& _TB;_T5=Cyc_stderr;_T6=_tag_fat("found '%c', %s\n",sizeof(char),16U);_T7=_tag_fat(_TC,sizeof(void*),2);Cyc_fprintf(_T5,_T6,_T7);}}{struct Cyc_Core_Failure_exn_struct*_TA=_cycalloc(sizeof(struct Cyc_Core_Failure_exn_struct));_TA->tag=Cyc_Core_Failure;
_TA->f1=_tag_fat("Sexp::string2object error",sizeof(char),26U);_T8=(struct Cyc_Core_Failure_exn_struct*)_TA;}_T9=(void*)_T8;_throw(_T9);}
# 284
struct Cyc_Sexp_Object Cyc_Sexp_fromstring(struct _fat_ptr str){struct Cyc_Sexp_StringEnv _T0;struct Cyc_Sexp_Parser _T1;struct Cyc_Sexp_StringEnv*_T2;struct Cyc_Sexp_Parser*_T3;struct Cyc_Sexp_Parser*_T4;struct Cyc_Sexp_Object _T5;{struct Cyc_Sexp_StringEnv _T6;
_T6.str=str;_T6.length=Cyc_strlen(str);_T6.offset=0U;_T0=_T6;}{struct Cyc_Sexp_StringEnv env=_T0;{struct Cyc_Sexp_Parser _T6;_T2=& env;
# 287
_T6.env=(struct Cyc_Sexp_StringEnv*)_T2;_T6.getc=Cyc_Sexp_string_getc;_T6.ungetc=Cyc_Sexp_string_ungetc;
_T6.error=Cyc_Sexp_string_error;_T1=_T6;}{
# 286
struct Cyc_Sexp_Parser p=_T1;_T3=& p;_T4=(struct Cyc_Sexp_Parser*)_T3;_T5=
# 289
Cyc_Sexp_parse(_T4);return _T5;}}}
# 296
static int Cyc_Sexp_hash_ulonglong(struct _tuple6*self){struct _tuple6*_T0;unsigned long long _T1;int _T2;_T0=self;_T1=_T0->v;_T2=(int)_T1;return _T2;}
static int Cyc_Sexp_hash_slonglong(struct _tuple7*self){struct _tuple7*_T0;long long _T1;int _T2;_T0=self;_T1=_T0->v;_T2=(int)_T1;return _T2;}
static int Cyc_Sexp_hash_uint(struct _tuple4*self){struct _tuple4*_T0;unsigned _T1;int _T2;_T0=self;_T1=_T0->v;_T2=(int)_T1;return _T2;}
static int Cyc_Sexp_hash_sint(struct _tuple5*self){struct _tuple5*_T0;int _T1;_T0=self;_T1=_T0->v;return _T1;}
static int Cyc_Sexp_hash_ushort(struct _tuple2*self){struct _tuple2*_T0;unsigned short _T1;int _T2;_T0=self;_T1=_T0->v;_T2=(int)_T1;return _T2;}
static int Cyc_Sexp_hash_sshort(struct _tuple3*self){struct _tuple3*_T0;short _T1;int _T2;_T0=self;_T1=_T0->v;_T2=(int)_T1;return _T2;}
static int Cyc_Sexp_hash_uchar(struct _tuple0*self){struct _tuple0*_T0;unsigned char _T1;int _T2;_T0=self;_T1=_T0->v;_T2=(int)_T1;return _T2;}
static int Cyc_Sexp_hash_schar(struct _tuple1*self){struct _tuple1*_T0;signed char _T1;int _T2;_T0=self;_T1=_T0->v;_T2=(int)_T1;return _T2;}
# 311
static int Cyc_Sexp_cmp_uint(struct _tuple4*x,struct _tuple4*y){struct _tuple4*_T0;unsigned _T1;struct _tuple4*_T2;unsigned _T3;unsigned _T4;int _T5;_T0=x;_T1=_T0->v;_T2=y;_T3=_T2->v;_T4=_T1 - _T3;_T5=(int)_T4;return _T5;}
static int Cyc_Sexp_cmp_sint(struct _tuple5*x,struct _tuple5*y){struct _tuple5*_T0;int _T1;struct _tuple5*_T2;int _T3;int _T4;_T0=x;_T1=_T0->v;_T2=y;_T3=_T2->v;_T4=_T1 - _T3;return _T4;}
static int Cyc_Sexp_cmp_ushort(struct _tuple2*x,struct _tuple2*y){struct _tuple2*_T0;unsigned short _T1;int _T2;struct _tuple2*_T3;unsigned short _T4;int _T5;int _T6;_T0=x;_T1=_T0->v;_T2=(int)_T1;_T3=y;_T4=_T3->v;_T5=(int)_T4;_T6=_T2 - _T5;return _T6;}
static int Cyc_Sexp_cmp_sshort(struct _tuple3*x,struct _tuple3*y){struct _tuple3*_T0;short _T1;int _T2;struct _tuple3*_T3;short _T4;int _T5;int _T6;_T0=x;_T1=_T0->v;_T2=(int)_T1;_T3=y;_T4=_T3->v;_T5=(int)_T4;_T6=_T2 - _T5;return _T6;}
static int Cyc_Sexp_cmp_uchar(struct _tuple0*x,struct _tuple0*y){struct _tuple0*_T0;unsigned char _T1;int _T2;struct _tuple0*_T3;unsigned char _T4;int _T5;int _T6;_T0=x;_T1=_T0->v;_T2=(int)_T1;_T3=y;_T4=_T3->v;_T5=(int)_T4;_T6=_T2 - _T5;return _T6;}
static int Cyc_Sexp_cmp_schar(struct _tuple1*x,struct _tuple1*y){struct _tuple1*_T0;signed char _T1;int _T2;struct _tuple1*_T3;signed char _T4;int _T5;int _T6;_T0=x;_T1=_T0->v;_T2=(int)_T1;_T3=y;_T4=_T3->v;_T5=(int)_T4;_T6=_T2 - _T5;return _T6;}
# 331 "sexp.cyc"
static void*Cyc_Sexp_accept_uchar(struct _tuple0*self,struct Cyc_Sexp_Visitor*visitor,void*env){struct Cyc_Sexp_Visitor*_T0;void*(*_T1)(void*,struct _tuple0*,struct Cyc_Sexp_Visitor*);struct Cyc_Sexp_Visitor*_T2;void*(*_T3)(void*,struct Cyc_Sexp_Object,struct Cyc_Sexp_Visitor*);void*(*_T4)(void*,struct Cyc_Sexp_Object,struct Cyc_Sexp_Visitor*);void*_T5;struct Cyc_Sexp_Object(*_T6)(struct _tuple0*);struct Cyc_Sexp_Object _T7;struct Cyc_Sexp_Visitor*_T8;void*_T9;void*_TA;_T0=visitor;{void*(*f)(void*,struct _tuple0*,struct Cyc_Sexp_Visitor*)=_T0->visit_uchar;_T1=f;if(_T1!=0)goto _TL34;_T2=visitor;_T3=_T2->visit_default;_T4=_check_null(_T3);_T5=env;{struct Cyc_Sexp_Object(*_TB)(struct _tuple0*)=(struct Cyc_Sexp_Object(*)(struct _tuple0*))Cyc_Sexp_up;_T6=_TB;}_T7=_T6(self);_T8=visitor;_T9=_T4(_T5,_T7,_T8);return _T9;_TL34: _TA=f(env,self,visitor);return _TA;}}
static void*Cyc_Sexp_accept_schar(struct _tuple1*self,struct Cyc_Sexp_Visitor*visitor,void*env){struct Cyc_Sexp_Visitor*_T0;void*(*_T1)(void*,struct _tuple1*,struct Cyc_Sexp_Visitor*);struct Cyc_Sexp_Visitor*_T2;void*(*_T3)(void*,struct Cyc_Sexp_Object,struct Cyc_Sexp_Visitor*);void*(*_T4)(void*,struct Cyc_Sexp_Object,struct Cyc_Sexp_Visitor*);void*_T5;struct Cyc_Sexp_Object(*_T6)(struct _tuple1*);struct Cyc_Sexp_Object _T7;struct Cyc_Sexp_Visitor*_T8;void*_T9;void*_TA;_T0=visitor;{void*(*f)(void*,struct _tuple1*,struct Cyc_Sexp_Visitor*)=_T0->visit_schar;_T1=f;if(_T1!=0)goto _TL36;_T2=visitor;_T3=_T2->visit_default;_T4=_check_null(_T3);_T5=env;{struct Cyc_Sexp_Object(*_TB)(struct _tuple1*)=(struct Cyc_Sexp_Object(*)(struct _tuple1*))Cyc_Sexp_up;_T6=_TB;}_T7=_T6(self);_T8=visitor;_T9=_T4(_T5,_T7,_T8);return _T9;_TL36: _TA=f(env,self,visitor);return _TA;}}
static void*Cyc_Sexp_accept_ushort(struct _tuple2*self,struct Cyc_Sexp_Visitor*visitor,void*env){struct Cyc_Sexp_Visitor*_T0;void*(*_T1)(void*,struct _tuple2*,struct Cyc_Sexp_Visitor*);struct Cyc_Sexp_Visitor*_T2;void*(*_T3)(void*,struct Cyc_Sexp_Object,struct Cyc_Sexp_Visitor*);void*(*_T4)(void*,struct Cyc_Sexp_Object,struct Cyc_Sexp_Visitor*);void*_T5;struct Cyc_Sexp_Object(*_T6)(struct _tuple2*);struct Cyc_Sexp_Object _T7;struct Cyc_Sexp_Visitor*_T8;void*_T9;void*_TA;_T0=visitor;{void*(*f)(void*,struct _tuple2*,struct Cyc_Sexp_Visitor*)=_T0->visit_ushort;_T1=f;if(_T1!=0)goto _TL38;_T2=visitor;_T3=_T2->visit_default;_T4=_check_null(_T3);_T5=env;{struct Cyc_Sexp_Object(*_TB)(struct _tuple2*)=(struct Cyc_Sexp_Object(*)(struct _tuple2*))Cyc_Sexp_up;_T6=_TB;}_T7=_T6(self);_T8=visitor;_T9=_T4(_T5,_T7,_T8);return _T9;_TL38: _TA=f(env,self,visitor);return _TA;}}
static void*Cyc_Sexp_accept_sshort(struct _tuple3*self,struct Cyc_Sexp_Visitor*visitor,void*env){struct Cyc_Sexp_Visitor*_T0;void*(*_T1)(void*,struct _tuple3*,struct Cyc_Sexp_Visitor*);struct Cyc_Sexp_Visitor*_T2;void*(*_T3)(void*,struct Cyc_Sexp_Object,struct Cyc_Sexp_Visitor*);void*(*_T4)(void*,struct Cyc_Sexp_Object,struct Cyc_Sexp_Visitor*);void*_T5;struct Cyc_Sexp_Object(*_T6)(struct _tuple3*);struct Cyc_Sexp_Object _T7;struct Cyc_Sexp_Visitor*_T8;void*_T9;void*_TA;_T0=visitor;{void*(*f)(void*,struct _tuple3*,struct Cyc_Sexp_Visitor*)=_T0->visit_sshort;_T1=f;if(_T1!=0)goto _TL3A;_T2=visitor;_T3=_T2->visit_default;_T4=_check_null(_T3);_T5=env;{struct Cyc_Sexp_Object(*_TB)(struct _tuple3*)=(struct Cyc_Sexp_Object(*)(struct _tuple3*))Cyc_Sexp_up;_T6=_TB;}_T7=_T6(self);_T8=visitor;_T9=_T4(_T5,_T7,_T8);return _T9;_TL3A: _TA=f(env,self,visitor);return _TA;}}
static void*Cyc_Sexp_accept_uint(struct _tuple4*self,struct Cyc_Sexp_Visitor*visitor,void*env){struct Cyc_Sexp_Visitor*_T0;void*(*_T1)(void*,struct _tuple4*,struct Cyc_Sexp_Visitor*);struct Cyc_Sexp_Visitor*_T2;void*(*_T3)(void*,struct Cyc_Sexp_Object,struct Cyc_Sexp_Visitor*);void*(*_T4)(void*,struct Cyc_Sexp_Object,struct Cyc_Sexp_Visitor*);void*_T5;struct Cyc_Sexp_Object(*_T6)(struct _tuple4*);struct Cyc_Sexp_Object _T7;struct Cyc_Sexp_Visitor*_T8;void*_T9;void*_TA;_T0=visitor;{void*(*f)(void*,struct _tuple4*,struct Cyc_Sexp_Visitor*)=_T0->visit_uint;_T1=f;if(_T1!=0)goto _TL3C;_T2=visitor;_T3=_T2->visit_default;_T4=_check_null(_T3);_T5=env;{struct Cyc_Sexp_Object(*_TB)(struct _tuple4*)=(struct Cyc_Sexp_Object(*)(struct _tuple4*))Cyc_Sexp_up;_T6=_TB;}_T7=_T6(self);_T8=visitor;_T9=_T4(_T5,_T7,_T8);return _T9;_TL3C: _TA=f(env,self,visitor);return _TA;}}
static void*Cyc_Sexp_accept_sint(struct _tuple5*self,struct Cyc_Sexp_Visitor*visitor,void*env){struct Cyc_Sexp_Visitor*_T0;void*(*_T1)(void*,struct _tuple5*,struct Cyc_Sexp_Visitor*);struct Cyc_Sexp_Visitor*_T2;void*(*_T3)(void*,struct Cyc_Sexp_Object,struct Cyc_Sexp_Visitor*);void*(*_T4)(void*,struct Cyc_Sexp_Object,struct Cyc_Sexp_Visitor*);void*_T5;struct Cyc_Sexp_Object(*_T6)(struct _tuple5*);struct Cyc_Sexp_Object _T7;struct Cyc_Sexp_Visitor*_T8;void*_T9;void*_TA;_T0=visitor;{void*(*f)(void*,struct _tuple5*,struct Cyc_Sexp_Visitor*)=_T0->visit_sint;_T1=f;if(_T1!=0)goto _TL3E;_T2=visitor;_T3=_T2->visit_default;_T4=_check_null(_T3);_T5=env;{struct Cyc_Sexp_Object(*_TB)(struct _tuple5*)=(struct Cyc_Sexp_Object(*)(struct _tuple5*))Cyc_Sexp_up;_T6=_TB;}_T7=_T6(self);_T8=visitor;_T9=_T4(_T5,_T7,_T8);return _T9;_TL3E: _TA=f(env,self,visitor);return _TA;}}
static void*Cyc_Sexp_accept_ulonglong(struct _tuple6*self,struct Cyc_Sexp_Visitor*visitor,void*env){struct Cyc_Sexp_Visitor*_T0;void*(*_T1)(void*,struct _tuple6*,struct Cyc_Sexp_Visitor*);struct Cyc_Sexp_Visitor*_T2;void*(*_T3)(void*,struct Cyc_Sexp_Object,struct Cyc_Sexp_Visitor*);void*(*_T4)(void*,struct Cyc_Sexp_Object,struct Cyc_Sexp_Visitor*);void*_T5;struct Cyc_Sexp_Object(*_T6)(struct _tuple6*);struct Cyc_Sexp_Object _T7;struct Cyc_Sexp_Visitor*_T8;void*_T9;void*_TA;_T0=visitor;{void*(*f)(void*,struct _tuple6*,struct Cyc_Sexp_Visitor*)=_T0->visit_ulonglong;_T1=f;if(_T1!=0)goto _TL40;_T2=visitor;_T3=_T2->visit_default;_T4=_check_null(_T3);_T5=env;{struct Cyc_Sexp_Object(*_TB)(struct _tuple6*)=(struct Cyc_Sexp_Object(*)(struct _tuple6*))Cyc_Sexp_up;_T6=_TB;}_T7=_T6(self);_T8=visitor;_T9=_T4(_T5,_T7,_T8);return _T9;_TL40: _TA=f(env,self,visitor);return _TA;}}
static void*Cyc_Sexp_accept_slonglong(struct _tuple7*self,struct Cyc_Sexp_Visitor*visitor,void*env){struct Cyc_Sexp_Visitor*_T0;void*(*_T1)(void*,struct _tuple7*,struct Cyc_Sexp_Visitor*);struct Cyc_Sexp_Visitor*_T2;void*(*_T3)(void*,struct Cyc_Sexp_Object,struct Cyc_Sexp_Visitor*);void*(*_T4)(void*,struct Cyc_Sexp_Object,struct Cyc_Sexp_Visitor*);void*_T5;struct Cyc_Sexp_Object(*_T6)(struct _tuple7*);struct Cyc_Sexp_Object _T7;struct Cyc_Sexp_Visitor*_T8;void*_T9;void*_TA;_T0=visitor;{void*(*f)(void*,struct _tuple7*,struct Cyc_Sexp_Visitor*)=_T0->visit_slonglong;_T1=f;if(_T1!=0)goto _TL42;_T2=visitor;_T3=_T2->visit_default;_T4=_check_null(_T3);_T5=env;{struct Cyc_Sexp_Object(*_TB)(struct _tuple7*)=(struct Cyc_Sexp_Object(*)(struct _tuple7*))Cyc_Sexp_up;_T6=_TB;}_T7=_T6(self);_T8=visitor;_T9=_T4(_T5,_T7,_T8);return _T9;_TL42: _TA=f(env,self,visitor);return _TA;}}
static void*Cyc_Sexp_accept_float(struct _tuple8*self,struct Cyc_Sexp_Visitor*visitor,void*env){struct Cyc_Sexp_Visitor*_T0;void*(*_T1)(void*,struct _tuple8*,struct Cyc_Sexp_Visitor*);struct Cyc_Sexp_Visitor*_T2;void*(*_T3)(void*,struct Cyc_Sexp_Object,struct Cyc_Sexp_Visitor*);void*(*_T4)(void*,struct Cyc_Sexp_Object,struct Cyc_Sexp_Visitor*);void*_T5;struct Cyc_Sexp_Object(*_T6)(struct _tuple8*);struct Cyc_Sexp_Object _T7;struct Cyc_Sexp_Visitor*_T8;void*_T9;void*_TA;_T0=visitor;{void*(*f)(void*,struct _tuple8*,struct Cyc_Sexp_Visitor*)=_T0->visit_float;_T1=f;if(_T1!=0)goto _TL44;_T2=visitor;_T3=_T2->visit_default;_T4=_check_null(_T3);_T5=env;{struct Cyc_Sexp_Object(*_TB)(struct _tuple8*)=(struct Cyc_Sexp_Object(*)(struct _tuple8*))Cyc_Sexp_up;_T6=_TB;}_T7=_T6(self);_T8=visitor;_T9=_T4(_T5,_T7,_T8);return _T9;_TL44: _TA=f(env,self,visitor);return _TA;}}
static void*Cyc_Sexp_accept_double(struct _tuple9*self,struct Cyc_Sexp_Visitor*visitor,void*env){struct Cyc_Sexp_Visitor*_T0;void*(*_T1)(void*,struct _tuple9*,struct Cyc_Sexp_Visitor*);struct Cyc_Sexp_Visitor*_T2;void*(*_T3)(void*,struct Cyc_Sexp_Object,struct Cyc_Sexp_Visitor*);void*(*_T4)(void*,struct Cyc_Sexp_Object,struct Cyc_Sexp_Visitor*);void*_T5;struct Cyc_Sexp_Object(*_T6)(struct _tuple9*);struct Cyc_Sexp_Object _T7;struct Cyc_Sexp_Visitor*_T8;void*_T9;void*_TA;_T0=visitor;{void*(*f)(void*,struct _tuple9*,struct Cyc_Sexp_Visitor*)=_T0->visit_double;_T1=f;if(_T1!=0)goto _TL46;_T2=visitor;_T3=_T2->visit_default;_T4=_check_null(_T3);_T5=env;{struct Cyc_Sexp_Object(*_TB)(struct _tuple9*)=(struct Cyc_Sexp_Object(*)(struct _tuple9*))Cyc_Sexp_up;_T6=_TB;}_T7=_T6(self);_T8=visitor;_T9=_T4(_T5,_T7,_T8);return _T9;_TL46: _TA=f(env,self,visitor);return _TA;}}
static void*Cyc_Sexp_accept_str(struct _tuple10*self,struct Cyc_Sexp_Visitor*visitor,void*env){struct Cyc_Sexp_Visitor*_T0;void*(*_T1)(void*,struct _tuple10*,struct Cyc_Sexp_Visitor*);struct Cyc_Sexp_Visitor*_T2;void*(*_T3)(void*,struct Cyc_Sexp_Object,struct Cyc_Sexp_Visitor*);void*(*_T4)(void*,struct Cyc_Sexp_Object,struct Cyc_Sexp_Visitor*);void*_T5;struct Cyc_Sexp_Object(*_T6)(struct _tuple10*);struct Cyc_Sexp_Object _T7;struct Cyc_Sexp_Visitor*_T8;void*_T9;void*_TA;_T0=visitor;{void*(*f)(void*,struct _tuple10*,struct Cyc_Sexp_Visitor*)=_T0->visit_str;_T1=f;if(_T1!=0)goto _TL48;_T2=visitor;_T3=_T2->visit_default;_T4=_check_null(_T3);_T5=env;{struct Cyc_Sexp_Object(*_TB)(struct _tuple10*)=(struct Cyc_Sexp_Object(*)(struct _tuple10*))Cyc_Sexp_up;_T6=_TB;}_T7=_T6(self);_T8=visitor;_T9=_T4(_T5,_T7,_T8);return _T9;_TL48: _TA=f(env,self,visitor);return _TA;}}
static void*Cyc_Sexp_accept_symbol(struct _tuple10*self,struct Cyc_Sexp_Visitor*visitor,void*env){struct Cyc_Sexp_Visitor*_T0;void*(*_T1)(void*,struct _tuple10*,struct Cyc_Sexp_Visitor*);struct Cyc_Sexp_Visitor*_T2;void*(*_T3)(void*,struct Cyc_Sexp_Object,struct Cyc_Sexp_Visitor*);void*(*_T4)(void*,struct Cyc_Sexp_Object,struct Cyc_Sexp_Visitor*);void*_T5;struct Cyc_Sexp_Object(*_T6)(struct _tuple10*);struct Cyc_Sexp_Object _T7;struct Cyc_Sexp_Visitor*_T8;void*_T9;void*_TA;_T0=visitor;{void*(*f)(void*,struct _tuple10*,struct Cyc_Sexp_Visitor*)=_T0->visit_symbol;_T1=f;if(_T1!=0)goto _TL4A;_T2=visitor;_T3=_T2->visit_default;_T4=_check_null(_T3);_T5=env;{struct Cyc_Sexp_Object(*_TB)(struct _tuple10*)=(struct Cyc_Sexp_Object(*)(struct _tuple10*))Cyc_Sexp_up;_T6=_TB;}_T7=_T6(self);_T8=visitor;_T9=_T4(_T5,_T7,_T8);return _T9;_TL4A: _TA=f(env,self,visitor);return _TA;}}
static void*Cyc_Sexp_accept_tuple(struct _tuple10*self,struct Cyc_Sexp_Visitor*visitor,void*env){struct Cyc_Sexp_Visitor*_T0;void*(*_T1)(void*,struct _tuple10*,struct Cyc_Sexp_Visitor*);struct Cyc_Sexp_Visitor*_T2;void*(*_T3)(void*,struct Cyc_Sexp_Object,struct Cyc_Sexp_Visitor*);void*(*_T4)(void*,struct Cyc_Sexp_Object,struct Cyc_Sexp_Visitor*);void*_T5;struct Cyc_Sexp_Object(*_T6)(struct _tuple10*);struct Cyc_Sexp_Object _T7;struct Cyc_Sexp_Visitor*_T8;void*_T9;void*_TA;_T0=visitor;{void*(*f)(void*,struct _tuple10*,struct Cyc_Sexp_Visitor*)=_T0->visit_tuple;_T1=f;if(_T1!=0)goto _TL4C;_T2=visitor;_T3=_T2->visit_default;_T4=_check_null(_T3);_T5=env;{struct Cyc_Sexp_Object(*_TB)(struct _tuple10*)=(struct Cyc_Sexp_Object(*)(struct _tuple10*))Cyc_Sexp_up;_T6=_TB;}_T7=_T6(self);_T8=visitor;_T9=_T4(_T5,_T7,_T8);return _T9;_TL4C: _TA=f(env,self,visitor);return _TA;}}
# 348
static unsigned Cyc_Sexp_hex2value(struct Cyc_Sexp_Parser*p,int ch){int _T0;unsigned _T1;int _T2;int _T3;unsigned _T4;int _T5;int _T6;unsigned _T7;struct Cyc_Sexp_Parser*_T8;int _T9;struct _fat_ptr _TA;
if(ch < 48)goto _TL4E;if(ch > 57)goto _TL4E;_T0=ch - 48;_T1=(unsigned)_T0;
return _T1;_TL4E:
 if(ch < 65)goto _TL50;if(ch > 70)goto _TL50;_T2=ch - 65;_T3=10 + _T2;_T4=(unsigned)_T3;
return _T4;_TL50:
 if(ch < 97)goto _TL52;if(ch > 102)goto _TL52;_T5=ch - 97;_T6=10 + _T5;_T7=(unsigned)_T6;
return _T7;_TL52: _T8=p;_T9=ch;_TA=
_tag_fat("expecting hex digit",sizeof(char),20U);Cyc_Sexp_perror(_T8,_T9,_TA);}
# 359
static unsigned Cyc_Sexp_nibble(unsigned i,unsigned x){unsigned _T0;
_TL57: if(i > 0U)goto _TL55;else{goto _TL56;}
_TL55: x=x >> 4;
# 360
i=i + -1;goto _TL57;_TL56: _T0=x & 15U;
# 362
return _T0;}
# 366
static unsigned long long Cyc_Sexp_parse_longlong(struct Cyc_Sexp_Parser*p){unsigned _T0;unsigned long long _T1;unsigned long long _T2;unsigned long long _T3;
unsigned long long res=0U;{
unsigned i=0U;_TL5B: if(i < 16U)goto _TL59;else{goto _TL5A;}
_TL59:{int ch=Cyc_Sexp_pgetc(p);_T0=
Cyc_Sexp_hex2value(p,ch);{unsigned long long v=(unsigned long long)_T0;_T1=res << 4;_T2=v;
res=_T1 | _T2;}}
# 368
i=i + 1;goto _TL5B;_TL5A:;}_T3=res;
# 373
return _T3;}
# 377
static void Cyc_Sexp_print_ulonglong(struct _tuple6*self,struct Cyc_Sexp_Printer*p){struct _tuple6*_T0;struct Cyc_Sexp_Printer*_T1;void(*_T2)(void*,struct _fat_ptr);struct Cyc_Sexp_Printer*_T3;void*_T4;struct _fat_ptr _T5;struct Cyc_Int_pa_PrintArg_struct _T6;unsigned long long _T7;struct Cyc_Int_pa_PrintArg_struct _T8;unsigned long long _T9;void**_TA;void**_TB;struct _fat_ptr _TC;struct _fat_ptr _TD;_T0=self;{
# 379
unsigned long long x=_T0->v;_T1=p;_T2=_T1->print;_T3=p;_T4=_T3->env;{struct Cyc_Int_pa_PrintArg_struct _TE;_TE.tag=1;_T7=x >> 32;
_TE.f1=(unsigned)_T7;_T6=_TE;}{struct Cyc_Int_pa_PrintArg_struct _TE=_T6;{struct Cyc_Int_pa_PrintArg_struct _TF;_TF.tag=1;_T9=x;_TF.f1=(unsigned)_T9;_T8=_TF;}{struct Cyc_Int_pa_PrintArg_struct _TF=_T8;void*_T10[2];_TA=_T10 + 0;*_TA=& _TE;_TB=_T10 + 1;*_TB=& _TF;_TC=_tag_fat("%08x%08x",sizeof(char),9U);_TD=_tag_fat(_T10,sizeof(void*),2);_T5=Cyc_aprintf(_TC,_TD);}}_T2(_T4,_T5);}}
# 383
static struct _tuple6*Cyc_Sexp_parse_ulonglong(struct Cyc_Sexp_Parser*p){unsigned long long _T0;struct _tuple6*_T1;_T0=
Cyc_Sexp_parse_longlong(p);_T1=Cyc_Sexp_mk_ulonglong(_T0);return _T1;}
# 387
static void Cyc_Sexp_print_slonglong(struct _tuple7*self,struct Cyc_Sexp_Printer*p){struct _tuple7*_T0;struct Cyc_Sexp_Printer*_T1;void(*_T2)(void*,struct _fat_ptr);struct Cyc_Sexp_Printer*_T3;void*_T4;struct _fat_ptr _T5;struct Cyc_Int_pa_PrintArg_struct _T6;long long _T7;struct Cyc_Int_pa_PrintArg_struct _T8;long long _T9;void**_TA;void**_TB;struct _fat_ptr _TC;struct _fat_ptr _TD;_T0=self;{
# 389
long long x=_T0->v;_T1=p;_T2=_T1->print;_T3=p;_T4=_T3->env;{struct Cyc_Int_pa_PrintArg_struct _TE;_TE.tag=1;_T7=x >> 32;
_TE.f1=(unsigned)_T7;_T6=_TE;}{struct Cyc_Int_pa_PrintArg_struct _TE=_T6;{struct Cyc_Int_pa_PrintArg_struct _TF;_TF.tag=1;_T9=x;_TF.f1=(unsigned)_T9;_T8=_TF;}{struct Cyc_Int_pa_PrintArg_struct _TF=_T8;void*_T10[2];_TA=_T10 + 0;*_TA=& _TE;_TB=_T10 + 1;*_TB=& _TF;_TC=_tag_fat("%08x%08x",sizeof(char),9U);_TD=_tag_fat(_T10,sizeof(void*),2);_T5=Cyc_aprintf(_TC,_TD);}}_T2(_T4,_T5);}}
# 392
static struct _tuple7*Cyc_Sexp_parse_slonglong(struct Cyc_Sexp_Parser*p){unsigned long long _T0;long long _T1;struct _tuple7*_T2;_T0=
Cyc_Sexp_parse_longlong(p);_T1=(long long)_T0;_T2=Cyc_Sexp_mk_slonglong(_T1);return _T2;}
# 396
static int Cyc_Sexp_cmp_ulonglong(struct _tuple6*x,struct _tuple6*y){struct _tuple6*_T0;unsigned long long _T1;struct _tuple6*_T2;unsigned long long _T3;struct _tuple6*_T4;unsigned long long _T5;struct _tuple6*_T6;unsigned long long _T7;_T0=x;_T1=_T0->v;_T2=y;_T3=_T2->v;
# 398
if(_T1 <= _T3)goto _TL5C;return 1;
_TL5C: _T4=x;_T5=_T4->v;_T6=y;_T7=_T6->v;if(_T5 >= _T7)goto _TL5E;return -1;
_TL5E: return 0;}
# 403
static int Cyc_Sexp_cmp_slonglong(struct _tuple7*x,struct _tuple7*y){struct _tuple7*_T0;long long _T1;struct _tuple7*_T2;long long _T3;struct _tuple7*_T4;long long _T5;struct _tuple7*_T6;long long _T7;_T0=x;_T1=_T0->v;_T2=y;_T3=_T2->v;
# 405
if(_T1 <= _T3)goto _TL60;return 1;
_TL60: _T4=x;_T5=_T4->v;_T6=y;_T7=_T6->v;if(_T5 >= _T7)goto _TL62;return -1;
_TL62: return 0;}
# 411
static unsigned Cyc_Sexp_parse_int(struct Cyc_Sexp_Parser*p){unsigned _T0;unsigned _T1;unsigned _T2;
unsigned res=0U;{
unsigned i=0U;_TL67: if(i < 8U)goto _TL65;else{goto _TL66;}
_TL65:{int ch=Cyc_Sexp_pgetc(p);
unsigned v=Cyc_Sexp_hex2value(p,ch);_T0=res << 4;_T1=v;
res=_T0 | _T1;}
# 413
i=i + 1;goto _TL67;_TL66:;}_T2=res;
# 418
return _T2;}
# 421
static void Cyc_Sexp_print_uint(struct _tuple4*self,struct Cyc_Sexp_Printer*p){struct _tuple4*_T0;struct Cyc_Sexp_Printer*_T1;void(*_T2)(void*,struct _fat_ptr);struct Cyc_Sexp_Printer*_T3;void*_T4;struct _fat_ptr _T5;struct Cyc_Int_pa_PrintArg_struct _T6;void**_T7;struct _fat_ptr _T8;struct _fat_ptr _T9;_T0=self;{
unsigned x=_T0->v;_T1=p;_T2=_T1->print;_T3=p;_T4=_T3->env;{struct Cyc_Int_pa_PrintArg_struct _TA;_TA.tag=1;
_TA.f1=x;_T6=_TA;}{struct Cyc_Int_pa_PrintArg_struct _TA=_T6;void*_TB[1];_T7=_TB + 0;*_T7=& _TA;_T8=_tag_fat("%08x",sizeof(char),5U);_T9=_tag_fat(_TB,sizeof(void*),1);_T5=Cyc_aprintf(_T8,_T9);}_T2(_T4,_T5);}}
# 426
static struct _tuple4*Cyc_Sexp_parse_uint(struct Cyc_Sexp_Parser*p){unsigned _T0;struct _tuple4*_T1;_T0=
Cyc_Sexp_parse_int(p);_T1=Cyc_Sexp_mk_uint(_T0);return _T1;}
# 430
static void Cyc_Sexp_print_sint(struct _tuple5*self,struct Cyc_Sexp_Printer*p){struct _tuple5*_T0;struct Cyc_Sexp_Printer*_T1;void(*_T2)(void*,struct _fat_ptr);struct Cyc_Sexp_Printer*_T3;void*_T4;struct _fat_ptr _T5;struct Cyc_Int_pa_PrintArg_struct _T6;int _T7;void**_T8;struct _fat_ptr _T9;struct _fat_ptr _TA;_T0=self;{
int x=_T0->v;_T1=p;_T2=_T1->print;_T3=p;_T4=_T3->env;{struct Cyc_Int_pa_PrintArg_struct _TB;_TB.tag=1;_T7=x;
_TB.f1=(unsigned)_T7;_T6=_TB;}{struct Cyc_Int_pa_PrintArg_struct _TB=_T6;void*_TC[1];_T8=_TC + 0;*_T8=& _TB;_T9=_tag_fat("%08x",sizeof(char),5U);_TA=_tag_fat(_TC,sizeof(void*),1);_T5=Cyc_aprintf(_T9,_TA);}_T2(_T4,_T5);}}
# 435
static struct _tuple5*Cyc_Sexp_parse_sint(struct Cyc_Sexp_Parser*p){unsigned _T0;int _T1;struct _tuple5*_T2;_T0=
Cyc_Sexp_parse_int(p);_T1=(int)_T0;_T2=Cyc_Sexp_mk_sint(_T1);return _T2;}
# 440
static unsigned short Cyc_Sexp_parse_short(struct Cyc_Sexp_Parser*p){unsigned _T0;unsigned _T1;unsigned _T2;unsigned short _T3;
unsigned res=0U;{
unsigned i=0U;_TL6B: if(i < 4U)goto _TL69;else{goto _TL6A;}
_TL69:{int ch=Cyc_Sexp_pgetc(p);
unsigned v=Cyc_Sexp_hex2value(p,ch);_T0=res << 4;_T1=v;
res=_T0 | _T1;}
# 442
i=i + 1;goto _TL6B;_TL6A:;}_T2=res;_T3=(unsigned short)_T2;
# 447
return _T3;}
# 451
static void Cyc_Sexp_print_ushort(struct _tuple2*self,struct Cyc_Sexp_Printer*p){struct _tuple2*_T0;struct Cyc_Sexp_Printer*_T1;void(*_T2)(void*,struct _fat_ptr);struct Cyc_Sexp_Printer*_T3;void*_T4;struct _fat_ptr _T5;struct Cyc_Int_pa_PrintArg_struct _T6;unsigned short _T7;unsigned _T8;struct Cyc_Int_pa_PrintArg_struct _T9;unsigned short _TA;unsigned _TB;struct Cyc_Int_pa_PrintArg_struct _TC;unsigned short _TD;unsigned _TE;struct Cyc_Int_pa_PrintArg_struct _TF;unsigned short _T10;unsigned _T11;void**_T12;void**_T13;void**_T14;void**_T15;struct _fat_ptr _T16;struct _fat_ptr _T17;_T0=self;{
unsigned short x=_T0->v;_T1=p;_T2=_T1->print;_T3=p;_T4=_T3->env;{struct Cyc_Int_pa_PrintArg_struct _T18;_T18.tag=1;_T7=x;_T8=(unsigned)_T7;
# 454
_T18.f1=Cyc_Sexp_nibble(3U,_T8);_T6=_T18;}{struct Cyc_Int_pa_PrintArg_struct _T18=_T6;{struct Cyc_Int_pa_PrintArg_struct _T19;_T19.tag=1;_TA=x;_TB=(unsigned)_TA;_T19.f1=Cyc_Sexp_nibble(2U,_TB);_T9=_T19;}{struct Cyc_Int_pa_PrintArg_struct _T19=_T9;{struct Cyc_Int_pa_PrintArg_struct _T1A;_T1A.tag=1;_TD=x;_TE=(unsigned)_TD;_T1A.f1=Cyc_Sexp_nibble(1U,_TE);_TC=_T1A;}{struct Cyc_Int_pa_PrintArg_struct _T1A=_TC;{struct Cyc_Int_pa_PrintArg_struct _T1B;_T1B.tag=1;_T10=x;_T11=(unsigned)_T10;_T1B.f1=Cyc_Sexp_nibble(0U,_T11);_TF=_T1B;}{struct Cyc_Int_pa_PrintArg_struct _T1B=_TF;void*_T1C[4];_T12=_T1C + 0;*_T12=& _T18;_T13=_T1C + 1;*_T13=& _T19;_T14=_T1C + 2;*_T14=& _T1A;_T15=_T1C + 3;*_T15=& _T1B;_T16=
# 453
_tag_fat("%x%x%x%x",sizeof(char),9U);_T17=_tag_fat(_T1C,sizeof(void*),4);_T5=Cyc_aprintf(_T16,_T17);}}}}_T2(_T4,_T5);}}
# 457
static struct _tuple2*Cyc_Sexp_parse_ushort(struct Cyc_Sexp_Parser*p){unsigned short _T0;struct _tuple2*_T1;_T0=
Cyc_Sexp_parse_short(p);_T1=Cyc_Sexp_mk_ushort(_T0);return _T1;}
# 461
static void Cyc_Sexp_print_sshort(struct _tuple3*self,struct Cyc_Sexp_Printer*p){struct _tuple3*_T0;struct Cyc_Sexp_Printer*_T1;void(*_T2)(void*,struct _fat_ptr);struct Cyc_Sexp_Printer*_T3;void*_T4;struct _fat_ptr _T5;struct Cyc_Int_pa_PrintArg_struct _T6;short _T7;unsigned _T8;struct Cyc_Int_pa_PrintArg_struct _T9;short _TA;unsigned _TB;struct Cyc_Int_pa_PrintArg_struct _TC;short _TD;unsigned _TE;struct Cyc_Int_pa_PrintArg_struct _TF;short _T10;unsigned _T11;void**_T12;void**_T13;void**_T14;void**_T15;struct _fat_ptr _T16;struct _fat_ptr _T17;_T0=self;{
short x=_T0->v;_T1=p;_T2=_T1->print;_T3=p;_T4=_T3->env;{struct Cyc_Int_pa_PrintArg_struct _T18;_T18.tag=1;_T7=x;_T8=(unsigned)_T7;
# 464
_T18.f1=Cyc_Sexp_nibble(3U,_T8);_T6=_T18;}{struct Cyc_Int_pa_PrintArg_struct _T18=_T6;{struct Cyc_Int_pa_PrintArg_struct _T19;_T19.tag=1;_TA=x;_TB=(unsigned)_TA;_T19.f1=Cyc_Sexp_nibble(2U,_TB);_T9=_T19;}{struct Cyc_Int_pa_PrintArg_struct _T19=_T9;{struct Cyc_Int_pa_PrintArg_struct _T1A;_T1A.tag=1;_TD=x;_TE=(unsigned)_TD;_T1A.f1=Cyc_Sexp_nibble(1U,_TE);_TC=_T1A;}{struct Cyc_Int_pa_PrintArg_struct _T1A=_TC;{struct Cyc_Int_pa_PrintArg_struct _T1B;_T1B.tag=1;_T10=x;_T11=(unsigned)_T10;_T1B.f1=Cyc_Sexp_nibble(0U,_T11);_TF=_T1B;}{struct Cyc_Int_pa_PrintArg_struct _T1B=_TF;void*_T1C[4];_T12=_T1C + 0;*_T12=& _T18;_T13=_T1C + 1;*_T13=& _T19;_T14=_T1C + 2;*_T14=& _T1A;_T15=_T1C + 3;*_T15=& _T1B;_T16=
# 463
_tag_fat("%x%x%x%x",sizeof(char),9U);_T17=_tag_fat(_T1C,sizeof(void*),4);_T5=Cyc_aprintf(_T16,_T17);}}}}_T2(_T4,_T5);}}
# 467
static struct _tuple3*Cyc_Sexp_parse_sshort(struct Cyc_Sexp_Parser*p){unsigned short _T0;short _T1;struct _tuple3*_T2;_T0=
Cyc_Sexp_parse_short(p);_T1=(short)_T0;_T2=Cyc_Sexp_mk_sshort(_T1);return _T2;}
# 472
static unsigned char Cyc_Sexp_parse_char(struct Cyc_Sexp_Parser*p){unsigned _T0;unsigned _T1;unsigned _T2;unsigned char _T3;
unsigned res=0U;{
unsigned i=0U;_TL6F: if(i < 2U)goto _TL6D;else{goto _TL6E;}
_TL6D:{int ch=Cyc_Sexp_pgetc(p);
unsigned v=Cyc_Sexp_hex2value(p,ch);_T0=res << 4;_T1=v;
res=_T0 | _T1;}
# 474
i=i + 1;goto _TL6F;_TL6E:;}_T2=res;_T3=(unsigned char)_T2;
# 479
return _T3;}
# 482
static void Cyc_Sexp_print_uchar(struct _tuple0*self,struct Cyc_Sexp_Printer*p){struct _tuple0*_T0;struct Cyc_Sexp_Printer*_T1;void(*_T2)(void*,struct _fat_ptr);struct Cyc_Sexp_Printer*_T3;void*_T4;struct _fat_ptr _T5;struct Cyc_Int_pa_PrintArg_struct _T6;unsigned char _T7;unsigned _T8;struct Cyc_Int_pa_PrintArg_struct _T9;unsigned char _TA;unsigned _TB;void**_TC;void**_TD;struct _fat_ptr _TE;struct _fat_ptr _TF;_T0=self;{
unsigned char x=_T0->v;_T1=p;_T2=_T1->print;_T3=p;_T4=_T3->env;{struct Cyc_Int_pa_PrintArg_struct _T10;_T10.tag=1;_T7=x;_T8=(unsigned)_T7;
_T10.f1=Cyc_Sexp_nibble(1U,_T8);_T6=_T10;}{struct Cyc_Int_pa_PrintArg_struct _T10=_T6;{struct Cyc_Int_pa_PrintArg_struct _T11;_T11.tag=1;_TA=x;_TB=(unsigned)_TA;_T11.f1=Cyc_Sexp_nibble(0U,_TB);_T9=_T11;}{struct Cyc_Int_pa_PrintArg_struct _T11=_T9;void*_T12[2];_TC=_T12 + 0;*_TC=& _T10;_TD=_T12 + 1;*_TD=& _T11;_TE=_tag_fat("%x%x",sizeof(char),5U);_TF=_tag_fat(_T12,sizeof(void*),2);_T5=Cyc_aprintf(_TE,_TF);}}_T2(_T4,_T5);}}
# 487
static struct _tuple0*Cyc_Sexp_parse_uchar(struct Cyc_Sexp_Parser*p){unsigned char _T0;struct _tuple0*_T1;_T0=
Cyc_Sexp_parse_char(p);_T1=Cyc_Sexp_mk_uchar(_T0);return _T1;}
# 491
static void Cyc_Sexp_print_schar(struct _tuple1*self,struct Cyc_Sexp_Printer*p){struct _tuple1*_T0;struct Cyc_Sexp_Printer*_T1;void(*_T2)(void*,struct _fat_ptr);struct Cyc_Sexp_Printer*_T3;void*_T4;struct _fat_ptr _T5;struct Cyc_Int_pa_PrintArg_struct _T6;signed char _T7;unsigned _T8;struct Cyc_Int_pa_PrintArg_struct _T9;signed char _TA;unsigned _TB;void**_TC;void**_TD;struct _fat_ptr _TE;struct _fat_ptr _TF;_T0=self;{
signed char x=_T0->v;_T1=p;_T2=_T1->print;_T3=p;_T4=_T3->env;{struct Cyc_Int_pa_PrintArg_struct _T10;_T10.tag=1;_T7=x;_T8=(unsigned)_T7;
_T10.f1=Cyc_Sexp_nibble(1U,_T8);_T6=_T10;}{struct Cyc_Int_pa_PrintArg_struct _T10=_T6;{struct Cyc_Int_pa_PrintArg_struct _T11;_T11.tag=1;_TA=x;_TB=(unsigned)_TA;_T11.f1=Cyc_Sexp_nibble(0U,_TB);_T9=_T11;}{struct Cyc_Int_pa_PrintArg_struct _T11=_T9;void*_T12[2];_TC=_T12 + 0;*_TC=& _T10;_TD=_T12 + 1;*_TD=& _T11;_TE=_tag_fat("%x%x",sizeof(char),5U);_TF=_tag_fat(_T12,sizeof(void*),2);_T5=Cyc_aprintf(_TE,_TF);}}_T2(_T4,_T5);}}
# 496
static struct _tuple1*Cyc_Sexp_parse_schar(struct Cyc_Sexp_Parser*p){unsigned char _T0;signed char _T1;struct _tuple1*_T2;_T0=
Cyc_Sexp_parse_char(p);_T1=(signed char)_T0;_T2=Cyc_Sexp_mk_schar(_T1);return _T2;}union Cyc_Sexp_IntFloat{unsigned i;float f;};
# 503
static float Cyc_Sexp_int2float(unsigned i){union Cyc_Sexp_IntFloat _T0;union Cyc_Sexp_IntFloat _T1;float _T2;{union Cyc_Sexp_IntFloat _T3;
_T3.i=i;_T0=_T3;}{union Cyc_Sexp_IntFloat u=_T0;_T1=u;_T2=_T1.f;return _T2;}}
# 506
static unsigned Cyc_Sexp_float2int(float f){union Cyc_Sexp_IntFloat _T0;union Cyc_Sexp_IntFloat _T1;unsigned _T2;{union Cyc_Sexp_IntFloat _T3;
_T3.f=f;_T0=_T3;}{union Cyc_Sexp_IntFloat u=_T0;_T1=u;_T2=_T1.i;return _T2;}}union Cyc_Sexp_DoubleLongLong{unsigned long long x;double d;};
# 510
static double Cyc_Sexp_longlong2double(unsigned long long x){union Cyc_Sexp_DoubleLongLong _T0;union Cyc_Sexp_DoubleLongLong _T1;double _T2;{union Cyc_Sexp_DoubleLongLong _T3;
_T3.x=x;_T0=_T3;}{union Cyc_Sexp_DoubleLongLong u=_T0;_T1=u;_T2=_T1.d;return _T2;}}
# 513
static unsigned long long Cyc_Sexp_double2longlong(double d){union Cyc_Sexp_DoubleLongLong _T0;union Cyc_Sexp_DoubleLongLong _T1;unsigned long long _T2;{union Cyc_Sexp_DoubleLongLong _T3;
_T3.d=d;_T0=_T3;}{union Cyc_Sexp_DoubleLongLong u=_T0;_T1=u;_T2=_T1.x;return _T2;}}
# 518
static int Cyc_Sexp_hash_float(struct _tuple8*self){struct _tuple8*_T0;float _T1;unsigned _T2;int _T3;_T0=self;_T1=_T0->v;_T2=Cyc_Sexp_float2int(_T1);_T3=(int)_T2;return _T3;}
# 520
static void Cyc_Sexp_print_float(struct _tuple8*self,struct Cyc_Sexp_Printer*p){struct _tuple8*_T0;struct Cyc_Sexp_Printer*_T1;void(*_T2)(void*,struct _fat_ptr);struct Cyc_Sexp_Printer*_T3;void*_T4;struct _fat_ptr _T5;struct Cyc_Int_pa_PrintArg_struct _T6;void**_T7;struct _fat_ptr _T8;struct _fat_ptr _T9;_T0=self;{
float x=_T0->v;_T1=p;_T2=_T1->print;_T3=p;_T4=_T3->env;{struct Cyc_Int_pa_PrintArg_struct _TA;_TA.tag=1;
_TA.f1=Cyc_Sexp_float2int(x);_T6=_TA;}{struct Cyc_Int_pa_PrintArg_struct _TA=_T6;void*_TB[1];_T7=_TB + 0;*_T7=& _TA;_T8=_tag_fat("%08x",sizeof(char),5U);_T9=_tag_fat(_TB,sizeof(void*),1);_T5=Cyc_aprintf(_T8,_T9);}_T2(_T4,_T5);}}
# 525
static struct _tuple8*Cyc_Sexp_parse_float(struct Cyc_Sexp_Parser*p){unsigned _T0;float _T1;struct _tuple8*_T2;_T0=
Cyc_Sexp_parse_int(p);_T1=Cyc_Sexp_int2float(_T0);_T2=Cyc_Sexp_mk_float(_T1);return _T2;}
# 529
static int Cyc_Sexp_cmp_float(struct _tuple8*x,struct _tuple8*y){struct _tuple8*_T0;float _T1;struct _tuple8*_T2;float _T3;float _T4;double _T5;float _T6;double _T7;_T0=x;_T1=_T0->v;_T2=y;_T3=_T2->v;{
float diff=_T1 - _T3;_T4=diff;_T5=(double)_T4;
if(_T5 >= 0.0)goto _TL70;return -1;
_TL70: _T6=diff;_T7=(double)_T6;if(_T7 <= 0.0)goto _TL72;return 1;
_TL72: return 0;}}
# 537
static int Cyc_Sexp_hash_double(struct _tuple9*self){struct _tuple9*_T0;double _T1;unsigned long long _T2;unsigned _T3;int _T4;_T0=self;_T1=_T0->v;_T2=
Cyc_Sexp_double2longlong(_T1);_T3=(unsigned)_T2;_T4=(int)_T3;return _T4;}
# 541
static void Cyc_Sexp_print_double(struct _tuple9*self,struct Cyc_Sexp_Printer*p){struct _tuple9*_T0;struct Cyc_Sexp_Printer*_T1;void(*_T2)(void*,struct _fat_ptr);struct Cyc_Sexp_Printer*_T3;void*_T4;struct _fat_ptr _T5;struct Cyc_Int_pa_PrintArg_struct _T6;unsigned long long _T7;struct Cyc_Int_pa_PrintArg_struct _T8;unsigned long long _T9;void**_TA;void**_TB;struct _fat_ptr _TC;struct _fat_ptr _TD;_T0=self;{
double d=_T0->v;
unsigned long long x=Cyc_Sexp_double2longlong(d);_T1=p;_T2=_T1->print;_T3=p;_T4=_T3->env;{struct Cyc_Int_pa_PrintArg_struct _TE;_TE.tag=1;_T7=x >> 32;
_TE.f1=(unsigned)_T7;_T6=_TE;}{struct Cyc_Int_pa_PrintArg_struct _TE=_T6;{struct Cyc_Int_pa_PrintArg_struct _TF;_TF.tag=1;_T9=x;_TF.f1=(unsigned)_T9;_T8=_TF;}{struct Cyc_Int_pa_PrintArg_struct _TF=_T8;void*_T10[2];_TA=_T10 + 0;*_TA=& _TE;_TB=_T10 + 1;*_TB=& _TF;_TC=_tag_fat("%08x%08x",sizeof(char),9U);_TD=_tag_fat(_T10,sizeof(void*),2);_T5=Cyc_aprintf(_TC,_TD);}}_T2(_T4,_T5);}}
# 548
static struct _tuple9*Cyc_Sexp_parse_double(struct Cyc_Sexp_Parser*p){unsigned long long _T0;double _T1;struct _tuple9*_T2;_T0=
Cyc_Sexp_parse_longlong(p);_T1=Cyc_Sexp_longlong2double(_T0);_T2=Cyc_Sexp_mk_double(_T1);return _T2;}
# 552
static int Cyc_Sexp_cmp_double(struct _tuple9*x,struct _tuple9*y){struct _tuple9*_T0;double _T1;struct _tuple9*_T2;double _T3;_T0=x;_T1=_T0->v;_T2=y;_T3=_T2->v;{
double diff=_T1 - _T3;
if(diff >= 0.0)goto _TL74;return -1;
_TL74: if(diff <= 0.0)goto _TL76;return 1;
_TL76: return 0;}}
# 560
static int Cyc_Sexp_hash_str(struct _tuple10*self){struct _tuple10*_T0;struct _fat_ptr _T1;int _T2;_T0=self;_T1=_T0->v;_T2=
Cyc_Hashtable_hash_string(_T1);return _T2;}
# 564
static int Cyc_Sexp_hash_symbol(struct _tuple10*self){struct _tuple10*_T0;struct _fat_ptr _T1;int _T2;_T0=self;_T1=_T0->v;_T2=
Cyc_Hashtable_hash_string(_T1);return _T2;}
# 568
static void Cyc_Sexp_print_symbol(struct _tuple10*self,struct Cyc_Sexp_Printer*p){struct Cyc_Sexp_Printer*_T0;void(*_T1)(void*,struct _fat_ptr);struct Cyc_Sexp_Printer*_T2;void*_T3;struct _tuple10*_T4;struct _fat_ptr _T5;_T0=p;_T1=_T0->print;_T2=p;_T3=_T2->env;_T4=self;_T5=_T4->v;
# 570
_T1(_T3,_T5);}
# 576
static struct _tuple10*Cyc_Sexp_parse_symbol(struct Cyc_Sexp_Parser*p){int _T0;struct Cyc_Sexp_Parser*_T1;int _T2;struct _fat_ptr _T3;int _T4;struct Cyc_List_List*_T5;int _T6;struct _fat_ptr _T7;int _T8;void*_T9;struct _fat_ptr _TA;unsigned _TB;int _TC;char*_TD;char*_TE;struct Cyc_List_List*_TF;void*_T10;int _T11;unsigned _T12;unsigned char*_T13;char*_T14;struct Cyc_List_List*_T15;struct _tuple10*_T16;
struct Cyc_List_List*chars=0;
int ch=Cyc_Sexp_pgetc(p);
_TL78: _T0=Cyc_Sexp_whitespace(ch);if(_T0)goto _TL79;else{goto _TL7A;}_TL79: ch=Cyc_Sexp_pgetc(p);goto _TL78;_TL7A:
 _TL7B: if(1)goto _TL7C;else{goto _TL7D;}
_TL7C: if(ch!=-1)goto _TL7E;_T1=p;_T2=ch;_T3=_tag_fat("unexpected end of file",sizeof(char),23U);Cyc_Sexp_perror(_T1,_T2,_T3);goto _TL7F;_TL7E: _TL7F: _T4=
Cyc_Sexp_whitespace(ch);if(_T4)goto _TL82;else{goto _TL83;}_TL83: if(ch==41)goto _TL82;else{goto _TL80;}
_TL82: Cyc_Sexp_pungetc(p,ch);goto _TL7D;_TL80:{struct Cyc_List_List*_T17=_cycalloc(sizeof(struct Cyc_List_List));_T6=ch;
# 586
_T17->hd=(void*)_T6;_T17->tl=chars;_T5=(struct Cyc_List_List*)_T17;}chars=_T5;
ch=Cyc_Sexp_pgetc(p);goto _TL7B;_TL7D:
# 589
 chars=Cyc_List_imp_rev(chars);_T8=
Cyc_List_length(chars);{unsigned _T17=_T8 + 1;_T9=_cyccalloc_atomic(sizeof(char),_T17);_T7=_tag_fat(_T9,sizeof(char),_T17);}{struct _fat_ptr buf=_T7;{
unsigned i=0U;_TL87: if(chars!=0)goto _TL85;else{goto _TL86;}
_TL85: _TA=buf;_TB=i;_TC=(int)_TB;{struct _fat_ptr _T17=_fat_ptr_plus(_TA,sizeof(char),_TC);_TD=_check_fat_subscript(_T17,sizeof(char),0U);_TE=(char*)_TD;{char _T18=*_TE;_TF=chars;_T10=_TF->hd;_T11=(int)_T10;{char _T19=(char)_T11;_T12=_get_fat_size(_T17,sizeof(char));if(_T12!=1U)goto _TL88;if(_T18!=0)goto _TL88;if(_T19==0)goto _TL88;_throw_arraybounds();goto _TL89;_TL88: _TL89: _T13=_T17.curr;_T14=(char*)_T13;*_T14=_T19;}}}_T15=chars;
# 591
chars=_T15->tl;i=i + 1;goto _TL87;_TL86:;}_T16=
# 593
Cyc_Sexp_mk_symbol(buf);return _T16;}}
# 597
static struct _fat_ptr Cyc_Sexp_escape(struct _fat_ptr s){struct _fat_ptr _T0;unsigned char*_T1;const char*_T2;unsigned _T3;int _T4;char _T5;int _T6;struct _fat_ptr _T7;unsigned char*_T8;const char*_T9;unsigned _TA;int _TB;char _TC;int _TD;struct _fat_ptr _TE;struct _fat_ptr _TF;unsigned _T10;int _T11;unsigned _T12;unsigned _T13;unsigned _T14;char*_T15;unsigned _T16;unsigned _T17;int _T18;unsigned _T19;unsigned _T1A;char*_T1B;unsigned _T1C;char*_T1D;struct _fat_ptr _T1E;unsigned char*_T1F;const char*_T20;unsigned _T21;int _T22;struct _fat_ptr _T23;unsigned char*_T24;const char*_T25;unsigned _T26;int _T27;char _T28;int _T29;struct _fat_ptr _T2A;unsigned char*_T2B;const char*_T2C;unsigned _T2D;int _T2E;char _T2F;int _T30;struct _fat_ptr _T31;unsigned _T32;int _T33;char*_T34;char*_T35;unsigned _T36;unsigned char*_T37;char*_T38;struct _fat_ptr _T39;unsigned _T3A;int _T3B;char*_T3C;char*_T3D;unsigned _T3E;unsigned char*_T3F;char*_T40;struct _fat_ptr _T41;
unsigned n=Cyc_strlen(s);
int escapes=0;{
unsigned i=0U;_TL8D: if(i < n)goto _TL8B;else{goto _TL8C;}
_TL8B: _T0=s;_T1=_T0.curr;_T2=(const char*)_T1;_T3=i;_T4=(int)_T3;_T5=_T2[_T4];_T6=(int)_T5;if(_T6==34)goto _TL90;else{goto _TL91;}_TL91: _T7=s;_T8=_T7.curr;_T9=(const char*)_T8;_TA=i;_TB=(int)_TA;_TC=_T9[_TB];_TD=(int)_TC;if(_TD==92)goto _TL90;else{goto _TL8E;}_TL90: escapes=escapes + 1;goto _TL8F;_TL8E: _TL8F:
# 600
 i=i + 1;goto _TL8D;_TL8C:;}
# 603
if(escapes!=0)goto _TL92;_TE=s;return _TE;_TL92: _T10=n;_T11=escapes;_T12=(unsigned)_T11;_T13=_T10 + _T12;_T14=_T13 + 1U;{unsigned _T42=_T14 + 1U;_T16=_check_times(_T42,sizeof(char));{char*_T43=_cycalloc_atomic(_T16);_T17=n;_T18=escapes;_T19=(unsigned)_T18;_T1A=_T17 + _T19;{unsigned _T44=_T1A + 1U;unsigned i;i=0;_TL97: if(i < _T44)goto _TL95;else{goto _TL96;}_TL95: _T1C=i;_T1B=_T43 + _T1C;
*_T1B='\000';i=i + 1;goto _TL97;_TL96: _T1D=_T43 + _T44;*_T1D=0;}_T15=(char*)_T43;}_TF=_tag_fat(_T15,sizeof(char),_T42);}{struct _fat_ptr news=_TF;
unsigned pos=0U;{
unsigned i=0U;_TL9B: if(i < n)goto _TL99;else{goto _TL9A;}
_TL99: _T1E=s;_T1F=_T1E.curr;_T20=(const char*)_T1F;_T21=i;_T22=(int)_T21;{char ch=_T20[_T22];_T23=s;_T24=_T23.curr;_T25=(const char*)_T24;_T26=i;_T27=(int)_T26;_T28=_T25[_T27];_T29=(int)_T28;
if(_T29==34)goto _TL9E;else{goto _TL9F;}_TL9F: _T2A=s;_T2B=_T2A.curr;_T2C=(const char*)_T2B;_T2D=i;_T2E=(int)_T2D;_T2F=_T2C[_T2E];_T30=(int)_T2F;if(_T30==92)goto _TL9E;else{goto _TL9C;}
_TL9E: _T31=news;_T32=pos;_T33=(int)_T32;{struct _fat_ptr _T42=_fat_ptr_plus(_T31,sizeof(char),_T33);_T34=_check_fat_subscript(_T42,sizeof(char),0U);_T35=(char*)_T34;{char _T43=*_T35;char _T44='\\';_T36=_get_fat_size(_T42,sizeof(char));if(_T36!=1U)goto _TLA0;if(_T43!=0)goto _TLA0;if(_T44==0)goto _TLA0;_throw_arraybounds();goto _TLA1;_TLA0: _TLA1: _T37=_T42.curr;_T38=(char*)_T37;*_T38=_T44;}}
pos=pos + 1;goto _TL9D;_TL9C: _TL9D: _T39=news;_T3A=pos;_T3B=(int)_T3A;{struct _fat_ptr _T42=_fat_ptr_plus(_T39,sizeof(char),_T3B);_T3C=_check_fat_subscript(_T42,sizeof(char),0U);_T3D=(char*)_T3C;{char _T43=*_T3D;char _T44=ch;_T3E=_get_fat_size(_T42,sizeof(char));if(_T3E!=1U)goto _TLA2;if(_T43!=0)goto _TLA2;if(_T44==0)goto _TLA2;_throw_arraybounds();goto _TLA3;_TLA2: _TLA3: _T3F=_T42.curr;_T40=(char*)_T3F;*_T40=_T44;}}
# 613
pos=pos + 1;}
# 606
i=i + 1;goto _TL9B;_TL9A:;}_T41=news;
# 615
return _T41;}}
# 619
static void Cyc_Sexp_print_str(struct _tuple10*self,struct Cyc_Sexp_Printer*p){struct Cyc_Sexp_Printer*_T0;void(*_T1)(void*,struct _fat_ptr);struct Cyc_Sexp_Printer*_T2;void*_T3;struct _fat_ptr _T4;struct Cyc_Sexp_Printer*_T5;void(*_T6)(void*,struct _fat_ptr);struct Cyc_Sexp_Printer*_T7;void*_T8;struct _tuple10*_T9;struct _fat_ptr _TA;struct _fat_ptr _TB;struct Cyc_Sexp_Printer*_TC;void(*_TD)(void*,struct _fat_ptr);struct Cyc_Sexp_Printer*_TE;void*_TF;struct _fat_ptr _T10;_T0=p;_T1=_T0->print;_T2=p;_T3=_T2->env;_T4=
_tag_fat("\"",sizeof(char),2U);_T1(_T3,_T4);_T5=p;_T6=_T5->print;_T7=p;_T8=_T7->env;_T9=self;_TA=_T9->v;_TB=
Cyc_Sexp_escape(_TA);_T6(_T8,_TB);_TC=p;_TD=_TC->print;_TE=p;_TF=_TE->env;_T10=
_tag_fat("\"",sizeof(char),2U);_TD(_TF,_T10);}
# 626
static int Cyc_Sexp_cmp_str(struct _tuple10*x,struct _tuple10*y){struct _tuple10*_T0;struct _fat_ptr _T1;struct _tuple10*_T2;struct _fat_ptr _T3;int _T4;_T0=x;_T1=_T0->v;_T2=y;_T3=_T2->v;_T4=
Cyc_strcmp(_T1,_T3);return _T4;}
# 630
static int Cyc_Sexp_cmp_symbol(struct _tuple10*x,struct _tuple10*y){struct _tuple10*_T0;struct _fat_ptr _T1;struct _tuple10*_T2;struct _fat_ptr _T3;int _T4;_T0=x;_T1=_T0->v;_T2=y;_T3=_T2->v;_T4=
Cyc_strcmp(_T1,_T3);return _T4;}
# 637
static struct _tuple10*Cyc_Sexp_parse_str(struct Cyc_Sexp_Parser*p){struct Cyc_Sexp_Parser*_T0;int _T1;struct _fat_ptr _T2;struct Cyc_Sexp_Parser*_T3;int _T4;struct _fat_ptr _T5;struct Cyc_List_List*_T6;int _T7;struct _fat_ptr _T8;int _T9;void*_TA;struct _fat_ptr _TB;unsigned _TC;int _TD;char*_TE;char*_TF;struct Cyc_List_List*_T10;void*_T11;int _T12;unsigned _T13;unsigned char*_T14;char*_T15;struct Cyc_List_List*_T16;struct _tuple10*_T17;
Cyc_Sexp_expectws(p,34);{
struct Cyc_List_List*chars=0;
_TLA4: if(1)goto _TLA5;else{goto _TLA6;}
_TLA5:{int ch=Cyc_Sexp_pgetc(p);
if(ch!=-1)goto _TLA7;_T0=p;_T1=ch;_T2=_tag_fat("unexpected end of file",sizeof(char),23U);Cyc_Sexp_perror(_T0,_T1,_T2);goto _TLA8;_TLA7: _TLA8:
 if(ch!=34)goto _TLA9;goto _TLA6;_TLA9:
 if(ch!=92)goto _TLAB;
ch=Cyc_Sexp_pgetc(p);
if(ch==34)goto _TLAD;if(ch==92)goto _TLAD;_T3=p;_T4=ch;_T5=_tag_fat("expected '\"' or '\\'",sizeof(char),20U);Cyc_Sexp_perror(_T3,_T4,_T5);goto _TLAE;_TLAD: _TLAE: goto _TLAC;_TLAB: _TLAC:{struct Cyc_List_List*_T18=_cycalloc(sizeof(struct Cyc_List_List));_T7=ch;
# 648
_T18->hd=(void*)_T7;_T18->tl=chars;_T6=(struct Cyc_List_List*)_T18;}chars=_T6;}goto _TLA4;_TLA6:
# 650
 chars=Cyc_List_imp_rev(chars);_T9=
Cyc_List_length(chars);{unsigned _T18=_T9 + 1;_TA=_cyccalloc_atomic(sizeof(char),_T18);_T8=_tag_fat(_TA,sizeof(char),_T18);}{struct _fat_ptr buf=_T8;{
unsigned i=0U;_TLB2: if(chars!=0)goto _TLB0;else{goto _TLB1;}
_TLB0: _TB=buf;_TC=i;_TD=(int)_TC;{struct _fat_ptr _T18=_fat_ptr_plus(_TB,sizeof(char),_TD);_TE=_check_fat_subscript(_T18,sizeof(char),0U);_TF=(char*)_TE;{char _T19=*_TF;_T10=chars;_T11=_T10->hd;_T12=(int)_T11;{char _T1A=(char)_T12;_T13=_get_fat_size(_T18,sizeof(char));if(_T13!=1U)goto _TLB3;if(_T19!=0)goto _TLB3;if(_T1A==0)goto _TLB3;_throw_arraybounds();goto _TLB4;_TLB3: _TLB4: _T14=_T18.curr;_T15=(char*)_T14;*_T15=_T1A;}}}_T16=chars;
# 652
chars=_T16->tl;i=i + 1;goto _TLB2;_TLB1:;}_T17=
# 654
Cyc_Sexp_mk_str(buf);return _T17;}}}
# 658
static void Cyc_Sexp_print_tuple(struct _tuple10*self,struct Cyc_Sexp_Printer*p){struct _tuple10*_T0;struct _fat_ptr _T1;struct Cyc_Sexp_Printer*_T2;struct _fat_ptr _T3;unsigned char*_T4;struct Cyc_Sexp_Object*_T5;unsigned _T6;int _T7;struct Cyc_Sexp_Object _T8;_T0=self;{
# 660
struct _fat_ptr vs=_T0->v;_T1=vs;{
unsigned n=_get_fat_size(_T1,sizeof(struct Cyc_Sexp_Object));
unsigned i=0U;_TLB8: if(i < n)goto _TLB6;else{goto _TLB7;}
_TLB6: _T2=p;_T3=vs;_T4=_T3.curr;_T5=(struct Cyc_Sexp_Object*)_T4;_T6=i;_T7=(int)_T6;_T8=_T5[_T7];Cyc_Sexp_print(_T2,_T8);
# 662
i=i + 1;goto _TLB8;_TLB7:;}}}
# 669
static int Cyc_Sexp_hash_tuple(struct _tuple10*x){struct _tuple10*_T0;struct _fat_ptr _T1;struct _fat_ptr _T2;unsigned char*_T3;struct Cyc_Sexp_Object*_T4;unsigned _T5;int _T6;unsigned _T7;struct Cyc_Sexp_Obj*_T8;unsigned _T9;unsigned _TA;unsigned _TB;int _TC;
unsigned res=0U;_T0=x;{
struct _fat_ptr vs=_T0->v;_T1=vs;{
unsigned n=_get_fat_size(_T1,sizeof(struct Cyc_Sexp_Object));{
unsigned i=0U;_TLBC: if(i < n)goto _TLBA;else{goto _TLBB;}
_TLBA: _T2=vs;_T3=_T2.curr;_T4=(struct Cyc_Sexp_Object*)_T3;_T5=i;_T6=(int)_T5;{struct Cyc_Sexp_Object _TD=_T4[_T6];struct Cyc_Sexp_Obj*_TE;_TE=_TD.self;{struct Cyc_Sexp_Obj*v=_TE;_T7=res << 8;_T8=v;_T9=(unsigned)_T8;_TA=_T9 & 255U;
res=_T7 | _TA;}}
# 673
i=i + 1;goto _TLBC;_TLBB:;}_TB=res;_TC=(int)_TB;
# 677
return _TC;}}}
# 681
static int Cyc_Sexp_cmp_tuple(struct _tuple10*x,struct _tuple10*y){struct _tuple10*_T0;struct _tuple10*_T1;struct _fat_ptr _T2;struct _fat_ptr _T3;struct _fat_ptr _T4;unsigned char*_T5;struct Cyc_Sexp_Object*_T6;unsigned _T7;int _T8;struct Cyc_Sexp_Object _T9;struct _fat_ptr _TA;unsigned char*_TB;struct Cyc_Sexp_Object*_TC;unsigned _TD;int _TE;struct Cyc_Sexp_Object _TF;int _T10;
if(x!=y)goto _TLBD;return 0;_TLBD: _T0=x;{
struct _fat_ptr xs=_T0->v;_T1=y;{
struct _fat_ptr ys=_T1->v;_T2=xs;{
unsigned nx=_get_fat_size(_T2,sizeof(struct Cyc_Sexp_Object));_T3=ys;{
unsigned ny=_get_fat_size(_T3,sizeof(struct Cyc_Sexp_Object));
if(nx >= ny)goto _TLBF;return -1;_TLBF:
 if(nx <= ny)goto _TLC1;return 1;_TLC1:{
unsigned i=0U;_TLC6: if(i < nx)goto _TLC4;else{goto _TLC5;}
_TLC4: _T4=xs;_T5=_T4.curr;_T6=(struct Cyc_Sexp_Object*)_T5;_T7=i;_T8=(int)_T7;_T9=_T6[_T8];_TA=ys;_TB=_TA.curr;_TC=(struct Cyc_Sexp_Object*)_TB;_TD=i;_TE=(int)_TD;_TF=_TC[_TE];{int c=Cyc_Sexp_cmp(_T9,_TF);
if(c==0)goto _TLC7;_T10=c;return _T10;_TLC7:;}
# 689
i=i + 1;goto _TLC6;_TLC5:;}
# 693
return 0;}}}}}
# 697
static struct Cyc_Sexp_Object Cyc_Sexp_next_list(struct Cyc_List_List**xs){struct Cyc_List_List**_T0;struct Cyc_List_List*_T1;struct Cyc_List_List*_T2;void*_T3;struct Cyc_List_List**_T4;struct Cyc_List_List**_T5;struct Cyc_List_List*_T6;struct Cyc_Sexp_Object*_T7;struct Cyc_Sexp_Object _T8;_T0=xs;_T1=*_T0;_T2=
_check_null(_T1);_T3=_T2->hd;{struct Cyc_Sexp_Object*h=(struct Cyc_Sexp_Object*)_T3;_T4=xs;_T5=xs;_T6=*_T5;
*_T4=_T6->tl;_T7=h;_T8=*_T7;
return _T8;}}
# 704
static struct _tuple10*Cyc_Sexp_parse_tuple(struct Cyc_Sexp_Parser*p){int _T0;struct _fat_ptr _T1;int _T2;struct Cyc_Sexp_Object*_T3;unsigned _T4;int _T5;struct Cyc_Sexp_Object*_T6;unsigned _T7;struct Cyc_List_List**_T8;struct _tuple10*_T9;struct Cyc_List_List*_TA;void*_TB;int _TC;struct Cyc_Sexp_Class*_TD;char _TE;int _TF;struct Cyc_Sexp_Class*_T10;struct Cyc_Sexp_Obj*(*_T11)(struct Cyc_Sexp_Parser*);struct Cyc_List_List*_T12;struct Cyc_Sexp_Object*_T13;struct Cyc_List_List*_T14;struct Cyc_Sexp_Parser*_T15;void(*_T16)(void*,int,struct _fat_ptr);struct Cyc_Sexp_Parser*_T17;void*_T18;int _T19;struct _fat_ptr _T1A;
struct Cyc_List_List*xs=0;
Next: {
int ch=Cyc_Sexp_pgetc(p);
_TLC9: _T0=Cyc_Sexp_whitespace(ch);if(_T0)goto _TLCA;else{goto _TLCB;}_TLCA: ch=Cyc_Sexp_pgetc(p);goto _TLC9;_TLCB:
 if(ch!=41)goto _TLCC;
Cyc_Sexp_pungetc(p,ch);
xs=Cyc_List_imp_rev(xs);{
int len=Cyc_List_length(xs);_T2=len;{unsigned _T1B=(unsigned)_T2;_T4=_check_times(_T1B,sizeof(struct Cyc_Sexp_Object));{struct Cyc_Sexp_Object*_T1C=_cycalloc(_T4);_T5=len;{unsigned _T1D=(unsigned)_T5;unsigned i;i=0;_TLD1: if(i < _T1D)goto _TLCF;else{goto _TLD0;}_TLCF: _T7=i;_T6=_T1C + _T7;_T8=& xs;
*_T6=Cyc_Sexp_next_list(_T8);i=i + 1;goto _TLD1;_TLD0:;}_T3=(struct Cyc_Sexp_Object*)_T1C;}_T1=_tag_fat(_T3,sizeof(struct Cyc_Sexp_Object),_T1B);}{struct _fat_ptr vs=_T1;_T9=
Cyc_Sexp_mk_tuple(vs);return _T9;}}_TLCC:{
# 716
struct Cyc_List_List*cs=Cyc_Sexp_classes;_TLD5: if(cs!=0)goto _TLD3;else{goto _TLD4;}
_TLD3: _TA=cs;_TB=_TA->hd;{struct Cyc_Sexp_Cls*_T1B=(struct Cyc_Sexp_Cls*)_TB;struct Cyc_Sexp_Class*_T1C;{struct Cyc_Sexp_Cls _T1D=*_T1B;_T1C=_T1D.vtable;}{struct Cyc_Sexp_Class*v=_T1C;_TC=ch;_TD=v;_TE=_TD->tag;_TF=(int)_TE;
if(_TC!=_TF)goto _TLD6;
Cyc_Sexp_expectws(p,40);_T10=v;_T11=_T10->parse;{
struct Cyc_Sexp_Obj*obj=_T11(p);
Cyc_Sexp_expectws(p,41);{struct Cyc_List_List*_T1D=_cycalloc(sizeof(struct Cyc_List_List));{struct Cyc_Sexp_Object*_T1E=_cycalloc(sizeof(struct Cyc_Sexp_Object));
*_T1E=Cyc_Sexp_up(obj);_T13=(struct Cyc_Sexp_Object*)_T1E;}_T1D->hd=_T13;_T1D->tl=xs;_T12=(struct Cyc_List_List*)_T1D;}xs=_T12;goto Next;}_TLD6:;}}_T14=cs;
# 716
cs=_T14->tl;goto _TLD5;_TLD4:;}_T15=p;_T16=_T15->error;_T17=p;_T18=_T17->env;_T19=ch;_T1A=
# 726
_tag_fat("unexpected tag",sizeof(char),15U);_T16(_T18,_T19,_T1A);}}static char _TmpG0[14U]="unsigned char";
# 743 "sexp.cyc"
struct Cyc_Sexp_Class Cyc_Sexp_uchar_class={(struct Cyc_Sexp_Obj*)0,'C',{_TmpG0,_TmpG0,_TmpG0 + 14U},(void(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Printer*))Cyc_Sexp_print_uchar,(struct Cyc_Sexp_Obj*(*)(struct Cyc_Sexp_Parser*))Cyc_Sexp_parse_uchar,(int(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Obj*))Cyc_Sexp_cmp_uchar,(int(*)(struct Cyc_Sexp_Obj*))Cyc_Sexp_hash_uchar,(void*(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Visitor*,void*))Cyc_Sexp_accept_uchar,0};static char _TmpG1[12U]="signed char";
struct Cyc_Sexp_Class Cyc_Sexp_schar_class={(struct Cyc_Sexp_Obj*)0,'c',{_TmpG1,_TmpG1,_TmpG1 + 12U},(void(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Printer*))Cyc_Sexp_print_schar,(struct Cyc_Sexp_Obj*(*)(struct Cyc_Sexp_Parser*))Cyc_Sexp_parse_schar,(int(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Obj*))Cyc_Sexp_cmp_schar,(int(*)(struct Cyc_Sexp_Obj*))Cyc_Sexp_hash_schar,(void*(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Visitor*,void*))Cyc_Sexp_accept_schar,0};static char _TmpG2[15U]="unsigned short";
struct Cyc_Sexp_Class Cyc_Sexp_ushort_class={(struct Cyc_Sexp_Obj*)0,'S',{_TmpG2,_TmpG2,_TmpG2 + 15U},(void(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Printer*))Cyc_Sexp_print_ushort,(struct Cyc_Sexp_Obj*(*)(struct Cyc_Sexp_Parser*))Cyc_Sexp_parse_ushort,(int(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Obj*))Cyc_Sexp_cmp_ushort,(int(*)(struct Cyc_Sexp_Obj*))Cyc_Sexp_hash_ushort,(void*(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Visitor*,void*))Cyc_Sexp_accept_ushort,0};static char _TmpG3[13U]="signed short";
struct Cyc_Sexp_Class Cyc_Sexp_sshort_class={(struct Cyc_Sexp_Obj*)0,'s',{_TmpG3,_TmpG3,_TmpG3 + 13U},(void(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Printer*))Cyc_Sexp_print_sshort,(struct Cyc_Sexp_Obj*(*)(struct Cyc_Sexp_Parser*))Cyc_Sexp_parse_sshort,(int(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Obj*))Cyc_Sexp_cmp_sshort,(int(*)(struct Cyc_Sexp_Obj*))Cyc_Sexp_hash_sshort,(void*(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Visitor*,void*))Cyc_Sexp_accept_sshort,0};static char _TmpG4[13U]="unsigned int";
struct Cyc_Sexp_Class Cyc_Sexp_uint_class={(struct Cyc_Sexp_Obj*)0,'I',{_TmpG4,_TmpG4,_TmpG4 + 13U},(void(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Printer*))Cyc_Sexp_print_uint,(struct Cyc_Sexp_Obj*(*)(struct Cyc_Sexp_Parser*))Cyc_Sexp_parse_uint,(int(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Obj*))Cyc_Sexp_cmp_uint,(int(*)(struct Cyc_Sexp_Obj*))Cyc_Sexp_hash_uint,(void*(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Visitor*,void*))Cyc_Sexp_accept_uint,0};static char _TmpG5[11U]="signed int";
struct Cyc_Sexp_Class Cyc_Sexp_sint_class={(struct Cyc_Sexp_Obj*)0,'i',{_TmpG5,_TmpG5,_TmpG5 + 11U},(void(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Printer*))Cyc_Sexp_print_sint,(struct Cyc_Sexp_Obj*(*)(struct Cyc_Sexp_Parser*))Cyc_Sexp_parse_sint,(int(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Obj*))Cyc_Sexp_cmp_sint,(int(*)(struct Cyc_Sexp_Obj*))Cyc_Sexp_hash_sint,(void*(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Visitor*,void*))Cyc_Sexp_accept_sint,0};static char _TmpG6[19U]="unsigned long long";
struct Cyc_Sexp_Class Cyc_Sexp_ulonglong_class={(struct Cyc_Sexp_Obj*)0,'L',{_TmpG6,_TmpG6,_TmpG6 + 19U},(void(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Printer*))Cyc_Sexp_print_ulonglong,(struct Cyc_Sexp_Obj*(*)(struct Cyc_Sexp_Parser*))Cyc_Sexp_parse_ulonglong,(int(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Obj*))Cyc_Sexp_cmp_ulonglong,(int(*)(struct Cyc_Sexp_Obj*))Cyc_Sexp_hash_ulonglong,(void*(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Visitor*,void*))Cyc_Sexp_accept_ulonglong,0};static char _TmpG7[17U]="signed long long";
struct Cyc_Sexp_Class Cyc_Sexp_slonglong_class={(struct Cyc_Sexp_Obj*)0,'l',{_TmpG7,_TmpG7,_TmpG7 + 17U},(void(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Printer*))Cyc_Sexp_print_slonglong,(struct Cyc_Sexp_Obj*(*)(struct Cyc_Sexp_Parser*))Cyc_Sexp_parse_slonglong,(int(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Obj*))Cyc_Sexp_cmp_slonglong,(int(*)(struct Cyc_Sexp_Obj*))Cyc_Sexp_hash_slonglong,(void*(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Visitor*,void*))Cyc_Sexp_accept_slonglong,0};static char _TmpG8[6U]="float";
struct Cyc_Sexp_Class Cyc_Sexp_float_class={(struct Cyc_Sexp_Obj*)0,'f',{_TmpG8,_TmpG8,_TmpG8 + 6U},(void(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Printer*))Cyc_Sexp_print_float,(struct Cyc_Sexp_Obj*(*)(struct Cyc_Sexp_Parser*))Cyc_Sexp_parse_float,(int(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Obj*))Cyc_Sexp_cmp_float,(int(*)(struct Cyc_Sexp_Obj*))Cyc_Sexp_hash_float,(void*(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Visitor*,void*))Cyc_Sexp_accept_float,0};static char _TmpG9[7U]="double";
struct Cyc_Sexp_Class Cyc_Sexp_double_class={(struct Cyc_Sexp_Obj*)0,'d',{_TmpG9,_TmpG9,_TmpG9 + 7U},(void(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Printer*))Cyc_Sexp_print_double,(struct Cyc_Sexp_Obj*(*)(struct Cyc_Sexp_Parser*))Cyc_Sexp_parse_double,(int(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Obj*))Cyc_Sexp_cmp_double,(int(*)(struct Cyc_Sexp_Obj*))Cyc_Sexp_hash_double,(void*(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Visitor*,void*))Cyc_Sexp_accept_double,0};static char _TmpGA[7U]="string";
struct Cyc_Sexp_Class Cyc_Sexp_str_class={(struct Cyc_Sexp_Obj*)0,'r',{_TmpGA,_TmpGA,_TmpGA + 7U},(void(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Printer*))Cyc_Sexp_print_str,(struct Cyc_Sexp_Obj*(*)(struct Cyc_Sexp_Parser*))Cyc_Sexp_parse_str,(int(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Obj*))Cyc_Sexp_cmp_str,(int(*)(struct Cyc_Sexp_Obj*))Cyc_Sexp_hash_str,(void*(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Visitor*,void*))Cyc_Sexp_accept_str,0};static char _TmpGB[7U]="symbol";
struct Cyc_Sexp_Class Cyc_Sexp_symbol_class={(struct Cyc_Sexp_Obj*)0,'y',{_TmpGB,_TmpGB,_TmpGB + 7U},(void(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Printer*))Cyc_Sexp_print_symbol,(struct Cyc_Sexp_Obj*(*)(struct Cyc_Sexp_Parser*))Cyc_Sexp_parse_symbol,(int(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Obj*))Cyc_Sexp_cmp_symbol,(int(*)(struct Cyc_Sexp_Obj*))Cyc_Sexp_hash_symbol,(void*(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Visitor*,void*))Cyc_Sexp_accept_symbol,0};static char _TmpGC[6U]="tuple";
struct Cyc_Sexp_Class Cyc_Sexp_tuple_class={(struct Cyc_Sexp_Obj*)0,'t',{_TmpGC,_TmpGC,_TmpGC + 6U},(void(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Printer*))Cyc_Sexp_print_tuple,(struct Cyc_Sexp_Obj*(*)(struct Cyc_Sexp_Parser*))Cyc_Sexp_parse_tuple,(int(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Obj*))Cyc_Sexp_cmp_tuple,(int(*)(struct Cyc_Sexp_Obj*))Cyc_Sexp_hash_tuple,(void*(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Visitor*,void*))Cyc_Sexp_accept_tuple,0};
# 763
static struct Cyc_Sexp_Obj**Cyc_Sexp_hashcons(struct Cyc_Sexp_Obj*x){struct Cyc_Sexp_Obj*_T0;struct Cyc_Sexp_Class*_T1;struct Cyc_Hashtable_Table*(*_T2)(int,int(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Obj*),int(*)(struct Cyc_Sexp_Obj*));struct Cyc_Hashtable_Table*(*_T3)(int,int(*)(void*,void*),int(*)(void*));struct Cyc_Sexp_Class*_T4;int(*_T5)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Obj*);struct Cyc_Sexp_Class*_T6;int(*_T7)(struct Cyc_Sexp_Obj*);struct Cyc_Sexp_Class*_T8;struct Cyc_Sexp_Obj**(*_T9)(struct Cyc_Hashtable_Table*,struct Cyc_Sexp_Obj*,int(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Obj*),int(*)(struct Cyc_Sexp_Obj*));void**(*_TA)(struct Cyc_Hashtable_Table*,void*,int(*)(void*,void*),int(*)(void*));struct Cyc_Hashtable_Table*_TB;struct Cyc_Sexp_Obj*_TC;struct Cyc_Sexp_Class*_TD;int(*_TE)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Obj*);struct Cyc_Sexp_Class*_TF;int(*_T10)(struct Cyc_Sexp_Obj*);struct Cyc_Sexp_Obj**_T11;_T0=x;{
struct Cyc_Sexp_Class*c=_T0->vtable;_T1=c;{
struct Cyc_Hashtable_Table*topt=_T1->hash_table;
struct Cyc_Hashtable_Table*t;
if(topt!=0)goto _TLD8;_T3=Cyc_Hashtable_create;{
struct Cyc_Hashtable_Table*(*_T12)(int,int(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Obj*),int(*)(struct Cyc_Sexp_Obj*))=(struct Cyc_Hashtable_Table*(*)(int,int(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Obj*),int(*)(struct Cyc_Sexp_Obj*)))_T3;_T2=_T12;}_T4=c;_T5=_T4->cmp;_T6=c;_T7=_T6->hash;t=_T2(101,_T5,_T7);_T8=c;
_T8->hash_table=t;goto _TLD9;
# 771
_TLD8: t=topt;_TLD9: _TA=Cyc_Hashtable_lookup_other_opt;{
# 773
struct Cyc_Sexp_Obj**(*_T12)(struct Cyc_Hashtable_Table*,struct Cyc_Sexp_Obj*,int(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Obj*),int(*)(struct Cyc_Sexp_Obj*))=(struct Cyc_Sexp_Obj**(*)(struct Cyc_Hashtable_Table*,struct Cyc_Sexp_Obj*,int(*)(struct Cyc_Sexp_Obj*,struct Cyc_Sexp_Obj*),int(*)(struct Cyc_Sexp_Obj*)))_TA;_T9=_T12;}_TB=t;_TC=x;_TD=c;_TE=_TD->cmp;_TF=c;_T10=_TF->hash;_T11=_T9(_TB,_TC,_TE,_T10);return _T11;}}}
# 778
struct _tuple0*Cyc_Sexp_mk_uchar(unsigned char v){struct _tuple0 _T0;struct Cyc_Sexp_Class*_T1;struct _tuple0**(*_T2)(struct _tuple0*);struct _tuple0*_T3;struct _tuple0*_T4;struct _tuple0**_T5;struct _tuple0*_T6;struct _tuple0*_T7;void(*_T8)(struct Cyc_Hashtable_Table*,struct _tuple0*,struct _tuple0*);void(*_T9)(struct Cyc_Hashtable_Table*,void*,void*);struct _tuple0 _TA;struct Cyc_Sexp_Class*_TB;struct Cyc_Hashtable_Table*_TC;struct Cyc_Hashtable_Table*_TD;struct _tuple0*_TE;struct _tuple0*_TF;struct _tuple0*_T10;{struct _tuple0 _T11;_T1=& Cyc_Sexp_uchar_class;
_T11.vtable=(struct Cyc_Sexp_Class*)_T1;_T11.v=v;_T0=_T11;}{struct _tuple0 obj=_T0;{
struct _tuple0**(*_T11)(struct _tuple0*)=(struct _tuple0**(*)(struct _tuple0*))Cyc_Sexp_hashcons;_T2=_T11;}_T3=& obj;_T4=(struct _tuple0*)_T3;{struct _tuple0**objopt=_T2(_T4);
if(objopt==0)goto _TLDA;_T5=objopt;_T6=*_T5;return _T6;_TLDA: {
struct _tuple0*objp;objp=_cycalloc(sizeof(struct _tuple0));_T7=objp;*_T7=obj;_T9=Cyc_Hashtable_insert;{
void(*_T11)(struct Cyc_Hashtable_Table*,struct _tuple0*,struct _tuple0*)=(void(*)(struct Cyc_Hashtable_Table*,struct _tuple0*,struct _tuple0*))_T9;_T8=_T11;}_TA=obj;_TB=_TA.vtable;_TC=_TB->hash_table;_TD=_check_null(_TC);_TE=objp;_TF=objp;_T8(_TD,_TE,_TF);_T10=objp;
return _T10;}}}}
# 786
struct Cyc_Sexp_Object Cyc_Sexp_obj_uchar(unsigned char v){struct Cyc_Sexp_Object(*_T0)(struct _tuple0*);struct _tuple0*_T1;struct Cyc_Sexp_Object _T2;{struct Cyc_Sexp_Object(*_T3)(struct _tuple0*)=(struct Cyc_Sexp_Object(*)(struct _tuple0*))Cyc_Sexp_up;_T0=_T3;}_T1=Cyc_Sexp_mk_uchar(v);_T2=_T0(_T1);return _T2;}
struct Cyc_Sexp_Cls Cyc_Sexp_uchar_class_w={(struct Cyc_Sexp_Class*)& Cyc_Sexp_uchar_class};
# 802 "sexp.cyc"
struct _tuple1*Cyc_Sexp_mk_schar(signed char v){struct _tuple1 _T0;struct Cyc_Sexp_Class*_T1;struct _tuple1**(*_T2)(struct _tuple1*);struct _tuple1*_T3;struct _tuple1*_T4;struct _tuple1**_T5;struct _tuple1*_T6;struct _tuple1*_T7;void(*_T8)(struct Cyc_Hashtable_Table*,struct _tuple1*,struct _tuple1*);void(*_T9)(struct Cyc_Hashtable_Table*,void*,void*);struct _tuple1 _TA;struct Cyc_Sexp_Class*_TB;struct Cyc_Hashtable_Table*_TC;struct Cyc_Hashtable_Table*_TD;struct _tuple1*_TE;struct _tuple1*_TF;struct _tuple1*_T10;{struct _tuple1 _T11;_T1=& Cyc_Sexp_schar_class;_T11.vtable=(struct Cyc_Sexp_Class*)_T1;_T11.v=v;_T0=_T11;}{struct _tuple1 obj=_T0;{struct _tuple1**(*_T11)(struct _tuple1*)=(struct _tuple1**(*)(struct _tuple1*))Cyc_Sexp_hashcons;_T2=_T11;}_T3=& obj;_T4=(struct _tuple1*)_T3;{struct _tuple1**objopt=_T2(_T4);if(objopt==0)goto _TLDC;_T5=objopt;_T6=*_T5;return _T6;_TLDC: {struct _tuple1*objp;objp=_cycalloc(sizeof(struct _tuple1));_T7=objp;*_T7=obj;_T9=Cyc_Hashtable_insert;{void(*_T11)(struct Cyc_Hashtable_Table*,struct _tuple1*,struct _tuple1*)=(void(*)(struct Cyc_Hashtable_Table*,struct _tuple1*,struct _tuple1*))_T9;_T8=_T11;}_TA=obj;_TB=_TA.vtable;_TC=_TB->hash_table;_TD=_check_null(_TC);_TE=objp;_TF=objp;_T8(_TD,_TE,_TF);_T10=objp;return _T10;}}}}struct Cyc_Sexp_Object Cyc_Sexp_obj_schar(signed char v){struct Cyc_Sexp_Object(*_T0)(struct _tuple1*);struct _tuple1*_T1;struct Cyc_Sexp_Object _T2;{struct Cyc_Sexp_Object(*_T3)(struct _tuple1*)=(struct Cyc_Sexp_Object(*)(struct _tuple1*))Cyc_Sexp_up;_T0=_T3;}_T1=Cyc_Sexp_mk_schar(v);_T2=_T0(_T1);return _T2;}struct Cyc_Sexp_Cls Cyc_Sexp_schar_class_w={(struct Cyc_Sexp_Class*)& Cyc_Sexp_schar_class};
struct _tuple2*Cyc_Sexp_mk_ushort(unsigned short v){struct _tuple2 _T0;struct Cyc_Sexp_Class*_T1;struct _tuple2**(*_T2)(struct _tuple2*);struct _tuple2*_T3;struct _tuple2*_T4;struct _tuple2**_T5;struct _tuple2*_T6;struct _tuple2*_T7;void(*_T8)(struct Cyc_Hashtable_Table*,struct _tuple2*,struct _tuple2*);void(*_T9)(struct Cyc_Hashtable_Table*,void*,void*);struct _tuple2 _TA;struct Cyc_Sexp_Class*_TB;struct Cyc_Hashtable_Table*_TC;struct Cyc_Hashtable_Table*_TD;struct _tuple2*_TE;struct _tuple2*_TF;struct _tuple2*_T10;{struct _tuple2 _T11;_T1=& Cyc_Sexp_ushort_class;_T11.vtable=(struct Cyc_Sexp_Class*)_T1;_T11.v=v;_T0=_T11;}{struct _tuple2 obj=_T0;{struct _tuple2**(*_T11)(struct _tuple2*)=(struct _tuple2**(*)(struct _tuple2*))Cyc_Sexp_hashcons;_T2=_T11;}_T3=& obj;_T4=(struct _tuple2*)_T3;{struct _tuple2**objopt=_T2(_T4);if(objopt==0)goto _TLDE;_T5=objopt;_T6=*_T5;return _T6;_TLDE: {struct _tuple2*objp;objp=_cycalloc(sizeof(struct _tuple2));_T7=objp;*_T7=obj;_T9=Cyc_Hashtable_insert;{void(*_T11)(struct Cyc_Hashtable_Table*,struct _tuple2*,struct _tuple2*)=(void(*)(struct Cyc_Hashtable_Table*,struct _tuple2*,struct _tuple2*))_T9;_T8=_T11;}_TA=obj;_TB=_TA.vtable;_TC=_TB->hash_table;_TD=_check_null(_TC);_TE=objp;_TF=objp;_T8(_TD,_TE,_TF);_T10=objp;return _T10;}}}}struct Cyc_Sexp_Object Cyc_Sexp_obj_ushort(unsigned short v){struct Cyc_Sexp_Object(*_T0)(struct _tuple2*);struct _tuple2*_T1;struct Cyc_Sexp_Object _T2;{struct Cyc_Sexp_Object(*_T3)(struct _tuple2*)=(struct Cyc_Sexp_Object(*)(struct _tuple2*))Cyc_Sexp_up;_T0=_T3;}_T1=Cyc_Sexp_mk_ushort(v);_T2=_T0(_T1);return _T2;}struct Cyc_Sexp_Cls Cyc_Sexp_ushort_class_w={(struct Cyc_Sexp_Class*)& Cyc_Sexp_ushort_class};
struct _tuple3*Cyc_Sexp_mk_sshort(short v){struct _tuple3 _T0;struct Cyc_Sexp_Class*_T1;struct _tuple3**(*_T2)(struct _tuple3*);struct _tuple3*_T3;struct _tuple3*_T4;struct _tuple3**_T5;struct _tuple3*_T6;struct _tuple3*_T7;void(*_T8)(struct Cyc_Hashtable_Table*,struct _tuple3*,struct _tuple3*);void(*_T9)(struct Cyc_Hashtable_Table*,void*,void*);struct _tuple3 _TA;struct Cyc_Sexp_Class*_TB;struct Cyc_Hashtable_Table*_TC;struct Cyc_Hashtable_Table*_TD;struct _tuple3*_TE;struct _tuple3*_TF;struct _tuple3*_T10;{struct _tuple3 _T11;_T1=& Cyc_Sexp_sshort_class;_T11.vtable=(struct Cyc_Sexp_Class*)_T1;_T11.v=v;_T0=_T11;}{struct _tuple3 obj=_T0;{struct _tuple3**(*_T11)(struct _tuple3*)=(struct _tuple3**(*)(struct _tuple3*))Cyc_Sexp_hashcons;_T2=_T11;}_T3=& obj;_T4=(struct _tuple3*)_T3;{struct _tuple3**objopt=_T2(_T4);if(objopt==0)goto _TLE0;_T5=objopt;_T6=*_T5;return _T6;_TLE0: {struct _tuple3*objp;objp=_cycalloc(sizeof(struct _tuple3));_T7=objp;*_T7=obj;_T9=Cyc_Hashtable_insert;{void(*_T11)(struct Cyc_Hashtable_Table*,struct _tuple3*,struct _tuple3*)=(void(*)(struct Cyc_Hashtable_Table*,struct _tuple3*,struct _tuple3*))_T9;_T8=_T11;}_TA=obj;_TB=_TA.vtable;_TC=_TB->hash_table;_TD=_check_null(_TC);_TE=objp;_TF=objp;_T8(_TD,_TE,_TF);_T10=objp;return _T10;}}}}struct Cyc_Sexp_Object Cyc_Sexp_obj_sshort(short v){struct Cyc_Sexp_Object(*_T0)(struct _tuple3*);struct _tuple3*_T1;struct Cyc_Sexp_Object _T2;{struct Cyc_Sexp_Object(*_T3)(struct _tuple3*)=(struct Cyc_Sexp_Object(*)(struct _tuple3*))Cyc_Sexp_up;_T0=_T3;}_T1=Cyc_Sexp_mk_sshort(v);_T2=_T0(_T1);return _T2;}struct Cyc_Sexp_Cls Cyc_Sexp_sshort_class_w={(struct Cyc_Sexp_Class*)& Cyc_Sexp_sshort_class};
struct _tuple4*Cyc_Sexp_mk_uint(unsigned v){struct _tuple4 _T0;struct Cyc_Sexp_Class*_T1;struct _tuple4**(*_T2)(struct _tuple4*);struct _tuple4*_T3;struct _tuple4*_T4;struct _tuple4**_T5;struct _tuple4*_T6;struct _tuple4*_T7;void(*_T8)(struct Cyc_Hashtable_Table*,struct _tuple4*,struct _tuple4*);void(*_T9)(struct Cyc_Hashtable_Table*,void*,void*);struct _tuple4 _TA;struct Cyc_Sexp_Class*_TB;struct Cyc_Hashtable_Table*_TC;struct Cyc_Hashtable_Table*_TD;struct _tuple4*_TE;struct _tuple4*_TF;struct _tuple4*_T10;{struct _tuple4 _T11;_T1=& Cyc_Sexp_uint_class;_T11.vtable=(struct Cyc_Sexp_Class*)_T1;_T11.v=v;_T0=_T11;}{struct _tuple4 obj=_T0;{struct _tuple4**(*_T11)(struct _tuple4*)=(struct _tuple4**(*)(struct _tuple4*))Cyc_Sexp_hashcons;_T2=_T11;}_T3=& obj;_T4=(struct _tuple4*)_T3;{struct _tuple4**objopt=_T2(_T4);if(objopt==0)goto _TLE2;_T5=objopt;_T6=*_T5;return _T6;_TLE2: {struct _tuple4*objp;objp=_cycalloc(sizeof(struct _tuple4));_T7=objp;*_T7=obj;_T9=Cyc_Hashtable_insert;{void(*_T11)(struct Cyc_Hashtable_Table*,struct _tuple4*,struct _tuple4*)=(void(*)(struct Cyc_Hashtable_Table*,struct _tuple4*,struct _tuple4*))_T9;_T8=_T11;}_TA=obj;_TB=_TA.vtable;_TC=_TB->hash_table;_TD=_check_null(_TC);_TE=objp;_TF=objp;_T8(_TD,_TE,_TF);_T10=objp;return _T10;}}}}struct Cyc_Sexp_Object Cyc_Sexp_obj_uint(unsigned v){struct Cyc_Sexp_Object(*_T0)(struct _tuple4*);struct _tuple4*_T1;struct Cyc_Sexp_Object _T2;{struct Cyc_Sexp_Object(*_T3)(struct _tuple4*)=(struct Cyc_Sexp_Object(*)(struct _tuple4*))Cyc_Sexp_up;_T0=_T3;}_T1=Cyc_Sexp_mk_uint(v);_T2=_T0(_T1);return _T2;}struct Cyc_Sexp_Cls Cyc_Sexp_uint_class_w={(struct Cyc_Sexp_Class*)& Cyc_Sexp_uint_class};
struct _tuple5*Cyc_Sexp_mk_sint(int v){struct _tuple5 _T0;struct Cyc_Sexp_Class*_T1;struct _tuple5**(*_T2)(struct _tuple5*);struct _tuple5*_T3;struct _tuple5*_T4;struct _tuple5**_T5;struct _tuple5*_T6;struct _tuple5*_T7;void(*_T8)(struct Cyc_Hashtable_Table*,struct _tuple5*,struct _tuple5*);void(*_T9)(struct Cyc_Hashtable_Table*,void*,void*);struct _tuple5 _TA;struct Cyc_Sexp_Class*_TB;struct Cyc_Hashtable_Table*_TC;struct Cyc_Hashtable_Table*_TD;struct _tuple5*_TE;struct _tuple5*_TF;struct _tuple5*_T10;{struct _tuple5 _T11;_T1=& Cyc_Sexp_sint_class;_T11.vtable=(struct Cyc_Sexp_Class*)_T1;_T11.v=v;_T0=_T11;}{struct _tuple5 obj=_T0;{struct _tuple5**(*_T11)(struct _tuple5*)=(struct _tuple5**(*)(struct _tuple5*))Cyc_Sexp_hashcons;_T2=_T11;}_T3=& obj;_T4=(struct _tuple5*)_T3;{struct _tuple5**objopt=_T2(_T4);if(objopt==0)goto _TLE4;_T5=objopt;_T6=*_T5;return _T6;_TLE4: {struct _tuple5*objp;objp=_cycalloc(sizeof(struct _tuple5));_T7=objp;*_T7=obj;_T9=Cyc_Hashtable_insert;{void(*_T11)(struct Cyc_Hashtable_Table*,struct _tuple5*,struct _tuple5*)=(void(*)(struct Cyc_Hashtable_Table*,struct _tuple5*,struct _tuple5*))_T9;_T8=_T11;}_TA=obj;_TB=_TA.vtable;_TC=_TB->hash_table;_TD=_check_null(_TC);_TE=objp;_TF=objp;_T8(_TD,_TE,_TF);_T10=objp;return _T10;}}}}struct Cyc_Sexp_Object Cyc_Sexp_obj_sint(int v){struct Cyc_Sexp_Object(*_T0)(struct _tuple5*);struct _tuple5*_T1;struct Cyc_Sexp_Object _T2;{struct Cyc_Sexp_Object(*_T3)(struct _tuple5*)=(struct Cyc_Sexp_Object(*)(struct _tuple5*))Cyc_Sexp_up;_T0=_T3;}_T1=Cyc_Sexp_mk_sint(v);_T2=_T0(_T1);return _T2;}struct Cyc_Sexp_Cls Cyc_Sexp_sint_class_w={(struct Cyc_Sexp_Class*)& Cyc_Sexp_sint_class};
struct _tuple6*Cyc_Sexp_mk_ulonglong(unsigned long long v){struct _tuple6 _T0;struct Cyc_Sexp_Class*_T1;struct _tuple6**(*_T2)(struct _tuple6*);struct _tuple6*_T3;struct _tuple6*_T4;struct _tuple6**_T5;struct _tuple6*_T6;struct _tuple6*_T7;void(*_T8)(struct Cyc_Hashtable_Table*,struct _tuple6*,struct _tuple6*);void(*_T9)(struct Cyc_Hashtable_Table*,void*,void*);struct _tuple6 _TA;struct Cyc_Sexp_Class*_TB;struct Cyc_Hashtable_Table*_TC;struct Cyc_Hashtable_Table*_TD;struct _tuple6*_TE;struct _tuple6*_TF;struct _tuple6*_T10;{struct _tuple6 _T11;_T1=& Cyc_Sexp_ulonglong_class;_T11.vtable=(struct Cyc_Sexp_Class*)_T1;_T11.v=v;_T0=_T11;}{struct _tuple6 obj=_T0;{struct _tuple6**(*_T11)(struct _tuple6*)=(struct _tuple6**(*)(struct _tuple6*))Cyc_Sexp_hashcons;_T2=_T11;}_T3=& obj;_T4=(struct _tuple6*)_T3;{struct _tuple6**objopt=_T2(_T4);if(objopt==0)goto _TLE6;_T5=objopt;_T6=*_T5;return _T6;_TLE6: {struct _tuple6*objp;objp=_cycalloc(sizeof(struct _tuple6));_T7=objp;*_T7=obj;_T9=Cyc_Hashtable_insert;{void(*_T11)(struct Cyc_Hashtable_Table*,struct _tuple6*,struct _tuple6*)=(void(*)(struct Cyc_Hashtable_Table*,struct _tuple6*,struct _tuple6*))_T9;_T8=_T11;}_TA=obj;_TB=_TA.vtable;_TC=_TB->hash_table;_TD=_check_null(_TC);_TE=objp;_TF=objp;_T8(_TD,_TE,_TF);_T10=objp;return _T10;}}}}struct Cyc_Sexp_Object Cyc_Sexp_obj_ulonglong(unsigned long long v){struct Cyc_Sexp_Object(*_T0)(struct _tuple6*);struct _tuple6*_T1;struct Cyc_Sexp_Object _T2;{struct Cyc_Sexp_Object(*_T3)(struct _tuple6*)=(struct Cyc_Sexp_Object(*)(struct _tuple6*))Cyc_Sexp_up;_T0=_T3;}_T1=Cyc_Sexp_mk_ulonglong(v);_T2=_T0(_T1);return _T2;}struct Cyc_Sexp_Cls Cyc_Sexp_ulonglong_class_w={(struct Cyc_Sexp_Class*)& Cyc_Sexp_ulonglong_class};
struct _tuple7*Cyc_Sexp_mk_slonglong(long long v){struct _tuple7 _T0;struct Cyc_Sexp_Class*_T1;struct _tuple7**(*_T2)(struct _tuple7*);struct _tuple7*_T3;struct _tuple7*_T4;struct _tuple7**_T5;struct _tuple7*_T6;struct _tuple7*_T7;void(*_T8)(struct Cyc_Hashtable_Table*,struct _tuple7*,struct _tuple7*);void(*_T9)(struct Cyc_Hashtable_Table*,void*,void*);struct _tuple7 _TA;struct Cyc_Sexp_Class*_TB;struct Cyc_Hashtable_Table*_TC;struct Cyc_Hashtable_Table*_TD;struct _tuple7*_TE;struct _tuple7*_TF;struct _tuple7*_T10;{struct _tuple7 _T11;_T1=& Cyc_Sexp_slonglong_class;_T11.vtable=(struct Cyc_Sexp_Class*)_T1;_T11.v=v;_T0=_T11;}{struct _tuple7 obj=_T0;{struct _tuple7**(*_T11)(struct _tuple7*)=(struct _tuple7**(*)(struct _tuple7*))Cyc_Sexp_hashcons;_T2=_T11;}_T3=& obj;_T4=(struct _tuple7*)_T3;{struct _tuple7**objopt=_T2(_T4);if(objopt==0)goto _TLE8;_T5=objopt;_T6=*_T5;return _T6;_TLE8: {struct _tuple7*objp;objp=_cycalloc(sizeof(struct _tuple7));_T7=objp;*_T7=obj;_T9=Cyc_Hashtable_insert;{void(*_T11)(struct Cyc_Hashtable_Table*,struct _tuple7*,struct _tuple7*)=(void(*)(struct Cyc_Hashtable_Table*,struct _tuple7*,struct _tuple7*))_T9;_T8=_T11;}_TA=obj;_TB=_TA.vtable;_TC=_TB->hash_table;_TD=_check_null(_TC);_TE=objp;_TF=objp;_T8(_TD,_TE,_TF);_T10=objp;return _T10;}}}}struct Cyc_Sexp_Object Cyc_Sexp_obj_slonglong(long long v){struct Cyc_Sexp_Object(*_T0)(struct _tuple7*);struct _tuple7*_T1;struct Cyc_Sexp_Object _T2;{struct Cyc_Sexp_Object(*_T3)(struct _tuple7*)=(struct Cyc_Sexp_Object(*)(struct _tuple7*))Cyc_Sexp_up;_T0=_T3;}_T1=Cyc_Sexp_mk_slonglong(v);_T2=_T0(_T1);return _T2;}struct Cyc_Sexp_Cls Cyc_Sexp_slonglong_class_w={(struct Cyc_Sexp_Class*)& Cyc_Sexp_slonglong_class};
struct _tuple8*Cyc_Sexp_mk_float(float v){struct _tuple8 _T0;struct Cyc_Sexp_Class*_T1;struct _tuple8**(*_T2)(struct _tuple8*);struct _tuple8*_T3;struct _tuple8*_T4;struct _tuple8**_T5;struct _tuple8*_T6;struct _tuple8*_T7;void(*_T8)(struct Cyc_Hashtable_Table*,struct _tuple8*,struct _tuple8*);void(*_T9)(struct Cyc_Hashtable_Table*,void*,void*);struct _tuple8 _TA;struct Cyc_Sexp_Class*_TB;struct Cyc_Hashtable_Table*_TC;struct Cyc_Hashtable_Table*_TD;struct _tuple8*_TE;struct _tuple8*_TF;struct _tuple8*_T10;{struct _tuple8 _T11;_T1=& Cyc_Sexp_float_class;_T11.vtable=(struct Cyc_Sexp_Class*)_T1;_T11.v=v;_T0=_T11;}{struct _tuple8 obj=_T0;{struct _tuple8**(*_T11)(struct _tuple8*)=(struct _tuple8**(*)(struct _tuple8*))Cyc_Sexp_hashcons;_T2=_T11;}_T3=& obj;_T4=(struct _tuple8*)_T3;{struct _tuple8**objopt=_T2(_T4);if(objopt==0)goto _TLEA;_T5=objopt;_T6=*_T5;return _T6;_TLEA: {struct _tuple8*objp;objp=_cycalloc(sizeof(struct _tuple8));_T7=objp;*_T7=obj;_T9=Cyc_Hashtable_insert;{void(*_T11)(struct Cyc_Hashtable_Table*,struct _tuple8*,struct _tuple8*)=(void(*)(struct Cyc_Hashtable_Table*,struct _tuple8*,struct _tuple8*))_T9;_T8=_T11;}_TA=obj;_TB=_TA.vtable;_TC=_TB->hash_table;_TD=_check_null(_TC);_TE=objp;_TF=objp;_T8(_TD,_TE,_TF);_T10=objp;return _T10;}}}}struct Cyc_Sexp_Object Cyc_Sexp_obj_float(float v){struct Cyc_Sexp_Object(*_T0)(struct _tuple8*);struct _tuple8*_T1;struct Cyc_Sexp_Object _T2;{struct Cyc_Sexp_Object(*_T3)(struct _tuple8*)=(struct Cyc_Sexp_Object(*)(struct _tuple8*))Cyc_Sexp_up;_T0=_T3;}_T1=Cyc_Sexp_mk_float(v);_T2=_T0(_T1);return _T2;}struct Cyc_Sexp_Cls Cyc_Sexp_float_class_w={(struct Cyc_Sexp_Class*)& Cyc_Sexp_float_class};
struct _tuple9*Cyc_Sexp_mk_double(double v){struct _tuple9 _T0;struct Cyc_Sexp_Class*_T1;struct _tuple9**(*_T2)(struct _tuple9*);struct _tuple9*_T3;struct _tuple9*_T4;struct _tuple9**_T5;struct _tuple9*_T6;struct _tuple9*_T7;void(*_T8)(struct Cyc_Hashtable_Table*,struct _tuple9*,struct _tuple9*);void(*_T9)(struct Cyc_Hashtable_Table*,void*,void*);struct _tuple9 _TA;struct Cyc_Sexp_Class*_TB;struct Cyc_Hashtable_Table*_TC;struct Cyc_Hashtable_Table*_TD;struct _tuple9*_TE;struct _tuple9*_TF;struct _tuple9*_T10;{struct _tuple9 _T11;_T1=& Cyc_Sexp_double_class;_T11.vtable=(struct Cyc_Sexp_Class*)_T1;_T11.v=v;_T0=_T11;}{struct _tuple9 obj=_T0;{struct _tuple9**(*_T11)(struct _tuple9*)=(struct _tuple9**(*)(struct _tuple9*))Cyc_Sexp_hashcons;_T2=_T11;}_T3=& obj;_T4=(struct _tuple9*)_T3;{struct _tuple9**objopt=_T2(_T4);if(objopt==0)goto _TLEC;_T5=objopt;_T6=*_T5;return _T6;_TLEC: {struct _tuple9*objp;objp=_cycalloc(sizeof(struct _tuple9));_T7=objp;*_T7=obj;_T9=Cyc_Hashtable_insert;{void(*_T11)(struct Cyc_Hashtable_Table*,struct _tuple9*,struct _tuple9*)=(void(*)(struct Cyc_Hashtable_Table*,struct _tuple9*,struct _tuple9*))_T9;_T8=_T11;}_TA=obj;_TB=_TA.vtable;_TC=_TB->hash_table;_TD=_check_null(_TC);_TE=objp;_TF=objp;_T8(_TD,_TE,_TF);_T10=objp;return _T10;}}}}struct Cyc_Sexp_Object Cyc_Sexp_obj_double(double v){struct Cyc_Sexp_Object(*_T0)(struct _tuple9*);struct _tuple9*_T1;struct Cyc_Sexp_Object _T2;{struct Cyc_Sexp_Object(*_T3)(struct _tuple9*)=(struct Cyc_Sexp_Object(*)(struct _tuple9*))Cyc_Sexp_up;_T0=_T3;}_T1=Cyc_Sexp_mk_double(v);_T2=_T0(_T1);return _T2;}struct Cyc_Sexp_Cls Cyc_Sexp_double_class_w={(struct Cyc_Sexp_Class*)& Cyc_Sexp_double_class};
struct _tuple10*Cyc_Sexp_mk_str(struct _fat_ptr v){struct _tuple10 _T0;struct Cyc_Sexp_Class*_T1;struct _tuple10**(*_T2)(struct _tuple10*);struct _tuple10*_T3;struct _tuple10*_T4;struct _tuple10**_T5;struct _tuple10*_T6;struct _tuple10*_T7;void(*_T8)(struct Cyc_Hashtable_Table*,struct _tuple10*,struct _tuple10*);void(*_T9)(struct Cyc_Hashtable_Table*,void*,void*);struct _tuple10 _TA;struct Cyc_Sexp_Class*_TB;struct Cyc_Hashtable_Table*_TC;struct Cyc_Hashtable_Table*_TD;struct _tuple10*_TE;struct _tuple10*_TF;struct _tuple10*_T10;{struct _tuple10 _T11;_T1=& Cyc_Sexp_str_class;_T11.vtable=(struct Cyc_Sexp_Class*)_T1;_T11.v=v;_T0=_T11;}{struct _tuple10 obj=_T0;{struct _tuple10**(*_T11)(struct _tuple10*)=(struct _tuple10**(*)(struct _tuple10*))Cyc_Sexp_hashcons;_T2=_T11;}_T3=& obj;_T4=(struct _tuple10*)_T3;{struct _tuple10**objopt=_T2(_T4);if(objopt==0)goto _TLEE;_T5=objopt;_T6=*_T5;return _T6;_TLEE: {struct _tuple10*objp;objp=_cycalloc(sizeof(struct _tuple10));_T7=objp;*_T7=obj;_T9=Cyc_Hashtable_insert;{void(*_T11)(struct Cyc_Hashtable_Table*,struct _tuple10*,struct _tuple10*)=(void(*)(struct Cyc_Hashtable_Table*,struct _tuple10*,struct _tuple10*))_T9;_T8=_T11;}_TA=obj;_TB=_TA.vtable;_TC=_TB->hash_table;_TD=_check_null(_TC);_TE=objp;_TF=objp;_T8(_TD,_TE,_TF);_T10=objp;return _T10;}}}}struct Cyc_Sexp_Object Cyc_Sexp_obj_str(struct _fat_ptr v){struct Cyc_Sexp_Object(*_T0)(struct _tuple10*);struct _tuple10*_T1;struct Cyc_Sexp_Object _T2;{struct Cyc_Sexp_Object(*_T3)(struct _tuple10*)=(struct Cyc_Sexp_Object(*)(struct _tuple10*))Cyc_Sexp_up;_T0=_T3;}_T1=Cyc_Sexp_mk_str(v);_T2=_T0(_T1);return _T2;}struct Cyc_Sexp_Cls Cyc_Sexp_str_class_w={(struct Cyc_Sexp_Class*)& Cyc_Sexp_str_class};
struct _tuple10*Cyc_Sexp_mk_symbol(struct _fat_ptr v){struct _tuple10 _T0;struct Cyc_Sexp_Class*_T1;struct _tuple10**(*_T2)(struct _tuple10*);struct _tuple10*_T3;struct _tuple10*_T4;struct _tuple10**_T5;struct _tuple10*_T6;struct _tuple10*_T7;void(*_T8)(struct Cyc_Hashtable_Table*,struct _tuple10*,struct _tuple10*);void(*_T9)(struct Cyc_Hashtable_Table*,void*,void*);struct _tuple10 _TA;struct Cyc_Sexp_Class*_TB;struct Cyc_Hashtable_Table*_TC;struct Cyc_Hashtable_Table*_TD;struct _tuple10*_TE;struct _tuple10*_TF;struct _tuple10*_T10;{struct _tuple10 _T11;_T1=& Cyc_Sexp_symbol_class;_T11.vtable=(struct Cyc_Sexp_Class*)_T1;_T11.v=v;_T0=_T11;}{struct _tuple10 obj=_T0;{struct _tuple10**(*_T11)(struct _tuple10*)=(struct _tuple10**(*)(struct _tuple10*))Cyc_Sexp_hashcons;_T2=_T11;}_T3=& obj;_T4=(struct _tuple10*)_T3;{struct _tuple10**objopt=_T2(_T4);if(objopt==0)goto _TLF0;_T5=objopt;_T6=*_T5;return _T6;_TLF0: {struct _tuple10*objp;objp=_cycalloc(sizeof(struct _tuple10));_T7=objp;*_T7=obj;_T9=Cyc_Hashtable_insert;{void(*_T11)(struct Cyc_Hashtable_Table*,struct _tuple10*,struct _tuple10*)=(void(*)(struct Cyc_Hashtable_Table*,struct _tuple10*,struct _tuple10*))_T9;_T8=_T11;}_TA=obj;_TB=_TA.vtable;_TC=_TB->hash_table;_TD=_check_null(_TC);_TE=objp;_TF=objp;_T8(_TD,_TE,_TF);_T10=objp;return _T10;}}}}struct Cyc_Sexp_Object Cyc_Sexp_obj_symbol(struct _fat_ptr v){struct Cyc_Sexp_Object(*_T0)(struct _tuple10*);struct _tuple10*_T1;struct Cyc_Sexp_Object _T2;{struct Cyc_Sexp_Object(*_T3)(struct _tuple10*)=(struct Cyc_Sexp_Object(*)(struct _tuple10*))Cyc_Sexp_up;_T0=_T3;}_T1=Cyc_Sexp_mk_symbol(v);_T2=_T0(_T1);return _T2;}struct Cyc_Sexp_Cls Cyc_Sexp_symbol_class_w={(struct Cyc_Sexp_Class*)& Cyc_Sexp_symbol_class};
struct _tuple10*Cyc_Sexp_mk_tuple(struct _fat_ptr v){struct _tuple10 _T0;struct Cyc_Sexp_Class*_T1;struct _tuple10**(*_T2)(struct _tuple10*);struct _tuple10*_T3;struct _tuple10*_T4;struct _tuple10**_T5;struct _tuple10*_T6;struct _tuple10*_T7;void(*_T8)(struct Cyc_Hashtable_Table*,struct _tuple10*,struct _tuple10*);void(*_T9)(struct Cyc_Hashtable_Table*,void*,void*);struct _tuple10 _TA;struct Cyc_Sexp_Class*_TB;struct Cyc_Hashtable_Table*_TC;struct Cyc_Hashtable_Table*_TD;struct _tuple10*_TE;struct _tuple10*_TF;struct _tuple10*_T10;{struct _tuple10 _T11;_T1=& Cyc_Sexp_tuple_class;_T11.vtable=(struct Cyc_Sexp_Class*)_T1;_T11.v=v;_T0=_T11;}{struct _tuple10 obj=_T0;{struct _tuple10**(*_T11)(struct _tuple10*)=(struct _tuple10**(*)(struct _tuple10*))Cyc_Sexp_hashcons;_T2=_T11;}_T3=& obj;_T4=(struct _tuple10*)_T3;{struct _tuple10**objopt=_T2(_T4);if(objopt==0)goto _TLF2;_T5=objopt;_T6=*_T5;return _T6;_TLF2: {struct _tuple10*objp;objp=_cycalloc(sizeof(struct _tuple10));_T7=objp;*_T7=obj;_T9=Cyc_Hashtable_insert;{void(*_T11)(struct Cyc_Hashtable_Table*,struct _tuple10*,struct _tuple10*)=(void(*)(struct Cyc_Hashtable_Table*,struct _tuple10*,struct _tuple10*))_T9;_T8=_T11;}_TA=obj;_TB=_TA.vtable;_TC=_TB->hash_table;_TD=_check_null(_TC);_TE=objp;_TF=objp;_T8(_TD,_TE,_TF);_T10=objp;return _T10;}}}}struct Cyc_Sexp_Object Cyc_Sexp_obj_tuple(struct _fat_ptr v){struct Cyc_Sexp_Object(*_T0)(struct _tuple10*);struct _tuple10*_T1;struct Cyc_Sexp_Object _T2;{struct Cyc_Sexp_Object(*_T3)(struct _tuple10*)=(struct Cyc_Sexp_Object(*)(struct _tuple10*))Cyc_Sexp_up;_T0=_T3;}_T1=Cyc_Sexp_mk_tuple(v);_T2=_T0(_T1);return _T2;}struct Cyc_Sexp_Cls Cyc_Sexp_tuple_class_w={(struct Cyc_Sexp_Class*)& Cyc_Sexp_tuple_class};
# 817
struct Cyc_Sexp_Object Cyc_Sexp_tuple(struct _fat_ptr objs){struct _fat_ptr _T0;struct _fat_ptr _T1;struct Cyc_Sexp_Object*_T2;unsigned _T3;struct _fat_ptr _T4;struct Cyc_Sexp_Object*_T5;unsigned _T6;struct _fat_ptr _T7;unsigned char*_T8;struct Cyc_Sexp_Object*_T9;unsigned _TA;int _TB;struct Cyc_Sexp_Object _TC;_T1=objs;{unsigned _TD=
_get_fat_size(_T1,sizeof(struct Cyc_Sexp_Object));_T3=_check_times(_TD,sizeof(struct Cyc_Sexp_Object));{struct Cyc_Sexp_Object*_TE=_cycalloc(_T3);_T4=objs;{unsigned _TF=_get_fat_size(_T4,sizeof(struct Cyc_Sexp_Object));unsigned i;i=0;_TLF7: if(i < _TF)goto _TLF5;else{goto _TLF6;}_TLF5: _T6=i;_T5=_TE + _T6;_T7=objs;_T8=_T7.curr;_T9=(struct Cyc_Sexp_Object*)_T8;_TA=i;_TB=(int)_TA;*_T5=_T9[_TB];i=i + 1;goto _TLF7;_TLF6:;}_T2=(struct Cyc_Sexp_Object*)_TE;}_T0=_tag_fat(_T2,sizeof(struct Cyc_Sexp_Object),_TD);}_TC=Cyc_Sexp_obj_tuple(_T0);return _TC;}
# 823
static struct Cyc_List_List Cyc_Sexp_c0={(void*)& Cyc_Sexp_uchar_class_w,0};
static struct Cyc_List_List Cyc_Sexp_c1={(void*)& Cyc_Sexp_schar_class_w,(struct Cyc_List_List*)& Cyc_Sexp_c0};
static struct Cyc_List_List Cyc_Sexp_c2={(void*)& Cyc_Sexp_ushort_class_w,(struct Cyc_List_List*)& Cyc_Sexp_c1};
static struct Cyc_List_List Cyc_Sexp_c3={(void*)& Cyc_Sexp_sshort_class_w,(struct Cyc_List_List*)& Cyc_Sexp_c2};
static struct Cyc_List_List Cyc_Sexp_c4={(void*)& Cyc_Sexp_uint_class_w,(struct Cyc_List_List*)& Cyc_Sexp_c3};
static struct Cyc_List_List Cyc_Sexp_c5={(void*)& Cyc_Sexp_sint_class_w,(struct Cyc_List_List*)& Cyc_Sexp_c4};
static struct Cyc_List_List Cyc_Sexp_c6={(void*)& Cyc_Sexp_ulonglong_class_w,(struct Cyc_List_List*)& Cyc_Sexp_c5};
static struct Cyc_List_List Cyc_Sexp_c7={(void*)& Cyc_Sexp_slonglong_class_w,(struct Cyc_List_List*)& Cyc_Sexp_c6};
static struct Cyc_List_List Cyc_Sexp_c8={(void*)& Cyc_Sexp_float_class_w,(struct Cyc_List_List*)& Cyc_Sexp_c7};
static struct Cyc_List_List Cyc_Sexp_c9={(void*)& Cyc_Sexp_double_class_w,(struct Cyc_List_List*)& Cyc_Sexp_c8};
static struct Cyc_List_List Cyc_Sexp_c10={(void*)& Cyc_Sexp_symbol_class_w,(struct Cyc_List_List*)& Cyc_Sexp_c9};
static struct Cyc_List_List Cyc_Sexp_c11={(void*)& Cyc_Sexp_str_class_w,(struct Cyc_List_List*)& Cyc_Sexp_c10};
static struct Cyc_List_List Cyc_Sexp_c12={(void*)& Cyc_Sexp_tuple_class_w,(struct Cyc_List_List*)& Cyc_Sexp_c11};
# 837
struct Cyc_List_List*Cyc_Sexp_classes=(struct Cyc_List_List*)& Cyc_Sexp_c12;
# 840
struct Cyc_Sexp_Visitor Cyc_Sexp_empty_visitor (void){struct Cyc_Sexp_Visitor _T0;{struct Cyc_Sexp_Visitor _T1;
# 842
_T1.visit_uchar=0;
_T1.visit_schar=0;
_T1.visit_ushort=0;
_T1.visit_sshort=0;
_T1.visit_uint=0;
_T1.visit_sint=0;
_T1.visit_ulonglong=0;
_T1.visit_slonglong=0;
_T1.visit_float=0;
_T1.visit_double=0;
_T1.visit_symbol=0;
_T1.visit_str=0;
_T1.visit_tuple=0;
_T1.visit_default=0;_T0=_T1;}
# 841
return _T0;}
# 858
struct Cyc_Sexp_Visitor Cyc_Sexp_default_visitor(void*(*def)(void*,struct Cyc_Sexp_Object,struct Cyc_Sexp_Visitor*)){struct Cyc_Sexp_Visitor _T0;
struct Cyc_Sexp_Visitor v=Cyc_Sexp_empty_visitor();
v.visit_default=def;_T0=v;
return _T0;}
