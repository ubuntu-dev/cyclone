#ifndef TAG_ELIM_H
#define TAG_ELIM_H

#include "smlng.h"

namespace TagElim {
extern struct SynthS {
  bool sz: 1;
  bool bold: 1;
  bool emph: 1;
  bool ital: 1;
  bool strong: 1;
  bool plain: 1;
  bool tt: 1;
  bool u1: 1; // invariant: true if u2 is true
  bool u2: 1; // invariant: true if u3 is true
  bool u3: 1;
  bool color: 1;
};
extern union Synth {
  struct SynthS s;
  unsigned short i;
};
typedef union Synth synth_t; // not a pointer
extern void up_opt_test();
extern $(doc_t, synth_t) up_opt(doc_t doc);
}

#endif
