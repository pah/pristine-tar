test:
	@rm -rf empty
	@for f in *.gz; do \
		echo "$$f: "; \
		pristine-gz gendelta "$$f" $$f.delta 2>&1 | sed 's/^/    /'; \
	done

clean:
	rm -rf *.delta
