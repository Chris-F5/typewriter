.PHONY: toplevel clean

tw: tw.ml
	ocamlfind ocamlopt -linkpkg -package camlpdf -o $@ $<

toplevel:


clean:
	rm -rf tw tw.cmi tw.cmx tw.o
