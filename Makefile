test:
	@rm -rf empty
	@for f in knownproblems/*.gz; do \
		echo "$$f: "; \
		pristine-gz gendelta "$$f" $$f.delta 2>&1 | sed 's/^/    /'; \
	done

testexternal:
	@if [ ! -e tars ]; then echo "Create a tars file listing tarballs to test"; false; fi
	rm -rf workdir failures
	for f in $$(cat tars); do \
		./testone $f
	done
	echo "done; check failures directory for problem tars"

clean:
	rm -rf *.delta failures
