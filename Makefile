test:
# We use an empty directory rather than botherin uncompressing the tarball,
# because it really shouldn't matter; pristine-tar will make a larger
# delta, but should still work, and pristine-gz is what really fails,
# and really is tested here.
	@rm -rf empty
	@mkdir empty
	@for f in *.gz; do \
		cd empty; \
		printf "$$f: "; \
		if ! pristine-tar gendelta "../$$f" delta >/dev/null 2>&1; then \
			echo "failed to reproduce"; \
		else \
			echo "succeeded to reproduce"; \
		fi; \
		cd .. ; \
	done
	@rm -rf empty
