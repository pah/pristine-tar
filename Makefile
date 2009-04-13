test:
	@rm -rf empty
	@for f in knownproblems/*.gz; do \
		echo "$$f: "; \
		pristine-gz gendelta "$$f" $$f.delta 2>&1 | sed 's/^/    /'; \
	done

testexternal:
	@if [ ! -e tars ]; then echo "Create a tars file listing tarballs to test"; false; fi
	mkdir -p failures
	rm -rf workdir
	top=`pwd`; \
	for f in $$(cat tars); do \
		echo $$f; \
		base=$$(basename $$f); \
		if ! pristine-tar gendelta $$f $$base.delta; then \
			cp $$f failures; \
		fi; \
		mkdir workdir; \
		tar xf $$f -C workdir; \
		cd workdir; cd * >/dev/null 2>&1; \
		if ! pristine-tar gentar $$top/$$base.delta $$top/$$base; then \
			cp $$f $$top/failures; \
		fi; \
		cd $$top; \
		if ! cmp $$base $$f; then \
			cp $$f failures; \
		fi; \
		rm -f $$base; \
		rm -rf workdir; \
	done
	echo "done; check failures directory for problem tars"

clean:
	rm -rf *.delta failures
