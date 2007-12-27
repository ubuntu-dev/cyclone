################################
## Earley version
################################
CALC_FILES = calc-flat-dfa calccrawl
CALC_OBJS:=$(foreach yfile,$(CALC_FILES),$(yfile).o)

calc-earley: $(CALC_OBJS) $(LIB_YAKKER)
	$(CYCLONE) -o $@ $(CALC_OBJS)  -lssl -lm $(LIB_YAKKER)

calc-flat-dfa.txt: examples/calc.bnf yakker
	./yakker -flatten-full -earley-gen-fsm start examples/calc.bnf > $@

calc-flat-dfa.cyc: examples/calc.bnf yakker
	./yakker -flatten-full -earley-gen-cyc start $< > $@

calccrawl.cyc: examples/calc.bnf yakker crawl-main.cyc
	./yakker -flatten-full -gen-crawl start $< > $@
	echo '#define CRAWLFUN p_start' >> $@
	echo '#include "crawl-main.cyc"' >> $@

#########
CALC_TG_FILES = calc-tg-dfa calc-tg-crawl parse_tab
CALC_TG_OBJS:=$(foreach yfile,$(CALC_TG_FILES),$(yfile).o)

calc-tg-earley: $(CALC_TG_OBJS) $(LIB_YAKKER)
	$(CYCLONE) -o $@ $(CALC_TG_OBJS)  -lssl -lm $(LIB_YAKKER) -lcrypto

CALC_TG_RUN_FILES = calc-tg-dfa calc-grm-dfa calc-tg-run parse_tab
CALC_TG_RUN_OBJS:=$(foreach yfile,$(CALC_TG_RUN_FILES),$(yfile).o)

calc-tg-run: $(CALC_TG_RUN_OBJS) $(LIB_YAKKER)
	$(CYCLONE) -o $@ $(CALC_TG_RUN_OBJS)  -lssl -lm $(LIB_YAKKER) -lcrypto

calc-tg-dfa.cyc: gen/calc-tg.bnf yakker
	./yakker -flatten-full -no-minus-elim -cyc-namespace CalcTGCycDFA -earley-gen-cyc start $< > $@

calc-grm-dfa.cyc: examples/calc.bnf yakker
	./yakker -flatten-full -cyc-namespace CalcCycDFA -earley-gen-grm-cyc examples/calc.bnf > $@

calc-tg-crawl.cyc: gen/calc-tg.bnf yakker crawl-main.cyc
	./yakker $(YAKFLAGS) -gen-crawl start \
                             -flatten-full \
                             -no-minus-elim \
                             -no-globals \
                             $< > $@
	echo '#define CRAWLFUN p_start' >> $@
	echo '#include "crawl-main.cyc"' >> $@

calc-tg-run.cyc: gen/calc-tg.bnf yakker tge-main.cyc
	./yakker $(YAKFLAGS) -gen-crawl start \
                             -flatten-full \
                             -no-minus-elim \
                             -no-globals \
                             $< > $@
	echo '#define TGFUN p_start' >> $@
	echo '#define CYC_TG_DFA_NS CalcTGCycDFA' >> $@
	echo '#define CYC_GRM_DFA_NS CalcCycDFA' >> $@
	echo '#include "tge-main.cyc"' >> $@

calc-scanf.cyc: gen/calc-tg.bnf yakker
	./yakker $(YAKFLAGS) -gen-crawl start \
                             -flatten-full \
                             -no-minus-elim \
                             -no-globals \
                             $< > $@
	echo '#include "tge-scanf.h"' >> $@
	echo 'int start_scanf(string_t input, const char ?fmt, ...const char?`H @ args){' >> $@
	echo '  let dfa_rep = EarleyCycBackend::init_dfa();' >> $@
	echo '  return internal_scanf(dfa_rep, p_start<>,input,fmt,args);' >> $@
	echo '}' >> $@

gen/calc-tg.bnf: examples/calc.bnf yakker
	./yakker -escape "\\%()" -flatten-full -bindgrammar -termgrammar_bnf $< > $@

#calc-tg-dfa.txt: gen/calc-tg.bnf yakker
#	./yakker -flatten-full -earley-gen-fsm start $< > $@

gen/calc-tg-flat.bnf: gen/calc-tg.bnf yakker
	./yakker $(YAKFLAGS) -flatten-full \
                             -no-minus-elim \
                             $< > $@
gen/calc-tg-dfa.dot: gen/calc-tg.bnf yakker
	./yakker -flatten-full -no-minus-elim -earley-gen-dot start $< > $@

gen/calc-flat-grm-dfa.txt: examples/calc.bnf yakker
	./yakker -flatten-full -earley-gen-grm-fsm examples/calc.bnf > $@

gen/calc-flat-grm-dfa.dot: examples/calc.bnf yakker
	./yakker -flatten-full -earley-gen-grm-dot examples/calc.bnf > $@


#######################################
# Earley debugging targets (old, possibly broken)
######################################

CALC_CYC_FILES = $(EBE_CYC_FILES) calc-dfa parse-main
CALC_DFA_FILES = $(EBE_DFA_FILES) earley parse-main
CALC_CYC_OBJS:=$(foreach yfile,$(CALC_CYC_FILES),$(yfile).o)
CALC_DFA_OBJS:=$(foreach yfile,$(CALC_DFA_FILES),$(yfile).o)

calc-cyc-eb: $(CALC_CYC_OBJS)
	$(CYCLONE) -o $@ $^  -lssl -lm

calc-dfa-eb: $(CALC_DFA_OBJS)
	$(CYCLONE) -o $@ $^ -lssl -lm
	
calc-dfa.cyc: examples/calc.bnf yakker
	./yakker -earley-gen-cyc start $< > $@

calc-dfa.txt: examples/calc.bnf yakker
	./yakker -earley-gen-fsm start examples/calc.bnf > $@


