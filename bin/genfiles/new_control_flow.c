#include <setjmp.h>
/* This is a C header file to be used by the output of the Cyclone to
   C translator.  The corresponding definitions are in file
   lib/runtime_cyc.c
*/
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

// pushes a frame on the stack
void _push_frame(struct _RuntimeStack *frame);

// pop N+1 frames from the stack (error if stack_size < N+1)
void _npop_frame(unsigned int n);

// returns top frame on the stack (NULL if stack is empty)
struct _RuntimeStack * _top_frame();

// pops off frames until a frame with the given tag is reached.  This
// frame is returned, or else NULL if none found.
struct _RuntimeStack * _pop_frame_until(int tag);

/***********************************************************************/
/* Low-level representations etc.                                      */
/***********************************************************************/

#ifdef NO_CYC_PREFIX
#define ADD_PREFIX(x) x
#else
#define ADD_PREFIX(x) Cyc_##x
#endif

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
struct _RegionPage {
#ifdef CYC_REGION_PROFILE
  unsigned total_bytes;
  unsigned free_bytes;
#endif
  struct _RegionPage *next;
  char data[1];  /*FJS: used to be size 0, but that's forbidden in ansi c*/
};

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
extern void   _reset_region(struct _RegionHandle *);
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
extern int _throw_null_fn(const char *filename, unsigned lineno);
extern int _throw_arraybounds_fn(const char *filename, unsigned lineno);
extern int _throw_badalloc_fn(const char *filename, unsigned lineno);
extern int _throw_match_fn(const char *filename, unsigned lineno);
extern int _throw_fn(void* e, const char *filename, unsigned lineno);
extern int _rethrow(void* e);
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
#ifdef __APPLE__
#define _INLINE_FUNCTIONS
#endif

#ifdef CYC_ANSI_OUTPUT
#define _INLINE  
#define _INLINE_FUNCTIONS
#else
#define _INLINE inline
#endif

#ifdef VC_C
#define _CYC_U_LONG_LONG_T __int64
#else
#ifdef GCC_C
#define _CYC_U_LONG_LONG_T unsigned long long
#else
#define _CYC_U_LONG_LONG_T unsigned long long
#endif
#endif

#ifdef NO_CYC_NULL_CHECKS
#define _check_null(ptr) (ptr)
#else
#ifdef _INLINE_FUNCTIONS
static _INLINE void *
_check_null_fn(const void *ptr, const char *filename, unsigned lineno) {
  void*_check_null_temp = (void*)(ptr);
  if (!_check_null_temp) _throw_null_fn(filename,lineno);
  return _check_null_temp;
}
#define _check_null(p) (_check_null_fn((p),__FILE__,__LINE__))
#else
#define _check_null(ptr) \
  ({ void*_check_null_temp = (void*)(ptr); \
     if (!_check_null_temp) _throw_null(); \
     _check_null_temp; })
#endif
#endif

#ifdef NO_CYC_BOUNDS_CHECKS
#define _check_known_subscript_null(ptr,bound,elt_sz,index) ({ \
  ((char *)ptr) + (elt_sz)*(index); })
#else
#ifdef _INLINE_FUNCTIONS
static _INLINE char *
_check_known_subscript_null_fn(void *ptr, unsigned bound, unsigned elt_sz, unsigned index, const char *filename, unsigned lineno) {
  void*_cks_ptr = (void*)(ptr);
  unsigned _cks_bound = (bound);
  unsigned _cks_elt_sz = (elt_sz);
  unsigned _cks_index = (index);
  if (!_cks_ptr) _throw_null_fn(filename,lineno);
  if (_cks_index >= _cks_bound) _throw_arraybounds_fn(filename,lineno);
  return ((char *)_cks_ptr) + _cks_elt_sz*_cks_index;
}
#define _check_known_subscript_null(p,b,e) (_check_known_subscript_null_fn(p,b,e,__FILE__,__LINE__))
#else
#define _check_known_subscript_null(ptr,bound,elt_sz,index) ({ \
  void*_cks_ptr = (void*)(ptr); \
  unsigned _cks_bound = (bound); \
  unsigned _cks_elt_sz = (elt_sz); \
  unsigned _cks_index = (index); \
  if (!_cks_ptr) _throw_null(); \
  if (_cks_index >= _cks_bound) _throw_arraybounds(); \
  ((char *)_cks_ptr) + _cks_elt_sz*_cks_index; })
#endif
#endif

#ifdef NO_CYC_BOUNDS_CHECKS
#define _check_known_subscript_notnull(bound,index) (index)
#else
#ifdef _INLINE_FUNCTIONS
static _INLINE unsigned
_check_known_subscript_notnull_fn(unsigned bound,unsigned index,const char *filename,unsigned lineno) { 
  unsigned _cksnn_bound = (bound); 
  unsigned _cksnn_index = (index); 
  if (_cksnn_index >= _cksnn_bound) _throw_arraybounds_fn(filename,lineno); 
  return _cksnn_index;
}
#define _check_known_subscript_notnull(b,i) (_check_known_subscript_notnull_fn(b,i,__FILE__,__LINE__))
#else
#define _check_known_subscript_notnull(bound,index) ({ \
  unsigned _cksnn_bound = (bound); \
  unsigned _cksnn_index = (index); \
  if (_cksnn_index >= _cksnn_bound) _throw_arraybounds(); \
  _cksnn_index; })
#endif
#endif

/* Add i to zero-terminated pointer x.  Checks for x being null and
   ensures that x[0..i-1] are not 0. */
#ifdef NO_CYC_BOUNDS_CHECK
#define _zero_arr_plus_char_fn(orig_x,orig_sz,orig_i,f,l) ((orig_x)+(orig_i))
#define _zero_arr_plus_short_fn(orig_x,orig_sz,orig_i,f,l) ((orig_x)+(orig_i))
#define _zero_arr_plus_int_fn(orig_x,orig_sz,orig_i,f,l) ((orig_x)+(orig_i))
#define _zero_arr_plus_float_fn(orig_x,orig_sz,orig_i,f,l) ((orig_x)+(orig_i))
#define _zero_arr_plus_double_fn(orig_x,orig_sz,orig_i,f,l) ((orig_x)+(orig_i))
#define _zero_arr_plus_longdouble_fn(orig_x,orig_sz,orig_i,f,l) ((orig_x)+(orig_i))
#define _zero_arr_plus_voidstar_fn(orig_x,orig_sz,orig_i,f,l) ((orig_x)+(orig_i))
#else
static _INLINE char *
_zero_arr_plus_char_fn(char *orig_x, unsigned int orig_sz, int orig_i,const char *filename, unsigned lineno) {
  unsigned int _czs_temp;
  if ((orig_x) == 0) _throw_null_fn(filename,lineno);
  if (orig_i < 0 || orig_sz == 0) _throw_arraybounds_fn(filename,lineno);
  for (_czs_temp=orig_sz-1; _czs_temp < orig_i; _czs_temp++)
    if (orig_x[_czs_temp] == 0) _throw_arraybounds_fn(filename,lineno);
  return orig_x + orig_i;
}
static _INLINE short *
_zero_arr_plus_short_fn(short *orig_x, unsigned int orig_sz, int orig_i,const char *filename, unsigned lineno) {
  unsigned int _czs_temp;
  if ((orig_x) == 0) _throw_null_fn(filename,lineno);
  if (orig_i < 0 || orig_sz == 0) _throw_arraybounds_fn(filename,lineno);
  for (_czs_temp=orig_sz-1; _czs_temp < orig_i; _czs_temp++)
    if (orig_x[_czs_temp] == 0) _throw_arraybounds_fn(filename,lineno);
  return orig_x + orig_i;
}
static _INLINE int *
_zero_arr_plus_int_fn(int *orig_x, unsigned int orig_sz, int orig_i, const char *filename, unsigned lineno) {
  unsigned int _czs_temp;
  if ((orig_x) == 0) _throw_null_fn(filename,lineno);
  if (orig_i < 0 || orig_sz == 0) _throw_arraybounds_fn(filename,lineno);
  for (_czs_temp=orig_sz-1; _czs_temp < orig_i; _czs_temp++)
    if (orig_x[_czs_temp] == 0) _throw_arraybounds_fn(filename,lineno);
  return orig_x + orig_i;
}
static _INLINE float *
_zero_arr_plus_float_fn(float *orig_x, unsigned int orig_sz, int orig_i,const char *filename, unsigned lineno) {
  unsigned int _czs_temp;
  if ((orig_x) == 0) _throw_null_fn(filename,lineno);
  if (orig_i < 0 || orig_sz == 0) _throw_arraybounds_fn(filename,lineno);
  for (_czs_temp=orig_sz-1; _czs_temp < orig_i; _czs_temp++)
    if (orig_x[_czs_temp] == 0) _throw_arraybounds_fn(filename,lineno);
  return orig_x + orig_i;
}
static _INLINE double *
_zero_arr_plus_double_fn(double *orig_x, unsigned int orig_sz, int orig_i,const char *filename, unsigned lineno) {
  unsigned int _czs_temp;
  if ((orig_x) == 0) _throw_null_fn(filename,lineno);
  if (orig_i < 0 || orig_sz == 0) _throw_arraybounds_fn(filename,lineno);
  for (_czs_temp=orig_sz-1; _czs_temp < orig_i; _czs_temp++)
    if (orig_x[_czs_temp] == 0) _throw_arraybounds_fn(filename,lineno);
  return orig_x + orig_i;
}
static _INLINE long double *
_zero_arr_plus_longdouble_fn(long double *orig_x, unsigned int orig_sz, int orig_i, const char *filename, unsigned lineno) {
  unsigned int _czs_temp;
  if ((orig_x) == 0) _throw_null_fn(filename,lineno);
  if (orig_i < 0 || orig_sz == 0) _throw_arraybounds_fn(filename,lineno);
  for (_czs_temp=orig_sz-1; _czs_temp < orig_i; _czs_temp++)
    if (orig_x[_czs_temp] == 0) _throw_arraybounds_fn(filename,lineno);
  return orig_x + orig_i;
}
static _INLINE void *
_zero_arr_plus_voidstar_fn(void **orig_x, unsigned int orig_sz, int orig_i,const char *filename,unsigned lineno) {
  unsigned int _czs_temp;
  if ((orig_x) == 0) _throw_null_fn(filename,lineno);
  if (orig_i < 0 || orig_sz == 0) _throw_arraybounds_fn(filename,lineno);
  for (_czs_temp=orig_sz-1; _czs_temp < orig_i; _czs_temp++)
    if (orig_x[_czs_temp] == 0) _throw_arraybounds_fn(filename,lineno);
  return orig_x + orig_i;
}
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
static _INLINE int
_get_zero_arr_size_char(const char *orig_x, unsigned int orig_offset) {
  const char *_gres_x = orig_x;
  unsigned int _gres = 0;
  if (_gres_x != 0) {
     _gres = orig_offset;
     _gres_x += orig_offset - 1;
     while (*_gres_x != 0) { _gres_x++; _gres++; }
  }
  return _gres; 
}
static _INLINE int
_get_zero_arr_size_short(const short *orig_x, unsigned int orig_offset) {
  const short *_gres_x = orig_x;
  unsigned int _gres = 0;
  if (_gres_x != 0) {
     _gres = orig_offset;
     _gres_x += orig_offset - 1;
     while (*_gres_x != 0) { _gres_x++; _gres++; }
  }
  return _gres; 
}
static _INLINE int
_get_zero_arr_size_int(const int *orig_x, unsigned int orig_offset) {
  const int *_gres_x = orig_x;
  unsigned int _gres = 0;
  if (_gres_x != 0) {
     _gres = orig_offset;
     _gres_x += orig_offset - 1;
     while (*_gres_x != 0) { _gres_x++; _gres++; }
  }
  return _gres; 
}
static _INLINE int
_get_zero_arr_size_float(const float *orig_x, unsigned int orig_offset) {
  const float *_gres_x = orig_x;
  unsigned int _gres = 0;
  if (_gres_x != 0) {
     _gres = orig_offset;
     _gres_x += orig_offset - 1;
     while (*_gres_x != 0) { _gres_x++; _gres++; }
  }
  return _gres; 
}
static _INLINE int
_get_zero_arr_size_double(const double *orig_x, unsigned int orig_offset) {
  const double *_gres_x = orig_x;
  unsigned int _gres = 0;
  if (_gres_x != 0) {
     _gres = orig_offset;
     _gres_x += orig_offset - 1;
     while (*_gres_x != 0) { _gres_x++; _gres++; }
  }
  return _gres; 
}
static _INLINE int
_get_zero_arr_size_longdouble(const long double *orig_x, unsigned int orig_offset) {
  const long double *_gres_x = orig_x;
  unsigned int _gres = 0;
  if (_gres_x != 0) {
     _gres = orig_offset;
     _gres_x += orig_offset - 1;
     while (*_gres_x != 0) { _gres_x++; _gres++; }
  }
  return _gres; 
}
static _INLINE int
_get_zero_arr_size_voidstar(const void **orig_x, unsigned int orig_offset) {
  const void **_gres_x = orig_x;
  unsigned int _gres = 0;
  if (_gres_x != 0) {
     _gres = orig_offset;
     _gres_x += orig_offset - 1;
     while (*_gres_x != 0) { _gres_x++; _gres++; }
  }
  return _gres; 
}


/* Does in-place addition of a zero-terminated pointer (x += e and ++x).  
   Note that this expands to call _zero_arr_plus_<type>_fn. */
static _INLINE char *
_zero_arr_inplace_plus_char_fn(char **x, int orig_i,const char *filename,unsigned lineno) {
  *x = _zero_arr_plus_char_fn(*x,1,orig_i,filename,lineno);
  return *x;
}
#define _zero_arr_inplace_plus_char(x,i) \
  _zero_arr_inplace_plus_char_fn((char **)(x),i,__FILE__,__LINE__)
static _INLINE short *
_zero_arr_inplace_plus_short_fn(short **x, int orig_i,const char *filename,unsigned lineno) {
  *x = _zero_arr_plus_short_fn(*x,1,orig_i,filename,lineno);
  return *x;
}
#define _zero_arr_inplace_plus_short(x,i) \
  _zero_arr_inplace_plus_short_fn((short **)(x),i,__FILE__,__LINE__)
static _INLINE int *
_zero_arr_inplace_plus_int(int **x, int orig_i,const char *filename,unsigned lineno) {
  *x = _zero_arr_plus_int_fn(*x,1,orig_i,filename,lineno);
  return *x;
}
#define _zero_arr_inplace_plus_int(x,i) \
  _zero_arr_inplace_plus_int_fn((int **)(x),i,__FILE__,__LINE__)
static _INLINE float *
_zero_arr_inplace_plus_float_fn(float **x, int orig_i,const char *filename,unsigned lineno) {
  *x = _zero_arr_plus_float_fn(*x,1,orig_i,filename,lineno);
  return *x;
}
#define _zero_arr_inplace_plus_float(x,i) \
  _zero_arr_inplace_plus_float_fn((float **)(x),i,__FILE__,__LINE__)
static _INLINE double *
_zero_arr_inplace_plus_double_fn(double **x, int orig_i,const char *filename,unsigned lineno) {
  *x = _zero_arr_plus_double_fn(*x,1,orig_i,filename,lineno);
  return *x;
}
#define _zero_arr_inplace_plus_double(x,i) \
  _zero_arr_inplace_plus_double_fn((double **)(x),i,__FILE__,__LINE__)
static _INLINE long double *
_zero_arr_inplace_plus_longdouble_fn(long double **x, int orig_i,const char *filename,unsigned lineno) {
  *x = _zero_arr_plus_longdouble_fn(*x,1,orig_i,filename,lineno);
  return *x;
}
#define _zero_arr_inplace_plus_longdouble(x,i) \
  _zero_arr_inplace_plus_longdouble_fn((long double **)(x),i,__FILE__,__LINE__)
static _INLINE void *
_zero_arr_inplace_plus_voidstar_fn(void ***x, int orig_i,const char *filename,unsigned lineno) {
  *x = _zero_arr_plus_voidstar_fn(*x,1,orig_i,filename,lineno);
  return *x;
}
#define _zero_arr_inplace_plus_voidstar(x,i) \
  _zero_arr_inplace_plus_voidstar_fn((void ***)(x),i,__FILE__,__LINE__)

/* Does in-place increment of a zero-terminated pointer (e.g., x++). */
static _INLINE char *
_zero_arr_inplace_plus_post_char_fn(char **x, int orig_i,const char *filename,unsigned lineno){
  char * _zap_res = *x;
  *x = _zero_arr_plus_char_fn(_zap_res,1,orig_i,filename,lineno);
  return _zap_res;
}
#define _zero_arr_inplace_plus_post_char(x,i) \
  _zero_arr_inplace_plus_post_char_fn((char **)(x),(i),__FILE__,__LINE__)
static _INLINE short *
_zero_arr_inplace_plus_post_short_fn(short **x, int orig_i,const char *filename,unsigned lineno){
  short * _zap_res = *x;
  *x = _zero_arr_plus_short_fn(_zap_res,1,orig_i,filename,lineno);
  return _zap_res;
}
#define _zero_arr_inplace_plus_post_short(x,i) \
  _zero_arr_inplace_plus_post_short_fn((short **)(x),(i),__FILE__,__LINE__)
static _INLINE int *
_zero_arr_inplace_plus_post_int_fn(int **x, int orig_i,const char *filename, unsigned lineno){
  int * _zap_res = *x;
  *x = _zero_arr_plus_int_fn(_zap_res,1,orig_i,filename,lineno);
  return _zap_res;
}
#define _zero_arr_inplace_plus_post_int(x,i) \
  _zero_arr_inplace_plus_post_int_fn((int **)(x),(i),__FILE__,__LINE__)
static _INLINE float *
_zero_arr_inplace_plus_post_float_fn(float **x, int orig_i,const char *filename, unsigned lineno){
  float * _zap_res = *x;
  *x = _zero_arr_plus_float_fn(_zap_res,1,orig_i,filename,lineno);
  return _zap_res;
}
#define _zero_arr_inplace_plus_post_float(x,i) \
  _zero_arr_inplace_plus_post_float_fn((float **)(x),(i),__FILE__,__LINE__)
static _INLINE double *
_zero_arr_inplace_plus_post_double_fn(double **x, int orig_i,const char *filename,unsigned lineno){
  double * _zap_res = *x;
  *x = _zero_arr_plus_double_fn(_zap_res,1,orig_i,filename,lineno);
  return _zap_res;
}
#define _zero_arr_inplace_plus_post_double(x,i) \
  _zero_arr_inplace_plus_post_double_fn((double **)(x),(i),__FILE__,__LINE__)
static _INLINE long double *
_zero_arr_inplace_plus_post_longdouble_fn(long double **x, int orig_i,const char *filename,unsigned lineno){
  long double * _zap_res = *x;
  *x = _zero_arr_plus_longdouble_fn(_zap_res,1,orig_i,filename,lineno);
  return _zap_res;
}
#define _zero_arr_inplace_plus_post_longdouble(x,i) \
  _zero_arr_inplace_plus_post_longdouble_fn((long double **)(x),(i),__FILE__,__LINE__)
static _INLINE void **
_zero_arr_inplace_plus_post_voidstar_fn(void ***x, int orig_i,const char *filename,unsigned lineno){
  void ** _zap_res = *x;
  *x = _zero_arr_plus_voidstar_fn(_zap_res,1,orig_i,filename,lineno);
  return _zap_res;
}
#define _zero_arr_inplace_plus_post_voidstar(x,i) \
  _zero_arr_inplace_plus_post_voidstar_fn((void***)(x),(i),__FILE__,__LINE__)

/* functions for dealing with dynamically sized pointers */
#ifdef NO_CYC_BOUNDS_CHECKS
#ifdef _INLINE_FUNCTIONS
static _INLINE unsigned char *
_check_dyneither_subscript(struct _dyneither_ptr arr,unsigned elt_sz,unsigned index) {
  struct _dyneither_ptr _cus_arr = (arr);
  unsigned _cus_elt_sz = (elt_sz);
  unsigned _cus_index = (index);
  unsigned char *_cus_ans = _cus_arr.curr + _cus_elt_sz * _cus_index;
  return _cus_ans;
}
#else
#define _check_dyneither_subscript(arr,elt_sz,index) ({ \
  struct _dyneither_ptr _cus_arr = (arr); \
  unsigned _cus_elt_sz = (elt_sz); \
  unsigned _cus_index = (index); \
  unsigned char *_cus_ans = _cus_arr.curr + _cus_elt_sz * _cus_index; \
  _cus_ans; })
#endif
#else
#ifdef _INLINE_FUNCTIONS
static _INLINE unsigned char *
_check_dyneither_subscript_fn(struct _dyneither_ptr arr,unsigned elt_sz,unsigned index,const char *filename, unsigned lineno) {
  struct _dyneither_ptr _cus_arr = (arr);
  unsigned _cus_elt_sz = (elt_sz);
  unsigned _cus_index = (index);
  unsigned char *_cus_ans = _cus_arr.curr + _cus_elt_sz * _cus_index;
  /* JGM: not needed! if (!_cus_arr.base) _throw_null(); */ 
  if (_cus_ans < _cus_arr.base || _cus_ans >= _cus_arr.last_plus_one)
    _throw_arraybounds_fn(filename,lineno);
  return _cus_ans;
}
#define _check_dyneither_subscript(a,s,i) \
  _check_dyneither_subscript_fn(a,s,i,__FILE__,__LINE__)
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
#endif

#ifdef _INLINE_FUNCTIONS
static _INLINE struct _dyneither_ptr
_tag_dyneither(const void *tcurr,unsigned elt_sz,unsigned num_elts) {
  struct _dyneither_ptr _tag_arr_ans;
  _tag_arr_ans.base = _tag_arr_ans.curr = (void*)(tcurr);
  _tag_arr_ans.last_plus_one = _tag_arr_ans.base + (elt_sz) * (num_elts);
  return _tag_arr_ans;
}
#else
#define _tag_dyneither(tcurr,elt_sz,num_elts) ({ \
  struct _dyneither_ptr _tag_arr_ans; \
  _tag_arr_ans.base = _tag_arr_ans.curr = (void*)(tcurr); \
  _tag_arr_ans.last_plus_one = _tag_arr_ans.base + (elt_sz) * (num_elts); \
  _tag_arr_ans; })
#endif

#ifdef _INLINE_FUNCTIONS
static _INLINE struct _dyneither_ptr *
_init_dyneither_ptr(struct _dyneither_ptr *arr_ptr,
                    void *arr, unsigned elt_sz, unsigned num_elts) {
  struct _dyneither_ptr *_itarr_ptr = (arr_ptr);
  void* _itarr = (arr);
  _itarr_ptr->base = _itarr_ptr->curr = _itarr;
  _itarr_ptr->last_plus_one = ((unsigned char *)_itarr) + (elt_sz) * (num_elts);
  return _itarr_ptr;
}
#else
#define _init_dyneither_ptr(arr_ptr,arr,elt_sz,num_elts) ({ \
  struct _dyneither_ptr *_itarr_ptr = (arr_ptr); \
  void* _itarr = (arr); \
  _itarr_ptr->base = _itarr_ptr->curr = _itarr; \
  _itarr_ptr->last_plus_one = ((char *)_itarr) + (elt_sz) * (num_elts); \
  _itarr_ptr; })
#endif

#ifdef NO_CYC_BOUNDS_CHECKS
#define _untag_dyneither_ptr(arr,elt_sz,num_elts) ((arr).curr)
#else
#ifdef _INLINE_FUNCTIONS
static _INLINE unsigned char *
_untag_dyneither_ptr_fn(struct _dyneither_ptr arr, 
                        unsigned elt_sz,unsigned num_elts,
                        const char *filename, unsigned lineno) {
  struct _dyneither_ptr _arr = (arr);
  unsigned char *_curr = _arr.curr;
  if ((_curr < _arr.base || _curr + (elt_sz) * (num_elts) > _arr.last_plus_one) &&
      _curr != (unsigned char *)0)
    _throw_arraybounds_fn(filename,lineno);
  return _curr;
}
#define _untag_dyneither_ptr(a,s,e) \
  _untag_dyneither_ptr_fn(a,s,e,__FILE__,__LINE__)
#else
#define _untag_dyneither_ptr(arr,elt_sz,num_elts) ({ \
  struct _dyneither_ptr _arr = (arr); \
  unsigned char *_curr = _arr.curr; \
  if ((_curr < _arr.base || _curr + (elt_sz) * (num_elts) > _arr.last_plus_one) &&\
      _curr != (unsigned char *)0) \
    _throw_arraybounds(); \
  _curr; })
#endif
#endif

#ifdef _INLINE_FUNCTIONS
static _INLINE unsigned
_get_dyneither_size(struct _dyneither_ptr arr,unsigned elt_sz) {
  struct _dyneither_ptr _get_arr_size_temp = (arr);
  unsigned char *_get_arr_size_curr=_get_arr_size_temp.curr;
  unsigned char *_get_arr_size_last=_get_arr_size_temp.last_plus_one;
  return (_get_arr_size_curr < _get_arr_size_temp.base ||
          _get_arr_size_curr >= _get_arr_size_last) ? 0 :
    ((_get_arr_size_last - _get_arr_size_curr) / (elt_sz));
}
#else
#define _get_dyneither_size(arr,elt_sz) \
  ({struct _dyneither_ptr _get_arr_size_temp = (arr); \
    unsigned char *_get_arr_size_curr=_get_arr_size_temp.curr; \
    unsigned char *_get_arr_size_last=_get_arr_size_temp.last_plus_one; \
    (_get_arr_size_curr < _get_arr_size_temp.base || \
     _get_arr_size_curr >= _get_arr_size_last) ? 0 : \
    ((_get_arr_size_last - _get_arr_size_curr) / (elt_sz));})
#endif

#ifdef _INLINE_FUNCTIONS
static _INLINE struct _dyneither_ptr
_dyneither_ptr_plus(struct _dyneither_ptr arr,unsigned elt_sz,int change) {
  struct _dyneither_ptr _ans = (arr);
  _ans.curr += ((int)(elt_sz))*(change);
  return _ans;
}
#else
#define _dyneither_ptr_plus(arr,elt_sz,change) ({ \
  struct _dyneither_ptr _ans = (arr); \
  _ans.curr += ((int)(elt_sz))*(change); \
  _ans; })
#endif

#ifdef _INLINE_FUNCTIONS
static _INLINE struct _dyneither_ptr
_dyneither_ptr_inplace_plus(struct _dyneither_ptr *arr_ptr,unsigned elt_sz,
                            int change) {
  struct _dyneither_ptr * _arr_ptr = (arr_ptr);
  _arr_ptr->curr += ((int)(elt_sz))*(change);
  return *_arr_ptr;
}
#else
#define _dyneither_ptr_inplace_plus(arr_ptr,elt_sz,change) ({ \
  struct _dyneither_ptr * _arr_ptr = (arr_ptr); \
  _arr_ptr->curr += ((int)(elt_sz))*(change); \
  *_arr_ptr; })
#endif

#ifdef _INLINE_FUNCTIONS
static _INLINE struct _dyneither_ptr
_dyneither_ptr_inplace_plus_post(struct _dyneither_ptr *arr_ptr,unsigned elt_sz,int change) {
  struct _dyneither_ptr * _arr_ptr = (arr_ptr);
  struct _dyneither_ptr _ans = *_arr_ptr;
  _arr_ptr->curr += ((int)(elt_sz))*(change);
  return _ans;
}
#else
#define _dyneither_ptr_inplace_plus_post(arr_ptr,elt_sz,change) ({ \
  struct _dyneither_ptr * _arr_ptr = (arr_ptr); \
  struct _dyneither_ptr _ans = *_arr_ptr; \
  _arr_ptr->curr += ((int)(elt_sz))*(change); \
  _ans; })
#endif

/* Decrease the upper bound on a fat pointer by numelts where sz is
   the size of the pointer's type.  Note that this can't be a macro
   if we're to get initializers right. */
static struct 
_dyneither_ptr _dyneither_ptr_decrease_size(struct _dyneither_ptr x,
                                            unsigned int sz,
                                            unsigned int numelts) {
  x.last_plus_one -= sz * numelts; 
  return x; 
}

/* Allocation */

extern void* GC_malloc(int);
extern void* GC_malloc_atomic(int);
extern void* GC_calloc(unsigned,unsigned);
extern void* GC_calloc_atomic(unsigned,unsigned);

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

/* FIX?  Not sure if we want to pass filename and lineno in here... */
static _INLINE void* _cycalloc(int n) {
  void * ans = (void *)GC_malloc(n);
  if(!ans)
    _throw_badalloc();
  return ans;
}
static _INLINE void* _cycalloc_atomic(int n) {
  void * ans = (void *)GC_malloc_atomic(n);
  if(!ans)
    _throw_badalloc();
  return ans;
}
static _INLINE void* _cyccalloc(unsigned n, unsigned s) {
  void* ans = (void*)GC_calloc(n,s);
  if (!ans)
    _throw_badalloc();
  return ans;
}
static _INLINE void* _cyccalloc_atomic(unsigned n, unsigned s) {
  void* ans = (void*)GC_calloc_atomic(n,s);
  if (!ans)
    _throw_badalloc();
  return ans;
}
#define MAX_MALLOC_SIZE (1 << 28)
static _INLINE unsigned int _check_times(unsigned x, unsigned y) {
  _CYC_U_LONG_LONG_T whole_ans = 
    ((_CYC_U_LONG_LONG_T)x)*((_CYC_U_LONG_LONG_T)y);
  unsigned word_ans = (unsigned)whole_ans;
  if(word_ans < whole_ans || word_ans > MAX_MALLOC_SIZE)
    _throw_badalloc();
  return word_ans;
}

#if defined(CYC_REGION_PROFILE) 
extern void* _profile_GC_malloc(int,const char *file,const char *func,
                                int lineno);
extern void* _profile_GC_malloc_atomic(int,const char *file,
                                       const char *func,int lineno);
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
#endif
#endif

# 35 "core.h"
 typedef char*Cyc_Cstring;
typedef char*Cyc_CstringNN;
typedef struct _dyneither_ptr Cyc_string_t;
# 40
typedef struct _dyneither_ptr Cyc_mstring_t;
# 43
typedef struct _dyneither_ptr*Cyc_stringptr_t;
# 47
typedef struct _dyneither_ptr*Cyc_mstringptr_t;
# 50
typedef char*Cyc_Cbuffer_t;
# 52
typedef char*Cyc_CbufferNN_t;
# 54
typedef struct _dyneither_ptr Cyc_buffer_t;
# 56
typedef struct _dyneither_ptr Cyc_mbuffer_t;
# 59
typedef int Cyc_bool;
# 26 "cycboot.h"
typedef unsigned long Cyc_size_t;
# 33
typedef unsigned short Cyc_mode_t;struct Cyc___cycFILE;
# 49
typedef struct Cyc___cycFILE Cyc_FILE;
# 53
extern struct Cyc___cycFILE*Cyc_stderr;struct Cyc_String_pa_PrintArg_struct{int tag;struct _dyneither_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 68
typedef void*Cyc_parg_t;
# 73
struct _dyneither_ptr Cyc_aprintf(struct _dyneither_ptr,struct _dyneither_ptr);
# 100
int Cyc_fprintf(struct Cyc___cycFILE*,struct _dyneither_ptr,struct _dyneither_ptr);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned int*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _dyneither_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _dyneither_ptr f1;};
# 127
typedef void*Cyc_sarg_t;extern char Cyc_FileCloseError[15];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14];struct Cyc_FileOpenError_exn_struct{char*tag;struct _dyneither_ptr f1;};
# 79 "core.h"
typedef unsigned int Cyc_Core_sizeof_t;struct Cyc_Core_Opt{void*v;};
# 83
typedef struct Cyc_Core_Opt*Cyc_Core_opt_t;struct _tuple0{void*f1;void*f2;};
# 113 "core.h"
void*Cyc_Core_snd(struct _tuple0*);
# 128
int Cyc_Core_ptrcmp(void*,void*);extern char Cyc_Core_Invalid_argument[17];struct Cyc_Core_Invalid_argument_exn_struct{char*tag;struct _dyneither_ptr f1;};extern char Cyc_Core_Failure[8];struct Cyc_Core_Failure_exn_struct{char*tag;struct _dyneither_ptr f1;};extern char Cyc_Core_Impossible[11];struct Cyc_Core_Impossible_exn_struct{char*tag;struct _dyneither_ptr f1;};extern char Cyc_Core_Not_found[10];struct Cyc_Core_Not_found_exn_struct{char*tag;};extern char Cyc_Core_Unreachable[12];struct Cyc_Core_Unreachable_exn_struct{char*tag;struct _dyneither_ptr f1;};
# 167
extern struct _RegionHandle*Cyc_Core_heap_region;
# 170
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;
# 205
typedef struct Cyc_Core_DynamicRegion*Cyc_Core_region_key_t;
# 211
typedef struct Cyc_Core_DynamicRegion*Cyc_Core_uregion_key_t;
# 216
typedef struct Cyc_Core_DynamicRegion*Cyc_Core_rcregion_key_t;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};
# 265 "core.h"
void Cyc_Core_rethrow(void*);
# 290 "core.h"
typedef void*Cyc_Core___cyclone_internal_array_t;
typedef void*Cyc_Core___nn_cyclone_internal_array_t;
typedef unsigned int Cyc_Core___cyclone_internal_singleton;struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 39 "list.h"
typedef struct Cyc_List_List*Cyc_List_list_t;
# 49 "list.h"
typedef struct Cyc_List_List*Cyc_List_List_t;
# 57
struct Cyc_List_List*Cyc_List_rlist(struct _RegionHandle*,struct _dyneither_ptr);
# 61
int Cyc_List_length(struct Cyc_List_List*x);
# 72
struct Cyc_List_List*Cyc_List_rcopy(struct _RegionHandle*,struct Cyc_List_List*x);
# 79
struct Cyc_List_List*Cyc_List_rmap(struct _RegionHandle*,void*(*f)(void*),struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[14];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 135
void Cyc_List_iter_c(void(*f)(void*,void*),void*env,struct Cyc_List_List*x);
# 172
struct Cyc_List_List*Cyc_List_rev(struct Cyc_List_List*x);
# 178
struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*x);
# 190
struct Cyc_List_List*Cyc_List_rappend(struct _RegionHandle*,struct Cyc_List_List*x,struct Cyc_List_List*y);extern char Cyc_List_Nth[4];struct Cyc_List_Nth_exn_struct{char*tag;};
# 242
void*Cyc_List_nth(struct Cyc_List_List*x,int n);
# 276
struct Cyc_List_List*Cyc_List_rzip(struct _RegionHandle*r1,struct _RegionHandle*r2,struct Cyc_List_List*x,struct Cyc_List_List*y);struct _tuple1{struct Cyc_List_List*f1;struct Cyc_List_List*f2;};
# 294
struct _tuple1 Cyc_List_split(struct Cyc_List_List*x);
# 319
int Cyc_List_memq(struct Cyc_List_List*l,void*x);struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};
# 34 "iter.h"
typedef struct Cyc_Iter_Iter Cyc_Iter_iter_t;
# 37
int Cyc_Iter_next(struct Cyc_Iter_Iter,void*);struct Cyc_Set_Set;
# 40 "set.h"
typedef struct Cyc_Set_Set*Cyc_Set_set_t;extern char Cyc_Set_Absent[7];struct Cyc_Set_Absent_exn_struct{char*tag;};struct Cyc_Dict_T;
# 46 "dict.h"
typedef const struct Cyc_Dict_T*Cyc_Dict_tree;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};
# 52
typedef struct Cyc_Dict_Dict Cyc_Dict_dict_t;extern char Cyc_Dict_Present[8];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7];struct Cyc_Dict_Absent_exn_struct{char*tag;};
# 87
struct Cyc_Dict_Dict Cyc_Dict_insert(struct Cyc_Dict_Dict d,void*k,void*v);
# 113
void*Cyc_Dict_lookup_other(struct Cyc_Dict_Dict d,int(*cmp)(void*,void*),void*k);
# 126 "dict.h"
int Cyc_Dict_lookup_bool(struct Cyc_Dict_Dict d,void*k,void**ans);
# 149
void Cyc_Dict_iter_c(void(*f)(void*,void*,void*),void*env,struct Cyc_Dict_Dict d);
# 33 "position.h"
typedef unsigned int Cyc_Position_seg_t;struct Cyc_Position_Error;
# 42
typedef struct Cyc_Position_Error*Cyc_Position_error_t;
# 47
extern int Cyc_Position_num_errors;struct Cyc_Relations_Reln;
# 69 "absyn.h"
typedef struct Cyc_Relations_Reln*Cyc_Relations_reln_t;
typedef struct Cyc_List_List*Cyc_Relations_relns_t;
# 74
typedef void*Cyc_Tcpat_decision_opt_t;
# 81
typedef unsigned int Cyc_Absyn_seg_t;
# 83
typedef struct _dyneither_ptr*Cyc_Absyn_field_name_t;
typedef struct _dyneither_ptr*Cyc_Absyn_var_t;
typedef struct _dyneither_ptr*Cyc_Absyn_tvarname_t;
typedef struct _dyneither_ptr*Cyc_Absyn_var_opt_t;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};
# 95
typedef union Cyc_Absyn_Nmspace Cyc_Absyn_nmspace_t;
union Cyc_Absyn_Nmspace Cyc_Absyn_Loc_n;
union Cyc_Absyn_Nmspace Cyc_Absyn_Rel_n(struct Cyc_List_List*);
# 99
union Cyc_Absyn_Nmspace Cyc_Absyn_Abs_n(struct Cyc_List_List*ns,int C_scope);struct _tuple2{union Cyc_Absyn_Nmspace f1;struct _dyneither_ptr*f2;};
# 102
typedef struct _tuple2*Cyc_Absyn_qvar_t;typedef struct _tuple2*Cyc_Absyn_qvar_opt_t;
typedef struct _tuple2*Cyc_Absyn_typedef_name_t;
typedef struct _tuple2*Cyc_Absyn_typedef_name_opt_t;
# 107
typedef enum Cyc_Absyn_Scope Cyc_Absyn_scope_t;
typedef struct Cyc_Absyn_Tqual Cyc_Absyn_tqual_t;
typedef enum Cyc_Absyn_Size_of Cyc_Absyn_size_of_t;
typedef struct Cyc_Absyn_Kind*Cyc_Absyn_kind_t;
typedef void*Cyc_Absyn_kindbound_t;
typedef struct Cyc_Absyn_Tvar*Cyc_Absyn_tvar_t;
typedef enum Cyc_Absyn_Sign Cyc_Absyn_sign_t;
typedef enum Cyc_Absyn_AggrKind Cyc_Absyn_aggr_kind_t;
typedef void*Cyc_Absyn_bounds_t;
typedef struct Cyc_Absyn_PtrAtts Cyc_Absyn_ptr_atts_t;
typedef struct Cyc_Absyn_PtrInfo Cyc_Absyn_ptr_info_t;
typedef struct Cyc_Absyn_VarargInfo Cyc_Absyn_vararg_info_t;
typedef struct Cyc_Absyn_FnInfo Cyc_Absyn_fn_info_t;
typedef struct Cyc_Absyn_DatatypeInfo Cyc_Absyn_datatype_info_t;
typedef struct Cyc_Absyn_DatatypeFieldInfo Cyc_Absyn_datatype_field_info_t;
typedef struct Cyc_Absyn_AggrInfo Cyc_Absyn_aggr_info_t;
typedef struct Cyc_Absyn_ArrayInfo Cyc_Absyn_array_info_t;
typedef void*Cyc_Absyn_type_t;typedef void*Cyc_Absyn_rgntype_t;typedef void*Cyc_Absyn_type_opt_t;
typedef union Cyc_Absyn_Cnst Cyc_Absyn_cnst_t;
typedef enum Cyc_Absyn_Primop Cyc_Absyn_primop_t;
typedef enum Cyc_Absyn_Incrementor Cyc_Absyn_incrementor_t;
typedef struct Cyc_Absyn_VarargCallInfo Cyc_Absyn_vararg_call_info_t;
typedef void*Cyc_Absyn_raw_exp_t;
typedef struct Cyc_Absyn_Exp*Cyc_Absyn_exp_t;typedef struct Cyc_Absyn_Exp*Cyc_Absyn_exp_opt_t;
typedef void*Cyc_Absyn_raw_stmt_t;
typedef struct Cyc_Absyn_Stmt*Cyc_Absyn_stmt_t;typedef struct Cyc_Absyn_Stmt*Cyc_Absyn_stmt_opt_t;
typedef void*Cyc_Absyn_raw_pat_t;
typedef struct Cyc_Absyn_Pat*Cyc_Absyn_pat_t;
typedef void*Cyc_Absyn_binding_t;
typedef struct Cyc_Absyn_Switch_clause*Cyc_Absyn_switch_clause_t;
typedef struct Cyc_Absyn_Fndecl*Cyc_Absyn_fndecl_t;
typedef struct Cyc_Absyn_Aggrdecl*Cyc_Absyn_aggrdecl_t;
typedef struct Cyc_Absyn_Datatypefield*Cyc_Absyn_datatypefield_t;
typedef struct Cyc_Absyn_Datatypedecl*Cyc_Absyn_datatypedecl_t;
typedef struct Cyc_Absyn_Typedefdecl*Cyc_Absyn_typedefdecl_t;
typedef struct Cyc_Absyn_Enumfield*Cyc_Absyn_enumfield_t;
typedef struct Cyc_Absyn_Enumdecl*Cyc_Absyn_enumdecl_t;
typedef struct Cyc_Absyn_Vardecl*Cyc_Absyn_vardecl_t;typedef struct Cyc_Absyn_Vardecl*Cyc_Absyn_vardecl_opt_t;
typedef void*Cyc_Absyn_raw_decl_t;
typedef struct Cyc_Absyn_Decl*Cyc_Absyn_decl_t;
typedef void*Cyc_Absyn_designator_t;
typedef void*Cyc_Absyn_absyn_annot_t;
typedef void*Cyc_Absyn_attribute_t;
typedef struct Cyc_List_List*Cyc_Absyn_attributes_t;
typedef struct Cyc_Absyn_Aggrfield*Cyc_Absyn_aggrfield_t;
typedef void*Cyc_Absyn_offsetof_field_t;
typedef struct Cyc_Absyn_MallocInfo Cyc_Absyn_malloc_info_t;
typedef enum Cyc_Absyn_Coercion Cyc_Absyn_coercion_t;
typedef struct Cyc_Absyn_PtrLoc*Cyc_Absyn_ptrloc_t;
# 158
enum Cyc_Absyn_Scope{Cyc_Absyn_Static  = 0,Cyc_Absyn_Abstract  = 1,Cyc_Absyn_Public  = 2,Cyc_Absyn_Extern  = 3,Cyc_Absyn_ExternC  = 4,Cyc_Absyn_Register  = 5};struct Cyc_Absyn_Tqual{int print_const: 1;int q_volatile: 1;int q_restrict: 1;int real_const: 1;unsigned int loc;};
# 179
enum Cyc_Absyn_Size_of{Cyc_Absyn_Char_sz  = 0,Cyc_Absyn_Short_sz  = 1,Cyc_Absyn_Int_sz  = 2,Cyc_Absyn_Long_sz  = 3,Cyc_Absyn_LongLong_sz  = 4};
# 184
enum Cyc_Absyn_AliasQual{Cyc_Absyn_Aliasable  = 0,Cyc_Absyn_Unique  = 1,Cyc_Absyn_Top  = 2};
# 190
enum Cyc_Absyn_KindQual{Cyc_Absyn_AnyKind  = 0,Cyc_Absyn_MemKind  = 1,Cyc_Absyn_BoxKind  = 2,Cyc_Absyn_RgnKind  = 3,Cyc_Absyn_EffKind  = 4,Cyc_Absyn_IntKind  = 5};struct Cyc_Absyn_Kind{enum Cyc_Absyn_KindQual kind;enum Cyc_Absyn_AliasQual aliasqual;};
# 210
enum Cyc_Absyn_Sign{Cyc_Absyn_Signed  = 0,Cyc_Absyn_Unsigned  = 1,Cyc_Absyn_None  = 2};
# 212
enum Cyc_Absyn_AggrKind{Cyc_Absyn_StructA  = 0,Cyc_Absyn_UnionA  = 1};struct _union_Constraint_Eq_constr{int tag;void*val;};struct _union_Constraint_Forward_constr{int tag;union Cyc_Absyn_Constraint*val;};struct _union_Constraint_No_constr{int tag;int val;};union Cyc_Absyn_Constraint{struct _union_Constraint_Eq_constr Eq_constr;struct _union_Constraint_Forward_constr Forward_constr;struct _union_Constraint_No_constr No_constr;};
# 221
typedef union Cyc_Absyn_Constraint*Cyc_Absyn_conref_t;struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct{int tag;struct Cyc_Absyn_Kind*f1;};struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;};struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_Tvar{struct _dyneither_ptr*name;int identity;void*kind;};struct Cyc_Absyn_DynEither_b_Absyn_Bounds_struct{int tag;};struct Cyc_Absyn_Upper_b_Absyn_Bounds_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_PtrLoc{unsigned int ptr_loc;unsigned int rgn_loc;unsigned int zt_loc;};struct Cyc_Absyn_PtrAtts{void*rgn;union Cyc_Absyn_Constraint*nullable;union Cyc_Absyn_Constraint*bounds;union Cyc_Absyn_Constraint*zero_term;struct Cyc_Absyn_PtrLoc*ptrloc;};struct Cyc_Absyn_PtrInfo{void*elt_typ;struct Cyc_Absyn_Tqual elt_tq;struct Cyc_Absyn_PtrAtts ptr_atts;};struct Cyc_Absyn_VarargInfo{struct _dyneither_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;int inject;};struct Cyc_Absyn_FnInfo{struct Cyc_List_List*tvars;void*effect;struct Cyc_Absyn_Tqual ret_tqual;void*ret_typ;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*cyc_varargs;struct Cyc_List_List*rgn_po;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;struct Cyc_List_List*requires_relns;struct Cyc_Absyn_Exp*ensures_clause;struct Cyc_List_List*ensures_relns;};struct Cyc_Absyn_UnknownDatatypeInfo{struct _tuple2*name;int is_extensible;};struct _union_DatatypeInfoU_UnknownDatatype{int tag;struct Cyc_Absyn_UnknownDatatypeInfo val;};struct _union_DatatypeInfoU_KnownDatatype{int tag;struct Cyc_Absyn_Datatypedecl**val;};union Cyc_Absyn_DatatypeInfoU{struct _union_DatatypeInfoU_UnknownDatatype UnknownDatatype;struct _union_DatatypeInfoU_KnownDatatype KnownDatatype;};struct Cyc_Absyn_DatatypeInfo{union Cyc_Absyn_DatatypeInfoU datatype_info;struct Cyc_List_List*targs;};struct Cyc_Absyn_UnknownDatatypeFieldInfo{struct _tuple2*datatype_name;struct _tuple2*field_name;int is_extensible;};struct _union_DatatypeFieldInfoU_UnknownDatatypefield{int tag;struct Cyc_Absyn_UnknownDatatypeFieldInfo val;};struct _tuple3{struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;};struct _union_DatatypeFieldInfoU_KnownDatatypefield{int tag;struct _tuple3 val;};union Cyc_Absyn_DatatypeFieldInfoU{struct _union_DatatypeFieldInfoU_UnknownDatatypefield UnknownDatatypefield;struct _union_DatatypeFieldInfoU_KnownDatatypefield KnownDatatypefield;};struct Cyc_Absyn_DatatypeFieldInfo{union Cyc_Absyn_DatatypeFieldInfoU field_info;struct Cyc_List_List*targs;};struct _tuple4{enum Cyc_Absyn_AggrKind f1;struct _tuple2*f2;struct Cyc_Core_Opt*f3;};struct _union_AggrInfoU_UnknownAggr{int tag;struct _tuple4 val;};struct _union_AggrInfoU_KnownAggr{int tag;struct Cyc_Absyn_Aggrdecl**val;};union Cyc_Absyn_AggrInfoU{struct _union_AggrInfoU_UnknownAggr UnknownAggr;struct _union_AggrInfoU_KnownAggr KnownAggr;};struct Cyc_Absyn_AggrInfo{union Cyc_Absyn_AggrInfoU aggr_info;struct Cyc_List_List*targs;};struct Cyc_Absyn_ArrayInfo{void*elt_type;struct Cyc_Absyn_Tqual tq;struct Cyc_Absyn_Exp*num_elts;union Cyc_Absyn_Constraint*zero_term;unsigned int zt_loc;};struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};
# 367
typedef void*Cyc_Absyn_raw_type_decl_t;struct Cyc_Absyn_TypeDecl{void*r;unsigned int loc;};
# 372
typedef struct Cyc_Absyn_TypeDecl*Cyc_Absyn_type_decl_t;struct Cyc_Absyn_VoidType_Absyn_Type_struct{int tag;};struct Cyc_Absyn_Evar_Absyn_Type_struct{int tag;struct Cyc_Core_Opt*f1;void*f2;int f3;struct Cyc_Core_Opt*f4;};struct Cyc_Absyn_VarType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Tvar*f1;};struct Cyc_Absyn_DatatypeType_Absyn_Type_struct{int tag;struct Cyc_Absyn_DatatypeInfo f1;};struct Cyc_Absyn_DatatypeFieldType_Absyn_Type_struct{int tag;struct Cyc_Absyn_DatatypeFieldInfo f1;};struct Cyc_Absyn_PointerType_Absyn_Type_struct{int tag;struct Cyc_Absyn_PtrInfo f1;};struct Cyc_Absyn_IntType_Absyn_Type_struct{int tag;enum Cyc_Absyn_Sign f1;enum Cyc_Absyn_Size_of f2;};struct Cyc_Absyn_FloatType_Absyn_Type_struct{int tag;int f1;};struct Cyc_Absyn_ArrayType_Absyn_Type_struct{int tag;struct Cyc_Absyn_ArrayInfo f1;};struct Cyc_Absyn_FnType_Absyn_Type_struct{int tag;struct Cyc_Absyn_FnInfo f1;};struct Cyc_Absyn_TupleType_Absyn_Type_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_AggrType_Absyn_Type_struct{int tag;struct Cyc_Absyn_AggrInfo f1;};struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct{int tag;enum Cyc_Absyn_AggrKind f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_EnumType_Absyn_Type_struct{int tag;struct _tuple2*f1;struct Cyc_Absyn_Enumdecl*f2;};struct Cyc_Absyn_AnonEnumType_Absyn_Type_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_RgnHandleType_Absyn_Type_struct{int tag;void*f1;};struct Cyc_Absyn_DynRgnType_Absyn_Type_struct{int tag;void*f1;void*f2;};struct Cyc_Absyn_TypedefType_Absyn_Type_struct{int tag;struct _tuple2*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_Typedefdecl*f3;void*f4;};struct Cyc_Absyn_ValueofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_TagType_Absyn_Type_struct{int tag;void*f1;};struct Cyc_Absyn_HeapRgn_Absyn_Type_struct{int tag;};struct Cyc_Absyn_UniqueRgn_Absyn_Type_struct{int tag;};struct Cyc_Absyn_RefCntRgn_Absyn_Type_struct{int tag;};struct Cyc_Absyn_AccessEff_Absyn_Type_struct{int tag;void*f1;};struct Cyc_Absyn_JoinEff_Absyn_Type_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_RgnsEff_Absyn_Type_struct{int tag;void*f1;};struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct{int tag;struct Cyc_Absyn_TypeDecl*f1;void**f2;};struct Cyc_Absyn_TypeofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_BuiltinType_Absyn_Type_struct{int tag;struct _dyneither_ptr f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;unsigned int f2;};struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;int f2;struct Cyc_Absyn_VarargInfo*f3;void*f4;struct Cyc_List_List*f5;struct Cyc_Absyn_Exp*f6;struct Cyc_Absyn_Exp*f7;};
# 443 "absyn.h"
typedef void*Cyc_Absyn_funcparams_t;
# 446
enum Cyc_Absyn_Format_Type{Cyc_Absyn_Printf_ft  = 0,Cyc_Absyn_Scanf_ft  = 1};struct Cyc_Absyn_Regparm_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Stdcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Cdecl_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Fastcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Noreturn_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Const_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Packed_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Section_att_Absyn_Attribute_struct{int tag;struct _dyneither_ptr f1;};struct Cyc_Absyn_Nocommon_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Shared_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Unused_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Weak_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllimport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllexport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_instrument_function_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Constructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Destructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_check_memory_usage_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Format_att_Absyn_Attribute_struct{int tag;enum Cyc_Absyn_Format_Type f1;int f2;int f3;};struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Noliveunique_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Noconsume_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Pure_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Mode_att_Absyn_Attribute_struct{int tag;struct _dyneither_ptr f1;};struct Cyc_Absyn_Alias_att_Absyn_Attribute_struct{int tag;struct _dyneither_ptr f1;};struct Cyc_Absyn_Always_inline_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct{int tag;union Cyc_Absyn_Constraint*f1;unsigned int f2;};struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_Exp*f1;union Cyc_Absyn_Constraint*f2;unsigned int f3;};struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_PtrAtts f1;struct Cyc_Absyn_Tqual f2;};struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct{int tag;void*f1;};struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_List_List*f1;unsigned int f2;int f3;};struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct{int tag;unsigned int f1;struct Cyc_List_List*f2;};
# 510
typedef void*Cyc_Absyn_type_modifier_t;struct _union_Cnst_Null_c{int tag;int val;};struct _tuple5{enum Cyc_Absyn_Sign f1;char f2;};struct _union_Cnst_Char_c{int tag;struct _tuple5 val;};struct _union_Cnst_Wchar_c{int tag;struct _dyneither_ptr val;};struct _tuple6{enum Cyc_Absyn_Sign f1;short f2;};struct _union_Cnst_Short_c{int tag;struct _tuple6 val;};struct _tuple7{enum Cyc_Absyn_Sign f1;int f2;};struct _union_Cnst_Int_c{int tag;struct _tuple7 val;};struct _tuple8{enum Cyc_Absyn_Sign f1;long long f2;};struct _union_Cnst_LongLong_c{int tag;struct _tuple8 val;};struct _tuple9{struct _dyneither_ptr f1;int f2;};struct _union_Cnst_Float_c{int tag;struct _tuple9 val;};struct _union_Cnst_String_c{int tag;struct _dyneither_ptr val;};struct _union_Cnst_Wstring_c{int tag;struct _dyneither_ptr val;};union Cyc_Absyn_Cnst{struct _union_Cnst_Null_c Null_c;struct _union_Cnst_Char_c Char_c;struct _union_Cnst_Wchar_c Wchar_c;struct _union_Cnst_Short_c Short_c;struct _union_Cnst_Int_c Int_c;struct _union_Cnst_LongLong_c LongLong_c;struct _union_Cnst_Float_c Float_c;struct _union_Cnst_String_c String_c;struct _union_Cnst_Wstring_c Wstring_c;};
# 536
enum Cyc_Absyn_Primop{Cyc_Absyn_Plus  = 0,Cyc_Absyn_Times  = 1,Cyc_Absyn_Minus  = 2,Cyc_Absyn_Div  = 3,Cyc_Absyn_Mod  = 4,Cyc_Absyn_Eq  = 5,Cyc_Absyn_Neq  = 6,Cyc_Absyn_Gt  = 7,Cyc_Absyn_Lt  = 8,Cyc_Absyn_Gte  = 9,Cyc_Absyn_Lte  = 10,Cyc_Absyn_Not  = 11,Cyc_Absyn_Bitnot  = 12,Cyc_Absyn_Bitand  = 13,Cyc_Absyn_Bitor  = 14,Cyc_Absyn_Bitxor  = 15,Cyc_Absyn_Bitlshift  = 16,Cyc_Absyn_Bitlrshift  = 17,Cyc_Absyn_Bitarshift  = 18,Cyc_Absyn_Numelts  = 19};
# 543
enum Cyc_Absyn_Incrementor{Cyc_Absyn_PreInc  = 0,Cyc_Absyn_PostInc  = 1,Cyc_Absyn_PreDec  = 2,Cyc_Absyn_PostDec  = 3};struct Cyc_Absyn_VarargCallInfo{int num_varargs;struct Cyc_List_List*injectors;struct Cyc_Absyn_VarargInfo*vai;};struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct{int tag;struct _dyneither_ptr*f1;};struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct{int tag;unsigned int f1;};
# 561
enum Cyc_Absyn_Coercion{Cyc_Absyn_Unknown_coercion  = 0,Cyc_Absyn_No_coercion  = 1,Cyc_Absyn_Null_to_NonNull  = 2,Cyc_Absyn_Other_coercion  = 3};struct Cyc_Absyn_MallocInfo{int is_calloc;struct Cyc_Absyn_Exp*rgn;void**elt_type;struct Cyc_Absyn_Exp*num_elts;int fat_result;int inline_call;};struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct{int tag;union Cyc_Absyn_Cnst f1;};struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct{int tag;enum Cyc_Absyn_Primop f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;enum Cyc_Absyn_Incrementor f2;};struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_VarargCallInfo*f3;int f4;};struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;};struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Exp*f2;int f3;enum Cyc_Absyn_Coercion f4;};struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Sizeoftyp_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _dyneither_ptr*f2;int f3;int f4;};struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _dyneither_ptr*f2;int f3;int f4;};struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct _tuple10{struct _dyneither_ptr*f1;struct Cyc_Absyn_Tqual f2;void*f3;};struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct{int tag;struct _tuple10*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;int f4;};struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;int f3;};struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct{int tag;struct _tuple2*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*f4;};struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_MallocInfo f1;};struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _dyneither_ptr*f2;};struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct{int tag;int f1;struct _dyneither_ptr f2;};struct Cyc_Absyn_Exp{void*topt;void*r;unsigned int loc;void*annot;};struct Cyc_Absyn_Skip_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;struct Cyc_Absyn_Stmt*f3;};struct _tuple11{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct{int tag;struct _tuple11 f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Break_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Continue_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct{int tag;struct _dyneither_ptr*f1;};struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _tuple11 f2;struct _tuple11 f3;struct Cyc_Absyn_Stmt*f4;};struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Switch_clause**f2;};struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct{int tag;struct _dyneither_ptr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct _tuple11 f2;};struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Stmt{void*r;unsigned int loc;void*annot;};struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct{int tag;struct Cyc_List_List*f1;int f2;};struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Pat*f1;};struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_AggrInfo*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Null_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct{int tag;enum Cyc_Absyn_Sign f1;int f2;};struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct{int tag;char f1;};struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct{int tag;struct _dyneither_ptr f1;int f2;};struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_UnknownId_p_Absyn_Raw_pat_struct{int tag;struct _tuple2*f1;};struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct{int tag;struct _tuple2*f1;struct Cyc_List_List*f2;int f3;};struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Pat{void*r;void*topt;unsigned int loc;};struct Cyc_Absyn_Switch_clause{struct Cyc_Absyn_Pat*pattern;struct Cyc_Core_Opt*pat_vars;struct Cyc_Absyn_Exp*where_clause;struct Cyc_Absyn_Stmt*body;unsigned int loc;};struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct{int tag;struct _tuple2*f1;};struct Cyc_Absyn_Global_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Funname_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Param_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Local_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Pat_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Vardecl{enum Cyc_Absyn_Scope sc;struct _tuple2*name;unsigned int varloc;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*initializer;void*rgn;struct Cyc_List_List*attributes;int escapes;};struct Cyc_Absyn_Fndecl{enum Cyc_Absyn_Scope sc;int is_inline;struct _tuple2*name;struct Cyc_List_List*tvs;void*effect;struct Cyc_Absyn_Tqual ret_tqual;void*ret_type;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*cyc_varargs;struct Cyc_List_List*rgn_po;struct Cyc_Absyn_Stmt*body;void*cached_typ;struct Cyc_Core_Opt*param_vardecls;struct Cyc_Absyn_Vardecl*fn_vardecl;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;struct Cyc_List_List*requires_relns;struct Cyc_Absyn_Exp*ensures_clause;struct Cyc_List_List*ensures_relns;};struct Cyc_Absyn_Aggrfield{struct _dyneither_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*width;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;};struct Cyc_Absyn_AggrdeclImpl{struct Cyc_List_List*exist_vars;struct Cyc_List_List*rgn_po;struct Cyc_List_List*fields;int tagged;};struct Cyc_Absyn_Aggrdecl{enum Cyc_Absyn_AggrKind kind;enum Cyc_Absyn_Scope sc;struct _tuple2*name;struct Cyc_List_List*tvs;struct Cyc_Absyn_AggrdeclImpl*impl;struct Cyc_List_List*attributes;int expected_mem_kind;};struct Cyc_Absyn_Datatypefield{struct _tuple2*name;struct Cyc_List_List*typs;unsigned int loc;enum Cyc_Absyn_Scope sc;};struct Cyc_Absyn_Datatypedecl{enum Cyc_Absyn_Scope sc;struct _tuple2*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*fields;int is_extensible;};struct Cyc_Absyn_Enumfield{struct _tuple2*name;struct Cyc_Absyn_Exp*tag;unsigned int loc;};struct Cyc_Absyn_Enumdecl{enum Cyc_Absyn_Scope sc;struct _tuple2*name;struct Cyc_Core_Opt*fields;};struct Cyc_Absyn_Typedefdecl{struct _tuple2*name;struct Cyc_Absyn_Tqual tq;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*kind;void*defn;struct Cyc_List_List*atts;int extern_c;};struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Pat*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;void*f4;};struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Typedefdecl*f1;};struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct{int tag;struct _dyneither_ptr*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct{int tag;struct _tuple2*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Porton_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Portoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Decl{void*r;unsigned int loc;};struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_FieldName_Absyn_Designator_struct{int tag;struct _dyneither_ptr*f1;};extern char Cyc_Absyn_EmptyAnnot[11];struct Cyc_Absyn_EmptyAnnot_Absyn_AbsynAnnot_struct{char*tag;};
# 977 "absyn.h"
extern struct Cyc_Absyn_Exp*Cyc_Absyn_exp_unsigned_one;
# 1018
struct Cyc_Absyn_Exp*Cyc_Absyn_signed_int_exp(int,unsigned int);
# 1029
struct Cyc_Absyn_Exp*Cyc_Absyn_prim1_exp(enum Cyc_Absyn_Primop,struct Cyc_Absyn_Exp*,unsigned int);
# 1159
struct Cyc_Absyn_Aggrfield*Cyc_Absyn_lookup_field(struct Cyc_List_List*,struct _dyneither_ptr*);
# 1161
struct Cyc_Absyn_Aggrfield*Cyc_Absyn_lookup_decl_field(struct Cyc_Absyn_Aggrdecl*,struct _dyneither_ptr*);
# 1173
struct Cyc_Absyn_Aggrdecl*Cyc_Absyn_get_known_aggrdecl(union Cyc_Absyn_AggrInfoU info);
# 1177
int Cyc_Absyn_is_nontagged_nonrequire_union_type(void*);
# 1179
int Cyc_Absyn_is_require_union_type(void*);struct Cyc_RgnOrder_RgnPO;
# 30 "rgnorder.h"
typedef struct Cyc_RgnOrder_RgnPO*Cyc_RgnOrder_rgn_po_t;
# 32
struct Cyc_RgnOrder_RgnPO*Cyc_RgnOrder_initial_fn_po(struct _RegionHandle*,struct Cyc_List_List*tvs,struct Cyc_List_List*po,void*effect,struct Cyc_Absyn_Tvar*fst_rgn,unsigned int);
# 39
struct Cyc_RgnOrder_RgnPO*Cyc_RgnOrder_add_outlives_constraint(struct _RegionHandle*,struct Cyc_RgnOrder_RgnPO*po,void*eff,void*rgn,unsigned int loc);
struct Cyc_RgnOrder_RgnPO*Cyc_RgnOrder_add_youngest(struct _RegionHandle*,struct Cyc_RgnOrder_RgnPO*po,struct Cyc_Absyn_Tvar*rgn,int opened);
struct Cyc_RgnOrder_RgnPO*Cyc_RgnOrder_add_unordered(struct _RegionHandle*,struct Cyc_RgnOrder_RgnPO*po,struct Cyc_Absyn_Tvar*rgn);
int Cyc_RgnOrder_effect_outlives(struct Cyc_RgnOrder_RgnPO*po,void*eff,void*rgn);
int Cyc_RgnOrder_satisfies_constraints(struct Cyc_RgnOrder_RgnPO*po,struct Cyc_List_List*constraints,void*default_bound,int do_pin);
# 45
int Cyc_RgnOrder_eff_outlives_eff(struct Cyc_RgnOrder_RgnPO*po,void*eff1,void*eff2);
# 48
void Cyc_RgnOrder_print_region_po(struct Cyc_RgnOrder_RgnPO*po);extern char Cyc_Tcenv_Env_error[10];struct Cyc_Tcenv_Env_error_exn_struct{char*tag;};struct Cyc_Tcenv_Genv{struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;};
# 48 "tcenv.h"
typedef struct Cyc_Tcenv_Genv*Cyc_Tcenv_genv_t;struct Cyc_Tcenv_Fenv;
# 52
typedef struct Cyc_Tcenv_Fenv*Cyc_Tcenv_fenv_t;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Tcenv_Genv*ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof;int in_extern_c_include;};
# 62
typedef struct Cyc_Tcenv_Tenv*Cyc_Tcenv_tenv_t;
# 86
enum Cyc_Tcenv_NewStatus{Cyc_Tcenv_NoneNew  = 0,Cyc_Tcenv_InNew  = 1,Cyc_Tcenv_InNewAggr  = 2};
# 30 "tcutil.h"
void*Cyc_Tcutil_impos(struct _dyneither_ptr fmt,struct _dyneither_ptr ap);
# 32
void Cyc_Tcutil_terr(unsigned int,struct _dyneither_ptr fmt,struct _dyneither_ptr ap);
# 34
void Cyc_Tcutil_warn(unsigned int,struct _dyneither_ptr fmt,struct _dyneither_ptr ap);
# 55
void*Cyc_Tcutil_compress(void*t);
# 90
void*Cyc_Tcutil_pointer_elt_type(void*t);
# 150
void*Cyc_Tcutil_rsubstitute(struct _RegionHandle*,struct Cyc_List_List*,void*);
# 163
struct Cyc_Absyn_Exp*Cyc_Tcutil_rsubsexp(struct _RegionHandle*r,struct Cyc_List_List*,struct Cyc_Absyn_Exp*);
# 172
void*Cyc_Tcutil_fndecl2typ(struct Cyc_Absyn_Fndecl*);
# 233 "tcutil.h"
int Cyc_Tcutil_is_bound_one(union Cyc_Absyn_Constraint*b);
# 265
int Cyc_Tcutil_is_noalias_pointer(void*t,int must_be_unique);
# 275
int Cyc_Tcutil_is_noalias_pointer_or_aggr(struct _RegionHandle*,void*t);
# 326
int Cyc_Tcutil_is_noreturn(void*);
# 344
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_type_bound(void*t);
# 348
struct Cyc_Absyn_Vardecl*Cyc_Tcutil_nonesc_vardecl(void*b);
# 351
struct Cyc_List_List*Cyc_Tcutil_filter_nulls(struct Cyc_List_List*l);struct _tuple12{unsigned int f1;int f2;};
# 28 "evexp.h"
struct _tuple12 Cyc_Evexp_eval_const_uint_exp(struct Cyc_Absyn_Exp*e);struct _union_RelnOp_RConst{int tag;unsigned int val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned int val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned int val;};struct _union_RelnOp_RReturn{int tag;unsigned int val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 38 "relations.h"
typedef union Cyc_Relations_RelnOp Cyc_Relations_reln_op_t;
# 40
union Cyc_Relations_RelnOp Cyc_Relations_RConst(unsigned int);union Cyc_Relations_RelnOp Cyc_Relations_RVar(struct Cyc_Absyn_Vardecl*);union Cyc_Relations_RelnOp Cyc_Relations_RNumelts(struct Cyc_Absyn_Vardecl*);
# 49
enum Cyc_Relations_Relation{Cyc_Relations_Req  = 0,Cyc_Relations_Rneq  = 1,Cyc_Relations_Rlte  = 2,Cyc_Relations_Rlt  = 3};
typedef enum Cyc_Relations_Relation Cyc_Relations_relation_t;struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};struct _tuple13{struct Cyc_Absyn_Exp*f1;enum Cyc_Relations_Relation f2;struct Cyc_Absyn_Exp*f3;};
# 63
struct _tuple13 Cyc_Relations_primop2relation(struct Cyc_Absyn_Exp*e1,enum Cyc_Absyn_Primop p,struct Cyc_Absyn_Exp*e2);
# 67
enum Cyc_Relations_Relation Cyc_Relations_flip_relation(enum Cyc_Relations_Relation r);
# 69
struct Cyc_Relations_Reln*Cyc_Relations_negate(struct _RegionHandle*,struct Cyc_Relations_Reln*);
# 75
int Cyc_Relations_exp2relnop(struct Cyc_Absyn_Exp*e,union Cyc_Relations_RelnOp*p);
# 83
struct Cyc_List_List*Cyc_Relations_exp2relns(struct _RegionHandle*r,struct Cyc_Absyn_Exp*e);
# 86
struct Cyc_List_List*Cyc_Relations_add_relation(struct _RegionHandle*rgn,union Cyc_Relations_RelnOp rop1,enum Cyc_Relations_Relation r,union Cyc_Relations_RelnOp rop2,struct Cyc_List_List*relns);
# 99
struct Cyc_List_List*Cyc_Relations_reln_assign_var(struct _RegionHandle*,struct Cyc_List_List*,struct Cyc_Absyn_Vardecl*,struct Cyc_Absyn_Exp*);
# 102
struct Cyc_List_List*Cyc_Relations_reln_assign_exp(struct _RegionHandle*,struct Cyc_List_List*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*);
# 107
struct Cyc_List_List*Cyc_Relations_reln_kill_exp(struct _RegionHandle*,struct Cyc_List_List*,struct Cyc_Absyn_Exp*);
# 109
struct Cyc_List_List*Cyc_Relations_copy_relns(struct _RegionHandle*,struct Cyc_List_List*);
# 111
int Cyc_Relations_same_relns(struct Cyc_List_List*,struct Cyc_List_List*);
# 117
void Cyc_Relations_print_relns(struct Cyc___cycFILE*,struct Cyc_List_List*);
# 120
struct _dyneither_ptr Cyc_Relations_rop2string(union Cyc_Relations_RelnOp r);
struct _dyneither_ptr Cyc_Relations_relation2string(enum Cyc_Relations_Relation r);
struct _dyneither_ptr Cyc_Relations_relns2string(struct Cyc_List_List*r);
# 128
int Cyc_Relations_consistent_relations(struct Cyc_List_List*rlns);struct Cyc_Hashtable_Table;
# 35 "hashtable.h"
typedef struct Cyc_Hashtable_Table*Cyc_Hashtable_table_t;
# 47
struct Cyc_Hashtable_Table*Cyc_Hashtable_rcreate(struct _RegionHandle*r,int sz,int(*cmp)(void*,void*),int(*hash)(void*));
# 50
void Cyc_Hashtable_insert(struct Cyc_Hashtable_Table*t,void*key,void*val);
# 52
void*Cyc_Hashtable_lookup(struct Cyc_Hashtable_Table*t,void*key);
# 56
void**Cyc_Hashtable_lookup_opt(struct Cyc_Hashtable_Table*t,void*key);
# 29 "jump_analysis.h"
typedef struct Cyc_Hashtable_Table*Cyc_JumpAnalysis_table_t;struct Cyc_JumpAnalysis_Jump_Anal_Result{struct Cyc_Hashtable_Table*pop_tables;struct Cyc_Hashtable_Table*succ_tables;struct Cyc_Hashtable_Table*pat_pop_tables;};
# 44 "jump_analysis.h"
typedef struct Cyc_JumpAnalysis_Jump_Anal_Result*Cyc_JumpAnalysis_jump_anal_res_t;
# 46
struct Cyc_JumpAnalysis_Jump_Anal_Result*Cyc_JumpAnalysis_jump_analysis(struct Cyc_List_List*tds);
# 41 "cf_flowinfo.h"
int Cyc_CfFlowInfo_anal_error;
void Cyc_CfFlowInfo_aerr(unsigned int loc,struct _dyneither_ptr fmt,struct _dyneither_ptr ap);struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_CfFlowInfo_MallocPt_CfFlowInfo_Root_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;};struct Cyc_CfFlowInfo_InitParam_CfFlowInfo_Root_struct{int tag;int f1;void*f2;};
# 52
typedef void*Cyc_CfFlowInfo_root_t;struct Cyc_CfFlowInfo_Place{void*root;struct Cyc_List_List*fields;};
# 63
typedef struct Cyc_CfFlowInfo_Place*Cyc_CfFlowInfo_place_t;
# 65
enum Cyc_CfFlowInfo_InitLevel{Cyc_CfFlowInfo_NoneIL  = 0,Cyc_CfFlowInfo_ThisIL  = 1,Cyc_CfFlowInfo_AllIL  = 2};
# 70
typedef enum Cyc_CfFlowInfo_InitLevel Cyc_CfFlowInfo_initlevel_t;extern char Cyc_CfFlowInfo_IsZero[7];struct Cyc_CfFlowInfo_IsZero_Absyn_AbsynAnnot_struct{char*tag;};extern char Cyc_CfFlowInfo_NotZero[8];struct Cyc_CfFlowInfo_NotZero_Absyn_AbsynAnnot_struct{char*tag;struct Cyc_List_List*f1;};extern char Cyc_CfFlowInfo_UnknownZ[9];struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct{char*tag;struct Cyc_List_List*f1;};
# 79
extern struct Cyc_CfFlowInfo_IsZero_Absyn_AbsynAnnot_struct Cyc_CfFlowInfo_IsZero_val;struct _union_AbsLVal_PlaceL{int tag;struct Cyc_CfFlowInfo_Place*val;};struct _union_AbsLVal_UnknownL{int tag;int val;};union Cyc_CfFlowInfo_AbsLVal{struct _union_AbsLVal_PlaceL PlaceL;struct _union_AbsLVal_UnknownL UnknownL;};
# 85
typedef union Cyc_CfFlowInfo_AbsLVal Cyc_CfFlowInfo_absLval_t;
union Cyc_CfFlowInfo_AbsLVal Cyc_CfFlowInfo_PlaceL(struct Cyc_CfFlowInfo_Place*);
union Cyc_CfFlowInfo_AbsLVal Cyc_CfFlowInfo_UnknownL();
# 90
typedef void*Cyc_CfFlowInfo_absRval_t;
typedef void*Cyc_CfFlowInfo_absRval_opt_t;
typedef struct Cyc_Dict_Dict Cyc_CfFlowInfo_flowdict_t;
typedef struct _dyneither_ptr Cyc_CfFlowInfo_aggrdict_t;struct Cyc_CfFlowInfo_UnionRInfo{int is_union;int fieldnum;};
# 98
typedef struct Cyc_CfFlowInfo_UnionRInfo Cyc_CfFlowInfo_union_rinfo_t;struct Cyc_CfFlowInfo_Zero_CfFlowInfo_AbsRVal_struct{int tag;};struct Cyc_CfFlowInfo_NotZeroAll_CfFlowInfo_AbsRVal_struct{int tag;};struct Cyc_CfFlowInfo_NotZeroThis_CfFlowInfo_AbsRVal_struct{int tag;};struct Cyc_CfFlowInfo_UnknownR_CfFlowInfo_AbsRVal_struct{int tag;enum Cyc_CfFlowInfo_InitLevel f1;};struct Cyc_CfFlowInfo_Esc_CfFlowInfo_AbsRVal_struct{int tag;enum Cyc_CfFlowInfo_InitLevel f1;};struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct{int tag;struct Cyc_CfFlowInfo_Place*f1;};struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct{int tag;struct Cyc_CfFlowInfo_UnionRInfo f1;struct _dyneither_ptr f2;};struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;void*f3;};struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct{int tag;struct Cyc_Absyn_Vardecl*f1;void*f2;};
# 119
typedef struct Cyc_Dict_Dict Cyc_CfFlowInfo_dict_set_t;
# 121
int Cyc_CfFlowInfo_update_place_set(struct Cyc_Dict_Dict*set,void*place,unsigned int loc);
# 126
typedef struct Cyc_Dict_Dict Cyc_CfFlowInfo_place_set_t;
struct Cyc_Dict_Dict Cyc_CfFlowInfo_union_place_set(struct Cyc_Dict_Dict s1,struct Cyc_Dict_Dict s2,int disjoint);struct _union_FlowInfo_BottomFL{int tag;int val;};struct _tuple14{struct Cyc_Dict_Dict f1;struct Cyc_List_List*f2;};struct _union_FlowInfo_ReachableFL{int tag;struct _tuple14 val;};union Cyc_CfFlowInfo_FlowInfo{struct _union_FlowInfo_BottomFL BottomFL;struct _union_FlowInfo_ReachableFL ReachableFL;};
# 142 "cf_flowinfo.h"
typedef union Cyc_CfFlowInfo_FlowInfo Cyc_CfFlowInfo_flow_t;
union Cyc_CfFlowInfo_FlowInfo Cyc_CfFlowInfo_BottomFL();
union Cyc_CfFlowInfo_FlowInfo Cyc_CfFlowInfo_ReachableFL(struct Cyc_Dict_Dict,struct Cyc_List_List*);struct Cyc_CfFlowInfo_FlowEnv{struct _RegionHandle*r;void*zero;void*notzeroall;void*notzerothis;void*unknown_none;void*unknown_this;void*unknown_all;void*esc_none;void*esc_this;void*esc_all;struct Cyc_Dict_Dict mt_flowdict;struct Cyc_Dict_Dict mt_place_set;struct Cyc_CfFlowInfo_Place*dummy_place;};
# 161
typedef struct Cyc_CfFlowInfo_FlowEnv*Cyc_CfFlowInfo_flow_env_t;
struct Cyc_CfFlowInfo_FlowEnv*Cyc_CfFlowInfo_new_flow_env(struct _RegionHandle*r);
# 164
int Cyc_CfFlowInfo_get_field_index(void*t,struct _dyneither_ptr*f);
int Cyc_CfFlowInfo_get_field_index_fs(struct Cyc_List_List*fs,struct _dyneither_ptr*f);
# 167
int Cyc_CfFlowInfo_root_cmp(void*,void*);
# 170
struct _dyneither_ptr Cyc_CfFlowInfo_aggrfields_to_aggrdict(struct Cyc_CfFlowInfo_FlowEnv*,struct Cyc_List_List*,int no_init_bits_only,void*);
void*Cyc_CfFlowInfo_typ_to_absrval(struct Cyc_CfFlowInfo_FlowEnv*,void*t,int no_init_bits_only,void*leafval);
void*Cyc_CfFlowInfo_make_unique_consumed(struct Cyc_CfFlowInfo_FlowEnv*fenv,void*t,struct Cyc_Absyn_Exp*consumer,int iteration,void*);
int Cyc_CfFlowInfo_is_unique_consumed(struct Cyc_Absyn_Exp*e,int env_iteration,void*r,int*needs_unconsume);
void*Cyc_CfFlowInfo_make_unique_unconsumed(struct Cyc_CfFlowInfo_FlowEnv*fenv,void*r);struct _tuple15{void*f1;struct Cyc_List_List*f2;};
struct _tuple15 Cyc_CfFlowInfo_unname_rval(struct _RegionHandle*rgn,void*rv);
# 178
enum Cyc_CfFlowInfo_InitLevel Cyc_CfFlowInfo_initlevel(struct Cyc_CfFlowInfo_FlowEnv*,struct Cyc_Dict_Dict d,void*r);
void*Cyc_CfFlowInfo_lookup_place(struct Cyc_Dict_Dict d,struct Cyc_CfFlowInfo_Place*place);
# 181
int Cyc_CfFlowInfo_flow_lessthan_approx(union Cyc_CfFlowInfo_FlowInfo f1,union Cyc_CfFlowInfo_FlowInfo f2);
# 183
void Cyc_CfFlowInfo_print_absrval(void*rval);
# 204 "cf_flowinfo.h"
struct Cyc_Dict_Dict Cyc_CfFlowInfo_escape_deref(struct Cyc_CfFlowInfo_FlowEnv*fenv,struct Cyc_Dict_Dict d,struct Cyc_Dict_Dict*all_changed,void*r);
# 208
struct Cyc_Dict_Dict Cyc_CfFlowInfo_assign_place(struct Cyc_CfFlowInfo_FlowEnv*fenv,unsigned int loc,struct Cyc_Dict_Dict d,struct Cyc_Dict_Dict*all_changed,struct Cyc_CfFlowInfo_Place*place,void*r);
# 213
union Cyc_CfFlowInfo_FlowInfo Cyc_CfFlowInfo_join_flow(struct Cyc_CfFlowInfo_FlowEnv*,struct Cyc_Dict_Dict*,union Cyc_CfFlowInfo_FlowInfo,union Cyc_CfFlowInfo_FlowInfo);struct _tuple16{union Cyc_CfFlowInfo_FlowInfo f1;void*f2;};
struct _tuple16 Cyc_CfFlowInfo_join_flow_and_rval(struct Cyc_CfFlowInfo_FlowEnv*,struct Cyc_Dict_Dict*all_changed,struct _tuple16 pr1,struct _tuple16 pr2);
# 219
union Cyc_CfFlowInfo_FlowInfo Cyc_CfFlowInfo_after_flow(struct Cyc_CfFlowInfo_FlowEnv*,struct Cyc_Dict_Dict*,union Cyc_CfFlowInfo_FlowInfo,union Cyc_CfFlowInfo_FlowInfo,struct Cyc_Dict_Dict,struct Cyc_Dict_Dict);
# 32 "new_control_flow.h"
void Cyc_NewControlFlow_cf_check(struct Cyc_JumpAnalysis_Jump_Anal_Result*tables,struct Cyc_List_List*ds);
# 37
extern int Cyc_NewControlFlow_warn_lose_unique;struct Cyc_Tcpat_TcPatResult{struct _tuple1*tvars_and_bounds_opt;struct Cyc_List_List*patvars;};
# 49 "tcpat.h"
typedef struct Cyc_Tcpat_TcPatResult Cyc_Tcpat_tcpat_result_t;struct Cyc_Tcpat_WhereTest_Tcpat_PatTest_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Tcpat_EqNull_Tcpat_PatTest_struct{int tag;};struct Cyc_Tcpat_NeqNull_Tcpat_PatTest_struct{int tag;};struct Cyc_Tcpat_EqEnum_Tcpat_PatTest_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Tcpat_EqAnonEnum_Tcpat_PatTest_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Tcpat_EqFloat_Tcpat_PatTest_struct{int tag;struct _dyneither_ptr f1;int f2;};struct Cyc_Tcpat_EqConst_Tcpat_PatTest_struct{int tag;unsigned int f1;};struct Cyc_Tcpat_EqDatatypeTag_Tcpat_PatTest_struct{int tag;int f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Tcpat_EqTaggedUnion_Tcpat_PatTest_struct{int tag;struct _dyneither_ptr*f1;int f2;};struct Cyc_Tcpat_EqExtensibleDatatype_Tcpat_PatTest_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;};
# 70
typedef void*Cyc_Tcpat_pat_test_t;struct Cyc_Tcpat_Dummy_Tcpat_Access_struct{int tag;};struct Cyc_Tcpat_Deref_Tcpat_Access_struct{int tag;};struct Cyc_Tcpat_TupleField_Tcpat_Access_struct{int tag;unsigned int f1;};struct Cyc_Tcpat_DatatypeField_Tcpat_Access_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;unsigned int f3;};struct Cyc_Tcpat_AggrField_Tcpat_Access_struct{int tag;int f1;struct _dyneither_ptr*f2;};
# 78
typedef void*Cyc_Tcpat_access_t;struct _union_PatOrWhere_pattern{int tag;struct Cyc_Absyn_Pat*val;};struct _union_PatOrWhere_where_clause{int tag;struct Cyc_Absyn_Exp*val;};union Cyc_Tcpat_PatOrWhere{struct _union_PatOrWhere_pattern pattern;struct _union_PatOrWhere_where_clause where_clause;};struct Cyc_Tcpat_PathNode{union Cyc_Tcpat_PatOrWhere orig_pat;void*access;};
# 88
typedef struct Cyc_Tcpat_PathNode*Cyc_Tcpat_path_node_t;
# 90
typedef void*Cyc_Tcpat_term_desc_t;
typedef struct Cyc_List_List*Cyc_Tcpat_path_t;struct Cyc_Tcpat_Rhs{int used;unsigned int pat_loc;struct Cyc_Absyn_Stmt*rhs;};
# 98
typedef struct Cyc_Tcpat_Rhs*Cyc_Tcpat_rhs_t;
# 100
typedef void*Cyc_Tcpat_decision_t;struct Cyc_Tcpat_Failure_Tcpat_Decision_struct{int tag;void*f1;};struct Cyc_Tcpat_Success_Tcpat_Decision_struct{int tag;struct Cyc_Tcpat_Rhs*f1;};struct Cyc_Tcpat_SwitchDec_Tcpat_Decision_struct{int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;void*f3;};
# 115
int Cyc_Tcpat_has_vars(struct Cyc_Core_Opt*pat_vars);struct Cyc_PP_Ppstate;
# 41 "pp.h"
typedef struct Cyc_PP_Ppstate*Cyc_PP_ppstate_t;struct Cyc_PP_Out;
# 43
typedef struct Cyc_PP_Out*Cyc_PP_out_t;struct Cyc_PP_Doc;
# 45
typedef struct Cyc_PP_Doc*Cyc_PP_doc_t;struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};
# 67 "absynpp.h"
struct _dyneither_ptr Cyc_Absynpp_exp2string(struct Cyc_Absyn_Exp*);
# 69
struct _dyneither_ptr Cyc_Absynpp_qvar2string(struct _tuple2*);
# 52 "new_control_flow.cyc"
typedef struct Cyc_Dict_Dict Cyc_NewControlFlow_dict_t;
# 55
int Cyc_NewControlFlow_warn_lose_unique=0;struct Cyc_NewControlFlow_CFStmtAnnot{int visited;};
# 62
typedef struct Cyc_NewControlFlow_CFStmtAnnot Cyc_NewControlFlow_cf_stmt_annot_t;static char Cyc_NewControlFlow_CFAnnot[8]="CFAnnot";struct Cyc_NewControlFlow_CFAnnot_Absyn_AbsynAnnot_struct{char*tag;struct Cyc_NewControlFlow_CFStmtAnnot f1;};struct Cyc_NewControlFlow_AnalEnv{struct Cyc_JumpAnalysis_Jump_Anal_Result*all_tables;struct Cyc_Hashtable_Table*succ_table;struct Cyc_Hashtable_Table*pat_pop_table;struct _RegionHandle*r;struct Cyc_CfFlowInfo_FlowEnv*fenv;int iterate_again;int iteration_num;int in_try;union Cyc_CfFlowInfo_FlowInfo tryflow;struct Cyc_Dict_Dict*all_changed;int noreturn;void*return_type;struct Cyc_List_List*unique_pat_vars;struct Cyc_List_List*param_roots;struct Cyc_List_List*noconsume_params;struct Cyc_Hashtable_Table*flow_table;struct Cyc_List_List*return_relns;};
# 97 "new_control_flow.cyc"
typedef struct Cyc_NewControlFlow_AnalEnv*Cyc_NewControlFlow_analenv_t;struct _tuple17{void*f1;int f2;};
# 105
static union Cyc_CfFlowInfo_FlowInfo Cyc_NewControlFlow_anal_stmt(struct Cyc_NewControlFlow_AnalEnv*,union Cyc_CfFlowInfo_FlowInfo,struct Cyc_Absyn_Stmt*,struct _tuple17*);
# 107
static union Cyc_CfFlowInfo_FlowInfo Cyc_NewControlFlow_anal_decl(struct Cyc_NewControlFlow_AnalEnv*,union Cyc_CfFlowInfo_FlowInfo,struct Cyc_Absyn_Decl*);struct _tuple18{union Cyc_CfFlowInfo_FlowInfo f1;union Cyc_CfFlowInfo_AbsLVal f2;};
static struct _tuple18 Cyc_NewControlFlow_anal_Lexp(struct Cyc_NewControlFlow_AnalEnv*,union Cyc_CfFlowInfo_FlowInfo,int expand_unique,int passthrough_consumes,struct Cyc_Absyn_Exp*);
static struct _tuple16 Cyc_NewControlFlow_anal_Rexp(struct Cyc_NewControlFlow_AnalEnv*,int copy_ctxt,union Cyc_CfFlowInfo_FlowInfo,struct Cyc_Absyn_Exp*);struct _tuple19{union Cyc_CfFlowInfo_FlowInfo f1;union Cyc_CfFlowInfo_FlowInfo f2;};
static struct _tuple19 Cyc_NewControlFlow_anal_test(struct Cyc_NewControlFlow_AnalEnv*,union Cyc_CfFlowInfo_FlowInfo,struct Cyc_Absyn_Exp*);
static struct Cyc_List_List*Cyc_NewControlFlow_noalias_ptrs_rec(struct Cyc_NewControlFlow_AnalEnv*env,struct Cyc_CfFlowInfo_Place*p,void*t);
# 114
static union Cyc_CfFlowInfo_FlowInfo Cyc_NewControlFlow_expand_unique_places(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_List_List*es);
# 119
static struct Cyc_NewControlFlow_CFStmtAnnot*Cyc_NewControlFlow_get_stmt_annot(struct Cyc_Absyn_Stmt*s){
void*_tmp0=s->annot;void*_tmp1=_tmp0;struct Cyc_NewControlFlow_CFStmtAnnot*_tmp5;if(((struct Cyc_NewControlFlow_CFAnnot_Absyn_AbsynAnnot_struct*)_tmp1)->tag == Cyc_NewControlFlow_CFAnnot){_LL1: _tmp5=(struct Cyc_NewControlFlow_CFStmtAnnot*)&((struct Cyc_NewControlFlow_CFAnnot_Absyn_AbsynAnnot_struct*)_tmp1)->f1;_LL2:
 return _tmp5;}else{_LL3: _LL4:
 s->annot=(void*)({struct Cyc_NewControlFlow_CFAnnot_Absyn_AbsynAnnot_struct*_tmp2=_cycalloc_atomic(sizeof(*_tmp2));_tmp2[0]=({struct Cyc_NewControlFlow_CFAnnot_Absyn_AbsynAnnot_struct _tmp3;_tmp3.tag=Cyc_NewControlFlow_CFAnnot;_tmp3.f1=({struct Cyc_NewControlFlow_CFStmtAnnot _tmp4;_tmp4.visited=0;_tmp4;});_tmp3;});_tmp2;});return Cyc_NewControlFlow_get_stmt_annot(s);}_LL0:;}
# 126
static union Cyc_CfFlowInfo_FlowInfo*Cyc_NewControlFlow_get_stmt_flow(struct Cyc_NewControlFlow_AnalEnv*env,struct Cyc_Absyn_Stmt*s){
union Cyc_CfFlowInfo_FlowInfo**sflow=((union Cyc_CfFlowInfo_FlowInfo**(*)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Stmt*key))Cyc_Hashtable_lookup_opt)(env->flow_table,s);
if(sflow == 0){
union Cyc_CfFlowInfo_FlowInfo*res=({union Cyc_CfFlowInfo_FlowInfo*_tmp6=_region_malloc(env->r,sizeof(*_tmp6));_tmp6[0]=Cyc_CfFlowInfo_BottomFL();_tmp6;});
((void(*)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Stmt*key,union Cyc_CfFlowInfo_FlowInfo*val))Cyc_Hashtable_insert)(env->flow_table,s,res);
return res;}
# 133
return*sflow;}struct _tuple20{struct Cyc_NewControlFlow_CFStmtAnnot*f1;union Cyc_CfFlowInfo_FlowInfo*f2;};
# 136
static struct _tuple20 Cyc_NewControlFlow_pre_stmt_check(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_Absyn_Stmt*s){
struct Cyc_NewControlFlow_CFStmtAnnot*_tmp7=Cyc_NewControlFlow_get_stmt_annot(s);
union Cyc_CfFlowInfo_FlowInfo*_tmp8=Cyc_NewControlFlow_get_stmt_flow(env,s);
# 144
*_tmp8=Cyc_CfFlowInfo_join_flow(env->fenv,env->all_changed,inflow,*_tmp8);
# 150
_tmp7->visited=env->iteration_num;
return({struct _tuple20 _tmp9;_tmp9.f1=_tmp7;_tmp9.f2=_tmp8;_tmp9;});}
# 160
static void Cyc_NewControlFlow_update_tryflow(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo new_flow){
if(env->in_try)
# 168
env->tryflow=Cyc_CfFlowInfo_join_flow(env->fenv,env->all_changed,new_flow,env->tryflow);}struct _tuple21{struct Cyc_NewControlFlow_AnalEnv*f1;unsigned int f2;struct Cyc_Dict_Dict f3;};
# 175
static void Cyc_NewControlFlow_check_unique_root(struct _tuple21*ckenv,void*root,void*rval){
# 177
struct _tuple21*_tmpA=ckenv;struct Cyc_NewControlFlow_AnalEnv*_tmp14;unsigned int _tmp13;struct Cyc_Dict_Dict _tmp12;_LL6: _tmp14=_tmpA->f1;_tmp13=_tmpA->f2;_tmp12=_tmpA->f3;_LL7:;{
void*_tmpB=root;struct Cyc_Absyn_Vardecl*_tmp11;if(((struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*)_tmpB)->tag == 0){_LL9: _tmp11=((struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*)_tmpB)->f1;_LLA:
# 180
 if(!((int(*)(struct Cyc_Dict_Dict d,void*k,void**ans))Cyc_Dict_lookup_bool)(_tmp12,root,& rval) && 
Cyc_Tcutil_is_noalias_pointer_or_aggr((_tmp14->fenv)->r,_tmp11->type)){
retry: {void*_tmpC=rval;void*_tmp10;switch(*((int*)_tmpC)){case 8: _LLE: _tmp10=(void*)((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmpC)->f2;_LLF:
 rval=_tmp10;goto retry;case 7: _LL10: _LL11:
 goto _LL13;case 3: if(((struct Cyc_CfFlowInfo_UnknownR_CfFlowInfo_AbsRVal_struct*)_tmpC)->f1 == Cyc_CfFlowInfo_NoneIL){_LL12: _LL13:
 goto _LL15;}else{goto _LL16;}case 0: _LL14: _LL15:
 goto _LLD;default: _LL16: _LL17:
# 189
({struct Cyc_String_pa_PrintArg_struct _tmpF;_tmpF.tag=0;_tmpF.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_qvar2string(_tmp11->name));({void*_tmpD[1]={& _tmpF};Cyc_Tcutil_warn(_tmp13,({const char*_tmpE="alias-free pointer(s) reachable from %s may become unreachable.";_tag_dyneither(_tmpE,sizeof(char),64);}),_tag_dyneither(_tmpD,sizeof(void*),1));});});
goto _LLD;}_LLD:;}}
# 193
goto _LL8;}else{_LLB: _LLC:
 goto _LL8;}_LL8:;};}
# 201
static void Cyc_NewControlFlow_update_flow(struct Cyc_NewControlFlow_AnalEnv*env,struct Cyc_Absyn_Stmt*s,union Cyc_CfFlowInfo_FlowInfo flow){
struct Cyc_NewControlFlow_CFStmtAnnot*_tmp15=Cyc_NewControlFlow_get_stmt_annot(s);
union Cyc_CfFlowInfo_FlowInfo*_tmp16=Cyc_NewControlFlow_get_stmt_flow(env,s);
union Cyc_CfFlowInfo_FlowInfo _tmp17=Cyc_CfFlowInfo_join_flow(env->fenv,env->all_changed,flow,*_tmp16);
# 207
if(Cyc_NewControlFlow_warn_lose_unique){
struct _tuple19 _tmp18=({struct _tuple19 _tmp1E;_tmp1E.f1=flow;_tmp1E.f2=_tmp17;_tmp1E;});struct _tuple19 _tmp19=_tmp18;struct Cyc_Dict_Dict _tmp1D;struct Cyc_Dict_Dict _tmp1C;if(((_tmp19.f1).ReachableFL).tag == 2){if(((_tmp19.f2).ReachableFL).tag == 2){_LL19: _tmp1D=(((_tmp19.f1).ReachableFL).val).f1;_tmp1C=(((_tmp19.f2).ReachableFL).val).f1;_LL1A: {
# 210
struct _tuple21 _tmp1A=({struct _tuple21 _tmp1B;_tmp1B.f1=env;_tmp1B.f2=s->loc;_tmp1B.f3=_tmp1C;_tmp1B;});
((void(*)(void(*f)(struct _tuple21*,void*,void*),struct _tuple21*env,struct Cyc_Dict_Dict d))Cyc_Dict_iter_c)(Cyc_NewControlFlow_check_unique_root,& _tmp1A,_tmp1D);
goto _LL18;}}else{goto _LL1B;}}else{_LL1B: _LL1C:
 goto _LL18;}_LL18:;}
# 216
if(!Cyc_CfFlowInfo_flow_lessthan_approx(_tmp17,*_tmp16)){
# 223
*_tmp16=_tmp17;
# 227
if(_tmp15->visited == env->iteration_num)
# 229
env->iterate_again=1;}}
# 234
static union Cyc_CfFlowInfo_FlowInfo Cyc_NewControlFlow_add_vars(struct Cyc_CfFlowInfo_FlowEnv*fenv,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_List_List*vds,void*leafval,unsigned int loc,int nameit){
# 238
union Cyc_CfFlowInfo_FlowInfo _tmp1F=inflow;struct Cyc_Dict_Dict _tmp27;struct Cyc_List_List*_tmp26;if((_tmp1F.BottomFL).tag == 1){_LL1E: _LL1F:
 return Cyc_CfFlowInfo_BottomFL();}else{_LL20: _tmp27=((_tmp1F.ReachableFL).val).f1;_tmp26=((_tmp1F.ReachableFL).val).f2;_LL21:
# 241
 for(0;vds != 0;vds=vds->tl){
struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*_tmp20=({struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*_tmp24=_region_malloc(fenv->r,sizeof(*_tmp24));_tmp24[0]=({struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct _tmp25;_tmp25.tag=0;_tmp25.f1=(struct Cyc_Absyn_Vardecl*)vds->hd;_tmp25;});_tmp24;});
void*_tmp21=Cyc_CfFlowInfo_typ_to_absrval(fenv,((struct Cyc_Absyn_Vardecl*)vds->hd)->type,0,leafval);
if(nameit)
_tmp21=(void*)({struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*_tmp22=_region_malloc(fenv->r,sizeof(*_tmp22));_tmp22[0]=({struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct _tmp23;_tmp23.tag=8;_tmp23.f1=(struct Cyc_Absyn_Vardecl*)vds->hd;_tmp23.f2=_tmp21;_tmp23;});_tmp22;});
# 248
_tmp27=((struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,void*k,void*v))Cyc_Dict_insert)(_tmp27,(void*)_tmp20,_tmp21);}
# 250
return Cyc_CfFlowInfo_ReachableFL(_tmp27,_tmp26);}_LL1D:;}
# 254
static int Cyc_NewControlFlow_relns_ok(struct _RegionHandle*r,struct Cyc_List_List*assume,struct Cyc_List_List*req){
# 261
for(0;(unsigned int)req;req=req->tl){
struct Cyc_Relations_Reln*_tmp28=Cyc_Relations_negate(r,(struct Cyc_Relations_Reln*)req->hd);
if(Cyc_Relations_consistent_relations(({struct Cyc_List_List*_tmp29=_region_malloc(r,sizeof(*_tmp29));_tmp29->hd=_tmp28;_tmp29->tl=assume;_tmp29;})))
return 0;}
# 266
return 1;}
# 269
static struct Cyc_Absyn_Exp*Cyc_NewControlFlow_strip_cast(struct Cyc_Absyn_Exp*e){
void*_tmp2A=e->r;void*_tmp2B=_tmp2A;struct Cyc_Absyn_Exp*_tmp2C;if(((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp2B)->tag == 13){_LL23: _tmp2C=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp2B)->f2;_LL24:
 return _tmp2C;}else{_LL25: _LL26:
 return e;}_LL22:;}
# 276
static void Cyc_NewControlFlow_check_union_requires(unsigned int loc,struct _RegionHandle*r,void*t,struct _dyneither_ptr*f,union Cyc_CfFlowInfo_FlowInfo inflow){
# 278
union Cyc_CfFlowInfo_FlowInfo _tmp2D=inflow;struct Cyc_List_List*_tmp4B;if((_tmp2D.BottomFL).tag == 1){_LL28: _LL29:
 return;}else{_LL2A: _tmp4B=((_tmp2D.ReachableFL).val).f2;_LL2B:
# 281
{void*_tmp2E=Cyc_Tcutil_compress(t);void*_tmp2F=_tmp2E;struct Cyc_List_List*_tmp4A;union Cyc_Absyn_AggrInfoU _tmp49;struct Cyc_List_List*_tmp48;switch(*((int*)_tmp2F)){case 11: _LL2D: _tmp49=(((struct Cyc_Absyn_AggrType_Absyn_Type_struct*)_tmp2F)->f1).aggr_info;_tmp48=(((struct Cyc_Absyn_AggrType_Absyn_Type_struct*)_tmp2F)->f1).targs;_LL2E: {
# 283
struct Cyc_Absyn_Aggrdecl*_tmp30=Cyc_Absyn_get_known_aggrdecl(_tmp49);
struct Cyc_Absyn_Aggrfield*_tmp31=Cyc_Absyn_lookup_decl_field(_tmp30,f);
struct Cyc_Absyn_Exp*_tmp32=((struct Cyc_Absyn_Aggrfield*)_check_null(_tmp31))->requires_clause;
if(_tmp32 != 0){
struct _RegionHandle _tmp33=_new_region("temp");struct _RegionHandle*temp=& _tmp33;_push_region(temp);
{struct Cyc_Absyn_Exp*_tmp34=Cyc_Tcutil_rsubsexp(temp,((struct Cyc_List_List*(*)(struct _RegionHandle*r1,struct _RegionHandle*r2,struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_rzip)(temp,temp,_tmp30->tvs,_tmp48),_tmp32);
# 290
if(!Cyc_NewControlFlow_relns_ok(r,_tmp4B,Cyc_Relations_exp2relns(temp,_tmp34))){
({struct Cyc_String_pa_PrintArg_struct _tmp38;_tmp38.tag=0;_tmp38.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*f);({struct Cyc_String_pa_PrintArg_struct _tmp37;_tmp37.tag=0;_tmp37.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_exp2string(Cyc_NewControlFlow_strip_cast(_tmp34)));({void*_tmp35[2]={& _tmp37,& _tmp38};Cyc_CfFlowInfo_aerr(loc,({const char*_tmp36="unable to prove %s, required to access %s";_tag_dyneither(_tmp36,sizeof(char),42);}),_tag_dyneither(_tmp35,sizeof(void*),2));});});});
({void*_tmp39=0;Cyc_fprintf(Cyc_stderr,({const char*_tmp3A="  [recorded facts on path: ";_tag_dyneither(_tmp3A,sizeof(char),28);}),_tag_dyneither(_tmp39,sizeof(void*),0));});
Cyc_Relations_print_relns(Cyc_stderr,_tmp4B);
({void*_tmp3B=0;Cyc_fprintf(Cyc_stderr,({const char*_tmp3C="]\n";_tag_dyneither(_tmp3C,sizeof(char),3);}),_tag_dyneither(_tmp3B,sizeof(void*),0));});}}
# 288
;_pop_region(temp);}
# 297
goto _LL2C;}case 12: _LL2F: _tmp4A=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp2F)->f2;_LL30: {
# 299
struct Cyc_Absyn_Aggrfield*_tmp3D=Cyc_Absyn_lookup_field(_tmp4A,f);
struct Cyc_Absyn_Exp*_tmp3E=((struct Cyc_Absyn_Aggrfield*)_check_null(_tmp3D))->requires_clause;
if(_tmp3E != 0){
struct _RegionHandle _tmp3F=_new_region("temp");struct _RegionHandle*temp=& _tmp3F;_push_region(temp);
if(!Cyc_NewControlFlow_relns_ok(r,_tmp4B,Cyc_Relations_exp2relns(temp,_tmp3E))){
({struct Cyc_String_pa_PrintArg_struct _tmp43;_tmp43.tag=0;_tmp43.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*f);({struct Cyc_String_pa_PrintArg_struct _tmp42;_tmp42.tag=0;_tmp42.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_exp2string(Cyc_NewControlFlow_strip_cast(_tmp3E)));({void*_tmp40[2]={& _tmp42,& _tmp43};Cyc_CfFlowInfo_aerr(loc,({const char*_tmp41="unable to prove %s, required to access %s";_tag_dyneither(_tmp41,sizeof(char),42);}),_tag_dyneither(_tmp40,sizeof(void*),2));});});});
({void*_tmp44=0;Cyc_fprintf(Cyc_stderr,({const char*_tmp45="  [recorded facts on path: ";_tag_dyneither(_tmp45,sizeof(char),28);}),_tag_dyneither(_tmp44,sizeof(void*),0));});
Cyc_Relations_print_relns(Cyc_stderr,_tmp4B);
({void*_tmp46=0;Cyc_fprintf(Cyc_stderr,({const char*_tmp47="]\n";_tag_dyneither(_tmp47,sizeof(char),3);}),_tag_dyneither(_tmp46,sizeof(void*),0));});}
# 303
;_pop_region(temp);}
# 310
goto _LL2C;}default: _LL31: _LL32:
 goto _LL2C;}_LL2C:;}
# 313
goto _LL27;}_LL27:;}
# 317
static union Cyc_CfFlowInfo_FlowInfo Cyc_NewControlFlow_use_Rval(struct Cyc_NewControlFlow_AnalEnv*env,unsigned int loc,union Cyc_CfFlowInfo_FlowInfo inflow,void*r){
union Cyc_CfFlowInfo_FlowInfo _tmp4C=inflow;struct Cyc_Dict_Dict _tmp52;struct Cyc_List_List*_tmp51;if((_tmp4C.BottomFL).tag == 1){_LL34: _LL35:
 return Cyc_CfFlowInfo_BottomFL();}else{_LL36: _tmp52=((_tmp4C.ReachableFL).val).f1;_tmp51=((_tmp4C.ReachableFL).val).f2;_LL37:
# 321
 if(Cyc_CfFlowInfo_initlevel(env->fenv,_tmp52,r)!= Cyc_CfFlowInfo_AllIL)
({void*_tmp4D=0;Cyc_CfFlowInfo_aerr(loc,({const char*_tmp4E="expression may not be fully initialized";_tag_dyneither(_tmp4E,sizeof(char),40);}),_tag_dyneither(_tmp4D,sizeof(void*),0));});{
struct Cyc_Dict_Dict _tmp4F=Cyc_CfFlowInfo_escape_deref(env->fenv,_tmp52,env->all_changed,r);
if(_tmp52.t == _tmp4F.t)return inflow;{
union Cyc_CfFlowInfo_FlowInfo _tmp50=Cyc_CfFlowInfo_ReachableFL(_tmp4F,_tmp51);
Cyc_NewControlFlow_update_tryflow(env,_tmp50);
return _tmp50;};};}_LL33:;}struct _tuple22{struct Cyc_Absyn_Tqual f1;void*f2;};
# 331
static void Cyc_NewControlFlow_check_nounique(struct Cyc_NewControlFlow_AnalEnv*env,unsigned int loc,void*t,void*r){
struct _tuple0 _tmp53=({struct _tuple0 _tmp66;_tmp66.f1=Cyc_Tcutil_compress(t);_tmp66.f2=r;_tmp66;});struct _tuple0 _tmp54=_tmp53;enum Cyc_Absyn_AggrKind _tmp65;struct Cyc_List_List*_tmp64;struct _dyneither_ptr _tmp63;union Cyc_Absyn_AggrInfoU _tmp62;struct Cyc_List_List*_tmp61;struct _dyneither_ptr _tmp60;struct Cyc_List_List*_tmp5F;struct _dyneither_ptr _tmp5E;struct Cyc_Absyn_Datatypefield*_tmp5D;struct _dyneither_ptr _tmp5C;void*_tmp5B;switch(*((int*)_tmp54.f2)){case 3: if(((struct Cyc_CfFlowInfo_UnknownR_CfFlowInfo_AbsRVal_struct*)_tmp54.f2)->f1 == Cyc_CfFlowInfo_NoneIL){_LL39: _LL3A:
 return;}else{switch(*((int*)_tmp54.f1)){case 4: if((((((struct Cyc_Absyn_DatatypeFieldType_Absyn_Type_struct*)_tmp54.f1)->f1).field_info).KnownDatatypefield).tag == 2)goto _LL4B;else{goto _LL4B;}case 10: goto _LL4B;case 11: goto _LL4B;case 12: goto _LL4B;case 5: goto _LL49;default: goto _LL4B;}}case 0: _LL3B: _LL3C:
 return;case 7: _LL3D: _LL3E:
 return;case 8: _LL3F: _tmp5B=(void*)((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp54.f2)->f2;_LL40:
 Cyc_NewControlFlow_check_nounique(env,loc,t,_tmp5B);return;default: switch(*((int*)_tmp54.f1)){case 4: if((((((struct Cyc_Absyn_DatatypeFieldType_Absyn_Type_struct*)_tmp54.f1)->f1).field_info).KnownDatatypefield).tag == 2){if(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp54.f2)->tag == 6){_LL41: _tmp5D=((((((struct Cyc_Absyn_DatatypeFieldType_Absyn_Type_struct*)_tmp54.f1)->f1).field_info).KnownDatatypefield).val).f2;_tmp5C=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp54.f2)->f2;_LL42:
# 338
 if(_tmp5D->typs == 0)
return;
_tmp5F=_tmp5D->typs;_tmp5E=_tmp5C;goto _LL44;}else{goto _LL4B;}}else{goto _LL4B;}case 10: if(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp54.f2)->tag == 6){_LL43: _tmp5F=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp54.f1)->f1;_tmp5E=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp54.f2)->f2;_LL44: {
# 342
unsigned int sz=(unsigned int)((int(*)(struct Cyc_List_List*x))Cyc_List_length)(_tmp5F);
{int i=0;for(0;i < sz;(i ++,_tmp5F=_tmp5F->tl)){
Cyc_NewControlFlow_check_nounique(env,loc,(*((struct _tuple22*)((struct Cyc_List_List*)_check_null(_tmp5F))->hd)).f2,*((void**)_check_dyneither_subscript(_tmp5E,sizeof(void*),i)));}}
# 346
return;}}else{goto _LL4B;}case 11: if(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp54.f2)->tag == 6){_LL45: _tmp62=(((struct Cyc_Absyn_AggrType_Absyn_Type_struct*)_tmp54.f1)->f1).aggr_info;_tmp61=(((struct Cyc_Absyn_AggrType_Absyn_Type_struct*)_tmp54.f1)->f1).targs;_tmp60=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp54.f2)->f2;_LL46: {
# 349
struct Cyc_Absyn_Aggrdecl*_tmp55=Cyc_Absyn_get_known_aggrdecl(_tmp62);
if(_tmp55->impl == 0)return;{
struct Cyc_List_List*_tmp56=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(_tmp55->impl))->fields;
struct _RegionHandle _tmp57=_new_region("temp");struct _RegionHandle*temp=& _tmp57;_push_region(temp);
{struct Cyc_List_List*_tmp58=((struct Cyc_List_List*(*)(struct _RegionHandle*r1,struct _RegionHandle*r2,struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_rzip)(temp,temp,_tmp55->tvs,_tmp61);
{int i=0;for(0;i < _get_dyneither_size(_tmp60,sizeof(void*));(i ++,_tmp56=_tmp56->tl)){
void*t=((struct Cyc_Absyn_Aggrfield*)((struct Cyc_List_List*)_check_null(_tmp56))->hd)->type;
if(_tmp58 != 0)t=Cyc_Tcutil_rsubstitute(temp,_tmp58,t);
Cyc_NewControlFlow_check_nounique(env,loc,t,((void**)_tmp60.curr)[i]);}}
# 359
_npop_handler(0);return;}
# 353
;_pop_region(temp);};}}else{goto _LL4B;}case 12: if(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp54.f2)->tag == 6){_LL47: _tmp65=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp54.f1)->f1;_tmp64=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp54.f1)->f2;_tmp63=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp54.f2)->f2;_LL48:
# 362
{int i=0;for(0;i < _get_dyneither_size(_tmp63,sizeof(void*));(i ++,_tmp64=_tmp64->tl)){
Cyc_NewControlFlow_check_nounique(env,loc,((struct Cyc_Absyn_Aggrfield*)((struct Cyc_List_List*)_check_null(_tmp64))->hd)->type,((void**)_tmp63.curr)[i]);}}
return;}else{goto _LL4B;}case 5: _LL49: if(
Cyc_Tcutil_is_noalias_pointer(t,0)){_LL4A:
({void*_tmp59=0;Cyc_Tcutil_warn(loc,({const char*_tmp5A="argument may still contain alias-free pointers";_tag_dyneither(_tmp5A,sizeof(char),47);}),_tag_dyneither(_tmp59,sizeof(void*),0));});
return;}else{goto _LL4B;}default: _LL4B: _LL4C:
 return;}}_LL38:;}
# 372
static union Cyc_CfFlowInfo_FlowInfo Cyc_NewControlFlow_use_nounique_Rval(struct Cyc_NewControlFlow_AnalEnv*env,unsigned int loc,void*t,union Cyc_CfFlowInfo_FlowInfo inflow,void*r){
union Cyc_CfFlowInfo_FlowInfo _tmp67=inflow;struct Cyc_Dict_Dict _tmp73;struct Cyc_List_List*_tmp72;if((_tmp67.BottomFL).tag == 1){_LL4E: _LL4F:
 return Cyc_CfFlowInfo_BottomFL();}else{_LL50: _tmp73=((_tmp67.ReachableFL).val).f1;_tmp72=((_tmp67.ReachableFL).val).f2;_LL51:
# 376
 if(!Cyc_Tcutil_is_noalias_pointer(t,0)){
({void*_tmp68=0;((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(({const char*_tmp69="noliveunique attribute requires alias-free pointer";_tag_dyneither(_tmp69,sizeof(char),51);}),_tag_dyneither(_tmp68,sizeof(void*),0));});
return Cyc_CfFlowInfo_BottomFL();}{
# 380
void*_tmp6A=Cyc_Tcutil_pointer_elt_type(t);
retry: {void*_tmp6B=r;struct Cyc_CfFlowInfo_Place*_tmp6F;void*_tmp6E;switch(*((int*)_tmp6B)){case 8: _LL53: _tmp6E=(void*)((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp6B)->f2;_LL54:
 r=_tmp6E;goto retry;case 5: _LL55: _tmp6F=((struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*)_tmp6B)->f1;_LL56:
# 384
 Cyc_NewControlFlow_check_nounique(env,loc,_tmp6A,Cyc_CfFlowInfo_lookup_place(_tmp73,_tmp6F));
goto _LL52;default: _LL57: _LL58:
# 387
 if(Cyc_Tcutil_is_noalias_pointer_or_aggr((env->fenv)->r,_tmp6A))
({void*_tmp6C=0;Cyc_Tcutil_warn(loc,({const char*_tmp6D="argument may contain live alias-free pointers";_tag_dyneither(_tmp6D,sizeof(char),46);}),_tag_dyneither(_tmp6C,sizeof(void*),0));});
return Cyc_NewControlFlow_use_Rval(env,loc,inflow,r);}_LL52:;}{
# 391
struct Cyc_Dict_Dict _tmp70=Cyc_CfFlowInfo_escape_deref(env->fenv,_tmp73,env->all_changed,r);
if(_tmp73.t == _tmp70.t)return inflow;{
union Cyc_CfFlowInfo_FlowInfo _tmp71=Cyc_CfFlowInfo_ReachableFL(_tmp70,_tmp72);
Cyc_NewControlFlow_update_tryflow(env,_tmp71);
return _tmp71;};};};}_LL4D:;}struct _tuple23{union Cyc_CfFlowInfo_FlowInfo f1;struct Cyc_List_List*f2;};
# 400
static struct _tuple23 Cyc_NewControlFlow_anal_unordered_Rexps(struct _RegionHandle*rgn,struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_List_List*es,int first_is_copy,int others_are_copy){
# 407
if(es == 0)
return({struct _tuple23 _tmp74;_tmp74.f1=inflow;_tmp74.f2=0;_tmp74;});
if(es->tl == 0){
struct _tuple16 _tmp75=Cyc_NewControlFlow_anal_Rexp(env,first_is_copy,inflow,(struct Cyc_Absyn_Exp*)es->hd);struct _tuple16 _tmp76=_tmp75;union Cyc_CfFlowInfo_FlowInfo _tmp7A;void*_tmp79;_LL5A: _tmp7A=_tmp76.f1;_tmp79=_tmp76.f2;_LL5B:;
return({struct _tuple23 _tmp77;_tmp77.f1=_tmp7A;_tmp77.f2=({struct Cyc_List_List*_tmp78=_region_malloc(rgn,sizeof(*_tmp78));_tmp78->hd=_tmp79;_tmp78->tl=0;_tmp78;});_tmp77;});}{
# 413
struct Cyc_Dict_Dict*outer_all_changed=env->all_changed;
struct Cyc_Dict_Dict this_all_changed;
union Cyc_CfFlowInfo_FlowInfo old_inflow;
union Cyc_CfFlowInfo_FlowInfo outflow;
struct Cyc_List_List*rvals;
# 420
inflow=Cyc_NewControlFlow_expand_unique_places(env,inflow,es);
do{
this_all_changed=(env->fenv)->mt_place_set;
# 424
env->all_changed=({struct Cyc_Dict_Dict*_tmp7B=_region_malloc(env->r,sizeof(*_tmp7B));_tmp7B[0]=(env->fenv)->mt_place_set;_tmp7B;});{
struct _tuple16 _tmp7C=Cyc_NewControlFlow_anal_Rexp(env,first_is_copy,inflow,(struct Cyc_Absyn_Exp*)es->hd);struct _tuple16 _tmp7D=_tmp7C;union Cyc_CfFlowInfo_FlowInfo _tmp86;void*_tmp85;_LL5D: _tmp86=_tmp7D.f1;_tmp85=_tmp7D.f2;_LL5E:;
outflow=_tmp86;
rvals=({struct Cyc_List_List*_tmp7E=_region_malloc(rgn,sizeof(*_tmp7E));_tmp7E->hd=_tmp85;_tmp7E->tl=0;_tmp7E;});
this_all_changed=Cyc_CfFlowInfo_union_place_set(this_all_changed,*((struct Cyc_Dict_Dict*)_check_null(env->all_changed)),0);
# 430
{struct Cyc_List_List*es2=es->tl;for(0;es2 != 0;es2=es2->tl){
env->all_changed=({struct Cyc_Dict_Dict*_tmp7F=_region_malloc(env->r,sizeof(*_tmp7F));_tmp7F[0]=(env->fenv)->mt_place_set;_tmp7F;});{
struct _tuple16 _tmp80=Cyc_NewControlFlow_anal_Rexp(env,others_are_copy,inflow,(struct Cyc_Absyn_Exp*)es2->hd);struct _tuple16 _tmp81=_tmp80;union Cyc_CfFlowInfo_FlowInfo _tmp84;void*_tmp83;_LL60: _tmp84=_tmp81.f1;_tmp83=_tmp81.f2;_LL61:;
rvals=({struct Cyc_List_List*_tmp82=_region_malloc(rgn,sizeof(*_tmp82));_tmp82->hd=_tmp83;_tmp82->tl=rvals;_tmp82;});
outflow=Cyc_CfFlowInfo_after_flow(env->fenv,& this_all_changed,outflow,_tmp84,this_all_changed,*((struct Cyc_Dict_Dict*)_check_null(env->all_changed)));
# 436
this_all_changed=Cyc_CfFlowInfo_union_place_set(this_all_changed,*((struct Cyc_Dict_Dict*)_check_null(env->all_changed)),0);};}}
# 439
old_inflow=inflow;
# 442
inflow=Cyc_CfFlowInfo_join_flow(env->fenv,outer_all_changed,inflow,outflow);};}while(!
# 447
Cyc_CfFlowInfo_flow_lessthan_approx(inflow,old_inflow));
if(outer_all_changed == 0)
env->all_changed=0;else{
# 451
env->all_changed=({struct Cyc_Dict_Dict*_tmp87=_region_malloc(env->r,sizeof(*_tmp87));_tmp87[0]=Cyc_CfFlowInfo_union_place_set(*outer_all_changed,this_all_changed,0);_tmp87;});}
# 453
Cyc_NewControlFlow_update_tryflow(env,outflow);
# 455
return({struct _tuple23 _tmp88;_tmp88.f1=outflow;_tmp88.f2=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(rvals);_tmp88;});};}
# 460
static struct _tuple16 Cyc_NewControlFlow_anal_use_ints(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_List_List*es){
# 463
struct _RegionHandle _tmp89=_new_region("rgn");struct _RegionHandle*rgn=& _tmp89;_push_region(rgn);
{struct _tuple23 _tmp8A=
Cyc_NewControlFlow_anal_unordered_Rexps(rgn,env,inflow,es,0,0);
# 464
struct _tuple23 _tmp8B=_tmp8A;union Cyc_CfFlowInfo_FlowInfo _tmp93;struct Cyc_List_List*_tmp92;_LL63: _tmp93=_tmp8B.f1;_tmp92=_tmp8B.f2;_LL64:;
# 466
{union Cyc_CfFlowInfo_FlowInfo _tmp8C=_tmp93;struct Cyc_Dict_Dict _tmp8F;if((_tmp8C.ReachableFL).tag == 2){_LL66: _tmp8F=((_tmp8C.ReachableFL).val).f1;_LL67:
# 468
 for(0;_tmp92 != 0;(_tmp92=_tmp92->tl,es=((struct Cyc_List_List*)_check_null(es))->tl)){
if(Cyc_CfFlowInfo_initlevel(env->fenv,_tmp8F,(void*)_tmp92->hd)== Cyc_CfFlowInfo_NoneIL)
({void*_tmp8D=0;Cyc_CfFlowInfo_aerr(((struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es))->hd)->loc,({const char*_tmp8E="expression may not be initialized";_tag_dyneither(_tmp8E,sizeof(char),34);}),_tag_dyneither(_tmp8D,sizeof(void*),0));});}
goto _LL65;}else{_LL68: _LL69:
 goto _LL65;}_LL65:;}{
# 474
struct _tuple16 _tmp91=({struct _tuple16 _tmp90;_tmp90.f1=_tmp93;_tmp90.f2=(env->fenv)->unknown_all;_tmp90;});_npop_handler(0);return _tmp91;};}
# 464
;_pop_region(rgn);}
# 482
static void*Cyc_NewControlFlow_consume_zero_rval(struct Cyc_NewControlFlow_AnalEnv*env,struct Cyc_Dict_Dict new_dict,struct Cyc_CfFlowInfo_Place*p,struct Cyc_Absyn_Exp*e,void*new_rval){
# 489
int needs_unconsume=0;
void*_tmp94=Cyc_CfFlowInfo_lookup_place(new_dict,p);
if(Cyc_CfFlowInfo_is_unique_consumed(e,env->iteration_num,_tmp94,& needs_unconsume))
(int)_throw((void*)({struct Cyc_Core_Impossible_exn_struct*_tmp95=_cycalloc(sizeof(*_tmp95));_tmp95[0]=({struct Cyc_Core_Impossible_exn_struct _tmp96;_tmp96.tag=Cyc_Core_Impossible;_tmp96.f1=({const char*_tmp97="consume_zero_ral";_tag_dyneither(_tmp97,sizeof(char),17);});_tmp96;});_tmp95;}));else{
# 494
if(needs_unconsume)
return Cyc_CfFlowInfo_make_unique_consumed(env->fenv,(void*)_check_null(e->topt),e,env->iteration_num,new_rval);else{
# 498
return new_rval;}}}
# 511 "new_control_flow.cyc"
static union Cyc_CfFlowInfo_FlowInfo Cyc_NewControlFlow_notzero(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,union Cyc_CfFlowInfo_FlowInfo outflow,struct Cyc_Absyn_Exp*e,enum Cyc_CfFlowInfo_InitLevel il,struct Cyc_List_List*names){
# 517
union Cyc_CfFlowInfo_FlowInfo _tmp98=outflow;struct Cyc_Dict_Dict _tmpA0;struct Cyc_List_List*_tmp9F;if((_tmp98.BottomFL).tag == 1){_LL6B: _LL6C:
 return outflow;}else{_LL6D: _tmpA0=((_tmp98.ReachableFL).val).f1;_tmp9F=((_tmp98.ReachableFL).val).f2;_LL6E: {
# 520
union Cyc_CfFlowInfo_AbsLVal _tmp99=(Cyc_NewControlFlow_anal_Lexp(env,inflow,0,0,e)).f2;union Cyc_CfFlowInfo_AbsLVal _tmp9A=_tmp99;struct Cyc_CfFlowInfo_Place*_tmp9E;if((_tmp9A.UnknownL).tag == 2){_LL70: _LL71:
# 524
 return outflow;}else{_LL72: _tmp9E=(_tmp9A.PlaceL).val;_LL73: {
# 528
void*nzval=il == Cyc_CfFlowInfo_AllIL?(env->fenv)->notzeroall:(env->fenv)->notzerothis;
for(0;names != 0;names=names->tl){
nzval=(void*)({struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*_tmp9B=_region_malloc((env->fenv)->r,sizeof(*_tmp9B));_tmp9B[0]=({struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct _tmp9C;_tmp9C.tag=8;_tmp9C.f1=(struct Cyc_Absyn_Vardecl*)names->hd;_tmp9C.f2=nzval;_tmp9C;});_tmp9B;});}
# 532
nzval=Cyc_NewControlFlow_consume_zero_rval(env,_tmpA0,_tmp9E,e,nzval);{
union Cyc_CfFlowInfo_FlowInfo _tmp9D=Cyc_CfFlowInfo_ReachableFL(Cyc_CfFlowInfo_assign_place(env->fenv,e->loc,_tmpA0,env->all_changed,_tmp9E,nzval),_tmp9F);
# 537
return _tmp9D;};}}_LL6F:;}}_LL6A:;}
# 546
static struct _tuple19 Cyc_NewControlFlow_splitzero(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,union Cyc_CfFlowInfo_FlowInfo outflow,struct Cyc_Absyn_Exp*e,enum Cyc_CfFlowInfo_InitLevel il,struct Cyc_List_List*names){
# 549
union Cyc_CfFlowInfo_FlowInfo _tmpA1=outflow;struct Cyc_Dict_Dict _tmpAD;struct Cyc_List_List*_tmpAC;if((_tmpA1.BottomFL).tag == 1){_LL75: _LL76:
 return({struct _tuple19 _tmpA2;_tmpA2.f1=outflow;_tmpA2.f2=outflow;_tmpA2;});}else{_LL77: _tmpAD=((_tmpA1.ReachableFL).val).f1;_tmpAC=((_tmpA1.ReachableFL).val).f2;_LL78: {
# 552
union Cyc_CfFlowInfo_AbsLVal _tmpA3=(Cyc_NewControlFlow_anal_Lexp(env,inflow,0,0,e)).f2;union Cyc_CfFlowInfo_AbsLVal _tmpA4=_tmpA3;struct Cyc_CfFlowInfo_Place*_tmpAB;if((_tmpA4.UnknownL).tag == 2){_LL7A: _LL7B:
 return({struct _tuple19 _tmpA5;_tmpA5.f1=outflow;_tmpA5.f2=outflow;_tmpA5;});}else{_LL7C: _tmpAB=(_tmpA4.PlaceL).val;_LL7D: {
# 555
void*nzval=il == Cyc_CfFlowInfo_AllIL?(env->fenv)->notzeroall:(env->fenv)->notzerothis;
void*zval=(env->fenv)->zero;
for(0;names != 0;names=names->tl){
nzval=(void*)({struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*_tmpA6=_region_malloc((env->fenv)->r,sizeof(*_tmpA6));_tmpA6[0]=({struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct _tmpA7;_tmpA7.tag=8;_tmpA7.f1=(struct Cyc_Absyn_Vardecl*)names->hd;_tmpA7.f2=nzval;_tmpA7;});_tmpA6;});
zval=(void*)({struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*_tmpA8=_region_malloc((env->fenv)->r,sizeof(*_tmpA8));_tmpA8[0]=({struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct _tmpA9;_tmpA9.tag=8;_tmpA9.f1=(struct Cyc_Absyn_Vardecl*)names->hd;_tmpA9.f2=zval;_tmpA9;});_tmpA8;});}
# 561
nzval=Cyc_NewControlFlow_consume_zero_rval(env,_tmpAD,_tmpAB,e,nzval);
zval=Cyc_NewControlFlow_consume_zero_rval(env,_tmpAD,_tmpAB,e,zval);
return({struct _tuple19 _tmpAA;_tmpAA.f1=
Cyc_CfFlowInfo_ReachableFL(Cyc_CfFlowInfo_assign_place(env->fenv,e->loc,_tmpAD,env->all_changed,_tmpAB,nzval),_tmpAC);_tmpAA.f2=
# 566
Cyc_CfFlowInfo_ReachableFL(Cyc_CfFlowInfo_assign_place(env->fenv,e->loc,_tmpAD,env->all_changed,_tmpAB,zval),_tmpAC);_tmpAA;});}}_LL79:;}}_LL74:;}
# 572
static struct Cyc_CfFlowInfo_NotZero_Absyn_AbsynAnnot_struct Cyc_NewControlFlow_mt_notzero_v={Cyc_CfFlowInfo_NotZero,0};
static struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct Cyc_NewControlFlow_mt_unknownz_v={Cyc_CfFlowInfo_UnknownZ,0};
# 580
static struct _tuple16 Cyc_NewControlFlow_anal_derefR(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,union Cyc_CfFlowInfo_FlowInfo f,struct Cyc_Absyn_Exp*e,void*r){
# 584
void*_tmpB0=Cyc_Tcutil_compress((void*)_check_null(e->topt));void*_tmpB1=_tmpB0;void*_tmpEF;union Cyc_Absyn_Constraint*_tmpEE;union Cyc_Absyn_Constraint*_tmpED;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpB1)->tag == 5){_LL7F: _tmpEF=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpB1)->f1).elt_typ;_tmpEE=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpB1)->f1).ptr_atts).bounds;_tmpED=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpB1)->f1).ptr_atts).zero_term;_LL80: {
# 586
union Cyc_CfFlowInfo_FlowInfo _tmpB2=f;struct Cyc_Dict_Dict _tmpE9;struct Cyc_List_List*_tmpE8;if((_tmpB2.BottomFL).tag == 1){_LL84: _LL85:
# 588
 return({struct _tuple16 _tmpB3;_tmpB3.f1=f;_tmpB3.f2=Cyc_CfFlowInfo_typ_to_absrval(env->fenv,_tmpEF,0,(env->fenv)->unknown_all);_tmpB3;});}else{_LL86: _tmpE9=((_tmpB2.ReachableFL).val).f1;_tmpE8=((_tmpB2.ReachableFL).val).f2;_LL87:
# 591
 if(Cyc_Tcutil_is_bound_one(_tmpEE)){
struct _tuple15 _tmpB4=Cyc_CfFlowInfo_unname_rval((env->fenv)->r,r);struct _tuple15 _tmpB5=_tmpB4;void*_tmpD6;struct Cyc_List_List*_tmpD5;_LL89: _tmpD6=_tmpB5.f1;_tmpD5=_tmpB5.f2;_LL8A:;{
void*_tmpB6=_tmpD6;enum Cyc_CfFlowInfo_InitLevel _tmpD4;struct Cyc_CfFlowInfo_Place*_tmpD3;struct Cyc_Absyn_Vardecl*_tmpD2;void*_tmpD1;switch(*((int*)_tmpB6)){case 8: _LL8C: _tmpD2=((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmpB6)->f1;_tmpD1=(void*)((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmpB6)->f2;_LL8D:
# 595
(int)_throw((void*)({struct Cyc_Core_Impossible_exn_struct*_tmpB7=_cycalloc(sizeof(*_tmpB7));_tmpB7[0]=({struct Cyc_Core_Impossible_exn_struct _tmpB8;_tmpB8.tag=Cyc_Core_Impossible;_tmpB8.f1=({const char*_tmpB9="named location in anal_derefR";_tag_dyneither(_tmpB9,sizeof(char),30);});_tmpB8;});_tmpB7;}));case 1: _LL8E: _LL8F:
 goto _LL91;case 2: _LL90: _LL91:
# 598
{void*_tmpBA=e->annot;void*_tmpBB=_tmpBA;struct Cyc_List_List*_tmpC0;if(((struct Cyc_CfFlowInfo_NotZero_Absyn_AbsynAnnot_struct*)_tmpBB)->tag == Cyc_CfFlowInfo_NotZero){_LL9B: _tmpC0=((struct Cyc_CfFlowInfo_NotZero_Absyn_AbsynAnnot_struct*)_tmpBB)->f1;_LL9C:
# 600
 if(!Cyc_Relations_same_relns(_tmpE8,_tmpC0))goto _LL9E;
goto _LL9A;}else{_LL9D: _LL9E:
# 604
{void*_tmpBC=e->r;void*_tmpBD=_tmpBC;if(((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmpBD)->tag == 22){_LLA0: _LLA1:
# 606
 e->annot=(void*)({struct Cyc_CfFlowInfo_NotZero_Absyn_AbsynAnnot_struct*_tmpBE=_cycalloc(sizeof(*_tmpBE));_tmpBE[0]=({struct Cyc_CfFlowInfo_NotZero_Absyn_AbsynAnnot_struct _tmpBF;_tmpBF.tag=Cyc_CfFlowInfo_NotZero;_tmpBF.f1=Cyc_Relations_copy_relns(Cyc_Core_heap_region,_tmpE8);_tmpBF;});_tmpBE;});
goto _LL9F;}else{_LLA2: _LLA3:
# 609
 e->annot=(void*)& Cyc_NewControlFlow_mt_notzero_v;
goto _LL9F;}_LL9F:;}
# 612
goto _LL9A;}_LL9A:;}
# 614
goto _LL8B;case 5: _LL92: _tmpD3=((struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*)_tmpB6)->f1;_LL93: {
# 618
int possibly_null=0;
{void*_tmpC1=e->annot;void*_tmpC2=_tmpC1;struct Cyc_List_List*_tmpCA;struct Cyc_List_List*_tmpC9;if(((struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct*)_tmpC2)->tag == Cyc_CfFlowInfo_UnknownZ){_LLA5: _tmpC9=((struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct*)_tmpC2)->f1;_LLA6:
# 621
 possibly_null=1;
_tmpCA=_tmpC9;goto _LLA8;}else{if(((struct Cyc_CfFlowInfo_NotZero_Absyn_AbsynAnnot_struct*)_tmpC2)->tag == Cyc_CfFlowInfo_NotZero){_LLA7: _tmpCA=((struct Cyc_CfFlowInfo_NotZero_Absyn_AbsynAnnot_struct*)_tmpC2)->f1;_LLA8:
# 624
 if(!Cyc_Relations_same_relns(_tmpE8,_tmpCA))goto _LLAA;
goto _LLA4;}else{_LLA9: _LLAA:
# 628
{void*_tmpC3=e->r;void*_tmpC4=_tmpC3;if(((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmpC4)->tag == 22){_LLAC: _LLAD:
# 630
 if(possibly_null)
e->annot=(void*)({struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct*_tmpC5=_cycalloc(sizeof(*_tmpC5));_tmpC5[0]=({struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct _tmpC6;_tmpC6.tag=Cyc_CfFlowInfo_UnknownZ;_tmpC6.f1=Cyc_Relations_copy_relns(Cyc_Core_heap_region,_tmpE8);_tmpC6;});_tmpC5;});else{
# 633
e->annot=(void*)({struct Cyc_CfFlowInfo_NotZero_Absyn_AbsynAnnot_struct*_tmpC7=_cycalloc(sizeof(*_tmpC7));_tmpC7[0]=({struct Cyc_CfFlowInfo_NotZero_Absyn_AbsynAnnot_struct _tmpC8;_tmpC8.tag=Cyc_CfFlowInfo_NotZero;_tmpC8.f1=Cyc_Relations_copy_relns(Cyc_Core_heap_region,_tmpE8);_tmpC8;});_tmpC7;});}
goto _LLAB;}else{_LLAE: _LLAF:
# 636
 if(possibly_null)
e->annot=(void*)& Cyc_NewControlFlow_mt_unknownz_v;else{
# 639
e->annot=(void*)& Cyc_NewControlFlow_mt_notzero_v;}
goto _LLAB;}_LLAB:;}
# 642
goto _LLA4;}}_LLA4:;}
# 644
return({struct _tuple16 _tmpCB;_tmpCB.f1=f;_tmpCB.f2=Cyc_CfFlowInfo_lookup_place(_tmpE9,_tmpD3);_tmpCB;});}case 0: _LL94: _LL95:
# 646
 e->annot=(void*)& Cyc_CfFlowInfo_IsZero_val;
return({struct _tuple16 _tmpCC;_tmpCC.f1=Cyc_CfFlowInfo_BottomFL();_tmpCC.f2=Cyc_CfFlowInfo_typ_to_absrval(env->fenv,_tmpEF,0,(env->fenv)->unknown_all);_tmpCC;});case 3: _LL96: _tmpD4=((struct Cyc_CfFlowInfo_UnknownR_CfFlowInfo_AbsRVal_struct*)_tmpB6)->f1;_LL97:
# 649
 f=Cyc_NewControlFlow_notzero(env,inflow,f,e,_tmpD4,_tmpD5);goto _LL99;default: _LL98: _LL99:
# 651
{void*_tmpCD=e->r;void*_tmpCE=_tmpCD;if(((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmpCE)->tag == 22){_LLB1: _LLB2:
# 653
 e->annot=(void*)({struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct*_tmpCF=_cycalloc(sizeof(*_tmpCF));_tmpCF[0]=({struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct _tmpD0;_tmpD0.tag=Cyc_CfFlowInfo_UnknownZ;_tmpD0.f1=Cyc_Relations_copy_relns(Cyc_Core_heap_region,_tmpE8);_tmpD0;});_tmpCF;});
goto _LLB0;}else{_LLB3: _LLB4:
# 656
 e->annot=(void*)& Cyc_NewControlFlow_mt_unknownz_v;
goto _LLB0;}_LLB0:;}
# 659
goto _LL8B;}_LL8B:;};}else{
# 662
void*_tmpD7=e->annot;void*_tmpD8=_tmpD7;struct Cyc_List_List*_tmpDB;if(((struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct*)_tmpD8)->tag == Cyc_CfFlowInfo_UnknownZ){_LLB6: _tmpDB=((struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct*)_tmpD8)->f1;_LLB7:
# 664
 if(!Cyc_Relations_same_relns(_tmpE8,_tmpDB))goto _LLB9;
goto _LLB5;}else{_LLB8: _LLB9:
# 667
 e->annot=(void*)({struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct*_tmpD9=_cycalloc(sizeof(*_tmpD9));_tmpD9[0]=({struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct _tmpDA;_tmpDA.tag=Cyc_CfFlowInfo_UnknownZ;_tmpDA.f1=Cyc_Relations_copy_relns(Cyc_Core_heap_region,_tmpE8);_tmpDA;});_tmpD9;});
goto _LLB5;}_LLB5:;}{
# 671
enum Cyc_CfFlowInfo_InitLevel _tmpDC=Cyc_CfFlowInfo_initlevel(env->fenv,_tmpE9,r);enum Cyc_CfFlowInfo_InitLevel _tmpDD=_tmpDC;switch(_tmpDD){case Cyc_CfFlowInfo_NoneIL: _LLBB: _LLBC: {
# 673
struct _tuple15 _tmpDE=Cyc_CfFlowInfo_unname_rval((env->fenv)->r,r);struct _tuple15 _tmpDF=_tmpDE;void*_tmpE5;_LLC2: _tmpE5=_tmpDF.f1;_LLC3:;
{void*_tmpE0=_tmpE5;if(((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmpE0)->tag == 7){_LLC5: _LLC6:
# 676
({void*_tmpE1=0;Cyc_CfFlowInfo_aerr(e->loc,({const char*_tmpE2="attempt to dereference a consumed alias-free pointer";_tag_dyneither(_tmpE2,sizeof(char),53);}),_tag_dyneither(_tmpE1,sizeof(void*),0));});
goto _LLC4;}else{_LLC7: _LLC8:
# 679
({void*_tmpE3=0;Cyc_CfFlowInfo_aerr(e->loc,({const char*_tmpE4="dereference of possibly uninitialized pointer";_tag_dyneither(_tmpE4,sizeof(char),46);}),_tag_dyneither(_tmpE3,sizeof(void*),0));});}_LLC4:;}
# 681
goto _LLBE;}case Cyc_CfFlowInfo_AllIL: _LLBD: _LLBE:
# 683
 return({struct _tuple16 _tmpE6;_tmpE6.f1=f;_tmpE6.f2=Cyc_CfFlowInfo_typ_to_absrval(env->fenv,_tmpEF,0,(env->fenv)->unknown_all);_tmpE6;});default: _LLBF: _LLC0:
# 685
 return({struct _tuple16 _tmpE7;_tmpE7.f1=f;_tmpE7.f2=Cyc_CfFlowInfo_typ_to_absrval(env->fenv,_tmpEF,0,(env->fenv)->unknown_none);_tmpE7;});}_LLBA:;};}_LL83:;}}else{_LL81: _LL82:
# 688
(int)_throw((void*)({struct Cyc_Core_Impossible_exn_struct*_tmpEA=_cycalloc(sizeof(*_tmpEA));_tmpEA[0]=({struct Cyc_Core_Impossible_exn_struct _tmpEB;_tmpEB.tag=Cyc_Core_Impossible;_tmpEB.f1=({const char*_tmpEC="right deref of non-pointer-type";_tag_dyneither(_tmpEC,sizeof(char),32);});_tmpEB;});_tmpEA;}));}_LL7E:;}
# 695
static struct Cyc_List_List*Cyc_NewControlFlow_add_subscript_reln(struct _RegionHandle*rgn,struct Cyc_List_List*relns,struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2){
# 698
union Cyc_Relations_RelnOp n2=Cyc_Relations_RConst(0);
int e2_valid_op=Cyc_Relations_exp2relnop(e2,& n2);
# 701
{void*_tmpF0=e1->r;void*_tmpF1=_tmpF0;void*_tmpF3;if(((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmpF1)->tag == 1){_LLCA: _tmpF3=(void*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmpF1)->f1;_LLCB: {
# 703
struct Cyc_Absyn_Vardecl*_tmpF2=Cyc_Tcutil_nonesc_vardecl(_tmpF3);
if(_tmpF2 != 0){
union Cyc_Relations_RelnOp n1=Cyc_Relations_RNumelts(_tmpF2);
if(e2_valid_op)
relns=Cyc_Relations_add_relation(rgn,n2,Cyc_Relations_Rlt,n1,relns);}
# 709
goto _LLC9;}}else{_LLCC: _LLCD:
 goto _LLC9;}_LLC9:;}{
# 713
struct Cyc_Absyn_Exp*bound=Cyc_Tcutil_get_type_bound((void*)_check_null(e1->topt));
if(bound != 0){
union Cyc_Relations_RelnOp rbound=Cyc_Relations_RConst(0);
if(Cyc_Relations_exp2relnop(bound,& rbound))
relns=Cyc_Relations_add_relation(rgn,n2,Cyc_Relations_Rlt,rbound,relns);}
# 719
return relns;};}
# 727
static union Cyc_CfFlowInfo_FlowInfo Cyc_NewControlFlow_restore_noconsume_arg(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_Absyn_Exp*exp,unsigned int loc,void*old_rval){
# 732
struct _tuple18 _tmpF4=Cyc_NewControlFlow_anal_Lexp(env,inflow,1,1,exp);struct _tuple18 _tmpF5=_tmpF4;union Cyc_CfFlowInfo_AbsLVal _tmp105;_LLCF: _tmp105=_tmpF5.f2;_LLD0:;
{struct _tuple18 _tmpF6=({struct _tuple18 _tmp104;_tmp104.f1=inflow;_tmp104.f2=_tmp105;_tmp104;});struct _tuple18 _tmpF7=_tmpF6;struct Cyc_Dict_Dict _tmp103;struct Cyc_List_List*_tmp102;struct Cyc_CfFlowInfo_Place*_tmp101;if(((_tmpF7.f1).ReachableFL).tag == 2){if(((_tmpF7.f2).PlaceL).tag == 1){_LLD2: _tmp103=(((_tmpF7.f1).ReachableFL).val).f1;_tmp102=(((_tmpF7.f1).ReachableFL).val).f2;_tmp101=((_tmpF7.f2).PlaceL).val;_LLD3: {
# 735
void*_tmpF8=Cyc_CfFlowInfo_typ_to_absrval(env->fenv,(void*)_check_null(exp->topt),0,(env->fenv)->unknown_all);
# 737
struct _tuple15 _tmpF9=Cyc_CfFlowInfo_unname_rval((env->fenv)->r,old_rval);struct _tuple15 _tmpFA=_tmpF9;void*_tmpFE;struct Cyc_List_List*_tmpFD;_LLD9: _tmpFE=_tmpFA.f1;_tmpFD=_tmpFA.f2;_LLDA:;
for(0;_tmpFD != 0;_tmpFD=_tmpFD->tl){
_tmpF8=(void*)({struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*_tmpFB=_region_malloc((env->fenv)->r,sizeof(*_tmpFB));_tmpFB[0]=({struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct _tmpFC;_tmpFC.tag=8;_tmpFC.f1=(struct Cyc_Absyn_Vardecl*)_tmpFD->hd;_tmpFC.f2=_tmpF8;_tmpFC;});_tmpFB;});}
# 742
_tmp103=Cyc_CfFlowInfo_assign_place(env->fenv,loc,_tmp103,env->all_changed,_tmp101,_tmpF8);
inflow=Cyc_CfFlowInfo_ReachableFL(_tmp103,_tmp102);
Cyc_NewControlFlow_update_tryflow(env,inflow);
# 748
goto _LLD1;}}else{_LLD6: _LLD7:
# 751
({void*_tmpFF=0;((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(({const char*_tmp100="noconsume() parameters must be l-values";_tag_dyneither(_tmp100,sizeof(char),40);}),_tag_dyneither(_tmpFF,sizeof(void*),0));});
goto _LLD1;}}else{_LLD4: _LLD5:
# 749
 goto _LLD1;}_LLD1:;}
# 754
return inflow;}
# 759
static struct _tuple16 Cyc_NewControlFlow_do_assign(struct Cyc_CfFlowInfo_FlowEnv*fenv,struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo outflow,struct Cyc_Absyn_Exp*lexp,union Cyc_CfFlowInfo_AbsLVal lval,struct Cyc_Absyn_Exp*rexp,void*rval,unsigned int loc){
# 767
union Cyc_CfFlowInfo_FlowInfo _tmp106=outflow;struct Cyc_Dict_Dict _tmp114;struct Cyc_List_List*_tmp113;if((_tmp106.BottomFL).tag == 1){_LLDC: _LLDD:
# 769
 return({struct _tuple16 _tmp107;_tmp107.f1=Cyc_CfFlowInfo_BottomFL();_tmp107.f2=rval;_tmp107;});}else{_LLDE: _tmp114=((_tmp106.ReachableFL).val).f1;_tmp113=((_tmp106.ReachableFL).val).f2;_LLDF: {
# 771
union Cyc_CfFlowInfo_AbsLVal _tmp108=lval;struct Cyc_CfFlowInfo_Place*_tmp112;if((_tmp108.PlaceL).tag == 1){_LLE1: _tmp112=(_tmp108.PlaceL).val;_LLE2: {
# 773
struct Cyc_Dict_Dict _tmp109=Cyc_CfFlowInfo_assign_place(fenv,loc,_tmp114,env->all_changed,_tmp112,rval);
_tmp113=Cyc_Relations_reln_assign_exp(fenv->r,_tmp113,lexp,rexp);
outflow=Cyc_CfFlowInfo_ReachableFL(_tmp109,_tmp113);
if(Cyc_NewControlFlow_warn_lose_unique  && 
Cyc_Tcutil_is_noalias_pointer_or_aggr((env->fenv)->r,(void*)_check_null(lexp->topt))){
# 779
struct _tuple15 _tmp10A=Cyc_CfFlowInfo_unname_rval(fenv->r,Cyc_CfFlowInfo_lookup_place(_tmp114,_tmp112));struct _tuple15 _tmp10B=_tmp10A;void*_tmp10F;_LLE6: _tmp10F=_tmp10B.f1;_LLE7:;{
void*_tmp10C=_tmp10F;switch(*((int*)_tmp10C)){case 3: if(((struct Cyc_CfFlowInfo_UnknownR_CfFlowInfo_AbsRVal_struct*)_tmp10C)->f1 == Cyc_CfFlowInfo_NoneIL){_LLE9: _LLEA:
 goto _LLEC;}else{goto _LLEF;}case 7: _LLEB: _LLEC:
 goto _LLEE;case 0: _LLED: _LLEE:
 goto _LLE8;default: _LLEF: _LLF0:
# 785
({void*_tmp10D=0;Cyc_Tcutil_warn(lexp->loc,({const char*_tmp10E="assignment may overwrite alias-free pointer(s)";_tag_dyneither(_tmp10E,sizeof(char),47);}),_tag_dyneither(_tmp10D,sizeof(void*),0));});
goto _LLE8;}_LLE8:;};}
# 790
Cyc_NewControlFlow_update_tryflow(env,outflow);
return({struct _tuple16 _tmp110;_tmp110.f1=outflow;_tmp110.f2=rval;_tmp110;});}}else{_LLE3: _LLE4:
# 793
 return({struct _tuple16 _tmp111;_tmp111.f1=Cyc_NewControlFlow_use_Rval(env,rexp->loc,outflow,rval);_tmp111.f2=rval;_tmp111;});}_LLE0:;}}_LLDB:;}
# 800
static union Cyc_CfFlowInfo_FlowInfo Cyc_NewControlFlow_do_initialize_var(struct Cyc_CfFlowInfo_FlowEnv*fenv,struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo f,struct Cyc_Absyn_Vardecl*vd,struct Cyc_Absyn_Exp*rexp,void*rval,unsigned int loc){
# 806
union Cyc_CfFlowInfo_FlowInfo _tmp115=f;struct Cyc_Dict_Dict _tmp11B;struct Cyc_List_List*_tmp11A;if((_tmp115.BottomFL).tag == 1){_LLF2: _LLF3:
 return Cyc_CfFlowInfo_BottomFL();}else{_LLF4: _tmp11B=((_tmp115.ReachableFL).val).f1;_tmp11A=((_tmp115.ReachableFL).val).f2;_LLF5:
# 811
 _tmp11B=Cyc_CfFlowInfo_assign_place(fenv,loc,_tmp11B,env->all_changed,({struct Cyc_CfFlowInfo_Place*_tmp116=_region_malloc(env->r,sizeof(*_tmp116));_tmp116->root=(void*)({struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*_tmp117=_region_malloc(env->r,sizeof(*_tmp117));_tmp117[0]=({struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct _tmp118;_tmp118.tag=0;_tmp118.f1=vd;_tmp118;});_tmp117;});_tmp116->fields=0;_tmp116;}),rval);
# 814
_tmp11A=Cyc_Relations_reln_assign_var(env->r,_tmp11A,vd,rexp);{
union Cyc_CfFlowInfo_FlowInfo _tmp119=Cyc_CfFlowInfo_ReachableFL(_tmp11B,_tmp11A);
Cyc_NewControlFlow_update_tryflow(env,_tmp119);
# 819
return _tmp119;};}_LLF1:;}struct _tuple24{struct Cyc_Absyn_Vardecl**f1;struct Cyc_Absyn_Exp*f2;};
# 823
static union Cyc_CfFlowInfo_FlowInfo Cyc_NewControlFlow_initialize_pat_vars(struct Cyc_CfFlowInfo_FlowEnv*fenv,struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_List_List*vds,int name_locs,unsigned int pat_loc,unsigned int exp_loc){
# 830
if(vds == 0)return inflow;{
# 833
struct Cyc_List_List*_tmp11C=((struct Cyc_List_List*(*)(struct Cyc_List_List*l))Cyc_Tcutil_filter_nulls)((((struct _tuple1(*)(struct Cyc_List_List*x))Cyc_List_split)(vds)).f1);
struct Cyc_List_List*es=0;
{struct Cyc_List_List*x=vds;for(0;x != 0;x=x->tl){
if((*((struct _tuple24*)x->hd)).f1 == 0)es=({struct Cyc_List_List*_tmp11D=_cycalloc(sizeof(*_tmp11D));_tmp11D->hd=(struct Cyc_Absyn_Exp*)_check_null((*((struct _tuple24*)x->hd)).f2);_tmp11D->tl=es;_tmp11D;});}}
# 839
inflow=Cyc_NewControlFlow_add_vars(fenv,inflow,_tmp11C,fenv->unknown_all,pat_loc,name_locs);
# 841
inflow=Cyc_NewControlFlow_expand_unique_places(env,inflow,es);
{struct Cyc_List_List*x=es;for(0;x != 0;x=x->tl){
# 845
struct _tuple16 _tmp11E=Cyc_NewControlFlow_anal_Rexp(env,1,inflow,(struct Cyc_Absyn_Exp*)x->hd);struct _tuple16 _tmp11F=_tmp11E;union Cyc_CfFlowInfo_FlowInfo _tmp121;void*_tmp120;_LLF7: _tmp121=_tmp11F.f1;_tmp120=_tmp11F.f2;_LLF8:;
inflow=Cyc_NewControlFlow_use_Rval(env,exp_loc,_tmp121,_tmp120);}}{
# 853
struct Cyc_List_List*_tmp122=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_rev)(vds);
for(0;_tmp122 != 0;_tmp122=_tmp122->tl){
struct _tuple24*_tmp123=(struct _tuple24*)_tmp122->hd;struct _tuple24*_tmp124=_tmp123;struct Cyc_Absyn_Vardecl**_tmp13F;struct Cyc_Absyn_Exp*_tmp13E;_LLFA: _tmp13F=_tmp124->f1;_tmp13E=_tmp124->f2;_LLFB:;
if(_tmp13F != 0  && _tmp13E != 0){
if(_tmp13E->topt == 0)
({struct Cyc_String_pa_PrintArg_struct _tmp127;_tmp127.tag=0;_tmp127.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
Cyc_Absynpp_exp2string(_tmp13E));({void*_tmp125[1]={& _tmp127};((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(({const char*_tmp126="oops! pattern init expr %s has no type!\n";_tag_dyneither(_tmp126,sizeof(char),41);}),_tag_dyneither(_tmp125,sizeof(void*),1));});});{
# 867
struct Cyc_List_List l=({struct Cyc_List_List _tmp13D;_tmp13D.hd=_tmp13E;_tmp13D.tl=0;_tmp13D;});
union Cyc_CfFlowInfo_FlowInfo _tmp128=Cyc_NewControlFlow_expand_unique_places(env,inflow,& l);
struct _tuple18 _tmp129=Cyc_NewControlFlow_anal_Lexp(env,_tmp128,0,0,_tmp13E);struct _tuple18 _tmp12A=_tmp129;union Cyc_CfFlowInfo_AbsLVal _tmp13C;_LLFD: _tmp13C=_tmp12A.f2;_LLFE:;{
struct _tuple16 _tmp12B=Cyc_NewControlFlow_anal_Rexp(env,1,_tmp128,_tmp13E);struct _tuple16 _tmp12C=_tmp12B;union Cyc_CfFlowInfo_FlowInfo _tmp13B;void*_tmp13A;_LL100: _tmp13B=_tmp12C.f1;_tmp13A=_tmp12C.f2;_LL101:;{
union Cyc_CfFlowInfo_FlowInfo _tmp12D=_tmp13B;struct Cyc_Dict_Dict _tmp139;struct Cyc_List_List*_tmp138;if((_tmp12D.ReachableFL).tag == 2){_LL103: _tmp139=((_tmp12D.ReachableFL).val).f1;_tmp138=((_tmp12D.ReachableFL).val).f2;_LL104:
# 873
 if(name_locs){
union Cyc_CfFlowInfo_AbsLVal _tmp12E=_tmp13C;struct Cyc_CfFlowInfo_Place*_tmp137;if((_tmp12E.PlaceL).tag == 1){_LL108: _tmp137=(_tmp12E.PlaceL).val;_LL109:
# 876
 _tmp13A=(void*)({struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*_tmp12F=_region_malloc(fenv->r,sizeof(*_tmp12F));_tmp12F[0]=({struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct _tmp130;_tmp130.tag=8;_tmp130.f1=*_tmp13F;_tmp130.f2=_tmp13A;_tmp130;});_tmp12F;});{
# 879
void*_tmp131=Cyc_CfFlowInfo_lookup_place(_tmp139,_tmp137);
_tmp131=(void*)({struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*_tmp132=_region_malloc(fenv->r,sizeof(*_tmp132));_tmp132[0]=({struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct _tmp133;_tmp133.tag=8;_tmp133.f1=*_tmp13F;_tmp133.f2=_tmp131;_tmp133;});_tmp132;});
_tmp139=Cyc_CfFlowInfo_assign_place(fenv,exp_loc,_tmp139,env->all_changed,_tmp137,_tmp131);
_tmp13B=Cyc_CfFlowInfo_ReachableFL(_tmp139,_tmp138);
goto _LL107;};}else{_LL10A: _LL10B:
# 886
 if(Cyc_Tcutil_is_noalias_pointer_or_aggr((env->fenv)->r,(void*)_check_null(_tmp13E->topt)) && !
# 888
Cyc_Tcutil_is_noalias_pointer_or_aggr((env->fenv)->r,(*_tmp13F)->type))
# 890
({struct Cyc_String_pa_PrintArg_struct _tmp136;_tmp136.tag=0;_tmp136.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
Cyc_Absynpp_exp2string(_tmp13E));({void*_tmp134[1]={& _tmp136};Cyc_CfFlowInfo_aerr(exp_loc,({const char*_tmp135="aliased pattern expression not an l-value: %s";_tag_dyneither(_tmp135,sizeof(char),46);}),_tag_dyneither(_tmp134,sizeof(void*),1));});});}_LL107:;}
# 898
inflow=Cyc_NewControlFlow_do_initialize_var(fenv,env,_tmp13B,*_tmp13F,_tmp13E,_tmp13A,pat_loc);
goto _LL102;}else{_LL105: _LL106:
# 901
 goto _LL102;}_LL102:;};};};}}
# 906
return inflow;};};}
# 909
static int Cyc_NewControlFlow_is_local_var_rooted_path(struct Cyc_Absyn_Exp*e,int cast_ok){
# 911
void*_tmp140=e->r;void*_tmp141=_tmp140;struct Cyc_Absyn_Exp*_tmp148;struct Cyc_Absyn_Exp*_tmp147;struct Cyc_Absyn_Exp*_tmp146;struct Cyc_Absyn_Exp*_tmp145;struct Cyc_Absyn_Exp*_tmp144;switch(*((int*)_tmp141)){case 1: switch(*((int*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp141)->f1)){case 4: _LL10D: _LL10E:
 goto _LL110;case 3: _LL10F: _LL110:
 goto _LL112;case 5: _LL111: _LL112:
 return 1;default: goto _LL11D;}case 19: _LL113: _tmp144=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp141)->f1;_LL114:
 _tmp145=_tmp144;goto _LL116;case 20: _LL115: _tmp145=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp141)->f1;_LL116:
 _tmp146=_tmp145;goto _LL118;case 21: _LL117: _tmp146=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp141)->f1;_LL118:
# 918
 return Cyc_NewControlFlow_is_local_var_rooted_path(_tmp146,cast_ok);case 22: _LL119: _tmp147=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp141)->f1;_LL11A: {
# 920
void*_tmp142=Cyc_Tcutil_compress((void*)_check_null(_tmp147->topt));void*_tmp143=_tmp142;if(((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp143)->tag == 10){_LL120: _LL121:
 return Cyc_NewControlFlow_is_local_var_rooted_path(_tmp147,cast_ok);}else{_LL122: _LL123:
 return 0;}_LL11F:;}case 13: _LL11B: _tmp148=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp141)->f2;_LL11C:
# 925
 if(cast_ok)return Cyc_NewControlFlow_is_local_var_rooted_path(_tmp148,cast_ok);else{
return 0;}default: _LL11D: _LL11E:
 return 0;}_LL10C:;}
# 931
static int Cyc_NewControlFlow_subst_param(struct Cyc_List_List*exps,union Cyc_Relations_RelnOp*rop){
union Cyc_Relations_RelnOp _tmp149=*rop;union Cyc_Relations_RelnOp _tmp14A=_tmp149;unsigned int _tmp14E;unsigned int _tmp14D;switch((_tmp14A.RParamNumelts).tag){case 5: _LL125: _tmp14D=(_tmp14A.RParam).val;_LL126: {
# 934
struct Cyc_Absyn_Exp*_tmp14B=((struct Cyc_Absyn_Exp*(*)(struct Cyc_List_List*x,int n))Cyc_List_nth)(exps,(int)_tmp14D);
return Cyc_Relations_exp2relnop(_tmp14B,rop);}case 6: _LL127: _tmp14E=(_tmp14A.RParamNumelts).val;_LL128: {
# 937
struct Cyc_Absyn_Exp*_tmp14C=((struct Cyc_Absyn_Exp*(*)(struct Cyc_List_List*x,int n))Cyc_List_nth)(exps,(int)_tmp14E);
return Cyc_Relations_exp2relnop(Cyc_Absyn_prim1_exp(Cyc_Absyn_Numelts,_tmp14C,0),rop);}default: _LL129: _LL12A:
 return 1;}_LL124:;}
# 943
static struct _dyneither_ptr Cyc_NewControlFlow_subst_param_string(struct Cyc_List_List*exps,union Cyc_Relations_RelnOp rop){
union Cyc_Relations_RelnOp _tmp14F=rop;unsigned int _tmp151;unsigned int _tmp150;switch((_tmp14F.RParamNumelts).tag){case 5: _LL12C: _tmp150=(_tmp14F.RParam).val;_LL12D:
# 946
 return Cyc_Absynpp_exp2string(((struct Cyc_Absyn_Exp*(*)(struct Cyc_List_List*x,int n))Cyc_List_nth)(exps,(int)_tmp150));case 6: _LL12E: _tmp151=(_tmp14F.RParamNumelts).val;_LL12F:
# 948
 return Cyc_Absynpp_exp2string(((struct Cyc_Absyn_Exp*(*)(struct Cyc_List_List*x,int n))Cyc_List_nth)(exps,(int)_tmp151));default: _LL130: _LL131:
 return Cyc_Relations_rop2string(rop);}_LL12B:;}
# 953
static void Cyc_NewControlFlow_check_fn_requires(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_List_List*exps,struct Cyc_List_List*req,unsigned int loc){
# 956
union Cyc_CfFlowInfo_FlowInfo _tmp152=inflow;struct Cyc_Dict_Dict _tmp15C;struct Cyc_List_List*_tmp15B;if((_tmp152.BottomFL).tag == 1){_LL133: _LL134:
 return;}else{_LL135: _tmp15C=((_tmp152.ReachableFL).val).f1;_tmp15B=((_tmp152.ReachableFL).val).f2;_LL136:
# 959
 for(0;req != 0;req=req->tl){
struct Cyc_Relations_Reln*_tmp153=(struct Cyc_Relations_Reln*)req->hd;
union Cyc_Relations_RelnOp rop1=_tmp153->rop1;
union Cyc_Relations_RelnOp rop2=_tmp153->rop2;
enum Cyc_Relations_Relation _tmp154=Cyc_Relations_flip_relation(_tmp153->relation);
if((!Cyc_NewControlFlow_subst_param(exps,& rop1) || !Cyc_NewControlFlow_subst_param(exps,& rop2)) || 
Cyc_Relations_consistent_relations(Cyc_Relations_add_relation(env->r,rop2,_tmp154,rop1,_tmp15B))){
struct _dyneither_ptr s1=Cyc_NewControlFlow_subst_param_string(exps,rop1);
struct _dyneither_ptr s2=Cyc_Relations_relation2string(_tmp153->relation);
struct _dyneither_ptr s3=Cyc_NewControlFlow_subst_param_string(exps,rop2);
({struct Cyc_String_pa_PrintArg_struct _tmp15A;_tmp15A.tag=0;_tmp15A.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
Cyc_Relations_relns2string(_tmp15B));({struct Cyc_String_pa_PrintArg_struct _tmp159;_tmp159.tag=0;_tmp159.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)s3);({struct Cyc_String_pa_PrintArg_struct _tmp158;_tmp158.tag=0;_tmp158.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)s2);({struct Cyc_String_pa_PrintArg_struct _tmp157;_tmp157.tag=0;_tmp157.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)s1);({void*_tmp155[4]={& _tmp157,& _tmp158,& _tmp159,& _tmp15A};Cyc_Tcutil_terr(loc,({const char*_tmp156="cannot prove that @requires clause %s %s %s is satisfied\n (all I know is %s)";_tag_dyneither(_tmp156,sizeof(char),77);}),_tag_dyneither(_tmp155,sizeof(void*),4));});});});});});
break;}}
# 974
goto _LL132;}_LL132:;}struct _tuple25{union Cyc_CfFlowInfo_AbsLVal f1;union Cyc_CfFlowInfo_FlowInfo f2;};struct _tuple26{struct Cyc_List_List*f1;struct Cyc_Absyn_Exp*f2;};
# 979
static struct _tuple16 Cyc_NewControlFlow_anal_Rexp(struct Cyc_NewControlFlow_AnalEnv*env,int copy_ctxt,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_Absyn_Exp*e){
# 983
struct Cyc_CfFlowInfo_FlowEnv*_tmp15D=env->fenv;
struct Cyc_Dict_Dict d;
struct Cyc_List_List*relns;
# 996
{union Cyc_CfFlowInfo_FlowInfo _tmp15E=inflow;struct Cyc_Dict_Dict _tmp161;struct Cyc_List_List*_tmp160;if((_tmp15E.BottomFL).tag == 1){_LL138: _LL139:
 return({struct _tuple16 _tmp15F;_tmp15F.f1=Cyc_CfFlowInfo_BottomFL();_tmp15F.f2=_tmp15D->unknown_all;_tmp15F;});}else{_LL13A: _tmp161=((_tmp15E.ReachableFL).val).f1;_tmp160=((_tmp15E.ReachableFL).val).f2;_LL13B:
 d=_tmp161;relns=_tmp160;}_LL137:;}
# 1011 "new_control_flow.cyc"
if((copy_ctxt  && Cyc_NewControlFlow_is_local_var_rooted_path(e,0)) && 
Cyc_Tcutil_is_noalias_pointer_or_aggr(env->r,(void*)_check_null(e->topt))){
# 1032 "new_control_flow.cyc"
struct _tuple18 _tmp162=Cyc_NewControlFlow_anal_Lexp(env,inflow,1,0,e);struct _tuple18 _tmp163=_tmp162;union Cyc_CfFlowInfo_FlowInfo _tmp173;union Cyc_CfFlowInfo_AbsLVal _tmp172;_LL13D: _tmp173=_tmp163.f1;_tmp172=_tmp163.f2;_LL13E:;{
struct _tuple18 _tmp164=({struct _tuple18 _tmp171;_tmp171.f1=_tmp173;_tmp171.f2=_tmp172;_tmp171;});struct _tuple18 _tmp165=_tmp164;struct Cyc_Dict_Dict _tmp170;struct Cyc_List_List*_tmp16F;struct Cyc_CfFlowInfo_Place*_tmp16E;if(((_tmp165.f1).ReachableFL).tag == 2){if(((_tmp165.f2).PlaceL).tag == 1){_LL140: _tmp170=(((_tmp165.f1).ReachableFL).val).f1;_tmp16F=(((_tmp165.f1).ReachableFL).val).f2;_tmp16E=((_tmp165.f2).PlaceL).val;_LL141: {
# 1035
void*_tmp166=Cyc_CfFlowInfo_lookup_place(_tmp170,_tmp16E);
int needs_unconsume=0;
if(Cyc_CfFlowInfo_is_unique_consumed(e,env->iteration_num,_tmp166,& needs_unconsume)){
({void*_tmp167=0;Cyc_CfFlowInfo_aerr(e->loc,({const char*_tmp168="expression attempts to copy a consumed alias-free value";_tag_dyneither(_tmp168,sizeof(char),56);}),_tag_dyneither(_tmp167,sizeof(void*),0));});
return({struct _tuple16 _tmp169;_tmp169.f1=Cyc_CfFlowInfo_BottomFL();_tmp169.f2=_tmp15D->unknown_all;_tmp169;});}else{
# 1041
if(needs_unconsume)
# 1043
return({struct _tuple16 _tmp16A;_tmp16A.f1=_tmp173;_tmp16A.f2=Cyc_CfFlowInfo_make_unique_unconsumed(_tmp15D,_tmp166);_tmp16A;});else{
# 1046
void*_tmp16B=Cyc_CfFlowInfo_make_unique_consumed(_tmp15D,(void*)_check_null(e->topt),e,env->iteration_num,_tmp166);
struct Cyc_Dict_Dict _tmp16C=Cyc_CfFlowInfo_assign_place(_tmp15D,e->loc,_tmp170,env->all_changed,_tmp16E,_tmp16B);
# 1058
return({struct _tuple16 _tmp16D;_tmp16D.f1=Cyc_CfFlowInfo_ReachableFL(_tmp16C,_tmp16F);_tmp16D.f2=_tmp166;_tmp16D;});}}}}else{goto _LL142;}}else{_LL142: _LL143:
# 1060
 goto _LL13F;}_LL13F:;};}{
# 1063
void*_tmp174=e->r;void*_tmp175=_tmp174;struct Cyc_Absyn_Stmt*_tmp382;struct Cyc_Absyn_Exp*_tmp381;void*_tmp380;int _tmp37F;struct Cyc_Absyn_Vardecl*_tmp37E;struct Cyc_Absyn_Exp*_tmp37D;struct Cyc_Absyn_Exp*_tmp37C;int _tmp37B;struct Cyc_List_List*_tmp37A;struct Cyc_List_List*_tmp379;enum Cyc_Absyn_AggrKind _tmp378;struct Cyc_List_List*_tmp377;struct Cyc_List_List*_tmp376;struct Cyc_List_List*_tmp375;struct Cyc_Absyn_Exp*_tmp374;struct Cyc_Absyn_Exp*_tmp373;struct Cyc_Absyn_Exp*_tmp372;struct Cyc_Absyn_Exp*_tmp371;struct Cyc_Absyn_Exp*_tmp370;struct Cyc_Absyn_Exp*_tmp36F;struct Cyc_Absyn_Exp*_tmp36E;struct Cyc_Absyn_Exp*_tmp36D;struct Cyc_Absyn_Exp*_tmp36C;struct Cyc_Absyn_Exp*_tmp36B;struct _dyneither_ptr*_tmp36A;struct Cyc_Absyn_Exp*_tmp369;struct _dyneither_ptr*_tmp368;struct Cyc_Absyn_Exp*_tmp367;struct _dyneither_ptr*_tmp366;struct Cyc_Absyn_Exp*_tmp365;struct Cyc_Absyn_Exp*_tmp364;struct Cyc_Absyn_Exp*_tmp363;struct Cyc_Absyn_Exp*_tmp362;struct Cyc_Absyn_Exp*_tmp361;struct Cyc_Absyn_Exp*_tmp360;int _tmp35F;struct Cyc_Absyn_Exp*_tmp35E;void**_tmp35D;struct Cyc_Absyn_Exp*_tmp35C;int _tmp35B;struct Cyc_Absyn_Exp*_tmp35A;struct Cyc_List_List*_tmp359;struct Cyc_Absyn_Exp*_tmp358;struct Cyc_Absyn_Exp*_tmp357;struct Cyc_Absyn_Exp*_tmp356;struct Cyc_Absyn_Exp*_tmp355;struct Cyc_Absyn_Exp*_tmp354;struct Cyc_Absyn_Exp*_tmp353;struct Cyc_Absyn_Exp*_tmp352;struct Cyc_Absyn_Exp*_tmp351;enum Cyc_Absyn_Primop _tmp350;struct Cyc_List_List*_tmp34F;struct Cyc_List_List*_tmp34E;struct Cyc_Absyn_Datatypedecl*_tmp34D;struct Cyc_Absyn_Vardecl*_tmp34C;struct Cyc_Absyn_Vardecl*_tmp34B;struct Cyc_Absyn_Vardecl*_tmp34A;struct Cyc_Absyn_Exp*_tmp349;struct Cyc_Absyn_Exp*_tmp348;struct Cyc_Absyn_Exp*_tmp347;struct Cyc_Absyn_Exp*_tmp346;switch(*((int*)_tmp175)){case 13: if(((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp175)->f4 == Cyc_Absyn_Null_to_NonNull){_LL145: _tmp346=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp175)->f2;_LL146: {
# 1067
struct _tuple16 _tmp176=Cyc_NewControlFlow_anal_Rexp(env,copy_ctxt,inflow,_tmp346);struct _tuple16 _tmp177=_tmp176;union Cyc_CfFlowInfo_FlowInfo _tmp17E;void*_tmp17D;_LL1AE: _tmp17E=_tmp177.f1;_tmp17D=_tmp177.f2;_LL1AF:;{
struct _tuple16 _tmp178=Cyc_NewControlFlow_anal_derefR(env,inflow,_tmp17E,_tmp346,_tmp17D);struct _tuple16 _tmp179=_tmp178;union Cyc_CfFlowInfo_FlowInfo _tmp17C;void*_tmp17B;_LL1B1: _tmp17C=_tmp179.f1;_tmp17B=_tmp179.f2;_LL1B2:;
return({struct _tuple16 _tmp17A;_tmp17A.f1=_tmp17C;_tmp17A.f2=_tmp17D;_tmp17A;});};}}else{_LL147: _tmp347=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp175)->f2;_LL148:
# 1073
 _tmp348=_tmp347;goto _LL14A;}case 11: _LL149: _tmp348=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmp175)->f1;_LL14A:
 _tmp349=_tmp348;goto _LL14C;case 12: _LL14B: _tmp349=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmp175)->f1;_LL14C:
 return Cyc_NewControlFlow_anal_Rexp(env,copy_ctxt,inflow,_tmp349);case 0: switch(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp175)->f1).Int_c).tag){case 1: _LL14D: _LL14E:
# 1077
 goto _LL150;case 5: if((((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp175)->f1).Int_c).val).f2 == 0){_LL14F: _LL150:
 return({struct _tuple16 _tmp17F;_tmp17F.f1=inflow;_tmp17F.f2=_tmp15D->zero;_tmp17F;});}else{_LL151: _LL152:
 goto _LL154;}default: _LL157: _LL158:
# 1083
 goto _LL15A;}case 1: switch(*((int*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp175)->f1)){case 2: _LL153: _LL154:
# 1080
 return({struct _tuple16 _tmp180;_tmp180.f1=inflow;_tmp180.f2=_tmp15D->notzeroall;_tmp180;});case 1: _LL163: _LL164:
# 1091
 return({struct _tuple16 _tmp182;_tmp182.f1=inflow;_tmp182.f2=Cyc_CfFlowInfo_typ_to_absrval(_tmp15D,(void*)_check_null(e->topt),0,_tmp15D->unknown_all);_tmp182;});case 3: _LL165: _tmp34A=((struct Cyc_Absyn_Param_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp175)->f1)->f1;_LL166:
# 1094
 _tmp34B=_tmp34A;goto _LL168;case 4: _LL167: _tmp34B=((struct Cyc_Absyn_Local_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp175)->f1)->f1;_LL168:
 _tmp34C=_tmp34B;goto _LL16A;case 5: _LL169: _tmp34C=((struct Cyc_Absyn_Pat_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp175)->f1)->f1;_LL16A: {
# 1098
struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct vdroot=({struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct _tmp184;_tmp184.tag=0;_tmp184.f1=_tmp34C;_tmp184;});
return({struct _tuple16 _tmp183;_tmp183.f1=inflow;_tmp183.f2=((void*(*)(struct Cyc_Dict_Dict d,int(*cmp)(void*,void*),void*k))Cyc_Dict_lookup_other)(d,Cyc_CfFlowInfo_root_cmp,(void*)& vdroot);_tmp183;});}default: _LL1A3: _LL1A4:
# 1706
 goto _LL1A6;}case 30: if(((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmp175)->f1 == 0){_LL155: _LL156:
# 1082
 goto _LL158;}else{_LL191: _tmp34E=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmp175)->f1;_tmp34D=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmp175)->f2;_LL192:
# 1562
 _tmp375=_tmp34E;goto _LL194;}case 17: _LL159: _LL15A:
# 1084
 goto _LL15C;case 16: _LL15B: _LL15C:
 goto _LL15E;case 18: _LL15D: _LL15E:
 goto _LL160;case 32: _LL15F: _LL160:
 goto _LL162;case 31: _LL161: _LL162:
 return({struct _tuple16 _tmp181;_tmp181.f1=inflow;_tmp181.f2=_tmp15D->unknown_all;_tmp181;});case 2: _LL16B: _tmp350=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp175)->f1;_tmp34F=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp175)->f2;_LL16C: {
# 1105
struct _tuple16 _tmp185=Cyc_NewControlFlow_anal_use_ints(env,inflow,_tmp34F);struct _tuple16 _tmp186=_tmp185;union Cyc_CfFlowInfo_FlowInfo _tmp189;void*_tmp188;_LL1B4: _tmp189=_tmp186.f1;_tmp188=_tmp186.f2;_LL1B5:;
return({struct _tuple16 _tmp187;_tmp187.f1=_tmp189;_tmp187.f2=_tmp188;_tmp187;});}case 4: _LL16D: _tmp351=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmp175)->f1;_LL16E: {
# 1109
struct Cyc_List_List _tmp18A=({struct Cyc_List_List _tmp198;_tmp198.hd=_tmp351;_tmp198.tl=0;_tmp198;});
struct _tuple16 _tmp18B=Cyc_NewControlFlow_anal_use_ints(env,inflow,& _tmp18A);struct _tuple16 _tmp18C=_tmp18B;union Cyc_CfFlowInfo_FlowInfo _tmp197;_LL1B7: _tmp197=_tmp18C.f1;_LL1B8:;{
struct _tuple18 _tmp18D=Cyc_NewControlFlow_anal_Lexp(env,_tmp197,0,0,_tmp351);struct _tuple18 _tmp18E=_tmp18D;union Cyc_CfFlowInfo_AbsLVal _tmp196;_LL1BA: _tmp196=_tmp18E.f2;_LL1BB:;
{struct _tuple25 _tmp18F=({struct _tuple25 _tmp194;_tmp194.f1=_tmp196;_tmp194.f2=_tmp197;_tmp194;});struct _tuple25 _tmp190=_tmp18F;struct Cyc_CfFlowInfo_Place*_tmp193;struct Cyc_Dict_Dict _tmp192;struct Cyc_List_List*_tmp191;if(((_tmp190.f1).PlaceL).tag == 1){if(((_tmp190.f2).ReachableFL).tag == 2){_LL1BD: _tmp193=((_tmp190.f1).PlaceL).val;_tmp192=(((_tmp190.f2).ReachableFL).val).f1;_tmp191=(((_tmp190.f2).ReachableFL).val).f2;_LL1BE:
# 1114
 _tmp191=Cyc_Relations_reln_kill_exp(_tmp15D->r,_tmp191,_tmp351);
_tmp197=Cyc_CfFlowInfo_ReachableFL(Cyc_CfFlowInfo_assign_place(_tmp15D,_tmp351->loc,_tmp192,env->all_changed,_tmp193,_tmp15D->unknown_all),_tmp191);
# 1119
goto _LL1BC;}else{goto _LL1BF;}}else{_LL1BF: _LL1C0:
 goto _LL1BC;}_LL1BC:;}
# 1122
return({struct _tuple16 _tmp195;_tmp195.f1=_tmp197;_tmp195.f2=_tmp15D->unknown_all;_tmp195;});};}case 3: if(((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp175)->f2 != 0){_LL16F: _tmp353=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp175)->f1;_tmp352=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp175)->f3;_LL170:
# 1125
 if(copy_ctxt  && Cyc_Tcutil_is_noalias_pointer_or_aggr(env->r,(void*)_check_null(e->topt))){
({void*_tmp199=0;Cyc_CfFlowInfo_aerr(e->loc,({const char*_tmp19A="cannot track alias-free pointers through multiple assignments";_tag_dyneither(_tmp19A,sizeof(char),62);}),_tag_dyneither(_tmp199,sizeof(void*),0));});
return({struct _tuple16 _tmp19B;_tmp19B.f1=Cyc_CfFlowInfo_BottomFL();_tmp19B.f2=_tmp15D->unknown_all;_tmp19B;});}{
# 1129
struct Cyc_List_List _tmp19C=({struct Cyc_List_List _tmp1AB;_tmp1AB.hd=_tmp352;_tmp1AB.tl=0;_tmp1AB;});
struct Cyc_List_List _tmp19D=({struct Cyc_List_List _tmp1AA;_tmp1AA.hd=_tmp353;_tmp1AA.tl=& _tmp19C;_tmp1AA;});
struct _tuple16 _tmp19E=Cyc_NewControlFlow_anal_use_ints(env,inflow,& _tmp19D);struct _tuple16 _tmp19F=_tmp19E;union Cyc_CfFlowInfo_FlowInfo _tmp1A9;_LL1C2: _tmp1A9=_tmp19F.f1;_LL1C3:;{
struct _tuple18 _tmp1A0=Cyc_NewControlFlow_anal_Lexp(env,_tmp1A9,0,0,_tmp353);struct _tuple18 _tmp1A1=_tmp1A0;union Cyc_CfFlowInfo_AbsLVal _tmp1A8;_LL1C5: _tmp1A8=_tmp1A1.f2;_LL1C6:;
{union Cyc_CfFlowInfo_FlowInfo _tmp1A2=_tmp1A9;struct Cyc_Dict_Dict _tmp1A6;struct Cyc_List_List*_tmp1A5;if((_tmp1A2.ReachableFL).tag == 2){_LL1C8: _tmp1A6=((_tmp1A2.ReachableFL).val).f1;_tmp1A5=((_tmp1A2.ReachableFL).val).f2;_LL1C9:
# 1135
{union Cyc_CfFlowInfo_AbsLVal _tmp1A3=_tmp1A8;struct Cyc_CfFlowInfo_Place*_tmp1A4;if((_tmp1A3.PlaceL).tag == 1){_LL1CD: _tmp1A4=(_tmp1A3.PlaceL).val;_LL1CE:
# 1139
 _tmp1A5=Cyc_Relations_reln_kill_exp(_tmp15D->r,_tmp1A5,_tmp353);
_tmp1A6=Cyc_CfFlowInfo_assign_place(_tmp15D,_tmp353->loc,_tmp1A6,env->all_changed,_tmp1A4,_tmp15D->unknown_all);
# 1142
_tmp1A9=Cyc_CfFlowInfo_ReachableFL(_tmp1A6,_tmp1A5);
# 1146
goto _LL1CC;}else{_LL1CF: _LL1D0:
# 1149
 goto _LL1CC;}_LL1CC:;}
# 1151
goto _LL1C7;}else{_LL1CA: _LL1CB:
 goto _LL1C7;}_LL1C7:;}
# 1154
return({struct _tuple16 _tmp1A7;_tmp1A7.f1=_tmp1A9;_tmp1A7.f2=_tmp15D->unknown_all;_tmp1A7;});};};}else{_LL171: _tmp355=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp175)->f1;_tmp354=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp175)->f3;_LL172:
# 1158
 if(copy_ctxt  && Cyc_Tcutil_is_noalias_pointer_or_aggr(env->r,(void*)_check_null(e->topt))){
({void*_tmp1AC=0;Cyc_CfFlowInfo_aerr(e->loc,({const char*_tmp1AD="cannot track alias-free pointers through multiple assignments";_tag_dyneither(_tmp1AD,sizeof(char),62);}),_tag_dyneither(_tmp1AC,sizeof(void*),0));});
return({struct _tuple16 _tmp1AE;_tmp1AE.f1=Cyc_CfFlowInfo_BottomFL();_tmp1AE.f2=_tmp15D->unknown_all;_tmp1AE;});}{
# 1162
struct Cyc_Dict_Dict*_tmp1AF=env->all_changed;
# 1164
inflow=Cyc_NewControlFlow_expand_unique_places(env,inflow,({struct Cyc_Absyn_Exp*_tmp1B0[2];_tmp1B0[1]=_tmp354;_tmp1B0[0]=_tmp355;((struct Cyc_List_List*(*)(struct _RegionHandle*,struct _dyneither_ptr))Cyc_List_rlist)(env->r,_tag_dyneither(_tmp1B0,sizeof(struct Cyc_Absyn_Exp*),2));}));
while(1){
env->all_changed=({struct Cyc_Dict_Dict*_tmp1B1=_region_malloc(env->r,sizeof(*_tmp1B1));_tmp1B1[0]=_tmp15D->mt_place_set;_tmp1B1;});{
struct _tuple18 _tmp1B2=Cyc_NewControlFlow_anal_Lexp(env,inflow,0,0,_tmp355);struct _tuple18 _tmp1B3=_tmp1B2;union Cyc_CfFlowInfo_FlowInfo _tmp1BE;union Cyc_CfFlowInfo_AbsLVal _tmp1BD;_LL1D2: _tmp1BE=_tmp1B3.f1;_tmp1BD=_tmp1B3.f2;_LL1D3:;{
struct Cyc_Dict_Dict _tmp1B4=*((struct Cyc_Dict_Dict*)_check_null(env->all_changed));
env->all_changed=({struct Cyc_Dict_Dict*_tmp1B5=_region_malloc(env->r,sizeof(*_tmp1B5));_tmp1B5[0]=_tmp15D->mt_place_set;_tmp1B5;});{
struct _tuple16 _tmp1B6=Cyc_NewControlFlow_anal_Rexp(env,1,inflow,_tmp354);struct _tuple16 _tmp1B7=_tmp1B6;union Cyc_CfFlowInfo_FlowInfo _tmp1BC;void*_tmp1BB;_LL1D5: _tmp1BC=_tmp1B7.f1;_tmp1BB=_tmp1B7.f2;_LL1D6:;{
struct Cyc_Dict_Dict _tmp1B8=*((struct Cyc_Dict_Dict*)_check_null(env->all_changed));
union Cyc_CfFlowInfo_FlowInfo _tmp1B9=Cyc_CfFlowInfo_after_flow(_tmp15D,& _tmp1B4,_tmp1BE,_tmp1BC,_tmp1B4,_tmp1B8);
# 1175
union Cyc_CfFlowInfo_FlowInfo _tmp1BA=Cyc_CfFlowInfo_join_flow(_tmp15D,_tmp1AF,inflow,_tmp1B9);
# 1178
if(Cyc_CfFlowInfo_flow_lessthan_approx(_tmp1BA,inflow)){
if(_tmp1AF == 0)
env->all_changed=0;else{
# 1182
*((struct Cyc_Dict_Dict*)_check_null(env->all_changed))=Cyc_CfFlowInfo_union_place_set(*_tmp1AF,
Cyc_CfFlowInfo_union_place_set(_tmp1B4,_tmp1B8,0),0);}
# 1185
return Cyc_NewControlFlow_do_assign(_tmp15D,env,_tmp1B9,_tmp355,_tmp1BD,_tmp354,_tmp1BB,e->loc);}
# 1188
inflow=_tmp1BA;};};};};}};}case 8: _LL173: _tmp357=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp175)->f1;_tmp356=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp175)->f2;_LL174: {
# 1193
struct _tuple16 _tmp1BF=Cyc_NewControlFlow_anal_Rexp(env,0,inflow,_tmp357);struct _tuple16 _tmp1C0=_tmp1BF;union Cyc_CfFlowInfo_FlowInfo _tmp1C2;void*_tmp1C1;_LL1D8: _tmp1C2=_tmp1C0.f1;_tmp1C1=_tmp1C0.f2;_LL1D9:;
return Cyc_NewControlFlow_anal_Rexp(env,copy_ctxt,_tmp1C2,_tmp356);}case 10: _LL175: _tmp358=((struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*)_tmp175)->f1;_LL176: {
# 1197
struct _tuple16 _tmp1C3=Cyc_NewControlFlow_anal_Rexp(env,1,inflow,_tmp358);struct _tuple16 _tmp1C4=_tmp1C3;union Cyc_CfFlowInfo_FlowInfo _tmp1C7;void*_tmp1C6;_LL1DB: _tmp1C7=_tmp1C4.f1;_tmp1C6=_tmp1C4.f2;_LL1DC:;
Cyc_NewControlFlow_use_Rval(env,_tmp358->loc,_tmp1C7,_tmp1C6);
return({struct _tuple16 _tmp1C5;_tmp1C5.f1=Cyc_CfFlowInfo_BottomFL();_tmp1C5.f2=Cyc_CfFlowInfo_typ_to_absrval(_tmp15D,(void*)_check_null(e->topt),0,_tmp15D->unknown_all);_tmp1C5;});}case 9: _LL177: _tmp35A=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp175)->f1;_tmp359=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp175)->f2;_LL178: {
# 1202
struct _RegionHandle _tmp1C8=_new_region("temp");struct _RegionHandle*temp=& _tmp1C8;_push_region(temp);
{struct Cyc_List_List*_tmp1C9=_tmp359;
struct _tuple23 _tmp1CA=Cyc_NewControlFlow_anal_unordered_Rexps(temp,env,inflow,({struct Cyc_List_List*_tmp1F7=_region_malloc(temp,sizeof(*_tmp1F7));_tmp1F7->hd=_tmp35A;_tmp1F7->tl=((struct Cyc_List_List*(*)(struct _RegionHandle*,struct Cyc_List_List*x))Cyc_List_rcopy)(temp,_tmp359);_tmp1F7;}),0,1);struct _tuple23 _tmp1CB=_tmp1CA;union Cyc_CfFlowInfo_FlowInfo _tmp1F6;struct Cyc_List_List*_tmp1F5;_LL1DE: _tmp1F6=_tmp1CB.f1;_tmp1F5=_tmp1CB.f2;_LL1DF:;{
# 1206
union Cyc_CfFlowInfo_FlowInfo _tmp1CC=Cyc_NewControlFlow_use_Rval(env,_tmp35A->loc,_tmp1F6,(void*)((struct Cyc_List_List*)_check_null(_tmp1F5))->hd);
_tmp1F5=_tmp1F5->tl;{
# 1209
struct Cyc_List_List*init_params=0;
struct Cyc_List_List*nolive_unique_params=0;
struct Cyc_List_List*noconsume_params=0;
struct Cyc_List_List*requires;
struct Cyc_List_List*ensures;
{void*_tmp1CD=Cyc_Tcutil_compress((void*)_check_null(_tmp35A->topt));void*_tmp1CE=_tmp1CD;void*_tmp1E0;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1CE)->tag == 5){_LL1E1: _tmp1E0=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1CE)->f1).elt_typ;_LL1E2:
# 1216
{void*_tmp1CF=Cyc_Tcutil_compress(_tmp1E0);void*_tmp1D0=_tmp1CF;struct Cyc_List_List*_tmp1DD;struct Cyc_List_List*_tmp1DC;struct Cyc_List_List*_tmp1DB;if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1D0)->tag == 9){_LL1E6: _tmp1DD=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1D0)->f1).attributes;_tmp1DC=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1D0)->f1).requires_relns;_tmp1DB=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1D0)->f1).ensures_relns;_LL1E7:
# 1218
 requires=_tmp1DC;
ensures=_tmp1DB;
for(0;_tmp1DD != 0;_tmp1DD=_tmp1DD->tl){
# 1222
void*_tmp1D1=(void*)_tmp1DD->hd;void*_tmp1D2=_tmp1D1;int _tmp1D8;int _tmp1D7;int _tmp1D6;switch(*((int*)_tmp1D2)){case 20: _LL1EB: _tmp1D6=((struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct*)_tmp1D2)->f1;_LL1EC:
# 1224
 init_params=({struct Cyc_List_List*_tmp1D3=_region_malloc(temp,sizeof(*_tmp1D3));_tmp1D3->hd=(void*)_tmp1D6;_tmp1D3->tl=init_params;_tmp1D3;});goto _LL1EA;case 21: _LL1ED: _tmp1D7=((struct Cyc_Absyn_Noliveunique_att_Absyn_Attribute_struct*)_tmp1D2)->f1;_LL1EE:
# 1226
 nolive_unique_params=({struct Cyc_List_List*_tmp1D4=_region_malloc(temp,sizeof(*_tmp1D4));_tmp1D4->hd=(void*)_tmp1D7;_tmp1D4->tl=nolive_unique_params;_tmp1D4;});
goto _LL1EA;case 22: _LL1EF: _tmp1D8=((struct Cyc_Absyn_Noconsume_att_Absyn_Attribute_struct*)_tmp1D2)->f1;_LL1F0:
# 1230
 noconsume_params=({struct Cyc_List_List*_tmp1D5=_region_malloc(temp,sizeof(*_tmp1D5));_tmp1D5->hd=(void*)_tmp1D8;_tmp1D5->tl=noconsume_params;_tmp1D5;});
goto _LL1EA;default: _LL1F1: _LL1F2:
 goto _LL1EA;}_LL1EA:;}
# 1234
goto _LL1E5;}else{_LL1E8: _LL1E9:
({void*_tmp1D9=0;((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(({const char*_tmp1DA="anal_Rexp: bad function type";_tag_dyneither(_tmp1DA,sizeof(char),29);}),_tag_dyneither(_tmp1D9,sizeof(void*),0));});}_LL1E5:;}
# 1237
goto _LL1E0;}else{_LL1E3: _LL1E4:
({void*_tmp1DE=0;((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(({const char*_tmp1DF="anal_Rexp: bad function type";_tag_dyneither(_tmp1DF,sizeof(char),29);}),_tag_dyneither(_tmp1DE,sizeof(void*),0));});}_LL1E0:;}
# 1240
{int i=1;for(0;_tmp1F5 != 0;(((_tmp1F5=_tmp1F5->tl,_tmp359=((struct Cyc_List_List*)_check_null(_tmp359))->tl)),++ i)){
if(((int(*)(struct Cyc_List_List*l,int x))Cyc_List_memq)(init_params,i)){
union Cyc_CfFlowInfo_FlowInfo _tmp1E1=_tmp1F6;struct Cyc_Dict_Dict _tmp1F0;if((_tmp1E1.BottomFL).tag == 1){_LL1F4: _LL1F5:
 goto _LL1F3;}else{_LL1F6: _tmp1F0=((_tmp1E1.ReachableFL).val).f1;_LL1F7:
# 1245
 if(Cyc_CfFlowInfo_initlevel(env->fenv,_tmp1F0,(void*)_tmp1F5->hd)== Cyc_CfFlowInfo_NoneIL)
({void*_tmp1E2=0;Cyc_CfFlowInfo_aerr(((struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(_tmp359))->hd)->loc,({const char*_tmp1E3="expression may not be initialized";_tag_dyneither(_tmp1E3,sizeof(char),34);}),_tag_dyneither(_tmp1E2,sizeof(void*),0));});
{union Cyc_CfFlowInfo_FlowInfo _tmp1E4=_tmp1CC;struct Cyc_Dict_Dict _tmp1EF;struct Cyc_List_List*_tmp1EE;if((_tmp1E4.BottomFL).tag == 1){_LL1F9: _LL1FA:
 goto _LL1F8;}else{_LL1FB: _tmp1EF=((_tmp1E4.ReachableFL).val).f1;_tmp1EE=((_tmp1E4.ReachableFL).val).f2;_LL1FC: {
# 1252
struct Cyc_Dict_Dict _tmp1E5=Cyc_CfFlowInfo_escape_deref(_tmp15D,_tmp1EF,env->all_changed,(void*)_tmp1F5->hd);
{void*_tmp1E6=(void*)_tmp1F5->hd;void*_tmp1E7=_tmp1E6;struct Cyc_CfFlowInfo_Place*_tmp1ED;if(((struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*)_tmp1E7)->tag == 5){_LL1FE: _tmp1ED=((struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*)_tmp1E7)->f1;_LL1FF:
# 1255
{void*_tmp1E8=Cyc_Tcutil_compress((void*)_check_null(((struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(_tmp359))->hd)->topt));void*_tmp1E9=_tmp1E8;void*_tmp1EC;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1E9)->tag == 5){_LL203: _tmp1EC=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1E9)->f1).elt_typ;_LL204:
# 1257
 _tmp1E5=Cyc_CfFlowInfo_assign_place(_tmp15D,((struct Cyc_Absyn_Exp*)_tmp359->hd)->loc,_tmp1E5,env->all_changed,_tmp1ED,
# 1259
Cyc_CfFlowInfo_typ_to_absrval(_tmp15D,_tmp1EC,0,_tmp15D->esc_all));
# 1262
goto _LL202;}else{_LL205: _LL206:
({void*_tmp1EA=0;((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(({const char*_tmp1EB="anal_Rexp:bad type for initialized arg";_tag_dyneither(_tmp1EB,sizeof(char),39);}),_tag_dyneither(_tmp1EA,sizeof(void*),0));});}_LL202:;}
# 1265
goto _LL1FD;}else{_LL200: _LL201:
 goto _LL1FD;}_LL1FD:;}
# 1268
_tmp1CC=Cyc_CfFlowInfo_ReachableFL(_tmp1E5,_tmp1EE);
goto _LL1F8;}}_LL1F8:;}
# 1271
goto _LL1F3;}_LL1F3:;}else{
# 1274
if(((int(*)(struct Cyc_List_List*l,int x))Cyc_List_memq)(nolive_unique_params,i))
# 1279
_tmp1CC=Cyc_NewControlFlow_use_nounique_Rval(env,((struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(_tmp359))->hd)->loc,(void*)_check_null(((struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(_tmp359))->hd)->topt),_tmp1CC,(void*)_tmp1F5->hd);else{
# 1289
_tmp1CC=Cyc_NewControlFlow_use_Rval(env,((struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(_tmp359))->hd)->loc,_tmp1CC,(void*)_tmp1F5->hd);
if(((int(*)(struct Cyc_List_List*l,int x))Cyc_List_memq)(noconsume_params,i) && 
Cyc_Tcutil_is_noalias_pointer((void*)_check_null(((struct Cyc_Absyn_Exp*)_tmp359->hd)->topt),0))
_tmp1CC=Cyc_NewControlFlow_restore_noconsume_arg(env,_tmp1CC,(struct Cyc_Absyn_Exp*)_tmp359->hd,((struct Cyc_Absyn_Exp*)_tmp359->hd)->loc,(void*)_tmp1F5->hd);}}}}
# 1296
Cyc_NewControlFlow_check_fn_requires(env,_tmp1CC,_tmp1C9,requires,e->loc);
# 1299
if(Cyc_Tcutil_is_noreturn((void*)_check_null(_tmp35A->topt))){
struct _tuple16 _tmp1F2=({struct _tuple16 _tmp1F1;_tmp1F1.f1=Cyc_CfFlowInfo_BottomFL();_tmp1F1.f2=Cyc_CfFlowInfo_typ_to_absrval(_tmp15D,(void*)_check_null(e->topt),0,_tmp15D->unknown_all);_tmp1F1;});_npop_handler(0);return _tmp1F2;}else{
# 1302
struct _tuple16 _tmp1F4=({struct _tuple16 _tmp1F3;_tmp1F3.f1=_tmp1CC;_tmp1F3.f2=Cyc_CfFlowInfo_typ_to_absrval(_tmp15D,(void*)_check_null(e->topt),0,_tmp15D->unknown_all);_tmp1F3;});_npop_handler(0);return _tmp1F4;}};};}
# 1203
;_pop_region(temp);}case 33: _LL179: _tmp35F=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp175)->f1).is_calloc;_tmp35E=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp175)->f1).rgn;_tmp35D=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp175)->f1).elt_type;_tmp35C=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp175)->f1).num_elts;_tmp35B=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp175)->f1).fat_result;_LL17A: {
# 1305
void*root=(void*)({struct Cyc_CfFlowInfo_MallocPt_CfFlowInfo_Root_struct*_tmp209=_region_malloc(_tmp15D->r,sizeof(*_tmp209));_tmp209[0]=({struct Cyc_CfFlowInfo_MallocPt_CfFlowInfo_Root_struct _tmp20A;_tmp20A.tag=1;_tmp20A.f1=_tmp35C;_tmp20A.f2=(void*)_check_null(e->topt);_tmp20A;});_tmp209;});
struct Cyc_CfFlowInfo_Place*place=({struct Cyc_CfFlowInfo_Place*_tmp208=_region_malloc(_tmp15D->r,sizeof(*_tmp208));_tmp208->root=root;_tmp208->fields=0;_tmp208;});
void*rval=(void*)({struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*_tmp206=_region_malloc(_tmp15D->r,sizeof(*_tmp206));_tmp206[0]=({struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct _tmp207;_tmp207.tag=5;_tmp207.f1=place;_tmp207;});_tmp206;});
void*place_val;
if(_tmp35B)place_val=_tmp15D->notzeroall;else{
if(_tmp35F)place_val=Cyc_CfFlowInfo_typ_to_absrval(_tmp15D,*((void**)_check_null(_tmp35D)),0,_tmp15D->zero);else{
place_val=Cyc_CfFlowInfo_typ_to_absrval(_tmp15D,*((void**)_check_null(_tmp35D)),0,_tmp15D->unknown_none);}}{
union Cyc_CfFlowInfo_FlowInfo outflow;
((int(*)(struct Cyc_Dict_Dict*set,struct Cyc_CfFlowInfo_Place*place,unsigned int loc))Cyc_CfFlowInfo_update_place_set)(env->all_changed,place,0);
if(_tmp35E != 0){
struct _RegionHandle _tmp1F8=_new_region("temp");struct _RegionHandle*temp=& _tmp1F8;_push_region(temp);
{struct _tuple23 _tmp1F9=Cyc_NewControlFlow_anal_unordered_Rexps(temp,env,inflow,({struct Cyc_Absyn_Exp*_tmp1FD[2];_tmp1FD[1]=_tmp35C;_tmp1FD[0]=_tmp35E;((struct Cyc_List_List*(*)(struct _RegionHandle*,struct _dyneither_ptr))Cyc_List_rlist)(temp,_tag_dyneither(_tmp1FD,sizeof(struct Cyc_Absyn_Exp*),2));}),1,1);struct _tuple23 _tmp1FA=_tmp1F9;union Cyc_CfFlowInfo_FlowInfo _tmp1FC;struct Cyc_List_List*_tmp1FB;_LL208: _tmp1FC=_tmp1FA.f1;_tmp1FB=_tmp1FA.f2;_LL209:;
# 1318
outflow=_tmp1FC;}
# 1316
;_pop_region(temp);}else{
# 1321
struct _tuple16 _tmp1FE=Cyc_NewControlFlow_anal_Rexp(env,1,inflow,_tmp35C);struct _tuple16 _tmp1FF=_tmp1FE;union Cyc_CfFlowInfo_FlowInfo _tmp200;_LL20B: _tmp200=_tmp1FF.f1;_LL20C:;
outflow=_tmp200;}{
# 1324
union Cyc_CfFlowInfo_FlowInfo _tmp201=outflow;struct Cyc_Dict_Dict _tmp205;struct Cyc_List_List*_tmp204;if((_tmp201.BottomFL).tag == 1){_LL20E: _LL20F:
 return({struct _tuple16 _tmp202;_tmp202.f1=outflow;_tmp202.f2=rval;_tmp202;});}else{_LL210: _tmp205=((_tmp201.ReachableFL).val).f1;_tmp204=((_tmp201.ReachableFL).val).f2;_LL211:
# 1327
 return({struct _tuple16 _tmp203;_tmp203.f1=Cyc_CfFlowInfo_ReachableFL(((struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,void*k,void*v))Cyc_Dict_insert)(_tmp205,root,place_val),_tmp204);_tmp203.f2=rval;_tmp203;});}_LL20D:;};};}case 34: _LL17B: _tmp361=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_tmp175)->f1;_tmp360=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_tmp175)->f2;_LL17C: {
# 1331
void*left_rval;
void*right_rval;
union Cyc_CfFlowInfo_FlowInfo outflow;
# 1337
struct _RegionHandle _tmp20B=_new_region("temp");struct _RegionHandle*temp=& _tmp20B;_push_region(temp);{
struct _tuple23 _tmp20C=Cyc_NewControlFlow_anal_unordered_Rexps(temp,env,inflow,({struct Cyc_Absyn_Exp*_tmp210[2];_tmp210[1]=_tmp360;_tmp210[0]=_tmp361;((struct Cyc_List_List*(*)(struct _RegionHandle*,struct _dyneither_ptr))Cyc_List_rlist)(temp,_tag_dyneither(_tmp210,sizeof(struct Cyc_Absyn_Exp*),2));}),0,0);struct _tuple23 _tmp20D=_tmp20C;union Cyc_CfFlowInfo_FlowInfo _tmp20F;struct Cyc_List_List*_tmp20E;_LL213: _tmp20F=_tmp20D.f1;_tmp20E=_tmp20D.f2;_LL214:;
# 1340
left_rval=(void*)((struct Cyc_List_List*)_check_null(_tmp20E))->hd;
right_rval=(void*)((struct Cyc_List_List*)_check_null(_tmp20E->tl))->hd;
outflow=_tmp20F;}{
# 1345
struct _tuple18 _tmp211=Cyc_NewControlFlow_anal_Lexp(env,outflow,0,0,_tmp361);struct _tuple18 _tmp212=_tmp211;union Cyc_CfFlowInfo_AbsLVal _tmp223;_LL216: _tmp223=_tmp212.f2;_LL217:;{
struct _tuple18 _tmp213=Cyc_NewControlFlow_anal_Lexp(env,outflow,0,0,_tmp360);struct _tuple18 _tmp214=_tmp213;union Cyc_CfFlowInfo_AbsLVal _tmp222;_LL219: _tmp222=_tmp214.f2;_LL21A:;
{union Cyc_CfFlowInfo_FlowInfo _tmp215=outflow;struct Cyc_Dict_Dict _tmp21F;struct Cyc_List_List*_tmp21E;if((_tmp215.ReachableFL).tag == 2){_LL21C: _tmp21F=((_tmp215.ReachableFL).val).f1;_tmp21E=((_tmp215.ReachableFL).val).f2;_LL21D:
# 1349
{union Cyc_CfFlowInfo_AbsLVal _tmp216=_tmp223;struct Cyc_CfFlowInfo_Place*_tmp219;if((_tmp216.PlaceL).tag == 1){_LL221: _tmp219=(_tmp216.PlaceL).val;_LL222:
# 1351
 _tmp21F=Cyc_CfFlowInfo_assign_place(_tmp15D,_tmp361->loc,_tmp21F,env->all_changed,_tmp219,right_rval);
# 1353
goto _LL220;}else{_LL223: _LL224:
# 1358
 if(Cyc_CfFlowInfo_initlevel(_tmp15D,_tmp21F,right_rval)!= Cyc_CfFlowInfo_AllIL)
({void*_tmp217=0;Cyc_Tcutil_terr(_tmp360->loc,({const char*_tmp218="expression may not be fully initialized";_tag_dyneither(_tmp218,sizeof(char),40);}),_tag_dyneither(_tmp217,sizeof(void*),0));});
goto _LL220;}_LL220:;}
# 1362
{union Cyc_CfFlowInfo_AbsLVal _tmp21A=_tmp222;struct Cyc_CfFlowInfo_Place*_tmp21D;if((_tmp21A.PlaceL).tag == 1){_LL226: _tmp21D=(_tmp21A.PlaceL).val;_LL227:
# 1364
 _tmp21F=Cyc_CfFlowInfo_assign_place(_tmp15D,_tmp360->loc,_tmp21F,env->all_changed,_tmp21D,left_rval);
# 1366
goto _LL225;}else{_LL228: _LL229:
# 1368
 if(Cyc_CfFlowInfo_initlevel(_tmp15D,_tmp21F,left_rval)!= Cyc_CfFlowInfo_AllIL)
({void*_tmp21B=0;Cyc_Tcutil_terr(_tmp361->loc,({const char*_tmp21C="expression may not be fully initialized";_tag_dyneither(_tmp21C,sizeof(char),40);}),_tag_dyneither(_tmp21B,sizeof(void*),0));});
goto _LL225;}_LL225:;}
# 1373
_tmp21E=Cyc_Relations_reln_kill_exp(_tmp15D->r,_tmp21E,_tmp361);
_tmp21E=Cyc_Relations_reln_kill_exp(_tmp15D->r,_tmp21E,_tmp360);
# 1376
outflow=Cyc_CfFlowInfo_ReachableFL(_tmp21F,_tmp21E);
Cyc_NewControlFlow_update_tryflow(env,outflow);
goto _LL21B;}else{_LL21E: _LL21F:
 goto _LL21B;}_LL21B:;}{
# 1383
struct _tuple16 _tmp221=({struct _tuple16 _tmp220;_tmp220.f1=outflow;_tmp220.f2=_tmp15D->unknown_all;_tmp220;});_npop_handler(0);return _tmp221;};};};
# 1337
;_pop_region(temp);}case 15: _LL17D: _tmp363=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmp175)->f1;_tmp362=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmp175)->f2;_LL17E: {
# 1386
void*root=(void*)({struct Cyc_CfFlowInfo_MallocPt_CfFlowInfo_Root_struct*_tmp236=_region_malloc(_tmp15D->r,sizeof(*_tmp236));_tmp236[0]=({struct Cyc_CfFlowInfo_MallocPt_CfFlowInfo_Root_struct _tmp237;_tmp237.tag=1;_tmp237.f1=_tmp362;_tmp237.f2=(void*)_check_null(e->topt);_tmp237;});_tmp236;});
struct Cyc_CfFlowInfo_Place*place=({struct Cyc_CfFlowInfo_Place*_tmp235=_region_malloc(_tmp15D->r,sizeof(*_tmp235));_tmp235->root=root;_tmp235->fields=0;_tmp235;});
void*rval=(void*)({struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*_tmp233=_region_malloc(_tmp15D->r,sizeof(*_tmp233));_tmp233[0]=({struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct _tmp234;_tmp234.tag=5;_tmp234.f1=place;_tmp234;});_tmp233;});
((int(*)(struct Cyc_Dict_Dict*set,struct Cyc_CfFlowInfo_Place*place,unsigned int loc))Cyc_CfFlowInfo_update_place_set)(env->all_changed,place,0);{
union Cyc_CfFlowInfo_FlowInfo outflow;
void*place_val;
if(_tmp363 != 0){
struct _RegionHandle _tmp224=_new_region("temp");struct _RegionHandle*temp=& _tmp224;_push_region(temp);
{struct _tuple23 _tmp225=Cyc_NewControlFlow_anal_unordered_Rexps(temp,env,inflow,({struct Cyc_Absyn_Exp*_tmp229[2];_tmp229[1]=_tmp362;_tmp229[0]=_tmp363;((struct Cyc_List_List*(*)(struct _RegionHandle*,struct _dyneither_ptr))Cyc_List_rlist)(temp,_tag_dyneither(_tmp229,sizeof(struct Cyc_Absyn_Exp*),2));}),1,1);struct _tuple23 _tmp226=_tmp225;union Cyc_CfFlowInfo_FlowInfo _tmp228;struct Cyc_List_List*_tmp227;_LL22B: _tmp228=_tmp226.f1;_tmp227=_tmp226.f2;_LL22C:;
# 1396
outflow=_tmp228;
place_val=(void*)((struct Cyc_List_List*)_check_null(((struct Cyc_List_List*)_check_null(_tmp227))->tl))->hd;}
# 1394
;_pop_region(temp);}else{
# 1400
struct _tuple16 _tmp22A=Cyc_NewControlFlow_anal_Rexp(env,1,inflow,_tmp362);struct _tuple16 _tmp22B=_tmp22A;union Cyc_CfFlowInfo_FlowInfo _tmp22D;void*_tmp22C;_LL22E: _tmp22D=_tmp22B.f1;_tmp22C=_tmp22B.f2;_LL22F:;
outflow=_tmp22D;
place_val=_tmp22C;}{
# 1404
union Cyc_CfFlowInfo_FlowInfo _tmp22E=outflow;struct Cyc_Dict_Dict _tmp232;struct Cyc_List_List*_tmp231;if((_tmp22E.BottomFL).tag == 1){_LL231: _LL232:
 return({struct _tuple16 _tmp22F;_tmp22F.f1=outflow;_tmp22F.f2=rval;_tmp22F;});}else{_LL233: _tmp232=((_tmp22E.ReachableFL).val).f1;_tmp231=((_tmp22E.ReachableFL).val).f2;_LL234:
# 1407
 return({struct _tuple16 _tmp230;_tmp230.f1=Cyc_CfFlowInfo_ReachableFL(((struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,void*k,void*v))Cyc_Dict_insert)(_tmp232,root,place_val),_tmp231);_tmp230.f2=rval;_tmp230;});}_LL230:;};};}case 14: _LL17F: _tmp364=((struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_tmp175)->f1;_LL180: {
# 1411
struct _tuple18 _tmp238=Cyc_NewControlFlow_anal_Lexp(env,inflow,0,0,_tmp364);struct _tuple18 _tmp239=_tmp238;union Cyc_CfFlowInfo_FlowInfo _tmp241;union Cyc_CfFlowInfo_AbsLVal _tmp240;_LL236: _tmp241=_tmp239.f1;_tmp240=_tmp239.f2;_LL237:;{
union Cyc_CfFlowInfo_AbsLVal _tmp23A=_tmp240;struct Cyc_CfFlowInfo_Place*_tmp23F;if((_tmp23A.UnknownL).tag == 2){_LL239: _LL23A:
 return({struct _tuple16 _tmp23B;_tmp23B.f1=_tmp241;_tmp23B.f2=_tmp15D->notzeroall;_tmp23B;});}else{_LL23B: _tmp23F=(_tmp23A.PlaceL).val;_LL23C:
 return({struct _tuple16 _tmp23C;_tmp23C.f1=_tmp241;_tmp23C.f2=(void*)({struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*_tmp23D=_region_malloc(env->r,sizeof(*_tmp23D));_tmp23D[0]=({struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct _tmp23E;_tmp23E.tag=5;_tmp23E.f1=_tmp23F;_tmp23E;});_tmp23D;});_tmp23C;});}_LL238:;};}case 19: _LL181: _tmp365=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp175)->f1;_LL182: {
# 1418
struct _tuple16 _tmp242=Cyc_NewControlFlow_anal_Rexp(env,0,inflow,_tmp365);struct _tuple16 _tmp243=_tmp242;union Cyc_CfFlowInfo_FlowInfo _tmp245;void*_tmp244;_LL23E: _tmp245=_tmp243.f1;_tmp244=_tmp243.f2;_LL23F:;
return Cyc_NewControlFlow_anal_derefR(env,inflow,_tmp245,_tmp365,_tmp244);}case 20: _LL183: _tmp367=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp175)->f1;_tmp366=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp175)->f2;_LL184: {
# 1422
struct _tuple16 _tmp246=Cyc_NewControlFlow_anal_Rexp(env,0,inflow,_tmp367);struct _tuple16 _tmp247=_tmp246;union Cyc_CfFlowInfo_FlowInfo _tmp264;void*_tmp263;_LL241: _tmp264=_tmp247.f1;_tmp263=_tmp247.f2;_LL242:;
if(_tmp367->topt == 0){
({struct Cyc_String_pa_PrintArg_struct _tmp24A;_tmp24A.tag=0;_tmp24A.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_exp2string(e));({void*_tmp248[1]={& _tmp24A};Cyc_fprintf(Cyc_stderr,({const char*_tmp249="aggrmember exp %s\n";_tag_dyneither(_tmp249,sizeof(char),19);}),_tag_dyneither(_tmp248,sizeof(void*),1));});});
({struct Cyc_String_pa_PrintArg_struct _tmp24D;_tmp24D.tag=0;_tmp24D.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_exp2string(_tmp367));({void*_tmp24B[1]={& _tmp24D};Cyc_fprintf(Cyc_stderr,({const char*_tmp24C="oops! %s.topt = null!\n";_tag_dyneither(_tmp24C,sizeof(char),23);}),_tag_dyneither(_tmp24B,sizeof(void*),1));});});}{
# 1427
void*_tmp24E=(void*)_check_null(_tmp367->topt);
if(Cyc_Absyn_is_nontagged_nonrequire_union_type(_tmp24E))
# 1430
return({struct _tuple16 _tmp24F;_tmp24F.f1=_tmp264;_tmp24F.f2=Cyc_CfFlowInfo_typ_to_absrval(_tmp15D,(void*)_check_null(e->topt),0,_tmp15D->unknown_all);_tmp24F;});
# 1432
if(Cyc_Absyn_is_require_union_type(_tmp24E))
Cyc_NewControlFlow_check_union_requires(_tmp367->loc,_tmp15D->r,_tmp24E,_tmp366,_tmp264);{
# 1435
struct _tuple15 _tmp250=Cyc_CfFlowInfo_unname_rval(_tmp15D->r,_tmp263);struct _tuple15 _tmp251=_tmp250;void*_tmp262;_LL244: _tmp262=_tmp251.f1;_LL245:;{
void*_tmp252=_tmp262;int _tmp261;int _tmp260;struct _dyneither_ptr _tmp25F;if(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp252)->tag == 6){_LL247: _tmp261=(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp252)->f1).is_union;_tmp260=(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp252)->f1).fieldnum;_tmp25F=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp252)->f2;_LL248: {
# 1438
int _tmp253=Cyc_CfFlowInfo_get_field_index((void*)_check_null(_tmp367->topt),_tmp366);
if((_tmp261  && _tmp260 != - 1) && _tmp260 != _tmp253)
return({struct _tuple16 _tmp254;_tmp254.f1=_tmp264;_tmp254.f2=Cyc_CfFlowInfo_typ_to_absrval(_tmp15D,(void*)_check_null(e->topt),1,_tmp15D->unknown_none);_tmp254;});
return({struct _tuple16 _tmp255;_tmp255.f1=_tmp264;_tmp255.f2=*((void**)_check_dyneither_subscript(_tmp25F,sizeof(void*),_tmp253));_tmp255;});}}else{_LL249: _LL24A:
# 1443
({void*_tmp256=0;Cyc_fprintf(Cyc_stderr,({const char*_tmp257="the bad rexp is :";_tag_dyneither(_tmp257,sizeof(char),18);}),_tag_dyneither(_tmp256,sizeof(void*),0));});
Cyc_CfFlowInfo_print_absrval(_tmp262);
({void*_tmp258=0;Cyc_fprintf(Cyc_stderr,({const char*_tmp259="\n";_tag_dyneither(_tmp259,sizeof(char),2);}),_tag_dyneither(_tmp258,sizeof(void*),0));});
(int)_throw((void*)({struct Cyc_Core_Impossible_exn_struct*_tmp25A=_cycalloc(sizeof(*_tmp25A));_tmp25A[0]=({struct Cyc_Core_Impossible_exn_struct _tmp25B;_tmp25B.tag=Cyc_Core_Impossible;_tmp25B.f1=(struct _dyneither_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp25E;_tmp25E.tag=0;_tmp25E.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_exp2string(e));({void*_tmp25C[1]={& _tmp25E};Cyc_aprintf(({const char*_tmp25D="anal_Rexp: AggrMember: %s";_tag_dyneither(_tmp25D,sizeof(char),26);}),_tag_dyneither(_tmp25C,sizeof(void*),1));});});_tmp25B;});_tmp25A;}));}_LL246:;};};};}case 37: _LL185: _tmp369=((struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_tmp175)->f1;_tmp368=((struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_tmp175)->f2;_LL186: {
# 1453
struct _tuple16 _tmp265=Cyc_NewControlFlow_anal_Rexp(env,0,inflow,_tmp369);struct _tuple16 _tmp266=_tmp265;union Cyc_CfFlowInfo_FlowInfo _tmp279;void*_tmp278;_LL24C: _tmp279=_tmp266.f1;_tmp278=_tmp266.f2;_LL24D:;
# 1455
if(Cyc_Absyn_is_nontagged_nonrequire_union_type((void*)_check_null(_tmp369->topt)))
return({struct _tuple16 _tmp267;_tmp267.f1=_tmp279;_tmp267.f2=_tmp15D->unknown_all;_tmp267;});{
struct _tuple15 _tmp268=Cyc_CfFlowInfo_unname_rval(_tmp15D->r,_tmp278);struct _tuple15 _tmp269=_tmp268;void*_tmp277;_LL24F: _tmp277=_tmp269.f1;_LL250:;{
void*_tmp26A=_tmp277;int _tmp276;int _tmp275;struct _dyneither_ptr _tmp274;if(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp26A)->tag == 6){_LL252: _tmp276=(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp26A)->f1).is_union;_tmp275=(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp26A)->f1).fieldnum;_tmp274=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp26A)->f2;_LL253: {
# 1460
int _tmp26B=Cyc_CfFlowInfo_get_field_index((void*)_check_null(_tmp369->topt),_tmp368);
if(_tmp276  && _tmp275 != - 1){
if(_tmp275 != _tmp26B)
return({struct _tuple16 _tmp26C;_tmp26C.f1=_tmp279;_tmp26C.f2=_tmp15D->zero;_tmp26C;});else{
# 1465
return({struct _tuple16 _tmp26D;_tmp26D.f1=_tmp279;_tmp26D.f2=_tmp15D->notzeroall;_tmp26D;});}}else{
# 1467
return({struct _tuple16 _tmp26E;_tmp26E.f1=_tmp279;_tmp26E.f2=_tmp15D->unknown_all;_tmp26E;});}}}else{_LL254: _LL255:
# 1469
(int)_throw((void*)({struct Cyc_Core_Impossible_exn_struct*_tmp26F=_cycalloc(sizeof(*_tmp26F));_tmp26F[0]=({struct Cyc_Core_Impossible_exn_struct _tmp270;_tmp270.tag=Cyc_Core_Impossible;_tmp270.f1=(struct _dyneither_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp273;_tmp273.tag=0;_tmp273.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_exp2string(e));({void*_tmp271[1]={& _tmp273};Cyc_aprintf(({const char*_tmp272="anal_Rexp: TagCheck_e: %s";_tag_dyneither(_tmp272,sizeof(char),26);}),_tag_dyneither(_tmp271,sizeof(void*),1));});});_tmp270;});_tmp26F;}));}_LL251:;};};}case 21: _LL187: _tmp36B=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp175)->f1;_tmp36A=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp175)->f2;_LL188: {
# 1474
struct _tuple16 _tmp27A=Cyc_NewControlFlow_anal_Rexp(env,0,inflow,_tmp36B);struct _tuple16 _tmp27B=_tmp27A;union Cyc_CfFlowInfo_FlowInfo _tmp295;void*_tmp294;_LL257: _tmp295=_tmp27B.f1;_tmp294=_tmp27B.f2;_LL258:;{
# 1477
struct _tuple16 _tmp27C=Cyc_NewControlFlow_anal_derefR(env,inflow,_tmp295,_tmp36B,_tmp294);struct _tuple16 _tmp27D=_tmp27C;union Cyc_CfFlowInfo_FlowInfo _tmp293;void*_tmp292;_LL25A: _tmp293=_tmp27D.f1;_tmp292=_tmp27D.f2;_LL25B:;{
# 1480
void*_tmp27E=Cyc_Tcutil_compress((void*)_check_null(_tmp36B->topt));void*_tmp27F=_tmp27E;void*_tmp291;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp27F)->tag == 5){_LL25D: _tmp291=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp27F)->f1).elt_typ;_LL25E:
# 1482
 if(Cyc_Absyn_is_nontagged_nonrequire_union_type(_tmp291))
# 1484
return({struct _tuple16 _tmp280;_tmp280.f1=_tmp293;_tmp280.f2=Cyc_CfFlowInfo_typ_to_absrval(_tmp15D,(void*)_check_null(e->topt),0,_tmp15D->unknown_all);_tmp280;});
# 1486
if(Cyc_Absyn_is_require_union_type(_tmp291))
Cyc_NewControlFlow_check_union_requires(_tmp36B->loc,_tmp15D->r,_tmp291,_tmp36A,_tmp295);{
# 1489
struct _tuple15 _tmp281=Cyc_CfFlowInfo_unname_rval(_tmp15D->r,_tmp292);struct _tuple15 _tmp282=_tmp281;void*_tmp28D;_LL262: _tmp28D=_tmp282.f1;_LL263:;{
void*_tmp283=_tmp28D;int _tmp28C;int _tmp28B;struct _dyneither_ptr _tmp28A;if(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp283)->tag == 6){_LL265: _tmp28C=(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp283)->f1).is_union;_tmp28B=(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp283)->f1).fieldnum;_tmp28A=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp283)->f2;_LL266: {
# 1492
int _tmp284=Cyc_CfFlowInfo_get_field_index(_tmp291,_tmp36A);
if((_tmp28C  && _tmp28B != - 1) && _tmp28B != _tmp284)
return({struct _tuple16 _tmp285;_tmp285.f1=_tmp293;_tmp285.f2=Cyc_CfFlowInfo_typ_to_absrval(_tmp15D,(void*)_check_null(e->topt),1,_tmp15D->unknown_none);_tmp285;});
return({struct _tuple16 _tmp286;_tmp286.f1=_tmp293;_tmp286.f2=*((void**)_check_dyneither_subscript(_tmp28A,sizeof(void*),_tmp284));_tmp286;});}}else{_LL267: _LL268:
(int)_throw((void*)({struct Cyc_Core_Impossible_exn_struct*_tmp287=_cycalloc(sizeof(*_tmp287));_tmp287[0]=({struct Cyc_Core_Impossible_exn_struct _tmp288;_tmp288.tag=Cyc_Core_Impossible;_tmp288.f1=({const char*_tmp289="anal_Rexp: AggrArrow";_tag_dyneither(_tmp289,sizeof(char),21);});_tmp288;});_tmp287;}));}_LL264:;};};}else{_LL25F: _LL260:
# 1498
(int)_throw((void*)({struct Cyc_Core_Impossible_exn_struct*_tmp28E=_cycalloc(sizeof(*_tmp28E));_tmp28E[0]=({struct Cyc_Core_Impossible_exn_struct _tmp28F;_tmp28F.tag=Cyc_Core_Impossible;_tmp28F.f1=({const char*_tmp290="anal_Rexp: AggrArrow ptr";_tag_dyneither(_tmp290,sizeof(char),25);});_tmp28F;});_tmp28E;}));}_LL25C:;};};}case 5: _LL189: _tmp36E=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp175)->f1;_tmp36D=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp175)->f2;_tmp36C=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp175)->f3;_LL18A: {
# 1502
struct _tuple19 _tmp296=Cyc_NewControlFlow_anal_test(env,inflow,_tmp36E);struct _tuple19 _tmp297=_tmp296;union Cyc_CfFlowInfo_FlowInfo _tmp29B;union Cyc_CfFlowInfo_FlowInfo _tmp29A;_LL26A: _tmp29B=_tmp297.f1;_tmp29A=_tmp297.f2;_LL26B:;{
struct _tuple16 _tmp298=Cyc_NewControlFlow_anal_Rexp(env,copy_ctxt,_tmp29B,_tmp36D);
struct _tuple16 _tmp299=Cyc_NewControlFlow_anal_Rexp(env,copy_ctxt,_tmp29A,_tmp36C);
# 1506
return Cyc_CfFlowInfo_join_flow_and_rval(_tmp15D,env->all_changed,_tmp298,_tmp299);};}case 6: _LL18B: _tmp370=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp175)->f1;_tmp36F=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp175)->f2;_LL18C: {
# 1509
struct _tuple19 _tmp29C=Cyc_NewControlFlow_anal_test(env,inflow,_tmp370);struct _tuple19 _tmp29D=_tmp29C;union Cyc_CfFlowInfo_FlowInfo _tmp2A7;union Cyc_CfFlowInfo_FlowInfo _tmp2A6;_LL26D: _tmp2A7=_tmp29D.f1;_tmp2A6=_tmp29D.f2;_LL26E:;{
struct _tuple16 _tmp29E=Cyc_NewControlFlow_anal_Rexp(env,copy_ctxt,_tmp2A7,_tmp36F);struct _tuple16 _tmp29F=_tmp29E;union Cyc_CfFlowInfo_FlowInfo _tmp2A5;void*_tmp2A4;_LL270: _tmp2A5=_tmp29F.f1;_tmp2A4=_tmp29F.f2;_LL271:;{
struct _tuple16 _tmp2A0=({struct _tuple16 _tmp2A3;_tmp2A3.f1=_tmp2A5;_tmp2A3.f2=_tmp2A4;_tmp2A3;});
struct _tuple16 _tmp2A1=({struct _tuple16 _tmp2A2;_tmp2A2.f1=_tmp2A6;_tmp2A2.f2=_tmp15D->zero;_tmp2A2;});
return Cyc_CfFlowInfo_join_flow_and_rval(_tmp15D,env->all_changed,_tmp2A0,_tmp2A1);};};}case 7: _LL18D: _tmp372=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp175)->f1;_tmp371=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp175)->f2;_LL18E: {
# 1516
struct _tuple19 _tmp2A8=Cyc_NewControlFlow_anal_test(env,inflow,_tmp372);struct _tuple19 _tmp2A9=_tmp2A8;union Cyc_CfFlowInfo_FlowInfo _tmp2B3;union Cyc_CfFlowInfo_FlowInfo _tmp2B2;_LL273: _tmp2B3=_tmp2A9.f1;_tmp2B2=_tmp2A9.f2;_LL274:;{
struct _tuple16 _tmp2AA=Cyc_NewControlFlow_anal_Rexp(env,copy_ctxt,_tmp2B2,_tmp371);struct _tuple16 _tmp2AB=_tmp2AA;union Cyc_CfFlowInfo_FlowInfo _tmp2B1;void*_tmp2B0;_LL276: _tmp2B1=_tmp2AB.f1;_tmp2B0=_tmp2AB.f2;_LL277:;{
struct _tuple16 _tmp2AC=({struct _tuple16 _tmp2AF;_tmp2AF.f1=_tmp2B1;_tmp2AF.f2=_tmp2B0;_tmp2AF;});
struct _tuple16 _tmp2AD=({struct _tuple16 _tmp2AE;_tmp2AE.f1=_tmp2B3;_tmp2AE.f2=_tmp15D->notzeroall;_tmp2AE;});
return Cyc_CfFlowInfo_join_flow_and_rval(_tmp15D,env->all_changed,_tmp2AC,_tmp2AD);};};}case 22: _LL18F: _tmp374=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp175)->f1;_tmp373=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp175)->f2;_LL190: {
# 1523
struct _RegionHandle _tmp2B4=_new_region("temp");struct _RegionHandle*temp=& _tmp2B4;_push_region(temp);
{struct _tuple23 _tmp2B5=Cyc_NewControlFlow_anal_unordered_Rexps(temp,env,inflow,({struct Cyc_Absyn_Exp*_tmp2DA[2];_tmp2DA[1]=_tmp373;_tmp2DA[0]=_tmp374;((struct Cyc_List_List*(*)(struct _RegionHandle*,struct _dyneither_ptr))Cyc_List_rlist)(temp,_tag_dyneither(_tmp2DA,sizeof(struct Cyc_Absyn_Exp*),2));}),0,1);struct _tuple23 _tmp2B6=_tmp2B5;union Cyc_CfFlowInfo_FlowInfo _tmp2D9;struct Cyc_List_List*_tmp2D8;_LL279: _tmp2D9=_tmp2B6.f1;_tmp2D8=_tmp2B6.f2;_LL27A:;{
# 1529
union Cyc_CfFlowInfo_FlowInfo _tmp2B7=_tmp2D9;
{union Cyc_CfFlowInfo_FlowInfo _tmp2B8=_tmp2D9;struct Cyc_Dict_Dict _tmp2BD;struct Cyc_List_List*_tmp2BC;if((_tmp2B8.ReachableFL).tag == 2){_LL27C: _tmp2BD=((_tmp2B8.ReachableFL).val).f1;_tmp2BC=((_tmp2B8.ReachableFL).val).f2;_LL27D:
# 1534
 if(Cyc_CfFlowInfo_initlevel(env->fenv,_tmp2BD,(void*)((struct Cyc_List_List*)_check_null(((struct Cyc_List_List*)_check_null(_tmp2D8))->tl))->hd)== Cyc_CfFlowInfo_NoneIL)
({void*_tmp2B9=0;Cyc_CfFlowInfo_aerr(_tmp373->loc,({const char*_tmp2BA="expression may not be initialized";_tag_dyneither(_tmp2BA,sizeof(char),34);}),_tag_dyneither(_tmp2B9,sizeof(void*),0));});{
struct Cyc_List_List*_tmp2BB=Cyc_NewControlFlow_add_subscript_reln(_tmp15D->r,_tmp2BC,_tmp374,_tmp373);
if(_tmp2BC != _tmp2BB)
_tmp2B7=Cyc_CfFlowInfo_ReachableFL(_tmp2BD,_tmp2BB);
goto _LL27B;};}else{_LL27E: _LL27F:
 goto _LL27B;}_LL27B:;}{
# 1542
void*_tmp2BE=Cyc_Tcutil_compress((void*)_check_null(_tmp374->topt));void*_tmp2BF=_tmp2BE;union Cyc_Absyn_Constraint*_tmp2D7;struct Cyc_List_List*_tmp2D6;switch(*((int*)_tmp2BF)){case 10: _LL281: _tmp2D6=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp2BF)->f1;_LL282: {
# 1544
struct _tuple15 _tmp2C0=Cyc_CfFlowInfo_unname_rval(_tmp15D->r,(void*)((struct Cyc_List_List*)_check_null(_tmp2D8))->hd);struct _tuple15 _tmp2C1=_tmp2C0;void*_tmp2C9;_LL288: _tmp2C9=_tmp2C1.f1;_LL289:;{
void*_tmp2C2=_tmp2C9;struct _dyneither_ptr _tmp2C8;if(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp2C2)->tag == 6){_LL28B: _tmp2C8=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp2C2)->f2;_LL28C: {
# 1547
unsigned int i=(Cyc_Evexp_eval_const_uint_exp(_tmp373)).f1;
struct _tuple16 _tmp2C4=({struct _tuple16 _tmp2C3;_tmp2C3.f1=_tmp2B7;_tmp2C3.f2=*((void**)_check_dyneither_subscript(_tmp2C8,sizeof(void*),(int)i));_tmp2C3;});_npop_handler(0);return _tmp2C4;}}else{_LL28D: _LL28E:
(int)_throw((void*)({struct Cyc_Core_Impossible_exn_struct*_tmp2C5=_cycalloc(sizeof(*_tmp2C5));_tmp2C5[0]=({struct Cyc_Core_Impossible_exn_struct _tmp2C6;_tmp2C6.tag=Cyc_Core_Impossible;_tmp2C6.f1=({const char*_tmp2C7="anal_Rexp: Subscript";_tag_dyneither(_tmp2C7,sizeof(char),21);});_tmp2C6;});_tmp2C5;}));}_LL28A:;};}case 5: _LL283: _tmp2D7=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2BF)->f1).ptr_atts).bounds;_LL284: {
# 1552
struct _tuple16 _tmp2CA=Cyc_NewControlFlow_anal_derefR(env,inflow,_tmp2D9,_tmp374,(void*)((struct Cyc_List_List*)_check_null(_tmp2D8))->hd);struct _tuple16 _tmp2CB=_tmp2CA;union Cyc_CfFlowInfo_FlowInfo _tmp2D2;void*_tmp2D1;_LL290: _tmp2D2=_tmp2CB.f1;_tmp2D1=_tmp2CB.f2;_LL291:;{
union Cyc_CfFlowInfo_FlowInfo _tmp2CC=_tmp2D2;if((_tmp2CC.BottomFL).tag == 1){_LL293: _LL294: {
struct _tuple16 _tmp2CE=({struct _tuple16 _tmp2CD;_tmp2CD.f1=_tmp2D2;_tmp2CD.f2=_tmp2D1;_tmp2CD;});_npop_handler(0);return _tmp2CE;}}else{_LL295: _LL296: {
struct _tuple16 _tmp2D0=({struct _tuple16 _tmp2CF;_tmp2CF.f1=_tmp2B7;_tmp2CF.f2=_tmp2D1;_tmp2CF;});_npop_handler(0);return _tmp2D0;}}_LL292:;};}default: _LL285: _LL286:
# 1557
(int)_throw((void*)({struct Cyc_Core_Impossible_exn_struct*_tmp2D3=_cycalloc(sizeof(*_tmp2D3));_tmp2D3[0]=({struct Cyc_Core_Impossible_exn_struct _tmp2D4;_tmp2D4.tag=Cyc_Core_Impossible;_tmp2D4.f1=({const char*_tmp2D5="anal_Rexp: Subscript -- bad type";_tag_dyneither(_tmp2D5,sizeof(char),33);});_tmp2D4;});_tmp2D3;}));}_LL280:;};};}
# 1524
;_pop_region(temp);}case 23: _LL193: _tmp375=((struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct*)_tmp175)->f1;_LL194: {
# 1564
struct _RegionHandle _tmp2DB=_new_region("temp");struct _RegionHandle*temp=& _tmp2DB;_push_region(temp);
{struct _tuple23 _tmp2DC=Cyc_NewControlFlow_anal_unordered_Rexps(temp,env,inflow,_tmp375,1,1);struct _tuple23 _tmp2DD=_tmp2DC;union Cyc_CfFlowInfo_FlowInfo _tmp2E9;struct Cyc_List_List*_tmp2E8;_LL298: _tmp2E9=_tmp2DD.f1;_tmp2E8=_tmp2DD.f2;_LL299:;{
struct _dyneither_ptr aggrdict=({unsigned int _tmp2E3=(unsigned int)
((int(*)(struct Cyc_List_List*x))Cyc_List_length)(_tmp375);void**_tmp2E4=(void**)_region_malloc(env->r,_check_times(sizeof(void*),_tmp2E3));struct _dyneither_ptr _tmp2E7=_tag_dyneither(_tmp2E4,sizeof(void*),_tmp2E3);{unsigned int _tmp2E5=_tmp2E3;unsigned int i;for(i=0;i < _tmp2E5;i ++){_tmp2E4[i]=(void*)({
void*_tmp2E6=(void*)((struct Cyc_List_List*)_check_null(_tmp2E8))->hd;
_tmp2E8=_tmp2E8->tl;
_tmp2E6;});}}_tmp2E7;});
# 1572
struct _tuple16 _tmp2E2=({struct _tuple16 _tmp2DE;_tmp2DE.f1=_tmp2E9;_tmp2DE.f2=(void*)({struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*_tmp2DF=_region_malloc(env->r,sizeof(*_tmp2DF));_tmp2DF[0]=({struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct _tmp2E0;_tmp2E0.tag=6;_tmp2E0.f1=({struct Cyc_CfFlowInfo_UnionRInfo _tmp2E1;_tmp2E1.is_union=0;_tmp2E1.fieldnum=- 1;_tmp2E1;});_tmp2E0.f2=aggrdict;_tmp2E0;});_tmp2DF;});_tmp2DE;});_npop_handler(0);return _tmp2E2;};}
# 1565
;_pop_region(temp);}case 29: _LL195: _tmp376=((struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*)_tmp175)->f2;_LL196: {
# 1575
struct Cyc_List_List*fs;
{void*_tmp2EA=Cyc_Tcutil_compress((void*)_check_null(e->topt));void*_tmp2EB=_tmp2EA;struct Cyc_List_List*_tmp2EF;if(((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp2EB)->tag == 12){_LL29B: _tmp2EF=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp2EB)->f2;_LL29C:
 fs=_tmp2EF;goto _LL29A;}else{_LL29D: _LL29E:
(int)_throw((void*)({struct Cyc_Core_Impossible_exn_struct*_tmp2EC=_cycalloc(sizeof(*_tmp2EC));_tmp2EC[0]=({struct Cyc_Core_Impossible_exn_struct _tmp2ED;_tmp2ED.tag=Cyc_Core_Impossible;_tmp2ED.f1=({const char*_tmp2EE="anal_Rexp:anon struct has bad type";_tag_dyneither(_tmp2EE,sizeof(char),35);});_tmp2ED;});_tmp2EC;}));}_LL29A:;}
# 1580
_tmp379=_tmp376;_tmp378=Cyc_Absyn_StructA;_tmp377=fs;goto _LL198;}case 28: if(((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp175)->f4 != 0){if(((struct Cyc_Absyn_Aggrdecl*)((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp175)->f4)->impl != 0){_LL197: _tmp379=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp175)->f3;_tmp378=(((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp175)->f4)->kind;_tmp377=((((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp175)->f4)->impl)->fields;_LL198: {
# 1582
void*exp_type=(void*)_check_null(e->topt);
struct _RegionHandle _tmp2F0=_new_region("temp");struct _RegionHandle*temp=& _tmp2F0;_push_region(temp);
{struct _tuple23 _tmp2F1=Cyc_NewControlFlow_anal_unordered_Rexps(temp,env,inflow,
((struct Cyc_List_List*(*)(struct _RegionHandle*,struct Cyc_Absyn_Exp*(*f)(struct _tuple26*),struct Cyc_List_List*x))Cyc_List_rmap)(temp,(struct Cyc_Absyn_Exp*(*)(struct _tuple26*))Cyc_Core_snd,_tmp379),1,1);
# 1584
struct _tuple23 _tmp2F2=_tmp2F1;union Cyc_CfFlowInfo_FlowInfo _tmp301;struct Cyc_List_List*_tmp300;_LL2A0: _tmp301=_tmp2F2.f1;_tmp300=_tmp2F2.f2;_LL2A1:;{
# 1586
struct _dyneither_ptr aggrdict=
Cyc_CfFlowInfo_aggrfields_to_aggrdict(_tmp15D,_tmp377,_tmp378 == Cyc_Absyn_UnionA,_tmp15D->unknown_all);
# 1589
int field_no=-1;
{int i=0;for(0;_tmp300 != 0;(((_tmp300=_tmp300->tl,_tmp379=_tmp379->tl)),++ i)){
struct Cyc_List_List*ds=(*((struct _tuple26*)((struct Cyc_List_List*)_check_null(_tmp379))->hd)).f1;for(0;ds != 0;ds=ds->tl){
void*_tmp2F3=(void*)ds->hd;void*_tmp2F4=_tmp2F3;struct _dyneither_ptr*_tmp2F9;if(((struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct*)_tmp2F4)->tag == 0){_LL2A3: _LL2A4:
(int)_throw((void*)({struct Cyc_Core_Impossible_exn_struct*_tmp2F5=_cycalloc(sizeof(*_tmp2F5));_tmp2F5[0]=({struct Cyc_Core_Impossible_exn_struct _tmp2F6;_tmp2F6.tag=Cyc_Core_Impossible;_tmp2F6.f1=({const char*_tmp2F7="anal_Rexp:Aggregate_e";_tag_dyneither(_tmp2F7,sizeof(char),22);});_tmp2F6;});_tmp2F5;}));}else{_LL2A5: _tmp2F9=((struct Cyc_Absyn_FieldName_Absyn_Designator_struct*)_tmp2F4)->f1;_LL2A6:
# 1596
 field_no=Cyc_CfFlowInfo_get_field_index_fs(_tmp377,_tmp2F9);
*((void**)_check_dyneither_subscript(aggrdict,sizeof(void*),field_no))=(void*)_tmp300->hd;
# 1599
if(_tmp378 == Cyc_Absyn_UnionA){
struct Cyc_Absyn_Exp*_tmp2F8=(*((struct _tuple26*)_tmp379->hd)).f2;
_tmp301=Cyc_NewControlFlow_use_Rval(env,_tmp2F8->loc,_tmp301,(void*)_tmp300->hd);
# 1603
Cyc_NewControlFlow_check_union_requires(_tmp2F8->loc,_tmp15D->r,exp_type,_tmp2F9,_tmp301);}}_LL2A2:;}}}{
# 1606
struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*_tmp2FA=({struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*_tmp2FD=_region_malloc(env->r,sizeof(*_tmp2FD));_tmp2FD[0]=({struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct _tmp2FE;_tmp2FE.tag=6;_tmp2FE.f1=({struct Cyc_CfFlowInfo_UnionRInfo _tmp2FF;_tmp2FF.is_union=_tmp378 == Cyc_Absyn_UnionA;_tmp2FF.fieldnum=field_no;_tmp2FF;});_tmp2FE.f2=aggrdict;_tmp2FE;});_tmp2FD;});
struct _tuple16 _tmp2FC=({struct _tuple16 _tmp2FB;_tmp2FB.f1=_tmp301;_tmp2FB.f2=(void*)_tmp2FA;_tmp2FB;});_npop_handler(0);return _tmp2FC;};};}
# 1584
;_pop_region(temp);}}else{goto _LL199;}}else{_LL199: _LL19A:
# 1610
({struct Cyc_String_pa_PrintArg_struct _tmp304;_tmp304.tag=0;_tmp304.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_exp2string(e));({void*_tmp302[1]={& _tmp304};((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(({const char*_tmp303="anal_Rexp:missing aggrdeclimpl in %s";_tag_dyneither(_tmp303,sizeof(char),37);}),_tag_dyneither(_tmp302,sizeof(void*),1));});});}case 25: _LL19B: _tmp37A=((struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_tmp175)->f1;_LL19C: {
# 1612
struct _RegionHandle _tmp305=_new_region("temp");struct _RegionHandle*temp=& _tmp305;_push_region(temp);
{struct Cyc_List_List*_tmp306=((struct Cyc_List_List*(*)(struct _RegionHandle*,struct Cyc_Absyn_Exp*(*f)(struct _tuple26*),struct Cyc_List_List*x))Cyc_List_rmap)(temp,(struct Cyc_Absyn_Exp*(*)(struct _tuple26*))Cyc_Core_snd,_tmp37A);
struct _tuple23 _tmp307=Cyc_NewControlFlow_anal_unordered_Rexps(temp,env,inflow,_tmp306,1,1);struct _tuple23 _tmp308=_tmp307;union Cyc_CfFlowInfo_FlowInfo _tmp30C;struct Cyc_List_List*_tmp30B;_LL2A8: _tmp30C=_tmp308.f1;_tmp30B=_tmp308.f2;_LL2A9:;
for(0;_tmp30B != 0;(_tmp30B=_tmp30B->tl,_tmp306=_tmp306->tl)){
_tmp30C=Cyc_NewControlFlow_use_Rval(env,((struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(_tmp306))->hd)->loc,_tmp30C,(void*)_tmp30B->hd);}{
struct _tuple16 _tmp30A=({struct _tuple16 _tmp309;_tmp309.f1=_tmp30C;_tmp309.f2=Cyc_CfFlowInfo_typ_to_absrval(_tmp15D,(void*)_check_null(e->topt),0,_tmp15D->unknown_all);_tmp309;});_npop_handler(0);return _tmp30A;};}
# 1613
;_pop_region(temp);}case 26: _LL19D: _tmp37E=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp175)->f1;_tmp37D=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp175)->f2;_tmp37C=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp175)->f3;_tmp37B=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp175)->f4;_LL19E: {
# 1621
struct _tuple16 _tmp30D=Cyc_NewControlFlow_anal_Rexp(env,1,inflow,_tmp37D);struct _tuple16 _tmp30E=_tmp30D;union Cyc_CfFlowInfo_FlowInfo _tmp331;void*_tmp330;_LL2AB: _tmp331=_tmp30E.f1;_tmp330=_tmp30E.f2;_LL2AC:;{
union Cyc_CfFlowInfo_FlowInfo _tmp30F=_tmp331;struct Cyc_Dict_Dict _tmp32F;struct Cyc_List_List*_tmp32E;if((_tmp30F.BottomFL).tag == 1){_LL2AE: _LL2AF:
 return({struct _tuple16 _tmp310;_tmp310.f1=_tmp331;_tmp310.f2=_tmp15D->unknown_all;_tmp310;});}else{_LL2B0: _tmp32F=((_tmp30F.ReachableFL).val).f1;_tmp32E=((_tmp30F.ReachableFL).val).f2;_LL2B1:
# 1625
 if(Cyc_CfFlowInfo_initlevel(env->fenv,_tmp32F,_tmp330)== Cyc_CfFlowInfo_NoneIL)
({void*_tmp311=0;Cyc_CfFlowInfo_aerr(_tmp37D->loc,({const char*_tmp312="expression may not be initialized";_tag_dyneither(_tmp312,sizeof(char),34);}),_tag_dyneither(_tmp311,sizeof(void*),0));});{
# 1629
struct Cyc_List_List*new_relns=_tmp32E;
union Cyc_Relations_RelnOp n1=Cyc_Relations_RVar(_tmp37E);
union Cyc_Relations_RelnOp n2=Cyc_Relations_RConst(0);
if(Cyc_Relations_exp2relnop(_tmp37D,& n2))
new_relns=Cyc_Relations_add_relation(env->r,n1,Cyc_Relations_Rlt,n2,_tmp32E);
# 1635
if(_tmp32E != new_relns)
_tmp331=Cyc_CfFlowInfo_ReachableFL(_tmp32F,new_relns);{
# 1639
void*_tmp313=_tmp330;switch(*((int*)_tmp313)){case 0: _LL2B3: _LL2B4:
 return({struct _tuple16 _tmp314;_tmp314.f1=_tmp331;_tmp314.f2=_tmp15D->unknown_all;_tmp314;});case 2: _LL2B5: _LL2B6:
 goto _LL2B8;case 1: _LL2B7: _LL2B8:
 goto _LL2BA;case 5: _LL2B9: _LL2BA: {
# 1644
struct Cyc_List_List _tmp315=({struct Cyc_List_List _tmp320;_tmp320.hd=_tmp37E;_tmp320.tl=0;_tmp320;});
_tmp331=Cyc_NewControlFlow_add_vars(_tmp15D,_tmp331,& _tmp315,_tmp15D->unknown_all,e->loc,0);{
# 1647
struct _tuple16 _tmp316=Cyc_NewControlFlow_anal_Rexp(env,1,_tmp331,_tmp37C);struct _tuple16 _tmp317=_tmp316;union Cyc_CfFlowInfo_FlowInfo _tmp31F;void*_tmp31E;_LL2BE: _tmp31F=_tmp317.f1;_tmp31E=_tmp317.f2;_LL2BF:;
{union Cyc_CfFlowInfo_FlowInfo _tmp318=_tmp31F;struct Cyc_Dict_Dict _tmp31D;if((_tmp318.BottomFL).tag == 1){_LL2C1: _LL2C2:
 return({struct _tuple16 _tmp319;_tmp319.f1=_tmp31F;_tmp319.f2=_tmp15D->unknown_all;_tmp319;});}else{_LL2C3: _tmp31D=((_tmp318.ReachableFL).val).f1;_LL2C4:
# 1651
 if(Cyc_CfFlowInfo_initlevel(_tmp15D,_tmp31D,_tmp31E)!= Cyc_CfFlowInfo_AllIL){
({void*_tmp31A=0;Cyc_CfFlowInfo_aerr(_tmp37D->loc,({const char*_tmp31B="expression may not be initialized";_tag_dyneither(_tmp31B,sizeof(char),34);}),_tag_dyneither(_tmp31A,sizeof(void*),0));});
return({struct _tuple16 _tmp31C;_tmp31C.f1=Cyc_CfFlowInfo_BottomFL();_tmp31C.f2=_tmp15D->unknown_all;_tmp31C;});}}_LL2C0:;}
# 1656
_tmp331=_tmp31F;
goto _LL2BC;};}default: _LL2BB: _LL2BC:
# 1659
 while(1){
struct Cyc_List_List _tmp321=({struct Cyc_List_List _tmp32C;_tmp32C.hd=_tmp37E;_tmp32C.tl=0;_tmp32C;});
_tmp331=Cyc_NewControlFlow_add_vars(_tmp15D,_tmp331,& _tmp321,_tmp15D->unknown_all,e->loc,0);{
struct _tuple16 _tmp322=Cyc_NewControlFlow_anal_Rexp(env,1,_tmp331,_tmp37C);struct _tuple16 _tmp323=_tmp322;union Cyc_CfFlowInfo_FlowInfo _tmp32B;void*_tmp32A;_LL2C6: _tmp32B=_tmp323.f1;_tmp32A=_tmp323.f2;_LL2C7:;
{union Cyc_CfFlowInfo_FlowInfo _tmp324=_tmp32B;struct Cyc_Dict_Dict _tmp328;if((_tmp324.BottomFL).tag == 1){_LL2C9: _LL2CA:
 goto _LL2C8;}else{_LL2CB: _tmp328=((_tmp324.ReachableFL).val).f1;_LL2CC:
# 1666
 if(Cyc_CfFlowInfo_initlevel(_tmp15D,_tmp328,_tmp32A)!= Cyc_CfFlowInfo_AllIL){
({void*_tmp325=0;Cyc_CfFlowInfo_aerr(_tmp37D->loc,({const char*_tmp326="expression may not be initialized";_tag_dyneither(_tmp326,sizeof(char),34);}),_tag_dyneither(_tmp325,sizeof(void*),0));});
return({struct _tuple16 _tmp327;_tmp327.f1=Cyc_CfFlowInfo_BottomFL();_tmp327.f2=_tmp15D->unknown_all;_tmp327;});}}_LL2C8:;}{
# 1671
union Cyc_CfFlowInfo_FlowInfo _tmp329=Cyc_CfFlowInfo_join_flow(_tmp15D,env->all_changed,_tmp331,_tmp32B);
if(Cyc_CfFlowInfo_flow_lessthan_approx(_tmp329,_tmp331))
break;
_tmp331=_tmp329;};};}
# 1676
return({struct _tuple16 _tmp32D;_tmp32D.f1=_tmp331;_tmp32D.f2=_tmp15D->unknown_all;_tmp32D;});}_LL2B2:;};};}_LL2AD:;};}case 27: _LL19F: _tmp381=((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmp175)->f1;_tmp380=(void*)((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmp175)->f2;_tmp37F=((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmp175)->f3;_LL1A0: {
# 1682
void*root=(void*)({struct Cyc_CfFlowInfo_MallocPt_CfFlowInfo_Root_struct*_tmp33D=_region_malloc(_tmp15D->r,sizeof(*_tmp33D));_tmp33D[0]=({struct Cyc_CfFlowInfo_MallocPt_CfFlowInfo_Root_struct _tmp33E;_tmp33E.tag=1;_tmp33E.f1=_tmp381;_tmp33E.f2=(void*)_check_null(e->topt);_tmp33E;});_tmp33D;});
struct Cyc_CfFlowInfo_Place*place=({struct Cyc_CfFlowInfo_Place*_tmp33C=_region_malloc(_tmp15D->r,sizeof(*_tmp33C));_tmp33C->root=root;_tmp33C->fields=0;_tmp33C;});
void*rval=(void*)({struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*_tmp33A=_region_malloc(_tmp15D->r,sizeof(*_tmp33A));_tmp33A[0]=({struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct _tmp33B;_tmp33B.tag=5;_tmp33B.f1=place;_tmp33B;});_tmp33A;});
void*place_val;
# 1690
place_val=Cyc_CfFlowInfo_typ_to_absrval(_tmp15D,_tmp380,0,_tmp15D->unknown_none);{
union Cyc_CfFlowInfo_FlowInfo outflow;
((int(*)(struct Cyc_Dict_Dict*set,struct Cyc_CfFlowInfo_Place*place,unsigned int loc))Cyc_CfFlowInfo_update_place_set)(env->all_changed,place,0);{
struct _tuple16 _tmp332=Cyc_NewControlFlow_anal_Rexp(env,1,inflow,_tmp381);struct _tuple16 _tmp333=_tmp332;union Cyc_CfFlowInfo_FlowInfo _tmp339;_LL2CE: _tmp339=_tmp333.f1;_LL2CF:;
outflow=_tmp339;{
union Cyc_CfFlowInfo_FlowInfo _tmp334=outflow;struct Cyc_Dict_Dict _tmp338;struct Cyc_List_List*_tmp337;if((_tmp334.BottomFL).tag == 1){_LL2D1: _LL2D2:
 return({struct _tuple16 _tmp335;_tmp335.f1=outflow;_tmp335.f2=rval;_tmp335;});}else{_LL2D3: _tmp338=((_tmp334.ReachableFL).val).f1;_tmp337=((_tmp334.ReachableFL).val).f2;_LL2D4:
# 1698
 return({struct _tuple16 _tmp336;_tmp336.f1=Cyc_CfFlowInfo_ReachableFL(((struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,void*k,void*v))Cyc_Dict_insert)(_tmp338,root,place_val),_tmp337);_tmp336.f2=rval;_tmp336;});}_LL2D0:;};};};}case 36: _LL1A1: _tmp382=((struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*)_tmp175)->f1;_LL1A2: {
# 1702
struct _tuple17 _tmp33F=({struct _tuple17 _tmp342;_tmp342.f1=(env->fenv)->unknown_all;_tmp342.f2=copy_ctxt;_tmp342;});
union Cyc_CfFlowInfo_FlowInfo _tmp340=Cyc_NewControlFlow_anal_stmt(env,inflow,_tmp382,& _tmp33F);
return({struct _tuple16 _tmp341;_tmp341.f1=_tmp340;_tmp341.f2=_tmp33F.f1;_tmp341;});}case 35: _LL1A5: _LL1A6:
# 1707
 goto _LL1A8;case 24: _LL1A7: _LL1A8:
 goto _LL1AA;case 38: _LL1A9: _LL1AA:
 goto _LL1AC;default: _LL1AB: _LL1AC:
# 1711
(int)_throw((void*)({struct Cyc_Core_Impossible_exn_struct*_tmp343=_cycalloc(sizeof(*_tmp343));_tmp343[0]=({struct Cyc_Core_Impossible_exn_struct _tmp344;_tmp344.tag=Cyc_Core_Impossible;_tmp344.f1=({const char*_tmp345="anal_Rexp, unexpected exp form";_tag_dyneither(_tmp345,sizeof(char),31);});_tmp344;});_tmp343;}));}_LL144:;};}
# 1721
static struct _tuple18 Cyc_NewControlFlow_anal_derefL(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_Absyn_Exp*e,union Cyc_CfFlowInfo_FlowInfo f,void*r,int passthrough_consumes,int expanded_place,struct Cyc_List_List*flds){
# 1730
struct Cyc_CfFlowInfo_FlowEnv*_tmp383=env->fenv;
void*_tmp384=Cyc_Tcutil_compress((void*)_check_null(e->topt));void*_tmp385=_tmp384;void*_tmp3B4;union Cyc_Absyn_Constraint*_tmp3B3;union Cyc_Absyn_Constraint*_tmp3B2;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp385)->tag == 5){_LL2D6: _tmp3B4=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp385)->f1).elt_typ;_tmp3B3=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp385)->f1).ptr_atts).bounds;_tmp3B2=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp385)->f1).ptr_atts).zero_term;_LL2D7: {
# 1733
union Cyc_CfFlowInfo_FlowInfo _tmp386=f;struct Cyc_Dict_Dict _tmp3AE;struct Cyc_List_List*_tmp3AD;if((_tmp386.BottomFL).tag == 1){_LL2DB: _LL2DC:
 return({struct _tuple18 _tmp387;_tmp387.f1=f;_tmp387.f2=Cyc_CfFlowInfo_UnknownL();_tmp387;});}else{_LL2DD: _tmp3AE=((_tmp386.ReachableFL).val).f1;_tmp3AD=((_tmp386.ReachableFL).val).f2;_LL2DE: {
# 1737
struct _tuple15 _tmp388=Cyc_CfFlowInfo_unname_rval((env->fenv)->r,r);struct _tuple15 _tmp389=_tmp388;void*_tmp3AC;struct Cyc_List_List*_tmp3AB;_LL2E0: _tmp3AC=_tmp389.f1;_tmp3AB=_tmp389.f2;_LL2E1:;
retry: {void*_tmp38A=_tmp3AC;enum Cyc_CfFlowInfo_InitLevel _tmp3A1;void*_tmp3A0;struct Cyc_List_List*_tmp39F;switch(*((int*)_tmp38A)){case 8: _LL2E3: _LL2E4:
# 1740
(int)_throw((void*)({struct Cyc_Core_Impossible_exn_struct*_tmp38B=_cycalloc(sizeof(*_tmp38B));_tmp38B[0]=({struct Cyc_Core_Impossible_exn_struct _tmp38C;_tmp38C.tag=Cyc_Core_Impossible;_tmp38C.f1=({const char*_tmp38D="named location in anal_derefL";_tag_dyneither(_tmp38D,sizeof(char),30);});_tmp38C;});_tmp38B;}));case 1: _LL2E5: _LL2E6:
 goto _LL2E8;case 2: _LL2E7: _LL2E8:
# 1743
 e->annot=(void*)({struct Cyc_CfFlowInfo_NotZero_Absyn_AbsynAnnot_struct*_tmp38E=_cycalloc(sizeof(*_tmp38E));_tmp38E[0]=({struct Cyc_CfFlowInfo_NotZero_Absyn_AbsynAnnot_struct _tmp38F;_tmp38F.tag=Cyc_CfFlowInfo_NotZero;_tmp38F.f1=Cyc_Relations_copy_relns(Cyc_Core_heap_region,_tmp3AD);_tmp38F;});_tmp38E;});goto _LL2E2;case 5: _LL2E9: _tmp3A0=(((struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*)_tmp38A)->f1)->root;_tmp39F=(((struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*)_tmp38A)->f1)->fields;_LL2EA:
# 1746
 if(expanded_place)
# 1749
e->annot=(void*)({struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct*_tmp390=_cycalloc(sizeof(*_tmp390));_tmp390[0]=({struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct _tmp391;_tmp391.tag=Cyc_CfFlowInfo_UnknownZ;_tmp391.f1=Cyc_Relations_copy_relns(Cyc_Core_heap_region,_tmp3AD);_tmp391;});_tmp390;});else{
# 1752
void*_tmp392=e->annot;void*_tmp393=_tmp392;if(((struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct*)_tmp393)->tag == Cyc_CfFlowInfo_UnknownZ){_LL2F2: _LL2F3:
# 1756
 e->annot=(void*)({struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct*_tmp394=_cycalloc(sizeof(*_tmp394));_tmp394[0]=({struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct _tmp395;_tmp395.tag=Cyc_CfFlowInfo_UnknownZ;_tmp395.f1=Cyc_Relations_copy_relns(Cyc_Core_heap_region,_tmp3AD);_tmp395;});_tmp394;});
goto _LL2F1;}else{_LL2F4: _LL2F5:
# 1759
 e->annot=(void*)({struct Cyc_CfFlowInfo_NotZero_Absyn_AbsynAnnot_struct*_tmp396=_cycalloc(sizeof(*_tmp396));_tmp396[0]=({struct Cyc_CfFlowInfo_NotZero_Absyn_AbsynAnnot_struct _tmp397;_tmp397.tag=Cyc_CfFlowInfo_NotZero;_tmp397.f1=Cyc_Relations_copy_relns(Cyc_Core_heap_region,_tmp3AD);_tmp397;});_tmp396;});
goto _LL2F1;}_LL2F1:;}
# 1763
if(Cyc_Tcutil_is_bound_one(_tmp3B3))
return({struct _tuple18 _tmp398;_tmp398.f1=f;_tmp398.f2=Cyc_CfFlowInfo_PlaceL(({struct Cyc_CfFlowInfo_Place*_tmp399=_region_malloc(_tmp383->r,sizeof(*_tmp399));_tmp399->root=_tmp3A0;_tmp399->fields=((struct Cyc_List_List*(*)(struct _RegionHandle*,struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_rappend)(_tmp383->r,_tmp39F,flds);_tmp399;}));_tmp398;});
goto _LL2E2;case 0: _LL2EB: _LL2EC:
# 1767
 e->annot=(void*)& Cyc_CfFlowInfo_IsZero_val;
return({struct _tuple18 _tmp39A;_tmp39A.f1=Cyc_CfFlowInfo_BottomFL();_tmp39A.f2=Cyc_CfFlowInfo_UnknownL();_tmp39A;});case 3: _LL2ED: _tmp3A1=((struct Cyc_CfFlowInfo_UnknownR_CfFlowInfo_AbsRVal_struct*)_tmp38A)->f1;_LL2EE:
# 1770
 if(Cyc_Tcutil_is_bound_one(_tmp3B3))
f=Cyc_NewControlFlow_notzero(env,inflow,f,e,_tmp3A1,_tmp3AB);
goto _LL2F0;default: _LL2EF: _LL2F0:
# 1775
 if(passthrough_consumes){
void*_tmp39B=_tmp3AC;void*_tmp39C;if(((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp39B)->tag == 7){_LL2F7: _tmp39C=(void*)((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp39B)->f3;_LL2F8:
 _tmp3AC=_tmp39C;goto retry;}else{_LL2F9: _LL2FA:
 goto _LL2F6;}_LL2F6:;}
# 1781
e->annot=(void*)({struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct*_tmp39D=_cycalloc(sizeof(*_tmp39D));_tmp39D[0]=({struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct _tmp39E;_tmp39E.tag=Cyc_CfFlowInfo_UnknownZ;_tmp39E.f1=Cyc_Relations_copy_relns(Cyc_Core_heap_region,_tmp3AD);_tmp39E;});_tmp39D;});}_LL2E2:;}
# 1783
if(Cyc_CfFlowInfo_initlevel(_tmp383,_tmp3AE,_tmp3AC)== Cyc_CfFlowInfo_NoneIL){
struct _tuple15 _tmp3A2=Cyc_CfFlowInfo_unname_rval((env->fenv)->r,_tmp3AC);struct _tuple15 _tmp3A3=_tmp3A2;void*_tmp3A9;_LL2FC: _tmp3A9=_tmp3A3.f1;_LL2FD:;{
void*_tmp3A4=_tmp3A9;if(((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp3A4)->tag == 7){_LL2FF: _LL300:
# 1787
({void*_tmp3A5=0;Cyc_CfFlowInfo_aerr(e->loc,({const char*_tmp3A6="attempt to dereference an alias-free that has already been copied";_tag_dyneither(_tmp3A6,sizeof(char),66);}),_tag_dyneither(_tmp3A5,sizeof(void*),0));});
goto _LL2FE;}else{_LL301: _LL302:
# 1790
({void*_tmp3A7=0;Cyc_CfFlowInfo_aerr(e->loc,({const char*_tmp3A8="dereference of possibly uninitialized pointer";_tag_dyneither(_tmp3A8,sizeof(char),46);}),_tag_dyneither(_tmp3A7,sizeof(void*),0));});
goto _LL2FE;}_LL2FE:;};}
# 1794
return({struct _tuple18 _tmp3AA;_tmp3AA.f1=f;_tmp3AA.f2=Cyc_CfFlowInfo_UnknownL();_tmp3AA;});}}_LL2DA:;}}else{_LL2D8: _LL2D9:
# 1796
(int)_throw((void*)({struct Cyc_Core_Impossible_exn_struct*_tmp3AF=_cycalloc(sizeof(*_tmp3AF));_tmp3AF[0]=({struct Cyc_Core_Impossible_exn_struct _tmp3B0;_tmp3B0.tag=Cyc_Core_Impossible;_tmp3B0.f1=({const char*_tmp3B1="left deref of non-pointer-type";_tag_dyneither(_tmp3B1,sizeof(char),31);});_tmp3B0;});_tmp3AF;}));}_LL2D5:;}
# 1804
static struct _tuple18 Cyc_NewControlFlow_anal_Lexp_rec(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,int expand_unique,int passthrough_consumes,struct Cyc_Absyn_Exp*e,struct Cyc_List_List*flds){
# 1807
struct Cyc_Dict_Dict d;
struct Cyc_CfFlowInfo_FlowEnv*_tmp3B5=env->fenv;
{union Cyc_CfFlowInfo_FlowInfo _tmp3B6=inflow;struct Cyc_Dict_Dict _tmp3B9;struct Cyc_List_List*_tmp3B8;if((_tmp3B6.BottomFL).tag == 1){_LL304: _LL305:
 return({struct _tuple18 _tmp3B7;_tmp3B7.f1=Cyc_CfFlowInfo_BottomFL();_tmp3B7.f2=Cyc_CfFlowInfo_UnknownL();_tmp3B7;});}else{_LL306: _tmp3B9=((_tmp3B6.ReachableFL).val).f1;_tmp3B8=((_tmp3B6.ReachableFL).val).f2;_LL307:
# 1812
 d=_tmp3B9;}_LL303:;}{
# 1816
void*_tmp3BA=e->r;void*_tmp3BB=_tmp3BA;struct Cyc_Absyn_Exp*_tmp418;struct _dyneither_ptr*_tmp417;struct Cyc_Absyn_Exp*_tmp416;struct Cyc_Absyn_Exp*_tmp415;struct Cyc_Absyn_Exp*_tmp414;struct Cyc_Absyn_Exp*_tmp413;struct _dyneither_ptr*_tmp412;struct Cyc_Absyn_Vardecl*_tmp411;struct Cyc_Absyn_Vardecl*_tmp410;struct Cyc_Absyn_Vardecl*_tmp40F;struct Cyc_Absyn_Vardecl*_tmp40E;struct Cyc_Absyn_Exp*_tmp40D;struct Cyc_Absyn_Exp*_tmp40C;struct Cyc_Absyn_Exp*_tmp40B;switch(*((int*)_tmp3BB)){case 13: _LL309: _tmp40B=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp3BB)->f2;_LL30A:
 _tmp40C=_tmp40B;goto _LL30C;case 11: _LL30B: _tmp40C=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmp3BB)->f1;_LL30C:
 _tmp40D=_tmp40C;goto _LL30E;case 12: _LL30D: _tmp40D=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmp3BB)->f1;_LL30E:
 return Cyc_NewControlFlow_anal_Lexp_rec(env,inflow,expand_unique,passthrough_consumes,_tmp40D,flds);case 1: switch(*((int*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp3BB)->f1)){case 3: _LL30F: _tmp40E=((struct Cyc_Absyn_Param_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp3BB)->f1)->f1;_LL310:
# 1821
 _tmp40F=_tmp40E;goto _LL312;case 4: _LL311: _tmp40F=((struct Cyc_Absyn_Local_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp3BB)->f1)->f1;_LL312:
 _tmp410=_tmp40F;goto _LL314;case 5: _LL313: _tmp410=((struct Cyc_Absyn_Pat_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp3BB)->f1)->f1;_LL314:
# 1824
 return({struct _tuple18 _tmp3BC;_tmp3BC.f1=inflow;_tmp3BC.f2=Cyc_CfFlowInfo_PlaceL(({struct Cyc_CfFlowInfo_Place*_tmp3BD=_region_malloc(env->r,sizeof(*_tmp3BD));_tmp3BD->root=(void*)({struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*_tmp3BE=_region_malloc(env->r,sizeof(*_tmp3BE));_tmp3BE[0]=({struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct _tmp3BF;_tmp3BF.tag=0;_tmp3BF.f1=_tmp410;_tmp3BF;});_tmp3BE;});_tmp3BD->fields=flds;_tmp3BD;}));_tmp3BC;});case 1: _LL315: _tmp411=((struct Cyc_Absyn_Global_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp3BB)->f1)->f1;_LL316:
# 1826
 return({struct _tuple18 _tmp3C0;_tmp3C0.f1=inflow;_tmp3C0.f2=Cyc_CfFlowInfo_UnknownL();_tmp3C0;});default: goto _LL31F;}case 21: _LL317: _tmp413=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp3BB)->f1;_tmp412=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp3BB)->f2;_LL318:
# 1829
{void*_tmp3C1=Cyc_Tcutil_compress((void*)_check_null(_tmp413->topt));void*_tmp3C2=_tmp3C1;void*_tmp3C7;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp3C2)->tag == 5){_LL322: _tmp3C7=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp3C2)->f1).elt_typ;_LL323:
# 1831
 if(!Cyc_Absyn_is_nontagged_nonrequire_union_type(_tmp3C7)){
Cyc_NewControlFlow_check_union_requires(_tmp413->loc,_tmp3B5->r,_tmp3C7,_tmp412,inflow);
flds=({struct Cyc_List_List*_tmp3C3=_region_malloc(env->r,sizeof(*_tmp3C3));_tmp3C3->hd=(void*)Cyc_CfFlowInfo_get_field_index(_tmp3C7,_tmp412);_tmp3C3->tl=flds;_tmp3C3;});}
# 1835
goto _LL321;}else{_LL324: _LL325:
(int)_throw((void*)({struct Cyc_Core_Impossible_exn_struct*_tmp3C4=_cycalloc(sizeof(*_tmp3C4));_tmp3C4[0]=({struct Cyc_Core_Impossible_exn_struct _tmp3C5;_tmp3C5.tag=Cyc_Core_Impossible;_tmp3C5.f1=({const char*_tmp3C6="anal_Lexp: AggrArrow ptr";_tag_dyneither(_tmp3C6,sizeof(char),25);});_tmp3C5;});_tmp3C4;}));}_LL321:;}
# 1838
_tmp414=_tmp413;goto _LL31A;case 19: _LL319: _tmp414=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp3BB)->f1;_LL31A:
# 1841
 if(expand_unique  && Cyc_Tcutil_is_noalias_pointer((void*)_check_null(_tmp414->topt),1)){
# 1843
struct _tuple18 _tmp3C8=
Cyc_NewControlFlow_anal_Lexp(env,inflow,
Cyc_Tcutil_is_noalias_pointer((void*)_check_null(_tmp414->topt),1),passthrough_consumes,_tmp414);
# 1843
struct _tuple18 _tmp3C9=_tmp3C8;union Cyc_CfFlowInfo_FlowInfo _tmp3E4;union Cyc_CfFlowInfo_AbsLVal _tmp3E3;_LL327: _tmp3E4=_tmp3C9.f1;_tmp3E3=_tmp3C9.f2;_LL328:;{
# 1847
struct _tuple18 _tmp3CA=({struct _tuple18 _tmp3E2;_tmp3E2.f1=_tmp3E4;_tmp3E2.f2=_tmp3E3;_tmp3E2;});struct _tuple18 _tmp3CB=_tmp3CA;struct Cyc_Dict_Dict _tmp3E1;struct Cyc_List_List*_tmp3E0;struct Cyc_CfFlowInfo_Place*_tmp3DF;if(((_tmp3CB.f1).ReachableFL).tag == 2){if(((_tmp3CB.f2).PlaceL).tag == 1){_LL32A: _tmp3E1=(((_tmp3CB.f1).ReachableFL).val).f1;_tmp3E0=(((_tmp3CB.f1).ReachableFL).val).f2;_tmp3DF=((_tmp3CB.f2).PlaceL).val;_LL32B: {
# 1849
void*_tmp3CC=Cyc_CfFlowInfo_lookup_place(_tmp3E1,_tmp3DF);
struct _tuple15 _tmp3CD=Cyc_CfFlowInfo_unname_rval((env->fenv)->r,_tmp3CC);struct _tuple15 _tmp3CE=_tmp3CD;void*_tmp3DE;struct Cyc_List_List*_tmp3DD;_LL32F: _tmp3DE=_tmp3CE.f1;_tmp3DD=_tmp3CE.f2;_LL330:;{
void*_tmp3CF=_tmp3DE;struct Cyc_Absyn_Vardecl*_tmp3DC;void*_tmp3DB;switch(*((int*)_tmp3CF)){case 8: _LL332: _tmp3DC=((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp3CF)->f1;_tmp3DB=(void*)((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp3CF)->f2;_LL333:
# 1853
(int)_throw((void*)({struct Cyc_Core_Impossible_exn_struct*_tmp3D0=_cycalloc(sizeof(*_tmp3D0));_tmp3D0[0]=({struct Cyc_Core_Impossible_exn_struct _tmp3D1;_tmp3D1.tag=Cyc_Core_Impossible;_tmp3D1.f1=({const char*_tmp3D2="bad named location in anal_Lexp:deref";_tag_dyneither(_tmp3D2,sizeof(char),38);});_tmp3D1;});_tmp3D0;}));case 7: if(((struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*)((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp3CF)->f3)->tag == 5){_LL334: _LL335:
 goto _LL337;}else{goto _LL338;}case 5: _LL336: _LL337:
# 1858
 return Cyc_NewControlFlow_anal_derefL(env,_tmp3E4,_tmp414,_tmp3E4,_tmp3CC,passthrough_consumes,0,flds);default: _LL338: _LL339: {
# 1865
enum Cyc_CfFlowInfo_InitLevel il=Cyc_CfFlowInfo_initlevel(_tmp3B5,_tmp3E1,_tmp3DE);
void*leaf=il == Cyc_CfFlowInfo_AllIL?_tmp3B5->unknown_all: _tmp3B5->unknown_none;
void*new_rval=Cyc_CfFlowInfo_typ_to_absrval(_tmp3B5,Cyc_Tcutil_pointer_elt_type((void*)_check_null(_tmp414->topt)),0,leaf);
void*new_root=(void*)({struct Cyc_CfFlowInfo_MallocPt_CfFlowInfo_Root_struct*_tmp3D9=_region_malloc(_tmp3B5->r,sizeof(*_tmp3D9));_tmp3D9[0]=({struct Cyc_CfFlowInfo_MallocPt_CfFlowInfo_Root_struct _tmp3DA;_tmp3DA.tag=1;_tmp3DA.f1=e;_tmp3DA.f2=(void*)_check_null(e->topt);_tmp3DA;});_tmp3D9;});
struct Cyc_CfFlowInfo_Place*place=({struct Cyc_CfFlowInfo_Place*_tmp3D8=_region_malloc(_tmp3B5->r,sizeof(*_tmp3D8));_tmp3D8->root=new_root;_tmp3D8->fields=0;_tmp3D8;});
void*res=(void*)({struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*_tmp3D6=_region_malloc(_tmp3B5->r,sizeof(*_tmp3D6));_tmp3D6[0]=({struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct _tmp3D7;_tmp3D7.tag=5;_tmp3D7.f1=place;_tmp3D7;});_tmp3D6;});
for(0;_tmp3DD != 0;_tmp3DD=_tmp3DD->tl){
res=(void*)({struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*_tmp3D3=_region_malloc(_tmp3B5->r,sizeof(*_tmp3D3));_tmp3D3[0]=({struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct _tmp3D4;_tmp3D4.tag=8;_tmp3D4.f1=(struct Cyc_Absyn_Vardecl*)_tmp3DD->hd;_tmp3D4.f2=res;_tmp3D4;});_tmp3D3;});}
((int(*)(struct Cyc_Dict_Dict*set,struct Cyc_CfFlowInfo_Place*place,unsigned int loc))Cyc_CfFlowInfo_update_place_set)(env->all_changed,place,0);
_tmp3E1=((struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,void*k,void*v))Cyc_Dict_insert)(_tmp3E1,new_root,new_rval);
_tmp3E1=Cyc_CfFlowInfo_assign_place(_tmp3B5,e->loc,_tmp3E1,env->all_changed,_tmp3DF,res);{
union Cyc_CfFlowInfo_FlowInfo _tmp3D5=Cyc_CfFlowInfo_ReachableFL(_tmp3E1,_tmp3E0);
# 1883
return Cyc_NewControlFlow_anal_derefL(env,_tmp3D5,_tmp414,_tmp3D5,res,passthrough_consumes,1,flds);};}}_LL331:;};}}else{goto _LL32C;}}else{_LL32C: _LL32D:
# 1886
 goto _LL329;}_LL329:;};}{
# 1889
struct _tuple16 _tmp3E5=Cyc_NewControlFlow_anal_Rexp(env,0,inflow,_tmp414);struct _tuple16 _tmp3E6=_tmp3E5;union Cyc_CfFlowInfo_FlowInfo _tmp3E8;void*_tmp3E7;_LL33B: _tmp3E8=_tmp3E6.f1;_tmp3E7=_tmp3E6.f2;_LL33C:;
return Cyc_NewControlFlow_anal_derefL(env,inflow,_tmp414,_tmp3E8,_tmp3E7,passthrough_consumes,0,flds);};case 22: _LL31B: _tmp416=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp3BB)->f1;_tmp415=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp3BB)->f2;_LL31C: {
# 1893
void*_tmp3E9=Cyc_Tcutil_compress((void*)_check_null(_tmp416->topt));void*_tmp3EA=_tmp3E9;union Cyc_Absyn_Constraint*_tmp406;switch(*((int*)_tmp3EA)){case 10: _LL33E: _LL33F: {
# 1895
unsigned int _tmp3EB=(Cyc_Evexp_eval_const_uint_exp(_tmp415)).f1;
return Cyc_NewControlFlow_anal_Lexp_rec(env,inflow,expand_unique,passthrough_consumes,_tmp416,({struct Cyc_List_List*_tmp3EC=_region_malloc(env->r,sizeof(*_tmp3EC));_tmp3EC->hd=(void*)_tmp3EB;_tmp3EC->tl=flds;_tmp3EC;}));}case 5: _LL340: _tmp406=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp3EA)->f1).ptr_atts).bounds;_LL341: {
# 1898
struct _RegionHandle _tmp3ED=_new_region("temp");struct _RegionHandle*temp=& _tmp3ED;_push_region(temp);
{struct _tuple23 _tmp3EE=Cyc_NewControlFlow_anal_unordered_Rexps(temp,env,inflow,({struct Cyc_Absyn_Exp*_tmp402[2];_tmp402[1]=_tmp415;_tmp402[0]=_tmp416;((struct Cyc_List_List*(*)(struct _RegionHandle*,struct _dyneither_ptr))Cyc_List_rlist)(temp,_tag_dyneither(_tmp402,sizeof(struct Cyc_Absyn_Exp*),2));}),0,1);struct _tuple23 _tmp3EF=_tmp3EE;union Cyc_CfFlowInfo_FlowInfo _tmp401;struct Cyc_List_List*_tmp400;_LL345: _tmp401=_tmp3EF.f1;_tmp400=_tmp3EF.f2;_LL346:;{
# 1901
union Cyc_CfFlowInfo_FlowInfo _tmp3F0=_tmp401;
{union Cyc_CfFlowInfo_FlowInfo _tmp3F1=_tmp401;struct Cyc_Dict_Dict _tmp3F6;struct Cyc_List_List*_tmp3F5;if((_tmp3F1.ReachableFL).tag == 2){_LL348: _tmp3F6=((_tmp3F1.ReachableFL).val).f1;_tmp3F5=((_tmp3F1.ReachableFL).val).f2;_LL349:
# 1904
 if(Cyc_CfFlowInfo_initlevel(_tmp3B5,_tmp3F6,(void*)((struct Cyc_List_List*)_check_null(((struct Cyc_List_List*)_check_null(_tmp400))->tl))->hd)== Cyc_CfFlowInfo_NoneIL)
({void*_tmp3F2=0;Cyc_CfFlowInfo_aerr(_tmp415->loc,({const char*_tmp3F3="expression may not be initialized";_tag_dyneither(_tmp3F3,sizeof(char),34);}),_tag_dyneither(_tmp3F2,sizeof(void*),0));});{
struct Cyc_List_List*_tmp3F4=Cyc_NewControlFlow_add_subscript_reln(_tmp3B5->r,_tmp3F5,_tmp416,_tmp415);
if(_tmp3F5 != _tmp3F4)
_tmp3F0=Cyc_CfFlowInfo_ReachableFL(_tmp3F6,_tmp3F4);
goto _LL347;};}else{_LL34A: _LL34B:
 goto _LL347;}_LL347:;}{
# 1912
struct _tuple18 _tmp3F7=Cyc_NewControlFlow_anal_derefL(env,inflow,_tmp416,_tmp401,(void*)((struct Cyc_List_List*)_check_null(_tmp400))->hd,passthrough_consumes,0,flds);struct _tuple18 _tmp3F8=_tmp3F7;union Cyc_CfFlowInfo_FlowInfo _tmp3FF;union Cyc_CfFlowInfo_AbsLVal _tmp3FE;_LL34D: _tmp3FF=_tmp3F8.f1;_tmp3FE=_tmp3F8.f2;_LL34E:;{
union Cyc_CfFlowInfo_FlowInfo _tmp3F9=_tmp3FF;if((_tmp3F9.BottomFL).tag == 1){_LL350: _LL351: {
struct _tuple18 _tmp3FB=({struct _tuple18 _tmp3FA;_tmp3FA.f1=_tmp3FF;_tmp3FA.f2=_tmp3FE;_tmp3FA;});_npop_handler(0);return _tmp3FB;}}else{_LL352: _LL353: {
struct _tuple18 _tmp3FD=({struct _tuple18 _tmp3FC;_tmp3FC.f1=_tmp3F0;_tmp3FC.f2=_tmp3FE;_tmp3FC;});_npop_handler(0);return _tmp3FD;}}_LL34F:;};};};}
# 1899
;_pop_region(temp);}default: _LL342: _LL343:
# 1918
(int)_throw((void*)({struct Cyc_Core_Impossible_exn_struct*_tmp403=_cycalloc(sizeof(*_tmp403));_tmp403[0]=({struct Cyc_Core_Impossible_exn_struct _tmp404;_tmp404.tag=Cyc_Core_Impossible;_tmp404.f1=({const char*_tmp405="anal_Lexp: Subscript -- bad type";_tag_dyneither(_tmp405,sizeof(char),33);});_tmp404;});_tmp403;}));}_LL33D:;}case 20: _LL31D: _tmp418=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp3BB)->f1;_tmp417=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp3BB)->f2;_LL31E: {
# 1922
void*_tmp407=(void*)_check_null(_tmp418->topt);
if(Cyc_Absyn_is_require_union_type(_tmp407))
Cyc_NewControlFlow_check_union_requires(_tmp418->loc,_tmp3B5->r,_tmp407,_tmp417,inflow);
# 1926
if(Cyc_Absyn_is_nontagged_nonrequire_union_type(_tmp407))
return({struct _tuple18 _tmp408;_tmp408.f1=inflow;_tmp408.f2=Cyc_CfFlowInfo_UnknownL();_tmp408;});
# 1929
return Cyc_NewControlFlow_anal_Lexp_rec(env,inflow,expand_unique,passthrough_consumes,_tmp418,({struct Cyc_List_List*_tmp409=_region_malloc(env->r,sizeof(*_tmp409));_tmp409->hd=(void*)
Cyc_CfFlowInfo_get_field_index(_tmp407,_tmp417);_tmp409->tl=flds;_tmp409;}));}default: _LL31F: _LL320:
# 1932
 return({struct _tuple18 _tmp40A;_tmp40A.f1=Cyc_CfFlowInfo_BottomFL();_tmp40A.f2=Cyc_CfFlowInfo_UnknownL();_tmp40A;});}_LL308:;};}
# 1936
static struct _tuple18 Cyc_NewControlFlow_anal_Lexp(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,int expand_unique,int passthrough_consumes,struct Cyc_Absyn_Exp*e){
# 1939
struct _tuple18 _tmp419=Cyc_NewControlFlow_anal_Lexp_rec(env,inflow,expand_unique,passthrough_consumes,e,0);struct _tuple18 _tmp41A=_tmp419;union Cyc_CfFlowInfo_FlowInfo _tmp41D;union Cyc_CfFlowInfo_AbsLVal _tmp41C;_LL355: _tmp41D=_tmp41A.f1;_tmp41C=_tmp41A.f2;_LL356:;
return({struct _tuple18 _tmp41B;_tmp41B.f1=_tmp41D;_tmp41B.f2=_tmp41C;_tmp41B;});}
# 1946
static union Cyc_CfFlowInfo_FlowInfo Cyc_NewControlFlow_expand_unique_places(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_List_List*es){
# 1949
{struct Cyc_List_List*x=es;for(0;x != 0;x=x->tl){
if(Cyc_NewControlFlow_is_local_var_rooted_path((struct Cyc_Absyn_Exp*)x->hd,1) && 
Cyc_Tcutil_is_noalias_pointer_or_aggr(env->r,(void*)_check_null(((struct Cyc_Absyn_Exp*)x->hd)->topt))){
struct _tuple18 _tmp41E=Cyc_NewControlFlow_anal_Lexp(env,inflow,1,0,(struct Cyc_Absyn_Exp*)x->hd);struct _tuple18 _tmp41F=_tmp41E;union Cyc_CfFlowInfo_FlowInfo _tmp420;_LL358: _tmp420=_tmp41F.f1;_LL359:;
inflow=_tmp420;}}}
# 1957
return inflow;}
# 1963
static struct _tuple19 Cyc_NewControlFlow_anal_primop_test(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,enum Cyc_Absyn_Primop p,struct Cyc_List_List*es){
# 1966
struct Cyc_CfFlowInfo_FlowEnv*_tmp421=env->fenv;
# 1969
void*r1;void*r2;
union Cyc_CfFlowInfo_FlowInfo f;
struct _RegionHandle _tmp422=_new_region("temp");struct _RegionHandle*temp=& _tmp422;_push_region(temp);{
struct _tuple23 _tmp423=Cyc_NewControlFlow_anal_unordered_Rexps(temp,env,inflow,es,0,0);struct _tuple23 _tmp424=_tmp423;union Cyc_CfFlowInfo_FlowInfo _tmp426;struct Cyc_List_List*_tmp425;_LL35B: _tmp426=_tmp424.f1;_tmp425=_tmp424.f2;_LL35C:;
r1=(void*)((struct Cyc_List_List*)_check_null(_tmp425))->hd;
r2=(void*)((struct Cyc_List_List*)_check_null(_tmp425->tl))->hd;
f=_tmp426;}{
# 1979
union Cyc_CfFlowInfo_FlowInfo _tmp427=f;struct Cyc_Dict_Dict _tmp471;struct Cyc_List_List*_tmp470;if((_tmp427.BottomFL).tag == 1){_LL35E: _LL35F: {
struct _tuple19 _tmp429=({struct _tuple19 _tmp428;_tmp428.f1=f;_tmp428.f2=f;_tmp428;});_npop_handler(0);return _tmp429;}}else{_LL360: _tmp471=((_tmp427.ReachableFL).val).f1;_tmp470=((_tmp427.ReachableFL).val).f2;_LL361: {
# 1982
struct Cyc_Absyn_Exp*_tmp42A=(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es))->hd;
struct Cyc_Absyn_Exp*_tmp42B=(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es->tl))->hd;
if(Cyc_CfFlowInfo_initlevel(env->fenv,_tmp471,r1)== Cyc_CfFlowInfo_NoneIL)
({void*_tmp42C=0;Cyc_CfFlowInfo_aerr(((struct Cyc_Absyn_Exp*)es->hd)->loc,({const char*_tmp42D="expression may not be initialized";_tag_dyneither(_tmp42D,sizeof(char),34);}),_tag_dyneither(_tmp42C,sizeof(void*),0));});
if(Cyc_CfFlowInfo_initlevel(env->fenv,_tmp471,r2)== Cyc_CfFlowInfo_NoneIL)
({void*_tmp42E=0;Cyc_CfFlowInfo_aerr(((struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es->tl))->hd)->loc,({const char*_tmp42F="expression may not be initialized";_tag_dyneither(_tmp42F,sizeof(char),34);}),_tag_dyneither(_tmp42E,sizeof(void*),0));});{
# 1989
union Cyc_CfFlowInfo_FlowInfo _tmp430=f;
union Cyc_CfFlowInfo_FlowInfo _tmp431=f;
# 1994
if(p == Cyc_Absyn_Eq  || p == Cyc_Absyn_Neq){
struct _tuple15 _tmp432=Cyc_CfFlowInfo_unname_rval((env->fenv)->r,r1);struct _tuple15 _tmp433=_tmp432;void*_tmp44E;struct Cyc_List_List*_tmp44D;_LL363: _tmp44E=_tmp433.f1;_tmp44D=_tmp433.f2;_LL364:;{
struct _tuple15 _tmp434=Cyc_CfFlowInfo_unname_rval((env->fenv)->r,r2);struct _tuple15 _tmp435=_tmp434;void*_tmp44C;struct Cyc_List_List*_tmp44B;_LL366: _tmp44C=_tmp435.f1;_tmp44B=_tmp435.f2;_LL367:;{
struct _tuple0 _tmp436=({struct _tuple0 _tmp44A;_tmp44A.f1=_tmp44E;_tmp44A.f2=_tmp44C;_tmp44A;});struct _tuple0 _tmp437=_tmp436;enum Cyc_CfFlowInfo_InitLevel _tmp449;enum Cyc_CfFlowInfo_InitLevel _tmp448;switch(*((int*)_tmp437.f1)){case 3: if(((struct Cyc_CfFlowInfo_Zero_CfFlowInfo_AbsRVal_struct*)_tmp437.f2)->tag == 0){_LL369: _tmp448=((struct Cyc_CfFlowInfo_UnknownR_CfFlowInfo_AbsRVal_struct*)_tmp437.f1)->f1;_LL36A: {
# 2001
struct _tuple19 _tmp438=Cyc_NewControlFlow_splitzero(env,f,f,_tmp42A,_tmp448,_tmp44D);struct _tuple19 _tmp439=_tmp438;union Cyc_CfFlowInfo_FlowInfo _tmp43F;union Cyc_CfFlowInfo_FlowInfo _tmp43E;_LL37E: _tmp43F=_tmp439.f1;_tmp43E=_tmp439.f2;_LL37F:;
{enum Cyc_Absyn_Primop _tmp43A=p;switch(_tmp43A){case Cyc_Absyn_Eq: _LL381: _LL382:
 _tmp430=_tmp43E;_tmp431=_tmp43F;goto _LL380;case Cyc_Absyn_Neq: _LL383: _LL384:
 _tmp430=_tmp43F;_tmp431=_tmp43E;goto _LL380;default: _LL385: _LL386:
(int)_throw((void*)({struct Cyc_Core_Impossible_exn_struct*_tmp43B=_cycalloc(sizeof(*_tmp43B));_tmp43B[0]=({struct Cyc_Core_Impossible_exn_struct _tmp43C;_tmp43C.tag=Cyc_Core_Impossible;_tmp43C.f1=({const char*_tmp43D="anal_test, zero-split";_tag_dyneither(_tmp43D,sizeof(char),22);});_tmp43C;});_tmp43B;}));}_LL380:;}
# 2007
goto _LL368;}}else{goto _LL37B;}case 0: switch(*((int*)_tmp437.f2)){case 3: _LL36B: _tmp449=((struct Cyc_CfFlowInfo_UnknownR_CfFlowInfo_AbsRVal_struct*)_tmp437.f2)->f1;_LL36C: {
# 2009
struct _tuple19 _tmp440=Cyc_NewControlFlow_splitzero(env,f,f,_tmp42B,_tmp449,_tmp44B);struct _tuple19 _tmp441=_tmp440;union Cyc_CfFlowInfo_FlowInfo _tmp447;union Cyc_CfFlowInfo_FlowInfo _tmp446;_LL388: _tmp447=_tmp441.f1;_tmp446=_tmp441.f2;_LL389:;
{enum Cyc_Absyn_Primop _tmp442=p;switch(_tmp442){case Cyc_Absyn_Eq: _LL38B: _LL38C:
 _tmp430=_tmp446;_tmp431=_tmp447;goto _LL38A;case Cyc_Absyn_Neq: _LL38D: _LL38E:
 _tmp430=_tmp447;_tmp431=_tmp446;goto _LL38A;default: _LL38F: _LL390:
(int)_throw((void*)({struct Cyc_Core_Impossible_exn_struct*_tmp443=_cycalloc(sizeof(*_tmp443));_tmp443[0]=({struct Cyc_Core_Impossible_exn_struct _tmp444;_tmp444.tag=Cyc_Core_Impossible;_tmp444.f1=({const char*_tmp445="anal_test, zero-split";_tag_dyneither(_tmp445,sizeof(char),22);});_tmp444;});_tmp443;}));}_LL38A:;}
# 2015
goto _LL368;}case 0: _LL36D: _LL36E:
# 2017
 if(p == Cyc_Absyn_Eq)_tmp431=Cyc_CfFlowInfo_BottomFL();else{
_tmp430=Cyc_CfFlowInfo_BottomFL();}
goto _LL368;case 1: _LL36F: _LL370:
 goto _LL372;case 2: _LL371: _LL372:
 goto _LL374;case 5: _LL373: _LL374:
 goto _LL376;default: goto _LL37B;}case 1: if(((struct Cyc_CfFlowInfo_Zero_CfFlowInfo_AbsRVal_struct*)_tmp437.f2)->tag == 0){_LL375: _LL376:
 goto _LL378;}else{goto _LL37B;}case 2: if(((struct Cyc_CfFlowInfo_Zero_CfFlowInfo_AbsRVal_struct*)_tmp437.f2)->tag == 0){_LL377: _LL378:
 goto _LL37A;}else{goto _LL37B;}case 5: if(((struct Cyc_CfFlowInfo_Zero_CfFlowInfo_AbsRVal_struct*)_tmp437.f2)->tag == 0){_LL379: _LL37A:
# 2026
 if(p == Cyc_Absyn_Neq)_tmp431=Cyc_CfFlowInfo_BottomFL();else{
_tmp430=Cyc_CfFlowInfo_BottomFL();}
goto _LL368;}else{goto _LL37B;}default: _LL37B: _LL37C:
 goto _LL368;}_LL368:;};};}
# 2037
{struct _tuple0 _tmp44F=({struct _tuple0 _tmp453;_tmp453.f1=Cyc_Tcutil_compress((void*)_check_null(_tmp42A->topt));_tmp453.f2=Cyc_Tcutil_compress((void*)_check_null(_tmp42B->topt));_tmp453;});struct _tuple0 _tmp450=_tmp44F;if(((struct Cyc_Absyn_IntType_Absyn_Type_struct*)_tmp450.f1)->tag == 6){if(((struct Cyc_Absyn_IntType_Absyn_Type_struct*)_tmp450.f1)->f1 == Cyc_Absyn_Unsigned){_LL392: _LL393:
 goto _LL395;}else{switch(*((int*)_tmp450.f2)){case 6: if(((struct Cyc_Absyn_IntType_Absyn_Type_struct*)_tmp450.f2)->f1 == Cyc_Absyn_Unsigned)goto _LL394;else{goto _LL39A;}case 19: goto _LL398;default: goto _LL39A;}}}else{if(((struct Cyc_Absyn_IntType_Absyn_Type_struct*)_tmp450.f2)->tag == 6){if(((struct Cyc_Absyn_IntType_Absyn_Type_struct*)_tmp450.f2)->f1 == Cyc_Absyn_Unsigned){_LL394: _LL395:
 goto _LL397;}else{if(((struct Cyc_Absyn_TagType_Absyn_Type_struct*)_tmp450.f1)->tag == 19)goto _LL396;else{goto _LL39A;}}}else{if(((struct Cyc_Absyn_TagType_Absyn_Type_struct*)_tmp450.f1)->tag == 19){_LL396: _LL397:
 goto _LL399;}else{if(((struct Cyc_Absyn_TagType_Absyn_Type_struct*)_tmp450.f2)->tag == 19){_LL398: _LL399:
 goto _LL391;}else{_LL39A: _LL39B: {
struct _tuple19 _tmp452=({struct _tuple19 _tmp451;_tmp451.f1=_tmp430;_tmp451.f2=_tmp431;_tmp451;});_npop_handler(0);return _tmp452;}}}}}_LL391:;}
# 2046
{enum Cyc_Absyn_Primop _tmp454=p;switch(_tmp454){case Cyc_Absyn_Eq: _LL39D: _LL39E:
 goto _LL3A0;case Cyc_Absyn_Neq: _LL39F: _LL3A0: goto _LL3A2;case Cyc_Absyn_Gt: _LL3A1: _LL3A2: goto _LL3A4;case Cyc_Absyn_Gte: _LL3A3: _LL3A4: goto _LL3A6;case Cyc_Absyn_Lt: _LL3A5: _LL3A6: goto _LL3A8;case Cyc_Absyn_Lte: _LL3A7: _LL3A8: goto _LL39C;default: _LL3A9: _LL3AA: {
struct _tuple19 _tmp456=({struct _tuple19 _tmp455;_tmp455.f1=_tmp430;_tmp455.f2=_tmp431;_tmp455;});_npop_handler(0);return _tmp456;}}_LL39C:;}{
# 2051
struct _RegionHandle*_tmp457=(env->fenv)->r;
struct _tuple13 _tmp458=Cyc_Relations_primop2relation(_tmp42A,p,_tmp42B);struct _tuple13 _tmp459=_tmp458;struct Cyc_Absyn_Exp*_tmp46F;enum Cyc_Relations_Relation _tmp46E;struct Cyc_Absyn_Exp*_tmp46D;_LL3AC: _tmp46F=_tmp459.f1;_tmp46E=_tmp459.f2;_tmp46D=_tmp459.f3;_LL3AD:;{
union Cyc_Relations_RelnOp n1=Cyc_Relations_RConst(0);
union Cyc_Relations_RelnOp n2=Cyc_Relations_RConst(0);
# 2056
if(Cyc_Relations_exp2relnop(_tmp46F,& n1) && Cyc_Relations_exp2relnop(_tmp46D,& n2)){
# 2058
struct Cyc_List_List*_tmp45A=Cyc_Relations_add_relation(_tmp457,n1,_tmp46E,n2,_tmp470);
# 2062
struct Cyc_List_List*_tmp45B=Cyc_Relations_add_relation(_tmp457,n2,Cyc_Relations_flip_relation(_tmp46E),n1,_tmp470);
struct _tuple19 _tmp45C=({struct _tuple19 _tmp46A;_tmp46A.f1=_tmp430;_tmp46A.f2=_tmp431;_tmp46A;});struct _tuple19 _tmp45D=_tmp45C;struct Cyc_Dict_Dict _tmp469;struct Cyc_Dict_Dict _tmp468;struct Cyc_Dict_Dict _tmp467;struct Cyc_Dict_Dict _tmp466;if(((_tmp45D.f1).ReachableFL).tag == 2){if(((_tmp45D.f2).ReachableFL).tag == 2){_LL3AF: _tmp467=(((_tmp45D.f1).ReachableFL).val).f1;_tmp466=(((_tmp45D.f2).ReachableFL).val).f1;_LL3B0: {
# 2065
struct _tuple19 _tmp45F=({struct _tuple19 _tmp45E;_tmp45E.f1=Cyc_CfFlowInfo_ReachableFL(_tmp467,_tmp45A);_tmp45E.f2=Cyc_CfFlowInfo_ReachableFL(_tmp466,_tmp45B);_tmp45E;});_npop_handler(0);return _tmp45F;}}else{_LL3B3: _tmp468=(((_tmp45D.f1).ReachableFL).val).f1;_LL3B4: {
# 2069
struct _tuple19 _tmp463=({struct _tuple19 _tmp462;_tmp462.f1=Cyc_CfFlowInfo_ReachableFL(_tmp468,_tmp45A);_tmp462.f2=_tmp431;_tmp462;});_npop_handler(0);return _tmp463;}}}else{if(((_tmp45D.f2).ReachableFL).tag == 2){_LL3B1: _tmp469=(((_tmp45D.f2).ReachableFL).val).f1;_LL3B2: {
# 2067
struct _tuple19 _tmp461=({struct _tuple19 _tmp460;_tmp460.f1=_tmp430;_tmp460.f2=Cyc_CfFlowInfo_ReachableFL(_tmp469,_tmp45B);_tmp460;});_npop_handler(0);return _tmp461;}}else{_LL3B5: _LL3B6: {
# 2071
struct _tuple19 _tmp465=({struct _tuple19 _tmp464;_tmp464.f1=_tmp430;_tmp464.f2=_tmp431;_tmp464;});_npop_handler(0);return _tmp465;}}}_LL3AE:;}else{
# 2074
struct _tuple19 _tmp46C=({struct _tuple19 _tmp46B;_tmp46B.f1=_tmp430;_tmp46B.f2=_tmp431;_tmp46B;});_npop_handler(0);return _tmp46C;}};};};}}_LL35D:;};
# 1971
;_pop_region(temp);}
# 2080
static struct _tuple19 Cyc_NewControlFlow_anal_test(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_Absyn_Exp*e){
# 2082
struct Cyc_CfFlowInfo_FlowEnv*_tmp472=env->fenv;
void*_tmp473=e->r;void*_tmp474=_tmp473;enum Cyc_Absyn_Primop _tmp4C3;struct Cyc_List_List*_tmp4C2;struct Cyc_Absyn_Exp*_tmp4C1;struct Cyc_Absyn_Exp*_tmp4C0;struct Cyc_Absyn_Exp*_tmp4BF;struct Cyc_Absyn_Exp*_tmp4BE;struct Cyc_Absyn_Exp*_tmp4BD;struct Cyc_Absyn_Exp*_tmp4BC;struct Cyc_Absyn_Exp*_tmp4BB;struct Cyc_Absyn_Exp*_tmp4BA;struct Cyc_Absyn_Exp*_tmp4B9;struct Cyc_Absyn_Exp*_tmp4B8;switch(*((int*)_tmp474)){case 5: _LL3B8: _tmp4BA=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp474)->f1;_tmp4B9=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp474)->f2;_tmp4B8=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp474)->f3;_LL3B9: {
# 2085
struct _tuple19 _tmp475=Cyc_NewControlFlow_anal_test(env,inflow,_tmp4BA);struct _tuple19 _tmp476=_tmp475;union Cyc_CfFlowInfo_FlowInfo _tmp481;union Cyc_CfFlowInfo_FlowInfo _tmp480;_LL3C7: _tmp481=_tmp476.f1;_tmp480=_tmp476.f2;_LL3C8:;{
struct _tuple19 _tmp477=Cyc_NewControlFlow_anal_test(env,_tmp481,_tmp4B9);struct _tuple19 _tmp478=_tmp477;union Cyc_CfFlowInfo_FlowInfo _tmp47F;union Cyc_CfFlowInfo_FlowInfo _tmp47E;_LL3CA: _tmp47F=_tmp478.f1;_tmp47E=_tmp478.f2;_LL3CB:;{
struct _tuple19 _tmp479=Cyc_NewControlFlow_anal_test(env,_tmp480,_tmp4B8);struct _tuple19 _tmp47A=_tmp479;union Cyc_CfFlowInfo_FlowInfo _tmp47D;union Cyc_CfFlowInfo_FlowInfo _tmp47C;_LL3CD: _tmp47D=_tmp47A.f1;_tmp47C=_tmp47A.f2;_LL3CE:;
return({struct _tuple19 _tmp47B;_tmp47B.f1=Cyc_CfFlowInfo_join_flow(_tmp472,env->all_changed,_tmp47F,_tmp47D);_tmp47B.f2=
Cyc_CfFlowInfo_join_flow(_tmp472,env->all_changed,_tmp47E,_tmp47C);_tmp47B;});};};}case 6: _LL3BA: _tmp4BC=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp474)->f1;_tmp4BB=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp474)->f2;_LL3BB: {
# 2091
struct _tuple19 _tmp482=Cyc_NewControlFlow_anal_test(env,inflow,_tmp4BC);struct _tuple19 _tmp483=_tmp482;union Cyc_CfFlowInfo_FlowInfo _tmp48A;union Cyc_CfFlowInfo_FlowInfo _tmp489;_LL3D0: _tmp48A=_tmp483.f1;_tmp489=_tmp483.f2;_LL3D1:;{
struct _tuple19 _tmp484=Cyc_NewControlFlow_anal_test(env,_tmp48A,_tmp4BB);struct _tuple19 _tmp485=_tmp484;union Cyc_CfFlowInfo_FlowInfo _tmp488;union Cyc_CfFlowInfo_FlowInfo _tmp487;_LL3D3: _tmp488=_tmp485.f1;_tmp487=_tmp485.f2;_LL3D4:;
return({struct _tuple19 _tmp486;_tmp486.f1=_tmp488;_tmp486.f2=Cyc_CfFlowInfo_join_flow(_tmp472,env->all_changed,_tmp489,_tmp487);_tmp486;});};}case 7: _LL3BC: _tmp4BE=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp474)->f1;_tmp4BD=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp474)->f2;_LL3BD: {
# 2095
struct _tuple19 _tmp48B=Cyc_NewControlFlow_anal_test(env,inflow,_tmp4BE);struct _tuple19 _tmp48C=_tmp48B;union Cyc_CfFlowInfo_FlowInfo _tmp493;union Cyc_CfFlowInfo_FlowInfo _tmp492;_LL3D6: _tmp493=_tmp48C.f1;_tmp492=_tmp48C.f2;_LL3D7:;{
struct _tuple19 _tmp48D=Cyc_NewControlFlow_anal_test(env,_tmp492,_tmp4BD);struct _tuple19 _tmp48E=_tmp48D;union Cyc_CfFlowInfo_FlowInfo _tmp491;union Cyc_CfFlowInfo_FlowInfo _tmp490;_LL3D9: _tmp491=_tmp48E.f1;_tmp490=_tmp48E.f2;_LL3DA:;
return({struct _tuple19 _tmp48F;_tmp48F.f1=Cyc_CfFlowInfo_join_flow(_tmp472,env->all_changed,_tmp493,_tmp491);_tmp48F.f2=_tmp490;_tmp48F;});};}case 8: _LL3BE: _tmp4C0=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp474)->f1;_tmp4BF=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp474)->f2;_LL3BF: {
# 2099
struct _tuple16 _tmp494=Cyc_NewControlFlow_anal_Rexp(env,0,inflow,_tmp4C0);struct _tuple16 _tmp495=_tmp494;union Cyc_CfFlowInfo_FlowInfo _tmp497;void*_tmp496;_LL3DC: _tmp497=_tmp495.f1;_tmp496=_tmp495.f2;_LL3DD:;
return Cyc_NewControlFlow_anal_test(env,_tmp497,_tmp4BF);}case 2: if(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp474)->f1 == Cyc_Absyn_Not){if(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp474)->f2 != 0){if(((struct Cyc_List_List*)((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp474)->f2)->tl == 0){_LL3C0: _tmp4C1=(struct Cyc_Absyn_Exp*)(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp474)->f2)->hd;_LL3C1: {
# 2102
struct _tuple19 _tmp498=Cyc_NewControlFlow_anal_test(env,inflow,_tmp4C1);struct _tuple19 _tmp499=_tmp498;union Cyc_CfFlowInfo_FlowInfo _tmp49C;union Cyc_CfFlowInfo_FlowInfo _tmp49B;_LL3DF: _tmp49C=_tmp499.f1;_tmp49B=_tmp499.f2;_LL3E0:;
return({struct _tuple19 _tmp49A;_tmp49A.f1=_tmp49B;_tmp49A.f2=_tmp49C;_tmp49A;});}}else{goto _LL3C2;}}else{goto _LL3C2;}}else{_LL3C2: _tmp4C3=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp474)->f1;_tmp4C2=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp474)->f2;_LL3C3:
# 2105
 return Cyc_NewControlFlow_anal_primop_test(env,inflow,_tmp4C3,_tmp4C2);}default: _LL3C4: _LL3C5: {
# 2109
struct _tuple16 _tmp49D=Cyc_NewControlFlow_anal_Rexp(env,0,inflow,e);struct _tuple16 _tmp49E=_tmp49D;union Cyc_CfFlowInfo_FlowInfo _tmp4B7;void*_tmp4B6;_LL3E2: _tmp4B7=_tmp49E.f1;_tmp4B6=_tmp49E.f2;_LL3E3:;{
union Cyc_CfFlowInfo_FlowInfo _tmp49F=_tmp4B7;struct Cyc_Dict_Dict _tmp4B5;if((_tmp49F.BottomFL).tag == 1){_LL3E5: _LL3E6:
 return({struct _tuple19 _tmp4A0;_tmp4A0.f1=_tmp4B7;_tmp4A0.f2=_tmp4B7;_tmp4A0;});}else{_LL3E7: _tmp4B5=((_tmp49F.ReachableFL).val).f1;_LL3E8: {
# 2113
struct _tuple15 _tmp4A1=Cyc_CfFlowInfo_unname_rval((env->fenv)->r,_tmp4B6);struct _tuple15 _tmp4A2=_tmp4A1;void*_tmp4B4;struct Cyc_List_List*_tmp4B3;_LL3EA: _tmp4B4=_tmp4A2.f1;_tmp4B3=_tmp4A2.f2;_LL3EB:;{
void*_tmp4A3=_tmp4B4;enum Cyc_CfFlowInfo_InitLevel _tmp4B2;struct Cyc_Absyn_Vardecl*_tmp4B1;void*_tmp4B0;switch(*((int*)_tmp4A3)){case 8: _LL3ED: _tmp4B1=((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp4A3)->f1;_tmp4B0=(void*)((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp4A3)->f2;_LL3EE:
# 2116
(int)_throw((void*)({struct Cyc_Core_Impossible_exn_struct*_tmp4A4=_cycalloc(sizeof(*_tmp4A4));_tmp4A4[0]=({struct Cyc_Core_Impossible_exn_struct _tmp4A5;_tmp4A5.tag=Cyc_Core_Impossible;_tmp4A5.f1=({const char*_tmp4A6="anal_test: bad namedlocation";_tag_dyneither(_tmp4A6,sizeof(char),29);});_tmp4A5;});_tmp4A4;}));case 0: _LL3EF: _LL3F0:
 return({struct _tuple19 _tmp4A7;_tmp4A7.f1=Cyc_CfFlowInfo_BottomFL();_tmp4A7.f2=_tmp4B7;_tmp4A7;});case 2: _LL3F1: _LL3F2:
 goto _LL3F4;case 1: _LL3F3: _LL3F4:
 goto _LL3F6;case 5: _LL3F5: _LL3F6:
 return({struct _tuple19 _tmp4A8;_tmp4A8.f1=_tmp4B7;_tmp4A8.f2=Cyc_CfFlowInfo_BottomFL();_tmp4A8;});case 3: if(((struct Cyc_CfFlowInfo_UnknownR_CfFlowInfo_AbsRVal_struct*)_tmp4A3)->f1 == Cyc_CfFlowInfo_NoneIL){_LL3F7: _LL3F8:
 goto _LL3FA;}else{_LL3FD: _tmp4B2=((struct Cyc_CfFlowInfo_UnknownR_CfFlowInfo_AbsRVal_struct*)_tmp4A3)->f1;_LL3FE:
# 2126
 return Cyc_NewControlFlow_splitzero(env,inflow,_tmp4B7,e,_tmp4B2,_tmp4B3);}case 4: if(((struct Cyc_CfFlowInfo_Esc_CfFlowInfo_AbsRVal_struct*)_tmp4A3)->f1 == Cyc_CfFlowInfo_NoneIL){_LL3F9: _LL3FA:
# 2122
 goto _LL3FC;}else{_LL3FF: _LL400:
# 2127
 return({struct _tuple19 _tmp4AC;_tmp4AC.f1=_tmp4B7;_tmp4AC.f2=_tmp4B7;_tmp4AC;});}case 7: _LL3FB: _LL3FC:
# 2124
({void*_tmp4A9=0;Cyc_CfFlowInfo_aerr(e->loc,({const char*_tmp4AA="expression may not be initialized";_tag_dyneither(_tmp4AA,sizeof(char),34);}),_tag_dyneither(_tmp4A9,sizeof(void*),0));});
return({struct _tuple19 _tmp4AB;_tmp4AB.f1=Cyc_CfFlowInfo_BottomFL();_tmp4AB.f2=Cyc_CfFlowInfo_BottomFL();_tmp4AB;});default: _LL401: _LL402:
# 2128
(int)_throw((void*)({struct Cyc_Core_Impossible_exn_struct*_tmp4AD=_cycalloc(sizeof(*_tmp4AD));_tmp4AD[0]=({struct Cyc_Core_Impossible_exn_struct _tmp4AE;_tmp4AE.tag=Cyc_Core_Impossible;_tmp4AE.f1=({const char*_tmp4AF="anal_test";_tag_dyneither(_tmp4AF,sizeof(char),10);});_tmp4AE;});_tmp4AD;}));}_LL3EC:;};}}_LL3E4:;};}}_LL3B7:;}struct _tuple27{unsigned int f1;struct Cyc_NewControlFlow_AnalEnv*f2;struct Cyc_Dict_Dict f3;};
# 2134
static void Cyc_NewControlFlow_check_for_unused_unique(struct _tuple27*ckenv,void*root,void*rval){
# 2136
struct _tuple27*_tmp4C4=ckenv;unsigned int _tmp4D0;struct Cyc_NewControlFlow_AnalEnv*_tmp4CF;struct Cyc_Dict_Dict _tmp4CE;_LL404: _tmp4D0=_tmp4C4->f1;_tmp4CF=_tmp4C4->f2;_tmp4CE=_tmp4C4->f3;_LL405:;{
void*_tmp4C5=root;struct Cyc_Absyn_Vardecl*_tmp4CD;if(((struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*)_tmp4C5)->tag == 0){_LL407: _tmp4CD=((struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*)_tmp4C5)->f1;_LL408:
# 2139
 if(Cyc_Tcutil_is_noalias_pointer_or_aggr((_tmp4CF->fenv)->r,_tmp4CD->type)){
struct _tuple15 _tmp4C6=Cyc_CfFlowInfo_unname_rval((_tmp4CF->fenv)->r,rval);struct _tuple15 _tmp4C7=_tmp4C6;void*_tmp4CC;_LL40C: _tmp4CC=_tmp4C7.f1;_LL40D:;{
void*_tmp4C8=_tmp4CC;switch(*((int*)_tmp4C8)){case 7: _LL40F: _LL410:
 goto _LL412;case 0: _LL411: _LL412:
 goto _LL414;case 3: if(((struct Cyc_CfFlowInfo_UnknownR_CfFlowInfo_AbsRVal_struct*)_tmp4C8)->f1 == Cyc_CfFlowInfo_NoneIL){_LL413: _LL414:
 goto _LL40E;}else{goto _LL415;}default: _LL415: _LL416:
# 2146
({struct Cyc_String_pa_PrintArg_struct _tmp4CB;_tmp4CB.tag=0;_tmp4CB.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_qvar2string(_tmp4CD->name));({void*_tmp4C9[1]={& _tmp4CB};Cyc_Tcutil_warn(_tmp4D0,({const char*_tmp4CA="unique pointers reachable from %s may become unreachable";_tag_dyneither(_tmp4CA,sizeof(char),57);}),_tag_dyneither(_tmp4C9,sizeof(void*),1));});});
goto _LL40E;}_LL40E:;};}
# 2150
goto _LL406;}else{_LL409: _LL40A:
 goto _LL406;}_LL406:;};}
# 2155
static void Cyc_NewControlFlow_check_init_params(unsigned int loc,struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo flow){
union Cyc_CfFlowInfo_FlowInfo _tmp4D1=flow;struct Cyc_Dict_Dict _tmp4D7;if((_tmp4D1.BottomFL).tag == 1){_LL418: _LL419:
 return;}else{_LL41A: _tmp4D7=((_tmp4D1.ReachableFL).val).f1;_LL41B:
# 2159
{struct Cyc_List_List*_tmp4D2=env->param_roots;for(0;_tmp4D2 != 0;_tmp4D2=_tmp4D2->tl){
if(Cyc_CfFlowInfo_initlevel(env->fenv,_tmp4D7,Cyc_CfFlowInfo_lookup_place(_tmp4D7,(struct Cyc_CfFlowInfo_Place*)_tmp4D2->hd))!= Cyc_CfFlowInfo_AllIL)
({void*_tmp4D3=0;Cyc_CfFlowInfo_aerr(loc,({const char*_tmp4D4="function may not initialize all the parameters with attribute 'initializes'";_tag_dyneither(_tmp4D4,sizeof(char),76);}),_tag_dyneither(_tmp4D3,sizeof(void*),0));});}}
# 2163
if(Cyc_NewControlFlow_warn_lose_unique){
struct _tuple27 _tmp4D5=({struct _tuple27 _tmp4D6;_tmp4D6.f1=loc;_tmp4D6.f2=env;_tmp4D6.f3=_tmp4D7;_tmp4D6;});
((void(*)(void(*f)(struct _tuple27*,void*,void*),struct _tuple27*env,struct Cyc_Dict_Dict d))Cyc_Dict_iter_c)(Cyc_NewControlFlow_check_for_unused_unique,& _tmp4D5,_tmp4D7);}
# 2167
return;}_LL417:;}
# 2176
static struct _tuple1 Cyc_NewControlFlow_get_unconsume_pat_vars(struct _RegionHandle*frgn,struct Cyc_List_List*vds){
# 2179
struct Cyc_List_List*_tmp4D8=0;
struct Cyc_List_List*_tmp4D9=0;
{struct Cyc_List_List*x=vds;for(0;x != 0;x=x->tl){
struct _tuple24*_tmp4DA=(struct _tuple24*)x->hd;struct _tuple24*_tmp4DB=_tmp4DA;struct Cyc_Absyn_Vardecl**_tmp4E4;struct Cyc_Absyn_Exp*_tmp4E3;_LL41D: _tmp4E4=_tmp4DB->f1;_tmp4E3=_tmp4DB->f2;_LL41E:;
if((_tmp4E4 != 0  && _tmp4E3 != 0) && 
Cyc_Tcutil_is_noalias_pointer((void*)_check_null(_tmp4E3->topt),0)){
struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*_tmp4DC=({struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*_tmp4E1=_region_malloc(frgn,sizeof(*_tmp4E1));_tmp4E1[0]=({struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct _tmp4E2;_tmp4E2.tag=0;_tmp4E2.f1=*_tmp4E4;_tmp4E2;});_tmp4E1;});
struct Cyc_CfFlowInfo_Place*_tmp4DD=({struct Cyc_CfFlowInfo_Place*_tmp4E0=_region_malloc(frgn,sizeof(*_tmp4E0));_tmp4E0->root=(void*)_tmp4DC;_tmp4E0->fields=0;_tmp4E0;});
_tmp4D8=({struct Cyc_List_List*_tmp4DE=_region_malloc(frgn,sizeof(*_tmp4DE));_tmp4DE->hd=_tmp4DD;_tmp4DE->tl=_tmp4D8;_tmp4DE;});
_tmp4D9=({struct Cyc_List_List*_tmp4DF=_region_malloc(frgn,sizeof(*_tmp4DF));_tmp4DF->hd=_tmp4E3;_tmp4DF->tl=_tmp4D9;_tmp4DF;});}}}
# 2191
return({struct _tuple1 _tmp4E5;_tmp4E5.f1=_tmp4D8;_tmp4E5.f2=_tmp4D9;_tmp4E5;});}struct _tuple28{int f1;void*f2;};
# 2197
static struct _tuple28 Cyc_NewControlFlow_noconsume_place_ok(struct Cyc_NewControlFlow_AnalEnv*env,struct Cyc_CfFlowInfo_Place*place,int do_unconsume,struct Cyc_Absyn_Vardecl*vd,union Cyc_CfFlowInfo_FlowInfo flow,unsigned int loc){
# 2204
union Cyc_CfFlowInfo_FlowInfo _tmp4E6=flow;struct Cyc_Dict_Dict _tmp4FB;if((_tmp4E6.BottomFL).tag == 1){_LL420: _LL421:
# 2206
({void*_tmp4E7=0;((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(({const char*_tmp4E8="noconsume_place_ok: flow became Bottom!";_tag_dyneither(_tmp4E8,sizeof(char),40);}),_tag_dyneither(_tmp4E7,sizeof(void*),0));});}else{_LL422: _tmp4FB=((_tmp4E6.ReachableFL).val).f1;_LL423: {
# 2213
struct Cyc_Absyn_Exp*_tmp4E9=Cyc_Absyn_exp_unsigned_one;
int _tmp4EA=0;
int _tmp4EB=1;
void*_tmp4EC=Cyc_CfFlowInfo_lookup_place(_tmp4FB,place);
void*_tmp4ED=_tmp4EC;
# 2225
int varok=0;
{void*_tmp4EE=_tmp4EC;struct Cyc_Absyn_Vardecl*_tmp4F9;void*_tmp4F8;if(((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp4EE)->tag == 8){_LL425: _tmp4F9=((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp4EE)->f1;_tmp4F8=(void*)((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp4EE)->f2;_LL426:
# 2228
 if(vd == _tmp4F9){
_tmp4ED=_tmp4F8;
# 2231
if(Cyc_Tcutil_is_noalias_pointer_or_aggr(env->r,vd->type)){
# 2233
if(Cyc_CfFlowInfo_is_unique_consumed(_tmp4E9,_tmp4EB,_tmp4ED,& _tmp4EA)){
if(!do_unconsume)
({struct Cyc_String_pa_PrintArg_struct _tmp4F1;_tmp4F1.tag=0;_tmp4F1.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
# 2237
Cyc_Absynpp_qvar2string(vd->name));({void*_tmp4EF[1]={& _tmp4F1};Cyc_CfFlowInfo_aerr(loc,({const char*_tmp4F0="function consumes parameter %s with attribute 'noconsume'";_tag_dyneither(_tmp4F0,sizeof(char),58);}),_tag_dyneither(_tmp4EF,sizeof(void*),1));});});}else{
# 2240
if(Cyc_CfFlowInfo_initlevel(env->fenv,_tmp4FB,_tmp4ED)!= Cyc_CfFlowInfo_AllIL  && !do_unconsume)
({struct Cyc_String_pa_PrintArg_struct _tmp4F4;_tmp4F4.tag=0;_tmp4F4.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
# 2243
Cyc_Absynpp_qvar2string(vd->name));({void*_tmp4F2[1]={& _tmp4F4};Cyc_CfFlowInfo_aerr(loc,({const char*_tmp4F3="function consumes value pointed to by parameter %s, which has attribute 'noconsume'";_tag_dyneither(_tmp4F3,sizeof(char),84);}),_tag_dyneither(_tmp4F2,sizeof(void*),1));});});else{
# 2245
varok=1;}}}else{
# 2248
varok=1;}}else{
# 2251
goto _LL428;}
goto _LL424;}else{_LL427: _LL428:
# 2255
 if(!Cyc_Tcutil_is_noalias_pointer_or_aggr(env->r,vd->type))
varok=1;else{
if(!do_unconsume)
({struct Cyc_String_pa_PrintArg_struct _tmp4F7;_tmp4F7.tag=0;_tmp4F7.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
# 2260
Cyc_Absynpp_qvar2string(vd->name));({void*_tmp4F5[1]={& _tmp4F7};Cyc_CfFlowInfo_aerr(loc,({const char*_tmp4F6="function parameter %s with attribute 'noconsume' no longer set to its original value";_tag_dyneither(_tmp4F6,sizeof(char),85);}),_tag_dyneither(_tmp4F5,sizeof(void*),1));});});}
goto _LL424;}_LL424:;}
# 2264
return({struct _tuple28 _tmp4FA;_tmp4FA.f1=varok;_tmp4FA.f2=_tmp4ED;_tmp4FA;});}}_LL41F:;}
# 2270
static struct Cyc_Absyn_Vardecl*Cyc_NewControlFlow_get_vd_from_place(struct Cyc_CfFlowInfo_Place*p){
struct Cyc_CfFlowInfo_Place*_tmp4FC=p;void*_tmp504;struct Cyc_List_List*_tmp503;_LL42A: _tmp504=_tmp4FC->root;_tmp503=_tmp4FC->fields;_LL42B:;
if(_tmp503 != 0)
({void*_tmp4FD=0;((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(({const char*_tmp4FE="unconsume_params: param to unconsume is non-variable";_tag_dyneither(_tmp4FE,sizeof(char),53);}),_tag_dyneither(_tmp4FD,sizeof(void*),0));});{
struct Cyc_Absyn_Vardecl*vd;
void*_tmp4FF=_tmp504;struct Cyc_Absyn_Vardecl*_tmp502;if(((struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*)_tmp4FF)->tag == 0){_LL42D: _tmp502=((struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*)_tmp4FF)->f1;_LL42E:
 return _tmp502;}else{_LL42F: _LL430:
({void*_tmp500=0;((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(({const char*_tmp501="unconsume_params: root is not a varroot";_tag_dyneither(_tmp501,sizeof(char),40);}),_tag_dyneither(_tmp500,sizeof(void*),0));});}_LL42C:;};}
# 2289 "new_control_flow.cyc"
static union Cyc_CfFlowInfo_FlowInfo Cyc_NewControlFlow_unconsume_exp(struct Cyc_NewControlFlow_AnalEnv*env,struct Cyc_Absyn_Exp*unconsume_exp,struct Cyc_Absyn_Vardecl*vd,int varok,void*ropt,union Cyc_CfFlowInfo_FlowInfo flow,unsigned int loc){
# 2297
{union Cyc_CfFlowInfo_FlowInfo _tmp505=flow;struct Cyc_Dict_Dict _tmp51E;if((_tmp505.BottomFL).tag == 1){_LL432: _LL433:
 return flow;}else{_LL434: _tmp51E=((_tmp505.ReachableFL).val).f1;_LL435: {
# 2304
struct _tuple18 _tmp506=Cyc_NewControlFlow_anal_Lexp(env,flow,0,1,unconsume_exp);struct _tuple18 _tmp507=_tmp506;union Cyc_CfFlowInfo_FlowInfo _tmp51D;union Cyc_CfFlowInfo_AbsLVal _tmp51C;_LL437: _tmp51D=_tmp507.f1;_tmp51C=_tmp507.f2;_LL438:;
# 2307
{union Cyc_CfFlowInfo_AbsLVal _tmp508=_tmp51C;struct Cyc_CfFlowInfo_Place*_tmp51B;if((_tmp508.PlaceL).tag == 1){_LL43A: _tmp51B=(_tmp508.PlaceL).val;_LL43B: {
# 2311
void*_tmp509=Cyc_CfFlowInfo_lookup_place(_tmp51E,_tmp51B);
{void*_tmp50A=_tmp509;struct Cyc_Absyn_Vardecl*_tmp51A;void*_tmp519;if(((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp50A)->tag == 8){_LL43F: _tmp51A=((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp50A)->f1;_tmp519=(void*)((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp50A)->f2;_LL440: {
# 2317
void*new_rval;
if(_tmp51A == vd){
# 2321
if(varok){
# 2323
_tmp509=Cyc_CfFlowInfo_make_unique_unconsumed(env->fenv,_tmp519);
# 2328
if(ropt != 0){
# 2334
struct _tuple16 _tmp50B=
Cyc_CfFlowInfo_join_flow_and_rval(env->fenv,env->all_changed,({struct _tuple16 _tmp50F;_tmp50F.f1=_tmp51D;_tmp50F.f2=_tmp509;_tmp50F;}),({struct _tuple16 _tmp510;_tmp510.f1=_tmp51D;_tmp510.f2=ropt;_tmp510;}));
# 2334
struct _tuple16 _tmp50C=_tmp50B;union Cyc_CfFlowInfo_FlowInfo _tmp50E;void*_tmp50D;_LL444: _tmp50E=_tmp50C.f1;_tmp50D=_tmp50C.f2;_LL445:;
# 2339
_tmp51D=_tmp50E;new_rval=_tmp50D;}else{
# 2344
new_rval=_tmp509;}}else{
# 2347
new_rval=_tmp519;}
# 2349
{union Cyc_CfFlowInfo_FlowInfo _tmp511=_tmp51D;struct Cyc_Dict_Dict _tmp515;struct Cyc_List_List*_tmp514;if((_tmp511.ReachableFL).tag == 2){_LL447: _tmp515=((_tmp511.ReachableFL).val).f1;_tmp514=((_tmp511.ReachableFL).val).f2;_LL448:
# 2351
 flow=Cyc_CfFlowInfo_ReachableFL(Cyc_CfFlowInfo_assign_place(env->fenv,loc,_tmp515,env->all_changed,_tmp51B,new_rval),_tmp514);
# 2355
goto _LL446;}else{_LL449: _LL44A:
# 2357
({void*_tmp512=0;((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(({const char*_tmp513="unconsume_params: joined flow became bot!";_tag_dyneither(_tmp513,sizeof(char),42);}),_tag_dyneither(_tmp512,sizeof(void*),0));});}_LL446:;}
# 2359
goto _LL43E;}else{
# 2361
goto _LL442;}
goto _LL43E;}}else{_LL441: _LL442:
# 2368
 if(ropt != 0  && !
Cyc_Tcutil_is_noalias_pointer_or_aggr(env->r,vd->type))
({struct Cyc_String_pa_PrintArg_struct _tmp518;_tmp518.tag=0;_tmp518.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
Cyc_Absynpp_exp2string(unconsume_exp));({void*_tmp516[1]={& _tmp518};Cyc_CfFlowInfo_aerr(loc,({const char*_tmp517="aliased expression %s was overwritten";_tag_dyneither(_tmp517,sizeof(char),38);}),_tag_dyneither(_tmp516,sizeof(void*),1));});});
# 2385 "new_control_flow.cyc"
goto _LL43E;}_LL43E:;}
# 2387
goto _LL439;}}else{_LL43C: _LL43D:
# 2393
 goto _LL439;}_LL439:;}
# 2395
goto _LL431;}}_LL431:;}
# 2397
return flow;}
# 2410 "new_control_flow.cyc"
static union Cyc_CfFlowInfo_FlowInfo Cyc_NewControlFlow_unconsume_params(struct Cyc_NewControlFlow_AnalEnv*env,struct Cyc_List_List*consumed_vals,struct Cyc_List_List*unconsume_exps,int is_region_open,union Cyc_CfFlowInfo_FlowInfo flow,unsigned int loc){
# 2416
{union Cyc_CfFlowInfo_FlowInfo _tmp51F=flow;if((_tmp51F.BottomFL).tag == 1){_LL44C: _LL44D:
 return flow;}else{_LL44E: _LL44F:
 goto _LL44B;}_LL44B:;}{
# 2420
int _tmp520=unconsume_exps != 0;
{struct Cyc_List_List*_tmp521=consumed_vals;for(0;_tmp521 != 0;
(_tmp521=_tmp521->tl,
unconsume_exps != 0?unconsume_exps=unconsume_exps->tl: 0)){
# 2427
struct Cyc_Absyn_Vardecl*_tmp522=Cyc_NewControlFlow_get_vd_from_place((struct Cyc_CfFlowInfo_Place*)_tmp521->hd);
struct _tuple28 _tmp523=
is_region_open?({struct _tuple28 _tmp527;_tmp527.f1=1;_tmp527.f2=0;_tmp527;}):
 Cyc_NewControlFlow_noconsume_place_ok(env,(struct Cyc_CfFlowInfo_Place*)_tmp521->hd,_tmp520,_tmp522,flow,loc);
# 2428
struct _tuple28 _tmp524=_tmp523;int _tmp526;void*_tmp525;_LL451: _tmp526=_tmp524.f1;_tmp525=_tmp524.f2;_LL452:;
# 2434
if(_tmp520)
flow=Cyc_NewControlFlow_unconsume_exp(env,(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(unconsume_exps))->hd,_tmp522,_tmp526,_tmp525,flow,loc);}}
# 2437
Cyc_NewControlFlow_update_tryflow(env,flow);
return flow;};}struct _tuple29{int f1;struct Cyc_Absyn_Stmt*f2;struct Cyc_List_List*f3;struct Cyc_List_List*f4;};
# 2441
static union Cyc_CfFlowInfo_FlowInfo Cyc_NewControlFlow_anal_scs(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_List_List*scs,unsigned int exp_loc){
# 2444
struct Cyc_CfFlowInfo_FlowEnv*_tmp528=env->fenv;
for(0;scs != 0;scs=scs->tl){
struct Cyc_Absyn_Switch_clause*_tmp529=(struct Cyc_Absyn_Switch_clause*)scs->hd;struct Cyc_Absyn_Switch_clause*_tmp52A=_tmp529;struct Cyc_Core_Opt*_tmp53E;struct Cyc_Absyn_Exp*_tmp53D;struct Cyc_Absyn_Stmt*_tmp53C;unsigned int _tmp53B;_LL454: _tmp53E=_tmp52A->pat_vars;_tmp53D=_tmp52A->where_clause;_tmp53C=_tmp52A->body;_tmp53B=_tmp52A->loc;_LL455:;{
struct _tuple1 _tmp52B=Cyc_NewControlFlow_get_unconsume_pat_vars((env->fenv)->r,(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(_tmp53E))->v);struct _tuple1 _tmp52C=_tmp52B;struct Cyc_List_List*_tmp53A;struct Cyc_List_List*_tmp539;_LL457: _tmp53A=_tmp52C.f1;_tmp539=_tmp52C.f2;_LL458:;{
union Cyc_CfFlowInfo_FlowInfo clause_inflow=Cyc_NewControlFlow_initialize_pat_vars(env->fenv,env,inflow,(struct Cyc_List_List*)_tmp53E->v,_tmp53A != 0,_tmp53B,exp_loc);
# 2452
union Cyc_CfFlowInfo_FlowInfo clause_outflow;
struct Cyc_List_List*_tmp52D=env->unique_pat_vars;
# 2455
if(Cyc_Tcpat_has_vars(_tmp53E))
env->unique_pat_vars=({struct Cyc_List_List*_tmp52E=_region_malloc(env->r,sizeof(*_tmp52E));_tmp52E->hd=({struct _tuple29*_tmp52F=_region_malloc(env->r,sizeof(*_tmp52F));_tmp52F->f1=0;_tmp52F->f2=_tmp53C;_tmp52F->f3=_tmp53A;_tmp52F->f4=_tmp539;_tmp52F;});_tmp52E->tl=_tmp52D;_tmp52E;});
# 2460
if(_tmp53D != 0){
struct Cyc_Absyn_Exp*wexp=_tmp53D;
struct _tuple19 _tmp530=Cyc_NewControlFlow_anal_test(env,clause_inflow,wexp);struct _tuple19 _tmp531=_tmp530;union Cyc_CfFlowInfo_FlowInfo _tmp533;union Cyc_CfFlowInfo_FlowInfo _tmp532;_LL45A: _tmp533=_tmp531.f1;_tmp532=_tmp531.f2;_LL45B:;
inflow=_tmp532;
clause_outflow=Cyc_NewControlFlow_anal_stmt(env,_tmp533,_tmp53C,0);}else{
# 2466
clause_outflow=Cyc_NewControlFlow_anal_stmt(env,clause_inflow,_tmp53C,0);}
# 2468
env->unique_pat_vars=_tmp52D;{
union Cyc_CfFlowInfo_FlowInfo _tmp534=clause_outflow;if((_tmp534.BottomFL).tag == 1){_LL45D: _LL45E:
 goto _LL45C;}else{_LL45F: _LL460:
# 2473
 clause_outflow=Cyc_NewControlFlow_unconsume_params(env,_tmp53A,_tmp539,0,clause_outflow,_tmp53B);
# 2475
if(scs->tl == 0)
return clause_outflow;else{
# 2480
if((struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(((struct Cyc_Absyn_Switch_clause*)((struct Cyc_List_List*)_check_null(scs->tl))->hd)->pat_vars))->v != 0)
({void*_tmp535=0;Cyc_CfFlowInfo_aerr(_tmp53C->loc,({const char*_tmp536="switch clause may implicitly fallthru";_tag_dyneither(_tmp536,sizeof(char),38);}),_tag_dyneither(_tmp535,sizeof(void*),0));});else{
# 2483
({void*_tmp537=0;Cyc_Tcutil_warn(_tmp53C->loc,({const char*_tmp538="switch clause may implicitly fallthru";_tag_dyneither(_tmp538,sizeof(char),38);}),_tag_dyneither(_tmp537,sizeof(void*),0));});}
# 2485
Cyc_NewControlFlow_update_flow(env,((struct Cyc_Absyn_Switch_clause*)((struct Cyc_List_List*)_check_null(scs->tl))->hd)->body,clause_outflow);}
# 2487
goto _LL45C;}_LL45C:;};};};}
# 2490
return Cyc_CfFlowInfo_BottomFL();}struct _tuple30{struct Cyc_NewControlFlow_AnalEnv*f1;struct Cyc_Dict_Dict f2;unsigned int f3;};
# 2495
static void Cyc_NewControlFlow_check_dropped_unique_vd(struct _tuple30*vdenv,struct Cyc_Absyn_Vardecl*vd){
# 2497
struct _tuple30*_tmp53F=vdenv;struct Cyc_NewControlFlow_AnalEnv*_tmp54A;struct Cyc_Dict_Dict _tmp549;unsigned int _tmp548;_LL462: _tmp54A=_tmp53F->f1;_tmp549=_tmp53F->f2;_tmp548=_tmp53F->f3;_LL463:;
if(Cyc_Tcutil_is_noalias_pointer_or_aggr((_tmp54A->fenv)->r,vd->type)){
# 2500
struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct vdroot=({struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct _tmp547;_tmp547.tag=0;_tmp547.f1=vd;_tmp547;});
# 2502
struct _tuple15 _tmp540=Cyc_CfFlowInfo_unname_rval((_tmp54A->fenv)->r,
((void*(*)(struct Cyc_Dict_Dict d,int(*cmp)(void*,void*),void*k))Cyc_Dict_lookup_other)(_tmp549,Cyc_CfFlowInfo_root_cmp,(void*)& vdroot));
# 2502
struct _tuple15 _tmp541=_tmp540;void*_tmp546;_LL465: _tmp546=_tmp541.f1;_LL466:;{
# 2504
void*_tmp542=_tmp546;switch(*((int*)_tmp542)){case 7: _LL468: _LL469:
 goto _LL46B;case 0: _LL46A: _LL46B:
 goto _LL46D;case 3: if(((struct Cyc_CfFlowInfo_UnknownR_CfFlowInfo_AbsRVal_struct*)_tmp542)->f1 == Cyc_CfFlowInfo_NoneIL){_LL46C: _LL46D:
 goto _LL467;}else{goto _LL46E;}default: _LL46E: _LL46F:
# 2509
({struct Cyc_String_pa_PrintArg_struct _tmp545;_tmp545.tag=0;_tmp545.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_qvar2string(vd->name));({void*_tmp543[1]={& _tmp545};Cyc_Tcutil_warn(_tmp548,({const char*_tmp544="unique pointers may still exist after variable %s goes out of scope";_tag_dyneither(_tmp544,sizeof(char),68);}),_tag_dyneither(_tmp543,sizeof(void*),1));});});
# 2511
goto _LL467;}_LL467:;};}}
# 2516
static void Cyc_NewControlFlow_check_dropped_unique(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_Absyn_Decl*decl){
# 2518
{union Cyc_CfFlowInfo_FlowInfo _tmp54B=inflow;struct Cyc_Dict_Dict _tmp558;if((_tmp54B.ReachableFL).tag == 2){_LL471: _tmp558=((_tmp54B.ReachableFL).val).f1;_LL472: {
# 2520
struct _tuple30 _tmp54C=({struct _tuple30 _tmp557;_tmp557.f1=env;_tmp557.f2=_tmp558;_tmp557.f3=decl->loc;_tmp557;});
struct Cyc_CfFlowInfo_FlowEnv*_tmp54D=env->fenv;
{void*_tmp54E=decl->r;void*_tmp54F=_tmp54E;struct Cyc_List_List*_tmp556;struct Cyc_List_List*_tmp555;struct Cyc_Absyn_Vardecl*_tmp554;switch(*((int*)_tmp54F)){case 0: _LL476: _tmp554=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp54F)->f1;_LL477:
# 2524
 Cyc_NewControlFlow_check_dropped_unique_vd(& _tmp54C,_tmp554);
goto _LL475;case 2: if(((struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)_tmp54F)->f2 != 0){_LL478: _tmp555=(struct Cyc_List_List*)(((struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)_tmp54F)->f2)->v;_LL479: {
# 2527
struct _tuple1 _tmp550=((struct _tuple1(*)(struct Cyc_List_List*x))Cyc_List_split)(_tmp555);struct _tuple1 _tmp551=_tmp550;struct Cyc_List_List*_tmp553;_LL47F: _tmp553=_tmp551.f1;_LL480:;{
struct Cyc_List_List*_tmp552=((struct Cyc_List_List*(*)(struct Cyc_List_List*l))Cyc_Tcutil_filter_nulls)(_tmp553);
_tmp556=_tmp552;goto _LL47B;};}}else{goto _LL47C;}case 3: _LL47A: _tmp556=((struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct*)_tmp54F)->f1;_LL47B:
# 2531
((void(*)(void(*f)(struct _tuple30*,struct Cyc_Absyn_Vardecl*),struct _tuple30*env,struct Cyc_List_List*x))Cyc_List_iter_c)(Cyc_NewControlFlow_check_dropped_unique_vd,& _tmp54C,_tmp556);
goto _LL475;default: _LL47C: _LL47D:
 goto _LL475;}_LL475:;}
# 2535
goto _LL470;}}else{_LL473: _LL474:
 goto _LL470;}_LL470:;}
# 2538
return;}
# 2544
static union Cyc_CfFlowInfo_FlowInfo Cyc_NewControlFlow_unconsume_pat_vars(struct Cyc_NewControlFlow_AnalEnv*env,struct Cyc_Absyn_Stmt*src,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_Absyn_Stmt*dest){
# 2548
int num_pop=((int(*)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Stmt*key))Cyc_Hashtable_lookup)(env->pat_pop_table,src);
{struct Cyc_List_List*x=env->unique_pat_vars;for(0;num_pop > 0;(x=x->tl,-- num_pop)){
struct _tuple29*_tmp559=(struct _tuple29*)((struct Cyc_List_List*)_check_null(x))->hd;struct _tuple29*_tmp55A=_tmp559;int _tmp55E;struct Cyc_Absyn_Stmt*_tmp55D;struct Cyc_List_List*_tmp55C;struct Cyc_List_List*_tmp55B;_LL482: _tmp55E=_tmp55A->f1;_tmp55D=_tmp55A->f2;_tmp55C=_tmp55A->f3;_tmp55B=_tmp55A->f4;_LL483:;
inflow=Cyc_NewControlFlow_unconsume_params(env,_tmp55C,_tmp55B,_tmp55E,inflow,dest->loc);}}
# 2553
return inflow;}
# 2559
static union Cyc_CfFlowInfo_FlowInfo Cyc_NewControlFlow_anal_stmt(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_Absyn_Stmt*s,struct _tuple17*rval_opt){
# 2562
union Cyc_CfFlowInfo_FlowInfo outflow;
struct _tuple20 _tmp55F=Cyc_NewControlFlow_pre_stmt_check(env,inflow,s);struct _tuple20 _tmp560=_tmp55F;struct Cyc_NewControlFlow_CFStmtAnnot*_tmp603;union Cyc_CfFlowInfo_FlowInfo*_tmp602;_LL485: _tmp603=_tmp560.f1;_tmp602=_tmp560.f2;_LL486:;
inflow=*_tmp602;{
struct Cyc_CfFlowInfo_FlowEnv*_tmp561=env->fenv;
# 2569
void*_tmp562=s->r;void*_tmp563=_tmp562;struct Cyc_Absyn_Stmt*_tmp601;struct Cyc_Absyn_Decl*_tmp600;struct Cyc_Absyn_Stmt*_tmp5FF;struct Cyc_Absyn_Vardecl*_tmp5FE;struct Cyc_Absyn_Exp*_tmp5FD;unsigned int _tmp5FC;struct Cyc_Absyn_Stmt*_tmp5FB;struct Cyc_List_List*_tmp5FA;struct Cyc_Absyn_Exp*_tmp5F9;unsigned int _tmp5F8;struct Cyc_Absyn_Stmt*_tmp5F7;struct Cyc_Absyn_Stmt*_tmp5F6;struct Cyc_List_List*_tmp5F5;void*_tmp5F4;struct Cyc_Absyn_Exp*_tmp5F3;struct Cyc_List_List*_tmp5F2;void*_tmp5F1;struct Cyc_List_List*_tmp5F0;struct Cyc_Absyn_Switch_clause*_tmp5EF;struct Cyc_Absyn_Exp*_tmp5EE;struct Cyc_Absyn_Exp*_tmp5ED;struct Cyc_Absyn_Stmt*_tmp5EC;struct Cyc_Absyn_Exp*_tmp5EB;struct Cyc_Absyn_Stmt*_tmp5EA;struct Cyc_Absyn_Stmt*_tmp5E9;struct Cyc_Absyn_Stmt*_tmp5E8;struct Cyc_Absyn_Exp*_tmp5E7;struct Cyc_Absyn_Stmt*_tmp5E6;struct Cyc_Absyn_Exp*_tmp5E5;struct Cyc_Absyn_Stmt*_tmp5E4;struct Cyc_Absyn_Stmt*_tmp5E3;struct Cyc_Absyn_Exp*_tmp5E2;struct Cyc_Absyn_Stmt*_tmp5E1;struct Cyc_Absyn_Stmt*_tmp5E0;struct Cyc_Absyn_Stmt*_tmp5DF;struct Cyc_Absyn_Stmt*_tmp5DE;struct Cyc_Absyn_Exp*_tmp5DD;struct Cyc_Absyn_Exp*_tmp5DC;switch(*((int*)_tmp563)){case 0: _LL488: _LL489:
 return inflow;case 3: if(((struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*)_tmp563)->f1 == 0){_LL48A: _LL48B:
# 2573
 if(env->noreturn)
({void*_tmp564=0;Cyc_CfFlowInfo_aerr(s->loc,({const char*_tmp565="`noreturn' function might return";_tag_dyneither(_tmp565,sizeof(char),33);}),_tag_dyneither(_tmp564,sizeof(void*),0));});
Cyc_NewControlFlow_check_init_params(s->loc,env,inflow);
Cyc_NewControlFlow_unconsume_params(env,env->noconsume_params,0,0,inflow,s->loc);
return Cyc_CfFlowInfo_BottomFL();}else{_LL48C: _tmp5DC=((struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*)_tmp563)->f1;_LL48D:
# 2579
 if(env->noreturn)
({void*_tmp566=0;Cyc_CfFlowInfo_aerr(s->loc,({const char*_tmp567="`noreturn' function might return";_tag_dyneither(_tmp567,sizeof(char),33);}),_tag_dyneither(_tmp566,sizeof(void*),0));});{
struct _tuple16 _tmp568=Cyc_NewControlFlow_anal_Rexp(env,1,inflow,(struct Cyc_Absyn_Exp*)_check_null(_tmp5DC));struct _tuple16 _tmp569=_tmp568;union Cyc_CfFlowInfo_FlowInfo _tmp56B;void*_tmp56A;_LL4B1: _tmp56B=_tmp569.f1;_tmp56A=_tmp569.f2;_LL4B2:;
_tmp56B=Cyc_NewControlFlow_use_Rval(env,_tmp5DC->loc,_tmp56B,_tmp56A);
Cyc_NewControlFlow_check_init_params(s->loc,env,_tmp56B);
Cyc_NewControlFlow_unconsume_params(env,env->noconsume_params,0,0,_tmp56B,s->loc);
return Cyc_CfFlowInfo_BottomFL();};}case 1: _LL48E: _tmp5DD=((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_tmp563)->f1;_LL48F: {
# 2588
struct _tuple17*_tmp56C=rval_opt;void**_tmp572;int _tmp571;if(_tmp56C != 0){_LL4B4: _tmp572=(void**)& _tmp56C->f1;_tmp571=_tmp56C->f2;_LL4B5: {
# 2590
struct _tuple16 _tmp56D=Cyc_NewControlFlow_anal_Rexp(env,_tmp571,inflow,_tmp5DD);struct _tuple16 _tmp56E=_tmp56D;union Cyc_CfFlowInfo_FlowInfo _tmp570;void*_tmp56F;_LL4B9: _tmp570=_tmp56E.f1;_tmp56F=_tmp56E.f2;_LL4BA:;
*_tmp572=_tmp56F;
return _tmp570;}}else{_LL4B6: _LL4B7:
# 2594
 return(Cyc_NewControlFlow_anal_Rexp(env,0,inflow,_tmp5DD)).f1;}_LL4B3:;}case 2: _LL490: _tmp5DF=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp563)->f1;_tmp5DE=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp563)->f2;_LL491:
# 2598
 return Cyc_NewControlFlow_anal_stmt(env,Cyc_NewControlFlow_anal_stmt(env,inflow,_tmp5DF,0),_tmp5DE,rval_opt);case 4: _LL492: _tmp5E2=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp563)->f1;_tmp5E1=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp563)->f2;_tmp5E0=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp563)->f3;_LL493: {
# 2601
struct _tuple19 _tmp573=Cyc_NewControlFlow_anal_test(env,inflow,_tmp5E2);struct _tuple19 _tmp574=_tmp573;union Cyc_CfFlowInfo_FlowInfo _tmp578;union Cyc_CfFlowInfo_FlowInfo _tmp577;_LL4BC: _tmp578=_tmp574.f1;_tmp577=_tmp574.f2;_LL4BD:;{
# 2608
union Cyc_CfFlowInfo_FlowInfo _tmp575=Cyc_NewControlFlow_anal_stmt(env,_tmp577,_tmp5E0,0);
union Cyc_CfFlowInfo_FlowInfo _tmp576=Cyc_NewControlFlow_anal_stmt(env,_tmp578,_tmp5E1,0);
return Cyc_CfFlowInfo_join_flow(_tmp561,env->all_changed,_tmp576,_tmp575);};}case 5: _LL494: _tmp5E5=(((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmp563)->f1).f1;_tmp5E4=(((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmp563)->f1).f2;_tmp5E3=((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmp563)->f2;_LL495: {
# 2616
struct _tuple20 _tmp579=Cyc_NewControlFlow_pre_stmt_check(env,inflow,_tmp5E4);struct _tuple20 _tmp57A=_tmp579;union Cyc_CfFlowInfo_FlowInfo*_tmp581;_LL4BF: _tmp581=_tmp57A.f2;_LL4C0:;{
union Cyc_CfFlowInfo_FlowInfo _tmp57B=*_tmp581;
struct _tuple19 _tmp57C=Cyc_NewControlFlow_anal_test(env,_tmp57B,_tmp5E5);struct _tuple19 _tmp57D=_tmp57C;union Cyc_CfFlowInfo_FlowInfo _tmp580;union Cyc_CfFlowInfo_FlowInfo _tmp57F;_LL4C2: _tmp580=_tmp57D.f1;_tmp57F=_tmp57D.f2;_LL4C3:;{
union Cyc_CfFlowInfo_FlowInfo _tmp57E=Cyc_NewControlFlow_anal_stmt(env,_tmp580,_tmp5E3,0);
Cyc_NewControlFlow_update_flow(env,_tmp5E4,_tmp57E);
return _tmp57F;};};}case 14: _LL496: _tmp5E8=((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmp563)->f1;_tmp5E7=(((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmp563)->f2).f1;_tmp5E6=(((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmp563)->f2).f2;_LL497: {
# 2626
union Cyc_CfFlowInfo_FlowInfo _tmp582=Cyc_NewControlFlow_anal_stmt(env,inflow,_tmp5E8,0);
struct _tuple20 _tmp583=Cyc_NewControlFlow_pre_stmt_check(env,_tmp582,_tmp5E6);struct _tuple20 _tmp584=_tmp583;union Cyc_CfFlowInfo_FlowInfo*_tmp58A;_LL4C5: _tmp58A=_tmp584.f2;_LL4C6:;{
union Cyc_CfFlowInfo_FlowInfo _tmp585=*_tmp58A;
struct _tuple19 _tmp586=Cyc_NewControlFlow_anal_test(env,_tmp585,_tmp5E7);struct _tuple19 _tmp587=_tmp586;union Cyc_CfFlowInfo_FlowInfo _tmp589;union Cyc_CfFlowInfo_FlowInfo _tmp588;_LL4C8: _tmp589=_tmp587.f1;_tmp588=_tmp587.f2;_LL4C9:;
Cyc_NewControlFlow_update_flow(env,_tmp5E8,_tmp589);
return _tmp588;};}case 9: _LL498: _tmp5EE=((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp563)->f1;_tmp5ED=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp563)->f2).f1;_tmp5EC=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp563)->f2).f2;_tmp5EB=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp563)->f3).f1;_tmp5EA=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp563)->f3).f2;_tmp5E9=((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp563)->f4;_LL499: {
# 2635
union Cyc_CfFlowInfo_FlowInfo _tmp58B=(Cyc_NewControlFlow_anal_Rexp(env,0,inflow,_tmp5EE)).f1;
struct _tuple20 _tmp58C=Cyc_NewControlFlow_pre_stmt_check(env,_tmp58B,_tmp5EC);struct _tuple20 _tmp58D=_tmp58C;union Cyc_CfFlowInfo_FlowInfo*_tmp599;_LL4CB: _tmp599=_tmp58D.f2;_LL4CC:;{
union Cyc_CfFlowInfo_FlowInfo _tmp58E=*_tmp599;
struct _tuple19 _tmp58F=Cyc_NewControlFlow_anal_test(env,_tmp58E,_tmp5ED);struct _tuple19 _tmp590=_tmp58F;union Cyc_CfFlowInfo_FlowInfo _tmp598;union Cyc_CfFlowInfo_FlowInfo _tmp597;_LL4CE: _tmp598=_tmp590.f1;_tmp597=_tmp590.f2;_LL4CF:;{
union Cyc_CfFlowInfo_FlowInfo _tmp591=Cyc_NewControlFlow_anal_stmt(env,_tmp598,_tmp5E9,0);
struct _tuple20 _tmp592=Cyc_NewControlFlow_pre_stmt_check(env,_tmp591,_tmp5EA);struct _tuple20 _tmp593=_tmp592;union Cyc_CfFlowInfo_FlowInfo*_tmp596;_LL4D1: _tmp596=_tmp593.f2;_LL4D2:;{
union Cyc_CfFlowInfo_FlowInfo _tmp594=*_tmp596;
union Cyc_CfFlowInfo_FlowInfo _tmp595=(Cyc_NewControlFlow_anal_Rexp(env,0,_tmp594,_tmp5EB)).f1;
Cyc_NewControlFlow_update_flow(env,_tmp5EC,_tmp595);
return _tmp597;};};};}case 11: if(((struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct*)_tmp563)->f2 != 0){_LL49A: _tmp5F0=((struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct*)_tmp563)->f1;_tmp5EF=*((struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct*)_tmp563)->f2;_LL49B: {
# 2647
struct _RegionHandle _tmp59A=_new_region("temp");struct _RegionHandle*temp=& _tmp59A;_push_region(temp);
{struct _tuple23 _tmp59B=Cyc_NewControlFlow_anal_unordered_Rexps(temp,env,inflow,_tmp5F0,1,1);struct _tuple23 _tmp59C=_tmp59B;union Cyc_CfFlowInfo_FlowInfo _tmp5A4;struct Cyc_List_List*_tmp5A3;_LL4D4: _tmp5A4=_tmp59C.f1;_tmp5A3=_tmp59C.f2;_LL4D5:;
# 2650
inflow=Cyc_NewControlFlow_unconsume_pat_vars(env,s,inflow,_tmp5EF->body);{
# 2652
struct Cyc_List_List*_tmp59D=((struct Cyc_List_List*(*)(struct Cyc_List_List*l))Cyc_Tcutil_filter_nulls)((((struct _tuple1(*)(struct Cyc_List_List*x))Cyc_List_split)((struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(_tmp5EF->pat_vars))->v)).f1);
_tmp5A4=Cyc_NewControlFlow_add_vars(_tmp561,_tmp5A4,_tmp59D,_tmp561->unknown_all,s->loc,0);
# 2655
{struct Cyc_List_List*x=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(_tmp5EF->pat_vars))->v;for(0;x != 0;x=x->tl){
struct _tuple24*_tmp59E=(struct _tuple24*)x->hd;struct _tuple24*_tmp59F=_tmp59E;struct Cyc_Absyn_Vardecl**_tmp5A1;struct Cyc_Absyn_Exp*_tmp5A0;_LL4D7: _tmp5A1=_tmp59F->f1;_tmp5A0=_tmp59F->f2;_LL4D8:;
if(_tmp5A1 != 0){
_tmp5A4=Cyc_NewControlFlow_do_initialize_var(_tmp561,env,_tmp5A4,*_tmp5A1,(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(_tmp5F0))->hd,(void*)((struct Cyc_List_List*)_check_null(_tmp5A3))->hd,s->loc);
_tmp5A3=_tmp5A3->tl;
_tmp5F0=_tmp5F0->tl;}}}
# 2663
Cyc_NewControlFlow_update_flow(env,_tmp5EF->body,_tmp5A4);{
union Cyc_CfFlowInfo_FlowInfo _tmp5A2=Cyc_CfFlowInfo_BottomFL();_npop_handler(0);return _tmp5A2;};};}
# 2648
;_pop_region(temp);}}else{_LL4AE: _LL4AF:
# 2833
(int)_throw((void*)({struct Cyc_Core_Impossible_exn_struct*_tmp5D9=_cycalloc(sizeof(*_tmp5D9));_tmp5D9[0]=({struct Cyc_Core_Impossible_exn_struct _tmp5DA;_tmp5DA.tag=Cyc_Core_Impossible;_tmp5DA.f1=({const char*_tmp5DB="anal_stmt -- bad stmt syntax or unimplemented stmt form";_tag_dyneither(_tmp5DB,sizeof(char),56);});_tmp5DA;});_tmp5D9;}));}case 6: _LL49C: _LL49D:
# 2670
 if(((struct Cyc_Absyn_Stmt*(*)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Stmt*key))Cyc_Hashtable_lookup)(env->succ_table,s)== 0){
{union Cyc_CfFlowInfo_FlowInfo _tmp5A5=inflow;if((_tmp5A5.ReachableFL).tag == 2){_LL4DA: _LL4DB:
# 2673
{void*_tmp5A6=Cyc_Tcutil_compress(env->return_type);void*_tmp5A7=_tmp5A6;switch(*((int*)_tmp5A7)){case 0: _LL4DF: _LL4E0:
 goto _LL4DE;case 7: _LL4E1: _LL4E2:
 goto _LL4E4;case 6: _LL4E3: _LL4E4:
# 2677
({void*_tmp5A8=0;Cyc_Tcutil_warn(s->loc,({const char*_tmp5A9="break may cause function not to return a value";_tag_dyneither(_tmp5A9,sizeof(char),47);}),_tag_dyneither(_tmp5A8,sizeof(void*),0));});
goto _LL4DE;default: _LL4E5: _LL4E6:
# 2680
({void*_tmp5AA=0;Cyc_Tcutil_terr(s->loc,({const char*_tmp5AB="break may cause function not to return a value";_tag_dyneither(_tmp5AB,sizeof(char),47);}),_tag_dyneither(_tmp5AA,sizeof(void*),0));});
goto _LL4DE;}_LL4DE:;}
# 2683
goto _LL4D9;}else{_LL4DC: _LL4DD:
 goto _LL4D9;}_LL4D9:;}
# 2686
if(env->noreturn)
({void*_tmp5AC=0;Cyc_CfFlowInfo_aerr(s->loc,({const char*_tmp5AD="`noreturn' function might return";_tag_dyneither(_tmp5AD,sizeof(char),33);}),_tag_dyneither(_tmp5AC,sizeof(void*),0));});
Cyc_NewControlFlow_check_init_params(s->loc,env,inflow);
Cyc_NewControlFlow_unconsume_params(env,env->noconsume_params,0,0,inflow,s->loc);
return Cyc_CfFlowInfo_BottomFL();}
# 2692
goto _LL49F;case 7: _LL49E: _LL49F:
 goto _LL4A1;case 8: _LL4A0: _LL4A1: {
# 2696
struct Cyc_Absyn_Stmt*_tmp5AE=((struct Cyc_Absyn_Stmt*(*)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Stmt*key))Cyc_Hashtable_lookup)(env->succ_table,s);
if(_tmp5AE == 0)
({void*_tmp5AF=0;Cyc_Tcutil_terr(s->loc,({const char*_tmp5B0="jump has no target (should be impossible)";_tag_dyneither(_tmp5B0,sizeof(char),42);}),_tag_dyneither(_tmp5AF,sizeof(void*),0));});
inflow=Cyc_NewControlFlow_unconsume_pat_vars(env,s,inflow,(struct Cyc_Absyn_Stmt*)_check_null(_tmp5AE));
# 2701
Cyc_NewControlFlow_update_flow(env,_tmp5AE,inflow);
return Cyc_CfFlowInfo_BottomFL();}case 10: _LL4A2: _tmp5F3=((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmp563)->f1;_tmp5F2=((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmp563)->f2;_tmp5F1=(void*)((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmp563)->f3;_LL4A3:
# 2707
 return Cyc_NewControlFlow_anal_scs(env,inflow,_tmp5F2,_tmp5F3->loc);case 15: _LL4A4: _tmp5F6=((struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*)_tmp563)->f1;_tmp5F5=((struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*)_tmp563)->f2;_tmp5F4=(void*)((struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*)_tmp563)->f3;_LL4A5: {
# 2712
int old_in_try=env->in_try;
union Cyc_CfFlowInfo_FlowInfo old_tryflow=env->tryflow;
env->in_try=1;
env->tryflow=inflow;{
union Cyc_CfFlowInfo_FlowInfo s1_outflow=Cyc_NewControlFlow_anal_stmt(env,inflow,_tmp5F6,0);
union Cyc_CfFlowInfo_FlowInfo scs_inflow=env->tryflow;
# 2720
env->in_try=old_in_try;
env->tryflow=old_tryflow;
# 2723
Cyc_NewControlFlow_update_tryflow(env,scs_inflow);{
union Cyc_CfFlowInfo_FlowInfo scs_outflow=Cyc_NewControlFlow_anal_scs(env,scs_inflow,_tmp5F5,0);
{union Cyc_CfFlowInfo_FlowInfo _tmp5B1=scs_outflow;if((_tmp5B1.BottomFL).tag == 1){_LL4E8: _LL4E9:
 goto _LL4E7;}else{_LL4EA: _LL4EB:
({void*_tmp5B2=0;Cyc_CfFlowInfo_aerr(s->loc,({const char*_tmp5B3="last catch clause may implicitly fallthru";_tag_dyneither(_tmp5B3,sizeof(char),42);}),_tag_dyneither(_tmp5B2,sizeof(void*),0));});}_LL4E7:;}
# 2729
outflow=s1_outflow;
# 2731
return outflow;};};}case 12: switch(*((int*)((struct Cyc_Absyn_Decl*)((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp563)->f1)->r)){case 2: if(((struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)((struct Cyc_Absyn_Decl*)((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp563)->f1)->r)->f2 != 0){_LL4A6: _tmp5FA=(struct Cyc_List_List*)(((struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)(((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp563)->f1)->r)->f2)->v;_tmp5F9=((struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)(((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp563)->f1)->r)->f3;_tmp5F8=(((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp563)->f1)->loc;_tmp5F7=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp563)->f2;_LL4A7: {
# 2741
struct _tuple1 _tmp5B4=Cyc_NewControlFlow_get_unconsume_pat_vars((env->fenv)->r,_tmp5FA);struct _tuple1 _tmp5B5=_tmp5B4;struct Cyc_List_List*_tmp5BA;struct Cyc_List_List*_tmp5B9;_LL4ED: _tmp5BA=_tmp5B5.f1;_tmp5B9=_tmp5B5.f2;_LL4EE:;
inflow=Cyc_NewControlFlow_initialize_pat_vars(_tmp561,env,inflow,_tmp5FA,_tmp5BA != 0,_tmp5F8,_tmp5F9->loc);{
struct Cyc_List_List*_tmp5B6=env->unique_pat_vars;
# 2745
env->unique_pat_vars=({struct Cyc_List_List*_tmp5B7=_region_malloc(env->r,sizeof(*_tmp5B7));_tmp5B7->hd=({struct _tuple29*_tmp5B8=_region_malloc(env->r,sizeof(*_tmp5B8));_tmp5B8->f1=0;_tmp5B8->f2=s;_tmp5B8->f3=_tmp5BA;_tmp5B8->f4=_tmp5B9;_tmp5B8;});_tmp5B7->tl=_tmp5B6;_tmp5B7;});
# 2750
inflow=Cyc_NewControlFlow_anal_stmt(env,inflow,_tmp5F7,rval_opt);
env->unique_pat_vars=_tmp5B6;
# 2754
inflow=Cyc_NewControlFlow_unconsume_params(env,_tmp5BA,_tmp5B9,0,inflow,_tmp5F8);
# 2758
return inflow;};}}else{goto _LL4AA;}case 4: _LL4A8: _tmp5FE=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)(((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp563)->f1)->r)->f2;_tmp5FD=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)(((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp563)->f1)->r)->f3;_tmp5FC=(((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp563)->f1)->loc;_tmp5FB=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp563)->f2;if(_tmp5FD != 0){_LL4A9: {
# 2770
struct Cyc_List_List l=({struct Cyc_List_List _tmp5D8;_tmp5D8.hd=_tmp5FD;_tmp5D8.tl=0;_tmp5D8;});
union Cyc_CfFlowInfo_FlowInfo _tmp5BB=Cyc_NewControlFlow_expand_unique_places(env,inflow,& l);
struct _tuple18 _tmp5BC=Cyc_NewControlFlow_anal_Lexp(env,_tmp5BB,0,0,_tmp5FD);struct _tuple18 _tmp5BD=_tmp5BC;union Cyc_CfFlowInfo_AbsLVal _tmp5D7;_LL4F0: _tmp5D7=_tmp5BD.f2;_LL4F1:;{
struct _tuple16 _tmp5BE=Cyc_NewControlFlow_anal_Rexp(env,1,_tmp5BB,_tmp5FD);struct _tuple16 _tmp5BF=_tmp5BE;union Cyc_CfFlowInfo_FlowInfo _tmp5D6;_LL4F3: _tmp5D6=_tmp5BF.f1;_LL4F4:;{
struct Cyc_List_List*_tmp5C0=0;
struct Cyc_List_List*_tmp5C1=0;
{union Cyc_CfFlowInfo_FlowInfo _tmp5C2=_tmp5D6;struct Cyc_Dict_Dict _tmp5D0;struct Cyc_List_List*_tmp5CF;if((_tmp5C2.ReachableFL).tag == 2){_LL4F6: _tmp5D0=((_tmp5C2.ReachableFL).val).f1;_tmp5CF=((_tmp5C2.ReachableFL).val).f2;_LL4F7:
# 2778
{union Cyc_CfFlowInfo_AbsLVal _tmp5C3=_tmp5D7;struct Cyc_CfFlowInfo_Place*_tmp5CE;if((_tmp5C3.PlaceL).tag == 1){_LL4FB: _tmp5CE=(_tmp5C3.PlaceL).val;_LL4FC: {
# 2782
void*_tmp5C4=Cyc_CfFlowInfo_lookup_place(_tmp5D0,_tmp5CE);
_tmp5C4=(void*)({struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*_tmp5C5=_region_malloc(_tmp561->r,sizeof(*_tmp5C5));_tmp5C5[0]=({struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct _tmp5C6;_tmp5C6.tag=8;_tmp5C6.f1=_tmp5FE;_tmp5C6.f2=_tmp5C4;_tmp5C6;});_tmp5C5;});
_tmp5D0=Cyc_CfFlowInfo_assign_place(_tmp561,_tmp5FD->loc,_tmp5D0,env->all_changed,_tmp5CE,_tmp5C4);
# 2786
_tmp5D6=Cyc_CfFlowInfo_ReachableFL(_tmp5D0,_tmp5CF);{
struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*_tmp5C7=({struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*_tmp5CC=_region_malloc(_tmp561->r,sizeof(*_tmp5CC));_tmp5CC[0]=({struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct _tmp5CD;_tmp5CD.tag=0;_tmp5CD.f1=_tmp5FE;_tmp5CD;});_tmp5CC;});
struct Cyc_CfFlowInfo_Place*_tmp5C8=({struct Cyc_CfFlowInfo_Place*_tmp5CB=_region_malloc(_tmp561->r,sizeof(*_tmp5CB));_tmp5CB->root=(void*)_tmp5C7;_tmp5CB->fields=0;_tmp5CB;});
_tmp5C0=({struct Cyc_List_List*_tmp5C9=_region_malloc(_tmp561->r,sizeof(*_tmp5C9));_tmp5C9->hd=_tmp5C8;_tmp5C9->tl=_tmp5C0;_tmp5C9;});
_tmp5C1=({struct Cyc_List_List*_tmp5CA=_region_malloc(_tmp561->r,sizeof(*_tmp5CA));_tmp5CA->hd=_tmp5FD;_tmp5CA->tl=_tmp5C1;_tmp5CA;});
goto _LL4FA;};}}else{_LL4FD: _LL4FE:
# 2798
 goto _LL4FA;}_LL4FA:;}
# 2800
goto _LL4F5;}else{_LL4F8: _LL4F9:
# 2802
 goto _LL4F5;}_LL4F5:;}{
# 2805
struct Cyc_List_List _tmp5D1=({struct Cyc_List_List _tmp5D5;_tmp5D5.hd=_tmp5FE;_tmp5D5.tl=0;_tmp5D5;});
_tmp5D6=Cyc_NewControlFlow_add_vars(_tmp561,_tmp5D6,& _tmp5D1,_tmp561->unknown_all,_tmp5FC,0);{
# 2809
struct Cyc_List_List*_tmp5D2=env->unique_pat_vars;
env->unique_pat_vars=({struct Cyc_List_List*_tmp5D3=_region_malloc(env->r,sizeof(*_tmp5D3));_tmp5D3->hd=({struct _tuple29*_tmp5D4=_region_malloc(env->r,sizeof(*_tmp5D4));_tmp5D4->f1=1;_tmp5D4->f2=s;_tmp5D4->f3=_tmp5C0;_tmp5D4->f4=_tmp5C1;_tmp5D4;});_tmp5D3->tl=_tmp5D2;_tmp5D3;});
# 2815
_tmp5D6=Cyc_NewControlFlow_anal_stmt(env,_tmp5D6,_tmp5FB,rval_opt);
env->unique_pat_vars=_tmp5D2;
# 2819
_tmp5D6=Cyc_NewControlFlow_unconsume_params(env,_tmp5C0,_tmp5C1,1,_tmp5D6,_tmp5FC);
# 2823
return _tmp5D6;};};};};}}else{goto _LL4AA;}default: _LL4AA: _tmp600=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp563)->f1;_tmp5FF=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp563)->f2;_LL4AB:
# 2826
 outflow=Cyc_NewControlFlow_anal_stmt(env,Cyc_NewControlFlow_anal_decl(env,inflow,_tmp600),_tmp5FF,rval_opt);
if(Cyc_NewControlFlow_warn_lose_unique)
Cyc_NewControlFlow_check_dropped_unique(env,outflow,_tmp600);
return outflow;}default: _LL4AC: _tmp601=((struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*)_tmp563)->f2;_LL4AD:
# 2831
 return Cyc_NewControlFlow_anal_stmt(env,inflow,_tmp601,rval_opt);}_LL487:;};}
# 2838
static void Cyc_NewControlFlow_check_nested_fun(struct Cyc_JumpAnalysis_Jump_Anal_Result*tables,struct Cyc_CfFlowInfo_FlowEnv*,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_Absyn_Fndecl*fd);
# 2842
static union Cyc_CfFlowInfo_FlowInfo Cyc_NewControlFlow_anal_decl(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_Absyn_Decl*decl){
struct Cyc_CfFlowInfo_FlowEnv*_tmp604=env->fenv;
void*_tmp605=decl->r;void*_tmp606=_tmp605;struct Cyc_Absyn_Tvar*_tmp61D;struct Cyc_Absyn_Vardecl*_tmp61C;struct Cyc_Absyn_Exp*_tmp61B;struct Cyc_Absyn_Fndecl*_tmp61A;struct Cyc_List_List*_tmp619;struct Cyc_Absyn_Vardecl*_tmp618;switch(*((int*)_tmp606)){case 0: _LL500: _tmp618=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp606)->f1;_LL501: {
# 2852
struct Cyc_List_List _tmp607=({struct Cyc_List_List _tmp60D;_tmp60D.hd=_tmp618;_tmp60D.tl=0;_tmp60D;});
inflow=Cyc_NewControlFlow_add_vars(_tmp604,inflow,& _tmp607,_tmp604->unknown_none,decl->loc,0);{
struct Cyc_Absyn_Exp*_tmp608=_tmp618->initializer;
if(_tmp608 == 0)
return inflow;{
struct _tuple16 _tmp609=Cyc_NewControlFlow_anal_Rexp(env,1,inflow,_tmp608);struct _tuple16 _tmp60A=_tmp609;union Cyc_CfFlowInfo_FlowInfo _tmp60C;void*_tmp60B;_LL50B: _tmp60C=_tmp60A.f1;_tmp60B=_tmp60A.f2;_LL50C:;
return Cyc_NewControlFlow_do_initialize_var(_tmp604,env,_tmp60C,_tmp618,_tmp608,_tmp60B,decl->loc);};};}case 3: _LL502: _tmp619=((struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct*)_tmp606)->f1;_LL503:
# 2861
 return Cyc_NewControlFlow_add_vars(_tmp604,inflow,_tmp619,_tmp604->unknown_none,decl->loc,0);case 1: _LL504: _tmp61A=((struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_tmp606)->f1;_LL505:
# 2864
 Cyc_NewControlFlow_check_nested_fun(env->all_tables,_tmp604,inflow,_tmp61A);{
void*t=(void*)_check_null(_tmp61A->cached_typ);
struct Cyc_Absyn_Vardecl*_tmp60E=(struct Cyc_Absyn_Vardecl*)_check_null(_tmp61A->fn_vardecl);
# 2870
return Cyc_NewControlFlow_add_vars(_tmp604,inflow,({struct Cyc_List_List*_tmp60F=_region_malloc(env->r,sizeof(*_tmp60F));_tmp60F->hd=_tmp60E;_tmp60F->tl=0;_tmp60F;}),_tmp604->unknown_all,decl->loc,0);};case 4: _LL506: _tmp61D=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmp606)->f1;_tmp61C=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmp606)->f2;_tmp61B=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmp606)->f3;_LL507:
# 2873
 if(_tmp61B != 0)
(int)_throw((void*)({struct Cyc_Core_Impossible_exn_struct*_tmp610=_cycalloc(sizeof(*_tmp610));_tmp610[0]=({struct Cyc_Core_Impossible_exn_struct _tmp611;_tmp611.tag=Cyc_Core_Impossible;_tmp611.f1=({const char*_tmp612="found open expression in declaration!";_tag_dyneither(_tmp612,sizeof(char),38);});_tmp611;});_tmp610;}));{
struct Cyc_List_List _tmp613=({struct Cyc_List_List _tmp614;_tmp614.hd=_tmp61C;_tmp614.tl=0;_tmp614;});
return Cyc_NewControlFlow_add_vars(_tmp604,inflow,& _tmp613,_tmp604->unknown_all,decl->loc,0);};default: _LL508: _LL509:
# 2879
(int)_throw((void*)({struct Cyc_Core_Impossible_exn_struct*_tmp615=_cycalloc(sizeof(*_tmp615));_tmp615[0]=({struct Cyc_Core_Impossible_exn_struct _tmp616;_tmp616.tag=Cyc_Core_Impossible;_tmp616.f1=({const char*_tmp617="anal_decl: unexpected decl variant";_tag_dyneither(_tmp617,sizeof(char),35);});_tmp616;});_tmp615;}));}_LL4FF:;}
# 2887
static void Cyc_NewControlFlow_check_fun(struct Cyc_JumpAnalysis_Jump_Anal_Result*tables,struct Cyc_Absyn_Fndecl*fd){
struct _handler_cons _tmp61E;_push_handler(& _tmp61E);{int _tmp620=0;if(setjmp(_tmp61E.handler))_tmp620=1;if(!_tmp620){
{struct _RegionHandle _tmp621=_new_region("frgn");struct _RegionHandle*frgn=& _tmp621;_push_region(frgn);
{struct Cyc_CfFlowInfo_FlowEnv*fenv=Cyc_CfFlowInfo_new_flow_env(frgn);
Cyc_NewControlFlow_check_nested_fun(tables,fenv,Cyc_CfFlowInfo_ReachableFL(fenv->mt_flowdict,0),fd);}
# 2890
;_pop_region(frgn);}
# 2889
;_pop_handler();}else{void*_tmp61F=(void*)_exn_thrown;void*_tmp622=_tmp61F;void*_tmp625;if(((struct Cyc_Dict_Absent_exn_struct*)_tmp622)->tag == Cyc_Dict_Absent){_LL50E: _LL50F:
# 2896
 if(Cyc_Position_num_errors > 0)
goto _LL50D;else{
Cyc_Core_rethrow((void*)({struct Cyc_Dict_Absent_exn_struct*_tmp623=_cycalloc_atomic(sizeof(*_tmp623));_tmp623[0]=({struct Cyc_Dict_Absent_exn_struct _tmp624;_tmp624.tag=Cyc_Dict_Absent;_tmp624;});_tmp623;}));}}else{_LL510: _tmp625=_tmp622;_LL511:(int)_rethrow(_tmp625);}_LL50D:;}};}
# 2902
static int Cyc_NewControlFlow_hash_ptr(void*s){
return(int)s;}
# 2907
static union Cyc_Relations_RelnOp Cyc_NewControlFlow_translate_rop(struct Cyc_List_List*vds,union Cyc_Relations_RelnOp r){
union Cyc_Relations_RelnOp _tmp626=r;unsigned int _tmp628;if((_tmp626.RParam).tag == 5){_LL513: _tmp628=(_tmp626.RParam).val;_LL514: {
# 2910
struct Cyc_Absyn_Vardecl*_tmp627=((struct Cyc_Absyn_Vardecl*(*)(struct Cyc_List_List*x,int n))Cyc_List_nth)(vds,(int)_tmp628);
if(!_tmp627->escapes)
return Cyc_Relations_RVar(_tmp627);
return r;}}else{_LL515: _LL516:
 return r;}_LL512:;}
# 2918
static void Cyc_NewControlFlow_check_nested_fun(struct Cyc_JumpAnalysis_Jump_Anal_Result*tables,struct Cyc_CfFlowInfo_FlowEnv*fenv,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_Absyn_Fndecl*fd){
# 2922
struct _RegionHandle*_tmp629=fenv->r;
unsigned int _tmp62A=(fd->body)->loc;
inflow=Cyc_NewControlFlow_add_vars(fenv,inflow,(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(fd->param_vardecls))->v,fenv->unknown_all,_tmp62A,1);{
# 2928
struct Cyc_List_List*param_roots=0;
struct Cyc_List_List*noconsume_roots=0;
struct _tuple14 _tmp62B=({union Cyc_CfFlowInfo_FlowInfo _tmp663=inflow;if((_tmp663.ReachableFL).tag != 2)_throw_match();(_tmp663.ReachableFL).val;});struct _tuple14 _tmp62C=_tmp62B;struct Cyc_Dict_Dict _tmp662;struct Cyc_List_List*_tmp661;_LL518: _tmp662=_tmp62C.f1;_tmp661=_tmp62C.f2;_LL519:;{
# 2933
struct Cyc_List_List*_tmp62D=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(fd->param_vardecls))->v;
{struct Cyc_List_List*_tmp62E=fd->requires_relns;for(0;_tmp62E != 0;_tmp62E=_tmp62E->tl){
struct Cyc_Relations_Reln*_tmp62F=(struct Cyc_Relations_Reln*)_tmp62E->hd;
_tmp661=Cyc_Relations_add_relation(_tmp629,Cyc_NewControlFlow_translate_rop(_tmp62D,_tmp62F->rop1),_tmp62F->relation,
Cyc_NewControlFlow_translate_rop(_tmp62D,_tmp62F->rop2),_tmp661);}}{
# 2942
struct Cyc_List_List*atts;
{void*_tmp630=Cyc_Tcutil_compress((void*)_check_null(fd->cached_typ));void*_tmp631=_tmp630;struct Cyc_List_List*_tmp634;if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp631)->tag == 9){_LL51B: _tmp634=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp631)->f1).attributes;_LL51C:
 atts=_tmp634;goto _LL51A;}else{_LL51D: _LL51E:
({void*_tmp632=0;((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(({const char*_tmp633="check_fun: non-function type cached with fndecl_t";_tag_dyneither(_tmp633,sizeof(char),50);}),_tag_dyneither(_tmp632,sizeof(void*),0));});}_LL51A:;}
# 2949
for(0;atts != 0;atts=atts->tl){
void*_tmp635=(void*)atts->hd;void*_tmp636=_tmp635;int _tmp656;int _tmp655;int _tmp654;switch(*((int*)_tmp636)){case 21: _LL520: _tmp654=((struct Cyc_Absyn_Noliveunique_att_Absyn_Attribute_struct*)_tmp636)->f1;_LL521: {
# 2952
struct Cyc_Absyn_Exp*bogus_exp=Cyc_Absyn_signed_int_exp(- 1,0);
struct Cyc_Absyn_Vardecl*_tmp637=((struct Cyc_Absyn_Vardecl*(*)(struct Cyc_List_List*x,int n))Cyc_List_nth)((struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(fd->param_vardecls))->v,_tmp654 - 1);
void*t=Cyc_Tcutil_compress(_tmp637->type);
void*elttype=Cyc_Tcutil_pointer_elt_type(t);
void*_tmp638=
Cyc_CfFlowInfo_make_unique_consumed(fenv,elttype,bogus_exp,- 1,
Cyc_CfFlowInfo_typ_to_absrval(fenv,elttype,0,fenv->unknown_all));
struct Cyc_CfFlowInfo_InitParam_CfFlowInfo_Root_struct*_tmp639=({struct Cyc_CfFlowInfo_InitParam_CfFlowInfo_Root_struct*_tmp640=_region_malloc(_tmp629,sizeof(*_tmp640));_tmp640[0]=({struct Cyc_CfFlowInfo_InitParam_CfFlowInfo_Root_struct _tmp641;_tmp641.tag=2;_tmp641.f1=_tmp654;_tmp641.f2=t;_tmp641;});_tmp640;});
struct Cyc_CfFlowInfo_Place*_tmp63A=({struct Cyc_CfFlowInfo_Place*_tmp63F=_region_malloc(_tmp629,sizeof(*_tmp63F));_tmp63F->root=(void*)_tmp639;_tmp63F->fields=0;_tmp63F;});
_tmp662=((struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,void*k,void*v))Cyc_Dict_insert)(_tmp662,(void*)_tmp639,_tmp638);
_tmp662=((struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,void*k,void*v))Cyc_Dict_insert)(_tmp662,(void*)({struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*_tmp63B=_region_malloc(_tmp629,sizeof(*_tmp63B));_tmp63B[0]=({struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct _tmp63C;_tmp63C.tag=0;_tmp63C.f1=_tmp637;_tmp63C;});_tmp63B;}),(void*)({struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*_tmp63D=_region_malloc(_tmp629,sizeof(*_tmp63D));_tmp63D[0]=({struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct _tmp63E;_tmp63E.tag=5;_tmp63E.f1=_tmp63A;_tmp63E;});_tmp63D;}));
goto _LL51F;}case 20: _LL522: _tmp655=((struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct*)_tmp636)->f1;_LL523: {
# 2965
struct Cyc_Absyn_Vardecl*_tmp642=((struct Cyc_Absyn_Vardecl*(*)(struct Cyc_List_List*x,int n))Cyc_List_nth)((struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(fd->param_vardecls))->v,_tmp655 - 1);
void*elttype=Cyc_Tcutil_pointer_elt_type(_tmp642->type);
struct Cyc_CfFlowInfo_InitParam_CfFlowInfo_Root_struct*_tmp643=({struct Cyc_CfFlowInfo_InitParam_CfFlowInfo_Root_struct*_tmp64B=_region_malloc(_tmp629,sizeof(*_tmp64B));_tmp64B[0]=({struct Cyc_CfFlowInfo_InitParam_CfFlowInfo_Root_struct _tmp64C;_tmp64C.tag=2;_tmp64C.f1=_tmp655;_tmp64C.f2=elttype;_tmp64C;});_tmp64B;});
struct Cyc_CfFlowInfo_Place*_tmp644=({struct Cyc_CfFlowInfo_Place*_tmp64A=_region_malloc(_tmp629,sizeof(*_tmp64A));_tmp64A->root=(void*)_tmp643;_tmp64A->fields=0;_tmp64A;});
_tmp662=((struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,void*k,void*v))Cyc_Dict_insert)(_tmp662,(void*)_tmp643,Cyc_CfFlowInfo_typ_to_absrval(fenv,elttype,0,fenv->esc_none));
_tmp662=((struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,void*k,void*v))Cyc_Dict_insert)(_tmp662,(void*)({struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*_tmp645=_region_malloc(_tmp629,sizeof(*_tmp645));_tmp645[0]=({struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct _tmp646;_tmp646.tag=0;_tmp646.f1=_tmp642;_tmp646;});_tmp645;}),(void*)({struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*_tmp647=_region_malloc(_tmp629,sizeof(*_tmp647));_tmp647[0]=({struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct _tmp648;_tmp648.tag=5;_tmp648.f1=_tmp644;_tmp648;});_tmp647;}));
param_roots=({struct Cyc_List_List*_tmp649=_region_malloc(_tmp629,sizeof(*_tmp649));_tmp649->hd=_tmp644;_tmp649->tl=param_roots;_tmp649;});
goto _LL51F;}case 22: _LL524: _tmp656=((struct Cyc_Absyn_Noconsume_att_Absyn_Attribute_struct*)_tmp636)->f1;_LL525: {
# 2974
struct Cyc_Absyn_Vardecl*_tmp64D=((struct Cyc_Absyn_Vardecl*(*)(struct Cyc_List_List*x,int n))Cyc_List_nth)((struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(fd->param_vardecls))->v,_tmp656 - 1);
if(Cyc_Tcutil_is_noalias_pointer(_tmp64D->type,0)){
struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*_tmp64E=({struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*_tmp652=_region_malloc(_tmp629,sizeof(*_tmp652));_tmp652[0]=({struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct _tmp653;_tmp653.tag=0;_tmp653.f1=_tmp64D;_tmp653;});_tmp652;});
struct Cyc_CfFlowInfo_Place*_tmp64F=({struct Cyc_CfFlowInfo_Place*_tmp651=_region_malloc(_tmp629,sizeof(*_tmp651));_tmp651->root=(void*)_tmp64E;_tmp651->fields=0;_tmp651;});
noconsume_roots=({struct Cyc_List_List*_tmp650=_region_malloc(_tmp629,sizeof(*_tmp650));_tmp650->hd=_tmp64F;_tmp650->tl=noconsume_roots;_tmp650;});}
# 2980
goto _LL51F;}default: _LL526: _LL527:
 goto _LL51F;}_LL51F:;}
# 2983
inflow=Cyc_CfFlowInfo_ReachableFL(_tmp662,_tmp661);{
# 2985
int noreturn=Cyc_Tcutil_is_noreturn(Cyc_Tcutil_fndecl2typ(fd));
struct Cyc_Hashtable_Table*flow_table=
((struct Cyc_Hashtable_Table*(*)(struct _RegionHandle*r,int sz,int(*cmp)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*),int(*hash)(struct Cyc_Absyn_Stmt*)))Cyc_Hashtable_rcreate)(_tmp629,33,(int(*)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*))Cyc_Core_ptrcmp,(int(*)(struct Cyc_Absyn_Stmt*s))Cyc_NewControlFlow_hash_ptr);
struct Cyc_NewControlFlow_AnalEnv*env=({struct Cyc_NewControlFlow_AnalEnv*_tmp660=_region_malloc(_tmp629,sizeof(*_tmp660));_tmp660->all_tables=tables;_tmp660->succ_table=
# 2990
((struct Cyc_Hashtable_Table*(*)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Fndecl*key))Cyc_Hashtable_lookup)(tables->succ_tables,fd);_tmp660->pat_pop_table=
((struct Cyc_Hashtable_Table*(*)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Fndecl*key))Cyc_Hashtable_lookup)(tables->pat_pop_tables,fd);_tmp660->r=_tmp629;_tmp660->fenv=fenv;_tmp660->iterate_again=1;_tmp660->iteration_num=0;_tmp660->in_try=0;_tmp660->tryflow=inflow;_tmp660->all_changed=0;_tmp660->noreturn=noreturn;_tmp660->return_type=fd->ret_type;_tmp660->unique_pat_vars=0;_tmp660->param_roots=param_roots;_tmp660->noconsume_params=noconsume_roots;_tmp660->flow_table=flow_table;_tmp660->return_relns=fd->ensures_relns;_tmp660;});
# 2995
union Cyc_CfFlowInfo_FlowInfo outflow=inflow;
while(env->iterate_again  && !Cyc_CfFlowInfo_anal_error){
++ env->iteration_num;
# 3001
env->iterate_again=0;
outflow=Cyc_NewControlFlow_anal_stmt(env,inflow,fd->body,0);}{
# 3004
union Cyc_CfFlowInfo_FlowInfo _tmp657=outflow;if((_tmp657.BottomFL).tag == 1){_LL529: _LL52A:
 goto _LL528;}else{_LL52B: _LL52C:
# 3007
 Cyc_NewControlFlow_check_init_params(_tmp62A,env,outflow);
Cyc_NewControlFlow_unconsume_params(env,env->noconsume_params,0,0,outflow,_tmp62A);
# 3011
if(noreturn)
({void*_tmp658=0;Cyc_CfFlowInfo_aerr(_tmp62A,({const char*_tmp659="`noreturn' function might (implicitly) return";_tag_dyneither(_tmp659,sizeof(char),46);}),_tag_dyneither(_tmp658,sizeof(void*),0));});else{
# 3014
void*_tmp65A=Cyc_Tcutil_compress(fd->ret_type);void*_tmp65B=_tmp65A;switch(*((int*)_tmp65B)){case 0: _LL52E: _LL52F:
 goto _LL52D;case 7: _LL530: _LL531:
 goto _LL533;case 6: _LL532: _LL533:
# 3018
({void*_tmp65C=0;Cyc_Tcutil_warn(_tmp62A,({const char*_tmp65D="function may not return a value";_tag_dyneither(_tmp65D,sizeof(char),32);}),_tag_dyneither(_tmp65C,sizeof(void*),0));});goto _LL52D;default: _LL534: _LL535:
# 3020
({void*_tmp65E=0;Cyc_CfFlowInfo_aerr(_tmp62A,({const char*_tmp65F="function may not return a value";_tag_dyneither(_tmp65F,sizeof(char),32);}),_tag_dyneither(_tmp65E,sizeof(void*),0));});goto _LL52D;}_LL52D:;}
# 3022
goto _LL528;}_LL528:;};};};};};}
# 3026
void Cyc_NewControlFlow_cf_check(struct Cyc_JumpAnalysis_Jump_Anal_Result*tables,struct Cyc_List_List*ds){
for(0;ds != 0;ds=ds->tl){
Cyc_CfFlowInfo_anal_error=0;{
void*_tmp664=((struct Cyc_Absyn_Decl*)ds->hd)->r;void*_tmp665=_tmp664;struct Cyc_List_List*_tmp669;struct Cyc_List_List*_tmp668;struct Cyc_List_List*_tmp667;struct Cyc_Absyn_Fndecl*_tmp666;switch(*((int*)_tmp665)){case 1: _LL537: _tmp666=((struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_tmp665)->f1;_LL538:
 Cyc_NewControlFlow_check_fun(tables,_tmp666);goto _LL536;case 11: _LL539: _tmp667=((struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct*)_tmp665)->f1;_LL53A:
 _tmp668=_tmp667;goto _LL53C;case 10: _LL53B: _tmp668=((struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*)_tmp665)->f2;_LL53C:
 _tmp669=_tmp668;goto _LL53E;case 9: _LL53D: _tmp669=((struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmp665)->f2;_LL53E:
 Cyc_NewControlFlow_cf_check(tables,_tmp669);goto _LL536;case 12: _LL53F: _LL540:
 goto _LL536;default: _LL541: _LL542:
 goto _LL536;}_LL536:;};}}
