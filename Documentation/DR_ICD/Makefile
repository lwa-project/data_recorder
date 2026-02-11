# Makefile for DR ICD document

MAIN = DR_ICD
LATEX = pdflatex
BIBTEX = bibtex

.PHONY: all clean distclean

all: $(MAIN).pdf

$(MAIN).pdf: $(MAIN).tex lwa-icd.cls config/*.tex frontmatter/*.tex sections/*.tex appendices/*.tex figures/logo.png
	$(LATEX) $(MAIN)
	$(LATEX) $(MAIN)
	$(LATEX) $(MAIN)

clean:
	rm -f $(MAIN).aux $(MAIN).log $(MAIN).toc $(MAIN).out $(MAIN).lot
	rm -f $(MAIN).lof $(MAIN).bbl $(MAIN).blg
	rm -f sections/*.aux appendices/*.aux frontmatter/*.aux config/*.aux

distclean: clean
	rm -f $(MAIN).pdf
