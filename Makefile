.PHONY: run clean

tw: tw.ml
	ocamlopt -o $@ $<

run:
	ocaml tw.ml

clean:
	rm -rf tw tw.cmi tw.cmx tw.o
