# Makefile for LWA ICD Documentation
# Usage:
#   make          - Build all documents
#   make clean    - Remove build artifacts
#   make distclean - Remove all generated files including PDFs

LATEX = pdflatex
LATEX_FLAGS = -interaction=nonstopmode -halt-on-error

# Documents
DR_ICD_DIR = DR_ICD
DR_ICD_DOC = DR_ICD

.PHONY: all clean distclean dr_icd

# Default target
all: dr_icd

# DP ICD document
dr_icd: $(DR_ICD_DOC).pdf

$(DR_ICD_DOC).pdf: $(DR_ICD_DIR)/lwa-icd.cls \
                    $(DR_ICD_DIR)/$(DR_ICD_DOC).tex \
                    $(wildcard $(DR_ICD_DIR)/config/*.tex) \
                    $(wildcard $(DR_ICD_DIR)/frontmatter/*.tex) \
                    $(wildcard $(DR_ICD_DIR)/sections/*.tex) \
                    $(wildcard $(DR_ICD_DIR)/appendices/*.tex)
	cd $(DR_ICD_DIR) && $(LATEX) $(LATEX_FLAGS) $(DR_ICD_DOC).tex
	cd $(DR_ICD_DIR) && $(LATEX) $(LATEX_FLAGS) $(DR_ICD_DOC).tex
	mv $(DR_ICD_DIR)/$(DR_ICD_DOC).pdf .

# Clean build artifacts
clean:
	cd $(DR_ICD_DIR) && rm -f *.aux *.log *.out *.toc *.lof *.lot *.fls *.fdb_latexmk *.synctex.gz
	cd $(DR_ICD_DIR) && rm -f sections/*.aux appendices/*.aux frontmatter/*.aux config/*.aux

# Remove all generated files including PDFs
distclean: clean
	rm -f $(DR_ICD_DOC).pdf

# Help
help:
	@echo "LWA ICD Documentation Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build all documents (default)"
	@echo "  dr_icd    - Build DP ICD document"
	@echo "  clean      - Remove build artifacts"
	@echo "  distclean  - Remove all generated files including PDFs"
	@echo "  help       - Show this help message"
