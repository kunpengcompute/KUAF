ACLOCAL_AMFLAGS = -I m4

if HAVE_CRYPTO
if HAVE_WD
SUBDIRS = src
endif
endif

if HAVE_CRYPTO3
if HAVE_WD
SUBDIRS = src
endif
endif

if COVER
cover: all
	-lcov --capture --directory src --output-file coverage.info
	-genhtml coverage.info --output-directory coverage_report
	@echo "Coverage report: file://$(CURDIR)/coverage_report/index.html"

clean-coverage:
	-rm -rf coverage.info coverage_report
	-find . -name "*.gcda" -o -name "*.gcno" | xargs rm -f

clean-local: clean-coverage
endif

.PHONY: cover clean-coverage