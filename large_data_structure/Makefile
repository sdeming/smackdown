OCAMLC=ocamlopt
OCAMLCFLAGS=-unsafe
LIBS=str.cmxa unix.cmxa

lds: lds.ml
	$(OCAMLC) $(OCAMLCFLAGS) -o lds $(LIBS) lds.ml

clean:
	rm -f lds lds.cmi lds.cmo lds.cmx lds.o >/dev/null 2>&1
